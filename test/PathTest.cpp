#include "Path.h"
#include <gtest/gtest.h>
#include <ostream>

std::ostream& operator<<(std::ostream& os, const Vec2& v)
{
    return os << "Vec2(" << v.x << "," << v.y << ")";
}


std::ostream& operator<<(std::ostream& os, const Path& p)
{
    os << "Path(n=" << p.numPoints();
    for(int i = 0; i < std::min(5, p.numPoints()); ++i)
    {
	os << ", " << p[i];
    }
    
    return os << ")";
}


TEST(Path, constructor_trivial)
{
    Path p;
}

TEST(Path, constructor_vector)
{
    Vec2 v;
    Path p(1, &v);
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

TEST(Path, translate)
{
    Path p("0,0 1,1");
    p.translate(Vec2(3,-3));
    
    ASSERT_EQ(Vec2(3,-3), p[0]);
    ASSERT_EQ(Vec2(4,-2), p[1]);
}

TEST(Path, rotate)
{
    Path p("0,0 1,1");
    p.rotate(b2Mat22(0, -1, 1, 0));
    
    ASSERT_EQ(Vec2(0,0), p[0]);
    ASSERT_EQ(Vec2(-1,1), p[1]);
}
