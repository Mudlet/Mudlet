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
 * A text map label is rendered into a cached pixmap at the size it is drawn
 * at. Below the size its font can still be read at, that rendering is dropped
 * for a scaled copy of the label's own pixmap instead.
 *
 * Why not a spec: the cache is private to the widget and nothing in the Lua
 * API reports what a label was drawn from.
 *
 * Run with: ctest -R MapTextLabelCacheTest -V
 */

#include <QPainter>
#include <QPixmap>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "Host.h"
#include "HostManager.h"
#include "MudletInstanceCoordinator.h"
#include "MudletApp.h"
#include "PortableModeTestHelper.h"
#include "T2DMap.h"
#include "TMap.h"
#include "TMapLabel.h"
#include "mudlet.h"

#include "GroupedTest.h"

class MapTextLabelCacheTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    Host* mpHost = nullptr;
    T2DMap* mp2dMap = nullptr;
    const QString mProfileName = qsl("MapTextLabelCache-Test");

    void deleteProfileDirectory() const
    {
        QDir dir(MudletApp::getMudletPath(enums::profileHomePath, mProfileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
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
        QCOMPARE(MudletApp::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory();

        auto* hostManager = HostManager::self();
        QVERIFY2(hostManager->addHost(mProfileName, qsl("23"), QString(), QString()), "failed to create the Host");
        mpHost = hostManager->getHost(mProfileName);
        QVERIFY(mpHost);

        // The two things dlgMapper's constructor sets that this path needs; the
        // dialog itself is not needed for it
        mp2dMap = new T2DMap();
        mp2dMap->mpMap = mpHost->mpMap.data();
        mp2dMap->mpHost = mpHost;
    }

    void cleanupTestCase()
    {
        delete mp2dMap;
        mp2dMap = nullptr;
        mpHost = nullptr;
        if (mudlet::self()) {
            deleteProfileDirectory();
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    // The fall-back replaces the pixmap the cache painter is still working on,
    // so the painter has to be finished with it first (#8836).
    void aLabelTooSmallToReadIsScaledWithoutFreeingThePixmapUnderThePainter()
    {
        QTest::failOnWarning(QRegularExpression(qsl("Cannot destroy paint device")));
        QTest::failOnWarning(QRegularExpression(qsl("Cache lookup failed")));

        TMapLabel label;
        label.text = qsl("a map label with a good many words in it");
        label.font = QFont(qsl("Bitstream Vera Sans"), 10);
        label.pix = QPixmap(200, 40);
        label.pix.fill(Qt::red);
        label.bgColor = Qt::black;
        label.fgColor = Qt::white;
        label.outlineColor = Qt::white;

        // drawScaledLabel() falls through to the same scaled pixmap when either
        // of these is empty, so the red below would not mean the cache ran
        QVERIFY(!label.text.isEmpty());
        QVERIFY(!label.font.family().isEmpty());

        QPixmap target(16, 16);
        target.fill(Qt::blue);
        QPainter painter(&target);
        mp2dMap->flushTextLabelPixmapCache();
        mp2dMap->drawScaledLabel(painter, QPointF(0, 0), label, 1, QRectF(0, 0, 4, 4));
        painter.end();

        // The label's own pixmap, scaled - which is what says the too-small
        // branch ran rather than the one that renders the text.
        QCOMPARE(target.toImage().pixelColor(1, 1), QColor(Qt::red));
    }
};

#include "MapTextLabelCacheTest.moc"
MUDLET_GROUPED_TEST_MAIN(MapTextLabelCacheTest)
