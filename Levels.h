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

#include <string>
#include <vector>

class Levels
{
 public:
  Levels( int numDirs=0, const char** dirs=NULL );
  
  /// Add levels specified by the given path
  /// Path may be a level file, level collection file, or a directory.
  /// If a directory, all entries are recursively evaluated by addPath.
  void addPath( const char* path );
  
  int  numLevels() const;
  
  /// Load contents of level file into specified buffer
  /// @return number bytes read
  int load( int i, unsigned char* buf, int bufLen );
  
  std::string levelName( int i, bool pretty=true ) const;
  
  /// @return level index corresponding to file; negative if file not found
  int findLevel( const char *file ) const;

  unsigned int  numCollections() const;
  
  /// @return collection index given level index; negative if level not found
  int  collectionFromLevel( unsigned int l, int *indexInCol=NULL ) const;
  
  std::string collectionName( int i, bool pretty=true ) const;
  
  /// @return number of levels in given collection
  int  collectionSize(unsigned c) const;
  
  /// @return level of collection c, file i
  int  collectionLevel(unsigned int c, unsigned int i) const;

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
    std::vector<LevelDesc> levels;
  };

  bool addLevel( const std::string& file, int rank=-1, int index=-1 );
  bool addLevel( Collection& collection,
		 const std::string& file, int rank, int index );
  const LevelDesc* findLevel( unsigned int i ) const;
  Collection& getCollection( const std::string& file );
  bool scanCollection( const std::string& file, int rank );

  unsigned int m_numLevels;
  std::vector<Collection> m_collections;
};

#endif //LEVELS_H
