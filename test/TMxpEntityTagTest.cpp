#include <MxpTag.h>
#include <QTest>
#include <TMxpEntityTagHandler.h>
#include <TMxpTagParser.h>
#include <TMxpTagProcessor.h>
#include <TEntityResolver.h>

#include "TMxpStubClient.h"



class TMxpEntityTagTest : public QObject {
    Q_OBJECT

private:
private slots:

    void initTestCase()
    {}

    void testEntityTagNew()
    {
		// set a new entity:
		
        TMxpTagParser parser;
		TMxpStubContext ctx;
        TMxpStubClient stub;
     
        MxpStartTag* entityTag = parser.parseStartTag("<!EN CDIR 'filename'>");
    
        TMxpEntityTagHandler entityTagHandler;
        TMxpTagHandler& tagHandler = entityTagHandler;
        tagHandler.handleTag(ctx, stub, entityTag);

        QCOMPARE(ctx.getEntityResolver().interpolate("cd &cdir;"), "cd filename");
    }

    void testEntityTagRedefine()
    {
		// redefine existing entity:
		
        TMxpTagParser parser;
		TMxpStubContext ctx;
        TMxpStubClient stub;
     
        MxpStartTag* entityTag = parser.parseStartTag("<!EN Atom 'org value'>");
    
        TMxpEntityTagHandler entityTagHandler;
        TMxpTagHandler& tagHandler = entityTagHandler;
        tagHandler.handleTag(ctx, stub, entityTag);
		
		QCOMPARE(ctx.getEntityResolver().interpolate("cd &ATOM;"), "cd org value");
		
		entityTag = parser.parseStartTag("<!entity atom 'new value'>");
		tagHandler.handleTag(ctx, stub, entityTag);

        QCOMPARE(ctx.getEntityResolver().interpolate("cd &ATOM;"), "cd new value");
    }
	
	void testEntityTagEmpty()
    {
		// set an entity to empty string: Note that the direct call
		// to parseStartTag does not handle '' as in <!EN item ''> well.
		// In the real mudlet code '' is resolved to empty string in advance
		
        TMxpTagParser parser;
		TMxpStubContext ctx;
        TMxpStubClient stub;
     
        MxpStartTag* entityTag = parser.parseStartTag("<!en item torch>");
    
        TMxpEntityTagHandler entityTagHandler;
        TMxpTagHandler& tagHandler = entityTagHandler;
        tagHandler.handleTag(ctx, stub, entityTag);

		QCOMPARE(ctx.getEntityResolver().interpolate("examine &Item;"), "examine torch");

		entityTag = parser.parseStartTag("<!Entity item>");
		tagHandler.handleTag(ctx, stub, entityTag);

        QCOMPARE(ctx.getEntityResolver().interpolate("examine &Item;"), "examine ");
    }

	void testEntityTagDelete()
    {
		// remove an existing entity:
		
        TMxpTagParser parser;
		TMxpStubContext ctx;
        TMxpStubClient stub;
     
        MxpStartTag* entityTag = parser.parseStartTag("<!EN weapon sword>");
    
        TMxpEntityTagHandler entityTagHandler;
        TMxpTagHandler& tagHandler = entityTagHandler;
        tagHandler.handleTag(ctx, stub, entityTag);

        QCOMPARE(ctx.getEntityResolver().interpolate("wield &weapon;"), "wield sword");
		
		entityTag = parser.parseStartTag("<!Entity Weapon Delete>");
		tagHandler.handleTag(ctx, stub, entityTag);

        QCOMPARE(ctx.getEntityResolver().interpolate("wield &weapon;"), "wield &weapon;");
    }
	
    void cleanupTestCase()
    {}
};

#include "TMxpEntityTagTest.moc"
QTEST_MAIN(TMxpEntityTagTest)
