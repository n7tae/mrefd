# Distroless has glibc, CA certificates and tzdata, but no shell or package
# manager. The nonroot tag runs as uid 65532.
FROM gcr.io/distroless/cc-debian13:nonroot@sha256:83b9b50586253821cf63ffcec87d3d94265a093ad30da232039af962dd8f4edf AS runtime

FROM debian:13-slim@sha256:a29215f6a35e51e22adffa17f89e9d2ef06214e64a2bad10d765c46aea49f11f AS build

SHELL ["/bin/bash", "-eo", "pipefail", "-c"]

ARG OPENDHT_VERSION=v4.4.1

# Install build dependencies
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
    build-essential ca-certificates cmake git pkg-config \
    libargon2-dev libasio-dev libfmt-dev libgnutls28-dev libmsgpack-cxx-dev nettle-dev \
    libcurl4-gnutls-dev nlohmann-json3-dev \
    && rm -rf /var/lib/apt/lists/*

# Debian 13 has no libopendht-dev, so build it as the README describes
RUN git clone --depth 1 --branch "$OPENDHT_VERSION" https://github.com/savoirfairelinux/opendht.git /opendht \
    && cmake -S /opendht -B /opendht/build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr \
        -DOPENDHT_TOOLS=OFF -DOPENDHT_DOWNLOAD_DEPS=OFF -DBUILD_TESTING=OFF \
    && cmake --build /opendht/build -j"$(nproc)" \
    && cmake --install /opendht/build

# Set the working directory
WORKDIR /app
# Copy the sources (see .dockerignore)
COPY . ./

# The config files are mounted at /app/config at run time
RUN cp example.mk mrefd.mk \
    && make -j"$(nproc)"

# Collect the shared libraries mrefd links that the runtime image doesn't
# already have, keeping their Debian paths
COPY --from=runtime /usr/lib/ /base/usr/lib/
RUN mkdir -p /runtime /app/data \
    && ldd mrefd | awk '$2 == "=>" {print $3}' | while read -r lib; do \
        dir="$(dirname "$(readlink -f "$lib")")"; \
        [ -e "/base$dir/$(basename "$lib")" ] || { mkdir -p "/runtime$dir" && cp -L "$lib" "/runtime$dir/"; }; \
    done

FROM runtime

COPY --from=build /runtime/ /
COPY --from=build /app/mrefd /usr/local/bin/mrefd
# Owned by nonroot so a new volume mounted here is writable
COPY --from=build --chown=nonroot:nonroot /app/data /app/data

EXPOSE 17000/udp 17171/udp

ENTRYPOINT [ "/usr/local/bin/mrefd" ]
CMD [ "/app/config/mrefd.cfg" ]
