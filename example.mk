# mrefd compile-time configuration

# This is for the "install" target:
# If you don't like to use symbolic links for the mrefd.{cfg,blacklist,whitelist,interlink}
# files and you want to copy instead of link set this to "false".
USESYMLINK = true

# This is the folder where the executable file will be installed.
BINDIR = /usr/local/bin

# This is the folder where config files (or symbolic links) are located.
CFGDIR = /usr/local/etc

# For debugging support, set DEBUG to true.
DEBUG = false

# To disable DHT support, set DHT to false.
DHT = true
