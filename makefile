CXXFLAGS = -O2 -Wall
APP = numptyphysics

DESTDIR ?=
PREFIX = /opt/numptyphysics

CXXFLAGS += -DINSTALL_BASE_PATH=\"$(PREFIX)/data\"

SOURCES = $(wildcard *.cpp)
SOURCES_TEST = $(wildcard test/*.cpp)

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
OBJECTS_TEST = $(SOURCES_TEST:.cpp=.o) $(OBJECTS) $(OBJECTS_OS_TEST)

Dialogs.cpp: help_text_html.h

%_html.h: %.html
	xxd -i $< $@

$(APP): $(OBJECTS) $(OBJECTS_OS) $(BOX2D_SOURCE)/$(BOX2D_LIBRARY)
	$(CXX) -o $@ $^ $(LIBS)

check: tester
	./tester

gtest-all.o: $(GTEST_DIR)/src/gtest-all.cc
	$(CXX) -c -I$(GTEST_DIR) -o $@ $^ $(LIBS) -lpthread
	
gtest_main.o: $(GTEST_DIR)/src/gtest_main.cc
	$(CXX) -c -I$(GTEST_DIR) -o $@ $^ $(LIBS) -lpthread

$(OBJECTS_OS_TEST): $(SOURCES_OS)
	$(CXX) $(CXXFLAGS) -DTEST -c -o $@ $^

tester: $(OBJECTS_TEST) $(BOX2D_SOURCE)/$(BOX2D_LIBRARY) gtest-all.o gtest_main.o
	$(CXX) -o $@ $^ $(LIBS) -lpthread
	
clean:
	rm -f $(OBJECTS) $(OBJECTS_OS) $(OBJECTS_TEST) $(OBJECTS_OS_TEST)
	rm -f $(DEPENDENCIES)
	rm -f help_text_html.h
	$(MAKE) -C Box2D/Source clean

distclean: clean
	rm -f $(APP)

install: $(APP)
	mkdir -p $(DESTDIR)/$(PREFIX)/bin
	install -m 755 $(APP) $(DESTDIR)/$(PREFIX)/bin/
	mkdir -p $(DESTDIR)/usr/share/applications
	install -m 644 $(APP).desktop $(DESTDIR)/usr/share/applications/
	mkdir -p $(DESTDIR)/$(PREFIX)/data
	cp -rpv data/*.png data/*.ttf data/*.npz $(DESTDIR)/$(PREFIX)/data/


.PHONY: all clean distclean
.DEFAULT: all

