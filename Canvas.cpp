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

#include <string>
#include "Common.h"
#include "Config.h"
#include "Canvas.h"
#include "Path.h"
#include <stdexcept>

#include <SDL.h>
#include <SDL_image.h>

#define Window X11Window //oops
#define Font X11Font //oops
#include "Swipe.h"
#include <SDL_syswm.h>
#ifndef WIN32
#include <X11/X.h>
#include <X11/Xlib.h>
#endif
#undef Window

// zoomer.cpp
extern SDL_Surface *zoomSurface(SDL_Surface * src, double zoomx, double zoomy);


// extract RGB colour components as 8bit values from RGB888
#define R32(p) (((p)>>16)&0xff)
#define G32(p) (((p)>>8)&0xff)
#define B32(p) ((p)&0xff)

// extract RGB colour components as 8bit values from RGB565
#define R16(p) (((p)>>8)&0xf8)
#define G16(p) (((p)>>3)&0xfc)
#define B16(p) (((p)<<3)&0xf8)

#define R16G16B16_TO_RGB888(r,g,b) \
  ((((r)<<8)&0xff0000) | ((g)&0x00ff00) | (((b)>>8)))

#define R16G16B16_TO_RGB565(r,g,b) \
  ((Uint16)( ((r)&0xf800) | (((g)>>5)&0x07e0) | (((b)>>11)&0x001f) ))

#define RGB888_TO_RGB565(p) \
  ((Uint16)( (((p)>>8)&0xf800) | (((p)>>5)&0x07e0) | (((p)>>3)&0x001f) ))


void ExtractRgb( uint32 c, int& r, int &g, int &b ) 
{
  r = R32(c); g = G32(c); b = B32(c);
}

void ExtractRgb( uint16 c, int& r, int &g, int &b )
{
  r = R16(c); g = G16(c); b = B16(c);
}


template <typename PIX> 
inline void AlphaBlend( PIX& p, int cr, int cg, int cb, int a, int ia )
{
  throw "not implemented";
}

inline void AlphaBlend( Uint16& p, int cr, int cg, int cb, int a, int ia )
{ //565
  p = R16G16B16_TO_RGB565( a * cr + ia * R16(p),
			   a * cg + ia * G16(p),
			   a * cb + ia * B16(p) );
}

inline void AlphaBlend( Uint32& p, int cr, int cg, int cb, int a, int ia )

{ //888
  p = R16G16B16_TO_RGB888( a * cr + ia * R32(p),
			   a * cg + ia * G32(p),
			   a * cb + ia * B32(p) );
}

#define ALPHA_MAX 0xff

template <typename PIX, unsigned W>
struct AlphaBrush
{
  int m_r, m_g, m_b, m_c;
  inline AlphaBrush( PIX c )
  {
    m_c = c;
    ExtractRgb( c, m_r, m_g, m_b );
  }
  inline void ink( PIX* pix, int step, int a ) 
  {
    int ia = ALPHA_MAX - a;
    int o=-W/2;
    AlphaBlend( *(pix+o*step), m_r, m_g, m_b, a, ia );
    o++;
    for ( ; o<=W/2; o++ ) {
      *(pix+o*step) = m_c;
    } 
    AlphaBlend( *(pix+o*step), m_r, m_g, m_b, ia, a );
  }
};

template <typename PIX>
struct AlphaBrush<PIX,1>
{
  int m_r, m_g, m_b, m_c;
  inline AlphaBrush( PIX c )
  {
    m_c = c;
    ExtractRgb( c, m_r, m_g, m_b );
  }
  inline void ink( PIX* pix, int step, int a ) 
  {
    int ia = ALPHA_MAX - a;
    AlphaBlend( *(pix-step), m_r, m_g, m_b, a, ia );
    AlphaBlend( *(pix), m_r, m_g, m_b, ia, a );
  }
};

template <typename PIX>
struct AlphaBrush<PIX,3>
{
  int m_r, m_g, m_b, m_c;
  inline AlphaBrush( PIX c )
  {
    m_c = c;
    ExtractRgb( c, m_r, m_g, m_b );
  }
  inline void ink( PIX* pix, int step, int a ) 
  {
    int ia = ALPHA_MAX - a;
    AlphaBlend( *(pix-step), m_r, m_g, m_b, a, ia );
    *(pix) = m_c;
    AlphaBlend( *(pix+step), m_r, m_g, m_b, ia, a );
  }
};



template <typename PIX, unsigned THICK> 
inline void renderLine( void *buf,
			int byteStride,
			int x1, int y1, int x2, int y2,
			PIX color )
{  
  PIX *pix = (PIX*)((char*)buf+byteStride*y1) + x1;
  int lg_delta, sh_delta, cycle, lg_step, sh_step;
  int alpha, alpha_step, alpha_reset;
  int pixStride = byteStride/sizeof(PIX);
  AlphaBrush<PIX,THICK> brush( color );

  lg_delta = x2 - x1;
  sh_delta = y2 - y1;
  lg_step = Sgn(lg_delta);
  lg_delta = Abs(lg_delta);
  sh_step = Sgn(sh_delta);
  sh_delta = Abs(sh_delta);
  if ( sh_step < 0 )  pixStride = -pixStride;

  // in theory should be able to do this with just a single step
  // variable - ie: combine cycle and alpha as in wu algorithm
  if (sh_delta < lg_delta) {
    cycle = lg_delta >> 1;
    alpha = ALPHA_MAX >> 1;
    alpha_step = -(ALPHA_MAX * sh_delta/(lg_delta+1));
    alpha_reset = alpha_step < 0 ? ALPHA_MAX : 0;
    int count = lg_step>0 ? x2-x1 : x1-x2;
    while ( count-- ) {
      brush.ink( pix, pixStride, alpha );
      cycle += sh_delta;
      alpha += alpha_step;
      pix += lg_step;
      if (cycle > lg_delta) {
	cycle -= lg_delta;
	alpha = alpha_reset;
	pix += pixStride;
      }
    }
  } else {
    cycle = sh_delta >> 1;
    alpha = ALPHA_MAX >> 1;
    alpha_step = -lg_step * Abs(ALPHA_MAX * lg_delta/(sh_delta+1));
    alpha_reset = alpha_step < 0 ? ALPHA_MAX : 0;
    int count = sh_step>0 ? y2-y1 : y1-y2;
    while ( count-- ) {
      brush.ink( pix, 1, alpha );
      cycle += lg_delta;
      alpha += alpha_step;
      pix += pixStride;
      if (cycle > sh_delta) {
	cycle -= sh_delta;
	alpha = alpha_reset;
	pix += lg_step;
      }
    }
  }
}


Canvas::Canvas( int w, int h )
  : m_surface(NULL),
    m_bgColour(0),
    m_bgImage(NULL)
{
  m_surface = SDL_CreateRGBSurface( SDL_SWSURFACE, w, h, 32,
				    0xFF0000, 0x00FF00, 0x0000FF, 0xFF000000 );
  resetClip();
}


Canvas::Canvas( SDL_Surface* surface )
  : m_surface(surface),
    m_bgColour(0),
    m_bgImage(NULL)
{
  resetClip();
}

Canvas::~Canvas()
{
  if (m_surface) {
    SDL_FreeSurface(m_surface);
  }
}

int Canvas::width() const
{
  return m_surface->w;
}

int Canvas::height() const
{
  return m_surface->h;
}

int Canvas::makeColour( int r, int g, int b ) const
{
  return SDL_MapRGB( m_surface->format, r, g, b );
}

int Canvas::makeColour( int c ) const
{
  return SDL_MapRGB( m_surface->format,
		     (c>>16)&0xff, (c>>8)&0xff, (c>>0)&0xff );
}

void Canvas::resetClip()
{
  if ( m_surface ) {
    setClip( 0, 0, width(), height() );
  } else {
    setClip( 0, 0, 0, 0 );
  }
}

void Canvas::setClip( int x, int y, int w, int h )
{
  m_clip = Rect(x,y,x+w-1,y+h-1);
}

void Canvas::setBackground( int c )
{
  m_bgColour = c;
}

void Canvas::setBackground( Canvas* bg )
{
  m_bgImage = bg;
}

void Canvas::clear()
{
  if ( m_bgImage ) {
    SDL_BlitSurface( m_bgImage->m_surface, NULL, m_surface, NULL );
  } else {
    SDL_FillRect( m_surface, NULL, m_bgColour );
  }
}

void Canvas::fade( const Rect& rr ) 
{
  Uint32 bpp;
  Rect r = rr;
  r.clipTo( m_clip );
  bpp = m_surface->format->BytesPerPixel;
  char* row = (char*)m_surface->pixels;
  int pixStride = width();
  int w = r.br.x - r.tl.x;
  int h = r.br.y - r.tl.y;
  row += (r.tl.x + r.tl.y * pixStride) * bpp;

  SDL_LockSurface(m_surface);
  switch ( bpp ) {
  case 2: 
    for ( int r=h; r>0; r-- ) {
      for ( int i=0;i<w;i++) {
	((Uint16*)row)[i] = (((Uint16*)row)[i]>>1) & 0x7bef;
      }
      row += pixStride * bpp;
    }
    break;
  case 4:
    for ( int r=h; r>0; r-- ) {
      for ( int i=0;i<w;i++) {
	((Uint32*)row)[i] = (((Uint32*)row)[i]>>1) & 0x7f7f7f;
      }
      row += pixStride * bpp;
    }
    break;
  }
  SDL_UnlockSurface(m_surface);
}


Canvas* Canvas::scale( int factor ) const
{
  Canvas *c = new Canvas( width()/factor, height()/factor );  
  if ( c ) {
    if ( factor==4 && m_surface->format->BytesPerPixel==2 ) {
      const uint16 MASK2LSB = 0xe79c;
      int dpitch = c->m_surface->pitch / sizeof(uint16_t);
      int spitch = m_surface->pitch / sizeof(uint16_t);
      uint16_t *drow = (uint16_t*) c->m_surface->pixels;
      for ( int y=0;y<c->height();y++ ) {
	for ( int x=0;x<c->width();x++ ) {
          uint16 p = 0;
	  uint16_t *srow = (uint16_t*)m_surface->pixels
  	                    + (y*spitch+x)*factor;
	  for ( int yy=0;yy<4;yy++ ) {
            uint16 q = 0;
	    for ( int xx=0;xx<4;xx++ ) {
	      q += (srow[xx]&MASK2LSB)>>2;
	    }
            p += (q&MASK2LSB)>>2;
            srow += spitch;
	  }
          drow[x] = p;
	}
	drow += dpitch;
      }
    } else if (m_surface->format->BytesPerPixel==2 ) {
      int dpitch = c->m_surface->pitch / sizeof(uint16_t);
      int spitch = m_surface->pitch / sizeof(uint16_t);
      uint16_t *drow = (uint16_t*)c->m_surface->pixels;
      for ( int y=0;y<c->height();y++ ) {
	for ( int x=0;x<c->width();x++ ) {
	  uint16_t *srow = (uint16_t*)m_surface->pixels
  	                    + (y*spitch+x)*factor;
	  uint32_t r=0,g=0,b=0;
	  for ( int yy=0;yy<factor;yy++ ) {
	    for ( int xx=0;xx<factor;xx++ ) {
	      r += srow[xx] & 0xF800;
	      g += srow[xx] & 0x07e0;
	      b += srow[xx] & 0x001F;
	    }
            srow += spitch;
	  }
	  r /= factor*factor;
	  g /= factor*factor;
	  b /= factor*factor;
          drow[x] = (r&0xF800)|(g&0x07e0)|(b&0x001F);
	}
	drow += dpitch;
      }
    } else {
      for ( int y=0;y<c->height();y++ ) {
	for ( int x=0;x<c->width();x++ ) {
	  int r=0,g=0,b=0;
	  Uint8 rr,gg,bb;
	  for ( int yy=0;yy<factor;yy++ ) {
	    for ( int xx=0;xx<factor;xx++ ) {
	      SDL_GetRGB( readPixel( x*factor+xx, y*factor+yy ),
			  m_surface->format, &rr,&gg,&bb );
	      r += rr;
	      g += gg;
	      b += bb;
	    }
	  }
	  int div = factor*factor;
	  c->drawPixel( x, y, makeColour(r/div,g/div,b/div) );
	}
      }
    }
  }
  return c;
}


void Canvas::scale( int w, int h )
{
  if ( w!=width() && h!=height() ) {
    SDL_Surface *s = zoomSurface( m_surface,
				  (double)w/(double)width(),
				  (double)h/(double)height() );
    if ( s ) {
      SDL_FreeSurface( m_surface );
      m_surface = s;
    }
  }
}

template <typename CoordType, typename SizeType>
SDL_Rect make_SDL_Rect(const CoordType& x, const CoordType& y,
		       const SizeType& w, const SizeType& h)
{
    SDL_Rect r = { static_cast<Sint16>(x), static_cast<Sint16>(y), 
		   static_cast<Uint16>(w), static_cast<Uint16>(h) };
    return r;		   
}

void Canvas::clear( const Rect& r )
{
  if ( m_bgImage ) {
    SDL_Rect srcRect = make_SDL_Rect(r.tl.x, r.tl.y, r.br.x-r.tl.x+1, r.br.y-r.tl.y+1);
    SDL_BlitSurface( m_bgImage->m_surface, &srcRect, m_surface, &srcRect );
  } else {
    drawRect( r, m_bgColour );
  }
}

void Canvas::drawImage( Canvas *canvas, int x, int y )
{
  Rect dest(x,y,x+canvas->width(),y+canvas->height());
  dest.clipTo(m_clip);

  SDL_Rect sdlsrc = make_SDL_Rect(dest.tl.x-x, dest.tl.y-y, dest.width(), dest.height());
  SDL_Rect sdldst = make_SDL_Rect(dest.tl.x, dest.tl.y, 0, 0);
  SDL_BlitSurface( canvas->m_surface, &sdlsrc, m_surface, &sdldst );
}

void Canvas::drawPixel( int x, int y, int c )
{
  Uint32 bpp, ofs;

  bpp = m_surface->format->BytesPerPixel;
  ofs = m_surface->pitch*y;
  char* row = (char*)m_surface->pixels + ofs;

  SDL_LockSurface(m_surface);
  switch ( bpp ) {
  case 2: ((Uint16*)row)[x] = c; break;
  case 4: ((Uint32*)row)[x] = c; break;
  }
  SDL_UnlockSurface(m_surface);
}

int Canvas::readPixel( int x, int y ) const
{
  Uint32 bpp, ofs;
  int c;

  bpp = m_surface->format->BytesPerPixel;
  ofs = m_surface->pitch*y;
  char* row = (char*)m_surface->pixels + ofs;

  SDL_LockSurface(m_surface);
  switch ( bpp ) {
  case 2: c = ((Uint16*)row)[x]; break;
  case 4: c = ((Uint32*)row)[x]; break;
  default: c=0; break;
  }
  SDL_UnlockSurface(m_surface);
  return c;
}

void Canvas::drawLine( int x1, int y1, int x2, int y2, int color )
{  
  int lg_delta, sh_delta, cycle, lg_step, sh_step;
  lg_delta = x2 - x1;
  sh_delta = y2 - y1;
  lg_step = Sgn(lg_delta);
  lg_delta = Abs(lg_delta);
  sh_step = Sgn(sh_delta);
  sh_delta = Abs(sh_delta);
  if (sh_delta < lg_delta) {
    cycle = lg_delta >> 1;
    while (x1 != x2) {
      drawPixel( x1, y1, color);
      cycle += sh_delta;
      if (cycle > lg_delta) {
	cycle -= lg_delta;
	y1 += sh_step;
      }
      x1 += lg_step;
    }
    drawPixel( x1, y1, color);
  }
  cycle = sh_delta >> 1;
  while (y1 != y2) {
    drawPixel( x1, y1, color);
    cycle += lg_delta;
    if (cycle > sh_delta) {
      cycle -= sh_delta;
      x1 += lg_step;
    }
    y1 += sh_step;
  }
  drawPixel( x1, y1, color);
}

void Canvas::drawPath( const Path& path, int color, bool thick )
{
  // allow for thick lines in clipping
  Rect clip = m_clip;
  clip.tl.x++; clip.tl.y++;
  clip.br.x--; clip.br.y--;

  int i=0;
  const int n = path.numPoints();
 
  for ( ; i<n && !clip.contains( path.point(i) ); i++ ) {
    //skip clipped start pt
  }
  i++;
  SDL_LockSurface(m_surface);
  for ( ; i<n; i++ ) {
    // pt i-1 is guranteed to be inside clipping    
    const Vec2& p2 = path.point(i);
    if ( clip.contains( p2 ) ) {
      const Vec2& p1 = path.point(i-1);
      switch ( m_surface->format->BytesPerPixel ) {
      case 2:      
	if ( thick ) {
	  renderLine<Uint16,3>( m_surface->pixels,
				m_surface->pitch,
				p1.x, p1.y, p2.x, p2.y, color );
	} else {
	  renderLine<Uint16,1>( m_surface->pixels,
				m_surface->pitch,
				p1.x, p1.y, p2.x, p2.y, color );
	}
	break;
      case 4:
	if ( thick ) {
	  renderLine<Uint32,3>( m_surface->pixels,
				m_surface->pitch,
				p1.x, p1.y, p2.x, p2.y, color );
	} else {
	  renderLine<Uint32,1>( m_surface->pixels,
				m_surface->pitch,
				p1.x, p1.y, p2.x, p2.y, color );
	}
	break;
      }
    } else {
      for ( ; i<n && !clip.contains( path.point(i) ); i++ ) {
	//skip until we find a unclipped pt - this will be p1 next
	//time around
      }
    }
  }
  SDL_UnlockSurface(m_surface);
  
}

void Canvas::drawRect( int x, int y, int w, int h, int c, bool fill )
{
  if ( fill ) {
    Rect dest(x,y,x+w,y+h);
    dest.clipTo(m_clip);
    SDL_Rect r = make_SDL_Rect(dest.tl.x, dest.tl.y, dest.width(), dest.height());
    SDL_FillRect( m_surface, &r, c );
  } else {
    SDL_Rect f = make_SDL_Rect(x, y, w, h);
    SDL_Rect r;
    r=f; r.h=1; SDL_FillRect( m_surface, &r, c );
    r.y+=f.h-1; SDL_FillRect( m_surface, &r, c );
    r=f; r.w=1; SDL_FillRect( m_surface, &r, c );
    r.x+=f.w-1; SDL_FillRect( m_surface, &r, c );
  }
}

void Canvas::drawRect( const Rect& r, int c, bool fill )
{
  drawRect( r.tl.x, r.tl.y, r.br.x-r.tl.x+1, r.br.y-r.tl.y+1, c, fill );
}



Window::Window( int w, int h, const char* title, const char* winclass, bool fullscreen )
  : m_title(title)
{
  if ( winclass ) {
    char s[80];
    snprintf(s,80,"SDL_VIDEO_X11_WMCLASS=%s",winclass);
    putenv(s);
  }

#ifdef USE_HILDON
  fullscreen = true;
  SDL_ShowCursor( SDL_DISABLE );
#endif
  
//  int st = SDL_CreateWindowAndRenderer(0, 0, SDL_WINDOW_FULLSCREEN_DESKTOP, &m_window, &m_renderer);
  int st = SDL_CreateWindowAndRenderer(w, h, 0, &m_window, &m_renderer);
  if (st != 0 || m_window == NULL || m_renderer == NULL)
      throw std::runtime_error("Failed to create window");
  
  m_surface = SDL_GetWindowSurface(m_window);

  resetClip();
  
  SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 255);
  SDL_RenderClear(m_renderer);

  memset(&(Swipe::m_syswminfo), 0, sizeof(SDL_SysWMinfo));
  SDL_VERSION(&(Swipe::m_syswminfo.version));
  SDL_GetWindowWMInfo(m_window, &(Swipe::m_syswminfo));
  Swipe::lock(true);
}


void Window::update( const Rect& r )
{
  if ( r.tl.x < width() && r.tl.y < height() ) {
    int x1 = std::max( 0, r.tl.x );
    int y1 = std::max( 0, r.tl.y );
    int x2 = std::min( width()-1, r.br.x );
    int y2 = std::min( height()-1, r.br.y );
    int w  = std::max( 0, x2-x1 );
    int h  = std::max( 0, y2-y1 );
    if ( w > 0 && h > 0 ) {
	// TODO: SDL_UpdateWindowSurfaceRects(m_window, NULL, 0);
      SDL_UpdateWindowSurface(m_window);
#ifdef USE_HILDON
#if MAEMO_VERSION >= 5
      static bool captured = false;
      if (!captured) {
	SDL_SysWMinfo sys;
	SDL_VERSION( &sys.version );
	SDL_GetWMInfo( &sys );

	// setup hildon pre-load screenshot
	XEvent xev = { 0 };
	xev.xclient.type = ClientMessage;
	xev.xclient.serial = 0;
	xev.xclient.send_event = True;
	xev.xclient.display = sys.info.x11.display;
	xev.xclient.window = XDefaultRootWindow(xev.xclient.display);
	xev.xclient.message_type = XInternAtom (xev.xclient.display,
						"_HILDON_LOADING_SCREENSHOT",
						False);
	xev.xclient.format = 32;
	xev.xclient.data.l[0] = 0;
	//xev.xclient.data.l[1] = sys.info.x11.fswindow;
	//xev.xclient.data.l[1] = sys.info.x11.wmwindow;
	xev.xclient.data.l[1] = sys.info.x11.window;
	XSendEvent (xev.xclient.display,
		    xev.xclient.window,
		    False,
		    SubstructureRedirectMask | SubstructureNotifyMask,
		    &xev);
	XFlush (xev.xclient.display);
	XSync (xev.xclient.display, False);
	captured = true;
      }
#endif
#endif
    }
  }
}

void Window::raise()
{
    SDL_RaiseWindow(m_window);
}


Image::Image( const char* file, bool alpha )
{
  //alpha = false;
  std::string f( "data/" );
  SDL_Surface* img = IMG_Load((f+file).c_str());
  if ( !img ) {
    f = std::string( DEFAULT_RESOURCE_PATH "/" );
    img = IMG_Load((f+file).c_str());
  }
  if ( img ) {
    m_surface = img;
  } else {
    m_surface = SDL_CreateRGBSurface( 0, 32, 32, 32,
				      0x00FF0000,
                                      0x0000FF00,
                                      0x000000FF,
                                      0xFF000000);
    drawRect(0,0,32,32,0xff0000);
  }
  resetClip();
}



int Canvas::writeBMP( const char* filename ) const
{
#pragma pack(push,1)
  typedef struct {
    unsigned short int type;         /* Magic identifier */
    unsigned int size;               /* File size in bytes */
    unsigned short int reserved1, reserved2;
    unsigned int offset;             /* Offset to image data, bytes */
  } BMPHEADER;
  
  typedef struct {
    unsigned int size;               /* Header size in bytes      */
    int width,height;                /* Width and height of image */
    unsigned short int planes;       /* Number of colour planes   */
    unsigned short int bits;         /* Bits per pixel            */
    unsigned int compression;        /* Compression type          */
    unsigned int imagesize;          /* Image size in bytes       */
    int xresolution,yresolution;     /* Pixels per meter          */
    unsigned int ncolours;           /* Number of colours         */
    unsigned int importantcolours;   /* Important colours         */
  } BMPINFOHEADER;
  int check_BMPHEADER[(sizeof(BMPHEADER)==14)-1];
  int check_BMPINFOHEADER[(sizeof(BMPINFOHEADER)==40)-1];
#pragma pack(pop)
    
  if (sizeof(check_BMPHEADER) != 0 || sizeof(check_BMPINFOHEADER) != 0)
      return 0;
  
  int w = width();
  int h = height();
  BMPHEADER     head = { 'B'|('M'<<8), 14+40+w*h*3u, 0, 0, 14+40 };
  BMPINFOHEADER info = { 40, w, h, 1, 24, 0, w*h*3u, 100, 100, 0, 0 };

  FILE *f = fopen( filename, "wb" );
  if ( f ) {
    Uint32 bpp;
    bpp = m_surface->format->BytesPerPixel;

    fwrite( &head, 14, 1, f );
    fwrite( &info, 40, 1, f );
    for ( int y=h-1; y>=0; y-- ) {
      for ( int x=0; x<w; x++ ) {
	int p = readPixel( x, y );
	if ( bpp==2 ) {
	  p = R16G16B16_TO_RGB888( R16(p), G16(p), B16(p) );
	}
	fwrite( &p, 3, 1, f );
      }
    }
    fclose(f);
    return 1;
  }
  return 0;
}
