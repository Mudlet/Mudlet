#include <MxpTag.h>
#include <QTest>
#include <TMxpVersionTagHandler.h>
#include <TMxpTagParser.h>
#include <TMxpTagProcessor.h>
#include <TEntityResolver.h>

#include "TMxpStubClient.h"



class TMxpVersionTagTest : public QObject {
    Q_OBJECT

private:
private slots:

    void initTestCase()
    {}

    void testVersionTag()
    {
		// Vanilla use of version
		
        TMxpTagParser parser;
		TMxpStubContext ctx;
        TMxpStubClient stub;
     
        MxpStartTag* versionTag = parser.parseStartTag("<Version>");
    
        TMxpVersionTagHandler versionTagHandler;
        TMxpTagHandler& tagHandler = versionTagHandler;
        tagHandler.handleTag(ctx, stub, versionTag);

        QCOMPARE(stub.sentToServer, "\n\u001B[1z<VERSION MXP=1.0 CLIENT=Mudlet VERSION=Stub-1.0>\n");
    }
	
	 void testVersionStyle()
    {
		// Set a Style (Version of MXP Template of mud)
		
        TMxpTagParser parser;
		TMxpStubContext ctx;
        TMxpStubClient stub;
     
		// style is generally just a string, but compared to other implementations we must at least allow for full 9 digit
		// (which always fit into a 32bit int)
        MxpStartTag* versionTag = parser.parseStartTag("<Version 987654321>");
    
        TMxpVersionTagHandler versionTagHandler;
        TMxpTagHandler& tagHandler = versionTagHandler;
        tagHandler.handleTag(ctx, stub, versionTag);

		// NO return value when setting style (this is a grey area.. but Z/CMUD do it this way and the MXP definition
		// seems to imply this interpretation:
		// The client caches this version information and returns it when requested by a plain <VERSION> request.
		// One can interprete this like, it ONLY returns it when using a plain request... but...)
        QCOMPARE(stub.sentToServer, "");
		
		// From now on, return it with version
		versionTag = parser.parseStartTag("<VERSION>");
		tagHandler.handleTag(ctx, stub, versionTag);
		QCOMPARE(stub.sentToServer, "\n\u001B[1z<VERSION MXP=1.0 CLIENT=Mudlet VERSION=Stub-1.0 STYLE=987654321>\n");
    }
	
    void cleanupTestCase()
    {}
};

#include "TMxpVersionTagTest.moc"
QTEST_MAIN(TMxpVersionTagTest)
