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

#include <QFutureWatcher>
#include <QImage>
#include <QPainter>
#include <QScopeGuard>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <memory>

#include "ExitAppearance.h"
#include "Host.h"
#include "HostManager.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "RoomAppearance.h"
#include "TArea.h"
#include "TMap.h"
#include "TMapLabel.h"
#include "TMapPainter.h"
#include "TRoom.h"
#include "T2DMap.h"
#include "TNullMapView.h"
#include "TRoomDB.h"
#include "mapInfoContributorManager.h"
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

    // The image an export wrote, once the save it handed to a pool thread has reported back to its owner
    static QImage exported(const QObject* pSaveOwner, const QString& filePath)
    {
        if (!QTest::qWaitFor(
                    [pSaveOwner]() {
                        return pSaveOwner->findChildren<QFutureWatcherBase*>(QString(), Qt::FindDirectChildrenOnly).isEmpty();
                    },
                    10000)) {
            return QImage();
        }
        return QImage(filePath).convertToFormat(QImage::Format_ARGB32);
    }

    static bool holds(const QImage& image, const QColor& colour)
    {
        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                if (image.pixelColor(x, y) == colour) {
                    return true;
                }
            }
        }
        return false;
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

    static constexpr int scmPlayerMarkerStyle = 3;
    static QColor markerColour() { return QColor(255, 128, 0); }
    static QColor infoBoxColour() { return QColor(0, 128, 128); }
    // Centred on the west room at the area's zoom of 7.5, a map unit is 40 pixels across the 400 by 300 view
    static constexpr qreal scmViewZoom = 7.5;
    static QPoint westCentreInView() { return QPoint(200, 150); }
    static QPoint eastCentreInView() { return QPoint(280, 150); }

    // The player in the west room, drawn as a solid marker, and the settings T2DMap::init() takes from the profile
    void setUpPlayerView()
    {
        map()->mRoomIdHash[map()->mProfileName] = scmWestRoom;
        map()->mPlayerRoomStyle = scmPlayerMarkerStyle;
        map()->mPlayerRoomInnerDiameterPercentage = 0;
        map()->mPlayerRoomInnerColor = markerColour();
        map()->mPlayerRoomOuterColor = markerColour();
        roomDB()->getArea(mAreaId)->set2DMapZoom(scmViewZoom);
        mpHost->mMapperUseAntiAlias = false;
        mpHost->mMapperCenterSmallAreas = false;
        mpHost->mMapInfoBg = infoBoxColour();
    }

    TMapPainter::View playerView() const
    {
        TMapPainter::View view;
        view.mpMap = map();
        view.mpHost = mpHost;
        view.mSize = QSize(scmImageWidth, scmImageHeight);
        view.eSize = mpHost->mLineSize;
        view.mBSize = mpHost->mRoomBorderSize;
        view.rSize = mpHost->mRoomSize;
        view.mMapperUseAntiAlias = mpHost->mMapperUseAntiAlias;
        view.mLargeAreaExitArrows = mpHost->getLargeAreaExitArrows();
        view.mPlayerRoomColorGradientStops =
                RoomAppearance::playerRoomStops(map()->mPlayerRoomStyle, map()->mPlayerRoomInnerDiameterPercentage, map()->mPlayerRoomInnerColor, map()->mPlayerRoomOuterColor);
        return view;
    }

    static QImage paintedView(TMapPainter& painter, const TMapPainter::View& view, TMapPainter::PaintResult& result)
    {
        QImage image = blankImage();
        QPainter imagePainter(&image);
        painter.paintView(imagePainter, view, result);
        return image;
    }

    // An info box line from a contributor of the test's own, which counts its calls and keeps what it was asked about
    struct InfoBoxCalls
    {
        int count = 0;
        int roomId = 0;
        int selectionSize = 0;
        int displayAreaId = 0;
    };
    std::shared_ptr<InfoBoxCalls> showInfoBox()
    {
        auto calls = std::make_shared<InfoBoxCalls>();
        map()->mMapInfoContributorManager->registerContributor(qsl("MapPainterTest"), [calls](int roomID, int selectionSize, int, int displayAreaId, QColor&) {
            ++calls->count;
            calls->roomId = roomID;
            calls->selectionSize = selectionSize;
            calls->displayAreaId = displayAreaId;
            return MapInfoProperties{false, false, qsl("Painted by the test"), QColor(Qt::white)};
        });
        mpHost->mMapInfoContributors = {qsl("MapPainterTest")};
        return calls;
    }

    void hideInfoBox()
    {
        map()->mMapInfoContributorManager->removeContributor(qsl("MapPainterTest"));
        mpHost->mMapInfoContributors.clear();
    }

    // What both the painter and the mapper widget must show of the view: the player's marker on the west room,
    // the east room, and the info box's background above its first line
    static QString probeFailure(const QImage& image)
    {
        if (image.pixelColor(westCentreInView()) != markerColour()) {
            return qsl("the player's marker is %1").arg(image.pixelColor(westCentreInView()).name());
        }
        if (image.pixelColor(eastCentreInView()) != QColor(Qt::blue)) {
            return qsl("the east room is %1").arg(image.pixelColor(eastCentreInView()).name());
        }
        if (image.pixelColor(12, 12) != infoBoxColour()) {
            return qsl("the info box corner is %1").arg(image.pixelColor(12, 12).name());
        }
        return QString();
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

    // An export makes the room symbols at a room size of its own, so the view it was made from makes them again
    void aRoomDrawnAfterAnExportIsDrawnAsBefore()
    {
        roomDB()->getRoom(scmEastRoom)->mSymbol = qsl("W");
        TMapPainter painter;
        painter.setView(view());
        const auto drawnEastRoom = [this, &painter]() {
            QImage image = blankImage();
            QPainter imagePainter(&image);
            QFont roomIdFont;
            QFont roomNameFont;
            QPen pen;
            const QPointF centre = onImage(QPointF(2, 0));
            painter.drawRoom(imagePainter, roomIdFont, roomNameFont, pen, roomDB()->getRoom(scmEastRoom), false, false, false, scmWestRoom, centre.x(), centre.y(), false);
            return image;
        };
        const QImage before = drawnEastRoom();
        QCOMPARE(painter.symbolPixmapCacheCount(), 1);

        QObject saveOwner;
        const QString exportFile = qsl("%1/symbol-export.png").arg(mConfigDir.path());
        const auto [exportStarted, exportMessage] = painter.exportArea(mAreaId, exportFile, 0, 1.0, false, &saveOwner);
        QVERIFY2(exportStarted, qPrintable(exportMessage));
        QVERIFY(!exported(&saveOwner, exportFile).isNull());

        const QImage after = drawnEastRoom();
        QVERIFY2(after == before, qPrintable(saved(after, qsl("after-export")) + qsl("; ") + saved(before, qsl("before-export"))));
    }

    // With no main window the map is drawn by the same code as the mapper's, so an export is the same image
    void anExportWithNoMainWindowIsTheMappersImage()
    {
        TRoom* pWestRoom = roomDB()->getRoom(scmWestRoom);
        TRoom* pEastRoom = roomDB()->getRoom(scmEastRoom);
        QVERIFY(pWestRoom->setDoor(qsl("e"), 2));
        pWestRoom->customLines[qsl("n")] = QList<QPointF>{QPointF(0, 2), QPointF(2, 2)};
        pWestRoom->customLinesColor[qsl("n")] = lineColour();
        pWestRoom->customLinesStyle[qsl("n")] = Qt::DashLine;
        pWestRoom->customLinesArrow[qsl("n")] = true;
        pWestRoom->calcRoomDimensions();
        pEastRoom->setExitStub(DIR_NORTH, true);
        const int otherAreaId = roomDB()->addArea(qsl("Elsewhere"));
        constexpr int roomElsewhere = 3;
        QVERIFY(map()->addRoom(roomElsewhere));
        QVERIFY(map()->setRoomArea(roomElsewhere, otherAreaId));
        QVERIFY(map()->setRoomCoordinates(roomElsewhere, 4, 0, 0));
        QVERIFY(map()->setExit(scmEastRoom, roomElsewhere, DIR_EAST));
        TArea* pArea = roomDB()->getArea(mAreaId);
        TMapLabel label;
        label.pos = QVector3D(-2, 2, 0);
        label.size = QSizeF(1, 1);
        label.pix = QPixmap(10, 10);
        label.pix.fill(Qt::magenta);
        pArea->mMapLabels.insert(1, label);
        mpHost->mShowRoomID = true;
        auto restoreRoomIds = qScopeGuard([this]() {
            mpHost->mShowRoomID = false;
        });

        // What dlgMapper's constructor sets on its 2D map
        T2DMap mapperMap;
        mapperMap.mpMap = map();
        mapperMap.mpHost = mpHost;
        mapperMap.init();
        mapperMap.mShowRoomID = mpHost->mShowRoomID;
        mapperMap.mBubbleMode = mpHost->mBubbleMode;
        TNullMapView nullView(mpHost);

        const QString mapperFile = qsl("%1/mapper.png").arg(mConfigDir.path());
        const QString nullViewFile = qsl("%1/null-view.png").arg(mConfigDir.path());
        const auto [mapperExported, mapperMessage] = mapperMap.exportAreaToImage(mAreaId, mapperFile, 0, 1.0);
        QVERIFY2(mapperExported, qPrintable(mapperMessage));
        const auto [nullViewExported, nullViewMessage] = nullView.exportAreaToImage(mAreaId, nullViewFile, 0, 1.0, false);
        QVERIFY2(nullViewExported, qPrintable(nullViewMessage));

        const QImage mapperImage = exported(&mapperMap, mapperFile);
        const QImage nullViewImage = exported(&nullView, nullViewFile);
        QVERIFY(!mapperImage.isNull());
        QVERIFY(!nullViewImage.isNull());
        QVERIFY(holds(mapperImage, Qt::red));
        QVERIFY(holds(mapperImage, Qt::blue));
        QVERIFY(holds(mapperImage, Qt::magenta));
        QCOMPARE(nullViewImage.size(), mapperImage.size());
        QVERIFY2(nullViewImage == mapperImage, qPrintable(saved(nullViewImage, qsl("null-view-export")) + qsl("; ") + saved(mapperImage, qsl("mapper-export"))));
    }

    // A whole view is painted centred on the player, with their marker, the rooms around them and the info box
    void aViewIsPaintedCentredOnThePlayer()
    {
        setUpPlayerView();
        const auto calls = showInfoBox();
        auto removeInfoBox = qScopeGuard([this]() {
            hideInfoBox();
        });

        TMapPainter painter;
        TMapPainter::PaintResult result;
        const QImage image = paintedView(painter, playerView(), result);

        const QString failure = probeFailure(image);
        QVERIFY2(failure.isEmpty(), qPrintable(failure + qsl("; ") + saved(image, qsl("view"))));
        // Half a cell is a room's whole width at the default room size, so the cell's edge is the background
        QVERIFY2(image.pixelColor(eastCentreInView() + QPoint(18, 0)) == mpHost->mBgColor_2, qPrintable(saved(image, qsl("view"))));
        QCOMPARE(result.view.mAreaID, mAreaId);
        QCOMPARE(result.view.mRoomID, scmWestRoom);
        QCOMPARE(result.view.mRX, westCentreInView().x());
        QCOMPARE(result.view.mRY, westCentreInView().y());
        QCOMPARE(result.view.xyzoom, scmViewZoom);
        QVERIFY(result.view.mShiftMode);
        QVERIFY(result.areaChanged.has_value());
        QCOMPARE(result.areaChanged->first, mAreaId);
        QCOMPARE(result.areaChanged->second, -2);
        QCOMPARE(calls->count, 1);
        QCOMPARE(calls->roomId, scmWestRoom);
        QCOMPARE(calls->displayAreaId, mAreaId);
    }

    // In grid mode a room fills its whole cell
    void aGridModeViewFillsEachCell()
    {
        setUpPlayerView();
        roomDB()->getArea(mAreaId)->gridMode = true;

        TMapPainter painter;
        TMapPainter::PaintResult result;
        const QImage image = paintedView(painter, playerView(), result);

        for (const QPoint& offset : {QPoint(0, 0), QPoint(18, 0), QPoint(-18, 18)}) {
            QVERIFY2(image.pixelColor(eastCentreInView() + offset) == QColor(Qt::blue), qPrintable(saved(image, qsl("grid"))));
        }
        QVERIFY2(image.pixelColor(eastCentreInView() + QPoint(-40, 0)) == mpHost->mBgColor_2, qPrintable(saved(image, qsl("grid"))));
    }

    // Zoomed out to a couple of pixels a room, a room is a blob of its colour and the player's still has its marker
    void aZoomedOutViewDrawsRoomsAsBlobs()
    {
        setUpPlayerView();
        roomDB()->getArea(mAreaId)->set2DMapZoom(150.0);

        TMapPainter painter;
        TMapPainter::PaintResult result;
        const QImage image = paintedView(painter, playerView(), result);

        QVERIFY(result.view.mRoomWidth < TMapPainter::cLodPixelThreshold);
        QVERIFY2(image.pixelColor(westCentreInView() + QPoint(4, 0)) == QColor(Qt::blue), qPrintable(saved(image, qsl("lod"))));
        QVERIFY2(image.pixelColor(westCentreInView()) == markerColour(), qPrintable(saved(image, qsl("lod"))));
    }

    // The info box starts to the right of the selection's room list while that is shown
    void theInfoBoxStartsRightOfTheSelectionList()
    {
        setUpPlayerView();
        showInfoBox();
        auto removeInfoBox = qScopeGuard([this]() {
            hideInfoBox();
        });
        TMapPainter::View view = playerView();
        view.mSelectionListVisible = true;
        view.mSelectionListX = 0;
        view.mSelectionListWidth = 100;

        TMapPainter painter;
        TMapPainter::PaintResult result;
        const QImage image = paintedView(painter, view, result);

        QVERIFY2(image.pixelColor(112, 12) == infoBoxColour(), qPrintable(saved(image, qsl("info-box"))));
        QVERIFY2(image.pixelColor(12, 12) != infoBoxColour(), qPrintable(saved(image, qsl("info-box"))));
    }

    // Following the player into another area drops the selected rooms left behind, and says the area changed
    void followingThePlayerIntoAnotherAreaPrunesTheSelection()
    {
        setUpPlayerView();
        const int otherAreaId = roomDB()->addArea(qsl("Left behind"));
        constexpr int roomLeftBehind = 3;
        QVERIFY(map()->addRoom(roomLeftBehind));
        QVERIFY(map()->setRoomArea(roomLeftBehind, otherAreaId));
        QVERIFY(map()->setRoomCoordinates(roomLeftBehind, 0, 0, 0));
        const auto calls = showInfoBox();
        auto removeInfoBox = qScopeGuard([this]() {
            hideInfoBox();
        });
        TMapPainter::View view = playerView();
        view.mAreaID = otherAreaId;
        view.mLastViewedAreaID = otherAreaId;
        view.mMultiSelectionSet = {scmWestRoom, roomLeftBehind};
        view.mMultiSelectionHighlightRoomId = roomLeftBehind;
        view.mSelectionListVisible = true;
        view.mSelectionListWidth = 100;

        TMapPainter painter;
        TMapPainter::PaintResult result;
        const QImage image = paintedView(painter, view, result);

        QVERIFY(result.selectionPruned);
        QCOMPARE(result.view.mMultiSelectionSet, QSet<int>{scmWestRoom});
        QCOMPARE(result.view.mMultiSelectionHighlightRoomId, scmWestRoom);
        QVERIFY(!result.view.mSelectionListVisible);
        QVERIFY(result.areaChanged.has_value());
        QCOMPARE(result.areaChanged->first, mAreaId);
        QCOMPARE(result.areaChanged->second, otherAreaId);
        QCOMPARE(result.view.mLastViewedAreaID, mAreaId);
        QCOMPARE(calls->roomId, scmWestRoom);
        QCOMPARE(calls->selectionSize, 1);
        QVERIFY2(image.pixelColor(12, 12) == infoBoxColour(), qPrintable(saved(image, qsl("pruned"))));
    }

    // A click on an area exit's marker is handed back as a speedwalk to the room it leads to
    void aClickOnAnAreaExitIsHandedBackAsAPendingSpeedwalk()
    {
        setUpPlayerView();
        const int otherAreaId = roomDB()->addArea(qsl("Through the exit"));
        constexpr int roomThroughTheExit = 3;
        QVERIFY(map()->addRoom(roomThroughTheExit));
        QVERIFY(map()->setRoomArea(roomThroughTheExit, otherAreaId));
        QVERIFY(map()->setRoomCoordinates(roomThroughTheExit, 4, 0, 0));
        QVERIFY(map()->setExit(scmEastRoom, roomThroughTheExit, DIR_EAST));
        TMapPainter::View view = playerView();
        view.mAreaID = mAreaId;
        view.mPick = true;
        view.mStartSpeedWalk = true;
        // The marker is a room's width beyond the room the exit leaves
        view.mPHighlight = eastCentreInView() + QPoint(40, 0);

        TMapPainter painter;
        TMapPainter::PaintResult result;
        paintedView(painter, view, result);

        QVERIFY(!result.view.mPick);
        QVERIFY(!result.view.mStartSpeedWalk);
        QVERIFY(result.pendingSpeedWalk.has_value());
        QCOMPARE(result.pendingSpeedWalk->first, scmWestRoom);
        QCOMPARE(result.pendingSpeedWalk->second, roomThroughTheExit);
    }

    // The mapper widget draws through the painter, so it shows the same view
    void theMapperWidgetPaintsTheSameView()
    {
        setUpPlayerView();
        showInfoBox();
        auto removeInfoBox = qScopeGuard([this]() {
            hideInfoBox();
        });
        T2DMap mapperMap;
        mapperMap.mpMap = map();
        mapperMap.mpHost = mpHost;
        mapperMap.init();
        mapperMap.setPlayerRoomStyle(map()->mPlayerRoomStyle);
        mapperMap.resize(scmImageWidth, scmImageHeight);

        const QImage image = mapperMap.grab().toImage().convertToFormat(QImage::Format_ARGB32);

        QCOMPARE(image.size(), QSize(scmImageWidth, scmImageHeight));
        const QString failure = probeFailure(image);
        QVERIFY2(failure.isEmpty(), qPrintable(failure + qsl("; ") + saved(image, qsl("mapper-widget"))));
        QCOMPARE(mapperMap.getAreaId(), mAreaId);
        QCOMPARE(mapperMap.getCenterRoomId(), scmWestRoom);
    }

    // The info box's Lua reads the area from the mapper, as getMapZoom() and setMapZoom() do, so the frame has moved it
    // there by then; and an export from that Lua leaves the frame to finish as it was drawing it
    void theInfoBoxLuaSeesTheFramesAreaAndCanExport()
    {
        setUpPlayerView();
        T2DMap mapperMap;
        mapperMap.mpMap = map();
        mapperMap.mpHost = mpHost;
        mapperMap.init();
        mapperMap.setPlayerRoomStyle(map()->mPlayerRoomStyle);
        mapperMap.resize(scmImageWidth, scmImageHeight);
        const QString exportFile = qsl("%1/from-the-info-box.png").arg(mConfigDir.path());
        QList<int> areasSeen;
        map()->mMapInfoContributorManager->registerContributor(qsl("MapPainterTest"), [&mapperMap, &areasSeen, exportFile, this](int, int, int, int, QColor&) {
            areasSeen.append(mapperMap.getAreaId());
            if (areasSeen.size() == 1) {
                mapperMap.exportAreaToImage(mAreaId, exportFile, 0, 1.0);
            }
            return MapInfoProperties{false, false, qsl("Painted by the test"), QColor(Qt::white)};
        });
        mpHost->mMapInfoContributors = {qsl("MapPainterTest")};
        auto removeInfoBox = qScopeGuard([this]() {
            hideInfoBox();
        });

        const QImage image = mapperMap.grab().toImage().convertToFormat(QImage::Format_ARGB32);
        mapperMap.grab();

        QCOMPARE(areasSeen, (QList<int>{mAreaId, mAreaId}));
        QVERIFY(!exported(&mapperMap, exportFile).isNull());
        const QString failure = probeFailure(image);
        QVERIFY2(failure.isEmpty(), qPrintable(failure + qsl("; ") + saved(image, qsl("info-box-export"))));
        QCOMPARE(roomDB()->getArea(mAreaId)->get2DMapZoom(), scmViewZoom);
        QCOMPARE(mapperMap.getAreaId(), mAreaId);
        QCOMPARE(mapperMap.getCenterRoomId(), scmWestRoom);
    }

    // Square and round rooms make their symbols at different sizes, and each painter keeps its own, so one painter
    // switching to round rooms leaves another to remake its symbols when it switches
    void eachPainterRemakesItsSymbolsForRoundRooms()
    {
        setUpPlayerView();
        hideInfoBox();
        roomDB()->getRoom(scmEastRoom)->mSymbol = qsl("W");
        TMapPainter::View roundView = playerView();
        roundView.mBubbleMode = true;

        TMapPainter painter;
        TMapPainter otherPainter;
        TMapPainter freshPainter;
        TMapPainter::PaintResult squareResult;
        paintedView(painter, playerView(), squareResult);
        TMapPainter::PaintResult result;
        paintedView(otherPainter, roundView, result);
        // As the painter's caller keeps the view between frames, so the room size the symbols were made at
        TMapPainter::View roundAgain = squareResult.view;
        roundAgain.mBubbleMode = true;
        const QImage round = paintedView(painter, roundAgain, result);
        const QImage fresh = paintedView(freshPainter, roundView, result);
        QVERIFY2(round == fresh, qPrintable(saved(round, qsl("round")) + qsl("; ") + saved(fresh, qsl("round-fresh"))));
    }

    // A double-click no room or area exit takes is spent with its frame, or a later one would act on whatever moved
    // under it
    void aClickNothingTakesIsSpentWithItsFrame()
    {
        setUpPlayerView();
        hideInfoBox();
        TMapPainter::View view = playerView();
        view.mPick = true;
        view.mStartSpeedWalk = true;
        view.mPHighlight = westCentreInView() - QPoint(80, 0);

        TMapPainter painter;
        TMapPainter::PaintResult result;
        paintedView(painter, view, result);

        QVERIFY(!result.view.mPick);
        QVERIFY(!result.view.mStartSpeedWalk);
        QVERIFY(!result.pendingSpeedWalk.has_value());
    }

    // So is one on a frame that draws no area, with or without saying why, or the first frame that has one would
    // act on it
    void aClickOnAFrameWithNoAreaIsSpent()
    {
        setUpPlayerView();
        hideInfoBox();
        TMapPainter::View view = playerView();
        view.mPick = true;
        view.mStartSpeedWalk = true;
        view.mPHighlight = eastCentreInView();
        // A secondary view stays on its own area, which may be gone
        view.mIsSecondaryView = true;
        view.mAreaID = mAreaId + 1;
        QVERIFY(!roomDB()->getArea(view.mAreaID));
        TMapPainter painter;
        TMapPainter::PaintResult result;
        paintedView(painter, view, result);
        QVERIFY(!result.view.mPick);
        QVERIFY(!result.view.mStartSpeedWalk);

        view.mIsSecondaryView = false;
        map()->mapClear();
        for (const bool suppressed : {false, true}) {
            view.mSuppressEmptyStateMessage = suppressed;
            paintedView(painter, view, result);
            QVERIFY(!result.view.mPick);
            QVERIFY(!result.view.mStartSpeedWalk);
        }
    }
};

#include "MapPainterTest.moc"
MUDLET_GROUPED_TEST_MAIN(MapPainterTest)
