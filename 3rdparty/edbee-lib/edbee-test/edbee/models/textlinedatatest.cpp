/**
 * Copyright 2011-2013 - Reliable Bits Software by Blommers IT. All Rights Reserved.
 * Author Rick Blommers
 */

#include "textlinedatatest.h"

#include "edbee/models/textlinedata.h"

#include "edbee/debug.h"
#include "moc_textlinedatatest.cpp"

namespace edbee {

static QString filledTestString( const TextLineDataManager& ldm )
{
    QString str;
    for( int i=0; i<ldm.length(); ++i ) {
        if( ldm.at(i) ) {
            str.append("X");
        } else {
            str.append("-");
        }
    }
    return str;
}

/// A special class for detecting 'destructor' and destroy calls
class DestructorTestList : public TextLineDataList
{
public:
    DestructorTestList( int& destroyRef, int& destructorRef) : destroyCallCountRef_(destroyRef), destructorCallCountRef_(destructorRef) {}
    virtual ~DestructorTestList() { ++destructorCallCountRef_; }
    virtual void destroy( TextLineDataManager* manager ) { Q_UNUSED(manager); ++destroyCallCountRef_; }
private:
    int& destroyCallCountRef_;
    int& destructorCallCountRef_;
};



/// Tests the line data manager
void TextLineDataTest::testLineDataManager()
{

    TextLineDataManager ldm;
    testEqual( filledTestString(ldm ), "-");

    // insert a line
    ldm.fillWithEmpty(0,0,2);
    testEqual( filledTestString(ldm ), "---");

    // set an item
    int destroyCount=0;
    int destructCount=0;
    DestructorTestList* list = new DestructorTestList(destroyCount, destructCount);
    ldm.giveList(1, list );
    testEqual( filledTestString(ldm ), "-X-");
    testEqual( destroyCount, 0 );
    testEqual( destructCount, 0 );

    // inserting an item should move the data items
    ldm.fillWithEmpty(1,0,1);
    testEqual( filledTestString(ldm ), "--X-");
    testEqual( destroyCount, 0 );
    testEqual( destructCount, 0 );

    // taking a list item sould not call the destructor or destroy method
    testEqual( (quintptr)list, (quintptr)ldm.takeList(2) );
    testEqual( filledTestString(ldm ), "----");
    testEqual( destroyCount, 0 );
    testEqual( destructCount, 0 );

    // giving a list back should work
    ldm.giveList(1, list );
    testEqual( filledTestString(ldm ), "-X--");
    testEqual( destroyCount, 0 );
    testEqual( destructCount, 0 );

    // fill with empty call destroy and the destructor of the list
    ldm.fillWithEmpty(1,2,0);
    testEqual( filledTestString(ldm ), "--");
    testEqual( destroyCount, 1 );
    testEqual( destructCount, 1 );

}

} // edbee
