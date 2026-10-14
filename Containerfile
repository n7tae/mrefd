FROM debian:13-slim AS build

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
# Copy project to the build context
COPY . ./

# The config files are mounted at /app/config at run time
RUN cp example.mk mrefd.mk \
    && make

# Collect the shared libraries mrefd links, except the C/C++ runtime that the
# distroless image already has, keeping their Debian paths
RUN ldd mrefd | awk '$2 == "=>" {print $3}' \
    | grep -v -E '/(libc|libm|libresolv|libgcc_s|libstdc\+\+)\.so' \
    | while read -r lib; do \
        dir="/runtime$(dirname "$(readlink -f "$lib")")"; \
        mkdir -p "$dir" && cp -L "$lib" "$dir/"; \
    done

# Distroless has glibc, CA certificates and tzdata, but no shell or package manager
FROM gcr.io/distroless/cc-debian13

COPY --from=build /runtime/ /
COPY --from=build /app/mrefd /usr/local/bin/mrefd

EXPOSE 17000/udp 17171/udp

CMD [ "/usr/local/bin/mrefd", "/app/config/mrefd.cfg" ]
