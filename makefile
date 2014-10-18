CXXFLAGS = -O2 -Wall
APP = numptyphysics

DESTDIR ?=
PREFIX = /usr

# Standard directories
BINDIR = $(PREFIX)/bin
DATAROOTDIR = $(PREFIX)/share
DESKTOPDIR = $(DATAROOTDIR)/applications
MANDIR = $(DATAROOTDIR)/man

# App-specific directories
DATADIR = $(DATAROOTDIR)/$(APP)


CXXFLAGS += -DINSTALL_BASE_PATH=\"$(DATADIR)\"

SOURCES = $(wildcard *.cpp)

all: $(APP)

# Required modules (uses pkg-config)
PKGS = sdl SDL_image x11

CXXFLAGS += $(shell pkg-config --cflags $(PKGS))
LIBS += $(shell pkg-config --libs $(PKGS))

# No pkg-config module for SDL_ttf and zlib (on some systems)
LIBS += -lSDL_ttf -lz


# Box2D Library
CXXFLAGS += -IBox2D/Include
BOX2D_SOURCE := Box2D/Source
BOX2D_LIBRARY := Gen/float/libbox2d.a
LIBS += $(BOX2D_SOURCE)/$(BOX2D_LIBRARY)

$(BOX2D_SOURCE)/$(BOX2D_LIBRARY):
	$(MAKE) -C $(BOX2D_SOURCE) $(BOX2D_LIBRARY)


# Pick the right OS-specific module here
SOURCES += os/OsFreeDesktop.cpp
CXXFLAGS += -I.

# Dependency tracking
DEPENDENCIES = $(SOURCES:.cpp=.d)
CXXFLAGS += -MD
-include $(DEPENDENCIES)

OBJECTS = $(SOURCES:.cpp=.o)

Dialogs.cpp: help_text_html.h

%_html.h: %.html
	xxd -i $< $@

$(APP): $(OBJECTS) $(BOX2D_SOURCE)/$(BOX2D_LIBRARY)
	$(CXX) -o $@ $^ $(LIBS)


clean:
	rm -f $(OBJECTS)
	rm -f $(DEPENDENCIES)
	rm -f help_text_html.h
	$(MAKE) -C Box2D/Source clean

distclean: clean
	rm -f $(APP)

install: $(APP)
	mkdir -p $(DESTDIR)$(BINDIR)
	install -m 755 $(APP) $(DESTDIR)$(BINDIR)
	mkdir -p $(DESTDIR)$(DESKTOPDIR)
	install -m 644 $(APP).desktop $(DESTDIR)$(DESKTOPDIR)
	mkdir -p $(DESTDIR)$(DATADIR)
	cp -rpv data/*.png data/*.ttf data/*.npz $(DESTDIR)$(DATADIR)
	mkdir -p $(DESTDIR)$(MANDIR)/man6
	install -m 644 $(APP).6 $(DESTDIR)$(MANDIR)/man6


.PHONY: all clean distclean
.DEFAULT: all

