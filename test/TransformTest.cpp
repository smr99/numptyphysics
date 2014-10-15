#include "Scene.h"
#include <gtest/gtest.h>
#include "TestCommon.h"


TEST(TransformVec2, identity)
{
    Transform trans(1, 0, Vec2(0, 0));

    Vec2 v(7, 99);
    trans.transform(v);
    ASSERT_EQ(Vec2(7, 99), v);

    trans.inverseTransform(v);
    ASSERT_EQ(Vec2(7, 99), v);
}

TEST(TransformVec2, scale)
{
    Transform trans(4, 0, Vec2(0, 0));

    Vec2 v(7, 99);
    trans.transform(v);
    ASSERT_EQ(Vec2(4*7, 4*99), v);

    trans.inverseTransform(v);
    ASSERT_EQ(Vec2(7, 99), v);
}

TEST(TransformVec2, rotate)
{
    Transform trans(1, M_PI/2.0, Vec2(0, 0));

    Vec2 v(512, -1000);
    trans.transform(v);
    ASSERT_EQ(Vec2(1000, 512), v);

    trans.inverseTransform(v);
    ASSERT_VEC2_NEAR(Vec2(512, -1000), v);
}

TEST(TransformVec2, translate)
{
    Transform trans(1, 0, Vec2(1, 3));

    Vec2 v(7, -1000);
    trans.transform(v);
    ASSERT_EQ(Vec2(8, -997), v);

    trans.inverseTransform(v);
    ASSERT_EQ(Vec2(7, -1000), v);
}

TEST(TransformVec2, general)
{
    Transform trans(8, M_PI, Vec2(1, 3));

    Vec2 v(7, -1000);
    trans.transform(v);
    ASSERT_EQ(Vec2(8 * (-7) + 1, 8 * 1000 + 3), v);

    trans.inverseTransform(v);
    ASSERT_VEC2_NEAR(Vec2(7, -1000), v);
}

TEST(TransformPath, input_is_preserved)
{
    Path p("0,0 5,0 5,5 0,5");

    Transform trans(8, M_PI, Vec2(1, 3));
    Path pTrans;
    
    trans.transform(p, pTrans);
    ASSERT_EQ(4, p.size());
    ASSERT_EQ(4, pTrans.size());
    
    ASSERT_EQ(Vec2(0,0), p[0]);
    ASSERT_EQ(Vec2(5,0), p[1]);
    ASSERT_EQ(Vec2(5,5), p[2]);
    ASSERT_EQ(Vec2(0,5), p[3]);
}

TEST(TransformPath, general)
{
    Path p("0,0 128,0 0,128");

    Transform trans(8, M_PI/2.0, Vec2(1, 3));
    Path pTrans;
    
    trans.transform(p, pTrans);
    
    ASSERT_EQ(Vec2(1, 3), pTrans[0]);
    ASSERT_EQ(Vec2(8*0+1, 8*128+3), pTrans[1]);
    ASSERT_EQ(Vec2(8*-128+1, 8*0+3), pTrans[2]);
}
