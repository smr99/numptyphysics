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
#ifndef OS_H
#define OS_H

#include "Event.h"
#include <stdlib.h>
#include <string>


class Accelerometer;
class WidgetParent;

class Os
{
 public:  
  virtual ~Os() {}
  /**
   * @brief A hook to poll the OS for events
   * 
   * Override this function in order to read OS-specific devices for input.
   * Any such input should be translated into an SDL_Event using SDL_PushEvent().
   * 
   * @return void
   */
  virtual void  poll() {};
  /**
   * @brief Enumeration function that returns the next launch file
   * 
   * This function is called repeatedly to enumerate a collection of launch files.
   * Return NULL to indicate end of list.
   * 
   * @return char*
   */
  virtual char* getLaunchFile() { return NULL; }
  /**
   * @brief Open web browser to display the given url
   * 
   * @param url ...
   * @return true if open succeeds
   */
  virtual bool  openBrowser( const char* url ) = 0;
  /**
   * @brief Obtain Accelerometer, if available.
   * 
   * @return Accelerometer*, or NULL if no Accelerometer available.
   */
  virtual Accelerometer*  getAccelerometer() { return NULL; }
  virtual EventMap* getEventMap( EventMapType type );
  
  /**
   * @brief Ensure the directory exists
   * 
   * This function attempts to create the named directory if it 
   * initially does not exist.
   * 
   * @param dirPath path name of a directory
   * @return true if the directory exists on exit
   */
  
  bool ensurePath(const std::string& dirPath);
  /**
   * @brief Test for file existence
   * 
   * @param file path name of a file
   * @return true if the file exists
   */
  bool exists(const std::string& file);
  static Os* get();
  static const char pathSep;
};

class OsObj
{
 public:
  Os* operator->() { return Os::get(); }
};
extern OsObj OS;

extern int npmain(int argc, char** argv);

#endif //OS_H
