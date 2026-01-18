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

    void testRemoveUnreferencedLinks()
    {
        TLinkStore store(10);

        QStringList links1;
        links1.append("command1");
        QStringList hints1;
        hints1.append("hint1");

        QStringList links2;
        links2.append("command2");
        QStringList hints2;
        hints2.append("hint2");

        QStringList links3;
        links3.append("command3");
        QStringList hints3;
        hints3.append("hint3");

        int id1 = store.addLinks(links1, hints1);
        int id2 = store.addLinks(links2, hints2);
        int id3 = store.addLinks(links3, hints3);

        // Verify all links exist
        QCOMPARE(store.getLinksConst(id1), links1);
        QCOMPARE(store.getLinksConst(id2), links2);
        QCOMPARE(store.getLinksConst(id3), links3);

        // Simulate only id2 is still referenced in buffer
        QSet<int> referencedIds;
        referencedIds.insert(id2);

        store.removeUnreferencedLinks(referencedIds, nullptr);

        // id2 should still exist
        QCOMPARE(store.getLinksConst(id2), links2);
        QCOMPARE(store.getHintsConst(id2), hints2);

        // id1 and id3 should be removed
        QVERIFY(store.getLinksConst(id1).isEmpty());
        QVERIFY(store.getHintsConst(id1).isEmpty());
        QVERIFY(store.getLinksConst(id3).isEmpty());
        QVERIFY(store.getHintsConst(id3).isEmpty());
    }

    void testRemoveUnreferencedLinksEmpty()
    {
        TLinkStore store(10);

        QStringList links;
        links.append("command");
        QStringList hints;
        hints.append("hint");

        int id1 = store.addLinks(links, hints);
        int id2 = store.addLinks(links, hints);

        // Empty reference set - all links should be removed
        QSet<int> emptySet;
        store.removeUnreferencedLinks(emptySet, nullptr);

        QVERIFY(store.getLinksConst(id1).isEmpty());
        QVERIFY(store.getHintsConst(id1).isEmpty());
        QVERIFY(store.getLinksConst(id2).isEmpty());
        QVERIFY(store.getHintsConst(id2).isEmpty());
    }

    void testRemoveUnreferencedLinksNone()
    {
        TLinkStore store(10);

        QStringList links;
        links.append("command");
        QStringList hints;
        hints.append("hint");

        int id1 = store.addLinks(links, hints);
        int id2 = store.addLinks(links, hints);

        // Both links referenced - none should be removed
        QSet<int> referencedIds;
        referencedIds.insert(id1);
        referencedIds.insert(id2);

        store.removeUnreferencedLinks(referencedIds, nullptr);

        QCOMPARE(store.getLinksConst(id1), links);
        QCOMPARE(store.getHintsConst(id1), hints);
        QCOMPARE(store.getLinksConst(id2), links);
        QCOMPARE(store.getHintsConst(id2), hints);
    }

    void testRemoveUnreferencedLinksWithExpireNames()
    {
        TLinkStore store(10);

        QStringList links;
        links.append("command");
        QStringList hints;
        hints.append("hint");

        int id1 = store.addLinks(links, hints, nullptr, QVector<int>(), "expire_group");
        int id2 = store.addLinks(links, hints);

        // Verify expire name is set for id1
        QCOMPARE(store.getExpireName(id1), QString("expire_group"));
        QVERIFY(store.getExpireName(id2).isEmpty());

        // Simulate only id2 is still referenced in buffer
        QSet<int> referencedIds;
        referencedIds.insert(id2);

        store.removeUnreferencedLinks(referencedIds, nullptr);

        // id1 should be removed along with its expire name
        QVERIFY(store.getLinksConst(id1).isEmpty());
        QVERIFY(store.getHintsConst(id1).isEmpty());
        QVERIFY(store.getExpireName(id1).isEmpty());

        // id2 should still exist
        QCOMPARE(store.getLinksConst(id2), links);
        QCOMPARE(store.getHintsConst(id2), hints);
    }

    void testRemoveUnreferencedLinksWithLuaReferences()
    {
        TLinkStore store(10);

        QStringList links;
        links.append("command");
        QStringList hints;
        hints.append("hint");

        QVector<int> luaRefs;
        luaRefs.append(42);

        int id1 = store.addLinks(links, hints, nullptr, luaRefs);
        int id2 = store.addLinks(links, hints);

        // Simulate only id2 is still referenced in buffer
        QSet<int> referencedIds;
        referencedIds.insert(id2);

        store.removeUnreferencedLinks(referencedIds, nullptr);

        // id1 should be removed (freeReference is a no-op in test builds)
        QVERIFY(store.getLinksConst(id1).isEmpty());
        QVERIFY(store.getHintsConst(id1).isEmpty());

        // id2 should still exist
        QCOMPARE(store.getLinksConst(id2), links);
        QCOMPARE(store.getHintsConst(id2), hints);
    }

    void cleanupTestCase()
    {
    }
};

#include "TLinkStoreTest.moc"
QTEST_MAIN(TLinkStoreTest)
