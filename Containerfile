FROM debian:12.11-slim

# Install dependencies
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
    git build-essential g++ libcurl4-gnutls-dev libopendht-dev \
    && rm -rf /var/lib/apt/lists/*

# Set the working directory
WORKDIR /app
# Copy project to the build context
COPY . ./

# Copy default config files into app dir
RUN cp config/mrefd.blacklist . \
    && cp config/mrefd.whitelist . \
    && cp config/mrefd.interlink . \
    && cp example.mk mrefd.mk \
    && cp example.cfg mrefd.cfg

RUN make \
    && make install

EXPOSE 17000/udp 17171/udp

CMD [ "/usr/local/bin/mrefd", "/app/config/mrefd.cfg" ]
