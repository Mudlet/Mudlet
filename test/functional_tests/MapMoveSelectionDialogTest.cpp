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
 * The 2D map's "Move to position" dialog must not take a coordinate it cannot
 * read as 0: that silently moves the whole selection onto the origin, and the
 * map has no undo.
 *
 * The dialog is modal and only the context menu opens it, so this cannot be a
 * busted spec.
 *
 * Run with: ctest -R MapMoveSelectionDialogTest -V
 */

#include <QApplication>
#include <QDialog>
#include <QFileInfo>
#include <QLineEdit>
#include <QPushButton>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "Host.h"
#include "HostManager.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "T2DMap.h"
#include "TMap.h"
#include "TRoom.h"
#include "TRoomDB.h"
#include "mudlet.h"

#include "GroupedTest.h"

class MapMoveSelectionDialogTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    Host* mpHost = nullptr;
    T2DMap* mp2dMap = nullptr;
    const QString mProfileName = qsl("MapMoveSelectionDialog-Test");
    static constexpr int scmRoomA = 100;
    static constexpr int scmRoomB = 101;

    static bool portableMarkerPresent()
    {
        return QFileInfo::exists(qsl("%1/portable.txt").arg(QCoreApplication::applicationDirPath())) || QFileInfo::exists(qsl("%1/.config/mudlet/portable.txt").arg(QDir::homePath()));
    }

    TMap* map() const { return mpHost->mpMap.data(); }
    TRoomDB* roomDB() const { return mpHost->mpMap->mpRoomDB.get(); }

    void deleteProfileDirectory() const
    {
        QDir dir(MudletApp::getMudletPath(enums::profileHomePath, mProfileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }

    void buildSelection()
    {
        map()->mapClear();
        const int areaId = roomDB()->addArea(qsl("area"));
        QVERIFY(areaId > 0);
        QVERIFY(map()->addRoom(scmRoomA));
        QVERIFY(map()->addRoom(scmRoomB));
        QVERIFY(map()->setRoomArea(scmRoomA, areaId));
        QVERIFY(map()->setRoomArea(scmRoomB, areaId));
        QVERIFY(map()->setRoomCoordinates(scmRoomA, 5, 6, 7));
        QVERIFY(map()->setRoomCoordinates(scmRoomB, 8, 6, 7));
        mp2dMap->switchArea(areaId);
        mp2dMap->mMultiSelectionSet = {scmRoomA, scmRoomB};
    }

    QPoint roomPosition(const int roomId) const
    {
        const TRoom* pRoom = roomDB()->getRoom(roomId);
        return pRoom ? QPoint(pRoom->x(), pRoom->y()) : QPoint();
    }

    // Opens the dialog, hands it to fillIn() once it is up, and returns once
    // the dialog has closed.
    void runDialog(const std::function<void(const QList<QLineEdit*>&, QPushButton*)>& fillIn)
    {
        bool dialogSeen = false;
        QTimer::singleShot(0, this, [&dialogSeen, &fillIn]() {
            auto* dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget());
            QVERIFY(dialog);
            const QList<QLineEdit*> edits = dialog->findChildren<QLineEdit*>();
            const QList<QPushButton*> buttons = dialog->findChildren<QPushButton*>();
            QCOMPARE(edits.size(), 3);
            QCOMPARE(buttons.size(), 2);
            dialogSeen = true;
            // OK is created before Cancel
            fillIn(edits, buttons.at(0));
            if (dialog->isVisible()) {
                dialog->reject();
            }
        });
        mp2dMap->slot_movePosition();
        QVERIFY(dialogSeen);
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory();

        auto* hostManager = HostManager::self();
        QVERIFY2(hostManager->addHost(mProfileName, qsl("23"), QString(), QString()), "failed to create the Host");
        mpHost = hostManager->getHost(mProfileName);
        QVERIFY(mpHost);
        QVERIFY(map());

        mp2dMap = new T2DMap();
        mp2dMap->mpMap = map();
        mp2dMap->mpHost = mpHost;
    }

    void cleanupTestCase()
    {
        delete mp2dMap;
        mp2dMap = nullptr;
        if (mudlet::self()) {
            deleteProfileDirectory();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void unreadableCoordinateCannotBeAccepted_data()
    {
        QTest::addColumn<QString>("text");
        QTest::newRow("empty") << QString();
        QTest::newRow("letters") << qsl("abc");
        QTest::newRow("typo") << qsl("1O");
        QTest::newRow("fraction") << qsl("2.5");
        QTest::newRow("lone minus") << qsl("-");
        QTest::newRow("group separator") << qsl("1,000");
    }

    void unreadableCoordinateCannotBeAccepted()
    {
        QFETCH(QString, text);
        buildSelection();

        bool okEnabled = true;
        runDialog([&](const QList<QLineEdit*>& edits, QPushButton* ok) {
            edits.at(0)->setText(text);
            okEnabled = ok->isEnabled();
            ok->click();
        });

        QCOMPARE(roomPosition(scmRoomA), QPoint(5, 6));
        QCOMPARE(roomPosition(scmRoomB), QPoint(8, 6));
        QVERIFY2(!okEnabled, "OK stayed enabled with an unreadable x coordinate");
    }

    void validCoordinatesMoveTheSelection()
    {
        buildSelection();

        bool okEnabled = false;
        runDialog([&](const QList<QLineEdit*>& edits, QPushButton* ok) {
            edits.at(0)->setText(qsl("-10"));
            edits.at(1)->setText(qsl("20"));
            edits.at(2)->setText(qsl("7"));
            okEnabled = ok->isEnabled();
            ok->click();
        });

        QVERIFY(okEnabled);
        const int centre = mp2dMap->getCenterSelectedRoomId();
        QVERIFY(centre == scmRoomA || centre == scmRoomB);
        const int dx = -10 - (centre == scmRoomA ? 5 : 8);
        QCOMPARE(roomPosition(scmRoomA), QPoint(5 + dx, 20));
        QCOMPARE(roomPosition(scmRoomB), QPoint(8 + dx, 20));
    }
};

#include "MapMoveSelectionDialogTest.moc"
MUDLET_GROUPED_TEST_MAIN(MapMoveSelectionDialogTest)
