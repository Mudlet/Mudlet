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
 * createMapLabel() renders only the part of its 2000x2000 canvas that the text
 * covers. Each label here must match, pixel for pixel, the crop of a whole
 * canvas rendered the same way.
 *
 * Why not a spec: the label's pixmap never reaches Lua.
 *
 * Run with: ctest -R MapLabelRenderTest -V
 */

#include <QPainter>
#include <QPixmap>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <cmath>

#include "Host.h"
#include "HostManager.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "TArea.h"
#include "TMap.h"
#include "TMapLabel.h"
#include "TRoomDB.h"
#include "mudlet.h"

#include "GroupedTest.h"

class MapLabelRenderTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    Host* mpHost = nullptr;
    int mAreaId = -1;
    const QString mProfileName = qsl("MapLabelRender-Test");

    void deleteProfileDirectory() const
    {
        QDir dir(MudletApp::getMudletPath(enums::profileHomePath, mProfileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }

    static QImage wholeCanvasCrop(const QString& text, const QFont& font, const QColor& fg, const QColor& bg, const QColor& outline, QRectF& br)
    {
        const QRectF lr(0, 0, 2000, 2000);
        QPixmap pix(lr.size().toSize());
        pix.fill(Qt::transparent);
        QPainter lp(&pix);
        lp.fillRect(lr, bg);
        lp.setRenderHint(QPainter::Antialiasing);
        lp.setFont(font);
        QPen outlinePen(outline);
        outlinePen.setWidth(1);
        lp.setPen(outlinePen);
        if (fg != outline) {
            lp.drawText(QRect(19, 70, 2000, 2000), Qt::AlignLeft | Qt::AlignTop, text, &br);
            lp.drawText(QRect(21, 70, 2000, 2000), Qt::AlignLeft | Qt::AlignTop, text, &br);
            lp.drawText(QRect(20, 69, 2000, 2000), Qt::AlignLeft | Qt::AlignTop, text, &br);
            lp.drawText(QRect(20, 71, 2000, 2000), Qt::AlignLeft | Qt::AlignTop, text, &br);
        }
        lp.setPen(fg);
        lp.drawText(QRect(20, 70, 2000, 2000), Qt::AlignLeft | Qt::AlignTop, text, &br);
        lp.end();
        const QRect brRect = br.normalized().toRect();
        return pix.copy(brRect).toImage();
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
        mAreaId = mpHost->mpMap->mpRoomDB->addArea(qsl("Labels"));
        QVERIFY(mAreaId > 0);
    }

    void cleanupTestCase()
    {
        mpHost = nullptr;
        if (mudlet::self()) {
            deleteProfileDirectory();
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void aLabelMatchesTheCropOfTheWholeCanvas_data()
    {
        QTest::addColumn<QString>("text");
        QTest::addColumn<QString>("fontName");
        QTest::addColumn<int>("fontSize");
        QTest::addColumn<QColor>("fg");
        QTest::addColumn<QColor>("bg");
        QTest::addColumn<QColor>("outline");
        QTest::addColumn<qreal>("zoom");

        const QString vera = qsl("Bitstream Vera Sans");
        const QString defaultFont;
        QTest::newRow("outlined") << qsl("Market Square") << vera << 12 << QColor(Qt::yellow) << QColor(40, 40, 120) << QColor(Qt::black) << 30.0;
        QTest::newRow("no outline") << qsl("Market Square") << vera << 12 << QColor(Qt::white) << QColor(Qt::darkGreen) << QColor(Qt::white) << 30.0;
        QTest::newRow("translucent") << qsl("Fog") << vera << 20 << QColor(255, 0, 0, 128) << QColor(0, 0, 255, 60) << QColor(0, 255, 0, 200) << 10.0;
        QTest::newRow("transparent background") << qsl("gate") << vera << 9 << QColor(Qt::white) << QColor(Qt::transparent) << QColor(Qt::black) << 40.0;
        QTest::newRow("multi-line") << qsl("North\nTower\n\tof the keep") << vera << 14 << QColor(Qt::cyan) << QColor(Qt::black) << QColor(Qt::red) << 30.0;
        QTest::newRow("default font") << qsl("Here be dragons") << defaultFont << 50 << QColor(Qt::white) << QColor(Qt::black) << QColor(Qt::white) << 40.0;
        QTest::newRow("wider than the canvas") << QString(200, QChar('W')) << vera << 30 << QColor(Qt::white) << QColor(Qt::gray) << QColor(Qt::blue) << 30.0;
        QTest::newRow("taller than the canvas") << QStringList(80, qsl("row")).join(QChar('\n')) << vera << 30 << QColor(Qt::white) << QColor(Qt::gray) << QColor(Qt::blue) << 30.0;
        QTest::newRow("non-latin") << qsl("北の塔 Ελλάδα") << vera << 16 << QColor(Qt::white) << QColor(Qt::black) << QColor(Qt::magenta) << 25.0;
        // Lines whose height divides 2000 fill the canvas exactly, which is where Qt's
        // text layout stops early and skips clipping to the canvas unless asked for the
        // bounding rectangle. Line heights depend on the fonts installed, so find them.
        const QString tallEdgeInk = QStringList(400, qsl("jÅy")).join(QChar('\n'));
        QPixmap probe(1, 1);
        QPainter probePainter(&probe);
        int canvasTallRows = 0;
        for (const QString& family : {vera, defaultFont}) {
            for (int size = 4; size <= 120; ++size) {
                probePainter.setFont(QFont(family, size));
                const qreal height = probePainter.boundingRect(QRectF(0, 0, 2000, 2000), Qt::AlignLeft | Qt::AlignTop, tallEdgeInk).height();
                if (height > 2000 && std::fmod(2000.0, height / 400) == 0) {
                    QTest::addRow("canvas-tall %s %dpt", qPrintable(family.isEmpty() ? qsl("default") : family), size)
                            << tallEdgeInk << family << size << QColor(Qt::white) << QColor(Qt::black) << QColor(Qt::red) << 30.0;
                    ++canvasTallRows;
                }
            }
        }
        if (!canvasTallRows) {
            QTest::newRow("canvas-tall") << tallEdgeInk << vera << 30 << QColor(Qt::white) << QColor(Qt::black) << QColor(Qt::red) << 30.0;
        }
        QTest::newRow("very large") << qsl("ƒjÅ") << vera << 150 << QColor(Qt::white) << QColor(Qt::black) << QColor(Qt::red) << 30.0;
        QTest::newRow("whitespace") << qsl("   ") << vera << 12 << QColor(Qt::white) << QColor(Qt::black) << QColor(Qt::red) << 30.0;
    }

    void aLabelMatchesTheCropOfTheWholeCanvas()
    {
        QFETCH(QString, text);
        QFETCH(QString, fontName);
        QFETCH(int, fontSize);
        QFETCH(QColor, fg);
        QFETCH(QColor, bg);
        QFETCH(QColor, outline);
        QFETCH(qreal, zoom);

        TMap* pMap = mpHost->mpMap.data();
        const std::optional<QString> font = fontName.isEmpty() ? std::nullopt : std::optional<QString>(fontName);
        const int labelId = pMap->createMapLabel(mAreaId, text, 1.0f, 2.0f, 0.0f, fg, bg, true, false, true, zoom, fontSize, font, outline);
        QVERIFY(labelId >= 0);
        const auto label = pMap->mpRoomDB->getArea(mAreaId)->mMapLabels.value(labelId);

        QRectF br;
        const QImage expected = wholeCanvasCrop(text, label.font, fg, bg, outline, br);
        const QImage actual = label.pix.toImage();
        QCOMPARE(actual.size(), expected.size());
        QCOMPARE(actual.convertToFormat(QImage::Format_ARGB32_Premultiplied), expected.convertToFormat(QImage::Format_ARGB32_Premultiplied));
        const QSizeF size(br.normalized().width() / zoom, br.normalized().height() / zoom);
        QCOMPARE(label.size, size);
        QCOMPARE(label.clickSize, size);
    }
};

#include "MapLabelRenderTest.moc"
MUDLET_GROUPED_TEST_MAIN(MapLabelRenderTest)
