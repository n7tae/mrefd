# MREFD

This *mrefd* repository builds a new kind of M17 open-source Reflector. Although *mrefd* has evolved, most of the code is originally based on groundbreaking development of XLXD and the copyrights of all relevant source files reflect this. The sources are published under GPL Licenses.

## Introduction

The *mrefd* reflector is for connecting M17 clients together. *mrefd* can be configured with up to 26 different channels. M17 clients (M17 repeaters, M17 hot-spots and other MREFD reflectors) can be linked to a channel. An incoming M17 voice stream from one of the clients will be heard by all the other clients.

Encrypted voice streams will pass through an *mrefd* channel, but **only** if they are configured for it.

*mrefd* uses **DVIN**, the *Digital Voice Information Network*. **DVIN** is a distributed hash table network for publishing and retrieving connection information. Each M17 client on the **DVIN** that can accept inbound connections publishes its connection information. Other M17 clients wishing to connect only need to know the callsign of the destination. The connecting information is available in the **DVIN**.

The dashboard for *mrefd* is available [here](https://github.com/kc1awv/gomrefdash.git).

Only systemd-based operating systems are supported. Debian or Ubuntu is recommended. If you want to install this on a non-systemd based OS, you are on your own. This repository is designed so that you don't have to modify any file in the repository when you build your system. Any file you need to modify to properly configure your reflector will be a file you copy from your locally cloned repo. This makes it easier to update the source code when this repository is updated. Follow the instructions below to build your M17 reflector.

## A few words about valid callsign secondary suffixes

There are two callsign secondary suffixes allowed by *mrefd*. Either must follow the client station's callsign immediately (without any space between the callsign and the secondary suffix). The first, usually used to differentiate multiple stations, begins with a dash, "-", followed by a single alphanumeric character. The second, usually used to indicate a temporary change in locations begins with a stroke, "/", and followed by up to three alphanumeric characters. Keep in mind that M17 callsigns are limited to nine characters and on some client software, the ninth character is reserved for a module. Because of the space limitation and the fact that the majority of hams have six character callsigns, *mrefd* will reject connections from callsigns using *both* the dash and stroke secondary suffixes.

## Usage

The packages which are described in this document are designed to install server software which is used for the digital voice network infrastructure. It requires a 24/7 internet connection which can support 20 voice streams or more to connect repeaters and hot-spots and other digital voice clients!

- The server can build a reflector that support IPv4, IPv6 or both (dual stack).
- The public IP addresses should have a DNS record which must be published in the common host files.

## Installation

Below are instructions for building an mrefd reflector.

### After a clean installation of a Debian-based OS make sure to run update and upgrade

```bash
sudo apt update
sudo apt upgrade
```

### Required packages (some of these may already be installed)

```bash
sudo apt install git build-essential g++ libcurl4-gnutls-dev
```

The `libcurl4-gnutls-dev` package is only necessary if you use OpenDHT. OpenDHT and gomrefdash both have their own requirements.

### DVIN support (optional, but highly recommended)

**DVIN** is implemented using a distributed hash table provided by OpenDHT.

OpenDHT is available [here](https://github./com/savoirfairelinux/opendht.git). Building and installing instructions are in the [OpenDHT Wiki](https://github.com/savoirfairelinux/opendht/wiki/Build-the-library). Pascal support and proxy-server support (RESTinio) is not required for mrefd and so can be considered optional. With this in mind, this should work on Debian/Ubuntu-based systems:

```bash
# Install OpenDHT dependencies
sudo apt install libncurses5-dev libreadline-dev nettle-dev libgnutls28-dev libargon2-0-dev libmsgpack-dev  libssl-dev libfmt-dev libjsoncpp-dev libhttp-parser-dev libasio-dev cmake pkg-config

# clone the repo
git clone https://github.com/savoirfairelinux/opendht.git

# build and install
cd opendht
mkdir build && cd build
cmake -DOPENDHT_PYTHON=OFF -DCMAKE_INSTALL_PREFIX=/usr ..
make
sudo make install
```

Please note that there is no easy way to uninstall OpenDHT once it's been installed.

### Download the *mrefd* repository and enter the directory

```bash
git clone https://github.com/n7tae/mrefd.git
cd mrefd
```

### Create and edit your blacklist, whitelist and linking files

```bash
cp config/mrefd.blacklist .
cp config/mrefd.whitelist .
cp config/mrefd.interlink .
```

Use your favorite editor to modify each of these files. If you want a totally open network, the blacklist and whitelist files are ready to go. The blacklist determine which callsigns can't use the reflector. The whitelist determines which callsigns can use the reflector. The mrefd reflector will monitor these file and dynamically update itself whenever anything changes. There is no need to stop and restart the reflector.

The mrefd.interlink file sets up the M17<--->M17 peer group linking. Please read the comments in this file. An M17 interlink now has to be configured on both sides of the link. Linked reflectors can share multiple modules, but cross module linking, for example, linking M17-000 module A to M17-001 module B is not supported. Using the **DVIN** greatly simplifies setting up a peer group. You don't have to specify an IP address or port number as the **DVIN** will provide it, and this information comes directly from the peer. Using the **DVIN** will prevent you from interlinking a channel where the peer channel has encryption enabled and you do not, or *vice versa*.

If your reflector or your desired peer doesn't use **DVIN**, you can specify the IP address and port in the mrefd.interlink file. It will be then up to the reflector admins to make sure the encryption configurations match.

Inter-linking a channel to more than one other reflector demands all reflector in a group are linked to all other reflectors in the group. This will result in the shortest possible latency between a client and any other client on the group. This XLX-like mode of linking is enforced by implementing a *one hop* policy where a voice stream is marked by a reflector when it is passed to another reflector. The receiving reflector will then know not to pass the voice stream on to any other reflector.

Group administration will require coordination among the admins of all involved reflectors. If a group member drops out or if a new member wants to join a group, all other group members will need to remove or add a line to their mrefd.interlink file.

### Configuring your reflector

Configuring, compiling and maintaining your reflector build is easy! Start by copying the configuration example files:

```bash
cp example.mk mrefd.mk     # this  file  sets the compile-time configuration
cp example.cfg mrefd.cfg   # while this file sets the run-time configuration
```

Use your favorite editor to edit mrefd.mk and mrefd.cfg. These file contains comments so it should be obvious what values need changing.

### Compiling and installing your system

After you have written your configuration files, you can build and install your system:

```bash
make && sudo make install
```

Use this command to compile and install your system. Use `sudo make uninstall` to uninstall mrefd.

### Stopping and starting the service manually

To stop the execution of mrefd:

```bash
sudo systemctl stop mrefd
```

You can start mrefd by replacing `stop` with `start`, or you can restart it by using `restart`.

To view the mrefd log:

```bash
sudo journalctl -u mrefd -f
```

The blacklist, whitelist and interlink files can be modified in real time while xrfd is executing and the reflector will update itself within a few seconds. If `USESYMLINK` is true in your mrefd.mk file, then you can edit the files in your build directory. Otherwise, you will need to edit the files in the `CFGDIR` folder.

### Install the Dashboard

The official M17 dashboard is in another repo.

```bash
git clone https://github.com/kc1awv/gomrefdash.git
```

This a a dashboard based on the `go` programming language. If you don't have it, you will need to install it first. Follow the instructions on that repo for getting your dashboard up and running.

## Updating mrefd

Here is the safest way to update your reflector:

```bash
sudo make uninstall
make clean
git pull
make
sudo make install
```

If, after doing the `git pull`, you see that it's downloaded a new example.cfg file, it would probably be a good idea to remake your mrefd.cfg file. There may be new options of which you might want to take advantage.

## Running multiple instances

It should be fairly straightforward to install multiple mrefd instances on a single server. Make sure each instance is using a different listening port. Things to be aware of:

- The reflector executable name is defined in the Makefile at the line "EXE=mrefd".
- The configuration file is read at start-up and defined in mrefd.service file. Each instance would require a different service file.
- Each instance will need its own config files with unique paths to those files.
- Each instance will need a unique url so that each will have its own webpage.
- Need to add unix sockets for interlinking between instances.

This is **not** a task for a beginner!

## Firewall settings

MREFD requires the following port to be open to inbound network traffic:

- UDP port 17000 for M17
- UDP port 17171 for DHT
- TCP port 80 for HTTP
- TCP port 443 for HTTPS

## Copyright

- Copyright © 2020-2022 Thomas A. Early N7TAE
