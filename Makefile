# Copyright (c) 2022 by Thomas A. Early N7TAE
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

# if you make changed in these two variable, you'll need to change things
# in the main.h file as well as the systemd service file.
BINDIR = /usr/local/bin
CFGDIR = /usr/local/etc

CFLAGS += -c -W -std=c++17 -MMD -c

ifneq (,$(wildcard debug))
CFLAGS += -ggdb3
endif

ifeq ("$(OS)","Windows_NT")
CFLAGS += -D_GNU_SOURCE
endif

LDFLAGS=-pthread -lopendht

SRCS = base.cpp bwset.cpp callsign.cpp client.cpp clients.cpp configure.cpp crc.cpp gatekeeper.cpp ip.cpp notification.cpp packet.cpp packetstream.cpp peer.cpp peermap.cpp peermapitem.cpp peers.cpp protocol.cpp reflector.cpp udpsocket.cpp user.cpp users.cpp version.cpp main.cpp

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

install : $(EXE).blacklist $(EXE).whitelist $(EXE).interlink $(EXE).cfg
	ln -s $(shell pwd)/$(EXE).blacklist $(CFGDIR)/$(EXE).blacklist
	ln -s $(shell pwd)/$(EXE).whitelist $(CFGDIR)/$(EXE).whitelist
	ln -s $(shell pwd)/$(EXE).interlink $(CFGDIR)/$(EXE).interlink
	ln -s $(shell pwd)/$(EXE).cfg $(CFGDIR)/$(EXE).cfg
	cp -f systemd/$(EXE).service /etc/systemd/system/
	cp -f $(EXE) $(BINDIR)
	systemctl enable $(EXE).service
	systemctl daemon-reload
	systemctl start $(EXE)

uninstall :
	rm -f $(CFGDIR)/$(EXE).blacklist
	rm -f $(CFGDIR)/$(EXE).whitelist
	rm -f $(CFGDIR)/$(EXE).interlink
	rm -f $(CFGDIR)/$(EXE).cfg
	systemctl stop $(EXE).service
	systemctl disable $(EXE).service
	rm -f /etc/systemd/system/$(EXE).service
	systemctl daemon-reload
