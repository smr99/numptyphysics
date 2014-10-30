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

#ifndef LEVELS_H
#define LEVELS_H

#include "Array.h"
#include <string>

class Levels
{
 public:
  Levels( int numDirs=0, const char** dirs=NULL );
  void addPath( const char* path );
  bool addLevel( const std::string& file, int rank=-1, int index=-1 );
  int  numLevels() const;
  int load( int i, unsigned char* buf, int bufLen );
  
  std::string levelName( int i, bool pretty=true ) const;
  
  /// @return level index corresponding to file; negative if file not found
  int findLevel( const char *file ) const;

  int  numCollections() const;
  
  /// @return collection index given level index; negative if level not found
  int  collectionFromLevel( int l, int *indexInCol=NULL ) const;
  
  std::string collectionName( int i, bool pretty=true ) const;
  
  /// @return number of levels in given collection
  int  collectionSize(int c) const;
  
  /// @return level of collection c, file i
  int  collectionLevel(int c, int i) const;

  std::string demoPath(int l) const;
  std::string demoName(int l) const;
  bool hasDemo(int l) const;

 private:

  struct LevelDesc
  {
  LevelDesc( const std::string& f,int r=0, int i=-1)
  : file(f), index(i), rank(r) {}
    std::string file;
    int         index;
    int         rank;
  };

  struct Collection
  {
    std::string file;
    std::string name;
    int rank;
    Array<LevelDesc*> levels;
  };

  bool addLevel( Collection* collection,
		 const std::string& file, int rank, int index );
  LevelDesc* findLevel( int i ) const;
  Collection* getCollection( const std::string& file );
  bool scanCollection( const std::string& file, int rank );

  int m_numLevels;
  Array<Collection*> m_collections;
};

#endif //LEVELS_H
