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

CFLAGS += -c -W -std=c++11 -MMD -MD -c

ifeq ($(debug), true)
CFLAGS += -ggdb3
endif

ifeq ("$(OS)","Windows_NT")
CFLAGS += -D_GNU_SOURCE
endif

LDFLAGS=-pthread

SRCS = base.cpp bwset.cpp callsign.cpp client.cpp clients.cpp crc.cpp gatekeeper.cpp ip.cpp m17client.cpp m17peer.cpp m17protocol.cpp notification.cpp packet.cpp packetstream.cpp peer.cpp peermap.cpp peermapitem.cpp peers.cpp protocol.cpp reflector.cpp timepoint.cpp udpsocket.cpp user.cpp users.cpp version.cpp main.cpp

LSTS = $(addprefix config/$(EXE).,blacklist whitelist interlink)

OBJS = $(SRCS:.cpp=.o)
DEPS = $(SRCS:.cpp=.d)

EXE=mrefd

all : $(EXE) test-all

test-all : test-all.o crc.o callsign.o
	$(CXX) $^ -o $@

$(EXE) : $(OBJS)
	$(CXX) $^ -o $@ $(LDFLAGS)

%.o : %.cpp
	$(CXX) $(CFLAGS) $< -o $@

clean :
	$(RM) *.o *.d $(EXE) test-all

-include $(DEPS)

install : $(LSTS)
	if [ -e $(CFGDIR)/$(EXE).blacklist ]; then \
		mv -f $(CFGDIR)/$(EXE).blacklist{,.bak}; \
	fi
	if [ -e $(CFGDIR)/$(EXE).whitelist ]; then \
		mv -f $(CFGDIR)/$(EXE).whitelist{,.bak}; \
	fi
	if [ -e $(CFGDIR)/$(EXE).interlink ]; then \
		mv -f $(CFGDIR)/$(EXE).interlink{,.bak}; \
	fi
	cp -f $(LSTS) $(CFGDIR)
	cp -f $(EXE) $(BINDIR)
	mkdir -p $(DATADIR)
	if [ -e /etc/systemd ]; then \
		cp -f systemd/$(EXE).service /etc/systemd/system/; \
		systemctl enable $(EXE).service; \
		systemctl daemon-reload; \
		systemctl start $(EXE).service; \
	fi

uninstall :
	if [ -e /etc/systemd ]; then \
		systemctl stop $(EXE).service; \
		systemctl disable $(EXE).service; \
		$(RM) /etc/systemd/system/$(EXE).service; \
		systemctl daemon-reload; \
	fi
	$(RM) $(BINDIR)/$(EXE)
	$(RM) $(CFGDIR)/$(EXE).blacklist{,.bak}
	$(RM) $(CFGDIR)/$(EXE).whitelist{,.bak}
	$(RM) $(CFGDIR)/$(EXE).interlink{,.bak}
	$(RM) $(CFGDIR)/dmrid.dat
