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
 * A scaled map label is rendered into a cached pixmap at the size it is drawn
 * at. An image label, or a text one below the size its font can still be read
 * at, is cached as a scaled copy of the label's own pixmap instead.
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

    // The fall-back must not free a pixmap a painter is still working on.
    void aLabelTooSmallToReadIsScaledWithoutFreeingThePixmapUnderThePainter()
    {
        QTest::failOnWarning(QRegularExpression(qsl("Cannot destroy paint device")));
        QTest::failOnWarning(QRegularExpression(qsl("Text Label Pixmap cache is full")));

        TMapLabel label;
        label.text = qsl("a map label with a good many words in it");
        label.font = QFont(qsl("Bitstream Vera Sans"), 10);
        label.pix = QPixmap(200, 40);
        label.pix.fill(Qt::red);
        label.bgColor = Qt::black;
        label.fgColor = Qt::white;
        label.outlineColor = Qt::white;

        // An empty text or font takes the same scaled-pixmap path, so the red
        // below would not show that the too-small check ran
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

    // The font is fitted to the box, so every line of a multi-line label has to
    // be measured or the outer lines are drawn outside the box and cut off (#10990).
    void everyLineOfAMultiLineLabelFitsInsideTheBox()
    {
        TMapLabel label;
        label.text = qsl("HH\nHH\nHH");
        label.font = QFont(qsl("Bitstream Vera Sans"), 10);
        label.pix = QPixmap(400, 60);
        label.pix.fill(Qt::red);
        label.bgColor = Qt::black;
        label.fgColor = Qt::white;
        label.outlineColor = Qt::white;

        QPixmap target(400, 60);
        target.fill(Qt::blue);
        QPainter painter(&target);
        mp2dMap->flushTextLabelPixmapCache();
        mp2dMap->drawScaledLabel(painter, QPointF(0, 0), label, 2, QRectF(0, 0, 400, 60));
        painter.end();

        const QImage image = target.toImage();
        QCOMPARE(image.pixelColor(0, 0), QColor(Qt::black));
        auto rowHasText = [&image](int y) {
            for (int x = 0; x < image.width(); ++x) {
                if (image.pixelColor(x, y).lightness() > 128) {
                    return true;
                }
            }
            return false;
        };
        int bands = 0;
        bool inBand = false;
        for (int y = 0; y < image.height(); ++y) {
            const bool hasText = rowHasText(y);
            if (hasText && !inBand) {
                ++bands;
            }
            inBand = hasText;
        }
        QVERIFY2(!rowHasText(0), "text is cut off at the top of the label");
        QVERIFY2(!rowHasText(image.height() - 1), "text is cut off at the bottom of the label");
        QCOMPARE(bands, 3);
    }

    // Image labels are cached at their drawn size too, so a replaced image has
    // to be drawn afresh rather than served from the cache.
    void aReplacedLabelImageIsNotServedFromTheCache()
    {
        TMapLabel label;
        label.pix = QPixmap(20, 10);
        label.pix.fill(Qt::red);

        QPixmap target(8, 8);
        QPainter painter(&target);
        mp2dMap->flushTextLabelPixmapCache();
        mp2dMap->drawScaledLabel(painter, QPointF(0, 0), label, 3, QRectF(0, 0, 8, 8));
        QCOMPARE(target.toImage().pixelColor(4, 4), QColor(Qt::red));
        QCOMPARE(mp2dMap->textLabelPixmapCacheCount(), 1);

        label.pix = QPixmap(20, 10);
        label.pix.fill(Qt::green);
        mp2dMap->drawScaledLabel(painter, QPointF(0, 0), label, 3, QRectF(0, 0, 8, 8));
        painter.end();
        QCOMPARE(target.toImage().pixelColor(4, 4), QColor(Qt::green));
    }

    void anEditedLabelColourIsNotServedFromTheCache()
    {
        TMapLabel label;
        label.text = qsl("HH");
        label.font = QFont(qsl("Bitstream Vera Sans"), 10);
        // Red would mean the text was not rendered at all
        label.pix = QPixmap(100, 40);
        label.pix.fill(Qt::red);
        label.bgColor = Qt::black;
        label.fgColor = Qt::white;
        label.outlineColor = Qt::white;

        QPixmap target(100, 40);
        QPainter painter(&target);
        mp2dMap->flushTextLabelPixmapCache();
        mp2dMap->drawScaledLabel(painter, QPointF(0, 0), label, 4, QRectF(0, 0, 100, 40));
        QCOMPARE(target.toImage().pixelColor(0, 0), QColor(Qt::black));
        QCOMPARE(mp2dMap->textLabelPixmapCacheCount(), 1);

        label.bgColor = Qt::blue;
        mp2dMap->drawScaledLabel(painter, QPointF(0, 0), label, 4, QRectF(0, 0, 100, 40));
        painter.end();
        QCOMPARE(target.toImage().pixelColor(0, 0), QColor(Qt::blue));
    }

    // A zoomed-out area can have hundreds of labels on screen; if they do not
    // all stay cached, every frame renders every one of them again.
    void manySmallLabelsOnScreenAllStayCached()
    {
        constexpr int labelCount = 300;
        QPixmap target(40, 20);
        QPainter painter(&target);
        mp2dMap->flushTextLabelPixmapCache();
        for (int id = 1; id <= labelCount; ++id) {
            TMapLabel label;
            label.text = qsl("Label %1").arg(id);
            label.font = QFont(qsl("Bitstream Vera Sans"), 10);
            label.pix = QPixmap(40, 20);
            label.pix.fill(Qt::red);
            label.bgColor = Qt::black;
            label.fgColor = Qt::white;
            label.outlineColor = Qt::white;
            mp2dMap->drawScaledLabel(painter, QPointF(0, 0), label, id, QRectF(0, 0, 40, 20));
        }
        painter.end();
        QCOMPARE(mp2dMap->textLabelPixmapCacheCount(), labelCount);
    }

    // Zoomed in, a few labels on screen can together outgrow the cache's
    // budget; evicting one still on screen would redraw it every frame.
    void labelsOnScreenStayCachedPastTheBudget()
    {
        mp2dMap->setTextLabelPixmapBudgetKiB(1024);
        QPixmap target(8, 8);
        QPainter painter(&target);
        QList<TMapLabel> labels(8);
        for (TMapLabel& label : labels) {
            label.pix = QPixmap(4, 4);
            label.pix.fill(Qt::red);
        }
        auto drawLabel = [this, &painter, &labels](int id) {
            // 256 KiB each
            mp2dMap->drawScaledLabel(painter, QPointF(0, 0), labels[id], id, QRectF(0, 0, 256, 256));
        };
        mp2dMap->startTextLabelFrame();
        for (int id = 2; id <= 7; ++id) {
            drawLabel(id);
        }
        const qsizetype firstFrame = mp2dMap->textLabelPixmapCacheCount();
        // The next frame pans label 1 into view and label 7 out; nothing it
        // draws should have been evicted to make room for label 1
        mp2dMap->startTextLabelFrame();
        for (int id = 1; id <= 6; ++id) {
            drawLabel(id);
        }
        painter.end();
        const qsizetype secondFrame = mp2dMap->textLabelPixmapCacheCount();
        mp2dMap->setTextLabelPixmapBudgetKiB(T2DMap::scmTextLabelPixmapBudgetKiB);
        QCOMPARE(firstFrame, 6);
        QCOMPARE(secondFrame, 7);
    }
};

#include "MapTextLabelCacheTest.moc"
MUDLET_GROUPED_TEST_MAIN(MapTextLabelCacheTest)
