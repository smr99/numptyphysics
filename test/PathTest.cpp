#include "Path.h"
#include <gtest/gtest.h>
#include "TestCommon.h"


using namespace std;


TEST(Path, constructor_trivial)
{
    Path p;
}

TEST(Path, constructor_string)
{
    Path p("0,0 5,0 5,5 0,5");
    ASSERT_EQ(4, p.size());
    
    ASSERT_EQ(Vec2(0,0), p[0]);
    ASSERT_EQ(Vec2(5,0), p[1]);
    ASSERT_EQ(Vec2(5,5), p[2]);
    ASSERT_EQ(Vec2(0,5), p[3]);
}

TEST(Path, constructor_copy)
{
    Path p("0,0 5,0 5,5 0,5");
    Path q(p);
    
    ASSERT_EQ(Vec2(0,0), q[0]);
    ASSERT_EQ(Vec2(5,0), q[1]);
    ASSERT_EQ(Vec2(5,5), q[2]);
    ASSERT_EQ(Vec2(0,5), q[3]);

    ASSERT_NE(&p[0], &q[0]);
    ASSERT_NE(&p[1], &q[1]);
    ASSERT_NE(&p[2], &q[2]);
    ASSERT_NE(&p[3], &q[3]);   
}

TEST(Path, copy_operator)
{
    Path p("0,0 5,0 5,5 0,5");
    Path q = p;
    
    ASSERT_EQ(Vec2(0,0), q[0]);
    ASSERT_EQ(Vec2(5,0), q[1]);
    ASSERT_EQ(Vec2(5,5), q[2]);
    ASSERT_EQ(Vec2(0,5), q[3]);

    ASSERT_NE(&p[0], &q[0]);
    ASSERT_NE(&p[1], &q[1]);
    ASSERT_NE(&p[2], &q[2]);
    ASSERT_NE(&p[3], &q[3]);   
}

TEST(Path, translate)
{
    Path p("0,0 1,1");
    p.translate(Vec2(3,-3));
    
    ASSERT_EQ(Vec2(3,-3), p[0]);
    ASSERT_EQ(Vec2(4,-2), p[1]);
}

TEST(Path, rotate_b2Mat22)
{
    Path p("0,0 1,1");
    p.rotate(b2Mat22(0, -1, 1, 0));
    
    ASSERT_EQ(Vec2(0,0), p[0]);
    ASSERT_EQ(Vec2(-1,1), p[1]);
}

TEST(Path, rotate_b2Rot)
{
    Path p("0,0 100,0");
    p.rotate(b2Rot(M_PI_2));
    
    ASSERT_EQ(Vec2(0,0), p[0]);
    ASSERT_EQ(Vec2(0,100), p[1]);
}

TEST(Path, append)
{
    Path p("0,0 100,0");
    Vec2 v(1, 2);

    p.append(v);
    ASSERT_EQ(3, p.size());
    ASSERT_EQ(v, p[2]);
    ASSERT_NE(&v, &p[2]);
}

TEST(Path, trim)
{
    Path p("0,0 1,1 2,2 3,3 4,4");
    ASSERT_EQ(5, p.size());

    p.trim(3);
    ASSERT_EQ(2, p.size());
    ASSERT_EQ(Vec2(0,0), p[0]);
    ASSERT_EQ(Vec2(1,1), p[1]);    
}

TEST(Path, erase)
{
    Path p("0,0 1,1 2,2 3,3");

    p.erase(1);
    ASSERT_EQ(3, p.size());
    ASSERT_EQ(Vec2(0,0), p[0]);
    ASSERT_EQ(Vec2(2,2), p[1]);    
    ASSERT_EQ(Vec2(3,3), p[2]);    
}

