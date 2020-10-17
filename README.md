# MREFD

MREFD is a new M17 open source Reflector. Most of the code is originally based on XLXD and the copyrights of all relevent source files reflect this. The sources are published under GPL Licenses.

## Introduction

This is prototype software. Currently, this totally ignores, and in fact assumes, that no client is using M17 encrytion. Incoming M17 voice streams are readdressed to each client by rewriting the destination callsign and then recalculating the CRC for each packet.

Only systemd-based operating systems are supported. Debian or Ubuntu is recommended. If you want to install this on a non-systemd based OS, you are on your own. Also, by default, mrefd is built without gdb support. Finally, this repository is designed so that you don't have to modify any file in the repository when you build your system. Any file you need to modify to properly configure your reflector will be a file you copy from you locally cloned repo. This makes it easier to update the source code when this repository is updated. Follow the instructions below to build your transcoding XLX reflector or tri-mode XRF reflector.

## Usage

The packages which are described in this document are designed to install server software which is used for the digital voice network infrastructure. It requires a 24/7 internet connection which can support 20 voice streams or more to connect repeaters and hot-spots and other digitial voice clients!

- The server can build a reflector that support IPv4, IPv6 or both (dual stack).
- The public IP addresses should have a DNS record which must be published in the common host files.

## Installation

Below are instructions for building either an mrefd reflector.

### After a clean installation of Debian make sure to run update and upgrade

```bash
sudo apt update
sudo apt upgrade
```

### Required packages (some of these will probably already be installed)

```bash
sudo apt install git
sudo apt install apache2 php5
sudo apt install build-essential
sudo apt install g++
```

### Download the repository and enter the directory

```bash
git clone https://github.com/n7tae/mrefd.git
cd mrefdd
```

### Create and edit your blacklist, whitelist and linking files

```bash
cp ../config/mrefd.blacklist .
cp ../config/mrefd.whitelist .
cp ../config/mrefd.interlink .
```

Use your favorite editor to modify each of these files. If you want a totally open network, the blacklist and whitelist files are ready to go. The blacklist determine which callsigns can't use the reflector. The whitelist determines which callsigns can use the reflector. The interlink file sets up the M17<--->M17 out-going M17 peer linking.

### Configuring your reflector

Configuring, compiling and maintaining your reflector build is easy! Start the configuration script in the base directory of you cloned repo:

```bash
./rconfig
```

There are only a few things that need to be specified. Most important are, the reflector callsign and the IP addresses for the IPv4 and IPv6 listen ports. The reflector callsign must be exactly 7 characters beginning with "M17-". The remaining 3 characters can be an combination of numbers or letters.

Dual-stack operation is enabled by specifying both an IPv4 and IPv6 address. IPv4-only single stack can be specified by leaving the IPv6 address set to `none`. It's even possible to operate in an IPv6-only configuration by leaving the IPv4 address to the default `none`.

Be sure to write out the configuration files and look at the three different configration files that are created. The first file, reflector.cfg is the memory file for rconfig so that if you start that script again, it will remember how you left things. There is one `.h` file and one `.mk` file for the reflector. You should **not** modify these files by hand unless you really know exactly how they work.

After the configuration files have been written, exit the rconfig script and type `make && sudo make install` and that should compile and start your reflector.

### Compling and installing your system

After you have written your configutation files, you can build and install your system:

```bash
./radmin
```

Use this command to compile and install your system. It can also be used to uninstall your system. It will use the information in reflector.cfg to perform each task. This radmin menu can also perform other tasks like restarting the reflector or transcoder process. It can also be used to update the software, if the system is uninstalled.

### Stoping and starting the service manually

```bash
sudo systemctl stop mrefd
```

You can start mrefd by replacing `stop` with `start`, or you can restart it by using `restart`.

To view the mrefd log:

```bash
sudo journalctl -u mrefd -f
```

The blacklist, whitelist and interlink files can be modified in real time while xrfd is executing and the reflector will update itself within a few seconds.

### Copy dashboard to /var/www

```bash
sudo cp -r ~/xlxd/dashboard /var/www/db     # or where ever you system www root is located
```

Please note that your www root directory might be some place else. There is one file that needs configuration. Edit the copied files, not the ones from the repository:

- **pgs/config.inc.php** - At a minimum set your email address, country and comment. **Do not** enable the calling home feature. This feature is not appropriate for mrefd.

## Updating mrefd

Start by first moving to the build directory and do a `git pull`. If any .h or .cpp fiiles have updates, you can do a `make && sudo make uninstall && sudo make install`.

## Firewall settings

MREFD requires the following port to be open and forwarded properly for network traffic:

- UDP port 17000

## Bugs

Reflector-to-reflector interlinking is not yet working. Don't even bother...

## Copyright

- Copyright © 2020 Thomas A. Early N7TAE
