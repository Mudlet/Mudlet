/***************************************************************************
 *   Copyright (C) 2022 by Gustavo Sousa - gustavocms@gmail.com            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "TMxpEntityTagHandler.h"
#include "TMxpStubClient.h"
#include <QTest>
#include <TMxpProcessor.h>
#include <TMxpTagParser.h>
#include <TMxpTagProcessor.h>

class TMxpEntityTagHandlerTest : public QObject {
  Q_OBJECT

private:
private slots:
  static QSharedPointer<MxpNode> parseNode(const QString &tagText) {
    auto nodes = TMxpTagParser::parseToMxpNodeList(tagText);
    return !nodes.empty() ? nodes.first() : nullptr;
  }

  void processInput(TMxpProcessor &processor, const std::string& input) {
    for (char ch : input) {
      processor.processMxpInput(ch, true);
    }
  }

  void testPublish() {
    TMxpStubClient stub;
    TMxpTagProcessor processor;

    auto tag = parseNode("<!EN ob \"street lamp\" publish>");
    processor.handleNode(processor, stub, tag.get());

    QCOMPARE(processor.getEntityResolver().getResolution("&ob;"),
             "street lamp");

    QCOMPARE(stub.mPublishedEntityName, "&ob;");
    QCOMPARE(stub.mPublishedEntityValue, "street lamp");
  }

  void testPrivate() {
    TMxpStubClient stub;
    TMxpTagProcessor processor;

    auto tag = parseNode("<!EN ob \"street lamp\" private>");
    processor.handleNode(processor, stub, tag.get());

    QCOMPARE(processor.getEntityResolver().getResolution("&ob;"),
             "street lamp");

    QCOMPARE(stub.mPublishedEntityName, "");
    QCOMPARE(stub.mPublishedEntityValue, "");
  }

  void testAddRemoveStart() {
    TMxpStubClient stub;
    TMxpTagProcessor processor;

    processor.getEntityResolver().registerEntity("&entity;", "v1");
    QCOMPARE(processor.getEntityResolver().getResolution("&entity;"), "v1");

    processor.handleNode(processor, stub,
                         parseNode("<!EN entity \"v2\" add>").get());
    QCOMPARE(processor.getEntityResolver().getResolution("&entity;"), "v1|v2");

    processor.handleNode(processor, stub,
                         parseNode("<!EN entity \"v1\" remove>").get());
    QCOMPARE(processor.getEntityResolver().getResolution("&entity;"), "v2");

    processor.handleNode(processor, stub,
                         parseNode("<!EN entity \"v2\" remove>").get());
    QCOMPARE(processor.getEntityResolver().getResolution("&entity;"), "");
  }

  void testAddRemoveEnd() {
    TMxpStubClient stub;
    TMxpTagProcessor processor;

    processor.getEntityResolver().registerEntity("&entity;", "v1");
    QCOMPARE(processor.getEntityResolver().getResolution("&entity;"), "v1");

    processor.handleNode(processor, stub,
                         parseNode("<!EN entity \"v2\" add>").get());
    QCOMPARE(processor.getEntityResolver().getResolution("&entity;"), "v1|v2");

    processor.handleNode(processor, stub,
                         parseNode("<!EN entity \"v2\" remove>").get());
    QCOMPARE(processor.getEntityResolver().getResolution("&entity;"), "v1");
  }

  void testAddRemovePartOfItemValue() {
    TMxpStubClient stub;
    TMxpTagProcessor processor;

    processor.getEntityResolver().registerEntity("&entity;", "my text example");
    QCOMPARE(processor.getEntityResolver().getResolution("&entity;"),
             "my text example");

    processor.handleNode(processor, stub,
                         parseNode("<!EN entity \"my value\" add>").get());
    QCOMPARE(processor.getEntityResolver().getResolution("&entity;"),
             "my text example|my value");

    processor.handleNode(processor, stub,
                         parseNode("<!EN entity \"my text\" remove>").get());
    QCOMPARE(processor.getEntityResolver().getResolution("&entity;"),
             "my text example|my value");

    processor.handleNode(processor, stub,
                         parseNode("<!EN entity \"example\" remove>").get());
    QCOMPARE(processor.getEntityResolver().getResolution("&entity;"),
             "my text example|my value");

    processor.handleNode(processor, stub,
                         parseNode("<!EN entity \"value\" remove>").get());
    QCOMPARE(processor.getEntityResolver().getResolution("&entity;"),
             "my text example|my value");

    processor.handleNode(processor, stub,
                         parseNode("<!EN entity \"my\" remove>").get());
    QCOMPARE(processor.getEntityResolver().getResolution("&entity;"),
             "my text example|my value");

    processor.handleNode(processor, stub,
                         parseNode("<!EN entity \"my v\" remove>").get());
    QCOMPARE(processor.getEntityResolver().getResolution("&entity;"),
             "my text example|my value");
  }

  void testInterpolation() {
    TMxpStubClient stub;
    TMxpProcessor processor(&stub);
    processor.setMode(6); // !EN and SEND are SECURE tags

    std::string input =
        "<!EN ob \"street lamp\" private><send href=\"kill &ob;\">kill</send>";
    processInput(processor, input);

    QCOMPARE(stub.mHrefs.size(), 1);
    QCOMPARE(stub.mHrefs[0], "send([[kill street lamp]])");

    QCOMPARE(stub.mHints.size(), 1);
    QCOMPARE(stub.mHints[0], "kill street lamp");
  }

  void testDelete() {
    TMxpStubClient stub;
    TMxpTagProcessor processor;

    processor.getEntityResolver().registerEntity("&entity;", "v1");
    QCOMPARE(processor.getEntityResolver().getResolution("&entity;"), "v1");

    processor.handleNode(processor, stub,
                         parseNode("<!EN entity delete>").get());
    QCOMPARE(processor.getEntityResolver().getResolution("&entity;"),
             "&entity;");

    processor.getEntityResolver().registerEntity("&myEntity;", "v2");
    QCOMPARE(processor.getEntityResolver().getResolution("&myEntity;"), "v2");

    processor.handleNode(processor, stub,
                         parseNode("<!EN myEntity delete>").get());
    QCOMPARE(processor.getEntityResolver().getResolution("&myEntity;"),
             "&myEntity;");
  }

  void testEntityModification() {
    TMxpStubClient stub;
    TMxpTagProcessor processor;

    processor.getEntityResolver().registerEntity("&entity;", "v1");
    QCOMPARE(processor.getEntityResolver().getResolution("&entity;"), "v1");

    processor.handleNode(processor, stub, parseNode("<!EN entity ''>").get());
    QCOMPARE(processor.getEntityResolver().getResolution("&entity;"), "");

    processor.handleNode(processor, stub, parseNode("<!EN entity V2>").get());
    QCOMPARE(processor.getEntityResolver().getResolution("&entity;"), "V2");

    processor.handleNode(processor, stub, parseNode("<!EN entity>").get());
    QCOMPARE(processor.getEntityResolver().getResolution("&entity;"), "");

    processor.handleNode(processor, stub, parseNode("<!EN entity V3>").get());
    QCOMPARE(processor.getEntityResolver().getResolution("&entity;"), "V3");

    processor.handleNode(processor, stub, parseNode("<!EN entity \"\">").get());
    QCOMPARE(processor.getEntityResolver().getResolution("&entity;"), "");
  }

  void testEmptyEntityInterpolation() {
    TMxpStubClient stub;
    TMxpProcessor mxpProcessor(&stub);
    mxpProcessor.setMode(6);

    std::string input = "<!en entity ''><send href=\"examine ob&entity;\" "
                        "hint=\"examine&entity;\">examine</send>";
    processInput(mxpProcessor, input);

    QCOMPARE(stub.mHrefs.size(), 1);
    QCOMPARE(stub.mHrefs[0], "send([[examine ob]])");

    QCOMPARE(stub.mHints.size(), 1);
    QCOMPARE(stub.mHints[0], "examine");
  }
};

#include "TMxpEntityTagHandlerTest.moc"
QTEST_MAIN(TMxpEntityTagHandlerTest)
