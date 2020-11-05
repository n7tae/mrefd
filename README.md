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
sudo apt install apache2 php5
sudo apt install build-essential
sudo apt install g++
```

### Download the repository and enter the directory

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

The mrefd.interlink file sets up the M17<--->M17 peer group linking. Please read the comments in this file. An M17 interlink now has to be configured on both sides of the link. Linked reflectors can share multiple modules, but cross module linking, for example, linking M17-000 module A to M17-001 module B is not supported. Also group linking demands all reflector in a group are linked to all other reflectors in the group. This will result in the shortest possible latency between a client and any other client on the group. This XLX-like mode of linking is inforced by implementing a *one hop* policy where a voice stream is marked by a reflector when it is passed to another reflector. The receiving reflector will then know not to pass the voice stream on to any other reflector.

A reflector can be a member of more that one group, but you should never configure reflector module to be more than in a single group.

Group adminstration will require coordination among the admins of all involved reflectors. If a group memeber drops out or if a new member wants to join a group, all other group members will need to remove or add a line to their mrefd.interlink file.

Finally, if both ends of a link in a group support IPv6, that part of the group can use IPv6 while the rest of the group links can use IPv4.

### Configuring your reflector

Configuring, compiling and maintaining your reflector build is easy! Start the configuration script in the base directory of you cloned repo:

```bash
./rconfig
```

There are only a few things that need to be specified. Most important are, the reflector callsign and the IP addresses for the IPv4 and IPv6 listen ports. The reflector callsign must be exactly 7 characters beginning with "M17-". The remaining 3 characters can be an combination of numbers or letters. Input will automatically be capitolized by rconfig.

Dual-stack operation is enabled by specifying both an IPv4 and IPv6 address. IPv4-only single stack can be specified by leaving the IPv6 address set to `none`. It's even possible to operate in an IPv6-only configuration by leaving the IPv4 address to the default `none`.

Be sure to write out the configuration files and look at the three different configration files that are created. The first file, reflector.cfg is the memory file for rconfig so that if you start that script again, it will remember how you left things. There is one `.h` file and one `.mk` file for the reflector. You should **not** modify these files by hand unless you really know exactly how they work.

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

### Copy dashboard to /var/www

The official M17 dashboard is in another repo.

```bash
git clone git://github.com/M17-project/ref-dash /var/www/html     # or where ever you system www root is located
```

Follow the instructions on that repo for configuring your dashboard.

## Updating mrefd

Here is the safest way to update your reflector:

```bash
sudo make uninstall
make clean
git pull
make
sudo make install
```

If, after doing the `git pull`, you see that it's downloaded a new rconfig script, it would probably be a good idea to run it before doing a `make`. There may be new build options of which you might want to take advantage.

## Firewall settings

MREFD requires the following port to be open to inbound network traffic:

- UDP port 17000 for M17
- TCP port 80 for HTTP
- TCP port 443 for HTTPS

## Copyright

- Copyright © 2020 Thomas A. Early N7TAE
