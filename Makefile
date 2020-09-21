# Copyright (c) 2020 by Thomas A. Early N7TAE
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# locations for the executibles and other files are set here
# NOTE: IF YOU CHANGE THESE, YOU WILL NEED TO UPDATE THE service.* FILES AND
# if you change these locations, make sure the sgs.service file is updated!
# you will also break hard coded paths in the dashboard file, index.php.

include configure.mk

# if you make changed in these two variable, you'll need to change things
# in the main.h file as well as the systemd service file.
BINDIR = /usr/local/bin
CFGDIR = /usr/local/etc
DATADIR = /var/lib/m17ref

CC = g++

ifeq ($(debug), true)
CFLAGS = -ggdb3 -W -c -std=c++11 -MMD -MD -c
else
CFLAGS = -c -W -std=c++11 -MMD -MD -c
endif

LDFLAGS=-pthread

SRCS = callsign.cpp callsignlist.cpp callsignlistitem.cpp client.cpp clients.cpp crc.cpp gatekeeper.cpp ip.cpp m17client.cpp m17peer.cpp m17protocol.cpp notification.cpp packetstream.cpp peer.cpp peers.cpp peercallsignlist.cpp protocol.cpp reflector.cpp timepoint.cpp udpsocket.cpp user.cpp users.cpp version.cpp main.cpp

OBJS = $(SRCS:.cpp=.o)
DEPS = $(SRCS:.cpp=.d)

EXE=mrefd

all : $(EXE)

$(EXE) : $(OBJS)
	$(CC) $^ -o $@ $(LDFLAGS)

%.o : %.cpp
	g++ $(CFLAGS) $< -o $@

clean :
	$(RM) *.o *.d $(EXE)

-include $(DEPS)

install :
	ln -s $(shell pwd)/$(EXE).blacklist $(CFGDIR)/$(EXE).blacklist
	ln -s $(shell pwd)/$(EXE).whitelist $(CFGDIR)/$(EXE).whitelist
	ln -s $(shell pwd)/$(EXE).interlink $(CFGDIR)/$(EXE).interlink
	cp -f ../systemd/$(EXE).service /etc/systemd/system/
	cp -f $(EXE) $(BINDIR)
	mkdir -p $(DATADIR)
	systemctl enable $(EXE).service
	systemctl daemon-reload
	systemctl start $(EXE)

uninstall :
	rm -f $(CFGDIR)/$(EXE).blacklist
	rm -f $(CFGDIR)/$(EXE).whitelist
	rm -f $(CFGDIR)/$(EXE).interlink
	systemctl stop $(EXE).service
	rm -f $(CFGDIR)/dmrid.dat
	systemctl disable $(EXE).service
	rm -f /etc/systemd/system/$(EXE).service
	systemctl daemon-reload
