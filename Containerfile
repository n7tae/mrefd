FROM debian:12.11-slim

# Install dependencies
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
    ca-certificates git build-essential g++ libcurl4-gnutls-dev libopendht-dev nlohmann-json3-dev \
    && rm -rf /var/lib/apt/lists/*

# Set the working directory
WORKDIR /app
# Copy project to the build context
COPY . ./

# The config files are mounted at /app/config at run time
RUN cp example.mk mrefd.mk \
    && make \
    && cp mrefd /usr/local/bin/

EXPOSE 17000/udp 17171/udp

CMD [ "/usr/local/bin/mrefd", "/app/config/mrefd.cfg" ]
