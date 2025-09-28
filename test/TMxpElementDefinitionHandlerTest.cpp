/***************************************************************************
 *   Copyright (C) 2025 by Nicolas Keita - nicolaskeita2@@gmail.com        *
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

#include <QtTest/QtTest>
#include <QString>
#include <QStringList>
#include "TMxpStubClient.h"
#include "TMxpProcessor.h"
#include "TMxpElementRegistry.h"

class TmxpElementDefinitionHandlerTest : public QObject {
    Q_OBJECT

private slots:
    void testMxpElementRegistration_data()
    {
        QTest::addColumn<QString>("name");
        QTest::addColumn<QString>("inputString");
        QTest::addColumn<QStringList>("expectedAttrs");
        QTest::addColumn<QString>("expectedFlags");
        QTest::addColumn<bool>("expectedOpen");
        QTest::addColumn<bool>("expectedEmpty");
        QTest::addColumn<int>("expectedParsedDefinitionCount");

        QStringList attrs1;
        QTest::newRow("Test1")
            << "Test1"
            << R"(<!ELEMENT Test1 FLAG='Weird & Co'>)"
            << attrs1
            << QString("Weird & Co")
            << false
            << false
            << 0;

        QStringList attrs2 = {"name", "type"};
        QTest::newRow("Test2")
            << "Test2"
            << R"(<!ELEMENT Test2 ATT="NAME=John TYPE=Admin" FLAG='SpecialFlag' OPEN EMPTY>)"
            << attrs2
            << QString("SpecialFlag")
            << true
            << true
            << 0;

        QStringList attrs3;
        QTest::newRow("Test3")
            << "Test3"
            << R"(<!ELEMENT Test3 '<tag1 attr="val1"><tag2 attr="val2"><tag3 attr="val3">'>)"
            << attrs3
            << QString()
            << false
            << false
            << 3;
    }

    void testMxpElementRegistration()
    {
        QFETCH(QString, name);
        QFETCH(QString, inputString);
        QFETCH(QStringList, expectedAttrs);
        QFETCH(bool, expectedOpen);
        QFETCH(bool, expectedEmpty);
        QFETCH(int, expectedParsedDefinitionCount);
        TMxpStubClient client;
        TMxpProcessor mxpProcessor(&client);

        for (QChar c : inputString) {
            char ch = c.toLatin1();
            mxpProcessor.processMxpInput(ch, true);
        }

        const TMxpElementRegistry &registry = mxpProcessor.getMxpTagProcessor().getElementRegistry();
        QVERIFY(registry.containsElement(name));
        const TMxpElement &element = registry.getElement(name);
        QCOMPARE(element.attrs, expectedAttrs);
        QCOMPARE(element.open, expectedOpen);
        QCOMPARE(element.empty, expectedEmpty);
        QCOMPARE(element.parsedDefinition.size(), expectedParsedDefinitionCount);
    }

    void testParserDoesNotFreezeOnAmpersand_data() {
        QTest::addColumn<QString>("messageWithAmpersand");

        QTest::newRow("no amp") << "abc";
        QTest::newRow("solo amp") << "&\n";
        QTest::newRow("amp followed by space") << "& followed by space\n";
        QTest::newRow("amp at start no semicolon") << "&start\n";
        QTest::newRow("amp + newline") << "line with &\nnext line\n";
        QTest::newRow("amp + numbers") << "version &123;\n";
        QTest::newRow("amp + special chars") << "weird &entity-._#;\n";
        QTest::newRow("multiple consecutive amps") << "text &&&& more text\n";
        QTest::newRow("only amps") << "&&&&\n";
        QTest::newRow("amp + emoji") << "smile &😊;\n";
    }

    void testParserDoesNotFreezeOnAmpersand()
    {
        TMxpStubClient          stub;
        TMxpProcessor           mxpProcessor(&stub);
        TMxpProcessingResult    lastResult = HANDLER_FALL_THROUGH;
        QFETCH(QString, messageWithAmpersand);

        for (char c : messageWithAmpersand.toStdString()) {
            lastResult = mxpProcessor.processMxpInput(c, true);
        }

        QVERIFY(lastResult != HANDLER_NEXT_CHAR);
    }

    void testParserDoesNotFreezeOnUnescapedTagStart_data()
    {
        QTest::addColumn<QString>("input_string");

        QTest::newRow("starts_with_less_than") << "<test<!EN ob \"lamp\">";
        QTest::newRow("less_than_second_char") << "a<test<!EN ob \"lamp\">";
        QTest::newRow("multiple_less_than_before_valid_tag") << "<<<randomtext<!EN ob \"lamp\">";
    }

    void testParserDoesNotFreezeOnUnescapedTagStart() {

        TMxpStubClient  stub;
        TMxpProcessor   mxpProcessor(&stub);
        QFETCH(QString, input_string);

        for (char c : input_string.toStdString()) {
            mxpProcessor.processMxpInput(c, true);
        }
        QCOMPARE(mxpProcessor.getMxpTagProcessor().getEntityResolver().getResolution("&ob;"), "lamp");
    }
};

#include "TMxpElementDefinitionHandlerTest.moc"
QTEST_MAIN(TmxpElementDefinitionHandlerTest)