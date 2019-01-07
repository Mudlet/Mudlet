/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2013-2016, 2018-2019 by Stephen Lyons                   *
 *                                               - slysven@virginmedia.com *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
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


#include "T2DMap.h"


#include "Host.h"
#include "TArea.h"
#include "TConsole.h"
#include "TEvent.h"
#include "TRoomDB.h"
#include "dlgMapper.h"
#include "dlgRoomExits.h"
#include "mudlet.h"

#include "pre_guard.h"
#include <QtEvents>
#include <QtUiTools>
#include "post_guard.h"

T2DMap::T2DMap(QWidget* parent)
: QWidget(parent)
, mpMap()
, xyzoom(20)
, mRX()
, mRY()
, mPick()
, mTarget()
, mStartSpeedWalk()
, mRoomBeingMoved()
, mRoomWidth()
, mRoomHeight()
, mChosenRoomColor(5)
, xspan()
, yspan()
, mMultiSelection()
, mMultiRect()
, mPopupMenu()
, mNewMoveAction()
, mMapInfoRect()
, mFontHeight(20)
, mShowRoomID(false)
, gzoom(20)
, rSize(0.5)
, eSize(3.0)
, mRoomID()
, mAreaID()
, mOx()
, mOy()
, mOz()
, mShiftMode()
, mShowInfo(true)
, arealist_combobox()
, mpCustomLinesDialog()
, mCustomLinesRoomFrom()
, mCustomLinesRoomTo()
, mpCurrentLineStyle()
, mCurrentLineStyle(Qt::SolidLine)
, mpCurrentLineColor()
, mCurrentLineColor(Qt::red)
, mpCurrentLineArrow()
, mCurrentLineArrow(true)
, mBubbleMode()
, mMapperUseAntiAlias(true)
, mLabelHighlighted(false)
, mMoveLabel()
, mCustomLineSelectedRoom()
, mCustomLineSelectedExit()
, mCustomLineSelectedPoint(-1)
, mMultiSelectionListWidget(this)
, mSizeLabel()
, isCenterViewCall()
, mDialogLock()
, mMultiSelectionHighlightRoomId(0)
, mIsSelectionSorting(true)
, mIsSelectionSortByNames()
, mIsSelectionUsingNames(false)
, mSymbolFontSize(1)
{
    mMultiSelectionListWidget.setColumnCount(2);
    mMultiSelectionListWidget.hideColumn(1);
    QStringList headerLabels;
    headerLabels << tr("Room Id") << tr("Room Name");
    mMultiSelectionListWidget.setHeaderLabels(headerLabels);
    mMultiSelectionListWidget.setToolTip(
	    QStringLiteral("<html><head/><body><p>%1</p></body></html>").arg(
                    tr("Click on a line to select or deselect that room number (with the given name if the rooms are named) to add or remove the room from the selection. "
                       "Click on the relevant header to sort by that method.  Note that the name column will only show if at least one of the rooms has a name.")));
    mMultiSelectionListWidget.setUniformRowHeights(true);
    mMultiSelectionListWidget.setItemsExpandable(false);
    mMultiSelectionListWidget.setSelectionMode(QAbstractItemView::MultiSelection); // Was ExtendedSelection
    mMultiSelectionListWidget.setRootIsDecorated(false);
    QSizePolicy multiSelectionSizePolicy(QSizePolicy::Maximum, QSizePolicy::Expanding);
    mMultiSelectionListWidget.setSizePolicy(multiSelectionSizePolicy);
    mMultiSelectionListWidget.setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    mMultiSelectionListWidget.setFrameShape(QFrame::NoFrame);
    mMultiSelectionListWidget.setFrameShadow(QFrame::Plain);
    mMultiSelectionListWidget.header()->setProperty("showSortIndicator", QVariant(true));
    mMultiSelectionListWidget.header()->setSectionsMovable(false);
    mMultiSelectionListWidget.header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    mMultiSelectionListWidget.header()->setStretchLastSection(true);
    mMultiSelectionListWidget.setSortingEnabled(mIsSelectionSorting);
    mMultiSelectionListWidget.resize(120, 100);
    mMultiSelectionListWidget.move(0, 0);
    mMultiSelectionListWidget.hide();
    connect(&mMultiSelectionListWidget, &QTreeWidget::itemSelectionChanged, this, &T2DMap::slot_roomSelectionChanged);
}

void T2DMap::init()
{
    if (!mpHost || !mpMap) {
        return;
    }

    isCenterViewCall = false;

    eSize = mpMap->mpHost->mLineSize;
    rSize = mpMap->mpHost->mRoomSize;
    mMapperUseAntiAlias = mpHost->mMapperUseAntiAlias;
    flushSymbolPixmapCache();
}

QColor T2DMap::getColor(int id)
{
    QColor color;

    TRoom* room = mpMap->mpRoomDB->getRoom(id);
    if (!room) {
        return color;
    }

    int env = room->environment;
    if (mpMap->envColors.contains(env)) {
        env = mpMap->envColors[env];
    } else {
        if (!mpMap->customEnvColors.contains(env)) {
            env = 1;
        }
    }
    switch (env) {
    case 1:
        color = mpHost->mRed_2;
        break;

    case 2:
        color = mpHost->mGreen_2;
        break;
    case 3:
        color = mpHost->mYellow_2;
        break;

    case 4:
        color = mpHost->mBlue_2;
        break;

    case 5:
        color = mpHost->mMagenta_2;
        break;
    case 6:
        color = mpHost->mCyan_2;
        break;
    case 7:
        color = mpHost->mWhite_2;
        break;
    case 8:
        color = mpHost->mBlack_2;
        break;

    case 9:
        color = mpHost->mLightRed_2;
        break;

    case 10:
        color = mpHost->mLightGreen_2;
        break;
    case 11:
        color = mpHost->mLightYellow_2;
        break;

    case 12:
        color = mpHost->mLightBlue_2;
        break;

    case 13:
        color = mpHost->mLightMagenta_2;
        break;
    case 14:
        color = mpHost->mLightCyan_2;
        break;
    case 15:
        color = mpHost->mLightWhite_2;
        break;
    case 16:
        color = mpHost->mLightBlack_2;
        break;
    default: //user defined room color
        if (!mpMap->customEnvColors.contains(env)) {
            if (16 < env && env < 232)
            {
                quint8 base = env - 16;
                quint8 r = base / 36;
                quint8 g = (base - (r * 36)) / 6;
                quint8 b = (base - (r * 36)) - (g * 6);

		r = r * 51;
		g = g * 51;
		b = b * 51;
                color = QColor(r, g, b, 255);
            } else if (231 < env && env < 256) {
                quint8 k = ((env - 232) * 10) + 8;
                color = QColor(k, k, k, 255);
            }
            break;
        }
        color = mpMap->customEnvColors[env];
    }
    return color;
}

void T2DMap::shiftDown()
{
    mShiftMode = true;
    mOy--;
    update();
}

void T2DMap::toggleShiftMode()
{
    mShiftMode = !mShiftMode;
    update();
}

void T2DMap::shiftUp()
{
    mShiftMode = true;
    mOy++;
    update();
}

void T2DMap::shiftLeft()
{
    mShiftMode = true;
    mOx--;
    update();
}

void T2DMap::shiftRight()
{
    mShiftMode = true;
    mOx++;
    update();
}
void T2DMap::shiftZup()
{
    mShiftMode = true;
    mOz++;
    update();
}

void T2DMap::shiftZdown()
{
    mShiftMode = true;
    mOz--;
    update();
}


void T2DMap::slot_switchArea(const QString& newAreaName)
{
    Host* pHost = mpHost;
    if (!pHost || !mpMap) {
        return;
    }

    int playerRoomId = mpMap->mRoomIdHash.value(pHost->getName());
    TRoom* pPlayerRoom = mpMap->mpRoomDB->getRoom(playerRoomId);
    int playerAreaID = -2; // Cannot be valid (but -1 can be)!
    if (pPlayerRoom) {
        playerAreaID = pPlayerRoom->getArea();
    }

    QMapIterator<int, QString> it(mpMap->mpRoomDB->getAreaNamesMap());
    while (it.hasNext()) {
        it.next();
        int areaID = it.key();

        auto areaName = it.value();
        TArea* area = mpMap->mpRoomDB->getArea(areaID);
        if (area && newAreaName == areaName) {
            mAreaID = areaID;
            mShiftMode = true;
            area->calcSpan();

            if (areaID == playerAreaID) {
                // We are switching back to the area that has the player in it
                // recenter view on that room!
                mOx = pPlayerRoom->x;
                // Map y coordinates are reversed on 2D map!
                mOy = -pPlayerRoom->y;
                mOz = pPlayerRoom->z;
                repaint();
                // Pass the coordinates to the TMap instance to pass to the 3D
                // mapper
                mpMap->set3DViewCenter(mAreaID, mOx, -mOy, mOz);
                // escape early
                return;
            }

            bool validRoomFound = false;
            if (!area->zLevels.contains(mOz)) {
                // If the current map z-coordinate value is NOT one that is used
                // for this then get the FIRST room in the area and goto the
                // mathematical midpoint of all the rooms on the same
                // z-coordinate.
                QSetIterator<int> itRoom(area->getAreaRooms());
                // key is z-coordinate, value is count of rooms on that level
                QMap<int, int> roomsCountLevelMap;
                while (itRoom.hasNext()) {
                    int checkRoomID = itRoom.next();
                    TRoom* room = mpMap->mpRoomDB->getRoom(checkRoomID);
                    if (room) {
                        validRoomFound = true;
                        if (roomsCountLevelMap.contains(room->z)) {
                            ++roomsCountLevelMap[room->z];
                        } else {
                            roomsCountLevelMap[room->z] = 1;
                        }
                    }
                }

                if (validRoomFound) {
                    QMapIterator<int, int> itRoomsCount(roomsCountLevelMap);
                    // Start at highest value and work down
                    itRoomsCount.toBack();
                    // This will be Okay as we KNOW there is at least one entry
                    itRoomsCount.previous();
                    int maxRoomCountOnLevel = 0;
                    // Initialisation value, will get overwritten
                    int minLevelWithMaxRoomCount = itRoomsCount.key();
                    // Return to the back so the previous() in the do loop works
                    // correctly
                    itRoomsCount.next();
                    do {
                        itRoomsCount.previous();
                        if (maxRoomCountOnLevel < itRoomsCount.value()) {
                            maxRoomCountOnLevel = itRoomsCount.value();
                            minLevelWithMaxRoomCount = itRoomsCount.key();
                        }
                    } while (itRoomsCount.hasPrevious());

                    // We now have lowest level with the highest number of rooms
                    // Now find the geometry center of the rooms on THAT level
                    // In a similar manner to the getCenterSelection() method
                    itRoom.toFront();
                    float mean_x = 0.0;
                    float mean_y = 0.0;
                    uint processedRoomCount = 0;
                    QSet<TRoom*> roomsToConsider; // Hold on to relevant rooms for
                                         // following step
                    while (itRoom.hasNext()) {
                        TRoom* room = mpMap->mpRoomDB->getRoom(itRoom.next());
                        if (!room || room->z != minLevelWithMaxRoomCount) {
                            continue;
                        }

                        roomsToConsider.insert(room);
                        mean_x += (static_cast<float>(room->x - mean_x)) / ++processedRoomCount;
                        mean_y += (static_cast<float>(room->y - mean_y)) / processedRoomCount;
                    }

                    // We now have the position that is the "centre" of the
                    // rooms on this level - just need to find the room nearest
                    // to that:
                    QSetIterator<TRoom*> itpRoom(roomsToConsider);
                    float closestSquareDistance = -1.0;
                    TRoom* closestCenterRoom = nullptr;
                    while (itpRoom.hasNext()) {
                        TRoom* room = itpRoom.next();
                        QVector2D meanToRoom(static_cast<float>(room->x) - mean_x, static_cast<float>(room->y) - mean_y);
                        if (closestSquareDistance < -0.5) {
                            // Test for first time around loop - for initialisation
                            // Don't use an equality to zero test, we are using
                            // floats so need to allow for a little bit of
                            // fuzzyness!
                            closestSquareDistance = meanToRoom.lengthSquared();
                            closestCenterRoom = room;
                        } else {
                            float currentRoomSquareDistance = meanToRoom.lengthSquared();
                            if (closestSquareDistance > currentRoomSquareDistance) {
                                closestSquareDistance = currentRoomSquareDistance;
                                closestCenterRoom = room;
                            }
                        }
                    }

                    mOx = closestCenterRoom->x;
                    // Map y coordinates are reversed on 2D map!
                    mOy = -closestCenterRoom->y;
                    mOz = closestCenterRoom->z;
                }

                if (!validRoomFound) {
                    //no rooms, go to 0,0,0
                    mOx = 0;
                    mOy = 0;
                    mOz = 0;
                }
            } else {
                // Else the selected area DOES have rooms on the same
                // z-coordinate. Now find the geometric center of the rooms on
                // the given level, in a similar manner to the
                // getCenterSelection() method
                float mean_x = 0.0;
                float mean_y = 0.0;
                uint processedRoomCount = 0;
                QSet<TRoom*> roomsToConsider; // Hold on to relevant rooms for
                                     // following step
                QSetIterator<int> itRoom(area->getAreaRooms());
                while (itRoom.hasNext()) {
                    TRoom* room = mpMap->mpRoomDB->getRoom(itRoom.next());
                    if (!room || room->z != mOz) {
                        continue;
                    }

                    roomsToConsider.insert(room);
                    mean_x += (static_cast<float>(room->x - mean_x)) / ++processedRoomCount;
                    mean_y += (static_cast<float>(room->y - mean_y)) / processedRoomCount;
                }

                // We now have the position that is the "centre" of the
                // rooms on this level - just need to find the room nearest
                // to that:
                QSetIterator<TRoom*> itpRoom(roomsToConsider);
                float closestSquareDistance = -1.0;
                TRoom* closestCenterRoom = nullptr;
                while (itpRoom.hasNext()) {
                    TRoom* room = itpRoom.next();
                    QVector2D meanToRoom(static_cast<float>(room->x) - mean_x, static_cast<float>(room->y) - mean_y);
                    if (closestSquareDistance < -0.5) {
                        // Test for first time around loop - for initialisation
                        // Don't use an equality to zero test, we are using
                        // floats so need to allow for a little bit of
                        // fuzzyness!
                        closestSquareDistance = meanToRoom.lengthSquared();
                        closestCenterRoom = room;
                    } else {
                        float currentRoomSquareDistance = meanToRoom.lengthSquared();
                        if (closestSquareDistance > currentRoomSquareDistance) {
                            closestSquareDistance = currentRoomSquareDistance;
                            closestCenterRoom = room;
                        }
                    }
                }

                if (closestCenterRoom) {
                    mOx = closestCenterRoom->x;
                    // Map y coordinates are reversed on 2D map!
                    mOy = -closestCenterRoom->y;
                }
            }
            repaint();
            // Pass the coordinates to the TMap instance to pass to the 3D mapper
            mpMap->set3DViewCenter(mAreaID, mOx, -mOy, mOz);
            return;
        }
    }
}

// key format: <"W_" or "B_" for White/Black><QString of one or more QChars>
void T2DMap::addSymbolToPixmapCache(const QString key, const bool gridMode)
{
    // Some constants used to prevent small, unreadable symbols:
    static float symbolLowerSizeLimit = 8.0;
    static unsigned int minimumUsableFontSize = 8;

    // Draw onto a rectangle that will fit the room symbol rectangle,
    // Must tweak the size so it fits within circle when round room symbols are
    // used and also accommodate fixed sizes for gridmode:
    QRectF symbolRectangle;
    if (gridMode && mBubbleMode) {
        symbolRectangle = QRectF(0.0, 0.0, 0.707 * mRoomWidth, 0.707 * mRoomHeight);
    } else if (mBubbleMode) {
        symbolRectangle = QRectF(0.0, 0.0, 0.707 * mRoomWidth * rSize, 0.707 * mRoomHeight * rSize);
    } else if (gridMode) {
        symbolRectangle = QRectF(0.0, 0.0, mRoomWidth, mRoomHeight);
    } else {
        symbolRectangle = QRectF(0.0, 0.0, mRoomWidth * rSize, mRoomHeight * rSize);
    }

    QPixmap* pixmap = new QPixmap(symbolRectangle.toRect().size());
    pixmap->fill(Qt::transparent);

    if (symbolRectangle.width() < symbolLowerSizeLimit || symbolRectangle.height() < symbolLowerSizeLimit) {
        // if the space to draw the symbol on is too small then do not create
        // anything on the pixmap as it will be unreadable - instead insert an
        // empty pixmap:
        mSymbolPixmapCache.insert(key, pixmap);
        return;
    }

    QString symbolString(key.mid(2));
    QPainter symbolPainter(pixmap);
    if (key.startsWith(QLatin1String("W_"))) {
        symbolPainter.setPen(Qt::white);
    } else {
        symbolPainter.setPen(Qt::black);
    }
    symbolPainter.setFont(mpMap->mMapSymbolFont);
    symbolPainter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform, true);

    QFontMetrics mapSymbolFontMetrics = symbolPainter.fontMetrics();
    QVector<quint32> codePoints = symbolString.toUcs4();
    QVector<bool> isUsable;
    for (uint i = 0; i < codePoints.size(); ++i) {
        isUsable.append(mapSymbolFontMetrics.inFontUcs4(codePoints.at(i)));
    }

    QFont fontForThisSymbol = mpMap->mMapSymbolFont;
    bool needToFallback = isUsable.contains(false);
    // Oh dear at least one grapheme is not represented in either the selected
    // or any font as set elsewhere
    if (needToFallback) {
        symbolString = QString(QChar::ReplacementCharacter);
        // Clear the setting that may be forcing only the specified font to be
        // used, as it may not have the Replacement Character glyph...
        fontForThisSymbol.setStyleStrategy(static_cast<QFont::StyleStrategy>(mpMap->mMapSymbolFont.styleStrategy() & ~(QFont::NoFontMerging)));
    }

    qreal fudgeFactor = symbolRectangle.toRect().width() * mpMap->mMapSymbolFontFudgeFactor;
    QRectF testRectangle(0, 0, fudgeFactor, fudgeFactor);
    testRectangle.moveCenter(pixmap->rect().center());
    QRectF boundaryRect;
    // Try larger font sizes until it won't fit
    do {
        fontForThisSymbol.setPointSize(++mSymbolFontSize);
        symbolPainter.setFont(fontForThisSymbol);
        boundaryRect = symbolPainter.boundingRect(pixmap->rect(), Qt::AlignCenter, symbolString);
        // Use a limit on mSymbolFontSize otherwise some broken fonts can
        // lock the system into a very slow loop as it gets very large
    } while (testRectangle.contains(boundaryRect) && mSymbolFontSize < 255);
    // Then try smaller ones until it will
    do {
        fontForThisSymbol.setPointSize(--mSymbolFontSize);
        symbolPainter.setFont(fontForThisSymbol);
        boundaryRect = symbolPainter.boundingRect(pixmap->rect(), Qt::AlignCenter, symbolString);
        // Use a limit on mSymbolFontSize otherwise some broken fonts can
        // lock the system into a very slow loop as it gets very large
    } while (!testRectangle.contains(boundaryRect) && mSymbolFontSize > minimumUsableFontSize);

    if (testRectangle.contains(boundaryRect)) {
        fontForThisSymbol.setPointSize(++mSymbolFontSize);
        symbolPainter.drawText(pixmap->rect(), Qt::AlignCenter | Qt::TextSingleLine, symbolString);
    }
    // Else, it still doesn't fit, must be a long string, too bad, leave
    // the  pixmap untouched so nothing will be shown when it is used

    if (!mSymbolPixmapCache.insert(key, pixmap)) {
        qDebug("T2DMap::addSymbolToPixmapCache() ALERT: Map Room Symbol Pixmap cache is full...!");
    }
}

// Revised to use a QCache to hold QPixmap * to generated images for room symbols
void T2DMap::paintEvent(QPaintEvent* e)
{
    if (!mpMap) {
        return;
    }
    bool __Pick = mPick;
    QElapsedTimer renderTimer;
    renderTimer.start();

    QPainter painter(this);
    if (!painter.isActive()) {
        return;
    }

    mAreaExitsList.clear();

    float widgetWidth = width();
    float widgetHeight = height();

    if (widgetWidth < 10 || widgetHeight < 10) {
        return;
    }

    if (widgetWidth > widgetHeight) {
        xspan = xyzoom * (widgetWidth / widgetHeight);
        yspan = xyzoom;
    } else {
        xspan = xyzoom;
        yspan = xyzoom * (widgetHeight / widgetWidth);
    }

    mRoomWidth = widgetWidth / xspan;
    mRoomHeight = widgetHeight / yspan;

    static bool oldBubbleMode = false;
    if (oldBubbleMode != mBubbleMode) {
        // If the round/square room selection has changed this will invalidate
        // all the previously generated pixmaps:
        flushSymbolPixmapCache();
        oldBubbleMode = mBubbleMode;
    }

    mSymbolFontSize = 1;
    mMapSymbolFont = mpMap->mMapSymbolFont;
    mMapSymbolFont.setBold(false);
    mMapSymbolFont.setItalic(false);
    mMapSymbolFont.setUnderline(false);
    mMapSymbolFont.setOverline(false);
    mMapSymbolFont.setStrikeOut(false);

    QList<int> exitList;
    QList<int> oneWayExits;
    TRoom* pPlayerRoom = mpMap->mpRoomDB->getRoom(mpMap->mRoomIdHash.value(mpHost->getName()));
    if (!pPlayerRoom) {
        painter.save();
        painter.fillRect(0, 0, width(), height(), Qt::transparent);
        auto font(painter.font());
        font.setPointSize(10);
        painter.setFont(font);
        painter.drawText(0, 0, widgetWidth, widgetHeight, Qt::AlignCenter | Qt::TextWordWrap, tr("No map or no valid position."));
        painter.restore();
        return;
    }

    int ox;
    int oy;
    if (mRoomID != mpMap->mRoomIdHash.value(mpHost->getName()) && mShiftMode) {
        mShiftMode = false;
    }
    TArea* playerArea;
    TRoom* playerRoom;
    int playerAreaID = pPlayerRoom->getArea();
    if ((!__Pick && !mShiftMode) || mpMap->mNewMove) {
        mShiftMode = true;
        // das ist nur hier von Interesse, weil es nur hier einen map editor
        // gibt -> map wird unter Umstaenden nicht geupdated, deshalb force ich
        // mit mNewRoom ein map update bei centerview()
        mpMap->mNewMove = false;

        if (!mpMap->mpRoomDB->getArea(playerAreaID)) {
            return;
        }
        mRoomID = mpMap->mRoomIdHash.value(mpHost->getName());
        playerRoom = mpMap->mpRoomDB->getRoom(mRoomID);
        if (!playerRoom) {
            return;
        }

        mAreaID = playerRoom->getArea();
        playerArea = mpMap->mpRoomDB->getArea(mAreaID);
        if (!playerArea) {
            return;
        }
        ox = playerRoom->x;
        oy = playerRoom->y * -1;
        mOx = ox;
        mOy = oy;
        mOz = playerRoom->z;
    } else {
        playerRoom = mpMap->mpRoomDB->getRoom(mRoomID);
        playerArea = mpMap->mpRoomDB->getArea(mAreaID);
        if (!playerRoom || !playerArea) {
            return;
        }
        ox = mOx;
        oy = mOy;
    }

    mRX = qRound(mRoomWidth * ((xspan / 2.0) - ox));
    mRY = qRound(mRoomHeight * ((yspan / 2.0) - oy));

    // This could be the coordinates of the center of the window?
    int px = qRound((mRoomWidth * xspan) / 2.0);
    int py = qRound((mRoomHeight * yspan) / 2.0);

    TArea* pArea = playerArea;

    int zLevel = mOz;

    float exitWidth = 1 / eSize * mRoomWidth * rSize;

    painter.fillRect(0, 0, width(), height(), mpHost->mBgColor_2);

    auto pen = painter.pen();
    pen.setColor(mpHost->mFgColor_2);
    pen.setWidthF(exitWidth);
    if (mMapperUseAntiAlias) {
        painter.setRenderHint(QPainter::Antialiasing);
    } else {
        painter.setRenderHint(QPainter::NonCosmeticDefaultPen);
    }
    painter.setPen(pen);

    if (mpMap->mapLabels.contains(mAreaID)) {
        QMapIterator<int, TMapLabel> it(mpMap->mapLabels[mAreaID]);
        while (it.hasNext()) {
            it.next();
            auto mapLabel = it.value();
            if (mapLabel.pos.z() != mOz) {
                continue;
            }
            if (mapLabel.text.length() < 1) {
                mpMap->mapLabels[mAreaID][it.key()].text = tr("no text", "Default text if a label is created in mapper with no text");
            }
            QPointF labelPosition;
            int labelX = mapLabel.pos.x() * mRoomWidth + mRX;
            int labelY = mapLabel.pos.y() * mRoomHeight * -1 + mRY;

            labelPosition.setX(labelX);
            labelPosition.setY(labelY);
            int labelWidth = abs(qRound(mapLabel.size.width() * mRoomWidth));
            int labelHeight = abs(qRound(mapLabel.size.height() * mRoomHeight));
            if (!((0 < labelX || 0 < labelX + labelWidth) && (widgetWidth > labelX || widgetWidth > labelX + labelWidth))) {
                continue;
            }
            if (!((0 < labelY || 0 < labelY + labelHeight) && (widgetHeight > labelY || widgetHeight > labelY + labelHeight))) {
                continue;
            }
            QRectF labelPaintRectangle = QRect(mapLabel.pos.x() * mRoomWidth + mRX, mapLabel.pos.y() * mRoomHeight * -1 + mRY, labelWidth, labelHeight);
            if (!mapLabel.showOnTop) {
                if (!mapLabel.noScaling) {
                    painter.drawPixmap(labelPosition, mapLabel.pix.scaled(labelPaintRectangle.size().toSize()));
                    mpMap->mapLabels[mAreaID][it.key()].clickSize.setWidth(labelPaintRectangle.width());
                    mpMap->mapLabels[mAreaID][it.key()].clickSize.setHeight(labelPaintRectangle.height());
                } else {
                    painter.drawPixmap(labelPosition, mapLabel.pix);
                    mpMap->mapLabels[mAreaID][it.key()].clickSize.setWidth(mapLabel.pix.width());
                    mpMap->mapLabels[mAreaID][it.key()].clickSize.setHeight(mapLabel.pix.height());
                }
            }

            if (mapLabel.highlight) {
                labelPaintRectangle.setSize(mapLabel.clickSize);
                painter.fillRect(labelPaintRectangle, QColor(255, 155, 55, 190));
            }
        }
    }

    if (!pArea->gridMode) {
        int customLineDestinationTarget = 0;
        if (mCustomLinesRoomTo > 0) {
            customLineDestinationTarget = mCustomLinesRoomTo;
        } else if (mCustomLineSelectedRoom > 0 && !mCustomLineSelectedExit.isEmpty()) {
            TRoom* pSR = mpMap->mpRoomDB->getRoom(mCustomLineSelectedRoom);
            if (pSR) {
                if (mCustomLineSelectedExit == QLatin1String("nw")) {
                    customLineDestinationTarget = pSR->getNorthwest();
                } else if (mCustomLineSelectedExit == QLatin1String("n")) {
                    customLineDestinationTarget = pSR->getNorth();
                } else if (mCustomLineSelectedExit == QLatin1String("ne")) {
                    customLineDestinationTarget = pSR->getNortheast();
                } else if (mCustomLineSelectedExit == QLatin1String("up")) {
                    customLineDestinationTarget = pSR->getUp();
                } else if (mCustomLineSelectedExit == QLatin1String("w")) {
                    customLineDestinationTarget = pSR->getWest();
                } else if (mCustomLineSelectedExit == QLatin1String("e")) {
                    customLineDestinationTarget = pSR->getEast();
                } else if (mCustomLineSelectedExit == QLatin1String("down")) {
                    customLineDestinationTarget = pSR->getDown();
                } else if (mCustomLineSelectedExit == QLatin1String("sw")) {
                    customLineDestinationTarget = pSR->getSouthwest();
                } else if (mCustomLineSelectedExit == QLatin1String("s")) {
                    customLineDestinationTarget = pSR->getSouth();
                } else if (mCustomLineSelectedExit == QLatin1String("se")) {
                    customLineDestinationTarget = pSR->getSoutheast();
                } else if (mCustomLineSelectedExit == QLatin1String("in")) {
                    customLineDestinationTarget = pSR->getIn();
                } else if (mCustomLineSelectedExit == QLatin1String("out")) {
                    customLineDestinationTarget = pSR->getOut();
                } else {
                    QMapIterator<int, QString> otherExitIt = pSR->getOtherMap();
                    while (otherExitIt.hasNext()) {
                        otherExitIt.next();
                        if (otherExitIt.value().startsWith("0") || otherExitIt.value().startsWith("1")) {
                            if (otherExitIt.value().mid(1) == mCustomLineSelectedExit) {
                                customLineDestinationTarget = otherExitIt.key();
                                break;
                            }
                        } else if (otherExitIt.value() == mCustomLineSelectedExit) {
                            customLineDestinationTarget = otherExitIt.key();
                            break;
                        }
                    }
                }
            }
        }
        QSetIterator<int> itRoom2(pArea->getAreaRooms());
        while (itRoom2.hasNext()) {
            int _id = itRoom2.next();
            TRoom* room = mpMap->mpRoomDB->getRoom(_id);
            if (!room) {
                continue;
            }
            float rx = room->x * mRoomWidth + mRX;
            float ry = room->y * -1 * mRoomHeight + mRY;
            int rz = room->z;

            if (rz != zLevel) {
                continue;
            }

            if (room->customLines.empty()) {
                if (rx < 0 || ry < 0 || rx > widgetWidth || ry > widgetHeight) {
                    continue;
                }
            } else {
                float miny = room->min_y * -1 * mRoomHeight + static_cast<float>(mRY);
                float maxy = room->max_y * -1 * mRoomHeight + static_cast<float>(mRY);
                float minx = room->min_x * mRoomWidth + static_cast<float>(mRX);
                float maxx = room->max_x * mRoomWidth + static_cast<float>(mRX);

                if (!((minx > 0.0 || maxx > 0.0) && (static_cast<float>(widgetWidth) > minx || static_cast<float>(widgetWidth) > maxx))) {
                    continue;
                }

                if (!((miny > 0.0 || maxy > 0.0) && (static_cast<float>(widgetHeight) > miny || static_cast<float>(widgetHeight) > maxy))) {
                    continue;
                }
            }

            room->rendered = true;

            // exitList is a list of the destination rooms reached by exit lines
            // that are NOT custom exit lines from this room so are places to
            // which a straight line is to be drawn from the centre of this room
            // to (now half way for a two-way exit) the center of the exit room,
            // this does mean that multiple exits to the same room are drawn
            // on top of each other and that there is no indication from which
            // exit direction they are for...!
            exitList.clear();
            // oneWayExits contain the sub-set of exitList where the opposite
            // exit from the exit room does NOT return to the current room:
            oneWayExits.clear();
            if (!room->customLines.empty()) {
                // This room has custom exit lines:
                if (!room->customLines.contains(QStringLiteral("n"))) {
                    exitList.push_back(room->getNorth());
                    TRoom* pER = mpMap->mpRoomDB->getRoom(room->getNorth());
                    if (pER) {
                        if (pER->getSouth() != _id) {
                            oneWayExits.push_back(room->getNorth());
                        }
                    }
                }
                if (!room->customLines.contains(QStringLiteral("ne"))) {
                    exitList.push_back(room->getNortheast());
                    TRoom* pER = mpMap->mpRoomDB->getRoom(room->getNortheast());
                    if (pER) {
                        if (pER->getSouthwest() != _id) {
                            oneWayExits.push_back(room->getNortheast());
                        }
                    }
                }
                if (!room->customLines.contains(QStringLiteral("e"))) {
                    exitList.push_back(room->getEast());
                    TRoom* pER = mpMap->mpRoomDB->getRoom(room->getEast());
                    if (pER) {
                        if (pER->getWest() != _id) {
                            oneWayExits.push_back(room->getEast());
                        }
                    }
                }
                if (!room->customLines.contains(QStringLiteral("se"))) {
                    exitList.push_back(room->getSoutheast());
                    TRoom* pER = mpMap->mpRoomDB->getRoom(room->getSoutheast());
                    if (pER) {
                        if (pER->getNorthwest() != _id) {
                            oneWayExits.push_back(room->getSoutheast());
                        }
                    }
                }
                if (!room->customLines.contains(QStringLiteral("s"))) {
                    exitList.push_back(room->getSouth());
                    TRoom* pER = mpMap->mpRoomDB->getRoom(room->getSouth());
                    if (pER) {
                        if (pER->getNorth() != _id) {
                            oneWayExits.push_back(room->getSouth());
                        }
                    }
                }
                if (!room->customLines.contains(QStringLiteral("sw"))) {
                    exitList.push_back(room->getSouthwest());
                    TRoom* pER = mpMap->mpRoomDB->getRoom(room->getSouthwest());
                    if (pER) {
                        if (pER->getNortheast() != _id) {
                            oneWayExits.push_back(room->getSouthwest());
                        }
                    }
                }
                if (!room->customLines.contains(QStringLiteral("w"))) {
                    exitList.push_back(room->getWest());
                    TRoom* pER = mpMap->mpRoomDB->getRoom(room->getWest());
                    if (pER) {
                        if (pER->getEast() != _id) {
                            oneWayExits.push_back(room->getWest());
                        }
                    }
                }
                if (!room->customLines.contains(QStringLiteral("nw"))) {
                    exitList.push_back(room->getNorthwest());
                    TRoom* pER = mpMap->mpRoomDB->getRoom(room->getNorthwest());
                    if (pER) {
                        if (pER->getSoutheast() != _id) {
                            oneWayExits.push_back(room->getNorthwest());
                        }
                    }
                }
            } else {
                int exitRoomId = room->getNorth();
                if (exitRoomId > 0) {
                    exitList.push_back(exitRoomId);
                    TRoom* pER = mpMap->mpRoomDB->getRoom(exitRoomId);
                    if (pER) {
                        if (pER->getSouth() != _id) {
                            oneWayExits.push_back(exitRoomId);
                        }
                    }
                }
                exitRoomId = room->getNortheast();
                if (exitRoomId > 0) {
                    exitList.push_back(exitRoomId);
                    TRoom* pER = mpMap->mpRoomDB->getRoom(exitRoomId);
                    if (pER) {
                        if (pER->getSouthwest() != _id) {
                            oneWayExits.push_back(exitRoomId);
                        }
                    }
                }
                exitRoomId = room->getEast();
                if (exitRoomId > 0) {
                    exitList.push_back(exitRoomId);
                    TRoom* pER = mpMap->mpRoomDB->getRoom(exitRoomId);
                    if (pER) {
                        if (pER->getWest() != _id) {
                            oneWayExits.push_back(exitRoomId);
                        }
                    }
                }
                exitRoomId = room->getSoutheast();
                if (exitRoomId > 0) {
                    exitList.push_back(exitRoomId);
                    TRoom* pER = mpMap->mpRoomDB->getRoom(exitRoomId);
                    if (pER) {
                        if (pER->getNorthwest() != _id) {
                            oneWayExits.push_back(exitRoomId);
                        }
                    }
                }
                exitRoomId = room->getSouth();
                if (exitRoomId > 0) {
                    exitList.push_back(exitRoomId);
                    TRoom* pER = mpMap->mpRoomDB->getRoom(exitRoomId);
                    if (pER) {
                        if (pER->getNorth() != _id) {
                            oneWayExits.push_back(exitRoomId);
                        }
                    }
                }
                exitRoomId = room->getSouthwest();
                if (exitRoomId > 0) {
                    exitList.push_back(exitRoomId);
                    TRoom* pER = mpMap->mpRoomDB->getRoom(exitRoomId);
                    if (pER) {
                        if (pER->getNortheast() != _id) {
                            oneWayExits.push_back(exitRoomId);
                        }
                    }
                }
                exitRoomId = room->getWest();
                if (exitRoomId > 0) {
                    exitList.push_back(exitRoomId);
                    TRoom* pER = mpMap->mpRoomDB->getRoom(exitRoomId);
                    if (pER) {
                        if (pER->getEast() != _id) {
                            oneWayExits.push_back(exitRoomId);
                        }
                    }
                }
                exitRoomId = room->getNorthwest();
                if (exitRoomId > 0) {
                    exitList.push_back(exitRoomId);
                    TRoom* pER = mpMap->mpRoomDB->getRoom(exitRoomId);
                    if (pER) {
                        if (pER->getSoutheast() != _id) {
                            oneWayExits.push_back(exitRoomId);
                        }
                    }
                }
            }

            if (!room->customLines.empty()) {
                QPen oldPen = painter.pen();
                QMapIterator<QString, QList<QPointF>> itk(room->customLines);
                while (itk.hasNext()) {
                    itk.next();
                    QColor customLineColor;
                    if (_id == mCustomLineSelectedRoom && itk.key() == mCustomLineSelectedExit) {
                        customLineColor = QColor(255, 155, 55);
                    } else {
                        customLineColor = room->customLinesColor.value(itk.key(), Qt::red);
                    }

                    float ex = room->x * mRoomWidth + mRX;
                    float ey = room->y * mRoomHeight * -1 + mRY;
                    QPointF origin = QPointF(ex, ey);
                    // The following sets a point offset from the room center
                    // that depends on the exit direction that the custom line
                    // heads to from the room center - it forms a fixed segment
                    // that cannot be moved:
                    QPointF fixedOffsetPoint;
                    if (itk.key() == QLatin1String("n")) {
                        fixedOffsetPoint = QPointF(ex, ey - mRoomHeight / 2.0);
                    } else if (itk.key() == QLatin1String("ne")) {
                        fixedOffsetPoint = QPointF(ex + mRoomWidth / 2.0, ey - mRoomHeight / 2.0);
                    } else if (itk.key() == QLatin1String("e")) {
                        fixedOffsetPoint = QPointF(ex + mRoomWidth / 2.0, ey);
                    } else if (itk.key() == QLatin1String("se")) {
                        fixedOffsetPoint = QPointF(ex + mRoomWidth / 2.0, ey + mRoomHeight / 2.0);
                    } else if (itk.key() == QLatin1String("s")) {
                        fixedOffsetPoint = QPointF(ex, ey + mRoomHeight / 2.0);
                    } else if (itk.key() == QLatin1String("sw")) {
                        fixedOffsetPoint = QPointF(ex - mRoomWidth / 2.0, ey + mRoomHeight / 2.0);
                    } else if (itk.key() == QLatin1String("w")) {
                        fixedOffsetPoint = QPointF(ex - mRoomWidth / 2.0, ey);
                    } else if (itk.key() == QLatin1String("nw")) {
                        fixedOffsetPoint = QPointF(ex - mRoomWidth / 2.0, ey - mRoomHeight / 2.0);
                    } else {
                        fixedOffsetPoint = QPointF(ex, ey);
                    }
                    QPen customLinePen = painter.pen();
                    customLinePen.setCosmetic(mMapperUseAntiAlias);
                    customLinePen.setWidthF(exitWidth);
                    customLinePen.setColor(customLineColor);
                    customLinePen.setCapStyle(Qt::RoundCap);
                    customLinePen.setJoinStyle(Qt::RoundJoin);
                    customLinePen.setStyle(room->customLinesStyle.value(itk.key()));

                    QVector<QPointF> polyLinePoints;
                    QList<QPointF> customLinePoints = itk.value();
                    if (!customLinePoints.empty()) {
                        painter.setPen(customLinePen);
                        polyLinePoints << origin;
                        polyLinePoints << fixedOffsetPoint;
                        for (int pk = 0, total = customLinePoints.size(); pk < total; ++pk) {
                            polyLinePoints << QPointF(customLinePoints.at(pk).x() * mRoomWidth + mRX, customLinePoints.at(pk).y() * mRoomHeight * -1 + mRY);
                        }
                        painter.drawPolyline(polyLinePoints.data(), polyLinePoints.size());

                        if (room->customLinesArrow.value(itk.key())) {
                            QLineF l0 = QLineF(polyLinePoints.last(), polyLinePoints.at(polyLinePoints.size() - 2));
                            l0.setLength(exitWidth * 5.0);
                            QPointF _p1 = l0.p1();
                            QPointF _p2 = l0.p2();
                            QLineF l1 = QLineF(l0);
                            qreal w1 = l1.angle() - 90.0;
                            QLineF l2;
                            l2.setP1(_p2);
                            l2.setAngle(w1);
                            l2.setLength(exitWidth * 2.0);
                            QPointF _p3 = l2.p2();
                            l2.setAngle(l2.angle() + 180.0);
                            QPointF _p4 = l2.p2();
                            QPolygonF _poly;
                            _poly.append(_p1);
                            _poly.append(_p3);
                            _poly.append(_p4);
                            QBrush brush = painter.brush();
                            brush.setColor(customLineColor);
                            brush.setStyle(Qt::SolidPattern);
                            QPen arrowPen = painter.pen();
                            arrowPen.setCosmetic(mMapperUseAntiAlias);
                            arrowPen.setStyle(Qt::SolidLine);
                            painter.setPen(arrowPen);
                            painter.setBrush(brush);
                            painter.drawPolygon(_poly);
                        }

                        if (_id == mCustomLineSelectedRoom && itk.key() == mCustomLineSelectedExit) {
                            QPen _savedPen = painter.pen();
                            QPen _pen;
                            QBrush _brush = painter.brush();
                            painter.setBrush(Qt::NoBrush);
                            // The first two points in the polyLinePoints are
                            // fixed for all exit directions and do not get
                            // circular "handles":
                            for (int pk = 2, total = polyLinePoints.size(); pk < total; ++pk) {
                                if (pk == (mCustomLineSelectedPoint + 2)) {
                                    // Draw the selected point in yellow not orange.
                                    _pen = QPen(QColor(255, 255, 55), _savedPen.width(), Qt::SolidLine, Qt::FlatCap, _savedPen.joinStyle());
                                } else {
                                    _pen = QPen(_savedPen.color(), _savedPen.width(), Qt::SolidLine, Qt::FlatCap, _savedPen.joinStyle());
                                }
                                // Draw hollow circles not default filled ones!
                                painter.setPen(_pen);
                                painter.drawEllipse(polyLinePoints.at(pk), mRoomWidth / 4.0, mRoomWidth / 4.0);
                            }
                            painter.setPen(_savedPen);
                            painter.setBrush(_brush);
                        }
                    }
                }
                painter.setPen(oldPen);
            }

            // draw exit stubs
            QMap<int, QVector3D> unitVectors = mpMap->unitVectors;
            for (int direction : room->exitStubs) {
                QVector3D uDirection = unitVectors[direction];
                painter.drawLine(rx + rSize * uDirection.x() / 2.0,
                                 ry + rSize * uDirection.y(),
                                 rx + uDirection.x() * (rSize * mRoomWidth * 3.0 / 4.0),
                                 ry + uDirection.y() * (rSize * mRoomHeight * 3.0 / 4.0));
            }

            QPen __pen;
            for (int& k : exitList) {
                int rID = k;
                if (rID <= 0) {
                    continue;
                }

                bool areaExit;

                TRoom* pE = mpMap->mpRoomDB->getRoom(rID);
                if (!pE) {
                    continue;
                }

                if (pE->getArea() != mAreaID) {
                    areaExit = true;
                } else {
                    areaExit = false;
                }
                float ex = pE->x * mRoomWidth + mRX;
                float ey = pE->y * mRoomHeight * -1 + mRY;
                int ez = pE->z;

                QVector3D p1(ex, ey, ez);
                QVector3D p2(rx, ry, rz);
                QLine _line;
                if (!areaExit) {
                    // one way exit or 2 way exit?
                    if (!oneWayExits.contains(rID)) {
                        painter.drawLine(p1.toPointF(), p2.toPointF());
                    } else {
                        // one way exit draw arrow

                        QLineF l0 = QLineF(p2.toPointF(), p1.toPointF());
                        QLineF k0 = l0;
                        k0.setLength((l0.length() - exitWidth * 5.0) / 2.0);
                        qreal dx = k0.dx();
                        qreal dy = k0.dy();
                        QPen _tp = painter.pen();
                        QPen _tp2 = _tp;
                        _tp2.setStyle(Qt::DotLine);
                        painter.setPen(_tp2);
                        painter.drawLine(l0);
                        painter.setPen(_tp);
                        l0.setLength(exitWidth * 5.0);
                        QPointF _p1 = l0.p2();
                        QPointF _p2 = l0.p1();
                        QLineF l1 = QLineF(l0);
                        qreal w1 = l1.angle() - 90.0;
                        QLineF l2;
                        l2.setP1(_p2);
                        l2.setAngle(w1);
                        l2.setLength(exitWidth * 2.0);
                        QPointF _p3 = l2.p2();
                        l2.setAngle(l2.angle() + 180.0);
                        QPointF _p4 = l2.p2();
                        QPolygonF _poly;
                        _poly.append(_p1);
                        _poly.append(_p3);
                        _poly.append(_p4);

                        QBrush brush = painter.brush();
                        brush.setColor(QColor(255, 100, 100));
                        brush.setStyle(Qt::SolidPattern);
                        QPen arrowPen = painter.pen();
                        arrowPen.setCosmetic(mMapperUseAntiAlias);
                        arrowPen.setStyle(Qt::SolidLine);
                        painter.setPen(arrowPen);
                        painter.setBrush(brush);
                        painter.drawPolygon(_poly.translated(dx, dy));
                    }

                } else {
                    __pen = painter.pen();
                    QPoint _p;
                    pen = painter.pen();
                    pen.setWidthF(exitWidth);
                    pen.setCosmetic(mMapperUseAntiAlias);
                    pen.setColor(getColor(k));
                    painter.setPen(pen);
                    if (room->getSouth() == rID) {
                        _line = QLine(p2.x(), p2.y() + mRoomHeight, p2.x(), p2.y());
                        _p = QPoint(p2.x(), p2.y() + mRoomHeight / 2.0);
                    } else if (room->getNorth() == rID) {
                        _line = QLine(p2.x(), p2.y() - mRoomHeight, p2.x(), p2.y());
                        _p = QPoint(p2.x(), p2.y() - mRoomHeight / 2.0);
                    } else if (room->getWest() == rID) {
                        _line = QLine(p2.x() - mRoomWidth, p2.y(), p2.x(), p2.y());
                        _p = QPoint(p2.x() - mRoomWidth / 2.0, p2.y());
                    } else if (room->getEast() == rID) {
                        _line = QLine(p2.x() + mRoomWidth, p2.y(), p2.x(), p2.y());
                        _p = QPoint(p2.x() + mRoomWidth / 2.0, p2.y());
                    } else if (room->getNorthwest() == rID) {
                        _line = QLine(p2.x() - mRoomWidth, p2.y() - mRoomHeight, p2.x(), p2.y());
                        _p = QPoint(p2.x() - mRoomWidth / 2.0, p2.y() - mRoomHeight / 2.0);
                    } else if (room->getNortheast() == rID) {
                        _line = QLine(p2.x() + mRoomWidth, p2.y() - mRoomHeight, p2.x(), p2.y());
                        _p = QPoint(p2.x() + mRoomWidth / 2.0, p2.y() - mRoomHeight / 2.0);
                    } else if (room->getSoutheast() == rID) {
                        _line = QLine(p2.x() + mRoomWidth, p2.y() + mRoomHeight, p2.x(), p2.y());
                        _p = QPoint(p2.x() + mRoomWidth / 2.0, p2.y() + mRoomHeight / 2.0);
                    } else if (room->getSouthwest() == rID) {
                        _line = QLine(p2.x() - mRoomWidth, p2.y() + mRoomHeight, p2.x(), p2.y());
                        _p = QPoint(p2.x() - mRoomWidth / 2.0, p2.y() + mRoomHeight / 2.0);
                    }
                    painter.drawLine(_line);
                    mAreaExitsList[k] = _p;
                    QLineF l0 = QLineF(_line);
                    l0.setLength(exitWidth * 5.0);
                    QPointF _p1 = l0.p1();
                    QPointF _p2 = l0.p2();
                    QLineF l1 = QLineF(l0);
                    qreal w1 = l1.angle() - 90.0;
                    QLineF l2;
                    l2.setP1(_p2);
                    l2.setAngle(w1);
                    l2.setLength(exitWidth * 2.0);
                    QPointF _p3 = l2.p2();
                    l2.setAngle(l2.angle() + 180.0);
                    QPointF _p4 = l2.p2();
                    QPolygonF _poly;
                    _poly.append(_p1);
                    _poly.append(_p3);
                    _poly.append(_p4);
                    QBrush brush = painter.brush();
                    brush.setColor(getColor(k));
                    brush.setStyle(Qt::SolidPattern);
                    QPen arrowPen = painter.pen();
                    arrowPen.setCosmetic(mMapperUseAntiAlias);
                    painter.setPen(arrowPen);
                    painter.setBrush(brush);
                    painter.drawPolygon(_poly);
                    painter.setPen(__pen);
                }
                // doors
                if (!room->doors.empty()) {
                    int doorStatus = 0;
                    if (room->getSouth() == rID && room->doors.contains("s")) {
                        doorStatus = room->doors["s"];
                    } else if (room->getNorth() == rID && room->doors.contains("n")) {
                        doorStatus = room->doors["n"];
                    } else if (room->getSouthwest() == rID && room->doors.contains("sw")) {
                        doorStatus = room->doors["sw"];
                    } else if (room->getSoutheast() == rID && room->doors.contains("se")) {
                        doorStatus = room->doors["se"];
                    } else if (room->getNortheast() == rID && room->doors.contains("ne")) {
                        doorStatus = room->doors["ne"];
                    } else if (room->getNorthwest() == rID && room->doors.contains("nw")) {
                        doorStatus = room->doors["nw"];
                    } else if (room->getWest() == rID && room->doors.contains("w")) {
                        doorStatus = room->doors["w"];
                    } else if (room->getEast() == rID && room->doors.contains("e")) {
                        doorStatus = room->doors["e"];
                    }
                    if (doorStatus > 0) {
                        QLineF k0;
                        QRectF rect;
                        rect.setWidth(0.25 * mRoomWidth);
                        rect.setHeight(0.25 * mRoomHeight);
                        if (areaExit) {
                            k0 = QLineF(_line);
                        } else {
                            k0 = QLineF(p2.toPointF(), p1.toPointF());
                        }
                        k0.setLength((k0.length()) / 2.0);
                        rect.moveCenter(k0.p2());
                        QPen arrowPen = painter.pen();
                        QPen _tp = painter.pen();
                        arrowPen.setCosmetic(mMapperUseAntiAlias);
                        arrowPen.setStyle(Qt::SolidLine);
                        if (doorStatus == 1) { //open door
                            arrowPen.setColor(QColor(10, 155, 10));
                        } else if (doorStatus == 2) { //closed door
                            arrowPen.setColor(QColor(155, 155, 10));
                        } else { //locked door
                            arrowPen.setColor(QColor(155, 10, 10));
                        }
                        QBrush brush;
                        QBrush oldBrush;
                        painter.setPen(arrowPen);
                        painter.setBrush(brush);
                        painter.drawRect(rect);
                        painter.setBrush(oldBrush);
                        painter.setPen(_tp);
                    }
                }
            } // End of for( exitList )

            // Indicate destination for custom exit line drawing - double size
            // target yellow hollow circle
            // Similar code is now also used to indicate center target of
            // multiple selected rooms but that must be done after all the rooms
            // have been drawn otherwise later drawn rooms will overwrite the
            // mark, especially on areas in gridmode.
            if (customLineDestinationTarget > 0 && customLineDestinationTarget == _id) {
                QPen savePen = painter.pen();
                QBrush saveBrush = painter.brush();
                float roomRadius = mRoomWidth * 1.2;
                float roomDiagonal = mRoomWidth * 1.2;
                QPointF roomCenter = QPointF(rx, ry);

                QPen yellowPen(QColor(255, 255, 50, 192)); // Quarter opaque yellow pen
                yellowPen.setWidth(mRoomWidth * 0.1);
                QPainterPath myPath;
                painter.setPen(yellowPen);
                painter.setBrush(Qt::NoBrush);
                myPath.addEllipse(roomCenter, roomRadius, roomRadius);
                myPath.addEllipse(roomCenter, roomRadius / 2.0, roomRadius / 2.0);
                myPath.moveTo(rx - roomDiagonal, ry - roomDiagonal);
                myPath.lineTo(rx + roomDiagonal, ry + roomDiagonal);
                myPath.moveTo(rx + roomDiagonal, ry - roomDiagonal);
                myPath.lineTo(rx - roomDiagonal, ry + roomDiagonal);
                painter.drawPath(myPath);
                painter.setPen(savePen);
                painter.setBrush(saveBrush);
            }
        } // End of for( area Rooms )
    }     // End of NOT area gridmode

    // Draw label sizing or group selection box
    if (mSizeLabel) {
        painter.fillRect(mMultiRect, QColor(250, 190, 0, 190));
    } else {
        painter.fillRect(mMultiRect, QColor(190, 190, 190, 60));
    }

    QSetIterator<int> itRoom(pArea->getAreaRooms());
    while (itRoom.hasNext()) {
        int currentAreaRoom = itRoom.next();
        TRoom* room = mpMap->mpRoomDB->getRoom(currentAreaRoom);
        if (!room) {
            continue; // Was missing this safety step to skip missing rooms
        }
        float rx = room->x * mRoomWidth + mRX;
        float ry = room->y * -1 * mRoomHeight + mRY;
        int rz = room->z;

        if (rz != zLevel) {
            continue;
        }
        if (rx < 0 || ry < 0 || rx > widgetWidth || ry > widgetHeight) {
            continue;
        }

        room->rendered = false;
        QRectF roomRectangle;
        if (pArea->gridMode) {
            roomRectangle = QRectF(rx - mRoomWidth / 2.0, ry - mRoomHeight / 2.0, mRoomWidth, mRoomHeight);
        } else {
            roomRectangle = QRectF(rx - (mRoomWidth * rSize) / 2.0, ry - (mRoomHeight * rSize) / 2.0, mRoomWidth * rSize, mRoomHeight * rSize);
        }

        QColor roomColor;
        int roomEnvironment = room->environment;
        if (mpMap->envColors.contains(roomEnvironment)) {
            roomEnvironment = mpMap->envColors[roomEnvironment];
        } else {
            if (!mpMap->customEnvColors.contains(roomEnvironment)) {
                roomEnvironment = 1;
            }
        }
        switch (roomEnvironment) {
        case 1:
            roomColor = mpHost->mRed_2;
            break;

        case 2:
            roomColor = mpHost->mGreen_2;
            break;
        case 3:
            roomColor = mpHost->mYellow_2;
            break;

        case 4:
            roomColor = mpHost->mBlue_2;
            break;

        case 5:
            roomColor = mpHost->mMagenta_2;
            break;
        case 6:
            roomColor = mpHost->mCyan_2;
            break;
        case 7:
            roomColor = mpHost->mWhite_2;
            break;
        case 8:
            roomColor = mpHost->mBlack_2;
            break;

        case 9:
            roomColor = mpHost->mLightRed_2;
            break;

        case 10:
            roomColor = mpHost->mLightGreen_2;
            break;
        case 11:
            roomColor = mpHost->mLightYellow_2;
            break;

        case 12:
            roomColor = mpHost->mLightBlue_2;
            break;

        case 13:
            roomColor = mpHost->mLightMagenta_2;
            break;
        case 14:
            roomColor = mpHost->mLightCyan_2;
            break;
        case 15:
            roomColor = mpHost->mLightWhite_2;
            break;
        case 16:
            roomColor = mpHost->mLightBlack_2;
            break;
        default: //user defined room color
            if (mpMap->customEnvColors.contains(roomEnvironment)) {
                roomColor = mpMap->customEnvColors[roomEnvironment];
            } else {
                if (16 < roomEnvironment && roomEnvironment < 232)
                {
                    quint8 base = roomEnvironment - 16;
                    quint8 r = base / 36;
                    quint8 g = (base - (r * 36)) / 6;
                    quint8 b = (base - (r * 36)) - (g * 6);

		    r = r * 51;
		    g = g * 51;
		    b = b * 51;
                    roomColor = QColor(r, g, b, 255);
                } else if (231 < roomEnvironment && roomEnvironment < 256) {
                    quint8 k = ((roomEnvironment - 232) * 10) + 8;
                    roomColor = QColor(k, k, k, 255);
                }
	    }
        }

        if (((mPick || __Pick) && mPHighlight.x() >= roomRectangle.x() - (mRoomWidth * rSize) && mPHighlight.x() <= roomRectangle.x() + (mRoomWidth * rSize)
             && mPHighlight.y() >= roomRectangle.y() - (mRoomHeight * rSize)
             && mPHighlight.y() <= roomRectangle.y() + (mRoomHeight * rSize))
            || mMultiSelectionSet.contains(currentAreaRoom)) {
            painter.fillRect(roomRectangle, QColor(255, 155, 55));
            mPick = false;
            if (mStartSpeedWalk) {
                mStartSpeedWalk = false;
                float roomRadius = 0.4 * mRoomWidth;
                QPointF roomCenter = QPointF(rx, ry);
                QRadialGradient gradient(roomCenter, roomRadius);
                gradient.setColorAt(0.95, QColor(255, 0, 0, 150));
                gradient.setColorAt(0.80, QColor(150, 100, 100, 150));
                gradient.setColorAt(0.799, QColor(150, 100, 100, 100));
                gradient.setColorAt(0.7, QColor(255, 0, 0, 200));
                gradient.setColorAt(0, Qt::white);
                QPen transparentPen(Qt::transparent);
                QPainterPath diameterPath;
                painter.setBrush(gradient);
                painter.setPen(transparentPen);
                diameterPath.addEllipse(roomCenter, roomRadius, roomRadius);
                painter.drawPath(diameterPath);


                mTarget = currentAreaRoom;
                if (mpMap->mpRoomDB->getRoom(mTarget)) {
                    mpMap->mTargetID = mTarget;
                    if (mpMap->findPath(mpMap->mRoomIdHash.value(mpHost->getName()), mpMap->mTargetID)) {
                        mpHost->startSpeedWalk();
                    } else {
                        QString msg = tr("Mapper: Cannot find a path to this room using known exits.\n");
                        mpHost->mpConsole->printSystemMessage(msg);
                    }
                }
            }
        } else {
            if (mBubbleMode) {
                float roomRadius = 0.5 * rSize * mRoomWidth;
                QPointF roomCenter = QPointF(rx, ry);
                QRadialGradient gradient(roomCenter, roomRadius);
                gradient.setColorAt(0.85, roomColor);
                gradient.setColorAt(0, Qt::white);
                QPen transparentPen(Qt::transparent);
                QPainterPath diameterPath;
                painter.setBrush(gradient);
                painter.setPen(transparentPen);
                diameterPath.addEllipse(roomCenter, roomRadius, roomRadius);
                painter.drawPath(diameterPath);
            } else {
                painter.fillRect(roomRectangle, roomColor);
            }

            if (!mShowRoomID && !room->mSymbol.isEmpty()) {
                QString pixmapKey;
                if (roomColor.lightness() > 127) {
                    pixmapKey = QStringLiteral("B_%1").arg(room->mSymbol);
                } else {
                    pixmapKey = QStringLiteral("W_%1").arg(room->mSymbol);
                }
                if (!mSymbolPixmapCache.contains(pixmapKey)) {
                    addSymbolToPixmapCache(pixmapKey, pArea->gridMode);
                }

                painter.save();
                painter.setBackgroundMode(Qt::TransparentMode);

                QPixmap* pix = mSymbolPixmapCache.object(pixmapKey);
                if (!pix) {
                    qWarning("T2DMap::paintEvent() Alert: mSymbolPixmapCache failure, too many items to cache all of them for: \"%s\"", room->mSymbol.toUtf8().constData());
                } else {
                    /* For the non-scaling QPainter::drawPixmap() used now we
                     * have to position the generated pixmap containing the
                     * particular symbol for this room to Y when it would
                     * position it at X - this should be faster than the previous
                     * scaling QPainter::drawPixmap() as that would scale the
                     * pixmap to fit the Room Rectangle!
                     *
                     *               |<------->| roomRectangle.width()
                     * roomRectangle.topLeft-->X---------+
                     *              |  Room   |
                     *              |  Y---+  |
                     *              |  |Pix|  |
                     *              |  +---+  |
                     *              |Rectangle|
                     *              +---------+
                     *                 |<->|<--symbolRect.width()
                     * x-offset---->|<>|<-- (roomRectangle.width() - symbolRect.width())/2.0
                     * similarly for the y-offset
                     */

                    painter.drawPixmap(
                            QPoint(qRound(roomRectangle.left() + ((roomRectangle.width() - pix->width()) / 2.0)), qRound(roomRectangle.top() + ((roomRectangle.height() - pix->height()) / 2.0))),
                            *pix);
                }

                painter.restore();
            }

            if (room->highlight) {
                float roomRadius = (room->highlightRadius * mRoomWidth) / 2.0;
                QPointF roomCenter = QPointF(rx, ry);
                QRadialGradient gradient(roomCenter, roomRadius);
                gradient.setColorAt(0.85, room->highlightColor);
                gradient.setColorAt(0, room->highlightColor2);
                QPen transparentPen(Qt::transparent);
                QPainterPath diameterPath;
                painter.setBrush(gradient);
                painter.setPen(transparentPen);
                diameterPath.addEllipse(roomCenter, roomRadius, roomRadius);
                painter.drawPath(diameterPath);
            }

            if (mShowRoomID) {
                QPen roomIdPen = painter.pen();
                QColor roomIdColor;
                if (roomColor.red() + roomColor.green() + roomColor.blue() > 200) {
                    roomIdColor = QColor(Qt::black);
                } else {
                    roomIdColor = QColor(Qt::white);
                }
                painter.setPen(QPen(roomIdColor));
                painter.drawText(roomRectangle, Qt::AlignHCenter | Qt::AlignVCenter, QString::number(currentAreaRoom));
                painter.setPen(roomIdPen);
            }

            if (mShiftMode && currentAreaRoom == mpMap->mRoomIdHash.value(mpHost->getName())) {
                float roomRadius = (1.2 * mRoomWidth) / 2;
                QPointF roomCenter = QPointF(rx, ry);
                QRadialGradient gradient(roomCenter, roomRadius);
                gradient.setColorAt(0.95, QColor(255, 0, 0, 150));
                gradient.setColorAt(0.80, QColor(150, 100, 100, 150));
                gradient.setColorAt(0.799, QColor(150, 100, 100, 100));
                gradient.setColorAt(0.7, QColor(255, 0, 0, 200));
                gradient.setColorAt(0, Qt::white);
                QPen transparentPen(Qt::transparent);
                QPainterPath myPath;
                painter.setBrush(gradient);
                painter.setPen(transparentPen);
                myPath.addEllipse(roomCenter, roomRadius, roomRadius);
                painter.drawPath(myPath);
            }
        }

        // Change these from const to static to tweak them whilst running in a debugger...!
        const float allInsideTipOffsetFactor = 1 / 20.0f;
        const float upDownXOrYFactor = 1 / 3.1f;
        const float inOuterXFactor = 1 / 4.5f;
        const float inUpDownYFactor = 1 / 7.0f;
        const float outOuterXFactor = 1 / 2.2f;
        const float outUpDownYFactor = 1 / 5.5f;
        const float outInterXFactor = 1 / 3.5f;
        const float outerRealDoorPenThicknessFactor = 0.050f;
        const float outerStubDoorPenThicknessFactor = 0.025f;
        const float innerRealDoorPenThicknessFactor = 0.025f;
        const float innerStubDoorPenThicknessFactor = 0.0125f;

        QColor lc;
        if (roomColor.lightness() > 127) {
            lc = QColor(Qt::black);
        } else {
            lc = QColor(Qt::white);
        }
        pen = painter.pen();
        pen.setColor(lc);
        pen.setCosmetic(mMapperUseAntiAlias);
        pen.setCapStyle(Qt::RoundCap);
        pen.setJoinStyle(Qt::RoundJoin);
        QPen innerPen = pen;
        painter.save();

        QBrush innerBrush = painter.brush();
        innerBrush.setStyle(Qt::NoBrush);
        if (room->getUp() > 0 || room->exitStubs.contains(DIR_UP)) {
            QPolygonF poly_up;
            poly_up.append(QPointF(rx, ry + (mRoomHeight * rSize * allInsideTipOffsetFactor)));
            poly_up.append(QPointF(rx - (mRoomWidth * rSize * upDownXOrYFactor), ry + (mRoomHeight * rSize * upDownXOrYFactor)));
            poly_up.append(QPointF(rx + (mRoomWidth * rSize * upDownXOrYFactor), ry + (mRoomHeight * rSize * upDownXOrYFactor)));
            bool isDoor = true;
            QBrush brush = painter.brush();
            switch (room->doors.value(QStringLiteral("up"))) {
            case 1: //open door
                brush.setColor(QColor(10, 155, 10));
                innerPen.setColor(QColor(10, 155, 10));
                break;
            case 2: //closed door
                brush.setColor(QColor(155, 155, 10));
                innerPen.setColor(QColor(155, 155, 10));
                break;
            case 3:
                brush.setColor(QColor(155, 10, 10));
                innerPen.setColor(QColor(155, 10, 10));
                break;
            default:
                brush.setColor(lc);
                isDoor = false;
            }
            if (room->getUp() > 0) {
                pen.setWidthF(mRoomWidth * rSize * outerRealDoorPenThicknessFactor);
                innerPen.setWidthF(mRoomWidth * rSize * innerRealDoorPenThicknessFactor);
                brush.setStyle(Qt::Dense4Pattern);
            } else {
                pen.setWidthF(mRoomWidth * rSize * outerStubDoorPenThicknessFactor);
                innerPen.setWidthF(mRoomWidth * rSize * innerStubDoorPenThicknessFactor);
                brush.setStyle(Qt::DiagCrossPattern);
            }
            painter.setPen(pen);
            painter.setBrush(brush);
            painter.drawPolygon(poly_up);
            if (isDoor) {
                // Draw a narrower triangle on top of the existing one if there
                // is a door - to help emphasis the coloured door if the brush
                // from the main one is not obvious given the main room colour.
                painter.setPen(innerPen);
                painter.setBrush(innerBrush);
                painter.drawPolygon(poly_up);
            }
        }

        if (room->getDown() > 0 || room->exitStubs.contains(DIR_DOWN)) {
            QPolygonF poly_down;
            poly_down.append(QPointF(rx, ry - (mRoomHeight * rSize * allInsideTipOffsetFactor)));
            poly_down.append(QPointF(rx - (mRoomWidth * rSize * upDownXOrYFactor), ry - (mRoomHeight * rSize * upDownXOrYFactor)));
            poly_down.append(QPointF(rx + (mRoomWidth * rSize * upDownXOrYFactor), ry - (mRoomHeight * rSize * upDownXOrYFactor)));
            bool isDoor = true;
            QBrush brush = painter.brush();
            switch (room->doors.value(QStringLiteral("down"))) {
            case 1: //open door
                brush.setColor(QColor(10, 155, 10));
                innerPen.setColor(QColor(10, 155, 10));
                break;
            case 2: //closed door
                brush.setColor(QColor(155, 155, 10));
                innerPen.setColor(QColor(155, 155, 10));
                break;
            case 3:
                brush.setColor(QColor(155, 10, 10));
                innerPen.setColor(QColor(155, 10, 10));
                break;
            default:
                brush.setColor(lc);
                isDoor = false;
            }
            if (room->getDown() > 0) {
                pen.setWidthF(mRoomWidth * rSize * outerRealDoorPenThicknessFactor);
                innerPen.setWidthF(mRoomWidth * rSize * innerRealDoorPenThicknessFactor);
                brush.setStyle(Qt::Dense4Pattern);
            } else {
                pen.setWidthF(mRoomWidth * rSize * outerStubDoorPenThicknessFactor);
                innerPen.setWidthF(mRoomWidth * rSize * innerStubDoorPenThicknessFactor);
                brush.setStyle(Qt::DiagCrossPattern);
            }
            painter.setPen(pen);
            painter.setBrush(brush);
            painter.drawPolygon(poly_down);
            if (isDoor) {
                painter.setPen(innerPen);
                painter.setBrush(innerBrush);
                painter.drawPolygon(poly_down);
            }
        }

        if (room->getIn() > 0 || room->exitStubs.contains(DIR_IN)) {
            QPolygonF poly_in_left;
            QPolygonF poly_in_right;
            poly_in_left.append(QPointF(rx - (mRoomWidth * rSize * allInsideTipOffsetFactor), ry));
            poly_in_left.append(QPointF(rx - (mRoomWidth * rSize * inOuterXFactor), ry + (mRoomHeight * rSize * inUpDownYFactor)));
            poly_in_left.append(QPointF(rx - (mRoomWidth * rSize * inOuterXFactor), ry - (mRoomHeight * rSize * inUpDownYFactor)));
            poly_in_right.append(QPointF(rx + (mRoomWidth * rSize * allInsideTipOffsetFactor), ry));
            poly_in_right.append(QPointF(rx + (mRoomWidth * rSize * inOuterXFactor), ry + (mRoomHeight * rSize * inUpDownYFactor)));
            poly_in_right.append(QPointF(rx + (mRoomWidth * rSize * inOuterXFactor), ry - (mRoomHeight * rSize * inUpDownYFactor)));
            bool isDoor = true;
            QBrush brush = painter.brush();
            switch (room->doors.value(QStringLiteral("in"))) {
            case 1: //open door
                brush.setColor(QColor(10, 155, 10));
                innerPen.setColor(QColor(10, 155, 10));
                break;
            case 2: //closed door
                brush.setColor(QColor(155, 155, 10));
                innerPen.setColor(QColor(155, 155, 10));
                break;
            case 3:
                brush.setColor(QColor(155, 10, 10));
                innerPen.setColor(QColor(155, 10, 10));
                break;
            default:
                brush.setColor(lc);
                isDoor = false;
            }
            if (room->getIn() > 0) {
                pen.setWidthF(mRoomWidth * rSize * outerRealDoorPenThicknessFactor);
                innerPen.setWidthF(mRoomWidth * rSize * innerRealDoorPenThicknessFactor);
                brush.setStyle(Qt::Dense4Pattern);
            } else {
                pen.setWidthF(mRoomWidth * rSize * outerStubDoorPenThicknessFactor);
                innerPen.setWidthF(mRoomWidth * rSize * innerStubDoorPenThicknessFactor);
                brush.setStyle(Qt::DiagCrossPattern);
            }
            painter.setBrush(brush);
            painter.setPen(pen);
            painter.drawPolygon(poly_in_left);
            painter.drawPolygon(poly_in_right);
            if (isDoor) {
                painter.setPen(innerPen);
                painter.setBrush(innerBrush);
                painter.drawPolygon(poly_in_left);
                painter.drawPolygon(poly_in_right);
            }
        }

        if (room->getOut() > 0 || room->exitStubs.contains(DIR_OUT)) {
            QPolygonF poly_out_left;
            QPolygonF poly_out_right;
            poly_out_left.append(QPointF(rx - (mRoomWidth * rSize * outOuterXFactor), ry));
            poly_out_left.append(QPointF(rx - (mRoomWidth * rSize * outInterXFactor), ry + (mRoomHeight * rSize * outUpDownYFactor)));
            poly_out_left.append(QPointF(rx - (mRoomWidth * rSize * outInterXFactor), ry - (mRoomHeight * rSize * outUpDownYFactor)));
            poly_out_right.append(QPointF(rx + (mRoomWidth * rSize * outOuterXFactor), ry));
            poly_out_right.append(QPointF(rx + (mRoomWidth * rSize * outInterXFactor), ry + (mRoomHeight * rSize * outUpDownYFactor)));
            poly_out_right.append(QPointF(rx + (mRoomWidth * rSize * outInterXFactor), ry - (mRoomHeight * rSize * outUpDownYFactor)));
            bool isDoor = true;
            QBrush brush = painter.brush();
            switch (room->doors.value(QStringLiteral("out"))) {
            case 1: //open door
                brush.setColor(QColor(10, 155, 10));
                innerPen.setColor(QColor(10, 155, 10));
                break;
            case 2: //closed door
                brush.setColor(QColor(155, 155, 10));
                innerPen.setColor(QColor(155, 155, 10));
                break;
            case 3:
                brush.setColor(QColor(155, 10, 10));
                innerPen.setColor(QColor(155, 10, 10));
                break;
            default:
                brush.setColor(lc);
                isDoor = false;
            }
            if (room->getOut() > 0) {
                pen.setWidthF(mRoomWidth * rSize * outerRealDoorPenThicknessFactor);
                innerPen.setWidthF(mRoomWidth * rSize * innerRealDoorPenThicknessFactor);
                brush.setStyle(Qt::Dense4Pattern);
            } else {
                pen.setWidthF(mRoomWidth * rSize * outerStubDoorPenThicknessFactor);
                innerPen.setWidthF(mRoomWidth * rSize * innerStubDoorPenThicknessFactor);
                brush.setStyle(Qt::DiagCrossPattern);
            }
            painter.setBrush(brush);
            painter.setPen(pen);
            painter.drawPolygon(poly_out_left);
            painter.drawPolygon(poly_out_right);
            if (isDoor) {
                painter.setPen(innerPen);
                painter.setBrush(innerBrush);
                painter.drawPolygon(poly_out_left);
                painter.drawPolygon(poly_out_right);
            }
        }

        painter.restore();
        if (pArea->gridMode) {
            QMapIterator<int, QPoint> it(mAreaExitsList);
            while (it.hasNext()) {
                it.next();
                QPoint P = it.value();
                int rx = P.x();
                int ry = P.y();

                QRectF dr = QRectF(rx - mRoomWidth / 2.0, ry - mRoomHeight / 2.0, mRoomWidth, mRoomHeight);
                if (((mPick || __Pick) && mPHighlight.x() >= (dr.x() - mRoomWidth / 2.0) && mPHighlight.x() <= (dr.x() + mRoomWidth / 2.0) && mPHighlight.y() >= (dr.y() - mRoomHeight / 2.0)
                     && mPHighlight.y() <= (dr.y() + mRoomHeight / 2.0))
                    || mMultiSelectionSet.contains(currentAreaRoom)) {
                    painter.fillRect(dr, QColor(50, 255, 50));
                    mPick = false;
                    mTarget = it.key();
                    if (mpMap->mpRoomDB->getRoom(mTarget)) {
                        mpMap->mTargetID = mTarget;
                        if (mpMap->findPath(mpMap->mRoomIdHash.value(mpHost->getName()), mpMap->mTargetID)) {
                            mpHost->startSpeedWalk();
                        } else {
                            QString msg = tr("Mapper: Cannot find a path to this room using known exits.\n");
                            mpHost->mpConsole->printSystemMessage(msg);
                        }
                    }
                }
            }
        } else {
            QMapIterator<int, QPoint> it(mAreaExitsList);
            while (it.hasNext()) {
                it.next();
                QPoint P = it.value();
                int rx = P.x();
                int ry = P.y();

                QRectF dr = QRectF(rx, ry, mRoomWidth * rSize, mRoomHeight * rSize);
                if (((mPick || __Pick) && mPHighlight.x() >= (dr.x() - mRoomWidth / 3.0) && mPHighlight.x() <= (dr.x() + mRoomWidth / 3.0) && mPHighlight.y() >= (dr.y() - mRoomHeight / 3.0)
                     && mPHighlight.y() <= (dr.y() + mRoomHeight / 3.0))
                    && mStartSpeedWalk) {
                    mStartSpeedWalk = false;
                    float roomRadius = (0.8 * mRoomWidth) / 2.0;
                    QPointF roomCenter = QPointF(rx, ry);
                    QRadialGradient gradient(roomCenter, roomRadius);
                    gradient.setColorAt(0.95, QColor(255, 0, 0, 150));
                    gradient.setColorAt(0.80, QColor(150, 100, 100, 150));
                    gradient.setColorAt(0.799, QColor(150, 100, 100, 100));
                    gradient.setColorAt(0.7, QColor(255, 0, 0, 200));
                    gradient.setColorAt(0, Qt::white);
                    QPen transparentPen(Qt::transparent);
                    QPainterPath myPath;
                    painter.setBrush(gradient);
                    painter.setPen(transparentPen);
                    myPath.addEllipse(roomCenter, roomRadius, roomRadius);
                    painter.drawPath(myPath);

                    mPick = false;
                    mTarget = it.key();
                    if (mpMap->mpRoomDB->getRoom(mTarget)) {
                        mpMap->mTargetID = mTarget;
                        if (mpMap->findPath(mpMap->mRoomIdHash.value(mpHost->getName()), mpMap->mTargetID)) {
                            mpMap->mpHost->startSpeedWalk();
                        } else {
                            QString msg = tr("Mapper: Cannot find a path to this room using known exits.\n");
                            mpHost->mpConsole->printSystemMessage(msg);
                        }
                    }
                }
            }
        }
    }

    if (mpMap->mapLabels.contains(mAreaID)) {
        QMapIterator<int, TMapLabel> it(mpMap->mapLabels[mAreaID]);
        while (it.hasNext()) {
            it.next();
            auto labelID = it.key();
            auto label = it.value();

            if (label.pos.z() != mOz) {
                continue;
            }
            if (label.text.length() < 1) {
                mpMap->mapLabels[mAreaID][labelID].text = tr("no text", "Default text if a label is created in mapper with no text");
            }
            QPointF labelPosition;
            int labelX = label.pos.x() * mRoomWidth + mRX;
            int labelY = label.pos.y() * mRoomHeight * -1 + mRY;

            labelPosition.setX(labelX);
            labelPosition.setY(labelY);
            int labelWidth = abs(qRound(label.size.width() * mRoomWidth));
            int labelHeight = abs(qRound(label.size.height() * mRoomHeight));

            if (!((0 < labelX || 0 < labelX + labelWidth) && (widgetWidth > labelX || widgetWidth > labelX + labelWidth))) {
                continue;
            }
            if (!((0 < labelY || 0 < labelY + labelHeight) && (widgetHeight > labelY || widgetHeight > labelY + labelHeight))) {
                continue;
            }
            QRectF labelPaintRectangle = QRect(label.pos.x() * mRoomWidth + mRX, label.pos.y() * mRoomHeight * -1 + mRY, labelWidth, labelHeight);
            if (label.showOnTop) {
                if (!label.noScaling) {
                    painter.drawPixmap(labelPosition, label.pix.scaled(labelPaintRectangle.size().toSize()));
                    mpMap->mapLabels[mAreaID][labelID].clickSize.setWidth(labelPaintRectangle.width());
                    mpMap->mapLabels[mAreaID][labelID].clickSize.setHeight(labelPaintRectangle.height());
                } else {
                    painter.drawPixmap(labelPosition, label.pix);
                    mpMap->mapLabels[mAreaID][labelID].clickSize.setWidth(label.pix.width());
                    mpMap->mapLabels[mAreaID][labelID].clickSize.setHeight(label.pix.height());
                }
            }
            if (label.highlight) {
                labelPaintRectangle.setSize(label.clickSize);
                painter.fillRect(labelPaintRectangle, QColor(255, 155, 55, 190));
            }
        }
    }

    // Similar code was used to indicate target of custom exit line selected for
    // editing but this could not be done there because gridmode areas don't hit
    // that bit of code and later rooms would overwrite the target...
    if (mMultiSelectionHighlightRoomId > 0 && mMultiSelectionSet.size() > 1) {
        TRoom* pR_multiSelectionHighlight = mpMap->mpRoomDB->getRoom(mMultiSelectionHighlightRoomId);
        if (pR_multiSelectionHighlight) {
            float r_mSx = pR_multiSelectionHighlight->x * mRoomWidth + mRX;
            float r_mSy = pR_multiSelectionHighlight->y * -1 * mRoomHeight + mRY;
            QPen savePen = painter.pen();
            QBrush saveBrush = painter.brush();
            float roomRadius = mRoomWidth * 1.2;
            float roomDiagonal = mRoomWidth * 1.2;
            QPointF roomCenter = QPointF(r_mSx, r_mSy);

            QPen yellowPen(QColor(255, 255, 50, 192)); // Quarter opaque yellow pen
            yellowPen.setWidth(mRoomWidth * 0.1);
            QPainterPath myPath;
            painter.setPen(yellowPen);
            painter.setBrush(Qt::NoBrush);
            myPath.addEllipse(roomCenter, roomRadius, roomRadius);
            myPath.addEllipse(roomCenter, roomRadius / 2.0, roomRadius / 2.0);
            myPath.moveTo(r_mSx - roomDiagonal, r_mSy - roomDiagonal);
            myPath.lineTo(r_mSx + roomDiagonal, r_mSy + roomDiagonal);
            myPath.moveTo(r_mSx + roomDiagonal, r_mSy - roomDiagonal);
            myPath.lineTo(r_mSx - roomDiagonal, r_mSy + roomDiagonal);
            painter.drawPath(myPath);
            painter.setPen(savePen);
            painter.setBrush(saveBrush);
        }
    }

    QColor infoColor;
    if (mpHost->mBgColor_2.lightness() > 127) {
        infoColor = QColor(Qt::black);
    } else {
        infoColor = QColor(Qt::white);
    }

    // Draw central red circle:
    if (!mShiftMode) {
        painter.save();
        QPen transparentPen(Qt::transparent);
        QPainterPath myPath;
        if (mpHost->mMapStrongHighlight) {
            // Never set, no means to except via XMLImport, as dlgMapper class's
            // slot_toggleStrongHighlight is not wired up to anything
            QRectF dr = QRectF(px - (mRoomWidth * rSize) / 2.0, py - (mRoomHeight * rSize) / 2.0, mRoomWidth * rSize, mRoomHeight * rSize);
            painter.fillRect(dr, QColor(255, 0, 0, 150));

            float roomRadius = 0.95 * mRoomWidth;
            QPointF roomCenter = QPointF(px, py);
            QRadialGradient gradient(roomCenter, roomRadius);
            gradient.setColorAt(0.95, QColor(255, 0, 0, 150));
            gradient.setColorAt(0.80, QColor(150, 100, 100, 150));
            gradient.setColorAt(0.799, QColor(150, 100, 100, 100));
            gradient.setColorAt(0.7, QColor(255, 0, 0, 200));
            gradient.setColorAt(0, Qt::white);
            painter.setBrush(gradient);
            painter.setPen(transparentPen);
            myPath.addEllipse(roomCenter, roomRadius, roomRadius);
        } else {
            float roomRadius = 0.95 * mRoomWidth;
            QPointF roomCenter = QPointF(px, py);
            QRadialGradient gradient(roomCenter, roomRadius);
            gradient.setColorAt(0.95, QColor(255, 0, 0, 150));
            gradient.setColorAt(0.80, QColor(150, 100, 100, 150));
            gradient.setColorAt(0.799, QColor(150, 100, 100, 100));
            gradient.setColorAt(0.3, QColor(150, 150, 150, 100));
            gradient.setColorAt(0.1, QColor(255, 255, 255, 100));
            gradient.setColorAt(0, Qt::white);
            painter.setBrush(gradient);
            painter.setPen(transparentPen);
            myPath.addEllipse(roomCenter, roomRadius, roomRadius);
        }
        painter.drawPath(myPath);
        painter.restore();
    }


    // Work out text for information box, need to offset if room selection widget is present
    if (mShowInfo) {
        QString infoText;
        int roomID = mRoomID;
        if (!isCenterViewCall && !mMultiSelectionSet.empty()) {
            if (mpMap->mpRoomDB->getRoom(*(mMultiSelectionSet.constBegin()))) {
                roomID = mMultiSelectionHighlightRoomId;
            }
        }

        painter.save(); // Save painter state
        QFont f = painter.font();
        TRoom* _prid = mpMap->mpRoomDB->getRoom(roomID);
        if (_prid) {
            int _iaid = _prid->getArea();
            TArea* _paid = mpMap->mpRoomDB->getArea(_iaid);
            QString _paid_name = mpMap->mpRoomDB->getAreaNamesMap().value(_iaid);
            if (_paid) {
                infoText = tr("Area: %1 ID:%2 x:%3 <-> %4 y:%5 <-> %6 z:%7 <-> %8\n")
                                   .arg(_paid_name,
                                        QString::number(_iaid),
                                        QString::number(_paid->min_x),
                                        QString::number(_paid->max_x),
                                        QString::number(_paid->min_y),
                                        QString::number(_paid->max_y),
                                        QString::number(_paid->min_z),
                                        QString::number(_paid->max_z));
            } else {
                infoText = QChar::LineFeed;
            }

            if (!_prid->name.isEmpty()) {
                infoText.append(tr("Room Name: %1\n").arg(_prid->name));
            }

            uint selectionSize = mMultiSelectionSet.size();
            // Italicise the text if the current display area {mAreaID} is not the
            // same as the displayed text information - which happens when NO
            // room is selected AND the current area is NOT the one the player
            // is in (to emphasis that the displayed data is {mostly} not about
            // the CURRENTLY VISIBLE area)... make it bold if the player room IS
            // in the displayed map

            // If one or more rooms are selected - make the text slightly orange.
            switch (selectionSize) {
            case 0:
                infoText.append(
                        tr("Room ID: %1 (Current) Position on Map: (%2,%3,%4)\n").arg(QString::number(roomID), QString::number(_prid->x), QString::number(_prid->y), QString::number(_prid->z)));
                if (playerAreaID != mAreaID) {
                    f.setItalic(true);
                } else {
                    f.setBold(true);
                }
                break;
            case 1:
                infoText.append(
                        tr("Room ID: %1 (Selected) Position on Map: (%2,%3,%4)\n").arg(QString::number(roomID), QString::number(_prid->x), QString::number(_prid->y), QString::number(_prid->z)));
                f.setBold(true);
                if (infoColor.lightness() > 127) {
                    infoColor = QColor(255, 223, 191); // Slightly orange white
                } else {
                    infoColor = QColor(96, 48, 0); // Dark, slightly orange grey
                }
                break;
            default:
                infoText.append(tr("Room ID: %1 (%5 Selected) Position on Map: (%2,%3,%4)\n")
                                        .arg(QString::number(roomID), QString::number(_prid->x), QString::number(_prid->y), QString::number(_prid->z), QString::number(selectionSize)));
                f.setBold(true);
                if (infoColor.lightness() > 127) {
                    infoColor = QColor(255, 223, 191); // Slightly orange white
                } else {
                    infoColor = QColor(96, 48, 0); // Dark, slightly orange grey
                }
                break;
            }
        }

#ifdef QT_DEBUG
        infoText.append(tr("render time: %1S mO: (%2,%3,%4)").arg(renderTimer.nsecsElapsed() * 1.0e-9, 0, 'f', 3).arg(QString::number(mOx), QString::number(mOy), QString::number(mOz)));
#endif

        // Left margin for info widget:
        uint infoLeftSideAvoid = 10;
        if (mMultiSelectionListWidget.isVisible()) {
            // Room Selection Widget showing, so increase margin to avoid:
            infoLeftSideAvoid += mMultiSelectionListWidget.x() + mMultiSelectionListWidget.rect().width();
        }

        uint infoHeight = 5 + mFontHeight; // Account for first iteration
        QRect testRect;
        // infoRect has a 10 margin on either side and on top to widget frame.
        mMapInfoRect = QRect(infoLeftSideAvoid, 10, width() - 10 - infoLeftSideAvoid, infoHeight);
        do {
            infoHeight += mFontHeight;
            mMapInfoRect.setHeight(infoHeight);
            // Test in a rectangle that is 10 less on all sides:
            testRect = painter.boundingRect(
                    mMapInfoRect.left() + 10, mMapInfoRect.top() + 10, mMapInfoRect.width() - 20, mMapInfoRect.height() - 20, Qt::TextWordWrap | Qt::AlignLeft | Qt::AlignTop, infoText);

        } while ((testRect.height() > mMapInfoRect.height() - 20 || testRect.width() > mMapInfoRect.width() - 20) && infoHeight < height());
        // Last term above is needed to prevent runaway under "odd" conditions

        // Restore Grey translucent background, was useful for debugging!
        painter.fillRect(mMapInfoRect, QColor(150, 150, 150, 80));
        painter.setPen(infoColor);
        painter.setFont(f);
        painter.drawText(mMapInfoRect.left() + 10, mMapInfoRect.top() + 10, mMapInfoRect.width() - 20, mMapInfoRect.height() - 20, Qt::TextWordWrap | Qt::AlignLeft | Qt::AlignTop, infoText);
        //forget about font size changing and bolding/italicisation:
        painter.restore();
    }

    static bool isAreaWidgetValid = true; // Remember between uses
    QFont _f = mpMap->mpMapper->showArea->font();
    if (isAreaWidgetValid) {
        if (mAreaID == -1                                 // the map being shown is the "default" area
            && !mpMap->mpMapper->getDefaultAreaShown()) { // the area widget is not showing the "default" area

            isAreaWidgetValid = false; // So the widget CANNOT indicate the correct area
            // Set the area widget to indicate the area widget is NOT
            // showing valid text - so make it italic and crossed out
            _f.setItalic(true);
            _f.setUnderline(true);
            _f.setStrikeOut(true);
            _f.setOverline(true);
        }
    } else {
        if (!(mAreaID == -1 && !mpMap->mpMapper->getDefaultAreaShown())) {
            isAreaWidgetValid = true; // So the widget CAN now indicate the correct area
            // Reset to normal
            _f.setItalic(false);
            _f.setUnderline(false);
            _f.setStrikeOut(false);
            _f.setOverline(false);
        }
    }

    mpMap->mpMapper->showArea->setFont(_f);

    if (mHelpMsg.size() > 0) {
        painter.setPen(QColor(255, 155, 50));
        QFont _f = painter.font();
        QFont _f2 = _f;
        _f.setPointSize(12); // 20 was a little large
        _f.setBold(true);
        painter.setFont(_f);
        QRect _r = QRect(0, 0, widgetWidth, widgetHeight);
        painter.drawText(_r, Qt::AlignHCenter | Qt::AlignBottom | Qt::TextWordWrap, mHelpMsg);
        // Now draw text centered at bottom, so it does not clash with info window
        painter.setFont(_f2);
    }
}


void T2DMap::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (mDialogLock) {
        return;
    }
    int x = event->x();
    int y = event->y();
    mPHighlight = QPoint(x, y);
    mPick = true;
    mStartSpeedWalk = true;
    repaint();
}

void T2DMap::createLabel(QRectF labelRectangle)
{
    // N/U:     QRectF selectedRegion = labelRectangle;
    TMapLabel label;
    QFont font;
    QString text = tr("no text", "Default text if a label is created in mapper with no text");
    QString imagePath;

    mHelpMsg.clear();

    QMessageBox textOrImageDialog;
    textOrImageDialog.setText(tr("Type of label?"));
    QPushButton* textButton = textOrImageDialog.addButton(tr("Text Label"), QMessageBox::ActionRole);
    QPushButton* imageButton = textOrImageDialog.addButton(tr("Image Label"), QMessageBox::ActionRole);
    textOrImageDialog.setStandardButtons(QMessageBox::Cancel);
    textOrImageDialog.exec();
    if (textOrImageDialog.clickedButton() == textButton) {
        QString title = tr("Enter label text.");
        font = QFontDialog::getFont(nullptr);
        text = QInputDialog::getText(nullptr, title, title);
        if (text.length() < 1) {
            text = tr("no text", "Default text if a label is created in mapper with no text");
        }
        label.text = text;
        label.bgColor = QColorDialog::getColor(QColor(50, 50, 150, 100), nullptr, tr("Background color"));
        label.fgColor = QColorDialog::getColor(QColor(255, 255, 50, 255), nullptr, tr("Foreground color"));
    } else if (textOrImageDialog.clickedButton() == imageButton) {
       label.bgColor = QColor(50, 50, 150, 100);
        label.fgColor = QColor(255, 255, 50, 255);
        label.text = "";
        imagePath = QFileDialog::getOpenFileName(nullptr, tr("Select image"));
    } else {
        return;
    }

    QMessageBox backgroundOrForegroundDialog;
    backgroundOrForegroundDialog.setStandardButtons(QMessageBox::Cancel);
    backgroundOrForegroundDialog.setText(tr("Draw label as background or on top of everything?"));
    QPushButton* backgroundButton = backgroundOrForegroundDialog.addButton(tr("Background"), QMessageBox::ActionRole);
    QPushButton* foregroundButton = backgroundOrForegroundDialog.addButton(tr("Foreground"), QMessageBox::ActionRole);
    backgroundOrForegroundDialog.exec();
    bool showOnTop = false;
    if (backgroundOrForegroundDialog.clickedButton() == backgroundButton) {
        showOnTop = false;
    } else if (backgroundOrForegroundDialog.clickedButton() == foregroundButton) {
        showOnTop = true;
    } else {
        return;
    }

    label.showOnTop = showOnTop;
    QPixmap pixmap(fabs(labelRectangle.width()), fabs(labelRectangle.height()));
    QRect drawRectangle = labelRectangle.normalized().toRect();
    drawRectangle.moveTo(0, 0);
    //pixmap.fill(QColor(0,255,0,0));
    QPainter labelPainter(&pixmap);
    QPen labelPen;
    labelPainter.setFont(font);
    labelPen.setColor(label.fgColor);
    labelPainter.setPen(labelPen);
    labelPainter.fillRect(drawRectangle, label.bgColor);
    if (textOrImageDialog.clickedButton() == textButton) {
        labelPainter.drawText(drawRectangle, Qt::AlignHCenter | Qt::AlignCenter, text, nullptr);
    } else {
        QPixmap imagePixmap = QPixmap(imagePath);
        labelPainter.drawPixmap(QPoint(0, 0), imagePixmap.scaled(drawRectangle.size()));
    }
    label.pix = pixmap.copy(drawRectangle);
    labelRectangle = labelRectangle.normalized();
    float mx = (labelRectangle.topLeft().x() / mRoomWidth) + mOx - (xspan / 2.0);
    float my = (yspan / 2.0) - (labelRectangle.topLeft().y() / mRoomHeight) - mOy;

    float mx2 = (labelRectangle.bottomRight().x() / mRoomWidth) + mOx - (xspan / 2.0);
    float my2 = (yspan / 2.0) - (labelRectangle.bottomRight().y() / mRoomHeight) - mOy;
    label.pos = QVector3D(mx, my, mOz);
    label.size = QRectF(QPointF(mx, my), QPointF(mx2, my2)).normalized().size();
    if (!mpMap->mpRoomDB->getArea(mAreaID)) {
        return;
    }
    int labelID;
    if (!mpMap->mapLabels.contains(mAreaID)) {
        QMap<int, TMapLabel> m;
        m[0] = label;
        mpMap->mapLabels[mAreaID] = m;
    } else {
        labelID = mpMap->createMapLabelID(mAreaID);
        mpMap->mapLabels[mAreaID].insert(labelID, label);
    }
    update();
}

void T2DMap::mouseReleaseEvent(QMouseEvent* e)
{
    if (mMoveLabel) {
        mMoveLabel = false;
    }

    //move map with left mouse button + ALT (->
    if (mpMap->mLeftDown) {
        mpMap->mLeftDown = false;
    }

    if (e->button() & Qt::LeftButton) {
        mMultiSelection = false; // End drag-to-select rectangle resizing
        mHelpMsg.clear();
        if (mSizeLabel) {
            mSizeLabel = false;
            QRectF labelRect = mMultiRect;
            createLabel(labelRect);
        }
        mMultiRect = QRect(0, 0, 0, 0);
        update();
    }
}

bool T2DMap::event(QEvent* event)
{
    // NOTE: key events aren't being forwarded to T2DMap because the widget
    // currently never has focus because it's more comfortable for the user
    // to always have focus on the command line. If this were to be changed some
    // day the setFocusPolicy() calls in the constructor need to be uncommented

    if (event->type() == QEvent::KeyPress) {
        auto * ke = static_cast<QKeyEvent*>(event);
        //        if( ke->key() == Qt::Key_Delete )
        //        {
        //            if( mCustomLineSelectedRoom != 0  )
        //            {
        //                if( mpMap->rooms.contains(mCustomLineSelectedRoom) )
        //                {
        //                    TRoom * pR = mpMap->rooms[mCustomLineSelectedRoom];
        //                    if( pR->customLines.contains( mCustomLineSelectedExit) )
        //                    {
        //                        pR->customLines.remove(mCustomLineSelectedExit);
        //                        repaint();
        //                        mCustomLineSelectedRoom = 0;
        //                        mCustomLineSelectedExit = "";
        //                        mCustomLineSelectedPoint = -1;
        //                        return QWidget::event(event);
        //                    }
        //                }
        //            }
        //        }
    } else if (event->type() == QEvent::Resize) { // Tweak the room selection widget to fit
        resizeMultiSelectionWidget();
    }
    return QWidget::event(event);
}

void T2DMap::mousePressEvent(QMouseEvent* event)
{
    mNewMoveAction = true;
    if (event->buttons() & Qt::LeftButton) {
        // move map with left mouse button + ALT
        if (event->modifiers().testFlag(Qt::AltModifier)) {
            mpMap->mLeftDown = true;
        }

        // drawing new custom exit line
        if (mCustomLinesRoomFrom > 0) {
            if (mDialogLock) {
                return; // Prevent any line drawing until ready
            }

            TRoom* room = mpMap->mpRoomDB->getRoom(mCustomLinesRoomFrom);
            if (room) {
                float mx = (event->pos().x() / mRoomWidth) + mOx - (xspan / 2.0);
                float my = (yspan / 2.0) - (event->pos().y() / mRoomHeight) - mOy;
                // might be useful to have a snap to grid type option
                room->customLines[mCustomLinesRoomExit].push_back(QPointF(mx, my));
                room->calcRoomDimensions();
                repaint();
                return;
            }
        }

        // check click on custom exit lines
        if (mMultiSelectionSet.isEmpty()) {
            // But NOT if got one or more rooms already selected!
            TArea* pA = mpMap->mpRoomDB->getArea(mAreaID);
            if (pA) {
                TArea* pArea = pA;
                float mx = (event->pos().x() / mRoomWidth) + mOx - (xspan / 2.0);
                float my = (yspan / 2.0) - (event->pos().y() / mRoomHeight) - mOy;
                QPointF pc = QPointF(mx, my);
                QSetIterator<int> itRoom = pArea->rooms;
                while (itRoom.hasNext()) {
                    int currentRoomId = itRoom.next();
                    TRoom* room = mpMap->mpRoomDB->getRoom(currentRoomId);
                    if (!room) {
                        continue;
                    }
                    QMapIterator<QString, QList<QPointF>> it(room->customLines);
                    while (it.hasNext()) {
                        it.next();
                        const QList<QPointF>& _pL = it.value();
                        if (!_pL.empty()) {
                            // The way this code is structured means that EARLIER
                            // points are selected in preference to later ones!
                            // This might not be intuitive to the users...
                            float olx, oly, lx, ly;
                            for (int j = 0; j < _pL.size(); j++) {
                                if (j == 0) {
                                    // First segment of a custom line
                                    // start it at the centre of the room
                                    olx = room->x;
                                    oly = room->y;
                                    //FIXME: exit richtung beachten, um den Linienanfangspunkt zu berechnen
                                    lx = _pL[0].x();
                                    ly = _pL[0].y();
                                } else {
                                    // Not the first segment of a custom line
                                    // so start it at the end of the previous one
                                    olx = lx;
                                    oly = ly;
                                    lx = _pL[j].x();
                                    ly = _pL[j].y();
                                }
                                // End of each custom line segment is given

                                // click auf einen edit - punkt
                                if (mCustomLineSelectedRoom != 0) {
                                    // We have already chosen a line to edit
                                    if (fabs(mx - lx) <= 0.25 && fabs(my - ly) <= 0.25) {
                                        // And this looks close enough to a point that we should edit it
                                        mCustomLineSelectedPoint = j;
                                        return;
                                    }
                                }

                                // We have not previously chosen a line to edit
                                QLineF line = QLineF(olx, oly, lx, ly);
                                QLineF normal = line.normalVector();
                                QLineF tl;
                                tl.setP1(pc);
                                tl.setAngle(normal.angle());
                                tl.setLength(0.1);
                                QLineF tl2;
                                tl2.setP1(pc);
                                tl2.setAngle(normal.angle());
                                tl2.setLength(-0.1);
                                QPointF pi;
                                if ((line.intersect(tl, &pi) == QLineF::BoundedIntersection)
                                    || (line.intersect(tl2, &pi) == QLineF::BoundedIntersection)) {

                                    // Choose THIS line to edit as we have
                                    // clicked close enough to it...
                                    mCustomLineSelectedRoom = room->getId();
                                    mCustomLineSelectedExit = it.key();
                                    repaint();
                                    return;
                                }
                            }
                        }
                    }
                }
            }
            mCustomLineSelectedRoom = 0;
            mCustomLineSelectedExit = "";
        }

        if (mRoomBeingMoved) {
            // Moving rooms so end that
            mPick = true;

            setMouseTracking(false);
            mRoomBeingMoved = false;
        } else if (!mPopupMenu) {
            // Not in a context menu, so start selection mode - including drag to select
            mMultiSelection = true;
            mMultiRect = QRect(event->pos(), event->pos());
            int _roomID = mRoomID;
            if (!mpMap->mpRoomDB->getRoom(_roomID)) {
                return;
            }
            int _areaID = mAreaID;
            TArea* pArea = mpMap->mpRoomDB->getArea(_areaID);
            if (!pArea) {
                return;
            }
            float fx = ((xspan / 2.0) - mOx) * mRoomWidth;
            float fy = ((yspan / 2.0) - mOy) * mRoomHeight;

            if (!event->modifiers().testFlag(Qt::ControlModifier)) {
                // If control key NOT down then clear selection, and put up helpful text
                mHelpMsg = tr("Drag to select multiple rooms or labels, release to finish...");
                mMultiSelectionSet.clear();
            }

            QSetIterator<int> itRoom(pArea->getAreaRooms());
            while (itRoom.hasNext()) { // Scan to find rooms in selection
                int currentAreaRoom = itRoom.next();
                TRoom* room = mpMap->mpRoomDB->getRoom(currentAreaRoom);
                if (!room) {
                    continue;
                }
                int rx = room->x * mRoomWidth + fx;
                int ry = room->y * -1 * mRoomHeight + fy;
                int rz = room->z;

                int mx = event->pos().x();
                int my = event->pos().y();
                int mz = mOz;
                if ((abs(mx - rx) < qRound(mRoomWidth * rSize / 2.0))
                    && (abs(my - ry) < qRound(mRoomHeight * rSize / 2.0))
                    && (mz == rz)) {

                    if (mMultiSelectionSet.contains(currentAreaRoom)
                       && event->modifiers().testFlag(Qt::ControlModifier)) {

                        mMultiSelectionSet.remove(currentAreaRoom);
                    } else {
                        mMultiSelectionSet.insert(currentAreaRoom);
                    }

                    if (!mMultiSelectionSet.empty()) {
                        mMultiSelection = false;
                    }
                }
            }
            switch (mMultiSelectionSet.size()) {
            case 0:
                mMultiSelectionHighlightRoomId = 0;
                break;
            case 1:
                mMultiSelection = false; // OK, found one room so stop
                mMultiSelectionHighlightRoomId = mMultiSelectionSet.toList().first();
                mHelpMsg.clear();
                break;
            default:
                mMultiSelection = false; // OK, found more than one room so stop
                mHelpMsg.clear();
                getCenterSelection();
            }

            // select labels
            if (mpMap->mapLabels.contains(mAreaID)) {
                QMapIterator<int, TMapLabel> it(mpMap->mapLabels[mAreaID]);
                while (it.hasNext()) {
                    it.next();
                    if (it.value().pos.z() != mOz) {
                        continue;
                    }

                    QPointF labelPosition;
                    float labelX = it.value().pos.x() * mRoomWidth + mRX;
                    float labelY = it.value().pos.y() * mRoomHeight * -1 + mRY;

                    labelPosition.setX(labelX);
                    labelPosition.setY(labelY);
                    int mx = event->pos().x();
                    int my = event->pos().y();
                    QPoint click = QPoint(mx, my);
                    QRectF br = QRect(labelX, labelY, it.value().clickSize.width(), it.value().clickSize.height());
                    if (br.contains(click)) {
                        if (!it.value().highlight) {
                            mLabelHighlighted = true;
                            mpMap->mapLabels[mAreaID][it.key()].highlight = true;
                        } else {
                            mpMap->mapLabels[mAreaID][it.key()].highlight = false;
                            mLabelHighlighted = false;
                        }
                        update();
                        return;
                    }
                }
            }

            mLabelHighlighted = false;
            update();

            if (mMultiSelection && !mMultiSelectionSet.empty()
                                   && (event->modifiers().testFlag(Qt::ControlModifier))) {

                // We were dragging multi-selection rectangle, we had selected at
                // least one room and the user has <CTRL>-clicked with the mouse
                // so switch off the dragging
                mMultiSelection = false;
                mHelpMsg.clear();
            }

        } else { // In popup menu, so end that
            mPopupMenu = false;
        }

        // display room selection list widget if more than 1 room has been selected
        // -> user can manually change current selection if rooms are overlapping
        if (mMultiSelectionSet.size() > 1) {
            // We don't want to cause calls to slot_roomSelectionChanged() here!
            mMultiSelectionListWidget.blockSignals(true);
            mIsSelectionSorting = mMultiSelectionListWidget.isSortingEnabled();
            mIsSelectionSortByNames = (mMultiSelectionListWidget.sortColumn() == 1);
            mMultiSelectionListWidget.clear();
            // Do NOT sort whilst inserting items!
            mMultiSelectionListWidget.setSortingEnabled(false);
            QSetIterator<int> itRoom = mMultiSelectionSet;
            mIsSelectionUsingNames = false;
            while (itRoom.hasNext()) {
                auto _item = new QTreeWidgetItem;
                int multiSelectionRoomId = itRoom.next();
                // Pad with spaces so sorting works
                _item->setText(0, QStringLiteral("%1").arg(multiSelectionRoomId, 7));
                _item->setTextAlignment(0, Qt::AlignRight);
                TRoom* pR_multiSelection = mpMap->mpRoomDB->getRoom(multiSelectionRoomId);
                if (pR_multiSelection) {
                    QString multiSelectionRoomName = pR_multiSelection->name;
                    if (!multiSelectionRoomName.isEmpty()) {
                        _item->setText(1, multiSelectionRoomName);
                        _item->setTextAlignment(1, Qt::AlignLeft);
                        mIsSelectionUsingNames = true;
                    }
                }
                mMultiSelectionListWidget.addTopLevelItem(_item);
            }
            mMultiSelectionListWidget.setColumnHidden(1, !mIsSelectionUsingNames);
            // Can't sort if nothing to sort on, switch to sorting by room number
            if ((!mIsSelectionUsingNames) && mIsSelectionSortByNames && mIsSelectionSorting) {
                mIsSelectionSortByNames = false;
            }
            mMultiSelectionListWidget.sortByColumn(mIsSelectionSortByNames ? 1 : 0);
            mMultiSelectionListWidget.setSortingEnabled(mIsSelectionSorting);
            resizeMultiSelectionWidget();
            mMultiSelectionListWidget.selectAll();
            mMultiSelectionListWidget.blockSignals(false);
            mMultiSelectionListWidget.show();
            update();
        } else {
            mMultiSelectionListWidget.hide();
        }
    }


    if (event->buttons() & Qt::RightButton) {
        auto popup = new QMenu(this);
        popup->setToolTipsVisible(true);

        if (mCustomLinesRoomFrom > 0) {
            if (mDialogLock) {
                return;
            }

            TRoom* room = mpMap->mpRoomDB->getRoom(mCustomLinesRoomFrom);
            if (room) {
                QAction* action = new QAction(tr("Undo", "Menu option to undo drawing the last segment of a line in the mapper"), this);
                action->setToolTip(tr("Undo last point"));
                if (room->customLines.value(mCustomLinesRoomExit).count() > 1) {
                    connect(action, &QAction::triggered, this, &T2DMap::slot_undoCustomLineLastPoint);
                } else {
                    action->setEnabled(false);
                }

                QAction* action2 = new QAction(tr("Properties", "Menu option to change properties of a line in the mapper"), this);
                action2->setText("Properties...");  // Changed seperately, because the constructor silently copies the text elsewhere
                                                    // (tool-tip and/or object name IIRC) whereas the ellipsis is meant only for display
                action2->setToolTip(tr("Change the properties of this line"));
                connect(action2, &QAction::triggered, this, &T2DMap::slot_customLineProperties);

                QAction* action3 = new QAction(tr("Finish", "Menu option to finish drawing a line in the mapper"), this);
                action3->setToolTip(tr("Finish drawing this line"));
                connect(action3, &QAction::triggered, this, &T2DMap::slot_doneCustomLine);

                mPopupMenu = true;

                room->calcRoomDimensions();
                popup->addAction(action);
                popup->addAction(action2);
                popup->addAction(action3);

                popup->popup(mapToGlobal(event->pos()));
                update();
                return;
            }
        }

        if (!mLabelHighlighted && mCustomLineSelectedRoom == 0) {
            auto playerRoom = mpMap->mpRoomDB->getRoom(mpMap->mRoomIdHash.value(mpHost->getName()));
            auto pArea = mpMap->mpRoomDB->getArea(mAreaID);

            if (!playerRoom || !pArea) {
                auto createMap = new QAction(tr("Create new map"), this);
                connect(createMap, &QAction::triggered, this, &T2DMap::slot_newMap);

                auto loadMap = new QAction(tr("Load map"), this);
                connect(loadMap, &QAction::triggered, this, &T2DMap::slot_loadMap);

                mPopupMenu = true;

                popup->addAction(createMap);
                popup->addAction(loadMap);

                popup->popup(mapToGlobal(event->pos()));
                return;
            }

            if (mMultiSelectionSet.isEmpty()) {
                mpCreateRoomAction = new QAction(tr("Create room", "Menu option to create a new room in the mapper"), this);
                mpCreateRoomAction->setToolTip(tr("Create a new room here"));
                connect(mpCreateRoomAction.data(), &QAction::triggered, this, &T2DMap::slot_createRoom);
                popup->addAction(mpCreateRoomAction);
            }

            auto moveRoom = new QAction(tr("Move", "Menu option to move a room in the mapper"), this);
            moveRoom->setToolTip(tr("Move room"));
            connect(moveRoom, &QAction::triggered, this, &T2DMap::slot_moveRoom);

            auto deleteRoom = new QAction(tr("Delete", "Menu option to delete a room in the mapper"), this);
            deleteRoom->setToolTip(tr("Delete room"));
            connect(deleteRoom, &QAction::triggered, this, &T2DMap::slot_deleteRoom);

            auto recolorRoom = new QAction(tr("Color", "Menu option to change a room color in the mapper"), this);
            recolorRoom->setToolTip(tr("Change room color"));
            connect(recolorRoom, &QAction::triggered, this, &T2DMap::slot_changeColor);

            auto spreadRooms = new QAction(tr("Spread", "Menu option to space out rooms in the mapper"), this);
            spreadRooms->setToolTip(tr("Increase map X-Y spacing for the selected group of rooms"));
            connect(spreadRooms, &QAction::triggered, this, &T2DMap::slot_spread);

            auto shrinkRooms = new QAction(tr("Shrink", "Menu option to move rooms closer in the mapper"), this);
            shrinkRooms->setToolTip(tr("Decrease map X-Y spacing for the selected group of rooms"));
            connect(shrinkRooms, &QAction::triggered, this, &T2DMap::slot_shrink);

            auto lockRoom = new QAction(tr("Lock", "Menu option to lock a room for speed walks in the mapper"), this);
            lockRoom->setToolTip(tr("Lock room for speed walks"));
            connect(lockRoom, &QAction::triggered, this, &T2DMap::slot_lockRoom);

            auto unlockRoom = new QAction(tr("Unlock", "Menu option to unlock a room for speed walks in the mapper"), this);
            unlockRoom->setToolTip(tr("Unlock room for speed walks"));
            connect(unlockRoom, &QAction::triggered, this, &T2DMap::slot_unlockRoom);

            auto weightRoom = new QAction(tr("Weight", "Menu option to set a room weight in the mapper"), this);
            weightRoom->setToolTip(tr("Set room weight"));
            connect(weightRoom, &QAction::triggered, this, &T2DMap::slot_setRoomWeight);

            auto roomExits = new QAction(tr("Exits", "Menu option to set exits for a room in the mapper"), this);
            roomExits->setToolTip(tr("Set room exits"));
            connect(roomExits, &QAction::triggered, this, &T2DMap::slot_setExits);

            auto roomSymbol = new QAction(tr("Symbol", "Menu option to set symbol(s) to mark rooms in the mapper"), this);
            roomSymbol->setToolTip(tr("Set one or more symbols or letters to mark special rooms"));
            connect(roomSymbol, &QAction::triggered, this, &T2DMap::slot_setSymbol);

            auto moveRoomXY = new QAction(tr("Move to", "Menu option to move rooms elsewhere in the mapper"), this);
            moveRoomXY->setToolTip(tr("Move selected group to a given position"));
            connect(moveRoomXY, &QAction::triggered, this, &T2DMap::slot_movePosition);

            auto roomArea = new QAction(tr("Area", "Menu option to set the area ID of a room in the mapper"), this);
            roomArea->setToolTip(tr("Set room area ID"));
            connect(roomArea, &QAction::triggered, this, &T2DMap::slot_setArea);

            auto customExitLine = new QAction(tr("Custom exit lines", "Menu option to create custom exit lines in the mapper"), this);
            if (!pArea) {
                return;
            }

          if (pArea->gridMode) {
                // Disable custom exit lines in grid mode as they aren't visible anyway
                customExitLine->setToolTip(tr("Custom exit lines are not shown and are not editable in grid mode"));
                customExitLine->setEnabled(false);
            } else {
                customExitLine->setToolTip(tr("Replace an exit line with a custom line"));
                connect(customExitLine, &QAction::triggered, this, &T2DMap::slot_setCustomLine);
            }

            auto createLabel = new QAction(tr("Create label", "Menu option to add text or image to the mapper"), this);
            createLabel->setToolTip(tr("Create labels to show text or images."));
            connect(createLabel, &QAction::triggered, this, &T2DMap::slot_createLabel);

            auto setPlayerLocation = new QAction(tr("Set location", "Menu option to assign player location in the mapper"), this);
            if (mMultiSelectionSet.size() == 1) { // Only enable if ONE room is highlighted
                setPlayerLocation->setToolTip(tr("Set player current location to here"));
                connect(setPlayerLocation, &QAction::triggered, this, &T2DMap::slot_setPlayerLocation);
            } else {
                setPlayerLocation->setEnabled(false);
                setPlayerLocation->setToolTip(tr("Cannot set location when not exactly one room selected"));
            }

            mPopupMenu = true;

            popup->addAction(moveRoom);
            popup->addAction(roomExits);
            popup->addAction(customExitLine);
            popup->addAction(recolorRoom);
            popup->addAction(roomSymbol);
            //popup->addAction( action11 );
            popup->addAction(spreadRooms);
            popup->addAction(shrinkRooms);
            //popup->addAction( action5 );
            popup->addAction(lockRoom);
            popup->addAction(unlockRoom);
            popup->addAction(weightRoom);
            popup->addAction(deleteRoom);
            popup->addAction(moveRoomXY);

            popup->addAction(roomArea);

            popup->addAction(createLabel);
            popup->addAction(setPlayerLocation);

            popup->popup(mapToGlobal(event->pos()));
        } else if (mLabelHighlighted) {
            auto moveLabel = new QAction(tr("Move", "Menu option to move a label in the mapper"), this);
            moveLabel->setToolTip(tr("Move label"));
            connect(moveLabel, &QAction::triggered, this, &T2DMap::slot_moveLabel);
            auto deleteLabel = new QAction(tr("Delete", "Menu option to delete a label in the mapper"), this);
            deleteLabel->setToolTip(tr("Delete label"));
            connect(deleteLabel, &QAction::triggered, this, &T2DMap::slot_deleteLabel);
            mPopupMenu = true;
            popup->addAction(moveLabel);
            popup->addAction(deleteLabel);
            popup->popup(mapToGlobal(event->pos()));
        } else {
            // seems that if we get here then we have right clicked on a selected custom line?
//            qDebug("T2DMap::mousePressEvent(): reached else case, mCustomLineSelectedRoom=%i, Exit=%s, Point=%i",
//                   mCustomLineSelectedRoom,
//                   qPrintable(mCustomLineSelectedExit),
//                   mCustomLineSelectedPoint);

            if (mCustomLineSelectedRoom > 0) {
                TRoom* room = mpMap->mpRoomDB->getRoom(mCustomLineSelectedRoom);
                if (room) {
                    auto addPoint = new QAction(tr("Add point", "Menu option to add a point to a custom line in the mapper"), this);
                    if (mCustomLineSelectedPoint > -1)
                    // The first user manipulable point IS zero - line is
                    // drawn to it from a point around room symbol dependent
                    // on the exit direction - and we can now add even to it
                    {
                        connect(addPoint, &QAction::triggered, this, &T2DMap::slot_customLineAddPoint);
                        addPoint->setToolTip(tr("Divide segment by adding a new point mid-way along"));
                    } else {
                        addPoint->setEnabled(false);
                        addPoint->setToolTip(tr("Select a point first, then add a new point mid-way along the segment towards room"));
                    }

                    auto removePoint = new QAction(tr("Remove point", "Menu option to remove a point of a custom line in the mapper"), this);
                    // Permit this to be enabled if the current point is 0 or
                    // greater, but not if there is no others
                    if (mCustomLineSelectedPoint > -1) {
                        if (room->customLines.value(mCustomLineSelectedExit).count() > 1) {
                            connect(removePoint, &QAction::triggered, this, &T2DMap::slot_customLineRemovePoint);
                            if ((mCustomLineSelectedPoint + 1) < room->customLines.value(mCustomLineSelectedExit).count()) {
                                removePoint->setToolTip(tr("Merge pair of segments by removing this point"));
                            } else {
                                removePoint->setToolTip(tr("Remove last segment by removing this point"));
                            }
                        } else {
                            removePoint->setEnabled(false);
                            removePoint->setToolTip(tr(R"(use "delete line" to remove the only segment ending in an editable point)"));
                        }
                    } else {
                        removePoint->setEnabled(false);
                        removePoint->setToolTip(tr("Select a point first, then remove it"));
                    }

                    auto lineProperties = new QAction(tr("Properties", "Menu option to adjust a custom line in the mapper"), this);
                    lineProperties->setText("Properties...");  // Changed seperately, because the constructor silently copies the text elsewhere
                                                               // (tool-tip and/or object name IIRC) whereas the ellipsis is meant only for display
                    lineProperties->setToolTip(tr("Change the properties of this custom line"));
                    connect(lineProperties, &QAction::triggered, this, &T2DMap::slot_customLineProperties);

                    auto deleteLine = new QAction(tr("Delete line", "Menu option to delete a custom line in the mapper"), this);
                    deleteLine->setToolTip(tr("Delete all of this custom line"));
                    connect(deleteLine, &QAction::triggered, this, &T2DMap::slot_deleteCustomExitLine);

                    mPopupMenu = true;

                    popup->addAction(addPoint);
                    popup->addAction(removePoint);
                    popup->addAction(lineProperties);
                    popup->addAction(deleteLine);
                    popup->popup(mapToGlobal(event->pos()));
                }
            }
        }
        //this is placed at the end since it is likely someone will want to hook anywhere
        QMap<QString, QMenu*> userMenus;
        QMapIterator<QString, QStringList> it(mUserMenus);
        while (it.hasNext()) {
            it.next();
            QStringList menuInfo = it.value();
            QString displayName = menuInfo[1];
            auto userMenu = new QMenu(displayName, this);
            userMenus.insert(it.key(), userMenu);
        }
        it.toFront();
        while (it.hasNext()) {
            //take care of nested menus now since they're all made
            it.next();
            QStringList menuInfo = it.value();
            QString menuParent = menuInfo[0];
            if (menuParent == "") { //parentless
                popup->addMenu(userMenus[it.key()]);
            } else { //has a parent
                userMenus[menuParent]->addMenu(userMenus[it.key()]);
            }
        }
        //add our actions
        QMapIterator<QString, QStringList> it2(mUserActions);
        auto mapper = new QSignalMapper(this);
        while (it2.hasNext()) {
            it2.next();
            QStringList actionInfo = it2.value();
            auto action = new QAction(actionInfo[2], this);
            if (actionInfo[1] == "") { //no parent
                popup->addAction(action);
            } else if (userMenus.contains(actionInfo[1])) {
                userMenus[actionInfo[1]]->addAction(action);
            } else {
                delete action;
                continue;
            }
            mapper->setMapping(action, it2.key());
            // TODO: QSignalMapper is not compatible with the functor (Qt5)
            // style of QObject::connect(...) - it has been declared obsolete
            // and should be replaced with lamba functions to perform what the
            // slot method did...
            connect(action, SIGNAL(triggered()), mapper, SLOT(map()));
        }
        connect(mapper, SIGNAL(mapped(QString)), this, SLOT(slot_userAction(QString)));
    }
    update();
}

// returns the current mouse position as X, Y coordinates on the map
std::pair<int, int> T2DMap::getMousePosition()
{
    QPoint mousePosition = this->mapFromGlobal(QCursor::pos());

    float mx = (mousePosition.x() / mRoomWidth) + mOx - (xspan / 2.0);
    float my = (yspan / 2.0) - (mousePosition.y() / mRoomHeight) - mOy;

    return make_pair(std::round(mx), std::round(my));
}

void T2DMap::slot_createRoom()
{
    if (!mpHost) {
        return;
    }

    auto roomID = mpHost->mpMap->createNewRoomID();
    if (!mpHost->mpMap->addRoom(roomID)) {
        return;
    }

    mpHost->mpMap->setRoomArea(roomID, mAreaID, false);

    auto mousePosition = getMousePosition();
    mpHost->mpMap->setRoomCoordinates(roomID, mousePosition.first, mousePosition.second, mOz);

    mpHost->mpMap->mMapGraphNeedsUpdate = true;
    if (mpHost->mpMap->mpM) {
        mpHost->mpMap->mpM->update();
    }
    if (mpHost->mpMap->mpMapper->mp2dMap) {
        mpHost->mpMap->mpMapper->mp2dMap->isCenterViewCall = true;
        mpHost->mpMap->mpMapper->mp2dMap->update();
        mpHost->mpMap->mpMapper->mp2dMap->isCenterViewCall = false;
    }
}

// Used both by "Properties..." context menu item for existing lines AND
// during drawing new ones.
void T2DMap::slot_customLineProperties()
{
    QString exit;
    TRoom* room;

    if (mCustomLineSelectedRoom > 0) {
        room = mpMap->mpRoomDB->getRoom(mCustomLineSelectedRoom);
        exit = mCustomLineSelectedExit;
    } else {
        room = mpMap->mpRoomDB->getRoom(mCustomLinesRoomFrom);
        exit = mCustomLinesRoomExit;
    }

    if (room) {
        if (exit.isEmpty()) {
            qDebug("T2DMap::slot_customLineProperties() called but no exit is selected...");
            return;
        }
        if (room->customLines.contains(exit)) {
            QUiLoader loader;

            QFile file(":/ui/custom_lines_properties.ui");
            file.open(QFile::ReadOnly);
            auto* dialog = qobject_cast<QDialog*>(loader.load(&file, this));
            file.close();
            if (!dialog) {
                qWarning("T2DMap::slot_customLineProperties() ERROR: failed to create the dialog!");
                return;
            }
            dialog->setWindowIcon(QIcon(QStringLiteral(":/icons/mudlet_custom_exit_properties.png")));
            auto* le_toId = dialog->findChild<QLineEdit*>("toId");
            auto* le_fromId = dialog->findChild<QLineEdit*>("fromId");
            auto* le_cmd = dialog->findChild<QLineEdit*>("cmd");

            mpCurrentLineStyle = dialog->findChild<QComboBox*>("lineStyle");
            mpCurrentLineColor = dialog->findChild<QPushButton*>("lineColor");
            mpCurrentLineArrow = dialog->findChild<QCheckBox*>("arrow");
            if (!le_toId || !le_cmd || !le_fromId || !mpCurrentLineStyle || !mpCurrentLineColor || !mpCurrentLineArrow) {
                qWarning("T2DMap::slot_customLineProperties() ERROR: failed to find an element in the dialog!");
                return;
            }
            le_cmd->setText(exit);
            le_fromId->setText(QString::number(room->getId()));
            if (exit == QLatin1String("nw")) {
                le_toId->setText(QString::number(room->getNorthwest()));
            } else if (exit == QLatin1String("n")) {
                le_toId->setText(QString::number(room->getNorth()));
            } else if (exit == QLatin1String("ne")) {
                le_toId->setText(QString::number(room->getNortheast()));
            } else if (exit == QLatin1String("up")) {
                le_toId->setText(QString::number(room->getUp()));
            } else if (exit == QLatin1String("w")) {
                le_toId->setText(QString::number(room->getWest()));
            } else if (exit == QLatin1String("e")) {
                le_toId->setText(QString::number(room->getEast()));
            } else if (exit == QLatin1String("down")) {
                le_toId->setText(QString::number(room->getDown()));
            } else if (exit == QLatin1String("sw")) {
                le_toId->setText(QString::number(room->getSouthwest()));
            } else if (exit == QLatin1String("s")) {
                le_toId->setText(QString::number(room->getSouth()));
            } else if (exit == QLatin1String("se")) {
                le_toId->setText(QString::number(room->getSoutheast()));
            } else if (exit == QLatin1String("in")) {
                le_toId->setText(QString::number(room->getIn()));
            } else if (exit == QLatin1String("out")) {
                le_toId->setText(QString::number(room->getOut()));
            } else {
                bool isFound = false;
                QMapIterator<int, QString> otherExitIt = room->getOtherMap();
                while (otherExitIt.hasNext()) {
                    otherExitIt.next();
                    if (otherExitIt.value().startsWith(QLatin1String("0")) || otherExitIt.value().startsWith(QLatin1String("1"))) {
                        if (otherExitIt.value().mid(1) == exit) {
                            le_toId->setText(QString::number(otherExitIt.key()));
                            isFound = true;
                            break;
                        }
                    } else if (otherExitIt.value() == exit) {
                        le_toId->setText(QString::number(otherExitIt.key()));
                        isFound = true;
                        break;
                    }
                }
                if (!isFound) {
                    qWarning(R"(T2DMap::slot_customLineProperties() - WARNING: missing command "%s" from custom lines for room id %i)",
                             qPrintable(exit), room->getId());
                }
            }

            mpCurrentLineStyle->setIconSize(QSize(48, 24));
            mpCurrentLineStyle->addItem(QIcon(QPixmap(QStringLiteral(":/icons/solid-line.png"))), tr("Solid line"), static_cast<int>(Qt::SolidLine));
            mpCurrentLineStyle->addItem(QIcon(QPixmap(QStringLiteral(":/icons/dot-line.png"))), tr("Dot line"), static_cast<int>(Qt::DotLine));
            mpCurrentLineStyle->addItem(QIcon(QPixmap(QStringLiteral(":/icons/dash-line.png"))), tr("Dash line"), static_cast<int>(Qt::DashLine));
            mpCurrentLineStyle->addItem(QIcon(QPixmap(QStringLiteral(":/icons/dash-dot-line.png"))), tr("Dash-dot line"), static_cast<int>(Qt::DashDotLine));
            mpCurrentLineStyle->addItem(QIcon(QPixmap(QStringLiteral(":/icons/dash-dot-dot-line.png"))), tr("Dash-dot-dot line"), static_cast<int>(Qt::DashDotDotLine));
            Qt::PenStyle lineStyle = room->customLinesStyle.value(exit);
            mpCurrentLineStyle->setCurrentIndex(mpCurrentLineStyle->findData(static_cast<int>(lineStyle)));

            mpCurrentLineArrow->setChecked(room->customLinesArrow.value(exit));
            mCurrentLineColor = room->customLinesColor.value(exit);

            mpCurrentLineColor->setStyleSheet(QStringLiteral("background-color: %1").arg(mCurrentLineColor.name()));
            connect(mpCurrentLineColor, &QAbstractButton::clicked, this, &T2DMap::slot_customLineColor);
            dialog->adjustSize();

            if (dialog->exec() == QDialog::Accepted) {
                // Make the changes
                mCurrentLineStyle = static_cast<Qt::PenStyle>(mpCurrentLineStyle->currentData().toInt());
                room->customLinesStyle[exit] = mCurrentLineStyle;
                room->customLinesColor[exit] = mCurrentLineColor;
                room->customLinesArrow[exit] = mpCurrentLineArrow->checkState();
                mCurrentLineArrow = mpCurrentLineArrow->checkState();
            }
        }
        repaint();
    } else {
        qDebug("T2DMap::slot_customLineProperties() called but no line is selected...");
    }
}


void T2DMap::slot_customLineAddPoint()
{
    TRoom* room = mpMap->mpRoomDB->getRoom(mCustomLineSelectedRoom);
    if (!room) {
        return;
    }

    QLineF segment;
    if (mCustomLineSelectedPoint > 0) {
        segment = QLineF(room->customLines.value(mCustomLineSelectedExit)
                                .at(mCustomLineSelectedPoint - 1),
                                room->customLines.value(mCustomLineSelectedExit)
                                .at(mCustomLineSelectedPoint));
    } else if (mCustomLineSelectedPoint == 0) {
        // The first user manipulable point IS zero - line is drawn to it from a
        // point around room symbol dependent on the exit direction
        // The first segment of custom line stick out half of the distance
        // between two rooms at a map unit vector distance apart. so an added
        // point inserted before the first must be placed halfway between
        // the offset point and the previous first point
        QPointF customLineStartPoint;
        if (mCustomLineSelectedExit == QLatin1String("n")) {
            customLineStartPoint = QPointF(room->x, room->y + 0.5);
        } else if (mCustomLineSelectedExit == QLatin1String("s")) {
            customLineStartPoint = QPointF(room->x, room->y - 0.5);
        } else if (mCustomLineSelectedExit == QLatin1String("e")) {
            customLineStartPoint = QPointF(room->x + 0.5, room->y);
        } else if (mCustomLineSelectedExit == QLatin1String("w")) {
            customLineStartPoint = QPointF(room->x - 0.5, room->y);
        } else if (mCustomLineSelectedExit == QLatin1String("ne")) {
            customLineStartPoint = QPointF(room->x + 0.5, room->y + 0.5);
        } else if (mCustomLineSelectedExit == QLatin1String("nw")) {
            customLineStartPoint = QPointF(room->x - 0.5, room->y + 0.5);
        } else if (mCustomLineSelectedExit == QLatin1String("se")) {
            customLineStartPoint = QPointF(room->x + 0.5, room->y - 0.5);
        } else if (mCustomLineSelectedExit == QLatin1String("sw")) {
            customLineStartPoint = QPointF(room->x - 0.5, room->y - 0.5);
        } else {
            customLineStartPoint = QPointF(room->x, room->y);
        }
        segment = QLineF(customLineStartPoint, room->customLines.value(mCustomLineSelectedExit).at(0));
    }
    segment.setLength(segment.length() / 2.0);
    room->customLines[mCustomLineSelectedExit].insert(mCustomLineSelectedPoint, segment.p2());
    mCustomLineSelectedPoint++;
    // Need to update the TRoom {min|max}_{x|y} settings as they are used during
    // the painting process:
    room->calcRoomDimensions();
    repaint();
}


void T2DMap::slot_customLineRemovePoint()
{
    TRoom* room = mpMap->mpRoomDB->getRoom(mCustomLineSelectedRoom);
    if (!room) {
        return;
    }

    if (mCustomLineSelectedPoint > 0) {
        room->customLines[mCustomLineSelectedExit].removeAt(mCustomLineSelectedPoint);
        mCustomLineSelectedPoint--;
    } else if (mCustomLineSelectedPoint == 0 && room->customLines.value(mCustomLineSelectedExit).count() > 1) {
        // The first user manipulable point IS zero - line is drawn to it from a
        // point around room symbol dependent on the exit direction.  We can only
        // allow it's deletion if there is at least another one left.
        room->customLines[mCustomLineSelectedExit].removeAt(mCustomLineSelectedPoint);
    }
    // Need to update the TRoom {min|max}_{x|y} settings as they are used during
    // the painting process:
    room->calcRoomDimensions();
    repaint();
}


void T2DMap::slot_undoCustomLineLastPoint()
{
    if (mCustomLinesRoomFrom > 0) {
        TRoom* room = mpMap->mpRoomDB->getRoom(mCustomLinesRoomFrom);
        if (room) {
            if (room->customLines.value(mCustomLinesRoomExit).count() > 0) {
                room->customLines[mCustomLinesRoomExit].pop_back();
            }
            room->calcRoomDimensions();
        }
        repaint();
    }
}

void T2DMap::slot_doneCustomLine()
{
    if (mpCustomLinesDialog) {
        mpCustomLinesDialog->accept();
        mpCustomLinesDialog = nullptr;
    }
    mHelpMsg = "";
    mCustomLinesRoomFrom = 0;
    mCustomLinesRoomTo = 0;
    mCustomLinesRoomExit.clear();
    if (!mMultiSelectionSet.empty()) {
        TRoom* room = mpMap->mpRoomDB->getRoom(mCustomLineSelectedRoom);
        if (room) {
            room->calcRoomDimensions();
        }
    }
    update();
}

void T2DMap::slot_deleteCustomExitLine()
{
    if (mCustomLineSelectedRoom > 0) {
        TRoom* room = mpMap->mpRoomDB->getRoom(mCustomLineSelectedRoom);
        if (room) {
            room->customLinesArrow.remove(mCustomLineSelectedExit);
            room->customLinesColor.remove(mCustomLineSelectedExit);
            room->customLinesStyle.remove(mCustomLineSelectedExit);
            room->customLines.remove(mCustomLineSelectedExit);
            mCustomLineSelectedRoom = 0;
            mCustomLineSelectedExit = "";
            mCustomLineSelectedPoint = -1;
            repaint();
            room->calcRoomDimensions();
            TArea* area = mpMap->mpRoomDB->getArea(room->getArea());
            if (area) {
                area->calcSpan();
            }
        }
    }
}

void T2DMap::slot_moveLabel()
{
    mMoveLabel = true;
}

void T2DMap::slot_deleteLabel()
{
    if (mpMap->mapLabels.contains(mAreaID)) {
        QList<int> deleteList;
        QMapIterator<int, TMapLabel> it(mpMap->mapLabels[mAreaID]);
        while (it.hasNext()) {
            it.next();
            auto labelID = it.key();
            auto label = it.value();
            auto zlevel = static_cast<int>(it.value().pos.z());
            if (zlevel != mOz) {
                continue;
            }
            if (label.highlight) {
                deleteList.push_back(labelID);
            }
        }
        for (int& i : deleteList) {
            mpMap->mapLabels[mAreaID].remove(i);
        }
    }
    update();
}

void T2DMap::slot_editLabel()
{
}

void T2DMap::slot_setPlayerLocation()
{
    if (mMultiSelectionSet.size() != 1) {
        return; // Was <= 1 but that can't be right, and >1 doesn't seem right either
    }

    int _newRoomId = *(mMultiSelectionSet.constBegin());
    if (mpMap->mpRoomDB->getRoom(_newRoomId)) {
        // No need to check it is a DIFFERENT room - that is taken care of by en/dis-abling the control
        mpMap->mRoomIdHash[mpHost->getName()] = _newRoomId;
        mpMap->mNewMove = true;
        TEvent manualSetEvent;
        manualSetEvent.mArgumentList.append(QLatin1String("sysManualLocationSetEvent"));
        manualSetEvent.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
        manualSetEvent.mArgumentList.append(QString::number(_newRoomId));
        manualSetEvent.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
        mpHost->raiseEvent(manualSetEvent);
        update();
    }
}

void T2DMap::slot_userAction(QString uniqueName)
{
    TEvent event;
    QStringList userEvent = mUserActions[uniqueName];
    event.mArgumentList.append(userEvent[0]);
    event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    event.mArgumentList.append(uniqueName);
    event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    QSetIterator<int> itRoom(mMultiSelectionSet);
    if (itRoom.hasNext()) {
        while (itRoom.hasNext()) {
            event.mArgumentList.append(QString::number(itRoom.next()));
            event.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
        }
        mpHost->raiseEvent(event);
    }
    // Unreachable code as had effectively the same test as the previous "if"
    // mMultiSelectionList is now mMultiSelectionSet:
    //    else if( mMultiSelectionList.size() > 0 )
    //    {
    //        event.mArgumentList.append(QString::number(mMultiSelectionList[0]));
    //        event.mArgumentTypeList.append(ARGUMENT_TYPE_NUMBER);
    //        mpHost->raiseEvent( & event );
    //    }
    else {
        event.mArgumentList.append(uniqueName);
        event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
        for (int i = 0; i < userEvent.size(); i++) {
            event.mArgumentList.append(userEvent[i]);
            event.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
        }
        mpHost->raiseEvent(event);
    }
}

void T2DMap::slot_movePosition()
{
    if (!getCenterSelection()) {
        return;
    }

    TRoom* pR_start = mpMap->mpRoomDB->getRoom(mMultiSelectionHighlightRoomId);
    // pR has already been validated by getCenterSelection()

    auto dialog = new QDialog(this);
    auto gridLayout = new QGridLayout;
    dialog->setLayout(gridLayout);
    dialog->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    dialog->setContentsMargins(0, 0, 0, 0);
    auto pLEx = new QLineEdit(dialog);
    auto pLEy = new QLineEdit(dialog);
    auto pLEz = new QLineEdit(dialog);

    pLEx->setText(QString::number(pR_start->x));
    pLEy->setText(QString::number(pR_start->y));
    pLEz->setText(QString::number(pR_start->z));
    QLabel* pLa0 = new QLabel(tr("Move the selection, centered on\nthe highlighted room (%1) to:").arg(mMultiSelectionHighlightRoomId));
    // Record the starting coordinates - can be a help when working out how to move a block of rooms!
    QLabel* pLa1 = new QLabel(tr("x coordinate (was %1):").arg(pR_start->x));
    QLabel* pLa2 = new QLabel(tr("y coordinate (was %1):").arg(pR_start->y));
    QLabel* pLa3 = new QLabel(tr("z coordinate (was %1):").arg(pR_start->z));
    gridLayout->addWidget(pLa0, 0, 0, 1, 2, Qt::AlignCenter);
    gridLayout->addWidget(pLa1, 1, 0, Qt::AlignVCenter | Qt::AlignRight);
    gridLayout->addWidget(pLEx, 1, 1, Qt::AlignVCenter | Qt::AlignLeft);
    gridLayout->addWidget(pLa2, 2, 0, Qt::AlignVCenter | Qt::AlignRight);
    gridLayout->addWidget(pLEy, 2, 1, Qt::AlignVCenter | Qt::AlignLeft);
    gridLayout->addWidget(pLa3, 3, 0, Qt::AlignVCenter | Qt::AlignRight);
    gridLayout->addWidget(pLEz, 3, 1, Qt::AlignVCenter | Qt::AlignLeft);
    auto pButtonBar = new QWidget(dialog);

    auto boxLayout = new QHBoxLayout;
    pButtonBar->setLayout(boxLayout);
    pButtonBar->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));

    auto pB_ok = new QPushButton(pButtonBar);
    pB_ok->setText(tr("OK"));
    boxLayout->addWidget(pB_ok);
    connect(pB_ok, &QAbstractButton::clicked, dialog, &QDialog::accept);

    auto pB_abort = new QPushButton(pButtonBar);
    pB_abort->setText(tr("Cancel"));
    connect(pB_abort, &QAbstractButton::clicked, dialog, &QDialog::reject);
    boxLayout->addWidget(pB_abort);
    gridLayout->addWidget(pButtonBar, 4, 0, 1, 2, Qt::AlignCenter);

    if (dialog->exec() == QDialog::Accepted) {
        int dx = pLEx->text().toInt() - pR_start->x;
        int dy = pLEy->text().toInt() - pR_start->y;
        int dz = pLEz->text().toInt() - pR_start->z;

        mMultiRect = QRect(0, 0, 0, 0);

        QSetIterator<int> itRoom = mMultiSelectionSet;
        while (itRoom.hasNext()) {
            TRoom* room = mpMap->mpRoomDB->getRoom(itRoom.next());
            if (!room) {
                continue;
            }

            room->x += dx;
            room->y += dy;
            room->z += dz;
        }
    }
    repaint();
}


void T2DMap::slot_moveRoom()
{
    mRoomBeingMoved = true;
    setMouseTracking(true);
    mNewMoveAction = true;
}

void T2DMap::slot_setSymbol()
{
    // Now analyses and reports the existing symbols used in ALL the selected
    // rooms if more than once (and sorts by their frequency)
    // Also allows the existing letters to be deleted (by clearing all the displayed
    // letters) as previous code DID NOT - and the Cancel option works as well!
    if (mMultiSelectionSet.empty()) {
        return;
    }

    // First scan and count all the different symbol used
    TRoom* room;
    bool isAtLeastOneRoom = false;
    QHash<QString, unsigned int> usedSymbols;
    QSetIterator<int> itRoom = mMultiSelectionSet;
    QSet<TRoom*> roomPtrsSet;
    while (itRoom.hasNext()) {
        room = mpMap->mpRoomDB->getRoom(itRoom.next());
        if (!room) {
            continue;
        }

        roomPtrsSet.insert(room);
        isAtLeastOneRoom = true;
        if (room->mSymbol.isEmpty()) {
            continue;
        }

        QString thisLetter = QString(room->mSymbol);
        if (!thisLetter.isEmpty()) {
            if (usedSymbols.contains(thisLetter)) {
                (usedSymbols[thisLetter])++;
            } else {
                usedSymbols[thisLetter] = 1;
            }
        }
    }

    if (isAtLeastOneRoom) {
        QString newSymbol;
        bool isOk;
        // TODO: Replace the static QInputDialog::getText(...) calls with
        // one built manually - so that the font used for the QTextLinet entry
        // part can be made to use the same as will be used on the 2DMap itself
        // otherwise can get different glyphs being used between the two which
        // is confusing to the user...
        if (usedSymbols.isEmpty()) {
            // No existing symbols
            newSymbol = QInputDialog::getText(this,
                                              tr("Enter room symbol"),
                                              tr("Enter the symbol to use \n"
                                                 "for this/these room(s):", "", mMultiSelectionSet.size()),
                                              QLineEdit::Normal,
                                              QStringLiteral(""),
                                              & isOk,
                                              Qt::Dialog);
        } else if (usedSymbols.size() == 1) {
            newSymbol = QInputDialog::getText(this,
                                              tr("Enter room symbol"),
                                              ( mMultiSelectionSet.size() > 1 )
                                                  ? tr("The only used symbol is \"%1\" in one or\n"
                                                       "more of the selected rooms, delete this to\n"
                                                       "clear it from all selected rooms or replace\n"
                                                       "with a new symbol to use for all the rooms:")
                                                    .arg(usedSymbols.keys().first())
                                                  : tr("The symbol is \"%1\" in the selected room,\n"
                                                       "delete this to clear the symbol or replace\n"
                                                       "it with a new symbol for this room:")
                                                    .arg(usedSymbols.keys().first()),
                                              QLineEdit::Normal,
                                              usedSymbols.keys().first(),
                                              & isOk,
                                              Qt::Dialog);
        } else {
            QHashIterator<QString, unsigned int> itSymbolUsed(usedSymbols);
            QSet<unsigned int> symbolCountsSet;
            while (itSymbolUsed.hasNext()) {
                itSymbolUsed.next();
                symbolCountsSet.insert(itSymbolUsed.value());
            }
            QList<unsigned int> symbolCountsList = symbolCountsSet.toList();
            if (symbolCountsList.size() > 1) {
                std::sort(symbolCountsList.begin(), symbolCountsList.end());
            }
            QStringList displayStrings;
            for (int i = symbolCountsList.size() - 1; i >= 0; --i) {
                itSymbolUsed.toFront();
                while (itSymbolUsed.hasNext()) {
                    itSymbolUsed.next();
                    if (itSymbolUsed.value() == symbolCountsList.at(i)) {
                        displayStrings.append(tr("%1 {count:%2}",
                                                 "Everything after the first parameter (the '%1') will be removed by processing it as a QRegularExpression programmatically, ensure "
                                                 "the translated text has ` {` immediately after the '%1', and '}' as the very last character, so that the right portion can be "
                                                 "extracted if the user clicks on this item when it is shown in the QComboBox it is put in.")
                                                      .arg(itSymbolUsed.key())
                                                      .arg(QString::number(itSymbolUsed.value())));
                    }
                }
            }
            newSymbol = QInputDialog::getItem(this,                    // QWidget * parent
                                              tr("Enter room symbol"), // const QString & title
                                              tr("Choose an existing symbol from\n"
                                                 "the list (sorted by most commonly\n"
                                                 "used first) or enter one or more\n"
                                                 "new graphemes (\"visible characters\"),\n"
                                                 "to set; or a space to clear; all\n"
                                                 "selected rooms:",
                                                 // Intentional comment to separate two strings!
                                                 "Use line feeds to format text into a reasonable rectangle."), // const QString & label
                                              displayStrings,                                                   // QStringList & items
                                              0,                                                                // int current = 0
                                              true,                                                             // bool editable = true
                                              &isOk,                                                            // bool * ok = 0
                                              Qt::Dialog);
            if (isOk && displayStrings.contains(newSymbol)) {
                // The user has selected one of the existing items in the form
                // "XXXX {count:##}" and we need to chop off the stuff after the
                // "XXXX" to get what is needed:

                QRegularExpression countStripper(QStringLiteral("^(.*) {.*}$"));
                QRegularExpressionMatch match = countStripper.match(newSymbol);
                if (match.hasMatch() && match.lastCapturedIndex() > 0) {
                    // captured(0) is the whole string that matched, which is
                    // not what we want:
                    newSymbol = match.captured(1);
                }
            }
        }

        if (!isOk) {
            return;
        }

        if (newSymbol.isEmpty()) {
            QSetIterator<TRoom*> itRoomPtr(roomPtrsSet);
            while (itRoomPtr.hasNext()) {
                itRoomPtr.next()->mSymbol = QString();
            }
        } else {
            // 8.0 is the maximum supported by all the Qt versions (>= 5.7.0) we
            // handle/use/allow - by normalising the symbol we can ensure that
            // all the entered ones are decomposed and recomposed in a
            // "standard" way and will have the same sequence of codepoints:
            newSymbol = newSymbol.normalized(QString::NormalizationForm_C, QChar::Unicode_8_0);
            QSetIterator<TRoom*> itRoomPtr(roomPtrsSet);
            while (itRoomPtr.hasNext()) {
                itRoomPtr.next()->mSymbol = newSymbol;
            }
        }
        repaint();
    }
}

void T2DMap::slot_setImage()
{
}


void T2DMap::slot_deleteRoom()
{
    mpMap->mpRoomDB->removeRoom(mMultiSelectionSet);
    // mMultiSelectionSet gets cleared as rooms are removed by
    // TRoomDB::removeRoom() so no need to clear it here!
    mMultiRect = QRect(0, 0, 0, 0);
    mMultiSelectionListWidget.clear();
    mMultiSelectionListWidget.hide();
    repaint();
}

void T2DMap::slot_selectRoomColor(QListWidgetItem* pI)
{
    mChosenRoomColor = pI->text().toInt();
}

void T2DMap::slot_defineNewColor()
{
    auto color = QColorDialog::getColor(mpHost->mRed, this);
    if (color.isValid()) {
        auto environmentId = mpMap->customEnvColors.size() + 257 + 16;
        if (mpMap->customEnvColors.contains(environmentId)) {
            // find a new environment ID to use, starting with the latest
            // 'safe' number so the new environment is last in the dialog
            do {
                environmentId++;
            } while (mpMap->customEnvColors.contains(environmentId));
        }

        mpMap->customEnvColors[environmentId] = color;
        slot_changeColor();
    }
    repaint();
}

void T2DMap::slot_changeColor()
{
    auto dialog = new QDialog(this);
    auto vboxLayout = new QVBoxLayout;
    dialog->setLayout(vboxLayout);
    dialog->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    dialog->setContentsMargins(0, 0, 0, 0);
    auto listWidget = new QListWidget(dialog);
    listWidget->setViewMode(QListView::IconMode);
    listWidget->setResizeMode(QListView::Adjust);

    connect(listWidget, &QListWidget::itemDoubleClicked, dialog, &QDialog::accept);
    connect(listWidget, &QListWidget::itemClicked, this, &T2DMap::slot_selectRoomColor);
    listWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(listWidget, &QListWidget::customContextMenuRequested, this, [=]() {
        QMenu menu;
        menu.addAction(tr("Delete color", "Deletes an environment colour"), this, [=]() {
            auto selectedItem = listWidget->takeItem(listWidget->currentRow());
            auto colour = selectedItem->text();

            mpMap->customEnvColors.remove(colour.toInt());
            repaint();
        });

        menu.exec(QCursor::pos());
    });

    vboxLayout->addWidget(listWidget);
    auto pButtonBar = new QWidget(dialog);

    auto hboxLayout = new QHBoxLayout;
    pButtonBar->setLayout(hboxLayout);
    pButtonBar->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
    auto pB_newColor = new QPushButton(pButtonBar);
    pB_newColor->setText(tr("Define new color"));

    connect(pB_newColor, &QAbstractButton::clicked, dialog, &QDialog::reject);
    connect(pB_newColor, &QAbstractButton::clicked, this, &T2DMap::slot_defineNewColor);

    hboxLayout->addWidget(pB_newColor);

    auto pB_ok = new QPushButton(pButtonBar);
    pB_ok->setText(tr("OK"));
    hboxLayout->addWidget(pB_ok);
    connect(pB_ok, &QAbstractButton::clicked, dialog, &QDialog::accept);

    auto pB_abort = new QPushButton(pButtonBar);
    pB_abort->setText(tr("Cancel"));
    connect(pB_abort, &QAbstractButton::clicked, dialog, &QDialog::reject);
    hboxLayout->addWidget(pB_abort);
    vboxLayout->addWidget(pButtonBar);

    QMapIterator<int, QColor> it(mpMap->customEnvColors);
    while (it.hasNext()) {
        it.next();
        QColor c;
        c = it.value();
        auto pI = new QListWidgetItem(listWidget);
        QPixmap pix = QPixmap(50, 50);
        pix.fill(c);
        QIcon mi(pix);
        pI->setIcon(mi);
        pI->setText(QString::number(it.key()));
        listWidget->addItem(pI);
    }
    listWidget->sortItems();

    if (dialog->exec() == QDialog::Accepted && mpMap->customEnvColors.contains(mChosenRoomColor)) {
        // Only proceed if OK - "Cancel" now prevents change AND check for a valid
        // color here rather than inside the room change loop as before (only test
        // once rather than for each room)
        mMultiRect = QRect(0, 0, 0, 0);
        QSetIterator<int> itSelectedRoom(mMultiSelectionSet);
        while (itSelectedRoom.hasNext()) {
            TRoom* room = mpMap->mpRoomDB->getRoom(itSelectedRoom.next());
            if (room) {
                room->environment = mChosenRoomColor;
            }
        }

        update();
    }
}

void T2DMap::slot_spread()
{
    if (mMultiSelectionSet.size() < 2) { // nothing to do!
        return;
    }

    TRoom* pR_centerRoom = mpMap->mpRoomDB->getRoom(mMultiSelectionHighlightRoomId);
    if (!pR_centerRoom) {
        return;
    }

    // Move the dialog down to here so it doesn't fire up for some already
    // determined to be null (no change) case, also handle "Cancel" being pressed
    bool isOk = false;
    int spread = QInputDialog::getInt(this,
                                      tr("Spread out rooms"),
                                      tr("Increase the spacing of\n"
                                         "the selected rooms,\n"
                                         "centered on the\n"
                                         "highlighted room by a\n"
                                         "factor of:"),
                                      5,          // Initial value
                                      1,          // Minimum value
                                      2147483647, // Maximum value
                                      1,          // Step
                                      &isOk);
    if (spread == 1 || !isOk) {
        return;
    }

    mMultiRect = QRect(0, 0, 0, 0);
    int dx = pR_centerRoom->x * (1 - spread);
    int dy = pR_centerRoom->y * (1 - spread);
    QSetIterator<int> itSelectionRoom = mMultiSelectionSet;
    while (itSelectionRoom.hasNext()) {
        TRoom* pMovingR = mpMap->mpRoomDB->getRoom(itSelectionRoom.next());
        if (!pMovingR) {
            continue;
        }

        pMovingR->x *= spread;
        pMovingR->y *= spread;
        pMovingR->x += dx;
        pMovingR->y += dy;
        QMapIterator<QString, QList<QPointF>> itCustomLine(pMovingR->customLines);
        QMap<QString, QList<QPointF>> newCustomLinePointsMap;
        while (itCustomLine.hasNext()) {
            itCustomLine.next();
            QList<QPointF> customLinePoints = itCustomLine.value();
            for (auto& customLinePoint : customLinePoints) {
                QPointF movingPoint = customLinePoint;
                customLinePoint.setX(static_cast<float>(movingPoint.x() * spread + dx));
                customLinePoint.setY(static_cast<float>(movingPoint.y() * spread + dy));
            }
            newCustomLinePointsMap.insert(itCustomLine.key(), customLinePoints);
        }
        pMovingR->customLines = newCustomLinePointsMap;
        pMovingR->calcRoomDimensions();
    }
    repaint();
}

void T2DMap::slot_shrink()
{
    if (mMultiSelectionSet.size() < 2) { // nothing to do!
        return;
    }

    TRoom* pR_centerRoom = mpMap->mpRoomDB->getRoom(mMultiSelectionHighlightRoomId);
    if (!pR_centerRoom) {
        return;
    }

    // Move the dialog down to here so it doesn't fire up for some already
    // determined to be null (no change) case, also handle "Cancel" being pressed
    bool isOk = false;
    int spread = QInputDialog::getInt(this,
                                      tr("Shrink in rooms"),
                                      tr("Decrease the spacing of\n"
                                         "the selected rooms,\n"
                                         "centered on the\n"
                                         "highlighted room by a\n"
                                         "factor of:"),
                                      5,          // Initial value
                                      1,          // Minimum value
                                      2147483647, // Maximum value
                                      1,          // Step
                                      &isOk);
    if (spread == 1 || !isOk) {
        return;
    }

    mMultiRect = QRect(0, 0, 0, 0);
    int dx = pR_centerRoom->x * (1 - 1 / spread);
    int dy = pR_centerRoom->y * (1 - 1 / spread);

    QSetIterator<int> itSelectionRoom(mMultiSelectionSet);
    while (itSelectionRoom.hasNext()) {
        TRoom* pMovingR = mpMap->mpRoomDB->getRoom(itSelectionRoom.next());
        if (!pMovingR) {
            continue;
        }
        pMovingR->x /= spread;
        pMovingR->y /= spread;
        pMovingR->x += dx;
        pMovingR->y += dy;
        QMapIterator<QString, QList<QPointF>> itCustomLine(pMovingR->customLines);
        QMap<QString, QList<QPointF>> newCustomLinePointsMap;
        while (itCustomLine.hasNext()) {
            itCustomLine.next();
            QList<QPointF> customLinePoints = itCustomLine.value();
            for (auto& customLinePoint : customLinePoints) {
                QPointF movingPoint = customLinePoint;
                customLinePoint.setX(static_cast<float>(movingPoint.x() / spread + dx));
                customLinePoint.setY(static_cast<float>(movingPoint.y() / spread + dy));
            }
            newCustomLinePointsMap.insert(itCustomLine.key(), customLinePoints);
        }
        pMovingR->customLines = newCustomLinePointsMap;
        pMovingR->calcRoomDimensions();
    }
    repaint();
}

void T2DMap::slot_setExits()
{
    if (mMultiSelectionSet.empty()) {
        return;
    }
    if (mpMap->mpRoomDB->getRoom(mMultiSelectionHighlightRoomId)) {
        auto pD = new dlgRoomExits(mpHost, this);
        pD->init(mMultiSelectionHighlightRoomId);
        pD->show();
        pD->raise();
    }
}


void T2DMap::slot_setUserData()
{
}

void T2DMap::slot_lockRoom()
{
    if (mMultiSelectionSet.empty()) {
        return;
    }

    mMultiRect = QRect(0, 0, 0, 0);
    QSetIterator<int> itSelectedRoom(mMultiSelectionSet);
    while (itSelectedRoom.hasNext()) {
        TRoom* room = mpMap->mpRoomDB->getRoom(itSelectedRoom.next());
        if (room) {
            room->isLocked = true;
            mpMap->mMapGraphNeedsUpdate = true;
        }
    }
}

void T2DMap::slot_unlockRoom()
{
    if (mMultiSelectionSet.empty()) {
        return;
    }

    mMultiRect = QRect(0, 0, 0, 0);
    QSetIterator<int> itSelectedRoom(mMultiSelectionSet);
    while (itSelectedRoom.hasNext()) {
        TRoom* room = mpMap->mpRoomDB->getRoom(itSelectedRoom.next());
        if (room) {
            room->isLocked = false;
            mpMap->mMapGraphNeedsUpdate = true;
        }
    }
}

void T2DMap::slot_setRoomWeight()
{
    if (mMultiSelectionSet.isEmpty()) {
        return;
    }

    // First scan and count all the different weights used
    QMap<uint, uint> usedWeights; // key is weight, value is count of uses
    QSetIterator<int> itSelectedRoom = mMultiSelectionSet;
    TRoom* room;
    while (itSelectedRoom.hasNext()) {
        room = mpMap->mpRoomDB->getRoom(itSelectedRoom.next());
        if (!room) {
            continue;
        }

        int roomWeight = room->getWeight();
        if (roomWeight > 0) {
            if (usedWeights.contains(roomWeight)) {
                usedWeights[roomWeight] += 1;
            } else {
                usedWeights[roomWeight] = 1;
            }
        }
    }

    int newWeight = 1;
    bool isOk = false;
    // Choose most appropriate weight dialog on number of rooms selected and
    // how many of them have different weights already:
    if (mMultiSelectionSet.size() == 1) { // Just one room selected
        newWeight = QInputDialog::getInt(this,
                                         tr("Enter room weight"),
                                         tr("Enter new roomweight\n"
                                            "(= travel time), minimum\n"
                                            "(and default) is 1:",
                                            "Use line feeds to format text into a reasonable rectangle."),
                                         usedWeights.keys().first(),
                                         1,
                                         2147483647,
                                         1,
                                         &isOk);
    } else { // More than one room selected
        if (usedWeights.size() == 1) {
            newWeight = QInputDialog::getInt(this,
                                             tr("Enter room weight"),
                                             tr("Enter new roomweight\n"
                                                "(= travel time) for all\n"
                                                "selected rooms, minimum\n"
                                                "(and default) is 1 and\n"
                                                "the only current value\n"
                                                "used is:",
                                                "Use line feeds to format text into a reasonable rectangle."),
                                             usedWeights.keys().first(),
                                             1,
                                             2147483647,
                                             1,
                                             &isOk);
        } else {
            QMapIterator<uint, uint> itWeightsUsed = usedWeights;
            // Obtain a set of "used" weights
            QSet<uint> weightCountsSet;
            while (itWeightsUsed.hasNext()) {
                itWeightsUsed.next();
                weightCountsSet.insert(itWeightsUsed.value());
            }
            // Obtains a list of those weights sorted in ascending count of used
            QList<uint> weightCountsList = weightCountsSet.toList();
            if (weightCountsList.size() > 1) {
                std::sort(weightCountsList.begin(), weightCountsList.end());
            }
            // Build a list of the "used" weights in descending count of use
            QStringList displayStrings;
            for (int i = weightCountsList.size() - 1; i >= 0; --i) {
                itWeightsUsed.toFront();
                while (itWeightsUsed.hasNext()) {
                    itWeightsUsed.next();
                    if (itWeightsUsed.value() == weightCountsList.at(i)) {
                        if (itWeightsUsed.key() == 1) { // Indicate the "default" value which is unity weight
                            displayStrings.append(tr("%1 {count:%2, default}").arg(QString::number(itWeightsUsed.key()), QString::number(itWeightsUsed.value())));
                        } else {
                            displayStrings.append(tr("%1 {count:%2}").arg(QString::number(itWeightsUsed.key()), QString::number(itWeightsUsed.value())));
                        }
                    }
                }
            }
            if (!usedWeights.contains(1)) { // If unity weight was not used insert it at end of list
                displayStrings.append(tr("1 {count 0, default}"));
            }
            QString newWeightText = QInputDialog::getItem(this,                    // QWidget * parent
                                                          tr("Enter room weight"), // const QString & title
                                                          tr("Choose an existing\n"
                                                             "roomweight (= travel\n"
                                                             "time) from the list\n"
                                                             "(sorted by most commonly\n"
                                                             "used first) or enter a\n"
                                                             "new (positive) integer\n"
                                                             "value for all selected\n"
                                                             "rooms:",
                                                             "Use line feeds to format text into a reasonable rectangle."), // const QString & label
                                                          displayStrings,                                                   // QStringList & items
                                                          0,                                                                // int current = 0, last value in list
                                                          true,                                                             // bool editable = true
                                                          &isOk,                                                            // bool * ok = 0
                                                          nullptr,                                                                // Qt::WindowFlags flags = 0
                                                          Qt::ImhDigitsOnly);                                               // Qt::InputMethodHints inputMethodHints = Qt::ImhNone
            newWeight = 1;
            if (isOk) { // Don't do anything if cancel was pressed
                if (newWeightText.toInt() > 0) {
                    newWeight = newWeightText.toInt();
                } else {
                    isOk = false; // Prevent any change if the value is not reasonable
                }
            }
        }
    }

    if (isOk && newWeight > 0) { // Don't proceed if cancel was pressed or the value is not valid
        itSelectedRoom.toFront();
        while (itSelectedRoom.hasNext()) {
            room = mpMap->mpRoomDB->getRoom(itSelectedRoom.next());
            if (!room) {
                continue;
            }

            room->setWeight(newWeight);
        }
        mpMap->mMapGraphNeedsUpdate = true;
        repaint();
    }
}

void T2DMap::slot_loadMap() {
    if (!mpHost) {
        return;
    }

    QString fileName = QFileDialog::getOpenFileName(
                           this,
                           tr("Load Mudlet map"),
                           mudlet::getMudletPath(mudlet::profileMapsPath, mpHost->getName()),
                           tr("Mudlet map (*.dat);;Xml map data (*.xml);;Any file (*)",
                              "Do not change extensions (in braces) as they are used programmatically"));
    if (fileName.isEmpty()) {
        return;
    }

    if (fileName.endsWith(QStringLiteral(".xml"), Qt::CaseInsensitive)) {
        mpHost->mpConsole->importMap(fileName);
    } else {
        mpHost->mpConsole->loadMap(fileName);
    }
}

void T2DMap::slot_newMap()
{
    if (!mpHost) {
        return;
    }

    auto roomID = mpHost->mpMap->createNewRoomID();

    if (!mpHost->mpMap->addRoom(roomID)) {
        return;
    }

    mpHost->mpMap->setRoomArea(roomID, -1, false);
    mpHost->mpMap->setRoomCoordinates(roomID, 0, 0, 0);
    mpHost->mpMap->mMapGraphNeedsUpdate = true;

    auto room = mpHost->mpMap->mpRoomDB->getRoom(roomID);
    mpHost->mpMap->mRoomIdHash[mpHost->getName()] = roomID;
    mpHost->mpMap->mNewMove = true;
    if (mpHost->mpMap->mpM) {
        mpHost->mpMap->mpM->update();
    }

    if (mpHost->mpMap->mpMapper->mp2dMap) {
        mpHost->mpMap->mpMapper->mp2dMap->isCenterViewCall = true;
        mpHost->mpMap->mpMapper->mp2dMap->update();
        mpHost->mpMap->mpMapper->mp2dMap->isCenterViewCall = false;
        mpHost->mpMap->mpMapper->resetAreaComboBoxToPlayerRoomArea();
    }
}

void T2DMap::slot_setArea()
{
    QUiLoader loader;

    QFile file(":/ui/set_room_area.ui");
    file.open(QFile::ReadOnly);
    auto* set_room_area_dialog = qobject_cast<QDialog*>(loader.load(&file, this));
    file.close();
    if (!set_room_area_dialog) {
        return;
    }
    arealist_combobox = set_room_area_dialog->findChild<QComboBox*>("arealist_combobox");
    if (!arealist_combobox) {
        return;
    }

    QMapIterator<int, QString> it(mpMap->mpRoomDB->getAreaNamesMap());
    while (it.hasNext()) {
        it.next();
        int areaID = it.key();
        if (areaID > 0) {
            arealist_combobox->addItem(QStringLiteral("%1 (%2)").arg(it.value(), QString::number(areaID)), QVariant(areaID));
        }
    }

    if (set_room_area_dialog->exec() == QDialog::Rejected) { // Don't proceed if "cancel" was pressed
        return;
    }

    int newAreaId = arealist_combobox->itemData(arealist_combobox->currentIndex()).toInt();
    mMultiRect = QRect(0, 0, 0, 0);
    QSetIterator<int> itSelectedRoom = mMultiSelectionSet;
    while (itSelectedRoom.hasNext()) {
        int currentRoomId = itSelectedRoom.next();
        if (itSelectedRoom.hasNext()) { // NOT the last room in set -  so defer some area related recalculations
            mpMap->setRoomArea(currentRoomId, newAreaId, true);
        } else {
            // Is the LAST room, so be careful to do all that is needed to clean
            // up the affected areas (triggered by last "false" argument in next
            // line)...
            if (!(mpMap->setRoomArea(currentRoomId, newAreaId, false))) {
                // Failed on the last of multiple room area move so do the missed
                // out recalculations for the dirtied areas
                QSetIterator<TArea*> itpArea = mpMap->mpRoomDB->getAreaPtrList().toSet();
                while (itpArea.hasNext()) {
                    TArea* pArea = itpArea.next();
                    if (pArea->mIsDirty) {
                        pArea->determineAreaExits();
                        pArea->calcSpan();
                        pArea->mIsDirty = false;
                    }
                }
            }
        }
    }
    repaint();
}


void T2DMap::mouseMoveEvent(QMouseEvent* event)
{
    if (mpMap->mLeftDown && !mpMap->m2DPanMode && event->modifiers().testFlag(Qt::AltModifier)) {
        mpMap->m2DPanXStart = event->x();
        mpMap->m2DPanYStart = event->y();
        mpMap->m2DPanMode = true;
    }
    if (mpMap->m2DPanMode && !event->modifiers().testFlag(Qt::AltModifier)) {
        mpMap->m2DPanMode = false;
        mpMap->mLeftDown = false;
    }
    if (mpMap->m2DPanMode) {
        int x = event->x();
        int y = height() - event->y();
        if ((mpMap->m2DPanXStart - x) > 1) {
            shiftRight();
            mpMap->m2DPanXStart = x;
        } else if ((mpMap->m2DPanXStart - x) < -1) {
            shiftLeft();
            mpMap->m2DPanXStart = x;
        }
        if ((mpMap->m2DPanYStart - y) > 1) {
            shiftDown();
            mpMap->m2DPanYStart = y;
        } else if ((mpMap->m2DPanYStart - y) < -1) {
            shiftUp();
            mpMap->m2DPanYStart = y;
        }
        return;
    }

    if (mCustomLineSelectedRoom != 0 && mCustomLineSelectedPoint >= 0) {
        TRoom* room = mpMap->mpRoomDB->getRoom(mCustomLineSelectedRoom);
        if (room) {
            if (room->customLines.contains(mCustomLineSelectedExit)) {
                if (room->customLines[mCustomLineSelectedExit].size() > mCustomLineSelectedPoint) {
                    float mx = (event->pos().x() / mRoomWidth) + mOx - (xspan / 2.0);
                    float my = (yspan / 2.0) - (event->pos().y() / mRoomHeight) - mOy;
                    QPointF pc = QPointF(mx, my);
                    room->customLines[mCustomLineSelectedExit][mCustomLineSelectedPoint] = pc;
                    room->calcRoomDimensions();
                    repaint();
                    return;
                }
            }
        }
    }

    mCustomLineSelectedPoint = -1;

    //FIXME:
    if (mLabelHighlighted) {
        if (mpMap->mapLabels.contains(mAreaID)) {
            QMapIterator<int, TMapLabel> it(mpMap->mapLabels[mAreaID]);
            while (it.hasNext()) {
                it.next();
                auto labelID = it.key();
                auto label = it.value();

                if (label.pos.z() != mOz) {
                    continue;
                }
                if (!label.highlight) {
                    continue;
                }
                int mx = qRound((event->pos().x() / mRoomWidth) + mOx -(xspan / 2.0));
                int my = qRound((yspan / 2.0) - (event->pos().y() / mRoomHeight) - mOy);
                QVector3D p = QVector3D(mx, my, mOz);
                mpMap->mapLabels[mAreaID][labelID].pos = p;
            }
        }
        update();
    } else {
        mMoveLabel = false;
    }

    if ((mMultiSelection && !mRoomBeingMoved) || mSizeLabel) {
        //    (The drag to select (or size) rectangle is being actively resized and rooms are not being moved)
        // OR (We are sizing up a label)
        if (mNewMoveAction) {
            mMultiRect = QRect(event->pos(), event->pos());
            mNewMoveAction = false;
        } else {
            mMultiRect.setBottomLeft(event->pos());
        }

        if (!mpMap->mpRoomDB->getRoom(mRoomID)) {
            return;
        }
        TArea* pArea = mpMap->mpRoomDB->getArea(mAreaID);
        if (!pArea) {
            return;
        }

        float fx = xspan / 2.0 * mRoomWidth - mRoomWidth * mOx;
        float fy = yspan / 2.0 * mRoomHeight - mRoomHeight * mOy;

        if (!mSizeLabel) { // NOT sizing a label
            mMultiSelectionSet.clear();
            QSetIterator<int> itSelectedRoom(pArea->getAreaRooms());
            while (itSelectedRoom.hasNext()) {
                int currentRoomId = itSelectedRoom.next();
                TRoom* room = mpMap->mpRoomDB->getRoom(currentRoomId);
                if (!room) {
                    continue;
                }
                int rx = qRound(room->x      * mRoomWidth + fx);
                int ry = qRound(room->y * -1 * mRoomHeight + fy);
                int rz = room->z;

                // copy rooms on all z-levels if the shift key is being pressed
                // CHECK: Consider adding z-level to multi-selection Widget?
                if (rz != mOz && !(event->modifiers().testFlag(Qt::ShiftModifier))) {
                    continue;
                }

                QRectF dr;
                if (pArea->gridMode) {
                    dr = QRectF(rx - (mRoomWidth / 2.0), ry - (mRoomHeight / 2.0), mRoomWidth, mRoomHeight);
                } else {
                    dr = QRectF(rx - ((mRoomWidth * rSize) / 2.0), ry - ((mRoomHeight * rSize) / 2.0), mRoomWidth * rSize, mRoomHeight * rSize);
                }
                if (mMultiRect.contains(dr)) {
                    mMultiSelectionSet.insert(currentRoomId);
                }
            }
            switch (mMultiSelectionSet.size()) {
            case 0:
                mMultiSelectionHighlightRoomId = 0;
                break;
            case 1:
                mMultiSelectionHighlightRoomId = mMultiSelectionSet.toList().first();
                break;
            default:
                getCenterSelection(); // Sets mMultiSelectionHighlightRoomId to (a) central room
            }

            if (mMultiSelectionSet.size() > 1) {
                // We don't want to cause calls to slot_roomSelectionChanged() here!
                mMultiSelectionListWidget.blockSignals(true);
                // Save sorting state before we switch it off
                mIsSelectionSorting = mMultiSelectionListWidget.isSortingEnabled();
                mIsSelectionSortByNames = (mMultiSelectionListWidget.sortColumn() == 1);
                mMultiSelectionListWidget.clear();
                // Do NOT sort whilst inserting items!
                mMultiSelectionListWidget.setSortingEnabled(false);
                QSetIterator<int> itRoom = mMultiSelectionSet;
                mIsSelectionUsingNames = false;
                while (itRoom.hasNext()) {
                    auto item = new QTreeWidgetItem;
                    int multiSelectionRoomId = itRoom.next();
                    item->setText(0, QStringLiteral("%1").arg(multiSelectionRoomId, 7));
                    item->setTextAlignment(0, Qt::AlignRight);
                    TRoom* pR_multiSelection = mpMap->mpRoomDB->getRoom(multiSelectionRoomId);
                    if (pR_multiSelection) {
                        QString multiSelectionRoomName = pR_multiSelection->name;
                        if (!multiSelectionRoomName.isEmpty()) {
                            item->setText(1, multiSelectionRoomName);
                            item->setTextAlignment(1, Qt::AlignLeft);
                            mIsSelectionUsingNames = true;
                        }
                    }
                    mMultiSelectionListWidget.addTopLevelItem(item);
                }
                mMultiSelectionListWidget.setColumnHidden(1, !mIsSelectionUsingNames);
                // Can't sort if nothing to sort on, switch to sorting by room number
                if ((!mIsSelectionUsingNames) && mIsSelectionSortByNames && mIsSelectionSorting) {
                    mIsSelectionSortByNames = false;
                }
                mMultiSelectionListWidget.sortByColumn(mIsSelectionSortByNames ? 1 : 0);
                mMultiSelectionListWidget.setSortingEnabled(mIsSelectionSorting);
                resizeMultiSelectionWidget();
                mMultiSelectionListWidget.selectAll();
                mMultiSelectionListWidget.blockSignals(false);
                mMultiSelectionListWidget.show();
            } else {
                mMultiSelectionListWidget.hide();
            }
        }

        update();
        return;
    }

    if (mRoomBeingMoved && !mSizeLabel && !mMultiSelectionSet.isEmpty()) {
        mMultiRect = QRect(0, 0, 0, 0);
        if (!mpMap->mpRoomDB->getRoom(mRoomID)) {
            return;
        }
        TArea* pArea = mpMap->mpRoomDB->getArea(mAreaID);
        if (!pArea) {
            return;
        }

        if (!getCenterSelection()) {
            return;
        }

        TRoom* room = mpMap->mpRoomDB->getRoom(mMultiSelectionHighlightRoomId);
        if (!room) {
            return;
        }

        int dx = qRound((event->pos().x() / mRoomWidth) + mOx - (xspan / 2.0) + 1.0) - room->x;
        int dy = qRound((yspan / 2.0) - (event->pos().y() / mRoomHeight) - mOy - 1.0) - room->y;
        QSetIterator<int> itRoom = mMultiSelectionSet;
        while (itRoom.hasNext()) {
            room = mpMap->mpRoomDB->getRoom(itRoom.next());
            if (room) {
                room->x += dx;
                room->y += dy;
                room->z = mOz; // allow groups to be moved to a different z-level with the map editor

                QMapIterator<QString, QList<QPointF>> itk(room->customLines);
                QMap<QString, QList<QPointF>> newMap;
                while (itk.hasNext()) {
                    itk.next();
                    QList<QPointF> _pL = itk.value();
                    for (auto& point : _pL) {
                        QPointF op = point;
                        point.setX(static_cast<float>(op.x() + dx));
                        point.setY(static_cast<float>(op.y() + dy));
                    }
                    newMap.insert(itk.key(), _pL);
                }
                room->customLines = newMap;
                room->calcRoomDimensions();
            }
        }
        repaint();
    }
}

// Replacement for getTopLeftCenter - determines a room closest to geometrical
// mean of all the room selected, the result is stored in the class member
// mMultiSelectionHighlightRoomId and this returns true on successfully finding
// such a room
bool T2DMap::getCenterSelection()
{
    mMultiSelectionHighlightRoomId = 0;
    if (mMultiSelectionSet.isEmpty()) {
        return false;
    }

    QSetIterator<int> itRoom = mMultiSelectionSet;
    float mean_x = 0.0;
    float mean_y = 0.0;
    float mean_z = 0.0;
    uint processedRoomCount = 0;
    while (itRoom.hasNext()) {
        int currentRoomId = itRoom.next();
        TRoom* room = mpMap->mpRoomDB->getRoom(currentRoomId);
        if (!room) {
            continue;
        }

        mean_x += (static_cast<float>(room->x - mean_x)) / ++processedRoomCount;
        mean_y += (static_cast<float>(room->y - mean_y)) / processedRoomCount;
        mean_z += (static_cast<float>(room->z - mean_z)) / processedRoomCount;
    }

    if (processedRoomCount) {
        itRoom.toFront();
        float closestSquareDistance = -1.0;
        while (itRoom.hasNext()) {
            int currentRoomId = itRoom.next();
            TRoom* room = mpMap->mpRoomDB->getRoom(currentRoomId);
            if (!room) {
                continue;
            }

            QVector3D meanToRoom(static_cast<float>(room->x) - mean_x, static_cast<float>(room->y) - mean_y, static_cast<float>(room->z) - mean_z);
            if (closestSquareDistance < -0.5) {
                // Don't use an equality to zero test, we are using floats so
                // need to allow for a little bit of fuzzzyness!
                closestSquareDistance = meanToRoom.lengthSquared();
                mMultiSelectionHighlightRoomId = currentRoomId;
            } else {
                float currentRoomSquareDistance = meanToRoom.lengthSquared();
                if (closestSquareDistance > currentRoomSquareDistance) {
                    closestSquareDistance = currentRoomSquareDistance;
                    mMultiSelectionHighlightRoomId = currentRoomId;
                }
            }
        }
        return true;
    } else {
        return false;
    }
}

void T2DMap::exportAreaImage(int id)
{
    paintMap();
}

void T2DMap::wheelEvent(QWheelEvent* e)
{
    // If the mouse wheel is scrolling up and down through the
    // mMultiSelectionListWidget the wheelevents from that get passed up to here
    // when the end of the list is reached (i.e when it rejects those events)
    // - so that the mapper window doesn't THEN zoom in or out, swallow the
    // events, i.e. accept() them and return before hitting the zoom altering
    // code that follows.
    // However the event "pos()" depends on the widget it came from so we have
    // to use "globalPos()" instead and see how it lies in relation to the child
    // widget:
    QRect selectionListWidgetGlobalRect = QRect(mapToGlobal(mMultiSelectionListWidget.frameRect().topLeft()), mapToGlobal(mMultiSelectionListWidget.frameRect().bottomRight()));
    if (mMultiSelectionListWidget.isVisible() && selectionListWidgetGlobalRect.contains(e->globalPos())) {
        e->accept();
        return;
    }

    if (!(mpMap->mpRoomDB->getRoom(mRoomID) && mpMap->mpRoomDB->getArea(mAreaID))) {
        return;
    }

    // int delta = e->delta() / 8 / 15; // Deprecated in Qt 5.x ...!
    int delta = e->angleDelta().y() / (8 * 15);
    if (e->modifiers() & Qt::ControlModifier) { // Increase rate 10-fold if control key down - it makes scrolling through
                                                // a large number of items in a listwidget's contents easier AND this make it
                                                // easier to zoom in and out on LARGE area maps
        delta *= 10;
    }
    if (delta != 0) {
        mPick = false;
        int oldZoom = xyzoom;
        xyzoom = qMax(3, xyzoom + delta);
        if (oldZoom != xyzoom) {
            flushSymbolPixmapCache();
            update();
        }
        e->accept();
        return;
    }
    e->ignore();
    return;
}

void T2DMap::setMapZoom(int zoom)
{
    int oldZoom = xyzoom;
    xyzoom = qMax(3, zoom);
    if (oldZoom != xyzoom) {
        flushSymbolPixmapCache();
        update();
    }
}

void T2DMap::setRoomSize(double f)
{
    rSize = f;
    if (mpHost) {
        mpHost->mRoomSize = f;
    }
    flushSymbolPixmapCache();
    update();
}

void T2DMap::setExitSize(double f)
{
    eSize = f;
    if (mpHost) {
        mpHost->mLineSize = f;
    }
}

void T2DMap::slot_setCustomLine()
{
    if (mMultiSelectionSet.isEmpty()) {
        return;
    }
    TRoom* room = mpMap->mpRoomDB->getRoom(mMultiSelectionHighlightRoomId);
    if (!room) {
        return;
    }

    if (mpCustomLinesDialog) {
        // Refuse to create another instance if one is already present!
        // Just show it...
        mpCustomLinesDialog->raise();
        return;
    }

    QUiLoader loader;

    QFile file(":/ui/custom_lines.ui");
    file.open(QFile::ReadOnly);
    auto* dialog = qobject_cast<QDialog*>(loader.load(&file, this));
    file.close();
    if (!dialog) {
        return;
    }
    dialog->setWindowIcon(QIcon(QStringLiteral(":/icons/mudlet_custom_exit.png")));
    mCustomLinesRoomFrom = mMultiSelectionHighlightRoomId;
    mCustomLinesRoomTo = 0;
    mCustomLinesRoomExit = "";
    auto* button = dialog->findChild<QPushButton*>("nw");
    auto* specialExits = dialog->findChild<QTreeWidget*>("specialExits");
    mpCurrentLineStyle = dialog->findChild<QComboBox*>("lineStyle");
    mpCurrentLineColor = dialog->findChild<QPushButton*>("lineColor");
    mpCurrentLineArrow = dialog->findChild<QCheckBox*>("arrow");
    if (!button || !specialExits || !mpCurrentLineColor || !mpCurrentLineStyle || !mpCurrentLineArrow) {
        qWarning(R"(T2DMap::slot_setCustomLine() ERROR: failed to find "nw" exit line button or another element of the dialog!)");
        return;
    } else if (room->getNorthwest() <= 0) {
        button->setCheckable(false);
        button->setDisabled(true);
    } else {
        button->setCheckable(true);
        button->setChecked(room->customLines.contains(QStringLiteral("nw")));
        connect(button, &QAbstractButton::clicked, this, &T2DMap::slot_setCustomLine2);
    }

    button = dialog->findChild<QPushButton*>("n");
    if (!button) {
        qWarning(R"(T2DMap::slot_setCustomLine() ERROR: failed to find "n" exit line button!)");
        return;
    } else if (room->getNorth() <= 0) {
        button->setDisabled(true);
        button->setCheckable(false);
    } else {
        button->setCheckable(true);
        button->setChecked(room->customLines.contains(QStringLiteral("n")));
        connect(button, &QAbstractButton::clicked, this, &T2DMap::slot_setCustomLine2);
    }

    button = dialog->findChild<QPushButton*>("ne");
    if (!button) {
        qWarning(R"(T2DMap::slot_setCustomLine() ERROR: failed to find "ne" exit line button!)");
        return;
    } else if (room->getNortheast() <= 0) {
        button->setDisabled(true);
        button->setCheckable(false);
    } else {
        button->setCheckable(true);
        button->setChecked(room->customLines.contains(QStringLiteral("ne")));
        connect(button, &QAbstractButton::clicked, this, &T2DMap::slot_setCustomLine2);
    }

    button = dialog->findChild<QPushButton*>("up");
    if (!button) {
        qWarning(R"(T2DMap::slot_setCustomLine() ERROR: failed to find "up" exit line button!)");
        return;
    } else if (room->getUp() <= 0) {
        button->setDisabled(true);
        button->setCheckable(false);
    } else {
        button->setCheckable(true);
        button->setChecked(room->customLines.contains(QStringLiteral("up")));
        connect(button, &QAbstractButton::clicked, this, &T2DMap::slot_setCustomLine2);
    }

    button = dialog->findChild<QPushButton*>("w");
    if (!button) {
        qWarning(R"(T2DMap::slot_setCustomLine() ERROR: failed to find "w" exit line button!)");
        return;
    } else if (room->getWest() <= 0) {
        button->setCheckable(false);
        button->setDisabled(true);
    } else {
        button->setCheckable(true);
        button->setChecked(room->customLines.contains(QStringLiteral("w")));
        connect(button, &QAbstractButton::clicked, this, &T2DMap::slot_setCustomLine2);
    }

    button = dialog->findChild<QPushButton*>("e");
    if (!button) {
        qWarning(R"(T2DMap::slot_setCustomLine() ERROR: failed to find "e" exit line button!)");
        return;
    } else if (room->getEast() <= 0) {
        button->setDisabled(true);
        button->setCheckable(false);
    } else {
        button->setCheckable(true);
        button->setChecked(room->customLines.contains(QStringLiteral("e")));
        connect(button, &QAbstractButton::clicked, this, &T2DMap::slot_setCustomLine2);
    }

    button = dialog->findChild<QPushButton*>("down");
    if (!button) {
        qWarning(R"(T2DMap::slot_setCustomLine() ERROR: failed to find "down" exit line button!)");
        return;
    } else if (room->getDown() <= 0) {
        button->setDisabled(true);
        button->setCheckable(false);
    } else {
        button->setCheckable(true);
        button->setChecked(room->customLines.contains(QStringLiteral("down")));
        connect(button, &QAbstractButton::clicked, this, &T2DMap::slot_setCustomLine2);
    }

    button = dialog->findChild<QPushButton*>("sw");
    if (!button) {
        qWarning(R"(T2DMap::slot_setCustomLine() ERROR: failed to find "sw" exit line button!)");
        return;
    } else if (room->getSouthwest() <= 0) {
        button->setDisabled(true);
        button->setCheckable(false);
    } else {
        button->setCheckable(true);
        button->setChecked(room->customLines.contains(QStringLiteral("sw")));
        connect(button, &QAbstractButton::clicked, this, &T2DMap::slot_setCustomLine2);
    }

    button = dialog->findChild<QPushButton*>("s");
    if (!button) {
        qWarning(R"(T2DMap::slot_setCustomLine() ERROR: failed to find "s" exit line button!)");
        return;
    } else if (room->getSouth() <= 0) {
        button->setDisabled(true);
        button->setCheckable(false);
    } else {
        button->setCheckable(true);
        button->setChecked(room->customLines.contains(QStringLiteral("s")));
        connect(button, &QAbstractButton::clicked, this, &T2DMap::slot_setCustomLine2);
    }

    button = dialog->findChild<QPushButton*>("se");
    if (!button) {
        qWarning(R"(T2DMap::slot_setCustomLine() ERROR: failed to find "se" exit line button!)");
        return;
    } else if (room->getSoutheast() <= 0) {
        button->setDisabled(true);
        button->setCheckable(false);
    } else {
        button->setCheckable(true);
        button->setChecked(room->customLines.contains(QStringLiteral("se")));
        connect(button, &QAbstractButton::clicked, this, &T2DMap::slot_setCustomLine2);
    }

    button = dialog->findChild<QPushButton*>("in");
    if (!button) {
        qWarning(R"(T2DMap::slot_setCustomLine() ERROR: failed to find "in" exit line button!)");
        return;
    } else if (room->getIn() <= 0) {
        button->setDisabled(true);
        button->setCheckable(false);
    } else {
        button->setCheckable(true);
        button->setChecked(room->customLines.contains(QStringLiteral("in")));
        connect(button, &QAbstractButton::clicked, this, &T2DMap::slot_setCustomLine2);
    }

    button = dialog->findChild<QPushButton*>("out");
    if (!button) {
        qWarning(R"(T2DMap::slot_setCustomLine() ERROR: failed to find "out" exit line button!)");
        return;
    } else if (room->getOut() <= 0) {
        button->setDisabled(true);
        button->setCheckable(false);
    } else {
        button->setCheckable(true);
        button->setChecked(room->customLines.contains(QStringLiteral("out")));
        connect(button, &QAbstractButton::clicked, this, &T2DMap::slot_setCustomLine2);
    }

    QMapIterator<int, QString> it(room->getOtherMap());
    while (it.hasNext()) {
        it.next();
        int id_to = it.key();
        QString dir = it.value();
        if (dir.size() > 1) {
            if (dir.startsWith('0') || dir.startsWith('1')) {
                dir = dir.mid(1);
            }
        }
        auto pI = new QTreeWidgetItem(specialExits);
        if (room->customLines.contains(dir)) {
            pI->setCheckState(0, Qt::Checked);
        } else {
            pI->setCheckState(0, Qt::Unchecked);
        }
        pI->setTextAlignment(0, Qt::AlignHCenter);
        pI->setText(1, QString::number(id_to));
        pI->setTextAlignment(1, Qt::AlignRight);
        pI->setText(2, dir);
        pI->setTextAlignment(2, Qt::AlignLeft);
    }

    button = dialog->findChild<QPushButton*>("button_cancel");
    if (!button) {
        qWarning(R"(T2DMap::slot_setCustomLine() ERROR: failed to find "cancel" button!)");
        return;
    }
    connect(button, &QAbstractButton::clicked, dialog, &QDialog::reject);
    connect(dialog, &QDialog::rejected, this, &T2DMap::slot_cancelCustomLineDialog);

    mpCurrentLineStyle->setIconSize(QSize(48, 24));
    mpCurrentLineStyle->addItem(QIcon(QPixmap(QStringLiteral(":/icons/solid-line.png"))), tr("Solid line"), static_cast<int>(Qt::SolidLine));
    mpCurrentLineStyle->addItem(QIcon(QPixmap(QStringLiteral(":/icons/dot-line.png"))), tr("Dot line"), static_cast<int>(Qt::DotLine));
    mpCurrentLineStyle->addItem(QIcon(QPixmap(QStringLiteral(":/icons/dash-line.png"))), tr("Dash line"), static_cast<int>(Qt::DashLine));
    mpCurrentLineStyle->addItem(QIcon(QPixmap(QStringLiteral(":/icons/dash-dot-line.png"))), tr("Dash-dot line"), static_cast<int>(Qt::DashDotLine));
    mpCurrentLineStyle->addItem(QIcon(QPixmap(QStringLiteral(":/icons/dash-dot-dot-line.png"))), tr("Dash-dot-dot line"), static_cast<int>(Qt::DashDotDotLine));
    mpCurrentLineStyle->setCurrentIndex(mpCurrentLineStyle->findData(static_cast<int>(mCurrentLineStyle)));

    mpCurrentLineArrow->setChecked(mCurrentLineArrow);
    mpCurrentLineColor->setStyleSheet(QStringLiteral("background-color: %1").arg(mCurrentLineColor.name()));
    connect(specialExits, &QTreeWidget::itemClicked, this, &T2DMap::slot_setCustomLine2B);
    connect(mpCurrentLineColor, &QAbstractButton::clicked, this, &T2DMap::slot_customLineColor);
    dialog->adjustSize();
    mpCustomLinesDialog = dialog; // Don't assign the pointer value to the class member until ready to go
    mpCustomLinesDialog->show();
    mpCustomLinesDialog->raise();
    mDialogLock = true; // Prevent any line drawing until dialog has been used
}

void T2DMap::slot_customLineColor()
{
    QColor color;
    if (mCurrentLineColor.isValid()) {
        color = QColorDialog::getColor(mCurrentLineColor, this);
    } else {
        color = QColorDialog::getColor(mpHost->mFgColor_2, this);
    }

    if (color.isValid()) {
        mCurrentLineColor = color;
        QString styleSheet = QString("background-color:" + color.name());
        mpCurrentLineColor->setStyleSheet(styleSheet);
    }
}

// Called by dialog's reject event which is caused at least by "X" button on
// title bar and by ESC keypress...
void T2DMap::slot_cancelCustomLineDialog()
{
    mpCustomLinesDialog->deleteLater();
    mpCustomLinesDialog = nullptr;
    mCustomLinesRoomFrom = 0;
    mCustomLinesRoomTo = 0;
    mCustomLinesRoomExit.clear();
    mDialogLock = false;
}

void T2DMap::slot_setCustomLine2()
{
    auto* button = qobject_cast<QPushButton*>(sender());
    if (!button) {
        if (mpCustomLinesDialog) {
            mpCustomLinesDialog->reject();
        } else {
            // This is needed to escape from custom line exit drawing mode if
            // the dialog has disappeared, not likely I think/hope
            mCustomLinesRoomFrom = 0;
            mCustomLinesRoomTo = 0;
            mCustomLinesRoomExit.clear();
            mDialogLock = false;
        }
        return;
    }
    QString exit = button->text();
    mpCustomLinesDialog->hide(); // Hide but don't delete until done the custom line
    mCustomLinesRoomExit = exit;
    mDialogLock = false;
    TRoom* room = mpMap->mpRoomDB->getRoom(mCustomLinesRoomFrom);
    if (!room) {
        return;
    }
    if (exit == QLatin1String("nw")) {
        mCustomLinesRoomTo = room->getNorthwest(); // mCustomLinesRoomTo - wasn't being set!
    } else if (exit == QLatin1String("n")) {
        mCustomLinesRoomTo = room->getNorth();
    } else if (exit == QLatin1String("ne")) {
        mCustomLinesRoomTo = room->getNortheast();
    } else if (exit == QLatin1String("up")) {
        mCustomLinesRoomTo = room->getUp();
    } else if (exit == QLatin1String("w")) {
        mCustomLinesRoomTo = room->getWest();
    } else if (exit == QLatin1String("e")) {
        mCustomLinesRoomTo = room->getEast();
    } else if (exit == QLatin1String("down")) {
        mCustomLinesRoomTo = room->getDown();
    } else if (exit == QLatin1String("sw")) {
        mCustomLinesRoomTo = room->getSouthwest();
    } else if (exit == QLatin1String("s")) {
        mCustomLinesRoomTo = room->getSouth();
    } else if (exit == QLatin1String("se")) {
        mCustomLinesRoomTo = room->getSoutheast();
    } else if (exit == QLatin1String("in")) {
        mCustomLinesRoomTo = room->getIn();
    } else if (exit == QLatin1String("out")) {
        mCustomLinesRoomTo = room->getOut();
    } else {
        qWarning(R"(T2DMap::slot_setCustomLine2(): unable to identify exit "%s"to use!)", qPrintable(exit));
        return;
    }
    QList<QPointF> list;
    room->customLines[exit] = list;
    //    qDebug("T2DMap::slot_setCustomLine2() NORMAL EXIT: %s", qPrintable(exit));
    room->customLinesColor[exit] = mCurrentLineColor;
    /*
     *    qDebug("   COLOR(r,g,b): %i,%i,%i",
     *            mCurrentLineColor.red(),
     *            mCurrentLineColor.green(),
     *            mCurrentLineColor.blue() );
     */
    room->customLinesStyle[exit] = mCurrentLineStyle;
    //    qDebug("   LINE STYLE: %d", mCurrentLineStyle);
    room->customLinesArrow[exit] = mCurrentLineArrow;
    //    qDebug("   ARROW: %s", mCurrentLineArrow ? "Yes" : "No");

    mHelpMsg = tr("Left-click to add point, right-click to undo/change/finish...");
    // This message was previously being put up AFTER first click to set first segment was made....
    update();
}

void T2DMap::slot_setCustomLine2B(QTreeWidgetItem* special_exit, int column)
{
    Q_UNUSED(column);
    if (!special_exit) {
        return;
    }
    QString exit = special_exit->text(2);
    mpCustomLinesDialog->hide(); // Hide but don't delete until done the custom line
    mCustomLinesRoomExit = exit;
    mCustomLinesRoomTo = special_exit->text(1).toInt(); // Wasn't being set !
    mDialogLock = false;
    TRoom* room = mpMap->mpRoomDB->getRoom(mCustomLinesRoomFrom);
    if (!room) {
        return;
    }
    QList<QPointF> _list;
    room->customLines[exit] = _list;
    //    qDebug("T2DMap::slot_setCustomLine2B() SPECIAL EXIT: %s", qPrintable(exit));
    room->customLinesColor[exit] = mCurrentLineColor;
    /*
     *     qDebug("   COLOR(r,g,b): %i,%i,%i",
     *            mCurrentLineColor.red(),
     *            mCurrentLineColor.green(),
     *            mCurrentLineColor.blue() );
     */
    room->customLinesStyle[exit] = mCurrentLineStyle;
    //    qDebug("   LINE STYLE: %d", mCurrentLineStyle);
    room->customLinesArrow[exit] = mCurrentLineArrow;
    //    qDebug("   ARROW: %s", mCurrentLineArrow ? "Yes" : "No");
    mHelpMsg = tr("Left-click to add point, right-click to undo/change/finish...");
    // This message was previously being put up AFTER first click to set first segment was made....
    update();
}

void T2DMap::slot_createLabel()
{
    if (!mpMap->mpRoomDB->getArea(mAreaID)) {
        return;
    }

    mHelpMsg = tr("Left-click and drag a square for the size and position of your label");
    mSizeLabel = true;
    mMultiSelection = true;
    update();
}

void T2DMap::slot_roomSelectionChanged()
{
    QList<QTreeWidgetItem*> selection = mMultiSelectionListWidget.selectedItems();
    mMultiSelectionSet.clear();
    for (auto treeWidgetItem : selection) {
        int currentRoomId = treeWidgetItem->text(0).toInt();
        mMultiSelectionSet.insert(currentRoomId);
    }
    switch (mMultiSelectionSet.size()) {
    case 0:
        mMultiSelectionHighlightRoomId = 0;
        break;
    case 1:
        mMultiSelectionHighlightRoomId = *(mMultiSelectionSet.constBegin());
        break;
    default:
        getCenterSelection();
    }
    update();
}

void T2DMap::paintMap()
{
    // TODO: reimpliment this!
}

void T2DMap::resizeMultiSelectionWidget()
{
    int newWidth;
    if (mIsSelectionUsingNames) {
        if (width() <= 300) { // 0 - 300 => 0 - 200
            newWidth = 2 * width() / 3;
        } else if (width() <= 600) { // 300 - 600 => 200 - 300
            newWidth = 100 + width() / 3;
        } else { // 600+ => 300
            newWidth = 300;
        }
    } else {
        if (width() <= 300) { // 0 - 300 => 0 - 120
            newWidth = 2 * width() / 3;
        } else { // 300+ => 120
            newWidth = 120;
        }
    }
    int _newHeight = 300;
    if (mMultiSelectionListWidget.topLevelItemCount() > 0) {
        QTreeWidgetItem* rowItem = mMultiSelectionListWidget.topLevelItem(1);
        // The following factors are tweaks to ensure that the widget shows all
        // the rows, as the header seems bigger than the value returned, statics
        // used to enable values to be change by debugger at runtime!
        static float headerFactor = 1.2;
        static float rowFactor = 1.0;
        _newHeight = headerFactor * mMultiSelectionListWidget.header()->height();
        if (rowItem) { // Have some data rows - and we have forced them to be the same height:
            _newHeight += rowFactor * mMultiSelectionListWidget.topLevelItemCount() * mMultiSelectionListWidget.visualItemRect(rowItem).height();
        }
    }
    if (_newHeight < height()) {
        mMultiSelectionListWidget.resize(newWidth, _newHeight);
    } else {
        mMultiSelectionListWidget.resize(newWidth, height());
    }
}
