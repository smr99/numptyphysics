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
#include "Common.h"
#include "Config.h"
#include "Scene.h"
#include "Accelerometer.h"

#include <sstream>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <vector>


using namespace std;


Transform::Transform( float32 scale, float32 rotation, const Vec2& translation )
{
  set( scale, rotation, translation );
}

void Transform::set( float32 scale, float32 rotation, const Vec2& translation )
{
  if ( scale==0.0f && rotation==0.0f && translation==Vec2(0,0) ) {
    m_bypass = true;
  } else {
    float32 c = cosf(rotation), s = sinf(rotation);
    m_pos = translation;
    m_rot.ex.x = c * scale;
    m_rot.ex.y = s * scale;
    m_rot.ey.x = -s * scale;
    m_rot.ey.y = c * scale;
    m_invrot = m_rot.GetInverse();
    m_bypass = false;
  }
}

Transform worldToScreen( 0.5f, M_PI/2, Vec2(240,0) );

void configureScreenTransform( int w, int h )
{
  SCREEN_WIDTH = w;
  SCREEN_HEIGHT = h;
  FULLSCREEN_RECT = Rect(0,0,w-1,h-1);
  if ( w==WORLD_WIDTH && h==WORLD_HEIGHT ) { //unity
    worldToScreen.set( 0.0f, 0.0f, Vec2(0,0) );
  } else {
    float rot = 0.0f;
    Vec2 tr(0,0);
    if ( h > w ) { //portrait
      rot = M_PI/2;
      tr = Vec2( w, 0 );
      b2Swap( h, w );
    }
    float scalew = (float)w/(float)WORLD_WIDTH;
    float scaleh = (float)h/(float)WORLD_HEIGHT;
    if ( scalew < scaleh ) {
      worldToScreen.set( scalew, rot, tr );
    } else {
      worldToScreen.set( scaleh, rot, tr );
    }
  }
}


struct Joint
{
  Joint( Stroke *j1, Stroke* j2, unsigned char e )
    : joiner(j1), joinee(j2), end(e) {}
  Stroke *joiner;
  Stroke *joinee;
  unsigned char end; //of joiner
};

class Stroke
{
public:

private:
  struct JointDef : public b2RevoluteJointDef
  {
    JointDef( b2Body* b1, b2Body* b2, const b2Vec2& pt )
    {
      Initialize( b1, b2, pt );
      maxMotorTorque = 10.0f;
      motorSpeed = 0.0f;
      enableMotor = true;
    }
  };

  struct BoxDef : public b2FixtureDef
  {
    void init( const Vec2& p1, const Vec2& p2, int attr )
    {
      b2Vec2 barOrigin = p1;
      b2Vec2 bar = p2 - p1;
      bar *= 1.0f/PIXELS_PER_METREf;
      barOrigin *= 1.0f/PIXELS_PER_METREf;;
      m_shape.SetAsBox( bar.Length()/2.0f, 0.1f,
		0.5f*bar + barOrigin, vec2Angle( bar ));
      shape = &m_shape;
      friction = 0.3f;
      if ( attr & ATTRIB_GROUND ) {
	density = 0.0f;
      } else if ( attr & ATTRIB_GOAL ) {
	density = 100.0f;
      } else if ( attr & ATTRIB_TOKEN ) {
	density = 3.0f;
	friction = 0.1f;
      } else {
	density = 5.0f;
      }
      restitution = 0.2f;
    }
    
    b2PolygonShape m_shape;
  };

public:
  Stroke( const Path& path )
    : m_rawPath(path)
  {
    m_body = 0;
    m_colour = brushColours[DEFAULT_BRUSH];
    m_attributes = 0;
    m_origin = m_rawPath.point(0);
    m_rawPath.translate( -m_origin );
    reset();
  }  

  Stroke( const std::string& str ) 
  {
    int col = 0;
    m_body = 0;
    m_colour = brushColours[DEFAULT_BRUSH];
    m_attributes = 0;
    m_origin = Vec2(400,240);
    reset();
    const char *s = str.c_str();
    while ( *s && *s!=':' && *s!='\n' ) {
      switch ( *s ) {
      case 't': setAttribute( ATTRIB_TOKEN ); break;	
      case 'g': setAttribute( ATTRIB_GOAL ); break;	
      case 'f': setAttribute( ATTRIB_GROUND ); break;
      case 's': setAttribute( ATTRIB_SLEEPING ); break;
      case 'd': setAttribute( ATTRIB_DECOR ); break;
      default:
	if ( *s >= '0' && *s <= '9' ) {
	  col = col*10 + *s -'0';
	}
	break;
      }
      s++;
    }
    if ( col >= 0 && col < NUM_BRUSHES ) {
      m_colour = brushColours[col];
    }
    if ( *s++ == ':' ) {
      m_rawPath = Path(s);
    }
    if ( m_rawPath.size() < 2 ) {
      throw "invalid stroke def";
    }
    m_origin = m_rawPath.point(0);
    m_rawPath.translate( -m_origin );
    setAttribute( ATTRIB_DUMMY );
  }

  void reset( b2World* world=NULL )
  {
    if (m_body && world) {
      world->DestroyBody( m_body );
    }
    m_body = NULL;
    m_xformAngle = 7.0f;
    m_drawnBbox.tl = m_origin;
    m_drawnBbox.br = m_origin;
    m_jointed[0] = m_jointed[1] = false;
    m_shapePath = m_rawPath;
    m_hide = 0;
    m_drawn = false;
  }

  std::string asString()
  {
    std::stringstream s;
    s << 'S';
    if ( hasAttribute(ATTRIB_TOKEN) )    s<<'t';
    if ( hasAttribute(ATTRIB_GOAL) )     s<<'g';
    if ( hasAttribute(ATTRIB_GROUND) )   s<<'f';
    if ( hasAttribute(ATTRIB_SLEEPING) ) s<<'s';
    if ( hasAttribute(ATTRIB_DECOR) )    s<<'d';
    for ( int i=0; i<NUM_BRUSHES; i++ ) {
      if ( m_colour==brushColours[i] )  s<<i;
    }
    s << ":";
    Path opath = m_rawPath;
    opath.translate(m_origin);
    for ( unsigned i=0; i<opath.size(); i++ ) {
      const Vec2& p = opath.point(i);
      s <<' '<< p.x << ',' << p.y; 
    }
    s << std::endl;
    return s.str();
  }

  void setAttribute( Attribute a )
  {
    m_attributes |= a;
    if ( m_attributes & ATTRIB_TOKEN )     m_colour = brushColours[RED_BRUSH];
    else if ( m_attributes & ATTRIB_GOAL ) m_colour = brushColours[YELLOW_BRUSH];
  }

  void clearAttribute( Attribute a )
  {
    m_attributes &= ~a;
  }

  bool hasAttribute( Attribute a )
  {
    return (m_attributes&a) != 0;
  }
  void setColour( int c ) 
  {
    m_colour = c;
  }

  void createBodies( b2World& world )
  {
    process();
    if ( hasAttribute( ATTRIB_DECOR ) ){
      return; //decorators have no physical embodiment
    }
    int n = m_shapePath.numPoints();
    if ( n > 1 ) {
      b2BodyDef bodyDef;
      if (hasAttribute(ATTRIB_GROUND)) {
	  bodyDef.type = b2_staticBody;
      } else {
	  bodyDef.type = b2_dynamicBody;
      }
      bodyDef.position = m_origin;
      bodyDef.position *= 1.0f/PIXELS_PER_METREf;
      bodyDef.userData = this;
      if ( hasAttribute(ATTRIB_SLEEPING) ) {
	bodyDef.allowSleep = true;
	bodyDef.awake = false;
      }	  
      m_body = world.CreateBody( &bodyDef );
      for ( int i=1; i<n; i++ ) {
	BoxDef boxDef;
	boxDef.init( m_shapePath.point(i-1),
		     m_shapePath.point(i),
		     m_attributes );
	m_body->CreateFixture( &boxDef );
      }
      m_body->SetAwake(!hasAttribute(ATTRIB_SLEEPING));
    }
    transform();
  }

  void determineJoints( Stroke* other, vector<Joint>& joints )
  {
    if ( (m_attributes&ATTRIB_CLASSBITS)
	 != (other->m_attributes&ATTRIB_CLASSBITS)
	 || hasAttribute(ATTRIB_GROUND)
	 || hasAttribute(ATTRIB_UNJOINABLE)
	 || other->hasAttribute(ATTRIB_UNJOINABLE)) {
      // cannot joint goals or tokens to other things
      // and no point jointing ground endpts
      return;
    } 

    transform();
    for ( unsigned char end=0; end<2; end++ ) {
      if ( !m_jointed[end] ) {
	const Vec2& p = m_xformedPath.endpt(end);
	if ( other->distanceTo( p ) <= JOINT_TOLERANCE ) {
	  joints.push_back( Joint(this,other,end) );
	}
      }
    }
  }

  void join( b2World* world, Stroke* other, unsigned char end )
  {
    if ( !m_jointed[end] ) {
      b2Vec2 p = m_xformedPath.endpt( end );
      p *= 1.0f/PIXELS_PER_METREf;
      JointDef j( m_body, other->m_body, p );
      world->CreateJoint( &j );
      m_jointed[end] = true;
    }
  }

  void draw( Canvas& canvas, bool drawJoints=false )
  {
    if ( m_hide < HIDE_STEPS ) {
      int colour = canvas.makeColour(m_colour);
      bool thick = (canvas.width() > 400);
      transform();
      canvas.drawPath( m_screenPath, colour, thick );
      m_drawn = true;
      
      if ( drawJoints ) {
	int jointcolour = canvas.makeColour(0xff0000);
	for ( int e=0; e<2; e++ ) {
	  if (m_jointed[e]) {
	    const Vec2& pt = m_screenPath.endpt(e);
	    //canvas.drawPixel( pt.x, pt.y, jointcolour );
	    //canvas.drawRect( pt.x-1, pt.y-1, 3, 3, jointcolour );
	    canvas.drawRect( pt.x-1, pt.y, 3, 1, jointcolour );
	    canvas.drawRect( pt.x, pt.y-1, 1, 3, jointcolour );
	  }
	}
      }
    }
    m_drawnBbox = m_screenBbox;
  }

  void addPoint( const Vec2& pp ) 
  {
    Vec2 p = pp; p -= m_origin;
    if ( p == m_rawPath.point( m_rawPath.numPoints()-1 ) ) {
    } else {
      m_rawPath.push_back( p );
      m_drawn = false;
    }
  }

  void origin( const Vec2& p ) 
  {
    // todo 
    if ( m_body ) {
      b2Vec2 pw = p;
      pw *= 1.0f/PIXELS_PER_METREf;
      m_body->SetTransform( pw, m_body->GetAngle() );
    }
    m_origin = p;
    m_drawn = false;
  }

  b2Body* body() { return m_body; }

  float32 distanceTo( const Vec2& pt )
  {
    float32 best = 100000.0;
    transform();
    for ( int i=1; i<m_xformedPath.numPoints(); i++ ) {    
      Segment s( m_xformedPath.point(i-1), m_xformedPath.point(i) );
      float32 d = s.distanceTo( pt );
      if ( d < best ) {
        best = d;
      }
    }
    return best;
  }

  Rect screenBbox() 
  {
    transform();
    return m_screenBbox;
  }

  Rect lastDrawnBbox() 
  {
    return m_drawnBbox;
  }

  Rect worldBbox() 
  {
    return m_xformedPath.bbox();
  }

  bool isDirty()
  {
    return (!m_drawn || transform()) && !hasAttribute(ATTRIB_DELETED);
  }

  void hide()
  {
    if ( m_hide==0 ) {
      m_hide = 1;
      
      if (m_body) {
	// stash the body where no-one will find it
	m_body->SetTransform( b2Vec2(0.0f,SCREEN_HEIGHT*2.0f), 0.0f );
	m_body->SetLinearVelocity( b2Vec2(0.0f,0.0f) );
	m_body->SetAngularVelocity( 0.0f );
      }
    }
  }

  bool hidden()
  {
    return m_hide >= HIDE_STEPS;
  }

  int numPoints()
  {
    return m_rawPath.numPoints();
  }

  const Vec2& endpt( unsigned char end ) 
  {
    return m_xformedPath.endpt(end);
  }

private:
  static float32 vec2Angle( b2Vec2 v ) 
  {
    return b2Atan2(v.y, v.x);
  } 

  void process()
  {
    float32 thresh = SIMPLIFY_THRESHOLDf;
    m_rawPath.simplify( thresh );
    m_shapePath = m_rawPath;

    while ( m_shapePath.numPoints() > MULTI_VERTEX_LIMIT ) {
      thresh += SIMPLIFY_THRESHOLDf;
      m_shapePath.simplify( thresh );
    }
  }

  bool transform()
  {
    // distinguish between xformed raw and shape path as needed
    if ( m_hide ) {
      if ( m_hide < HIDE_STEPS ) {
	Vec2 o = m_screenBbox.centroid();
	m_screenPath -= o;
	m_screenPath.scale( 0.99 );
	m_screenPath += o;
	m_screenBbox = m_screenPath.bbox();      
	m_hide++;
	return true;
      }
    } else if ( m_body ) {
      if ( hasAttribute( ATTRIB_DECOR ) ) {
	return false; // decor never moves
      } else if ( hasAttribute( ATTRIB_GROUND )	   
		  && m_xformAngle == m_body->GetAngle() ) {
	return false; // ground strokes never move.
      } else if ( m_xformAngle != m_body->GetAngle() 
	   ||  ! (m_xformPos == m_body->GetPosition()) ) {
	b2Rot rot( m_body->GetAngle() );
	b2Vec2 orig = PIXELS_PER_METREf * m_body->GetPosition();
	m_xformedPath = m_rawPath;
	m_xformedPath.rotate( rot );
	m_xformedPath.translate( Vec2(orig) );
	m_xformAngle = m_body->GetAngle();
	m_xformPos = m_body->GetPosition();
	worldToScreen.transform( m_xformedPath, m_screenPath );
	m_screenBbox = m_screenPath.bbox();      
      } else {
	return false;
      }
    } else {
      m_xformedPath = m_rawPath;
      m_xformedPath.translate( m_origin );
      worldToScreen.transform( m_xformedPath, m_screenPath );
      m_screenBbox = m_screenPath.bbox();      
      return !hasAttribute(ATTRIB_DECOR);
    }
    return true;
  }

  Path      m_rawPath;
  int       m_colour;
  int       m_attributes;
  Vec2      m_origin;
  Path      m_shapePath;
  Path      m_xformedPath;
  Path      m_screenPath;
  float32   m_xformAngle;
  b2Vec2    m_xformPos;
  Rect      m_screenBbox;
  Rect      m_drawnBbox;
  bool      m_drawn;
  b2Body*   m_body;
  bool      m_jointed[2];
  int       m_hide;
};


Scene::Scene( bool noWorld )
  : m_world( NULL ),
    m_bgImage( NULL ),
    m_protect( 0 ),
    m_gravity(0.0f, 0.0f),
    m_dynamicGravity(false),
    m_accelerometer(Os::get()->getAccelerometer()),
    m_reset_sleepers(true)
{
  if ( !noWorld ) {
    resetWorld();
  }
}

Scene::~Scene()
{
  clear();
  if ( m_world ) {
    delete m_world;
  }
}

void Scene::resetWorld()
{
  const b2Vec2 gravity(0.0f, GRAVITY_ACCELf*PIXELS_PER_METREf/GRAVITY_FUDGEf);
  delete m_world;

  m_world = new b2World(gravity);
  m_world->SetAllowSleeping(true);
  m_world->SetContactListener( this );
  m_reset_sleepers = true;
}

Stroke* Scene::newStroke( const Path& p, int colour, int attribs ) {
  Stroke *s = new Stroke(p);
  s->setAttribute( (Attribute)attribs );

  switch ( colour ) {
  case 0: s->setAttribute( ATTRIB_TOKEN ); break;
  case 1: s->setAttribute( ATTRIB_GOAL ); break;
  default: s->setColour( brushColours[colour] ); break;
  }
  m_strokes.push_back( s );
  m_recorder.newStroke( p, colour, attribs );
  return s;
}

bool Scene::deleteStroke( Stroke *s ) {
  if ( s ) {
    vector<Stroke*>::iterator it = find(m_strokes.begin(), m_strokes.end(), s);
    if (it == m_strokes.end())
	return false;
    
    int i = it - m_strokes.begin();
    if ( i >= m_protect ) {
	reset(s);
	m_strokes.erase( it );
	m_deletedStrokes.push_back( s );
	m_recorder.deleteStroke( i );
	return true;
    }
  }
  return false;
}


void Scene::extendStroke( Stroke* s, const Vec2& pt )
{
  if ( s ) {
    vector<Stroke*>::iterator it = find(m_strokes.begin(), m_strokes.end(), s);
    if (it == m_strokes.end())
	return;

    int i = it - m_strokes.begin();
    if ( i >= m_protect ) {
      s->addPoint( pt );
      m_recorder.extendStroke( i, pt );
    }
  }
}

void Scene::moveStroke( Stroke* s, const Vec2& origin )
{
  if ( s ) {
    vector<Stroke*>::iterator it = find(m_strokes.begin(), m_strokes.end(), s);
    if (it == m_strokes.end())
	return;

    int i = it - m_strokes.begin();
    if ( i >= m_protect ) {
      s->origin( origin );
      m_recorder.moveStroke( i, origin );
    }
  }
}
	

bool Scene::activateStroke( Stroke *s )
{
  if (!activate(s))
    return false;

  vector<Stroke*>::iterator it = find(m_strokes.begin(), m_strokes.end(), s);
  if (it == m_strokes.end())
      return false;

  int i = it - m_strokes.begin();
  m_recorder.activateStroke( i );
  return true;
}

void Scene::getJointCandidates( Stroke* s, Path& pts )
{
  vector<Joint> joints;
  for ( int j=m_strokes.size()-1; j>=0; j-- ) {      
    if ( s != m_strokes[j] ) {
      s->determineJoints( m_strokes[j], joints );
      m_strokes[j]->determineJoints( s, joints );
    }
  }
  for ( int j=joints.size()-1; j>=0; j-- ) {
    pts.push_back( joints[j].joiner->endpt(joints[j].end) );
  }
}

bool Scene::activate( Stroke *s )
{
  if ( s->numPoints() > 1 ) {
    s->createBodies( *m_world );
    createJoints( s );
    return true;
  }
  return false;
}

void Scene::activateAll()
{
  for ( size_t i=0; i < m_strokes.size(); i++ ) {
    m_strokes[i]->createBodies( *m_world );
  }
  for ( size_t i=0; i < m_strokes.size(); i++ ) {
    createJoints( m_strokes[i] );
  }
}

void Scene::createJoints( Stroke *s )
{
  if ( s->body()==NULL ) {
    return;
  }
  vector<Joint> joints;
  for ( int j=m_strokes.size()-1; j>=0; j-- ) {      
    if ( s != m_strokes[j] && m_strokes[j]->body() ) {
      s->determineJoints( m_strokes[j], joints );
      m_strokes[j]->determineJoints( s, joints );
      for ( size_t i=0; i<joints.size(); i++ ) {
	joints[i].joiner->join( m_world, joints[i].joinee, joints[i].end );
      }
      joints.clear();
    }
  }    
}

void Scene::step( bool isPaused )
{
  m_recorder.tick(isPaused);
  isPaused |= m_player.tick();

  if ( !isPaused ) {
    if (m_accelerometer && m_dynamicGravity) {
      float32 gx, gy, gz;
      if ( m_accelerometer->poll( gx, gy, gz ) ) {
	
	if (m_dynamicGravity || gx*gx+gy*gy > 1.2*1.2)  {
	  const float32 factor = GRAVITY_ACCELf*PIXELS_PER_METREf/GRAVITY_FUDGEf;
	  m_currentGravity = b2Vec2( m_gravity.x + gx*factor, 
				     m_gravity.y + gy*factor );
	  m_world->SetGravity( m_currentGravity );
	} else if (!(m_currentGravity == m_gravity)) {
	  m_currentGravity += m_gravity;
	  m_currentGravity *= 0.5;
	  m_world->SetGravity( m_currentGravity );
	}
	//TODO record gravity
      }
    }

    m_world->Step( ITERATION_TIMESTEPf, VELOCITY_ITERATIONS, POSITION_ITERATIONS );
    
    if (m_reset_sleepers)
    {
	for(b2Body* body = m_world->GetBodyList(); body; body = body->GetNext())
	{
	    Stroke* stroke = static_cast<Stroke*>(body->GetUserData());
	    if (stroke)
		body->SetAwake(!stroke->hasAttribute(ATTRIB_SLEEPING));
	}
	m_reset_sleepers = false;
    }
    
    // clean up delete strokes
    for ( size_t i=0; i< m_strokes.size(); i++ ) {
      if ( m_strokes[i]->hasAttribute(ATTRIB_DELETED) ) {
	m_strokes[i]->clearAttribute(ATTRIB_DELETED);
	m_strokes[i]->hide();
      }	   
    }
    // check for token respawn
    for ( size_t i=0; i < m_strokes.size(); i++ ) {
      if ( m_strokes[i]->hasAttribute( ATTRIB_TOKEN )
	   && !BOUNDS_RECT.intersects( m_strokes[i]->worldBbox() ) ) {
	reset( m_strokes[i] );
	activate( m_strokes[i] );	  
      }
    }
  }
  calcDirtyArea();
}

// b2ContactListener callback when a new contact is detected
void Scene::BeginContact(b2Contact* contact)
{     
  // check for completion
  //if (c->GetManifoldCount() > 0) {
  Stroke* s1 = (Stroke*)contact->GetFixtureA()->GetBody()->GetUserData();
  Stroke* s2 = (Stroke*)contact->GetFixtureB()->GetBody()->GetUserData();
  if ( s1 && s2 ) {
    if ( s2->hasAttribute(ATTRIB_TOKEN) ) {
	b2Swap( s1, s2 );
    }
    if ( s1->hasAttribute(ATTRIB_TOKEN) 
	   && s2->hasAttribute(ATTRIB_GOAL) ) {
	s2->setAttribute(ATTRIB_DELETED);
	m_recorder.goal(1);
    }
  }
}

bool Scene::isCompleted()
{
  for ( size_t i=0; i < m_strokes.size(); i++ ) {
    if ( m_strokes[i]->hasAttribute( ATTRIB_GOAL )
	   && !m_strokes[i]->hidden() ) {
	return false;
    }
  }
  return true;
}

Rect Scene::dirtyArea()
{
  return m_dirtyArea;
}

void Scene::calcDirtyArea()
{
  Rect r;
  for ( size_t i=0; i<m_strokes.size(); i++ ) {
    if ( m_strokes[i]->isDirty() ) {
      // acumulate new areas to draw
      r.expand( m_strokes[i]->screenBbox() );
      // plus prev areas to erase
      r.expand( m_strokes[i]->lastDrawnBbox() );
    }
  }
  for ( size_t i=0; i<m_deletedStrokes.size(); i++ ) {
    // acumulate new areas to draw
    r.expand( m_strokes[i]->lastDrawnBbox() );
  }
  if ( !r.isEmpty() ) {
    // expand to allow for thick lines
    r.grow(1);
  }
  m_dirtyArea = r;
}
void Scene::draw( Canvas& canvas, const Rect& area )
{
  if ( m_bgImage ) {
    canvas.setBackground( m_bgImage );
  } else {
    canvas.setBackground( 0 );
  }
  canvas.clear( area );
  Rect clipArea = area;
  clipArea.tl.x--;
  clipArea.tl.y--;
  clipArea.br.x++;
  clipArea.br.y++;
  for ( size_t i=0; i<m_strokes.size(); i++ ) {
    if ( area.intersects( m_strokes[i]->screenBbox() ) ) {
	m_strokes[i]->draw( canvas );
    }
  }
  while ( m_deletedStrokes.size() ) {
    delete m_deletedStrokes[0];
    m_deletedStrokes.erase(m_deletedStrokes.begin());
  }
}

void Scene::reset( Stroke* s, bool purgeUnprotected )
{
  while ( purgeUnprotected && m_strokes.size() > static_cast<size_t>(m_protect) ) {
    m_strokes[m_strokes.size()-1]->reset(m_world);
    m_strokes.erase( --m_strokes.end() );
  }
  for ( size_t i=0; i<m_strokes.size(); i++ ) {
    if (s==NULL || s==m_strokes[i]) {
	m_strokes[i]->reset(m_world);
    }
  }
}

Stroke* Scene::strokeAtPoint( const Vec2 pt, float32 max )
{
  Stroke* best = NULL;
  for ( size_t i=0; i<m_strokes.size(); i++ ) {
    float32 d = m_strokes[i]->distanceTo( pt );
    if ( d < max ) {
	max = d;
	best = m_strokes[i];
    }
  }
  return best;
}

void Scene::clear()
{
  reset();
  while ( m_strokes.size() ) {
    delete m_strokes[0];
    m_strokes.erase(m_strokes.begin());
  }
  while ( m_deletedStrokes.size() ) {
    delete m_deletedStrokes[0];
    m_deletedStrokes.erase(m_strokes.begin());
  }
  if ( m_world ) {
    //step is required to actually destroy bodies and joints
    m_world->Step( ITERATION_TIMESTEPf, VELOCITY_ITERATIONS, POSITION_ITERATIONS );
  }
  m_log.clear();
}

void Scene::setGravity( const b2Vec2& g )
{
  m_gravity = m_currentGravity = g;
  if (m_world) {
    m_world->SetGravity( m_gravity );
  }
}

void Scene::setGravity( const std::string& s )
{
  for (size_t i=0; i<s.find(':'); i++) {
    switch (s[i]) {
    case 'd': m_dynamicGravity = true; break;
    }
  }

  std::string vector = s.substr(s.find(':')+1);
  float32 x,y;      
  if ( sscanf( vector.c_str(), "%f,%f", &x, &y )==2) {
    if ( m_world ) {
	b2Vec2 g(x,y);
	g *= PIXELS_PER_METREf/GRAVITY_FUDGEf;
	setGravity( g );
    }
  } else {
    throw std::invalid_argument(std::string("invalid gravity vector: " + vector));
  }
}

bool Scene::load( unsigned char *buf, int bufsize )
{
  std::string s( (const char*)buf, bufsize );
  std::stringstream in( s, std::ios::in );
  return load( in );
}

bool Scene::load( const std::string& file )
{
  std::ifstream in( file.c_str(), std::ios::in );
  return load( in ); 
}

bool Scene::load( std::istream& in )
{
  clear();
  resetWorld();
  m_dynamicGravity = false;
  if ( g_bgImage==NULL ) {
    g_bgImage = new Image("paper.png");
    g_bgImage->scale( SCREEN_WIDTH, SCREEN_HEIGHT );
  }
  m_bgImage = g_bgImage;
  std::string line;
  while ( !in.eof() ) {
    getline( in, line );
    parseLine( line );
  }
  protect();
  return true;
}


void Scene::start( bool replay )
{
  activateAll();
  if ( replay ) {
    m_recorder.stop();
    m_player.start( &m_log, this );
  } else {
    m_player.stop();
    m_recorder.start( &m_log );
  }
}


bool Scene::parseLine( const std::string& line )
{
  try {
    switch( line[0] ) {
    case 'T': m_title = line.substr(line.find(':')+1);  return true;
    case 'B': m_bg = line.substr(line.find(':')+1);     return true;
    case 'A': m_author = line.substr(line.find(':')+1); return true;
    case 'S': m_strokes.push_back( new Stroke(line) );     return true;
    case 'G': setGravity(line);                         return true;
    case 'E': m_log.append(line.substr(line.find(':')+1));return true;
    }
  } catch ( const char* e ) {
      throw std::invalid_argument(std::string("Stroke error: ") + e);
  }
  return false;
}

void Scene::protect( int n )
{
  m_protect = (n==-1 ? m_strokes.size() : n );
}

bool Scene::save( const std::string& file, bool saveLog )
{
  std::ofstream o( file.c_str(), std::ios::out );
  if ( o.is_open() ) {
    o << "Title: "<<m_title<<std::endl;
    o << "Author: "<<m_author<<std::endl;
    o << "Background: "<<m_bg<<std::endl;
    for ( size_t i=0; i<m_strokes.size() && (!saveLog || i< static_cast<size_t>(m_protect)); i++ ) {
	o << m_strokes[i]->asString();
    }

    if ( saveLog ) {      
      for ( size_t i=0; i<m_log.size(); i++ ) {
	o << "E: " << m_log.asString( i ) <<std::endl;
      }
    }

    o.close();
    return true;
  } else {
    return false;
  }
}


Image *Scene::g_bgImage = NULL;

