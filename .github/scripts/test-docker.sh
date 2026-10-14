#!/bin/bash
# Smoke test the docker compose stack set up as the README describes.
# The test reflector bootstraps from itself, so it never joins the Ham-DHT.
set -euo pipefail

cd "$(dirname "$0")/../.."
work=$(mktemp -d)
compose=(docker compose -p mrefd-test --env-file .env.example --env-file "$work/test.env")

fail() {
	echo "FAIL: $*" >&2
	exit 1
}

cleanup() {
	status=$?
	if [ "$status" -ne 0 ]; then
		"${compose[@]}" logs || true
	fi
	"${compose[@]}" down -v || true
	rm -rf "$work"
	exit "$status"
}
trap cleanup EXIT

# The README's mrefd.cfg edits
cp config/mrefd.blacklist config/mrefd.whitelist config/mrefd.interlink "$work/"
sed -e 's/^Callsign = .*/Callsign = M17-TST/' \
	-e 's#^PidPath = .*#PidPath = /app/data/mrefd.pid#' \
	-e 's#^JsonPath = .*#JsonPath = /app/data/mrefd.json#' \
	-e 's#^WhitelistPath = .*#WhitelistPath = /app/config/mrefd.whitelist#' \
	-e 's#^BlacklistPath = .*#BlacklistPath = /app/config/mrefd.blacklist#' \
	-e 's#^InterlinkPath = .*#InterlinkPath = /app/config/mrefd.interlink#' \
	-e 's#^DHTSavePath = .*#DHTSavePath = /app/data/mrefd.dht.bin#' \
	-e 's#^Bootstrap = .*#Bootstrap = 127.0.0.1#' \
	example.cfg > "$work/mrefd.cfg"
for key in PidPath JsonPath WhitelistPath BlacklistPath InterlinkPath DHTSavePath; do
	grep -q "^$key = /app/" "$work/mrefd.cfg" || fail "example.cfg has no $key; update the README"
done

# The README's config.inc.php edits
sed -e 's#^date_default_timezone_set(trim.*#date_default_timezone_set("UTC");#' \
	-e "s#'/var/run/mrefd.pid'#'/app/data/mrefd.pid'#" \
	-e "s#'/var/log/mrefd.json'#'/app/data/mrefd.json'#" \
	php-dash/include/config.inc.php.dist > "$work/config.inc.php"
grep -q "'JsonFile'.*'/app/data/mrefd.json'" "$work/config.inc.php" || fail "config.inc.php.dist has no JsonFile; update the README"

cat > "$work/test.env" <<EOF
CONFIG_PATH=$work/mrefd.cfg
BLACKLIST_PATH=$work/mrefd.blacklist
WHITELIST_PATH=$work/mrefd.whitelist
INTERLINK_PATH=$work/mrefd.interlink
DASH_CONFIG_PATH=$work/config.inc.php
HOST_M17_PORT=27000
HOST_DHT_PORT=27171
HOST_DASHBOARD_PORT=28080
EOF

"${compose[@]}" up -d --build

for _ in $(seq 60); do
	"${compose[@]}" logs mrefd | grep -q 'started and listening' && break
	sleep 2
done
"${compose[@]}" logs mrefd | grep -q 'started and listening' || fail "mrefd did not start"

user=$(docker inspect -f '{{.Config.User}}' "$("${compose[@]}" ps -q mrefd)")
[ "$user" = 65532 ] || fail "mrefd runs as '$user', not nonroot"

# A module that exists is ACKed, one that doesn't is NACKed
python3 - <<'EOF'
import socket

chars = " ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-/."
value = 0
for c in reversed("N0CALL"):
    value = value * 40 + chars.index(c)
callsign = value.to_bytes(6, "big")

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.settimeout(5)
for module, expected in ((b"Z", b"NACK"), (b"B", b"ACKN")):
    sock.sendto(b"CONN" + callsign + module, ("127.0.0.1", 27000))
    reply = sock.recvfrom(64)[0][:4]
    assert reply == expected, f"module {module}: got {reply}, expected {expected}"
sock.sendto(b"DISC" + callsign, ("127.0.0.1", 27000))
print("reflector answers CONN")
EOF

for page in users links peers; do
	curl -fsS --retry 10 --retry-connrefused --retry-delay 1 "http://localhost:28080/index.php?show=$page"
done > "$work/pages.html"
grep -q 'M17-TST' "$work/pages.html" || fail "dashboard does not show the reflector"
if grep -q -i -E 'Warning:|Deprecated:|Notice:|Fatal error' "$work/pages.html" ||
	"${compose[@]}" logs mrefd_dashboard | grep -q -i -E 'PHP (Warning|Deprecated|Notice|Fatal)'; then
	fail "dashboard has PHP errors"
fi

echo "docker smoke test passed"
