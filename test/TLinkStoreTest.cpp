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

#include <TLinkStore.h>
#include <QtTest/QtTest>

class TLinkStoreTest : public QObject {
Q_OBJECT

private:



    void initTestCase()
    {
    }

    static void testAddAndGet()
    {
        TLinkStore store(3);

        QStringList links;
        links.append("GET &href;");
        links.append("LOOK &href;");

        QStringList hints;
        hints.append("Get &href;");
        hints.append("Look at &href;");

        int id = store.addLinks(links, hints);

        QStringList links2 = store.getLinks(id);
        QCOMPARE(links2, links);
        QCOMPARE(links2[1], "LOOK &href;");

        QStringList hints2 = store.getHints(id);
        QCOMPARE(hints2, hints);
        QCOMPARE(hints2[1], "Look at &href;");
    }

    static void testNewGeneratedID()
    {
        TLinkStore store(3);

        QStringList links;

        store.addLinks(links, links);
        QCOMPARE(store.getCurrentLinkID(), 1);

        store.addLinks(links, links);
        QCOMPARE(store.getCurrentLinkID(), 2);

        store.addLinks(links, links);
        QCOMPARE(store.getCurrentLinkID(), 3);

        store.addLinks(links, links);
        QCOMPARE(store.getCurrentLinkID(), 1);

        store.addLinks(links, links);
        QCOMPARE(store.getCurrentLinkID(), 2);
    }

    static void testMaxId()
    {
        TLinkStore store(3);

        QStringList links;
        links.append("GET &href;");

        store.addLinks(links, links);
        store.addLinks(links, links);
        store.addLinks(links, links);

        QCOMPARE(store.getCurrentLinkID(), 3);
        QCOMPARE(store.getLinks(3), links);
    }

    void cleanupTestCase()
    {
    }
};

#include "TLinkStoreTest.moc"
QTEST_MAIN(TLinkStoreTest)
