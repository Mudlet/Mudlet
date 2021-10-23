/***************************************************************************
 *   Copyright (C) 2020 by Gustavo Sousa - gustavocms@gmail.com            *
 *   Copyright (C) 2021 by Florian Scheel - keneanung@gmail.com            *
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


#include <TEntityHandler.h>
#include <QtTest/QtTest>

class TEntityHandlerTest : public QObject {
Q_OBJECT

private:

private slots:

    void initTestCase()
    {
    }

    void testHandleSimpleString()
    {
        TEntityHandler handler;

        std::string str = "Someone says &quot;Hello&quot;";
        std::string result = processString(handler, str);

        QCOMPARE(result.c_str(), "Someone says \"Hello\"");
    }

    void testHandleUnfinishedString()
    {
        TEntityHandler handler;

        std::string str = "Someone says &quot;Hello&qu";
        std::string result = processString(handler, str);

        QCOMPARE(result.c_str(), "Someone says \"Hello");
    }

    void testLongSequence()
    {
        TEntityHandler handler;

        std::string str = "Long sequence: &01234567; asdf";
        std::string result = processString(handler, str);

        QCOMPARE(result.c_str(), "Long sequence: ; asdf");
    }

    void testHandleSplitInTwoParts()
    {
        TEntityHandler handler;

        std::string part1 = "Someone says &quot;Hello&qu";
        std::string result1 = processString(handler, part1);
        QCOMPARE(result1.c_str(), "Someone says \"Hello");

        std::string part2 = "ot; to you";
        std::string result2 = processString(handler, part2);
        QCOMPARE(result2.c_str(), "\" to you");

        std::string result = result1 + result2;

        qDebug() << result1.c_str();
        qDebug() << result2.c_str();
        qDebug() << result.c_str();

        QCOMPARE(result.c_str(), "Someone says \"Hello\" to you");
    }

    inline static std::string processString(TEntityHandler& handler, std::string& str)
    {
        size_t len = str.length();
        std::string result;
        for (size_t i = 0; i < len; i++) {
            if (!handler.handle(str[i])) {
                result += str[i];
            } else if (handler.isEntityResolved()) {
                result += handler.getResultAndReset();
            }
        }
        return result;
    }

    void cleanupTestCase()
    {
    }
};
#include "TEntityHandlerTest.moc"
QTEST_MAIN(TEntityHandlerTest)
