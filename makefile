CXXFLAGS = -g -O2 -Wall 

APP = numptyphysics

DESTDIR ?=
PREFIX = /usr

# Standard directories
BINDIR = $(PREFIX)/bin
DATAROOTDIR = $(PREFIX)/share
DESKTOPDIR = $(DATAROOTDIR)/applications
MANDIR = $(DATAROOTDIR)/man
PIXMAPDIR = $(DATAROOTDIR)/pixmaps

# App-specific directories
DATADIR = $(DATAROOTDIR)/$(APP)


CXXFLAGS += -DINSTALL_BASE_PATH=\"$(DATADIR)\"

SOURCES = $(wildcard *.cpp)
SOURCES_TEST = $(wildcard test/*.cpp)

all: $(APP)

# Required modules (uses pkg-config)
PKGS = box2d sdl SDL_image x11

CXXFLAGS += $(shell pkg-config --cflags $(PKGS))
LIBS += $(shell pkg-config --libs $(PKGS))

# No pkg-config module for SDL_ttf and zlib (on some systems)
LIBS += -lSDL_ttf -lz

	
GTEST_DIR = /usr/src/gtest
	
# Pick the right OS-specific module here
SOURCES_OS = os/OsFreeDesktop.cpp
CXXFLAGS += -I.

# Dependency tracking
DEPENDENCIES = $(SOURCES:.cpp=.d) $(SOURCES_TEST:.cpp=.d)
CXXFLAGS += -MD
-include $(DEPENDENCIES)

OBJECTS = $(SOURCES:.cpp=.o)
OBJECTS_OS = $(SOURCES_OS:.cpp=.o)
OBJECTS_OS_TEST = $(subst os,test,$(OBJECTS_OS))
OBJECTS_TEST = $(SOURCES_TEST:.cpp=.o) $(OBJECTS) $(OBJECTS_OS_TEST) gtest-all.o gtest_main.o

Dialogs.cpp: help_text_html.h

%_html.h: %.html
	xxd -i $< $@

$(APP): $(OBJECTS) $(OBJECTS_OS)
	$(CXX) -o $@ $^ $(LIBS)

check: tester
	./tester

gtest-all.o: $(GTEST_DIR)/src/gtest-all.cc
	$(CXX) -c -I$(GTEST_DIR) -o $@ $^ $(LIBS) -lpthread
	
gtest_main.o: $(GTEST_DIR)/src/gtest_main.cc
	$(CXX) -c -I$(GTEST_DIR) -o $@ $^ $(LIBS) -lpthread

$(OBJECTS_OS_TEST): $(SOURCES_OS)
	$(CXX) $(CXXFLAGS) -DTEST -c -o $@ $^

tester: $(OBJECTS_TEST)
	$(CXX) -o $@ $^ $(LIBS) -lpthread
	
clean:
	rm -f $(OBJECTS) $(OBJECTS_OS) $(OBJECTS_TEST) $(OBJECTS_OS_TEST)
	rm -f $(DEPENDENCIES)
	rm -f help_text_html.h

distclean: clean
	rm -f $(APP) tester

install: $(APP)
	mkdir -p $(DESTDIR)$(BINDIR)
	install -m 755 $(APP) $(DESTDIR)$(BINDIR)
	mkdir -p $(DESTDIR)$(DESKTOPDIR)
	install -m 644 $(APP).desktop $(DESTDIR)$(DESKTOPDIR)
	mkdir -p $(DESTDIR)$(DATADIR)
	cp -rpv data/*.png data/*.ttf data/*.npz $(DESTDIR)$(DATADIR)
	mkdir -p $(DESTDIR)$(MANDIR)/man6
	install -m 644 $(APP).6 $(DESTDIR)$(MANDIR)/man6
	mkdir -p $(DESTDIR)$(PIXMAPDIR)
	install -m 644 data/numptyphysics.png $(DESTDIR)$(PIXMAPDIR)


.PHONY: all clean distclean
.DEFAULT: all

