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

#include <cstring>
#include <sys/types.h>
#include <dirent.h>
#include <stdexcept>

#include "Levels.h"
#include "ZipFile.h"
#include "Config.h"
#include "Os.h"

using namespace std;

static const char MISC_COLLECTION[] = "My Levels";
static const char DEMO_COLLECTION[] = "My Solutions";


inline bool ends_with(const std::string& str, const std::string& suffix)
{
    return str.size() >= suffix.size()
	&& std::equal(suffix.rbegin(), suffix.rend(), str.rbegin());
}

static int rankFromPath( const string& p, int defaultrank=9999 )
{
  if (p==MISC_COLLECTION) {
    return 10000;
  } else if (p==DEMO_COLLECTION) {
    return 20000;
  }
  const char *c = p.data();
  size_t i = p.rfind(Os::pathSep);
  if ( i != string::npos ) {
    c += i+1;
    if ( *c=='L' || *c == 'C' ){
      c++;
      int rank=0;
      while ( *c>='0' && *c<='9' ) {
	rank = rank*10 + (*c)-'0';
	c++;
      }
      return rank;
    } else {
      c++;
    }
  }
  return defaultrank;
}

std::string nameFromPath(const std::string& path) 
{
  // TODO extract name from collection manifest
  std::string name;
  size_t i = path.rfind(Os::pathSep);
  if ( i != string::npos ) {
    i++;
  } else {
    i = 0;
  }
  if (path[i] == 'C') i++;
  if (path[i] == 'L') i++;
  while (path[i] >= '0' && path[i] <= '9') i++;
  if (path[i] == '_') i++;
  size_t e = path.rfind('.');
  name = path.substr(i,e-i);
  for (i=0; i<name.size(); i++) {
    if (name[i]=='-' || name[i]=='_' || name[i]=='.') {
      name[i] = ' ';
    }
  }
  return name;
}

Levels::Levels( int numFiles, const char** names )
  : m_numLevels(0)
{
  for ( int d=0;d<numFiles;d++ ) {
    addPath( names[d] );
  }
}

void Levels::addPath( const char* path )
{
  int len = strlen( path );
  if ( strcasecmp( path+len-4, ".npz" )==0 ) {
    scanCollection( string(path), rankFromPath(path) );
  } else if ( strcasecmp( path+len-4, ".nph" )==0 
	      || strcasecmp( path+len-4, ".npd" )==0) {
    addLevel( path, rankFromPath(path) );
  } else {
    DIR *dir = opendir( path );
    if ( dir ) {
      struct dirent* entry;
      while ( (entry = readdir( dir )) != NULL ) {
	if ( entry->d_name[0] != '.' ) {
	  string full( path );
	  full += "/";
	  full += entry->d_name;
	  //DANGER - recursion may not halt for linked dirs 
	  addPath( full.c_str() );
	}
      }
      closedir( dir );
    }
  }
}

bool Levels::addLevel( const string& file, int rank, int index )
{
  if (ends_with(file, ".npd")) {
    return addLevel( getCollection(DEMO_COLLECTION), file, rank, index );
  } else {
    return addLevel( getCollection(MISC_COLLECTION), file, rank, index );
  }
}

bool Levels::addLevel( Collection& collection,
		       const string& file, int rank, int index )
{
  vector<LevelDesc>::iterator it = collection.levels.begin();
  for( ; it != collection.levels.end(); ++it)
  {
      if (it->file == file && it->index == index)
	  return false;
      if (it->rank > rank)
	  break;
  }

  collection.levels.insert(it, LevelDesc(file, rank, index));
  m_numLevels++;
  return true;
}


Levels::Collection& Levels::getCollection( const std::string& file )
{
  for (unsigned i=0; i<m_collections.size(); i++) {
    if (m_collections[i].file == file) {
      return m_collections[i];
    }
  }

  Collection c;
  c.file = file;
  c.name = file;
  c.rank = rankFromPath(file);

  std::vector<Collection>::iterator it = m_collections.begin();
  for( ; it != m_collections.end(); ++it)
  {
      if (it->rank > c.rank)
	  break;
  }
  return *m_collections.insert(it, c);
}


bool Levels::scanCollection( const std::string& file, int rank )
{
    ZipFile zf(file);
    Collection& collection = getCollection(file);
    for ( int i=0; i<zf.numEntries(); i++ ) {
      addLevel( collection, file, rankFromPath(zf.entryName(i),rank), i );
    }
  return false;
}

int Levels::numLevels() const
{
  return m_numLevels;
}


int Levels::load( int i, unsigned char* buf, int bufLen )
{
  int l = 0;

  const LevelDesc *lev = findLevel(i);
  if (lev) {
    if ( lev->index >= 0 ) {
      ZipFile zf( lev->file.c_str() );
      if ( lev->index < zf.numEntries() ) {
	
	unsigned char* d = zf.extract( lev->index, &l);
	if ( d && l <= bufLen ) {
	  memcpy( buf, d, l );
	}
      }
    } else {
      FILE *f = fopen( lev->file.c_str(), "rt" );
      if ( f ) {
	l = fread( buf, 1, bufLen, f );
	fclose(f);
      }
    }
    return l;
  }

  throw std::invalid_argument("invalid level index");
}

std::string Levels::levelName( int i, bool pretty ) const
{
  std::string s = "end";
  const LevelDesc *lev = findLevel(i);
  if (lev) {
    if ( lev->index >= 0 ) {
      ZipFile zf( lev->file.c_str() );
      s = zf.entryName( lev->index );
    } else {
      s = lev->file;
    }
  } else {
    s = "err";
  }
  return pretty ? nameFromPath(s) : s;
}


unsigned int Levels::numCollections() const
{
  return m_collections.size();
}

int Levels::collectionFromLevel( unsigned int i, int *indexInCol ) const
{
  if (i < m_numLevels) {
    for ( unsigned c=0; c<m_collections.size(); c++ ) {
      if ( i >= m_collections[c].levels.size() ) {
	i -= m_collections[c].levels.size();
      } else {
	if (indexInCol) *indexInCol = i;
	return c;
      }
    }
  }
  return -1;
}

std::string Levels::collectionName( int i, bool pretty ) const
{
  if (i>=0 && static_cast<unsigned int>(i) < numCollections()) {
    if (pretty) {
      return nameFromPath(m_collections[i].name);
    } else {
      return m_collections[i].name;
    }
  }
  return "Bad Collection ID";
}


int Levels::collectionSize(unsigned c) const
{
  if (c>=0 && c<numCollections()) {
    return m_collections[c].levels.size();
  }
  return 0;
}

int Levels::collectionLevel(unsigned int c, unsigned int i) const
{
  if (c>=0 && c<numCollections()) {
    if (i>=0 && i<m_collections[c].levels.size()) {
      int l = i;
      for (unsigned int j=0; j<c; j++) {
	l += m_collections[j].levels.size();
      }
      return l;
    }
  }
  return 0;
}

std::string Levels::demoPath(int l) const
{
  std::string name = levelName(l,false);
  if (ends_with(name, ".npd")) {
    /* Kludge: If the level from which we want to save a demo is
     * already a demo file, return an empty string to signal
     * "don't have this demo" - see Game.cpp */
    return "";
  }

  int c = collectionFromLevel(l);
  std::string path = Config::userDataDir() + Os::pathSep
    + "Recordings" + Os::pathSep
    + collectionName(c,false);
  if (ends_with(path, ".npz")) {
    path.resize(path.length()-4);
  }
  return path;
}

std::string Levels::demoName(int l) const
{
  std::string name = levelName(l,false);
  size_t sep = name.rfind(Os::pathSep);
  if (sep != std::string::npos) {
    name = name.substr(sep);
  }
  if (ends_with(name, ".nph")) {
    name.resize(name.length()-4);
  }
  return demoPath(l) + Os::pathSep + name + ".npd";
}

bool Levels::hasDemo(int l) const
{
  return OS->exists(demoName(l));
}


const Levels::LevelDesc* Levels::findLevel( unsigned int i ) const
{
  if (i < m_numLevels) {
    for ( unsigned c=0; c<m_collections.size(); c++ ) {
      if ( i >= m_collections[c].levels.size() ) {
	i -= m_collections[c].levels.size();
      } else {
	return &m_collections[c].levels[i];
      }
    }
  }
  return NULL;
}


int Levels::findLevel( const char *file ) const
{
  int index = 0;
  for ( unsigned c=0; c<m_collections.size(); c++ ) {
    for ( unsigned i=0; i<m_collections[c].levels.size(); i++ ) {
      if ( m_collections[c].levels[i].file == file ) {
	return index + i;
      }
    }
    index += m_collections[c].levels.size();
  }
  return -1;
}


