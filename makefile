CXXFLAGS = -g -O0 -Wall -Wextra

APP = numptyphysics

DESTDIR ?=
PREFIX = /opt/numptyphysics

CXXFLAGS += -DINSTALL_BASE_PATH=\"$(PREFIX)/data\"

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
SOURCES += os/OsFreeDesktop.cpp
CXXFLAGS += -I.

# Dependency tracking
DEPENDENCIES = $(SOURCES:.cpp=.d) $(SOURCES_TEST:.cpp=.d)
CXXFLAGS += -MD
-include $(DEPENDENCIES)

OBJECTS = $(SOURCES:.cpp=.o)
OBJECTS_TEST = $(SOURCES_TEST:.cpp=.o) Path.o

Dialogs.cpp: help_text_html.h

%_html.h: %.html
	xxd -i $< $@

$(APP): $(OBJECTS)
	$(CXX) -o $@ $^ $(LIBS)

check: tester
	./tester

gtest-all.o: $(GTEST_DIR)/src/gtest-all.cc
	$(CXX) -c -I$(GTEST_DIR) -o $@ $^ $(LIBS) -lpthread
	
gtest_main.o: $(GTEST_DIR)/src/gtest_main.cc
	$(CXX) -c -I$(GTEST_DIR) -o $@ $^ $(LIBS) -lpthread
	
tester: $(OBJECTS_TEST) gtest-all.o gtest_main.o
	$(CXX) -o $@ $^ $(LIBS) -lpthread
	
clean:
	rm -f $(OBJECTS) $(OBJECTS_TEST)
	rm -f $(DEPENDENCIES)
	rm -f help_text_html.h

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

