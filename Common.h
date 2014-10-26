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

#ifndef COMMON_H
#define COMMON_H

#include "Box2D.h"
#define ARRAY_SIZE(aRR) (sizeof(aRR)/sizeof((aRR)[0]))
#define ASSERT(a)

#define FIXED_SHIFT 16
#define FIXED_ONE (1<<FIXED_SHIFT)
#define FLOAT32_TO_FIXED(float32) ((int)(float32*(float32)FIXED_ONE))
#define FIXED_TO_INT(iNT) ((iNT)>>FIXED_SHIFT)


/**
 * @brief 2D integer vector
 * Mainly used for screen positions.
 */
struct Vec2 {
  Vec2() {}
  Vec2( const Vec2& o ) : x(o.x), y(o.y) {}
  explicit Vec2( const b2Vec2& o ) : x((int)o.x), y((int)o.y) {}
  Vec2( int xx, int yy ) : x(xx), y(yy) {}
  
  void operator+=( const Vec2& o ) { x+=o.x; y+=o.y; }
  void operator-=( const Vec2& o ) { x-=o.x; y-=o.y; }
  void operator*=( int o ) { x*=o; y*=o; }

  Vec2 operator-() const { return Vec2(-x,-y); }
  bool operator==( const Vec2& o ) const { return x==o.x && y==o.y; }
  bool operator!=( const Vec2& o ) const { return !(*this==o); }
  operator b2Vec2() const { return b2Vec2((float32)x,(float32)y); } 
  
  Vec2 operator+( const Vec2& b ) const { return Vec2(x+b.x,y+b.y); }
  Vec2 operator-( const Vec2& b ) const { return Vec2(x-b.x,y-b.y); }
  Vec2 operator/( int r ) const { return Vec2(x/r,y/r); }
  Vec2 operator*( int r ) const { return Vec2(x*r,y*r); }

  int x,y;
};


/**
 * @brief Return new vector (min(a.x, b.x), min(a.y, b.y)) 
 */
inline Vec2 Min( const Vec2& a, const Vec2& b )
{
  Vec2 r;
  r.x = std::min(a.x,b.x);
  r.y = std::min(a.y,b.y);
  return r;
}

/**
 * @brief Return new vector (max(a.x, b.x), max(a.y, b.y)) 
 */
inline Vec2 Max( const Vec2& a, const Vec2& b )
{
  Vec2 r;
  r.x = std::max(a.x,b.x);
  r.y = std::max(a.y,b.y);
  return r;
}

#define Sgn(a) ((a)<0?-1:1)
#define Abs(a) ((a)<0?-(a):(a))


/**
 * @brief Rectangle with integer coordinates
 * Mainly used for screen rectangles.  
 * The Width and Height include boundary pixels; i.e. if left and right X values are the 
 * same, the Width is 1.  Similar for Height.
 * 
 * Oddly, size() doesn't include the boundary pixels.
 */
struct Rect {
  Rect() { clear(); }
  Rect( const Vec2& atl, const Vec2& abr ) : tl(atl), br(abr) {} 
  Rect( int x1, int y1, int x2, int y2 ) : tl(x1,y1), br(x2,y2) {}
  
  int width() const { return br.x - tl.x + 1;  }
  int height() const { return br.y - tl.y + 1;  }
  
  // This is inconsistent in that size() != Vec2(width(), height())
  Vec2 size() const { return br-tl; }

  void clear() { tl.x=tl.y=0; br.x=br.y=-1; }
  
  bool isEmpty() const { return tl.x > br.x || tl.y > br.y; }
  
  // If not empty, Expand by units on each of 4 sides
  void grow(int by) { 
    if (!isEmpty()) {
      tl.x -= by; tl.y -= by;
      br.x += by; br.y += by;
    }
  }
  
  // Expand this rectangle, if necessary, to include point v
  void expand( const Vec2& v ) { tl=Min(tl,v); br=Max(br,v); }

  // Expand this rectangle, if necessary, to include rectangle r
  void expand( const Rect& r ) { 
    if (isEmpty()) {
      *this = r;
    } else if (!r.isEmpty()) {
      expand(r.tl); 
      expand(r.br); 
    }
  }
  
  /// Clip this rectangle to rectangle r
  /// If rectangles do not intersect, the result is an empty rectangle
  void clipTo( const Rect& r ) { tl=Max(tl,r.tl); br=Min(br,r.br); }
  
  /// @return true if this rectangle contains point p
  bool contains( const Vec2& p ) const {
    return p.x >= tl.x && p.x <= br.x && p.y >= tl.y && p.y <= br.y;
  }
  
  /// @return true if this rectangle contains rectangle p
  bool contains( const Rect& p ) const {
    return contains(p.tl) && contains(p.br);
  }
  
  /// @return true if this rectangle intersects rectangle r
  bool intersects( const Rect& r ) const {
    return r.tl.x <= br.x
      && r.tl.y <= br.y
      && r.br.x >= tl.x 
      && r.br.y >= tl.y;
  }

  Vec2 centroid() const { return (tl+br)/2; }

  // Top Left and Bottom Right corners.
  Vec2 tl, br;
};


#endif //COMMON_H
