# MREFD

MREFD is a new M17 open source Reflector. Most of the code is originally based on groundbreaking development of XLXD and the copyrights of all relevent source files reflect this. The sources are published under GPL Licenses.

## Introduction

This is prototype software. Currently, this totally ignores, and in fact assumes, that no client is using M17 encrytion. Incoming M17 voice streams are readdressed to each client by rewriting the destination callsign and then recalculating the CRC for each packet.

Only systemd-based operating systems are supported. Debian or Ubuntu is recommended. If you want to install this on a non-systemd based OS, you are on your own. Also, by default, mrefd is built without gdb support. Finally, this repository is designed so that you don't have to modify any file in the repository when you build your system. Any file you need to modify to properly configure your reflector will be a file you copy from you locally cloned repo. This makes it easier to update the source code when this repository is updated. Follow the instructions below to build your M17 reflector.

## Usage

The packages which are described in this document are designed to install server software which is used for the digital voice network infrastructure. It requires a 24/7 internet connection which can support 20 voice streams or more to connect repeaters and hot-spots and other digitial voice clients!

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
sudo apt install git
sudo apt install build-essential
sudo apt install g++
```

OpenDHT and gomrefdash both have their own requirements.

### Distributed Hash Table (OpenDHT)

OpenDHT is available [here](https://github./com/savoirfairelinux/opendht.git). Building and installing instructions are in the [OpenDHT Wiki](https://github.com/savoirfairelinux/opendht/wiki/Build-the-library). Pascal support and proxy-server support (RESTinio) is not required for mrefd and so can be considered optional. With this in mind, this should work on Debian/Ubuntu-based systems:

```bash
# Install OpenDHT dependencies
sudo apt install libncurses5-dev libreadline-dev nettle-dev libgnutls28-dev libargon2-0-dev libmsgpack-dev  libssl-dev libfmt-dev libjsoncpp-dev libhttp-parser-dev libasio-dev cmake

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

The mrefd.interlink file sets up the M17<--->M17 peer group linking. Please read the comments in this file. An M17 interlink now has to be configured on both sides of the link. Linked reflectors can share multiple modules, but cross module linking, for example, linking M17-000 module A to M17-001 module B is not supported. Also group linking demands all reflector in a group are linked to all other reflectors in the group. This will result in the shortest possible latency between a client and any other client on the group. This XLX-like mode of linking is enforced by implementing a *one hop* policy where a voice stream is marked by a reflector when it is passed to another reflector. The receiving reflector will then know not to pass the voice stream on to any other reflector.

Group adminstration will require coordination among the admins of all involved reflectors. If a group memeber drops out or if a new member wants to join a group, all other group members will need to remove or add a line to their mrefd.interlink file.

Finally, if both ends of a link in a group support IPv6, that part of the group can use IPv6 while the rest of the group can use IPv4.

### Configuring your reflector

Configuring, compiling and maintaining your reflector build is easy! Start by copying the configuration example:

```bash
cp example.cfg mrefd.cfg
```

Use your favorite editor to edit mrefd.cfg. This file contains comments so it should be obvious what values need changing.

### Compile-time options

There are two compile-time options:

1. You can build mrefd with debugging support simply by creating an empty file, `touch debug`. Turn off debugging support with `rm debug`.
2. You can build mrefd wihtout DHT support by creating an empty file, `touch nodht`. Enable DHT with `rm nodht`. Of course, you don't need to build the OpenDHT library if you aren't going to use it. Keep in mind that registering your reflector with the M17 project will require DHT support.

If you turn one of these compile-time features off or on, you need to do a `make clean` and `make` after changing this feature. If mrefd is running, you also need to uninstall/reinstall mrefd.

### Compling and installing your system

After you have written your configutation files, you can build and install your system:

```bash
make && sudo make install
```

Use this command to compile and install your system. Use `sudo make uninstall` to uninstall mrefd.

### Stoping and starting the service manually

To stop the execution of mrefd:

```bash
sudo systemctl stop mrefd
```

You can start mrefd by replacing `stop` with `start`, or you can restart it by using `restart`.

To view the mrefd log:

```bash
sudo journalctl -u mrefd -f
```

The blacklist, whitelist and interlink files can be modified in real time while xrfd is executing and the reflector will update itself within a few seconds. Edit the files in build directory. While mrefd accesses the configuration files in /usr/local/etc, these files are symbolically linked back to your build directory.

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

It should be fairly straightforward to install multiple mrefd instances on a single server. Make sure each instance is using a different listening port. Things to me aware of:

- The reflector executable name is defined in the Makefile at the line "EXE=mrefd".
- The configuration file is read at start-up and defined in mrefd.service file. Each instance would require a different service file.
- Each instance will need its own support files with unique paths to those files.
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
