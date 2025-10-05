#include <TLinkStore.h>
#include <QtTest/QtTest>

class TLinkStoreTest : public QObject {
Q_OBJECT

private:

private slots:

    void initTestCase()
    {
    }

    void testAddAndGet()
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

    void testNewGeneratedID()
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

    void testMaxId()
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
