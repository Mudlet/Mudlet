/***************************************************************************
 *   Copyright (C) 2026 by Vadim Peretokin - vadim.peretokin@mudlet.org    *
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

/*
 * TMap tells the mapper drawing it (TMap::mpMapper) what changed through
 * signals, and dlgMapper acts on them. Each case here is one of those cues and
 * what the drawing mapper does with it, plus the rule that a second mapper of
 * the same profile, which is not drawing the map, ignores them.
 *
 * What the mapper does - its area dropdown, its selection list, its repaints,
 * its palette - is widget state no Lua function reads, so this cannot be a
 * busted spec.
 *
 * Run with: ctest -R MapViewCueTest -V
 */

#include <QApplication>
#include <QComboBox>
#include <QDir>
#include <QPalette>
#include <QScopeGuard>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTreeWidgetItem>
#include <QtTest/QtTest>

#include <memory>

#include "Host.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "T2DMap.h"
#include "TMap.h"
#include "TRoomDB.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgMapper.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

namespace {
const QByteArray scmMapXml = QByteArrayLiteral("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                                               "<map>\n"
                                               " <areas>\n"
                                               "  <area id=\"1\" name=\"Imported Area\"/>\n"
                                               " </areas>\n"
                                               " <rooms>\n"
                                               "  <room id=\"1\" area=\"1\" title=\"Entrance\" environment=\"2\">\n"
                                               "   <coord x=\"0\" y=\"0\" z=\"0\"/>\n"
                                               "  </room>\n"
                                               " </rooms>\n"
                                               "</map>\n");
} // namespace

class MapViewCueTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QTemporaryDir mMapDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    dlgMapper* mpMapper = nullptr;
    T2DMap* mp2dMap = nullptr;
    int mPaintCount = 0;
    const QString mProfileName = qsl("MapViewCue-Test");
    const QString mLocalhost = qsl("localhost");
    QString mPort;

    void deleteProfileDirectory() const
    {
        QDir dir(MudletApp::getMudletPath(enums::profileHomePath, mProfileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }

    TMap* map() const { return mpHost->mpMap.data(); }

    void buildMap()
    {
        TMap* pMap = map();
        pMap->setDefaultAreaShown(false);
        pMap->mapClear();
        const int groundAreaId = pMap->mpRoomDB->addArea(qsl("Ground"));
        const int upstairsAreaId = pMap->mpRoomDB->addArea(qsl("Upstairs"));
        QVERIFY(groundAreaId > 0);
        QVERIFY(upstairsAreaId > 0);
        QVERIFY(pMap->addRoom(1) && pMap->setRoomArea(1, groundAreaId) && pMap->setRoomCoordinates(1, 0, 0, 0));
        QVERIFY(pMap->addRoom(2) && pMap->setRoomArea(2, upstairsAreaId) && pMap->setRoomCoordinates(2, 0, 0, 1));
        pMap->mRoomIdHash[pMap->mProfileName] = 1;
    }

    QStringList areaListOf(const dlgMapper* pMapper) const
    {
        QStringList names;
        for (int i = 0; i < pMapper->comboBox_showArea->count(); ++i) {
            names << pMapper->comboBox_showArea->itemText(i);
        }
        return names;
    }

    // Lets any repaint already on its way land, so that a later count only
    // sees what the cue under test asked for.
    void settlePaints()
    {
        int before = -1;
        while (before != mPaintCount) {
            before = mPaintCount;
            QTest::qWait(50ms);
        }
        mPaintCount = 0;
    }

    // Delivers the repaints already asked for and nothing else: no timer or
    // later event gets to run, so a paint counted here was requested by the time
    // this is called, not by something that happened to repaint afterwards.
    bool paintRequested()
    {
        QCoreApplication::sendPostedEvents(nullptr, QEvent::UpdateRequest);
        return mPaintCount > 0;
    }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (watched == mp2dMap && event->type() == QEvent::Paint) {
            ++mPaintCount;
        }
        return QObject::eventFilter(watched, event);
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        // A config root of this process's own, so this never reads or writes
        // the developer's ~/.config/mudlet. Since #9712 the opt-in that makes
        // setupConfig() adopt a directory is $XDG_CONFIG_HOME/mudlet/profiles,
        // not the mudlet directory alone.
        QVERIFY(mConfigDir.isValid());
        QVERIFY(mMapDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0);
        mPort = QString::number(mpServer->serverPort());

        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(MudletApp::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        mudlet::self()->mSkipDefaultPackageInstall = true;
        MudletApp::getQSettings()->setValue(qsl("autosaveIntervalMinutes"), 0);
        deleteProfileDirectory();

        mpHost = TestProfile::create(mProfileName, mLocalhost, mPort);
        QVERIFY(mpHost);
        QSignalSpy connected(&(mpHost->mTelnet), &cTelnet::signal_connected);
        QVERIFY2(connected.wait(3s), "could not connect to the telnet stub");

        mudlet::self()->show();
        mpHost->showHideOrCreateMapper(false);
        mpMapper = map()->mpMapper;
        QVERIFY2(mpMapper, "the profile has no mapper to take the cues");
        mp2dMap = mpMapper->mp2dMap;
        QVERIFY(mp2dMap);
        mp2dMap->installEventFilter(this);
    }

    void cleanupTestCase()
    {
        if (mp2dMap) {
            mp2dMap->removeEventFilter(this);
        }
        mp2dMap = nullptr;
        mpMapper = nullptr;
        mpHost = nullptr;
        delete mpServer;
        mpServer = nullptr;
        if (mudlet::self()) {
            deleteProfileDirectory();
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void init()
    {
        buildMap();
        if (QTest::currentTestFailed()) {
            return;
        }
        QVERIFY2(map()->mpMapper == mpMapper, "the mapper made for this test is no longer the one drawing the map");
        mpMapper->show();
        mpMapper->updateAreaComboBox();
        QCOMPARE(areaListOf(mpMapper), (QStringList{qsl("Ground"), qsl("Upstairs")}));
    }

    void test_clearingTheMapEmptiesTheMapperAreaListAndSelectionList()
    {
        mp2dMap->mMultiSelectionListWidget.addTopLevelItem(new QTreeWidgetItem(QStringList{qsl("1")}));
        mp2dMap->mMultiSelectionListWidget.show();

        map()->mapClear();

        QCOMPARE(areaListOf(mpMapper), QStringList());
        QCOMPARE(mp2dMap->mMultiSelectionListWidget.topLevelItemCount(), 0);
        QVERIFY2(mp2dMap->mMultiSelectionListWidget.isHidden(), "clearing the map left the room selection list up");
    }

    void test_showingTheDefaultAreaListsItInTheMapper()
    {
        const QString defaultAreaName = map()->getDefaultAreaName();
        map()->setDefaultAreaShown(true);
        QVERIFY2(areaListOf(mpMapper).contains(defaultAreaName), "showing the default area did not add it to the mapper's area list");
        map()->setDefaultAreaShown(false);
        QVERIFY2(!areaListOf(mpMapper).contains(defaultAreaName), "hiding the default area left it in the mapper's area list");
    }

    void test_theMapperTakesUpTheApplicationPaletteWhenTheColoursChange()
    {
        QPalette stale = mpMapper->palette();
        const QColor staleWindow = QApplication::palette().color(QPalette::Window) == Qt::red ? Qt::green : Qt::red;
        stale.setColor(QPalette::Window, staleWindow);
        mpMapper->setPalette(stale);
        QCOMPARE(mpMapper->palette().color(QPalette::Window), staleWindow);

        map()->refreshMapperColours();

        QCOMPARE(mpMapper->palette().color(QPalette::Window), QApplication::palette().color(QPalette::Window));
    }

    void test_aSymbolChangeFlushesTheDrawingMappersSymbolCache()
    {
        const qreal before = map()->getSymbolFontFudgeFactor();
        auto restore = qScopeGuard([this, before]() {
            map()->setSymbolFontFudgeFactor(before);
        });
        mp2dMap->addSymbolToPixmapCache(qsl("test-key"), qsl("A"), Qt::white, false);
        QVERIFY2(mp2dMap->symbolPixmapCacheCount() > 0, "nothing was cached, so a flush would prove nothing");

        QVERIFY(map()->setSymbolFontFudgeFactor(before == 1.0 ? 1.5 : 1.0));

        QCOMPARE(mp2dMap->symbolPixmapCacheCount(), 0);
    }

    void test_aMoveFlagsAndRepaintsTheDrawingMapper()
    {
        // The move cue also brings the empty map overlay up to date, which can
        // repaint the map by itself; settle that first so it cannot stand in.
        mpMapper->updateEmptyStateOverlay();
        settlePaints();
        mp2dMap->mNewMoveAction = false;

        map()->updateArea(-1);
        // updateArea() cues the mapper from a zero timer, which is a call posted
        // to the map: deliver just that, so no unrelated repaint can land.
        QCoreApplication::sendPostedEvents(map(), QEvent::MetaCall);

        QVERIFY2(mp2dMap->mNewMoveAction, "the mapper was not told a move happened");
        QVERIFY2(paintRequested(), "the mapper was not repainted after a move");
    }

    void test_addingAndDeletingALabelRepaintsTheDrawingMapper()
    {
        const int areaId = map()->mpRoomDB->getAreaNamesMap().key(qsl("Ground"));
        QVERIFY(areaId > 0);
        settlePaints();

        const int labelId = map()->createMapLabel(areaId, qsl("hello"), 0, 0, 0, Qt::white, Qt::black, true, true, true);
        QVERIFY(labelId >= 0);
        QVERIFY2(paintRequested(), "the mapper was not repainted for a new label");

        settlePaints();
        map()->deleteMapLabel(areaId, labelId);
        QVERIFY2(paintRequested(), "the mapper was not repainted for a deleted label");
    }

    void test_anXmlMapImportRelistsAndShowsTheMapper()
    {
        const QString fileName = qsl("%1/cue.xml").arg(mMapDir.path());
        QFile writer(fileName);
        QVERIFY(writer.open(QIODevice::WriteOnly));
        QVERIFY(writer.write(scmMapXml) == scmMapXml.size());
        writer.close();
        mpMapper->hide();
        QSignalSpy loaded(map(), &TMap::signal_mapLoaded);
        QSignalSpy showRequested(map(), &TMap::signal_mapperShowRequested);

        QFile reader(fileName);
        QVERIFY(reader.open(QIODevice::ReadOnly));
        QString errMsg;
        QVERIFY2(map()->readXmlMapFile(reader, &errMsg), qPrintable(errMsg));

        QCOMPARE(areaListOf(mpMapper), QStringList{qsl("Imported Area")});
        QVERIFY2(!mpMapper->isHidden(), "importing a map did not show the mapper");
        // XMLimport::importPackage() cues the mapper once it has read the <map>,
        // and readXmlMapFile() cues it again with the import's outcome. Either
        // pair alone would satisfy the checks above, so count both.
        QCOMPARE(loaded.count(), 2);
        QCOMPARE(loaded.last().at(0).toBool(), true);
        QCOMPARE(showRequested.count(), 2);
    }

    // Only readXmlMapFile() knows the import failed, so this is the cue that
    // XMLimport cannot stand in for.
    void test_aFailedXmlMapImportCuesTheMapperWithTheFailure()
    {
        const QString fileName = qsl("%1/broken.xml").arg(mMapDir.path());
        QFile writer(fileName);
        QVERIFY(writer.open(QIODevice::WriteOnly));
        const QByteArray brokenXml = QByteArrayLiteral("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<map>\n <areas>\n </rooms>\n</map>\n");
        QVERIFY(writer.write(brokenXml) == brokenXml.size());
        writer.close();
        mpMapper->hide();
        QSignalSpy loaded(map(), &TMap::signal_mapLoaded);

        QFile reader(fileName);
        QVERIFY(reader.open(QIODevice::ReadOnly));
        QString errMsg;
        QVERIFY2(!map()->readXmlMapFile(reader, &errMsg), "a map file with mismatched tags was imported");

        QVERIFY(!loaded.isEmpty());
        QCOMPARE(loaded.last().at(0).toBool(), false);
        QVERIFY2(!mpMapper->isHidden(), "a failed import did not show the mapper");
    }

    // A second mapper of the same profile - one in a detached window, say -
    // is connected to the same map but is not the one drawing it.
    void test_onlyTheMapperDrawingTheMapFollowsItsCues()
    {
        auto other = std::make_unique<dlgMapper>(nullptr, mpHost, map());
        QVERIFY2(map()->mpMapper == mpMapper, "making a second mapper took the map over");
        other->updateAreaComboBox();
        const QStringList otherAreasBefore = areaListOf(other.get());
        other->mp2dMap->mNewMoveAction = false;
        mp2dMap->mNewMoveAction = false;

        map()->setDefaultAreaShown(true);
        auto hideDefault = qScopeGuard([this]() {
            map()->setDefaultAreaShown(false);
        });
        map()->updateArea(-1);
        QVERIFY2(QTest::qWaitFor(
                         [this]() {
                             return mp2dMap->mNewMoveAction;
                         },
                         2s),
                 "the drawing mapper was not told a move happened");

        QVERIFY2(areaListOf(mpMapper).contains(map()->getDefaultAreaName()), "the drawing mapper did not relist its areas");
        QCOMPARE(areaListOf(other.get()), otherAreasBefore);
        QVERIFY2(!other->mp2dMap->mNewMoveAction, "a mapper not drawing the map was told a move happened");
    }
};

#include "MapViewCueTest.moc"
MUDLET_GROUPED_TEST_MAIN(MapViewCueTest)
