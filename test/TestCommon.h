#include "Common.h"
#include "Path.h"
#include <ostream>

inline std::ostream& operator<<(std::ostream& os, const Vec2& v)
{
    return os << "Vec2(" << v.x << "," << v.y << ")";
}


inline std::ostream& operator<<(std::ostream& os, const Path& p)
{
    os << "Path(n=" << p.numPoints();
    for(int i = 0; i < std::min(5, p.numPoints()); ++i)
    {
	os << ", " << p[i];
    }
    
    return os << ")";
}


/// Assert vectors differ by at most one in each component
inline void ASSERT_VEC2_NEAR(const Vec2& expected, const Vec2& actual)
{
    Vec2 diff = expected - actual;
    ASSERT_LE(std::abs(diff.x), 1);
    ASSERT_LE(std::abs(diff.y), 1);
}
