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

#ifndef PATH_H
#define PATH_H

#include "Common.h"
#include <vector>


class Segment
{
public:
  Segment( const Vec2& p1, const Vec2& p2 )
    : m_p1(p1), m_p2(p2) {}
  float32 distanceTo( const Vec2& p );
private:
  Vec2 m_p1, m_p2;
};


class Path : public std::vector<Vec2>
{
public:
  Path();
  Path( const char *ptlist );

  void makeRelative();
  Path& translate(const Vec2& xlate);
  Path& rotate(const b2Mat22& rot);
  Path& rotate(const b2Rot& rot);
  Path& scale(float32 factor);

  inline const Vec2& origin() const { return at(0); }
  
  inline void append(Vec2 v)
  {
      push_back(v);
  }
  
  void trim( int i )
  {
    ASSERT( i < size() );
    resize(size() - i);
  }
  
  void erase( int i )
  {
    if (i < 0) return;
    std::vector<Vec2>::iterator it = begin() + i;
    std::vector<Vec2>::erase(it);
  }

  inline Path& operator&(const Vec2& other) 
  {
    push_back(other);
    return *this; 
  }
  
  inline Path& operator&(const b2Vec2& other) 
  {
    push_back(Vec2(other));
    return *this; 
  }
  
  inline Path operator+(const Vec2& p) const
  {
    Path r( *this );
    return r.translate( p );
  }

  inline Path operator-(const Vec2& p) const
  {
    Path r( *this );
    Vec2 n( -p.x, -p.y );
    return r.translate( n );
  }

  inline Path operator*(const b2Mat22& m) const
  {
    Path r( *this );
    return r.rotate( m );
  }

  inline Path& operator+=(const Vec2& p) 
  {
    return translate( p );
  }

  inline Path& operator-=(const Vec2& p) 
  {
    Vec2 n( -p.x, -p.y );
    return translate( n );
  }

  inline int   numPoints() const { return size(); }
  inline const Vec2& point(int i) const { return at(i); }
  inline Vec2& point(int i) { return at(i); }
  inline Vec2& first() { return at(0); }
  inline Vec2& last() { return at(size()-1); }
  inline Vec2& endpt(unsigned char end) { return end?last():first(); }

  void simplify( float32 threshold );
  Rect bbox() const;

 private:
  void simplifySub( int first, int last, float32 threshold, bool* keepflags );  
};

#endif //PATH_H
