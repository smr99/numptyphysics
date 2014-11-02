#include "Levels.h"
#include <gtest/gtest.h>
#include "TestCommon.h"


using namespace std;


TEST(Levels, constructor_trivial)
{
    Levels l;
}

TEST(Levels, empty)
{
    Levels l;
    
    ASSERT_EQ(0, l.numLevels());
    
    ASSERT_EQ(string("err"), l.levelName(0, false));
    ASSERT_EQ(string("err"), l.levelName(0, true));
    
    ASSERT_LT(l.findLevel("non-existent"), 0);
    ASSERT_EQ(0, l.numCollections());
    ASSERT_LT(l.collectionFromLevel(0), 0);

    ASSERT_EQ(0, l.collectionSize(0));
    ASSERT_EQ(0, l.collectionSize(1));

    ASSERT_EQ(0, l.collectionLevel(0, 0));

    ASSERT_FALSE(l.demoPath(0).empty());
    ASSERT_FALSE(l.demoName(0).empty());
    ASSERT_FALSE(l.hasDemo(0));
}

TEST(Levels, load_nonexistent)
{
    Levels l;
    
    l.addPath("/a/non/existent/path");
    ASSERT_EQ(0, l.numLevels());

    l.addPath("/a");
    ASSERT_EQ(0, l.numLevels());
}

TEST(Levels, load_nph)
{
    Levels l;
    
    l.addPath("data/L00_title.nph");
    ASSERT_EQ(1, l.numLevels());
    
    ASSERT_STREQ("title", l.levelName(0).c_str());
}

TEST(Levels, load_npz)
{
    Levels l;
    
    l.addPath("data/C10_Standard.npz");
    ASSERT_EQ(9, l.numLevels());
    
    ASSERT_STREQ("plane sailing", l.levelName(0).c_str());
    ASSERT_STREQ("the leap", l.levelName(1).c_str());
    ASSERT_STREQ("nautilus", l.levelName(8).c_str());
    
    ASSERT_EQ(1, l.numCollections());
    ASSERT_EQ(9, l.collectionSize(0));
    
    int indexInCol;
    ASSERT_EQ(0, l.collectionFromLevel(0, &indexInCol));
    ASSERT_EQ(0, indexInCol);

    ASSERT_EQ(0, l.collectionFromLevel(1, &indexInCol));
    ASSERT_EQ(1, indexInCol);

}

