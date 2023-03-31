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

EXE = mrefd

include $(EXE).mk

ifeq ($(USESYMLINK), true)
CPORLN = ln -s
else
CPORLN = cp -f
endif

CFLAGS += -c -W -std=c++17 -MMD -c
LDFLAGS=-pthread

ifeq ($(DEBUG), true)
CFLAGS += -ggdb3
endif

ifeq ($(DHT), true)
LDFLAGS += -lopendht -lcurl
else
CFLAGS += -DNO_DHT
endif

ifeq ("$(OS)","Windows_NT")
CFLAGS += -D_GNU_SOURCE
endif

SRCS = $(wildcard *.cpp)
OBJS = $(SRCS:.cpp=.o)
DEPS = $(SRCS:.cpp=.d)

all : $(EXE)

$(EXE) : $(OBJS)
	$(CXX) $^ -o $@ $(LDFLAGS)

%.o : %.cpp
	$(CXX) $(CFLAGS) $< -o $@

clean :
	$(RM) *.o *.d $(EXE) test-all

-include $(DEPS)

install : $(EXE).blacklist $(EXE).whitelist $(EXE).interlink $(EXE).cfg
	$(CPORLN) $(shell pwd)/$(EXE).blacklist $(CFGDIR)/$(EXE).blacklist
	$(CPORLN) $(shell pwd)/$(EXE).whitelist $(CFGDIR)/$(EXE).whitelist
	$(CPORLN) $(shell pwd)/$(EXE).interlink $(CFGDIR)/$(EXE).interlink
	$(CPORLN) $(shell pwd)/$(EXE).cfg $(CFGDIR)/$(EXE).cfg
	sed -e "s#XXX#$(CFGDIR)#" -e "s#YYY#$(EXE)#" systemd/mrefd.service > /etc/systemd/system/$(EXE).service
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
