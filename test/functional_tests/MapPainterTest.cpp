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
 * TMapPainter draws the 2D map's rooms, exits, doors and labels onto any
 * QPainter, with no widget behind it, so an app built on mudlet_core alone can
 * draw the map itself.
 *
 * Why not a spec: Lua has no way to hand the painter a QPainter of its own, or
 * to read back a pixel it drew.
 *
 * Run with: ctest -R MapPainterTest -V
 */

#include <QImage>
#include <QPainter>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "ExitAppearance.h"
#include "Host.h"
#include "HostManager.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "TArea.h"
#include "TMap.h"
#include "TMapLabel.h"
#include "TMapPainter.h"
#include "TRoom.h"
#include "TRoomDB.h"
#include "mudlet.h"

#include "GroupedTest.h"

class MapPainterTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    Host* mpHost = nullptr;
    int mAreaId = 0;
    const QString mProfileName = qsl("MapPainter-Test");

    static constexpr int scmWestRoom = 1;
    static constexpr int scmEastRoom = 2;
    static constexpr int scmRedEnvironment = 301;
    static constexpr int scmBlueEnvironment = 302;
    static constexpr int scmImageWidth = 400;
    static constexpr int scmImageHeight = 300;
    static constexpr float scmRoomPixels = 40.0f;
    // Where map (0, 0) lands; map y grows upwards, image y downwards
    static constexpr int scmOriginX = 100;
    static constexpr int scmOriginY = 150;

    static QColor lineColour() { return QColor(0, 255, 128); }

    TMap* map() const { return mpHost->mpMap.data(); }
    TRoomDB* roomDB() const { return mpHost->mpMap->mpRoomDB.get(); }

    static QPointF onImage(const QPointF& mapPoint) { return QPointF(scmOriginX + mapPoint.x() * scmRoomPixels, scmOriginY - mapPoint.y() * scmRoomPixels); }

    void deleteProfileDirectory() const
    {
        QDir dir(MudletApp::getMudletPath(enums::profileHomePath, mProfileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }

    TMapPainter::View view() const
    {
        TMapPainter::View view;
        view.mpMap = map();
        view.mpHost = mpHost;
        view.mSize = QSize(scmImageWidth, scmImageHeight);
        view.mRoomWidth = scmRoomPixels;
        view.mRoomHeight = scmRoomPixels;
        view.mRX = scmOriginX;
        view.mRY = scmOriginY;
        view.mAreaID = mAreaId;
        // Unblended pixels hold exactly the colour they were drawn in
        view.mMapperUseAntiAlias = false;
        return view;
    }

    static qreal distanceTo(const QLineF& segment, const QPointF& point)
    {
        const QPointF along = segment.p2() - segment.p1();
        const qreal lengthSquared = QPointF::dotProduct(along, along);
        const qreal t = lengthSquared > 0 ? std::clamp(QPointF::dotProduct(point - segment.p1(), along) / lengthSquared, 0.0, 1.0) : 0.0;
        return QLineF(point, segment.p1() + t * along).length();
    }

    static QImage blankImage()
    {
        QImage image(scmImageWidth, scmImageHeight, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::black);
        return image;
    }

    static QString saved(const QImage& image, const QString& name)
    {
        const QString path = qsl("%1/MapPainterTest-%2.png").arg(QDir::tempPath(), name);
        return image.save(path) ? qsl("The image is at %1").arg(path) : qsl("The image could not be saved to %1").arg(path);
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
        QVERIFY(map());
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

    // Two rooms two map units apart on one level, joined both ways, the west one in red and the east one in blue
    void init()
    {
        map()->mapClear();
        mAreaId = roomDB()->addArea(qsl("Painted area"));
        QVERIFY(mAreaId > 0);
        for (const auto& [roomId, x, environment] : {std::tuple{scmWestRoom, 0, scmRedEnvironment}, std::tuple{scmEastRoom, 2, scmBlueEnvironment}}) {
            QVERIFY(map()->addRoom(roomId));
            QVERIFY(map()->setRoomArea(roomId, mAreaId));
            QVERIFY(map()->setRoomCoordinates(roomId, x, 0, 0));
            roomDB()->getRoom(roomId)->environment = environment;
        }
        QVERIFY(map()->setExit(scmWestRoom, scmEastRoom, DIR_EAST));
        QVERIFY(map()->setExit(scmEastRoom, scmWestRoom, DIR_WEST));
        map()->mCustomEnvColors[scmRedEnvironment] = QColor(Qt::red);
        map()->mCustomEnvColors[scmBlueEnvironment] = QColor(Qt::blue);
        mpHost->mMapperShowRoomBorders = false;
    }

    // A room fills rSize of its cell, centred on its coordinates, in its environment's colour
    void aRoomIsFilledWithItsEnvironmentColour()
    {
        TMapPainter painter;
        painter.setView(view());
        QImage image = blankImage();
        {
            QPainter imagePainter(&image);
            QFont roomIdFont;
            QFont roomNameFont;
            QPen pen;
            for (const int roomId : {scmWestRoom, scmEastRoom}) {
                TRoom* pRoom = roomDB()->getRoom(roomId);
                const QPointF centre = onImage(QPointF(pRoom->x(), pRoom->y()));
                painter.drawRoom(imagePainter, roomIdFont, roomNameFont, pen, pRoom, false, false, false, -1, centre.x(), centre.y(), false);
            }
        }

        const QPoint westCentre = onImage(QPointF(0, 0)).toPoint();
        const QPoint eastCentre = onImage(QPointF(2, 0)).toPoint();
        QVERIFY2(image.pixelColor(westCentre) == QColor(Qt::red), qPrintable(saved(image, qsl("rooms"))));
        QVERIFY2(image.pixelColor(eastCentre) == QColor(Qt::blue), qPrintable(saved(image, qsl("rooms"))));
        // Half the default rSize of 0.5 is a quarter of the cell either side of the centre
        const int quarterCell = qRound(scmRoomPixels / 4);
        QVERIFY2(image.pixelColor(westCentre + QPoint(quarterCell - 2, 0)) == QColor(Qt::red), qPrintable(saved(image, qsl("rooms"))));
        QVERIFY2(image.pixelColor(westCentre + QPoint(quarterCell + 2, 0)) == QColor(Qt::black), qPrintable(saved(image, qsl("rooms"))));
    }

    // A door is two strokes across the exit line, in the colour of its status
    void aDoorIsDrawnAcrossItsExitInItsStatusColour()
    {
        TRoom* pWestRoom = roomDB()->getRoom(scmWestRoom);
        QVERIFY(pWestRoom->setDoor(qsl("e"), 1));

        TMapPainter painter;
        painter.setView(view());
        QImage image = blankImage();
        const QLineF exitLine(onImage(QPointF(0, 0)), onImage(QPointF(2, 0)));
        {
            QPainter imagePainter(&image);
            QPen pen(Qt::white);
            pen.setWidthF(3.0);
            imagePainter.setPen(pen);
            painter.drawDoor(imagePainter, *pWestRoom, qsl("e"), exitLine);
        }

        const auto [strokeA, strokeB] = ExitAppearance::doorStrokes(exitLine, scmRoomPixels * 2);
        for (const QLineF& stroke : {strokeA, strokeB}) {
            QVERIFY2(image.pixelColor(stroke.center().toPoint()) == ExitAppearance::doorColor(1), qPrintable(saved(image, qsl("door"))));
        }
        // Nothing is drawn away from the two strokes, whose pen is 4.5 pixels wide
        int strayPixels = 0;
        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                if (image.pixelColor(x, y) != QColor(Qt::black) && distanceTo(strokeA, QPointF(x, y)) > 3.5 && distanceTo(strokeB, QPointF(x, y)) > 3.5) {
                    ++strayPixels;
                }
            }
        }
        QVERIFY2(strayPixels == 0, qPrintable(saved(image, qsl("door"))));
    }

    // A custom line replaces its exit's straight line and runs through its points in its own colour
    void aCustomLineIsDrawnThroughItsPoints()
    {
        TRoom* pWestRoom = roomDB()->getRoom(scmWestRoom);
        const QList<QPointF> points{QPointF(0, 2), QPointF(2, 2), QPointF(2, 0)};
        pWestRoom->customLines[qsl("e")] = points;
        pWestRoom->customLinesColor[qsl("e")] = lineColour();
        pWestRoom->customLinesStyle[qsl("e")] = Qt::SolidLine;
        pWestRoom->customLinesArrow[qsl("e")] = false;
        TArea* pArea = roomDB()->getArea(mAreaId);
        QVERIFY(pArea);

        TMapPainter painter;
        painter.setView(view());
        QImage image = blankImage();
        {
            QPainter imagePainter(&image);
            QPen pen(Qt::white);
            const float exitWidth = 3.0f;
            pen.setWidthF(exitWidth);
            imagePainter.setPen(pen);
            const QRect roomBounds = TMapPainter::viewportRoomBounds(scmOriginX, scmOriginY, scmRoomPixels, scmRoomPixels, scmImageWidth, scmImageHeight);
            const QList<int> viewportRooms = pArea->getGridIndex().roomsInViewport(0, roomBounds.left(), roomBounds.right(), roomBounds.top(), roomBounds.bottom());
            QList<TMapPainter::ExitToPaint> exitList;
            QMap<int, QPointF> areaExitsMap;
            painter.paintRoomExits(imagePainter, pen, exitList, pArea, 0, roomBounds, viewportRooms, exitWidth, areaExitsMap);
        }

        // The line leaves from the side of the room its exit points to, so its first stretch is slanted
        for (const QPointF& along : {QPointF(1, 2), QPointF(2, 1)}) {
            QVERIFY2(image.pixelColor(onImage(along).toPoint()) == lineColour(), qPrintable(saved(image, qsl("custom-line"))));
        }
        // Inside the loop the custom line draws, nothing else is drawn
        QVERIFY2(image.pixelColor(onImage(QPointF(1, 1)).toPoint()) == QColor(Qt::black), qPrintable(saved(image, qsl("custom-line"))));
    }

    // An image label is scaled to fill the rectangle it is given
    void anImageLabelIsScaledIntoItsRectangle()
    {
        TMapLabel label;
        label.pix = QPixmap(10, 10);
        label.pix.fill(Qt::magenta);
        label.size = QSizeF(1.0, 1.0);

        TMapPainter painter;
        painter.setView(view());
        QImage image = blankImage();
        const QRectF paintRect(250, 50, 60, 40);
        {
            QPainter imagePainter(&image);
            painter.drawScaledLabel(imagePainter, paintRect.topLeft(), label, 1, paintRect);
        }

        QVERIFY2(image.pixelColor(paintRect.topLeft().toPoint() + QPoint(2, 2)) == QColor(Qt::magenta), qPrintable(saved(image, qsl("label"))));
        QVERIFY2(image.pixelColor(paintRect.bottomRight().toPoint() - QPoint(2, 2)) == QColor(Qt::magenta), qPrintable(saved(image, qsl("label"))));
        QVERIFY2(image.pixelColor(paintRect.bottomRight().toPoint() + QPoint(2, 2)) == QColor(Qt::black), qPrintable(saved(image, qsl("label"))));
        QCOMPARE(label.clickSize, paintRect.size());
        QCOMPARE(painter.textLabelPixmapCacheCount(), 1);
    }

    // A speedwalk runs Lua, which may delete the rooms being drawn, so a click the painter resolves is handed back
    // to whoever drew it rather than started
    void aClickOnARoomIsHandedBackAsAPendingSpeedwalk()
    {
        TMapPainter::View clickedView = view();
        clickedView.mPick = true;
        clickedView.mStartSpeedWalk = true;
        clickedView.mPHighlight = onImage(QPointF(2, 0)).toPoint();
        TMapPainter painter;
        painter.setView(clickedView);
        QImage image = blankImage();
        {
            QPainter imagePainter(&image);
            QFont roomIdFont;
            QFont roomNameFont;
            QPen pen;
            TRoom* pEastRoom = roomDB()->getRoom(scmEastRoom);
            const QPointF centre = onImage(QPointF(2, 0));
            painter.drawRoom(imagePainter, roomIdFont, roomNameFont, pen, pEastRoom, false, false, false, scmWestRoom, centre.x(), centre.y(), false);
        }

        QVERIFY(!painter.view().mPick);
        QVERIFY(!painter.view().mStartSpeedWalk);
        const auto pending = painter.takePendingSpeedWalk();
        QVERIFY(pending.has_value());
        QCOMPARE(pending->first, scmWestRoom);
        QCOMPARE(pending->second, scmEastRoom);
        QVERIFY(!painter.takePendingSpeedWalk().has_value());
    }
};

#include "MapPainterTest.moc"
MUDLET_GROUPED_TEST_MAIN(MapPainterTest)
