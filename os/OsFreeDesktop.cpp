/*
 * This file is part of NumptyPhysics
 * Copyright (C) 2008 Tim Edmonds
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3 of the
 * License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 */
#if !defined(USE_HILDON) && !defined(WIN32)

#include "Os.h"
#include "Config.h"
#include <stdlib.h>
#include <string>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>

/**
 * Include SDL, so that under Mac OS X it can rename my main()
 * function to SDL_main (else NP *will* crash on OS X).
 *
 * http://www.libsdl.org/faq.php?action=listentries&category=7#55
 **/
#include "SDL.h"

class OsFreeDesktop : public Os
{
 public:
  OsFreeDesktop()
  {
  }

  virtual bool openBrowser( const char* url )
  {
    if ( url && strlen(url) < 200 ) {
      char buf[256];
      snprintf(buf,256,"xdg-open '%s'",url);
      if ( system( buf ) == 0 ) {
	return true;
      }
    }
    return false;
  }
};


Os* Os::get()
{
  static OsFreeDesktop os;
  return &os;
}

const char Os::pathSep = '/';


#ifndef TEST
int main(int argc, char** argv)
{
    npmain(argc,argv);
}
#endif

#endif
