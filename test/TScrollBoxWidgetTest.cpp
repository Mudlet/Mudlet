/***************************************************************************
 *   Copyright (C) 2026 by gesslar                                         *
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

// The scroll range follows the content widget's size, and Lua cannot read it.

#include "GroupedTest.h"
#include "TScrollBox.h"

#include <QPointer>
#include <QtTest/QtTest>

class TScrollBoxWidgetTest : public QObject
{
    Q_OBJECT

private slots:
    void test_deletedChildStopsCounting()
    {
        TScrollBoxWidget content;
        auto top = new QWidget(&content);
        top->setGeometry(0, 0, 20, 20);
        QPointer<QWidget> bottom = new QWidget(&content);
        bottom->setGeometry(0, 200, 20, 20);
        content.adjustSize();
        QCOMPARE(content.height(), 220);

        // As deleteLabel() does it
        bottom->deleteLater();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QVERIFY(bottom.isNull());

        QCOMPARE(content.height(), 20);
    }

    void test_childMovedElsewhereStopsCounting()
    {
        TScrollBoxWidget content;
        QWidget elsewhere;
        auto top = new QWidget(&content);
        top->setGeometry(0, 0, 20, 20);
        auto bottom = new QWidget(&content);
        bottom->setGeometry(0, 200, 20, 20);
        content.adjustSize();
        QCOMPARE(content.height(), 220);

        bottom->setParent(&elsewhere);

        QCOMPARE(content.height(), 20);
    }
};

#include "TScrollBoxWidgetTest.moc"
MUDLET_GROUPED_TEST_MAIN(TScrollBoxWidgetTest)
