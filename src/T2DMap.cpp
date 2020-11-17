/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2013-2016, 2018-2020 by Stephen Lyons                   *
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
#if defined(INCLUDE_3DMAPPER)
#include "glwidget.h"
#endif


#include "pre_guard.h"
#include <QtEvents>
#include <QtUiTools>
#include "post_guard.h"

// QStringLiterals cannot be shared so define a common instance to use when
// there are multiple places where they are used within this file:

// replacement parameter supplied at point of use:
const QString& key_plain = QStringLiteral("%1");

const QString& key_n = QStringLiteral("n");
const QString& key_ne = QStringLiteral("ne");
const QString& key_e = QStringLiteral("e");
const QString& key_se = QStringLiteral("se");
const QString& key_s = QStringLiteral("s");
const QString& key_sw = QStringLiteral("sw");
const QString& key_w = QStringLiteral("w");
const QString& key_nw = QStringLiteral("nw");
const QString& key_up = QStringLiteral("up");
const QString& key_down = QStringLiteral("down");
const QString& key_in = QStringLiteral("in");
const QString& key_out = QStringLiteral("out");

const QString& key_icon_line_solid = QStringLiteral(":/icons/solid-line.png");
const QString& key_icon_line_dot = QStringLiteral(":/icons/dot-line.png");
const QString& key_icon_line_dash = QStringLiteral(":/icons/dash-line.png");
const QString& key_icon_line_dashDot = QStringLiteral(":/icons/dash-dot-line.png");
const QString& key_icon_line_dashDotDot = QStringLiteral(":/icons/dash-dot-dot-line.png");

const QString& key_dialog_ok_apply = QStringLiteral("dialog-ok-apply");
const QString& key_dialog_cancel = QStringLiteral("dialog-cancel");

const QString& key_icon_dialog_ok_apply = QStringLiteral(":/icons/dialog-ok-apply.png");
const QString& key_icon_dialog_cancel = QStringLiteral(":/icons/dialog-cancel.png");

// replacement parameter supplied at points of use:
const QString& key_styleSheet_backgroundColor = QStringLiteral("background-color: %1");

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
, mMaxRoomIdDigits(0)
{
    mMultiSelectionListWidget.setColumnCount(2);
    mMultiSelectionListWidget.hideColumn(1);
    QStringList headerLabels;
    headerLabels << tr("ID", "Room ID in the mapper widget") << tr("Name", "Room name in the mapper widget");
    mMultiSelectionListWidget.setHeaderLabels(headerLabels);
    mMultiSelectionListWidget.setToolTip(tr("<p>Click on a line to select or deselect that room number (with the given name if the "
                                                 "rooms are named) to add or remove the room from the selection.  Click on the "
                                                 "relevant header to sort by that method.  Note that the name column will only "
                                                 "show if at least one of the rooms has a name.</p>"));
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
            if (16 < env && env < 232) {
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

    int playerRoomId = mpMap->mRoomIdHash.value(mpMap->mProfileName);
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

                    if (closestCenterRoom) {
                        mOx = closestCenterRoom->x;
                        // Map y coordinates are reversed on 2D map!
                        mOy = -closestCenterRoom->y;
                        mOz = closestCenterRoom->z;
                    } else {
                        mOx = mOy = mOz = 0;
                    }
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

    auto pixmap = new QPixmap(symbolRectangle.toRect().size());
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

/*
 * Helper used to size fonts establishes the font size to use to draw the given
 * sample text centralized inside the given boundary (or rather the boundary
 * reduced by the margin (0-40) as a percentage). This margin is defaulted to
 * 10%.
 */
bool T2DMap::sizeFontToFitTextInRect( QFont & font, const QRectF & boundaryRect, const QString & text, const quint8 percentageMargin, qreal minFontSize )
{
    QFont _font = font;

    if (percentageMargin > 40) {
        qWarning() << "T2DMap::sizeFontToFitTextInRect(...) percentage margin" << percentageMargin << "exceeded recommended maximum (40%) !";
    }
    if (text.isEmpty()) {
        qWarning() << "T2DMap::sizeFontToFitTextInRect(...) called with no sample text!";
        return false;
    }

    qreal fontSize = qMax(minFontSize, font.pointSizeF());  // protect against too-small initial value
    QRectF testRect(boundaryRect.width() * (100 - percentageMargin) / 200.0,
                    boundaryRect.height() * (100 - percentageMargin) / 200.0,
                    boundaryRect.width() * (100 - percentageMargin) / 100.0,
                    boundaryRect.height() * (100 - percentageMargin) / 100.);

    // Increase the test font (using somewhat-large steps) until it does not fit any more
    QRectF neededRect;
    QPixmap _pixmap(qRound(1.0 + boundaryRect.width()), qRound(1.0 + boundaryRect.height()));
    QPainter _painter(&_pixmap);
    do {
        fontSize *= 1.2;
        _font.setPointSizeF(fontSize);
        _painter.setFont(_font);

        neededRect = _painter.boundingRect(testRect, Qt::AlignCenter | Qt::TextSingleLine | Qt::TextIncludeTrailingSpaces, text);
    } while (testRect.contains(neededRect));

    // Now decrease (using smaller steps) until it fits again
    bool isSizeTooSmall = false;
    do {
        fontSize /= 1.05;
        _font.setPointSizeF(fontSize);
        if (fontSize < minFontSize) {
            isSizeTooSmall = true;
        }

        _painter.setFont(_font);

        neededRect = _painter.boundingRect(testRect, Qt::AlignCenter | Qt::TextSingleLine | Qt::TextIncludeTrailingSpaces, text);
    } while ((!isSizeTooSmall) && (!testRect.contains(neededRect)));

    if (isSizeTooSmall) {
        return false;
    }

    font.setPointSizeF(fontSize);
    return true;
}

// Helper that refactors out code to start a speedwalk:
void T2DMap::initiateSpeedWalk(const int speedWalkStartRoomId, const int speedWalkTargetRoomId)
{
    mTarget = speedWalkTargetRoomId;
    if (mpMap->mpRoomDB->getRoom(speedWalkTargetRoomId)) {
        mpMap->mTargetID = speedWalkTargetRoomId;

        if (mpHost->checkForCustomSpeedwalk()) {
            mpHost->startSpeedWalk(speedWalkStartRoomId, speedWalkTargetRoomId);
        } else if (mpMap->findPath(speedWalkStartRoomId, speedWalkTargetRoomId)) {
            mpHost->startSpeedWalk();
        } else {
            mpHost->mpConsole->printSystemMessage(QStringLiteral("%1\n").arg(tr("Mapper: Cannot find a path from %1 to %2 using known exits.")
                                                          .arg(QString::number(speedWalkStartRoomId),
                                                               QString::number(speedWalkTargetRoomId))));
        }
    }
}

// This has been refactored to a separate function out of the paintEven() code
// because we need to use it in two places - one for every room that is not the
// player's room and then, AFTER all those have been drawn, once for the
// player's room if it is visible. This is so it is drawn LAST (and any effects,
// or extra markings for it do not get overwritten by the drawing of the other
// rooms)...
inline void T2DMap::drawRoom(QPainter& painter, QFont& roomVNumFont, QFont& mapNameFont, QPen& pen, TRoom* pRoom, const bool isGridMode, const bool areRoomIdsLegible, bool showRoomName, const int speedWalkStartRoomId, const float rx, const float ry, const bool picked)
{
    const int currentRoomId = pRoom->getId();
    pRoom->rendered = false;
    QRectF roomRectangle;
    QRectF roomNameRectangle;
    double realHeight;
    if (isGridMode) {
        realHeight = mRoomHeight;
        roomRectangle = QRectF(rx - mRoomWidth / 2.0, ry - mRoomHeight / 2.0, mRoomWidth, mRoomHeight);
    } else {
        // this dance is necessary to put the name just below the room rect, later
        realHeight = mRoomHeight * rSize;
        roomRectangle = QRectF(rx - (mRoomWidth * rSize) / 2.0, ry - realHeight / 2.0, mRoomWidth * rSize, realHeight);
    }

    roomNameRectangle = roomRectangle.adjusted(-2000, realHeight, 2000, realHeight);

    painter.save();
    painter.setFont(mapNameFont);
    roomNameRectangle = painter.boundingRect(roomNameRectangle, Qt::TextSingleLine|Qt::AlignTop|Qt::AlignCenter, pRoom->name);
    painter.restore();

    // We should be using the full area for testing for clicks even though
    // we only show a smaller one if the user has dialed down the room size
    // on NON-grid mode areas:
    const QRectF roomClickTestRectangle(QRectF(static_cast<qreal>(rx) - (static_cast<qreal>(mRoomWidth) / 2.0),
                                               static_cast<qreal>(ry) - (static_cast<qreal>(mRoomHeight) / 2.0),
                                               static_cast<qreal>(mRoomWidth), static_cast<qreal>(mRoomHeight)));

    QColor roomColor;
    int roomEnvironment = pRoom->environment;
    if (mpMap->envColors.contains(roomEnvironment)) {
        roomEnvironment = mpMap->envColors[roomEnvironment];
    } else {
        if (!mpMap->customEnvColors.contains(roomEnvironment)) {
            roomEnvironment = 1;
        }
    }
    // clang-format off
    switch (roomEnvironment) {
    case 1:     roomColor = mpHost->mRed_2;             break;
    case 2:     roomColor = mpHost->mGreen_2;           break;
    case 3:     roomColor = mpHost->mYellow_2;          break;
    case 4:     roomColor = mpHost->mBlue_2;            break;
    case 5:     roomColor = mpHost->mMagenta_2;         break;
    case 6:     roomColor = mpHost->mCyan_2;            break;
    case 7:     roomColor = mpHost->mWhite_2;           break;
    case 8:     roomColor = mpHost->mBlack_2;           break;
    case 9:     roomColor = mpHost->mLightRed_2;        break;
    case 10:    roomColor = mpHost->mLightGreen_2;      break;
    case 11:    roomColor = mpHost->mLightYellow_2;     break;
    case 12:    roomColor = mpHost->mLightBlue_2;       break;
    case 13:    roomColor = mpHost->mLightMagenta_2;    break;
    case 14:    roomColor = mpHost->mLightCyan_2;       break;
    case 15:    roomColor = mpHost->mLightWhite_2;      break;
    case 16:    roomColor = mpHost->mLightBlack_2;      break;
    // clang-format on
    default: //user defined room color
        if (mpMap->customEnvColors.contains(roomEnvironment)) {
            roomColor = mpMap->customEnvColors[roomEnvironment];
        } else {
            if (16 < roomEnvironment && roomEnvironment < 232) {
                quint8 base = roomEnvironment - 16;
                quint8 r = base / 36;
                quint8 g = (base - (r * 36)) / 6;
                quint8 b = (base - (r * 36)) - (g * 6);

                r *= 51;
                g *= 51;
                b *= 51;
                roomColor = QColor(r, g, b, 255);
            } else if (231 < roomEnvironment && roomEnvironment < 256) {
                quint8 k = ((roomEnvironment - 232) * 10) + 8;
                roomColor = QColor(k, k, k, 255);
            }
        }
    }

    if (((mPick || picked) && roomClickTestRectangle.contains(mPHighlight))
        || mMultiSelectionSet.contains(currentRoomId)) {

        // This room is in the selection:
        // TODO: replace this flood fill of the room with an orange rectangle with something a bit more subtle:
        painter.fillRect(roomRectangle, QColor(255, 155, 55));
        mPick = false;
        if (mStartSpeedWalk) {
            mStartSpeedWalk = false;
            // This draws a red circle around the room that was choosen as
            // the target for the speedwalk, but it is only shown for one
            // paintEvent call and it is not obvious that it is useful, note
            // that this is the code for a room being clicked on that is
            // within the area - there is a separate block of code further down
            // in this method that handles clicking on the out of area exit so
            // that a speed walk is done to the room in the OTHER area:
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

            initiateSpeedWalk(speedWalkStartRoomId, currentRoomId);
        }

    } else {
        // Room is NOT selected
        if (mBubbleMode) {
            float roomRadius = 0.5 * rSize * mRoomWidth;
            QPointF roomCenter = QPointF(rx, ry);
            // CHECK: The use of a gradient fill to a white center on round
            // rooms might look nice in some sitations but not in all:
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

        // Do we need to draw the room symbol:
        if (!(mShowRoomID && areRoomIdsLegible) && !pRoom->mSymbol.isEmpty()) {
            QString pixmapKey;
            if (roomColor.lightness() > 127) {
                pixmapKey = QStringLiteral("B_%1").arg(pRoom->mSymbol);
            } else {
                pixmapKey = QStringLiteral("W_%1").arg(pRoom->mSymbol);
            }
            if (!mSymbolPixmapCache.contains(pixmapKey)) {
                addSymbolToPixmapCache(pixmapKey, isGridMode);
            }

            painter.save();
            painter.setBackgroundMode(Qt::TransparentMode);

            QPixmap* pix = mSymbolPixmapCache.object(pixmapKey);
            if (!pix) {
                qWarning("T2DMap::paintEvent() Alert: mSymbolPixmapCache failure, too many items to cache all of them for: \"%s\"", pRoom->mSymbol.toUtf8().constData());
            } else {
                /*
                 * For the non-scaling QPainter::drawPixmap() used now we
                 * have to position the generated pixmap containing the
                 * particular symbol for this room to Y when it would
                 * position it at X - this should be faster than the previous
                 * scaling QPainter::drawPixmap() as that would scale the
                 * pixmap to fit the Room Rectangle!
                 *
                 *                         |<------->| roomRectangle.width()
                 * roomRectangle.topLeft-->X---------+
                 *                         |  Room   |
                 *                         |  Y---+  |
                 *                         |  |Pix|  |
                 *                         |  +---+  |
                 *                         |Rectangle|
                 *                         +---------+
                 *                            |<->|<--symbolRect.width()
                 *            x-offset---->|<>|<-- (roomRectangle.width() - symbolRect.width())/2.0
                 * similarly for the y-offset
                 */

                painter.drawPixmap(
                        QPoint(qRound(roomRectangle.left() + ((roomRectangle.width() - pix->width()) / 2.0)), qRound(roomRectangle.top() + ((roomRectangle.height() - pix->height()) / 2.0))),
                        *pix);
            }

            painter.restore();
        }

        // Do we need to draw the custom (user specified) highlight
        if (pRoom->highlight) {
            float roomRadius = (pRoom->highlightRadius * mRoomWidth) / 2.0;
            QPointF roomCenter = QPointF(rx, ry);
            QRadialGradient gradient(roomCenter, roomRadius);
            gradient.setColorAt(0.85, pRoom->highlightColor);
            gradient.setColorAt(0, pRoom->highlightColor2);
            QPen transparentPen(Qt::transparent);
            QPainterPath diameterPath;
            painter.setBrush(gradient);
            painter.setPen(transparentPen);
            diameterPath.addEllipse(roomCenter, roomRadius, roomRadius);
            painter.drawPath(diameterPath);
        }

        // Do we need to draw the room Id number:
        if (mShowRoomID && areRoomIdsLegible) {
            painter.save();
            QColor roomIdColor;
            if (roomColor.lightness() > 127) {
                roomIdColor = QColor(Qt::black);
            } else {
                roomIdColor = QColor(Qt::white);
            }
            painter.setPen(QPen(roomIdColor));
            painter.setFont(roomVNumFont);
            painter.drawText(roomRectangle, Qt::AlignCenter, QString::number(currentRoomId));
            painter.restore();
        }

    }
    // If there is a room name, draw it?
    if (showRoomName) {
        showRoomName = getUserDataBool(pRoom->userData, ROOM_UI_SHOWNAME, false);
    }
    if (showRoomName) {
        painter.save();

        QString namePosData = pRoom->userData.value(ROOM_UI_NAMEPOS);
        if (!namePosData.isEmpty()) {
            QPointF nameOffset {0, 0};
            QStringList posXY = namePosData.split(" ");
            bool ok1, ok2;
            double posX, posY;

            switch (posXY.count()) {
                case 1:
                // one value: treat as Y offset
                posY = posXY[0].toDouble(&ok1);
                if (ok1) {
                    nameOffset.setY(posY);
                }
                break;
                case 2:
                posX = posXY[0].toDouble(&ok1);
                posY = posXY[1].toDouble(&ok2);
                if (ok1 && ok2) {
                    nameOffset.setX(posX);
                    nameOffset.setY(posY);
                }
                break;
            }
            roomNameRectangle.adjust(mRoomWidth * nameOffset.x(),
                                    mRoomHeight * nameOffset.y(),
                                    mRoomWidth * nameOffset.x(),
                                    mRoomHeight * nameOffset.y());
        }
        auto roomNameColor = QColor((mpHost->mBgColor_2.lightness() > 127)
                                    ? Qt::black : Qt::white);
        painter.setPen(QPen(roomNameColor));
        painter.setFont(mapNameFont);
        painter.drawText(roomNameRectangle, Qt::AlignCenter, pRoom->name);
        painter.restore();
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
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
    if (roomColor.lightness() > 127) {
        lc = QColorConstants::Black;
    } else {
        lc = QColorConstants::White;
    }
#else
    if (roomColor.lightness() > 127) {
        lc = QColor(Qt::black);
    } else {
        lc = QColor(Qt::white);
    }
#endif
    pen = painter.pen();
    pen.setColor(lc);
    pen.setCosmetic(mMapperUseAntiAlias);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    QPen innerPen = pen;
    painter.save();

    QBrush innerBrush = painter.brush();
    innerBrush.setStyle(Qt::NoBrush);
    if (pRoom->getUp() > 0 || pRoom->exitStubs.contains(DIR_UP)) {
        QPolygonF poly_up;
        poly_up.append(QPointF(rx, ry + (mRoomHeight * rSize * allInsideTipOffsetFactor)));
        poly_up.append(QPointF(rx - (mRoomWidth * rSize * upDownXOrYFactor), ry + (mRoomHeight * rSize * upDownXOrYFactor)));
        poly_up.append(QPointF(rx + (mRoomWidth * rSize * upDownXOrYFactor), ry + (mRoomHeight * rSize * upDownXOrYFactor)));
        bool isDoor = true;
        QBrush brush = painter.brush();
        switch (pRoom->doors.value(key_up)) {
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
        if (pRoom->getUp() > 0) {
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

    if (pRoom->getDown() > 0 || pRoom->exitStubs.contains(DIR_DOWN)) {
        QPolygonF poly_down;
        poly_down.append(QPointF(rx, ry - (mRoomHeight * rSize * allInsideTipOffsetFactor)));
        poly_down.append(QPointF(rx - (mRoomWidth * rSize * upDownXOrYFactor), ry - (mRoomHeight * rSize * upDownXOrYFactor)));
        poly_down.append(QPointF(rx + (mRoomWidth * rSize * upDownXOrYFactor), ry - (mRoomHeight * rSize * upDownXOrYFactor)));
        bool isDoor = true;
        QBrush brush = painter.brush();
        switch (pRoom->doors.value(key_down)) {
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
        if (pRoom->getDown() > 0) {
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

    if (pRoom->getIn() > 0 || pRoom->exitStubs.contains(DIR_IN)) {
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
        switch (pRoom->doors.value(key_in)) {
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
        if (pRoom->getIn() > 0) {
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

    if (pRoom->getOut() > 0 || pRoom->exitStubs.contains(DIR_OUT)) {
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
        switch (pRoom->doors.value(key_out)) {
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
        if (pRoom->getOut() > 0) {
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
    if (!isGridMode) {
        QMapIterator<int, QPoint> it(mAreaExitsList);
        while (it.hasNext()) {
            it.next();
            QPoint P = it.value();
            int rx = P.x();
            int ry = P.y();

            QRectF dr = QRectF(rx, ry, mRoomWidth * rSize, mRoomHeight * rSize);

            // clang-format off
            if (((mPick || picked)
                 && mPHighlight.x() >= (dr.x() - mRoomWidth / 3.0)
                 && mPHighlight.x() <= (dr.x() + mRoomWidth / 3.0)
                 && mPHighlight.y() >= (dr.y() - mRoomHeight / 3.0)
                 && mPHighlight.y() <= (dr.y() + mRoomHeight / 3.0))
                && mStartSpeedWalk) {

                // clang-format on
                mStartSpeedWalk = false;
                // This draws a red circle around the out of area exit that
                // was choosen as the target for the speedwalk, but it is
                // only shown for one paintEvent call and it is not obvious
                // that it is useful, note that there is similar code for a
                // room being clicked on that is WITHIN the area, that is
                // above this point in the source code:
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
                initiateSpeedWalk(speedWalkStartRoomId, it.key());
            }
        }
    }
}

// Revised to use a QCache to hold QPixmap * to generated images for room symbols
void T2DMap::paintEvent(QPaintEvent* e)
{
    if (!mpMap) {
        return;
    }
    // As it happens this local is never changed in this method so it can be
    // made const:
    const bool __Pick = mPick;
    QElapsedTimer renderTimer;
    renderTimer.start();

    QPainter painter(this);
    if (!painter.isActive()) {
        return;
    }

    mAreaExitsList.clear();

    const float widgetWidth = width();
    const float widgetHeight = height();

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

    // the room name's font defaults to the symbol's
    // but may be overridden
    auto mapNameFont = mpMap->mMapSymbolFont;
    QString fontName = mpMap->mUserData.value(ROOM_UI_NAMEFONT);
    if (!fontName.isEmpty()) {
        QFont font;
        if (font.fromString(fontName)) {
            mapNameFont = font;
        }
    }
    mapNameFont.setBold(false);
    mapNameFont.setItalic(false);
    mapNameFont.setUnderline(false);
    mapNameFont.setOverline(false);
    mapNameFont.setStrikeOut(false);

    QList<int> exitList;
    QList<int> oneWayExits;
    int playerRoomId = mpMap->mRoomIdHash.value(mpMap->mProfileName);
    TRoom* pPlayerRoom = mpMap->mpRoomDB->getRoom(playerRoomId);
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

    qreal ox;
    qreal oy;
    if (mRoomID != playerRoomId && mShiftMode) {
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
        mRoomID = playerRoomId;
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
    QFont roomVNumFont = mpMap->mMapSymbolFont;

    bool isFontBigEnoughToShowRoomVnum = false;
    if (mShowRoomID) {
        /*
         * If we are to show the room Id numbers - find out the number of digits
         * that we will need to use; actually, knowing the digit count is also
         * useful for the room selection widget so perform this check EVERY time.
         * TODO: Eventually move this check to the TArea class and just redo it
         * when areas' room content changes.
         */
        int maxUsedRoomId = 0;
        QSetIterator<int> itRoomId(playerArea->getAreaRooms());
        while (itRoomId.hasNext()) {
            maxUsedRoomId = qMax(maxUsedRoomId, itRoomId.next());
        }
        mMaxRoomIdDigits = static_cast<quint8>(QString::number(maxUsedRoomId).length());

        QRectF roomTestRect;
        if (playerArea->gridMode) {
            roomTestRect = QRectF(0, 0, static_cast<qreal>(mRoomWidth), static_cast<qreal>(mRoomHeight));
        } else {
            roomTestRect = QRectF(0, 0, static_cast<qreal>(mRoomWidth) * rSize, static_cast<qreal>(mRoomHeight) * rSize);
        }
        static quint8 roomVnumMargin = 10;
        roomVNumFont.setBold(true);

        // QFont::PreferOutline will help to select a font that will scale to any
        // size - which is important for good rendering over a range of sizes
        // QFont::PreferAntialias will look better - except perhaps at very small
        // sizes (but we prevent that by checking in the method call afterwards):
        roomVNumFont.setStyleStrategy(QFont::StyleStrategy(QFont::PreferNoShaping|QFont::PreferAntialias|QFont::PreferOutline));

        isFontBigEnoughToShowRoomVnum = sizeFontToFitTextInRect(roomVNumFont, roomTestRect, QStringLiteral("8").repeated(mMaxRoomIdDigits), roomVnumMargin);
    }

    bool showRoomNames = mpMap->getRoomNamesShown() && !playerArea->gridMode;
    if (showRoomNames) {
        /*
         * Like above, except that we use the room height as the font size.
         */
        mapNameFont.setBold(true);

        mapNameFont.setStyleStrategy(QFont::StyleStrategy(QFont::PreferNoShaping|QFont::PreferAntialias|QFont::PreferOutline));

        double sizeAdjust = 0; // TODO add userdata setting to adjust this
        mapNameFont.setPointSizeF(static_cast<qreal>(mRoomWidth) * rSize * pow(1.1, sizeAdjust) / 2.0);
        showRoomNames = (mapNameFont.pointSizeF() > 3.0);
    }

    TArea* pArea = playerArea;

    int zLevel = mOz;

    float exitWidth = 1 / eSize * mRoomWidth * rSize;

    painter.fillRect(0, 0, width(), height(), mpHost->mBgColor_2);

    auto pen = painter.pen();
    pen.setColor(mpHost->mFgColor_2);
    pen.setWidthF(exitWidth);
    painter.setRenderHint(QPainter::Antialiasing, mMapperUseAntiAlias);
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
        paintAreaExits(painter, pen, exitList, oneWayExits, pArea, zLevel, exitWidth);
    }

    // Draw label sizing or group selection box
    if (mSizeLabel) {
        painter.fillRect(mMultiRect, QColor(250, 190, 0, 190));
    } else {
        painter.fillRect(mMultiRect, QColor(190, 190, 190, 60));
    }

    QPointF playerRoomOnWidgetCoordinates;
    bool isPlayerRoomVisible = false;
    // Draw the rooms:
    QSetIterator<int> itRoom(pArea->getAreaRooms());
    while (itRoom.hasNext()) {
        int currentAreaRoom = itRoom.next();
        TRoom* room = mpMap->mpRoomDB->getRoom(currentAreaRoom);
        if (!room) {
            continue;
        }

        if (room->z != zLevel) {
            continue;
        }

        float rx = room->x *       mRoomWidth + static_cast<float>(mRX);
        float ry = room->y * -1 * mRoomHeight + static_cast<float>(mRY);
        if (rx < 0 || ry < 0 || rx > widgetWidth || ry > widgetHeight) {
            continue;
        }

        if (playerRoomId == currentAreaRoom) {
            // We defer drawing THIS (the player's room) until the end
            isPlayerRoomVisible = true;
            playerRoomOnWidgetCoordinates = QPointF(static_cast<qreal>(rx), static_cast<qreal>(ry));
        } else {
            // Not the player's room:
            drawRoom(painter, roomVNumFont, mapNameFont, pen, room, pArea->gridMode, isFontBigEnoughToShowRoomVnum, showRoomNames, playerRoomId, rx, ry, __Pick);
        }
    } // End of while loop for each room in area

    if (isPlayerRoomVisible) {
        drawRoom(painter, roomVNumFont, mapNameFont, pen, playerRoom, pArea->gridMode, isFontBigEnoughToShowRoomVnum, showRoomNames, playerRoomId, static_cast<float>(playerRoomOnWidgetCoordinates.x()), static_cast<float>(playerRoomOnWidgetCoordinates.y()), __Pick);
        painter.save();
        QPen transparentPen(Qt::transparent);
        QPainterPath myPath;
        double roomRadius = (mpMap->mPlayerRoomOuterDiameterPercentage / 200.0) * static_cast<double>(mRoomWidth);
        QRadialGradient gradient(playerRoomOnWidgetCoordinates, roomRadius);
        if (mpHost->mMapStrongHighlight) {
            // Never set, no means to except via XMLImport, as dlgMapper class's
            // slot_toggleStrongHighlight is not wired up to anything
            QRectF dr = QRectF(playerRoomOnWidgetCoordinates.x() - (static_cast<double>(mRoomWidth) * rSize) / 2.0,
                               playerRoomOnWidgetCoordinates.y() - (static_cast<double>(mRoomHeight) * rSize) / 2.0,
                               static_cast<double>(mRoomWidth) * rSize, static_cast<double>(mRoomHeight) * rSize);
            painter.fillRect(dr, QColor(255, 0, 0, 150));

            gradient.setColorAt(0.95, QColor(255, 0, 0, 150));
            gradient.setColorAt(0.80, QColor(150, 100, 100, 150));
            gradient.setColorAt(0.799, QColor(150, 100, 100, 100));
            gradient.setColorAt(0.7, QColor(255, 0, 0, 200));
            gradient.setColorAt(0, Qt::white);
            painter.setBrush(gradient);
            painter.setPen(transparentPen);
            myPath.addEllipse(playerRoomOnWidgetCoordinates, roomRadius, roomRadius);
        } else {
            gradient.setStops(mPlayerRoomColorGradentStops);
            painter.setBrush(gradient);
            painter.setPen(transparentPen);
            myPath.addEllipse(playerRoomOnWidgetCoordinates, roomRadius, roomRadius);
        }
        painter.drawPath(myPath);
        painter.restore();
    }

    // Draw Labels above the map
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

    // Draw an indication of the central room of a multi-room selection.
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

    if (mShowInfo) {
        paintMapInfo(renderTimer, painter, playerAreaID == mAreaID, infoColor);
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

    if (!mHelpMsg.isEmpty()) {
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

static QPointF parseOffsetString(const QString& offsetStr)
{
    QPointF re {0, 0};
    QStringList posXY = offsetStr.split(" ");
    bool ok1, ok2;
    double posX, posY;

    switch (posXY.count()) {
        case 1:
        // one value: treat as Y offset
        posY = posXY[0].toDouble(&ok1);
        if (ok1) {
            re.setY(posY);
        }
        break;
        case 2:
        posX = posXY[0].toDouble(&ok1);
        posY = posXY[1].toDouble(&ok2);
        if (ok1 && ok2) {
            re.setX(posX);
            re.setY(posY);
        }
        break;
    }

    return re;
}

void T2DMap::paintAreaExits(QPainter& painter, QPen& pen, QList<int>& exitList, QList<int>& oneWayExits, const TArea* pArea, int zLevel, float exitWidth)
{
    const float widgetWidth = width();
    const float widgetHeight = height();

    const float area_miny = pArea->min_y * -1 * mRoomHeight + static_cast<float>(mRY);
    const float area_maxy = pArea->max_y * -1 * mRoomHeight + static_cast<float>(mRY);
    const float area_minx = pArea->min_x * mRoomWidth + static_cast<float>(mRX);
    const float area_maxx = pArea->max_x * mRoomWidth + static_cast<float>(mRX);
    const float area_width = abs(area_maxx - area_minx);
    const float area_height = abs(area_maxy - area_miny);

    int customLineDestinationTarget = 0;
    if (mCustomLinesRoomTo > 0) {
        customLineDestinationTarget = mCustomLinesRoomTo;
    } else if (mCustomLineSelectedRoom > 0 && !mCustomLineSelectedExit.isEmpty()) {
        TRoom* pSR = mpMap->mpRoomDB->getRoom(mCustomLineSelectedRoom);
        if (pSR) {
            if (mCustomLineSelectedExit == key_nw) {
                customLineDestinationTarget = pSR->getNorthwest();
            } else if (mCustomLineSelectedExit == key_n) {
                customLineDestinationTarget = pSR->getNorth();
            } else if (mCustomLineSelectedExit == key_ne) {
                customLineDestinationTarget = pSR->getNortheast();
            } else if (mCustomLineSelectedExit == key_up) {
                customLineDestinationTarget = pSR->getUp();
            } else if (mCustomLineSelectedExit == key_w) {
                customLineDestinationTarget = pSR->getWest();
            } else if (mCustomLineSelectedExit == key_e) {
                customLineDestinationTarget = pSR->getEast();
            } else if (mCustomLineSelectedExit == key_down) {
                customLineDestinationTarget = pSR->getDown();
            } else if (mCustomLineSelectedExit == key_sw) {
                customLineDestinationTarget = pSR->getSouthwest();
            } else if (mCustomLineSelectedExit == key_s) {
                customLineDestinationTarget = pSR->getSouth();
            } else if (mCustomLineSelectedExit == key_se) {
                customLineDestinationTarget = pSR->getSoutheast();
            } else if (mCustomLineSelectedExit == key_in) {
                customLineDestinationTarget = pSR->getIn();
            } else if (mCustomLineSelectedExit == key_out) {
                customLineDestinationTarget = pSR->getOut();
            } else {
                QMapIterator<int, QString> otherExitIt = pSR->getOtherMap();
                while (otherExitIt.hasNext()) {
                    otherExitIt.next();
                    if (otherExitIt.value().startsWith(QLatin1String("0")) || otherExitIt.value().startsWith(QLatin1String("1"))) {
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
        const float rx = room->x * mRoomWidth + mRX;
        const float ry = room->y * -1 * mRoomHeight + mRY;
        const int rz = room->z;

        if (rz != zLevel) {
            continue;
        }

        if (room->customLines.empty()) {
            if (rx < 0 || ry < 0 || rx > widgetWidth || ry > widgetHeight) {
                continue;
            }
        } else {
            const float miny = room->min_y * -1 * mRoomHeight + static_cast<float>(mRY);
            const float maxy = room->max_y * -1 * mRoomHeight + static_cast<float>(mRY);
            const float minx = room->min_x * mRoomWidth + static_cast<float>(mRX);
            const float maxx = room->max_x * mRoomWidth + static_cast<float>(mRX);

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
            if (!room->customLines.contains(key_n)) {
                exitList.push_back(room->getNorth());
                TRoom* pER = mpMap->mpRoomDB->getRoom(room->getNorth());
                if (pER) {
                    if (pER->getSouth() != _id) {
                        oneWayExits.push_back(room->getNorth());
                    }
                }
            }
            if (!room->customLines.contains(key_ne)) {
                exitList.push_back(room->getNortheast());
                TRoom* pER = mpMap->mpRoomDB->getRoom(room->getNortheast());
                if (pER) {
                    if (pER->getSouthwest() != _id) {
                        oneWayExits.push_back(room->getNortheast());
                    }
                }
            }
            if (!room->customLines.contains(key_e)) {
                exitList.push_back(room->getEast());
                TRoom* pER = mpMap->mpRoomDB->getRoom(room->getEast());
                if (pER) {
                    if (pER->getWest() != _id) {
                        oneWayExits.push_back(room->getEast());
                    }
                }
            }
            if (!room->customLines.contains(key_se)) {
                exitList.push_back(room->getSoutheast());
                TRoom* pER = mpMap->mpRoomDB->getRoom(room->getSoutheast());
                if (pER) {
                    if (pER->getNorthwest() != _id) {
                        oneWayExits.push_back(room->getSoutheast());
                    }
                }
            }
            if (!room->customLines.contains(key_s)) {
                exitList.push_back(room->getSouth());
                TRoom* pER = mpMap->mpRoomDB->getRoom(room->getSouth());
                if (pER) {
                    if (pER->getNorth() != _id) {
                        oneWayExits.push_back(room->getSouth());
                    }
                }
            }
            if (!room->customLines.contains(key_sw)) {
                exitList.push_back(room->getSouthwest());
                TRoom* pER = mpMap->mpRoomDB->getRoom(room->getSouthwest());
                if (pER) {
                    if (pER->getNortheast() != _id) {
                        oneWayExits.push_back(room->getSouthwest());
                    }
                }
            }
            if (!room->customLines.contains(key_w)) {
                exitList.push_back(room->getWest());
                TRoom* pER = mpMap->mpRoomDB->getRoom(room->getWest());
                if (pER) {
                    if (pER->getEast() != _id) {
                        oneWayExits.push_back(room->getWest());
                    }
                }
            }
            if (!room->customLines.contains(key_nw)) {
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

                const float ex = room->x * mRoomWidth + mRX;
                const float ey = room->y * mRoomHeight * -1 + mRY;
                QPointF origin = QPointF(ex, ey);
                // The following sets a point offset from the room center
                // that depends on the exit direction that the custom line
                // heads to from the room center - it forms a fixed segment
                // that cannot be moved:
                QPointF fixedOffsetPoint;
                if (itk.key() == key_n) {
                    fixedOffsetPoint = QPointF(ex, ey - mRoomHeight / 2.0);
                } else if (itk.key() == key_ne) {
                    fixedOffsetPoint = QPointF(ex + mRoomWidth / 2.0, ey - mRoomHeight / 2.0);
                } else if (itk.key() == key_e) {
                    fixedOffsetPoint = QPointF(ex + mRoomWidth / 2.0, ey);
                } else if (itk.key() == key_se) {
                    fixedOffsetPoint = QPointF(ex + mRoomWidth / 2.0, ey + mRoomHeight / 2.0);
                } else if (itk.key() == key_s) {
                    fixedOffsetPoint = QPointF(ex, ey + mRoomHeight / 2.0);
                } else if (itk.key() == key_sw) {
                    fixedOffsetPoint = QPointF(ex - mRoomWidth / 2.0, ey + mRoomHeight / 2.0);
                } else if (itk.key() == key_w) {
                    fixedOffsetPoint = QPointF(ex - mRoomWidth / 2.0, ey);
                } else if (itk.key() == key_nw) {
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

                    if (!getUserDataBool(room->userData, ROOM_UI_DONTSHOWAREAEXITNAME, false)) {
                        QPointF areaExitNameOffset = parseOffsetString(room->userData.value(ROOM_UI_AREAEXITNAMEOFFSET + "." + itk.key()));
                        int exitRoomId = -1;
                        QMapIterator<int, QString> otherExitIt = room->getOtherMap();
                        while (otherExitIt.hasNext()) {
                            otherExitIt.next();
                            if (otherExitIt.value().startsWith(QLatin1String("0")) || otherExitIt.value().startsWith(QLatin1String("1"))) {
                                if (otherExitIt.value().mid(1) == itk.key()) {
                                    exitRoomId = otherExitIt.key();
                                    break;
                                }
                            } else if (otherExitIt.value() == itk.key()) {
                                exitRoomId = otherExitIt.key();
                                break;
                            }
                        }
                        if (exitRoomId > -1) {
                            auto pExitRoom = mpMap->mpRoomDB->getRoom(exitRoomId);
                            if (pExitRoom && pExitRoom->getArea() != mAreaID) {
                                QRectF rectExitAreaName(0, 0, 250, 30);
                                QString rAreaName = mpMap->mpRoomDB->getAreaNamesMap().value(pExitRoom->getArea());
                                rectExitAreaName = painter.boundingRect(rectExitAreaName, 0, rAreaName) + QMarginsF(8, 2, 8, 2);

                                QPointF _p = polyLinePoints.last(), p2 = polyLinePoints.at(polyLinePoints.size() - 2);
                                if (abs(_p.x() - p2.x()) < 4 && _p.y() > p2.y()) {    // getSouth
                                    rectExitAreaName.moveTo(_p.x() - rectExitAreaName.width() / 2 + areaExitNameOffset.x(), _p.y() + areaExitNameOffset.y());
                                }
                                else if (abs(_p.x() - p2.x()) < 4 && _p.y() < p2.y()) {  // getNorth
                                    rectExitAreaName.moveTo(_p.x() - rectExitAreaName.width() / 2 + areaExitNameOffset.x(), _p.y() - rectExitAreaName.height() + areaExitNameOffset.y());
                                }
                                else if (_p.x() < p2.x() && abs(_p.y() - p2.y()) < 4) {  // getWest
                                    rectExitAreaName.moveTo(_p.x() - rectExitAreaName.width() + areaExitNameOffset.x(), _p.y() - rectExitAreaName.height() / 2 + areaExitNameOffset.y());
                                }
                                else if (_p.x() > p2.x() && abs(_p.y() - p2.y()) < 4) {  // getEast
                                    rectExitAreaName.moveTo(_p.x() + areaExitNameOffset.x() + areaExitNameOffset.x(), _p.y() - rectExitAreaName.height() / 2 + areaExitNameOffset.y());
                                }
                                else if (_p.x() < p2.x() && _p.y() < p2.y()) {  // getNorthwest
                                    rectExitAreaName.moveTo(_p.x() - rectExitAreaName.width() * 0.75 + areaExitNameOffset.x(), _p.y() - rectExitAreaName.height() + areaExitNameOffset.y());
                                }
                                else if (_p.x() > p2.x() && _p.y() < p2.y()) {  // getNortheast
                                    rectExitAreaName.moveTo(_p.x() - rectExitAreaName.width() * 0.25 + areaExitNameOffset.x(), _p.y() - rectExitAreaName.height() + areaExitNameOffset.y());
                                }
                                else if (_p.x() > p2.x() && _p.y() > p2.y()) {  // getSoutheast
                                    rectExitAreaName.moveTo(_p.x() - rectExitAreaName.width() * 0.25 + areaExitNameOffset.x(), _p.y() + areaExitNameOffset.y());
                                }
                                else if (_p.x() < p2.x() && _p.y() > p2.y()) {  // getSouthwest
                                    rectExitAreaName.moveTo(_p.x() - rectExitAreaName.width() * 0.75 + areaExitNameOffset.x(), _p.y() + areaExitNameOffset.y());
                                }

                                if (!(xyzoom > 40 && rectExitAreaName.width() >= qMax(area_width, area_height) / 3)) {
                                    painter.setPen(mpHost->mFgColor_2);
                                    painter.drawText(rectExitAreaName, Qt::AlignCenter, rAreaName);
                                }
                            }
                        }
                    }

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
        for (int direction : qAsConst(room->exitStubs)) {
            QVector3D uDirection = unitVectors[direction];
            painter.drawLine(
                    rx + rSize * uDirection.x() / 2.0, ry + rSize * uDirection.y(), rx + uDirection.x() * (rSize * mRoomWidth * 3.0 / 4.0), ry + uDirection.y() * (rSize * mRoomHeight * 3.0 / 4.0));
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

            areaExit = pE->getArea() != mAreaID;
            const float ex = pE->x * mRoomWidth + mRX;
            const float ey = pE->y * mRoomHeight * -1 + mRY;
            const int ez = pE->z;

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
                QRectF rectExitAreaName(0, 0, 250, 30);
                QString rAreaName = mpMap->mpRoomDB->getAreaNamesMap().value(pE->getArea());
                rectExitAreaName = painter.boundingRect(rectExitAreaName, 0, rAreaName) + QMarginsF(8, 2, 8, 2);

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
                    QPointF areaExitNameOffset = parseOffsetString(room->userData.value(ROOM_UI_AREAEXITNAMEOFFSET + "." + "south"));
                    rectExitAreaName.moveTo(_line.p1().x() - rectExitAreaName.width() / 2 + areaExitNameOffset.x(), _line.p1().y() + areaExitNameOffset.y());
                } else if (room->getNorth() == rID) {
                    _line = QLine(p2.x(), p2.y() - mRoomHeight, p2.x(), p2.y());
                    _p = QPoint(p2.x(), p2.y() - mRoomHeight / 2.0);
                    QPointF areaExitNameOffset = parseOffsetString(room->userData.value(ROOM_UI_AREAEXITNAMEOFFSET + "." + "north"));
                    rectExitAreaName.moveTo(_line.p1().x() - rectExitAreaName.width() / 2 + areaExitNameOffset.x(), _line.p1().y() - rectExitAreaName.height() + areaExitNameOffset.y());
                } else if (room->getWest() == rID) {
                    _line = QLine(p2.x() - mRoomWidth, p2.y(), p2.x(), p2.y());
                    _p = QPoint(p2.x() - mRoomWidth / 2.0, p2.y());
                    QPointF areaExitNameOffset = parseOffsetString(room->userData.value(ROOM_UI_AREAEXITNAMEOFFSET + "." + "west"));
                    rectExitAreaName.moveTo(_line.p1().x() - rectExitAreaName.width() + areaExitNameOffset.x(), _line.p1().y() - rectExitAreaName.height() / 2 + areaExitNameOffset.y());
                } else if (room->getEast() == rID) {
                    _line = QLine(p2.x() + mRoomWidth, p2.y(), p2.x(), p2.y());
                    _p = QPoint(p2.x() + mRoomWidth / 2.0, p2.y());
                    QPointF areaExitNameOffset = parseOffsetString(room->userData.value(ROOM_UI_AREAEXITNAMEOFFSET + "." + "east"));
                    rectExitAreaName.moveTo(_line.p1().x() + areaExitNameOffset.x(), _line.p1().y() - rectExitAreaName.height() / 2 + areaExitNameOffset.y());
                } else if (room->getNorthwest() == rID) {
                    _line = QLine(p2.x() - mRoomWidth, p2.y() - mRoomHeight, p2.x(), p2.y());
                    _p = QPoint(p2.x() - mRoomWidth / 2.0, p2.y() - mRoomHeight / 2.0);
                    QPointF areaExitNameOffset = parseOffsetString(room->userData.value(ROOM_UI_AREAEXITNAMEOFFSET + "." + "northwest"));
                    rectExitAreaName.moveTo(_line.p1().x() - rectExitAreaName.width() * 0.75 + areaExitNameOffset.x(), _line.p1().y() - rectExitAreaName.height() + areaExitNameOffset.y());
                } else if (room->getNortheast() == rID) {
                    _line = QLine(p2.x() + mRoomWidth, p2.y() - mRoomHeight, p2.x(), p2.y());
                    _p = QPoint(p2.x() + mRoomWidth / 2.0, p2.y() - mRoomHeight / 2.0);
                    QPointF areaExitNameOffset = parseOffsetString(room->userData.value(ROOM_UI_AREAEXITNAMEOFFSET + "." + "northeast"));
                    rectExitAreaName.moveTo(_line.p1().x() - rectExitAreaName.width() * 0.25 + areaExitNameOffset.x(), _line.p1().y() - rectExitAreaName.height() + areaExitNameOffset.y());
                } else if (room->getSoutheast() == rID) {
                    _line = QLine(p2.x() + mRoomWidth, p2.y() + mRoomHeight, p2.x(), p2.y());
                    _p = QPoint(p2.x() + mRoomWidth / 2.0, p2.y() + mRoomHeight / 2.0);
                    QPointF areaExitNameOffset = parseOffsetString(room->userData.value(ROOM_UI_AREAEXITNAMEOFFSET + "." + "southeast"));
                    rectExitAreaName.moveTo(_line.p1().x() - rectExitAreaName.width() * 0.25 + areaExitNameOffset.x(), _line.p1().y() + areaExitNameOffset.y());
                } else if (room->getSouthwest() == rID) {
                    _line = QLine(p2.x() - mRoomWidth, p2.y() + mRoomHeight, p2.x(), p2.y());
                    _p = QPoint(p2.x() - mRoomWidth / 2.0, p2.y() + mRoomHeight / 2.0);
                    QPointF areaExitNameOffset = parseOffsetString(room->userData.value(ROOM_UI_AREAEXITNAMEOFFSET + "." + "southwest"));
                    rectExitAreaName.moveTo(_line.p1().x() - rectExitAreaName.width() * 0.75 + areaExitNameOffset.x(), _line.p1().y() + areaExitNameOffset.y());
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

                if (!getUserDataBool(room->userData, ROOM_UI_DONTSHOWAREAEXITNAME, false)) {
                    if (!(xyzoom > 40 && rectExitAreaName.width() >= qMax(area_width, area_height) / 3)) {
                        painter.setPen(mpHost->mFgColor_2);
                        painter.drawText(rectExitAreaName, Qt::AlignCenter, rAreaName);
                    }
                }

                painter.setPen(__pen);
            }
            // doors
            if (!room->doors.empty()) {
                int doorStatus = 0;
                if (room->getSouth() == rID && room->doors.contains(key_s)) {
                    doorStatus = room->doors[key_s];
                } else if (room->getNorth() == rID && room->doors.contains(key_n)) {
                    doorStatus = room->doors[key_n];
                } else if (room->getSouthwest() == rID && room->doors.contains(key_sw)) {
                    doorStatus = room->doors[key_sw];
                } else if (room->getSoutheast() == rID && room->doors.contains(key_se)) {
                    doorStatus = room->doors[key_se];
                } else if (room->getNortheast() == rID && room->doors.contains(key_ne)) {
                    doorStatus = room->doors[key_ne];
                } else if (room->getNorthwest() == rID && room->doors.contains(key_nw)) {
                    doorStatus = room->doors[key_nw];
                } else if (room->getWest() == rID && room->doors.contains(key_w)) {
                    doorStatus = room->doors[key_w];
                } else if (room->getEast() == rID && room->doors.contains(key_e)) {
                    doorStatus = room->doors[key_e];
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
            const float roomRadius = mRoomWidth * 1.2;
            const float roomDiagonal = mRoomWidth * 1.2;
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
    } // end of loop for every room in area
}

// Work out text for information box, need to offset if room selection widget is present
void T2DMap::paintMapInfo(const QElapsedTimer& renderTimer, QPainter& painter, const bool showingCurrentArea, QColor& infoColor)
{
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
        int areaId = _prid->getArea();
        TArea* area = mpMap->mpRoomDB->getArea(areaId);
        QString areaName = mpMap->mpRoomDB->getAreaNamesMap().value(areaId);
        if (area) {
            infoText = tr("Area:%1%2 ID:%1%3 x:%1%4%1<‑>%1%5 y:%1%6%1<‑>%1%7 z:%1%8%1<‑>%1%9\n",
                          // Intentional separator
                          "This text uses non-breaking spaces (as '%1's, as Qt Creator cannot handle"
                          "them literally in raw strings) and non-breaking hyphens which are used to "
                          "prevent the line being split at some places it might otherwise be; when "
                          "translating please consider at which points the text may be divided to fit onto "
                          "more than one line. "
                          "%2 is the (text) name of the area, %3 is the number for it, "
                          "%4 to %9 are pairs (min <-> max) of extremes for each of x,y and z coordinates")
                               .arg(QChar(160),
                                    areaName,
                                    QString::number(areaId),
                                    QString::number(area->min_x),
                                    QString::number(area->max_x),
                                    QString::number(area->min_y),
                                    QString::number(area->max_y),
                                    QString::number(area->min_z),
                                    QString::number(area->max_z));
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
            infoText.append(tr("Room%1ID:%1%2 Position%1on%1Map: (%3,%4,%5) ‑%1current player location\n",
                               // Intentional comment to separate arguments
                               "This text uses non-breaking spaces (as '%1's, as Qt Creator cannot handle"
                               "them literally in raw strings) and a non-breaking hyphen which are used to "
                               "prevent the line being split at some places it might otherwise be; when "
                               "translating please consider at which points the text may be divided to fit onto "
                               "more than one line. "
                               "This text is for when NO rooms are selected, %3 is the room number "
                               "of, and %4-%6 are the x,y and z coordinates for, the current player's room.")
                                    .arg(QChar(160), QString::number(roomID), QString::number(_prid->x), QString::number(_prid->y), QString::number(_prid->z)));
            if (!showingCurrentArea) {
                f.setItalic(true);
            } else {
                f.setBold(true);
            }
            break;
        case 1:
            infoText.append(tr("Room%1ID:%1%2 Position%1on%1Map: (%3,%4,%5) ‑%1selected room\n",
                               // Intentional comment to separate arguments
                               "This text uses non-breaking spaces (as '%1's, as Qt Creator cannot handle"
                               "them literally in raw strings) and a non-breaking hyphen which are used to "
                               "prevent the line being split at some places it might otherwise be; when "
                               "translating please consider at which points the text may be divided to fit onto "
                               "more than one line. "
                               "This text is for when ONE room is selected, %3 is the room number "
                               "of, and %4-%6 are the x,y and z coordinates for, the selected Room.")
                                    .arg(QChar(160), QString::number(roomID), QString::number(_prid->x), QString::number(_prid->y), QString::number(_prid->z)));
            f.setBold(true);
            if (infoColor.lightness() > 127) {
                infoColor = QColor(255, 223, 191); // Slightly orange white
            } else {
                infoColor = QColor(96, 48, 0); // Dark, slightly orange grey
            }
            break;
        default:
            infoText.append(tr("Room%1ID:%1%2 Position%1on%1Map: (%3,%4,%5) ‑%1center of %n selected rooms\n",
                               // Intentional comment to separate arguments
                               "This text uses non-breaking spaces (as '%1's, as Qt Creator cannot handle"
                               "them literally in raw strings) and a non-breaking hyphen which are used to "
                               "prevent the line being split at some places it might otherwise be; when "
                               "translating please consider at which points the text may be divided to fit onto "
                               "more than one line. "
                               "This text is for when TWO or MORE rooms are selected; %1 is the room "
                               "number for which %2-%4 are the x,y and z coordinates of the room nearest the "
                               "middle of the selection. This room has the yellow cross-hairs. %n is the count "
                               "of rooms selected and will ALWAYS be greater than 1 in this situation. It is "
                               "provided so that non-English translations can select required plural forms as "
                               "needed.",
                               selectionSize)
                                    .arg(QChar(160), QString::number(roomID), QString::number(_prid->x), QString::number(_prid->y), QString::number(_prid->z)));
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
    infoText.append(tr("render time: %1S mO: (%2,%3,%4)",
                       // Intentional comment to separate arguments
                       "This is debug information that is not expected to be seen in release versions, "
                       "%1 is a decimal time period and %2-%4 are the x,y and z coordinates at the "
                       "center of the view (but y will be negative compared to previous room related "
                       "ones as it represents the real coordinate system for this widget which has "
                       "y increasing in a downward direction!)")
                            .arg(renderTimer.nsecsElapsed() * 1.0e-9, 0, 'f', 3)
                            .arg(QString::number(mOx), QString::number(mOy), QString::number(mOz)));
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
    TMapLabel label;
    QFont font;
    QString text = tr("no text", "Default text if a label is created in mapper with no text");
    QString imagePath;

    mHelpMsg.clear();

    QMessageBox textOrImageDialog;
    textOrImageDialog.setText(tr("Text label or image label?", "2D Mapper create label dialog text"));
    QPushButton* textButton = textOrImageDialog.addButton(tr("Text Label", "2D Mapper create label dialog button"), QMessageBox::ActionRole);
    QPushButton* imageButton = textOrImageDialog.addButton(tr("Image Label", "2D Mapper create label dialog button"), QMessageBox::ActionRole);
    textOrImageDialog.setStandardButtons(QMessageBox::Cancel);
    textOrImageDialog.exec();
    if (textOrImageDialog.clickedButton() == textButton) {
        QString title = tr("Enter label text.", "2D Mapper create label dialog title/text");
        font = QFontDialog::getFont(nullptr);
        text = QInputDialog::getText(nullptr, title, title);
        if (text.length() < 1) {
            text = tr("no text", "Default text if a label is created in mapper with no text");
        }
        label.text = text;
        label.bgColor = QColorDialog::getColor(QColor(50, 50, 150, 100), nullptr, tr("Background color", "2D Mapper create label color dialog title"));
        label.fgColor = QColorDialog::getColor(QColor(255, 255, 50, 255), nullptr, tr("Foreground color", "2D Mapper create label color dialog title"));
    } else if (textOrImageDialog.clickedButton() == imageButton) {
       label.bgColor = QColor(50, 50, 150, 100);
        label.text.clear();
        imagePath = QFileDialog::getOpenFileName(nullptr, tr("Select image", "2D Mapper create label file dialog title"));
    } else {
        return;
    }

    QMessageBox backgroundOrForegroundDialog;
    backgroundOrForegroundDialog.setStandardButtons(QMessageBox::Cancel);
    backgroundOrForegroundDialog.setText(tr("Draw label as background or on top of everything?", "2D Mapper create label dialog text"));
    QPushButton* backgroundButton = backgroundOrForegroundDialog.addButton(tr("Background", "2D Mapper create label dialog button"), QMessageBox::ActionRole);
    QPushButton* foregroundButton = backgroundOrForegroundDialog.addButton(tr("Foreground", "2D Mapper create label dialog button"), QMessageBox::ActionRole);
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
    //pixmap.fill(QColor(0, 255, 0, 0));
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
//        auto* ke = static_cast<QKeyEvent*>(event);
//        if (ke->key() == Qt::Key_Delete ) {
//            if (mCustomLineSelectedRoom != 0  ) {
//                if (mpMap->rooms.contains(mCustomLineSelectedRoom)) {
//                    TRoom * pR = mpMap->rooms[mCustomLineSelectedRoom];
//                    if (pR->customLines.contains( mCustomLineSelectedExit)) {
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
    mudlet::self()->activateProfile(mpHost);
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
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
                                if ((line.intersects(tl, &pi) == QLineF::BoundedIntersection) || (line.intersects(tl2, &pi) == QLineF::BoundedIntersection)) {
#else
                                if ((line.intersect(tl, &pi) == QLineF::BoundedIntersection) || (line.intersect(tl2, &pi) == QLineF::BoundedIntersection)) {
#endif
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
                mHelpMsg = tr("Drag to select multiple rooms or labels, release to finish...", "2D Mapper big, bottom of screen help message");
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
                if ((abs(mx - rx) < qRound(mRoomWidth * rSize / 2.0)) && (abs(my - ry) < qRound(mRoomHeight * rSize / 2.0)) && (mz == rz)) {
                    if (mMultiSelectionSet.contains(currentAreaRoom) && event->modifiers().testFlag(Qt::ControlModifier)) {
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
                mMultiSelectionHighlightRoomId = *(mMultiSelectionSet.begin());
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

            if (mMultiSelection && !mMultiSelectionSet.empty() && (event->modifiers().testFlag(Qt::ControlModifier))) {
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
                _item->setText(0, key_plain.arg(multiSelectionRoomId, mMaxRoomIdDigits));
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
            mMultiSelectionListWidget.sortByColumn(mIsSelectionSortByNames ? 1 : 0, Qt::AscendingOrder);
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
                auto customLineUndoLastPoint = new QAction(tr("Undo", "2D Mapper context menu (drawing custom exit line) item"), this);
                customLineUndoLastPoint->setToolTip(tr("Undo last point", "2D Mapper context menu (drawing custom exit line) item tooltip"));
                if (room->customLines.value(mCustomLinesRoomExit).count() > 1) {
                    connect(customLineUndoLastPoint, &QAction::triggered, this, &T2DMap::slot_undoCustomLineLastPoint);
                } else {
                    customLineUndoLastPoint->setEnabled(false);
                }

                auto customLineProperties = new QAction(tr("Properties", "2D Mapper context menu (drawing custom exit line) item name (but not used as display text as that is set separately)"), this);
                customLineProperties->setText(
                        tr("properties...", "2D Mapper context menu (drawing custom exit line) item display text (has to be entered separately as the ... would get stripped off otherwise)"));
                customLineProperties->setToolTip(tr("Change the properties of this line", "2D Mapper context menu (drawing custom exit line) item tooltip"));
                connect(customLineProperties, &QAction::triggered, this, &T2DMap::slot_customLineProperties);

                auto customLineFinish = new QAction(tr("Finish", "2D Mapper context menu (drawing custom exit line) item"), this);
                customLineFinish->setToolTip(tr("Finish drawing this line", "2D Mapper context menu (drawing custom exit line) item tooltip"));
                connect(customLineFinish, &QAction::triggered, this, &T2DMap::slot_doneCustomLine);

                mPopupMenu = true;

                room->calcRoomDimensions();
                popup->addAction(customLineUndoLastPoint);
                popup->addAction(customLineProperties);
                popup->addAction(customLineFinish);

                popup->popup(mapToGlobal(event->pos()));
                update();
                return;
            }
        }

        if (!mLabelHighlighted && mCustomLineSelectedRoom == 0) {
            auto playerRoom = mpMap->mpRoomDB->getRoom(mpMap->mRoomIdHash.value(mpMap->mProfileName));
            auto pArea = mpMap->mpRoomDB->getArea(mAreaID);

            if (!playerRoom || !pArea) {
                auto createMap = new QAction(tr("Create new map", "2D Mapper context menu (no map found) item"), this);
                connect(createMap, &QAction::triggered, this, &T2DMap::slot_newMap);

                auto loadMap = new QAction(tr("Load map", "2D Mapper context menu (no map found) item"), this);
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

            auto moveRoom = new QAction(tr("Move", "2D Mapper context menu (room) item"), this);
            moveRoom->setToolTip(tr("Move room", "2D Mapper context menu (room) item tooltip"));
            connect(moveRoom, &QAction::triggered, this, &T2DMap::slot_moveRoom);

            auto deleteRoom = new QAction(tr("Delete", "2D Mapper context menu (room) item"), this);
            deleteRoom->setToolTip(tr("Delete room", "2D Mapper context menu (room) item tooltip"));
            connect(deleteRoom, &QAction::triggered, this, &T2DMap::slot_deleteRoom);

            auto recolorRoom = new QAction(tr("Color", "2D Mapper context menu (room) item"), this);
            recolorRoom->setToolTip(tr("Change room color", "2D Mapper context menu (room) item tooltip"));
            connect(recolorRoom, &QAction::triggered, this, &T2DMap::slot_changeColor);

            auto spreadRooms = new QAction(tr("Spread", "2D Mapper context menu (room) item"), this);
            spreadRooms->setToolTip(tr("Increase map X-Y spacing for the selected group of rooms", "2D Mapper context menu (room) item tooltip"));
            connect(spreadRooms, &QAction::triggered, this, &T2DMap::slot_spread);

            auto shrinkRooms = new QAction(tr("Shrink", "2D Mapper context menu (room) item"), this);
            shrinkRooms->setToolTip(tr("Decrease map X-Y spacing for the selected group of rooms", "2D Mapper context menu (room) item tooltip"));
            connect(shrinkRooms, &QAction::triggered, this, &T2DMap::slot_shrink);

            auto lockRoom = new QAction(tr("Lock", "2D Mapper context menu (room) item"), this);
            lockRoom->setToolTip(tr("Lock room for speed walks", "2D Mapper context menu (room) item tooltip"));
            connect(lockRoom, &QAction::triggered, this, &T2DMap::slot_lockRoom);

            auto unlockRoom = new QAction(tr("Unlock", "2D Mapper context menu (room) item"), this);
            unlockRoom->setToolTip(tr("Unlock room for speed walks", "2D Mapper context menu (room) item tooltip"));
            connect(unlockRoom, &QAction::triggered, this, &T2DMap::slot_unlockRoom);

            auto weightRoom = new QAction(tr("Weight", "2D Mapper context menu (room) item"), this);
            weightRoom->setToolTip(tr("Set room weight", "2D Mapper context menu (room) item tooltip"));
            connect(weightRoom, &QAction::triggered, this, &T2DMap::slot_setRoomWeight);

            auto roomExits = new QAction(tr("Exits", "2D Mapper context menu (room) item"), this);
            roomExits->setToolTip(tr("Set room exits", "2D Mapper context menu (room) item tooltip"));
            connect(roomExits, &QAction::triggered, this, &T2DMap::slot_setExits);

            auto roomSymbol = new QAction(tr("Symbol", "2D Mapper context menu (room) item"), this);
            roomSymbol->setToolTip(tr("Set one or more symbols or letters to mark special rooms", "2D Mapper context menu (room) item tooltip"));
            connect(roomSymbol, &QAction::triggered, this, &T2DMap::slot_setSymbol);

            auto moveRoomXY = new QAction(tr("Move to", "2D Mapper context menu (room) item"), this);
            moveRoomXY->setToolTip(tr("Move selected group to a given position", "2D Mapper context menu (room) item tooltip"));
            connect(moveRoomXY, &QAction::triggered, this, &T2DMap::slot_movePosition);

            auto roomArea = new QAction(tr("Area", "2D Mapper context menu (room) item"), this);
            roomArea->setToolTip(tr("Set room's area number", "2D Mapper context menu (room) item tooltip"));
            connect(roomArea, &QAction::triggered, this, &T2DMap::slot_setArea);

            auto customExitLine = new QAction(tr("Custom exit line", "2D Mapper context menu (room) item"), this);
            if (pArea && !pArea->gridMode) {
                customExitLine->setToolTip(tr("Replace an exit line with a custom line", "2D Mapper context menu (room) item tooltip (enabled state)"));
                connect(customExitLine, &QAction::triggered, this, &T2DMap::slot_setCustomLine);
            } else {
                // Disable custom exit lines in grid mode as they aren't visible anyway
                customExitLine->setToolTip(tr("Custom exit lines are not shown and are not editable in grid mode", "2D Mapper context menu (room) item tooltip (disabled state)"));
                customExitLine->setEnabled(false);
            }

            auto createLabel = new QAction(tr("Create Label", "2D Mapper context menu (room) item"), this);
            createLabel->setToolTip(tr("Create labels to show text or images", "2D Mapper context menu (room) item tooltip"));
            connect(createLabel, &QAction::triggered, this, &T2DMap::slot_createLabel);

            auto setPlayerLocation = new QAction(tr("Set location", "2D Mapper context menu (room) item"), this);
            if (mMultiSelectionSet.size() == 1) { // Only enable if ONE room is highlighted
                setPlayerLocation->setToolTip(tr("Set player current location to here", "2D Mapper context menu (room) item tooltip (enabled state)"));
                connect(setPlayerLocation, &QAction::triggered, this, &T2DMap::slot_setPlayerLocation);
            } else {
                setPlayerLocation->setEnabled(false);
                setPlayerLocation->setToolTip(tr("Can only set location when exactly one room is selected", "2D Mapper context menu (room) item tooltip (disabled state)"));
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
            auto moveLabel = new QAction(tr("Move", "2D Mapper context menu (label) item"), this);
            moveLabel->setToolTip(tr("Move label", "2D Mapper context menu item (label) tooltip"));
            connect(moveLabel, &QAction::triggered, this, &T2DMap::slot_moveLabel);
            auto deleteLabel = new QAction(tr("Delete", "2D Mapper context menu (label) item"), this);
            deleteLabel->setToolTip(tr("Delete label", "2D Mapper context menu (label) item tooltip"));
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
                    auto addPoint = new QAction(tr("Add point", "2D Mapper context menu (custom line editing) item"), this);
                    if (mCustomLineSelectedPoint > -1)
                    // The first user manipulable point IS zero - line is
                    // drawn to it from a point around room symbol dependent
                    // on the exit direction - and we can now add even to it
                    {
                        connect(addPoint, &QAction::triggered, this, &T2DMap::slot_customLineAddPoint);
                        addPoint->setToolTip(tr("Divide segment by adding a new point mid-way along", "2D Mapper context menu (custom line editing) item tooltip (enabled state)"));
                    } else {
                        addPoint->setEnabled(false);
                        addPoint->setToolTip(tr("Select a point first, then add a new point mid-way along the segment towards room",
                                                "2D Mapper context menu (custom line editing) item tooltip (disabled state, i.e must do the suggested action first)"));
                    }

                    auto removePoint = new QAction(tr("Remove point", "2D Mapper context menu (custom line editing) item"), this);
                    // Permit this to be enabled if the current point is 0 or
                    // greater, but not if there is no others
                    if (mCustomLineSelectedPoint > -1) {
                        if (room->customLines.value(mCustomLineSelectedExit).count() > 1) {
                            connect(removePoint, &QAction::triggered, this, &T2DMap::slot_customLineRemovePoint);
                            if ((mCustomLineSelectedPoint + 1) < room->customLines.value(mCustomLineSelectedExit).count()) {
                                removePoint->setToolTip(tr("Merge pair of segments by removing this point",
                                                           "2D Mapper context menu (custom line editing) item tooltip (enabled state but will be able to be done again on this item)"));

                            } else {
                                removePoint->setToolTip(tr("Remove last segment by removing this point",
                                                           "2D Mapper context menu (custom line editing) item tooltip (enabled state but is the last time this action can be done on this item)"));
                            }
                        } else {
                            removePoint->setEnabled(false);
                            removePoint->setToolTip(tr(
                                    R"(use "delete line" to remove the only segment ending in an editable point)",
                                    R"(2D Mapper context menu (custom line editing) item tooltip (disabled state this action can not be done again on this item but something else can be the quoted action "delete line" should match the translation for that action))"));
                        }
                    } else {
                        removePoint->setEnabled(false);
                        removePoint->setToolTip(tr("Select a point first, then remove it",
                                                   "2D Mapper context menu (custom line editing) item tooltip (disabled state, user will need to do something before it can be used)"));
                    }

                    auto lineProperties = new QAction(tr("Properties", "2D Mapper context menu (custom line editing) item name (but not used as display text as that is set separately)"), this);
                    // Changed seperately, because the constructor silently copies the text elsewhere
                    // (tooltip and/or object name IIRC) whereas the ellipsis is meant only for display
                    lineProperties->setText(
                            tr("properties...", "2D Mapper context menu (custom line editing) item display text (has to be entered separately as the ... would get stripped off otherwise"));
                    lineProperties->setToolTip(tr("Change the properties of this custom line"));
                    connect(lineProperties, &QAction::triggered, this, &T2DMap::slot_customLineProperties);

                    auto deleteLine = new QAction(tr("Delete line", "2D Mapper context menu (custom line editing) item"), this);
                    deleteLine->setToolTip(tr("Delete all of this custom line", "2D Mapper context menu (custom line editing) item tooltip"));
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
            auto action = new QAction(actionInfo.at(2), this);
            if (actionInfo.at(1).isEmpty()) { //no parent
                popup->addAction(action);
            } else if (userMenus.contains(actionInfo.at(1))) {
                userMenus[actionInfo[1]]->addAction(action);
            } else {
                delete action;
                continue;
            }
            mapper->setMapping(action, it2.key());
            // TODO: QSignalMapper is not compatible with the functor (Qt5)
            // style of QObject::connect(...) - it has been declared obsolete
            // and should be replaced with lambda functions to perform what the
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

    return std::make_pair(std::round(mx), std::round(my));
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
#if defined(INCLUDE_3DMAPPER)
    if (mpHost->mpMap->mpM) {
        mpHost->mpMap->mpM->update();
    }
#endif
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

            QFile file(QStringLiteral(":/ui/custom_lines_properties.ui"));
            file.open(QFile::ReadOnly);
            auto* dialog = qobject_cast<QDialog*>(loader.load(&file, this));
            file.close();
            if (!dialog) {
                qWarning("T2DMap::slot_customLineProperties() ERROR: failed to create the dialog!");
                return;
            }
            dialog->setWindowIcon(QIcon(QStringLiteral(":/icons/mudlet_custom_exit_properties.png")));
            auto* le_toId = dialog->findChild<QLineEdit*>(QStringLiteral("toId"));
            auto* le_fromId = dialog->findChild<QLineEdit*>(QStringLiteral("fromId"));
            auto* le_cmd = dialog->findChild<QLineEdit*>(QStringLiteral("cmd"));

            mpCurrentLineStyle = dialog->findChild<QComboBox*>(QStringLiteral("lineStyle"));
            mpCurrentLineColor = dialog->findChild<QPushButton*>(QStringLiteral("lineColor"));
            mpCurrentLineArrow = dialog->findChild<QCheckBox*>(QStringLiteral("arrow"));
            if (!le_toId || !le_cmd || !le_fromId || !mpCurrentLineStyle || !mpCurrentLineColor || !mpCurrentLineArrow) {
                qWarning("T2DMap::slot_customLineProperties() ERROR: failed to find an element in the dialog!");
                return;
            }
            le_cmd->setText(exit);
            le_fromId->setText(QString::number(room->getId()));
            if (exit == key_nw) {
                le_toId->setText(QString::number(room->getNorthwest()));
            } else if (exit == key_n) {
                le_toId->setText(QString::number(room->getNorth()));
            } else if (exit == key_ne) {
                le_toId->setText(QString::number(room->getNortheast()));
            } else if (exit == key_up) {
                le_toId->setText(QString::number(room->getUp()));
            } else if (exit == key_w) {
                le_toId->setText(QString::number(room->getWest()));
            } else if (exit == key_e) {
                le_toId->setText(QString::number(room->getEast()));
            } else if (exit == key_down) {
                le_toId->setText(QString::number(room->getDown()));
            } else if (exit == key_sw) {
                le_toId->setText(QString::number(room->getSouthwest()));
            } else if (exit == key_s) {
                le_toId->setText(QString::number(room->getSouth()));
            } else if (exit == key_se) {
                le_toId->setText(QString::number(room->getSoutheast()));
            } else if (exit == key_in) {
                le_toId->setText(QString::number(room->getIn()));
            } else if (exit == key_out) {
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
            mpCurrentLineStyle->addItem(QIcon(QPixmap(key_icon_line_solid)), tr("Solid line"), static_cast<int>(Qt::SolidLine));
            mpCurrentLineStyle->addItem(QIcon(QPixmap(key_icon_line_dot)), tr("Dot line"), static_cast<int>(Qt::DotLine));
            mpCurrentLineStyle->addItem(QIcon(QPixmap(key_icon_line_dash)), tr("Dash line"), static_cast<int>(Qt::DashLine));
            mpCurrentLineStyle->addItem(QIcon(QPixmap(key_icon_line_dashDot)), tr("Dash-dot line"), static_cast<int>(Qt::DashDotLine));
            mpCurrentLineStyle->addItem(QIcon(QPixmap(key_icon_line_dashDotDot)), tr("Dash-dot-dot line"), static_cast<int>(Qt::DashDotDotLine));
            Qt::PenStyle lineStyle = room->customLinesStyle.value(exit);
            mpCurrentLineStyle->setCurrentIndex(mpCurrentLineStyle->findData(static_cast<int>(lineStyle)));

            mpCurrentLineArrow->setChecked(room->customLinesArrow.value(exit));
            mCurrentLineColor = room->customLinesColor.value(exit);

            mpCurrentLineColor->setStyleSheet(key_styleSheet_backgroundColor.arg(mCurrentLineColor.name()));
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
        if (mCustomLineSelectedExit == key_n) {
            customLineStartPoint = QPointF(room->x, room->y + 0.5);
        } else if (mCustomLineSelectedExit == key_s) {
            customLineStartPoint = QPointF(room->x, room->y - 0.5);
        } else if (mCustomLineSelectedExit == key_e) {
            customLineStartPoint = QPointF(room->x + 0.5, room->y);
        } else if (mCustomLineSelectedExit == key_w) {
            customLineStartPoint = QPointF(room->x - 0.5, room->y);
        } else if (mCustomLineSelectedExit == key_ne) {
            customLineStartPoint = QPointF(room->x + 0.5, room->y + 0.5);
        } else if (mCustomLineSelectedExit == key_nw) {
            customLineStartPoint = QPointF(room->x - 0.5, room->y + 0.5);
        } else if (mCustomLineSelectedExit == key_se) {
            customLineStartPoint = QPointF(room->x + 0.5, room->y - 0.5);
        } else if (mCustomLineSelectedExit == key_sw) {
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
        mpMap->mRoomIdHash[mpMap->mProfileName] = _newRoomId;
        mpMap->mNewMove = true;
        TEvent manualSetEvent {};
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
    TEvent event {};
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
    } else {
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
    QLabel* pLa0 = new QLabel(tr("Move the selection, centered on\n"
                                 "the highlighted room (%1) to:",
                                 // Intentional comment to separate arguments
                                 "Use linefeeds as necessary to format text into a reasonable rectangle of text, "
                                 "%1 is a room number")
                              .arg(mMultiSelectionHighlightRoomId));
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
    pB_ok->setText(tr("OK", "dialog (room(s) move) button"));
    boxLayout->addWidget(pB_ok);
    connect(pB_ok, &QAbstractButton::clicked, dialog, &QDialog::accept);

    auto pB_abort = new QPushButton(pButtonBar);
    pB_abort->setText(tr("Cancel", "dialog (room(s) move) button"));
    connect(pB_abort, &QAbstractButton::clicked, dialog, &QDialog::reject);
    boxLayout->addWidget(pB_abort);
    gridLayout->addWidget(pButtonBar, 4, 0, 1, 2, Qt::AlignCenter);

    if (!qApp->testAttribute(Qt::AA_DontShowIconsInMenus)) {
        pB_ok->setIcon(QIcon::fromTheme(key_dialog_ok_apply, QIcon(key_icon_dialog_ok_apply)));
        pB_abort->setIcon(QIcon::fromTheme(key_dialog_cancel, QIcon(key_icon_dialog_cancel)));
    }

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
                                              tr("Enter room symbol",
                                                 "Title for room symbol selection/entry dialog (used in three places)"),
                                              tr("Enter the symbol to use\n"
                                                 "for this/these %n room(s):",
                                                 // Intentional comment to separate arguments!
                                                 "this is for when applying a new room symbol to one or more rooms "
                                                 "and none have a symbol at present; use line feeds to format text "
                                                 "into a reasonable rectangle, %n is the number of rooms involved",
                                                 mMultiSelectionSet.size()),
                                              QLineEdit::Normal,
                                              QString(),
                                              & isOk,
                                              Qt::Dialog);
        } else if (usedSymbols.size() == 1) {
            newSymbol = QInputDialog::getText(this,
                                              tr("Enter room symbol",
                                                 "Title for room symbol selection/entry dialog (used in three places)"),
                                              // Intentional comment to separate arguments!
                                              ( mMultiSelectionSet.size() > 1 )
                                                  ? tr("The only used symbol is \"%1\" in one or\n"
                                                       "more of the selected %n room(s), delete this to\n"
                                                       "clear it from all selected rooms or replace\n"
                                                       "with a new symbol to use for all the rooms:",
                                                       // Intentional comment to separate arguments!
                                                       "This is for when applying a new room symbol to one or more rooms "
                                                       "and some have the SAME symbol (others may have none) at present, "
                                                       "%n is the total number of rooms involved and is at least two. "
                                                       "Use line feeds to format text into a reasonable rectangle.",
                                                       mMultiSelectionSet.size())
                                                    .arg(usedSymbols.keys().first())
                                                  : tr("The symbol is \"%1\" in the selected room,\n"
                                                       "delete this to clear the symbol or replace\n"
                                                       "it with a new symbol for this room:",
                                                       // Intentional comment to separate arguments!
                                                       "This is for when applying a new room symbol to one room. "
                                                       "Use line feeds to format text into a reasonable rectangle.")
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
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
            QList<unsigned int> symbolCountsList{symbolCountsSet.begin(), symbolCountsSet.end()};
#else
            QList<unsigned int> symbolCountsList{symbolCountsSet.toList()};
#endif
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
                                                 // Intentional comment to separate arguments
                                                 "Everything after the first parameter (the '%1') will be removed by processing "
                                                 "it as a QRegularExpression programmatically, ensure the translated text has "
                                                 "` {` immediately after the '%1', and '}' as the very last character, so that the "
                                                 "right portion can be extracted if the user clicks on this item when it is shown "
                                                 "in the QComboBox it is put in.")
                                              .arg(itSymbolUsed.key())
                                              .arg(QString::number(itSymbolUsed.value())));
                    }
                }
            }
            newSymbol = QInputDialog::getItem(this,                    // QWidget * parent
                                              tr("Enter room symbol",
                                                 "Title for room symbol selection/entry dialog (used in three places)"),
                                              tr("Choose:\n"
                                                 " • an existing symbol from the list below (sorted by most commonly used first)\n"
                                                 " • enter one or more graphemes (\"visible characters\") as a new symbol\n"
                                                 " • enter a space to clear any existing symbols\n"
                                                 "for all of the %n selected room(s):",
                                                 // Intentional comment to separate arguments!
                                                 "Use line feeds to format text into a reasonable rectangle if needed, "
                                                 "%n is the number of rooms involved.",
                                                 mMultiSelectionSet.size()), // const QString & label
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
    pB_ok->setText(tr("OK", "dialog (room(s) change color) button"));
    hboxLayout->addWidget(pB_ok);
    connect(pB_ok, &QAbstractButton::clicked, dialog, &QDialog::accept);

    auto pB_abort = new QPushButton(pButtonBar);
    pB_abort->setText(tr("Cancel", "dialog (room(s) change color) button"));
    connect(pB_abort, &QAbstractButton::clicked, dialog, &QDialog::reject);
    hboxLayout->addWidget(pB_abort);
    vboxLayout->addWidget(pButtonBar);

    if (!qApp->testAttribute(Qt::AA_DontShowIconsInMenus)) {
        pB_ok->setIcon(QIcon::fromTheme(key_dialog_ok_apply, QIcon(key_icon_dialog_ok_apply)));
        pB_abort->setIcon(QIcon::fromTheme(key_dialog_cancel, QIcon(key_icon_dialog_cancel)));
    }

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
                                      1000,       // Maximum value
                                      1,          // Step
                                      &isOk);
    if (spread == 1 || !isOk) {
        return;
    }

    mMultiRect = QRect(0, 0, 0, 0);
    int dx = pR_centerRoom->x;
    int dy = pR_centerRoom->y;
    QSetIterator<int> itSelectionRoom = mMultiSelectionSet;
    while (itSelectionRoom.hasNext()) {
        TRoom* pMovingR = mpMap->mpRoomDB->getRoom(itSelectionRoom.next());
        if (!pMovingR) {
            continue;
        }

        pMovingR->x = (pMovingR->x - dx) * spread + dx;
        pMovingR->y = (pMovingR->y - dy) * spread + dy;
        QMapIterator<QString, QList<QPointF>> itCustomLine(pMovingR->customLines);
        QMap<QString, QList<QPointF>> newCustomLinePointsMap;
        while (itCustomLine.hasNext()) {
            itCustomLine.next();
            QList<QPointF> customLinePoints = itCustomLine.value();
            for (auto& customLinePoint : customLinePoints) {
                QPointF movingPoint = customLinePoint;
                customLinePoint.setX(static_cast<float>((movingPoint.x() - dx) * spread + dx));
                customLinePoint.setY(static_cast<float>((movingPoint.y() - dx) * spread + dy));
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
                                      1000,       // Maximum value
                                      1,          // Step
                                      &isOk);
    if (spread == 1 || !isOk) {
        return;
    }

    mMultiRect = QRect(0, 0, 0, 0);
    int dx = pR_centerRoom->x;
    int dy = pR_centerRoom->y;

    QSetIterator<int> itSelectionRoom(mMultiSelectionSet);
    while (itSelectionRoom.hasNext()) {
        TRoom* pMovingR = mpMap->mpRoomDB->getRoom(itSelectionRoom.next());
        if (!pMovingR) {
            continue;
        }
        pMovingR->x = (pMovingR->x - dx) / spread + dx;
        pMovingR->y = (pMovingR->y - dy) / spread + dy;
        QMapIterator<QString, QList<QPointF>> itCustomLine(pMovingR->customLines);
        QMap<QString, QList<QPointF>> newCustomLinePointsMap;
        while (itCustomLine.hasNext()) {
            itCustomLine.next();
            QList<QPointF> customLinePoints = itCustomLine.value();
            for (auto& customLinePoint : customLinePoints) {
                QPointF movingPoint = customLinePoint;
                customLinePoint.setX(static_cast<float>((movingPoint.x() - dx) / spread + dx));
                customLinePoint.setY(static_cast<float>((movingPoint.y() - dx) / spread + dy));
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
                                            // Intentional comment to separate arguments
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
                                                // Intentional comment to separate arguments
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
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
            QList<uint> weightCountsList{weightCountsSet.begin(), weightCountsSet.end()};
#else
            QList<uint> weightCountsList{weightCountsSet.toList()};
#endif
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
                                                             // Intentional comment to separate arguments
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
                           mudlet::getMudletPath(mudlet::profileMapsPath, mpMap->mProfileName),
                           tr("Mudlet map (*.dat);;Xml map data (*.xml);;Any file (*)",
                              // Intentional comment to separate arguments
                              "Do not change extensions (in braces) or the ;;s as they are used programmatically"));
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

    mpHost->mpMap->mRoomIdHash[mpMap->mProfileName] = roomID;
    mpHost->mpMap->mNewMove = true;

#if defined(INCLUDE_3DMAPPER)
    if (mpHost->mpMap->mpM) {
        mpHost->mpMap->mpM->update();
    }
#endif

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

    QStringList sortedAreaList;
    sortedAreaList = mpMap->mpRoomDB->getAreaNamesMap().values();

    QCollator sorter;
    sorter.setNumericMode(true);
    sorter.setCaseSensitivity(Qt::CaseInsensitive);

    std::sort( sortedAreaList.begin(), sortedAreaList.end(), sorter);


    const QMap<int, QString>& areaNamesMap = mpMap->mpRoomDB->getAreaNamesMap();
    for (int i = 0, total = sortedAreaList.count(); i < total; ++i) {
        int areaId = areaNamesMap.key(sortedAreaList.at(i));
        arealist_combobox->addItem(QStringLiteral("%1 (%2)").arg(sortedAreaList.at(i), QString::number(areaId)), QString::number(areaId));
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
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
                QSet<TArea*> areaPtrsSet{mpMap->mpRoomDB->getAreaPtrList().begin(), mpMap->mpRoomDB->getAreaPtrList().end()};
                QSetIterator<TArea*> itpArea{areaPtrsSet};
#else
                QSetIterator<TArea*> itpArea{mpMap->mpRoomDB->getAreaPtrList().toSet()};
#endif
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
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
                mMultiSelectionHighlightRoomId = *(mMultiSelectionSet.begin());
#else
                mMultiSelectionHighlightRoomId = mMultiSelectionSet.toList().first();
#endif
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
                    item->setText(0, QStringLiteral("%1").arg(multiSelectionRoomId, mMaxRoomIdDigits));
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
                mMultiSelectionListWidget.sortByColumn(mIsSelectionSortByNames ? 1 : 0, Qt::AscendingOrder);
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
    if (e->modifiers() & Qt::ControlModifier) { // Increase rate if control key down - it makes scrolling through
                                                // a large number of items in a listwidget's contents easier AND this make it
                                                // easier to zoom in and out on LARGE area maps
        delta *= 5;
    }
    if (delta != 0) {
        mPick = false;
        qreal oldZoom = xyzoom;
        xyzoom = qMax(3.0, xyzoom * pow(1.07, delta));

        if (oldZoom != xyzoom) {
            const float widgetWidth = width();
            const float widgetHeight = height();
            float xs = 1.0;
            float ys = 1.0;
            if (widgetWidth > 10 && widgetHeight > 10) {
                if (widgetWidth > widgetHeight)
                    xs = (widgetWidth / widgetHeight);
                else
                    ys = (widgetHeight / widgetWidth);
            }

            // mouse pos within the widget
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
            const QPointF pos = e->position();
#else
            const QPoint pos = mapFromGlobal(e->globalPos());
#endif

            // Position of the mouse within the map, scaled -1 .. +1
            // i.e. if the mouse is in the center, nothing changes
            const float dx = 2.0 * pos.x() / widgetWidth - 1.0;
            const float dy = 2.0 * pos.y() / widgetHeight - 1.0;

            // now shift the origin by that, scaled by the difference in
            // zoom factors. Thus the point under the mouse stays in place.
            mOx += dx * (oldZoom - xyzoom) / 2.0 * xs;
            mOy += dy * (oldZoom - xyzoom) / 2.0 * ys;

            flushSymbolPixmapCache();
            update();
        }
        e->accept();
        return;
    }
    e->ignore();
    return;
}

void T2DMap::setMapZoom(qreal zoom)
{
    qreal oldZoom = xyzoom;
    xyzoom = qMax(3.0, zoom);
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
    mCustomLinesRoomExit.clear();
    auto* button = dialog->findChild<QPushButton*>(key_nw);
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
        button->setChecked(room->customLines.contains(key_nw));
        connect(button, &QAbstractButton::clicked, this, &T2DMap::slot_setCustomLine2);
    }

    button = dialog->findChild<QPushButton*>(key_n);
    if (!button) {
        qWarning(R"(T2DMap::slot_setCustomLine() ERROR: failed to find "n" exit line button!)");
        return;
    } else if (room->getNorth() <= 0) {
        button->setDisabled(true);
        button->setCheckable(false);
    } else {
        button->setCheckable(true);
        button->setChecked(room->customLines.contains(key_n));
        connect(button, &QAbstractButton::clicked, this, &T2DMap::slot_setCustomLine2);
    }

    button = dialog->findChild<QPushButton*>(key_ne);
    if (!button) {
        qWarning(R"(T2DMap::slot_setCustomLine() ERROR: failed to find "ne" exit line button!)");
        return;
    } else if (room->getNortheast() <= 0) {
        button->setDisabled(true);
        button->setCheckable(false);
    } else {
        button->setCheckable(true);
        button->setChecked(room->customLines.contains(key_ne));
        connect(button, &QAbstractButton::clicked, this, &T2DMap::slot_setCustomLine2);
    }

    button = dialog->findChild<QPushButton*>(key_up);
    if (!button) {
        qWarning(R"(T2DMap::slot_setCustomLine() ERROR: failed to find "up" exit line button!)");
        return;
    } else if (room->getUp() <= 0) {
        button->setDisabled(true);
        button->setCheckable(false);
    } else {
        button->setCheckable(true);
        button->setChecked(room->customLines.contains(key_up));
        connect(button, &QAbstractButton::clicked, this, &T2DMap::slot_setCustomLine2);
    }

    button = dialog->findChild<QPushButton*>(key_w);
    if (!button) {
        qWarning(R"(T2DMap::slot_setCustomLine() ERROR: failed to find "w" exit line button!)");
        return;
    } else if (room->getWest() <= 0) {
        button->setCheckable(false);
        button->setDisabled(true);
    } else {
        button->setCheckable(true);
        button->setChecked(room->customLines.contains(key_w));
        connect(button, &QAbstractButton::clicked, this, &T2DMap::slot_setCustomLine2);
    }

    button = dialog->findChild<QPushButton*>(key_e);
    if (!button) {
        qWarning(R"(T2DMap::slot_setCustomLine() ERROR: failed to find "e" exit line button!)");
        return;
    } else if (room->getEast() <= 0) {
        button->setDisabled(true);
        button->setCheckable(false);
    } else {
        button->setCheckable(true);
        button->setChecked(room->customLines.contains(key_e));
        connect(button, &QAbstractButton::clicked, this, &T2DMap::slot_setCustomLine2);
    }

    button = dialog->findChild<QPushButton*>(key_down);
    if (!button) {
        qWarning(R"(T2DMap::slot_setCustomLine() ERROR: failed to find "down" exit line button!)");
        return;
    } else if (room->getDown() <= 0) {
        button->setDisabled(true);
        button->setCheckable(false);
    } else {
        button->setCheckable(true);
        button->setChecked(room->customLines.contains(key_down));
        connect(button, &QAbstractButton::clicked, this, &T2DMap::slot_setCustomLine2);
    }

    button = dialog->findChild<QPushButton*>(key_sw);
    if (!button) {
        qWarning(R"(T2DMap::slot_setCustomLine() ERROR: failed to find "sw" exit line button!)");
        return;
    } else if (room->getSouthwest() <= 0) {
        button->setDisabled(true);
        button->setCheckable(false);
    } else {
        button->setCheckable(true);
        button->setChecked(room->customLines.contains(key_sw));
        connect(button, &QAbstractButton::clicked, this, &T2DMap::slot_setCustomLine2);
    }

    button = dialog->findChild<QPushButton*>(key_s);
    if (!button) {
        qWarning(R"(T2DMap::slot_setCustomLine() ERROR: failed to find "s" exit line button!)");
        return;
    } else if (room->getSouth() <= 0) {
        button->setDisabled(true);
        button->setCheckable(false);
    } else {
        button->setCheckable(true);
        button->setChecked(room->customLines.contains(key_s));
        connect(button, &QAbstractButton::clicked, this, &T2DMap::slot_setCustomLine2);
    }

    button = dialog->findChild<QPushButton*>(key_se);
    if (!button) {
        qWarning(R"(T2DMap::slot_setCustomLine() ERROR: failed to find "se" exit line button!)");
        return;
    } else if (room->getSoutheast() <= 0) {
        button->setDisabled(true);
        button->setCheckable(false);
    } else {
        button->setCheckable(true);
        button->setChecked(room->customLines.contains(key_se));
        connect(button, &QAbstractButton::clicked, this, &T2DMap::slot_setCustomLine2);
    }

    button = dialog->findChild<QPushButton*>(key_in);
    if (!button) {
        qWarning(R"(T2DMap::slot_setCustomLine() ERROR: failed to find "in" exit line button!)");
        return;
    } else if (room->getIn() <= 0) {
        button->setDisabled(true);
        button->setCheckable(false);
    } else {
        button->setCheckable(true);
        button->setChecked(room->customLines.contains(key_in));
        connect(button, &QAbstractButton::clicked, this, &T2DMap::slot_setCustomLine2);
    }

    button = dialog->findChild<QPushButton*>(key_out);
    if (!button) {
        qWarning(R"(T2DMap::slot_setCustomLine() ERROR: failed to find "out" exit line button!)");
        return;
    } else if (room->getOut() <= 0) {
        button->setDisabled(true);
        button->setCheckable(false);
    } else {
        button->setCheckable(true);
        button->setChecked(room->customLines.contains(key_out));
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
    mpCurrentLineStyle->addItem(QIcon(QPixmap(key_icon_line_solid)), tr("Solid line"), static_cast<int>(Qt::SolidLine));
    mpCurrentLineStyle->addItem(QIcon(QPixmap(key_icon_line_dot)), tr("Dot line"), static_cast<int>(Qt::DotLine));
    mpCurrentLineStyle->addItem(QIcon(QPixmap(key_icon_line_dash)), tr("Dash line"), static_cast<int>(Qt::DashLine));
    mpCurrentLineStyle->addItem(QIcon(QPixmap(key_icon_line_dashDot)), tr("Dash-dot line"), static_cast<int>(Qt::DashDotLine));
    mpCurrentLineStyle->addItem(QIcon(QPixmap(key_icon_line_dashDotDot)), tr("Dash-dot-dot line"), static_cast<int>(Qt::DashDotDotLine));
    mpCurrentLineStyle->setCurrentIndex(mpCurrentLineStyle->findData(static_cast<int>(mCurrentLineStyle)));

    mpCurrentLineArrow->setChecked(mCurrentLineArrow);
    mpCurrentLineColor->setStyleSheet(key_styleSheet_backgroundColor.arg(mCurrentLineColor.name()));
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
        mpCurrentLineColor->setStyleSheet(key_styleSheet_backgroundColor.arg(color.name()));
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

bool T2DMap::checkButtonIsForGivenDirection(const QPushButton* pButton, const QString& eKey, const int& roomId)
{
    if (pButton == mpCustomLinesDialog->findChild<QPushButton*>(eKey)) {
        mCustomLinesRoomTo = roomId;
        mCustomLinesRoomExit = eKey;
        return true;
    }

    return false;
}

void T2DMap::slot_setCustomLine2()
{
    auto pButton = qobject_cast<QPushButton*>(sender());
    if (!pButton) {
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

    mpCustomLinesDialog->hide(); // Hide but don't delete until done the custom line
    mDialogLock = false;
    TRoom* room = mpMap->mpRoomDB->getRoom(mCustomLinesRoomFrom);
    if (!room) {
        return;
    }
    // The button texts are going to be subject to translation so we need to go
    // for their addresses instead:
    bool isFound = checkButtonIsForGivenDirection(pButton, key_nw, room->getNorthwest());

    if (!isFound) {
        isFound = checkButtonIsForGivenDirection(pButton, key_n, room->getNorth());
    }

    if (!isFound) {
        isFound = checkButtonIsForGivenDirection(pButton, key_ne, room->getNortheast());
    }

    if (!isFound) {
        isFound = checkButtonIsForGivenDirection(pButton, key_up, room->getUp());
    }

    if (!isFound) {
        isFound = checkButtonIsForGivenDirection(pButton, key_w, room->getWest());
    }

    if (!isFound) {
        isFound = checkButtonIsForGivenDirection(pButton, key_e, room->getEast());
    }

    if (!isFound) {
        isFound = checkButtonIsForGivenDirection(pButton, key_down, room->getDown());
    }

    if (!isFound) {
        isFound = checkButtonIsForGivenDirection(pButton, key_sw, room->getSouthwest());
    }

    if (!isFound) {
        isFound = checkButtonIsForGivenDirection(pButton, key_s, room->getSouth());
    }

    if (!isFound) {
        isFound = checkButtonIsForGivenDirection(pButton, key_se, room->getSoutheast());
    }

    if (!isFound) {
        isFound = checkButtonIsForGivenDirection(pButton, key_in, room->getIn());
    }

    if (!isFound) {
        isFound = checkButtonIsForGivenDirection(pButton, key_out, room->getOut());
    }

    if (!isFound) {
        qWarning() << "T2DMap::slot_setCustomLine2() ERROR - unable to identify exit to use for the button with text:" << pButton->text() << "!)";
        return;
    }

    QList<QPointF> list;
    room->customLines[mCustomLinesRoomExit] = list;
    //    qDebug("T2DMap::slot_setCustomLine2() NORMAL EXIT: %s", qPrintable(exitKey));
    room->customLinesColor[mCustomLinesRoomExit] = mCurrentLineColor;
    /*
     *    qDebug("   COLOR(r,g,b): %i,%i,%i",
     *            mCurrentLineColor.red(),
     *            mCurrentLineColor.green(),
     *            mCurrentLineColor.blue() );
     */
    room->customLinesStyle[mCustomLinesRoomExit] = mCurrentLineStyle;
    //    qDebug("   LINE STYLE: %d", mCurrentLineStyle);
    room->customLinesArrow[mCustomLinesRoomExit] = mCurrentLineArrow;
    //    qDebug("   ARROW: %s", mCurrentLineArrow ? "Yes" : "No");

    mHelpMsg = tr("Left-click to add point, right-click to undo/change/finish...", "2D Mapper big, bottom of screen help message");
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
    mHelpMsg = tr("Left-click to add point, right-click to undo/change/finish...", "2D Mapper big, bottom of screen help message");
    // This message was previously being put up AFTER first click to set first segment was made....
    update();
}

void T2DMap::slot_createLabel()
{
    if (!mpMap->mpRoomDB->getArea(mAreaID)) {
        return;
    }

    mHelpMsg = tr("Left-click and drag a square for the size and position of your label", "2D Mapper big, bottom of screen help message");
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
        if (width() <= 750) {
            newWidth = 160;
        } else if (width() <= 890) { // 750-890 => 160-300
            newWidth = 160+width()-750;
        } else { // 890+ => 300
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

void T2DMap::setPlayerRoomStyle(const int type)
{
    if (!mpMap) {
        return;
    }

    // From Qt 5.6 does not deallocate any memory previously used:
    mPlayerRoomColorGradentStops.clear();
    // Indicate the LARGEST size we will need
    mPlayerRoomColorGradentStops.reserve(5);

    double factor = mpMap->mPlayerRoomInnerDiameterPercentage / 100.0;
    bool solid = (mpMap->mPlayerRoomInnerDiameterPercentage == 0);
    switch (type) {
    case 1: // Simple(?) shaded red ring:
        if (solid) {
            mPlayerRoomColorGradentStops.resize(3);
            mPlayerRoomColorGradentStops[0] = QGradientStop(0.000, QColor(255, 0, 0, 255));
            mPlayerRoomColorGradentStops[1] = QGradientStop(0.990, QColor(255, 0, 0, 255));
            mPlayerRoomColorGradentStops[2] = QGradientStop(1.000, QColor(255, 0, 0, 0));
        } else  {
            mPlayerRoomColorGradentStops.resize(5);
            mPlayerRoomColorGradentStops[0] = QGradientStop(0.000, QColor(255, 0, 0, 0));
            mPlayerRoomColorGradentStops[1] = QGradientStop(factor * 0.950, QColor(255, 0, 0, 0));
            mPlayerRoomColorGradentStops[2] = QGradientStop(factor * 1.050, QColor(255, 0, 0, 255));
            mPlayerRoomColorGradentStops[3] = QGradientStop(1.000 - (factor * 0.100), QColor(255, 0, 0, 255));
            mPlayerRoomColorGradentStops[4] = QGradientStop(1.000, QColor(255, 0, 0, 0));
        }
        break;
        // End of case 1:

    case 2: // Shaded bicolor (blue-yellow - so it ALWAYS contrasts with underlying room color) Ring:
        if (solid) {
            mPlayerRoomColorGradentStops.resize(3);
            mPlayerRoomColorGradentStops[0] = QGradientStop(0.000, QColor(255, 255, 0, 255));
            mPlayerRoomColorGradentStops[1] = QGradientStop(0.990, QColor(0, 0, 255, 255));
            mPlayerRoomColorGradentStops[2] = QGradientStop(1.000, QColor(0, 0, 255, 0));
        } else  {
            mPlayerRoomColorGradentStops.resize(5);
            mPlayerRoomColorGradentStops[0] = QGradientStop(0.000, QColor(255, 255, 0, 0));
            mPlayerRoomColorGradentStops[1] = QGradientStop(factor * 0.950, QColor(255, 255, 0, 0));
            mPlayerRoomColorGradentStops[2] = QGradientStop(factor * 1.050, QColor(255, 255, 0, 255));
            mPlayerRoomColorGradentStops[3] = QGradientStop(1.000 - (factor * 0.100), QColor(0, 0, 255, 255));
            mPlayerRoomColorGradentStops[4] = QGradientStop(1.000, QColor(0, 0, 255, 0));
        }
        break;
        // End of case 2:

    case 3: { // User set ring:
        if (solid) {
            mPlayerRoomColorGradentStops.resize(3);
            mPlayerRoomColorGradentStops[0] = QGradientStop(0.000, mpMap->mPlayerRoomInnerColor);
            mPlayerRoomColorGradentStops[1] = QGradientStop(0.990, mpMap->mPlayerRoomOuterColor);
            QColor transparentColor(mpMap->mPlayerRoomOuterColor);
            transparentColor.setAlpha(0);
            mPlayerRoomColorGradentStops[2] = QGradientStop(1.000, transparentColor);
        } else  {
            mPlayerRoomColorGradentStops.resize(5);
            QColor transparentColor(mpMap->mPlayerRoomInnerColor);
            transparentColor.setAlpha(0);
            mPlayerRoomColorGradentStops[0] = QGradientStop(1.000, transparentColor);
            mPlayerRoomColorGradentStops[1] = QGradientStop(factor * 0.950, transparentColor);
            mPlayerRoomColorGradentStops[2] = QGradientStop(factor * 1.050, mpMap->mPlayerRoomInnerColor);
            mPlayerRoomColorGradentStops[3] = QGradientStop(1.000 - (factor * 0.100), mpMap->mPlayerRoomOuterColor);
            transparentColor = mpMap->mPlayerRoomOuterColor;
            transparentColor.setAlpha(0);
            mPlayerRoomColorGradentStops[4] = QGradientStop(1.000, transparentColor);
        }
        break;
        } // End of case 3:

    default: // Sort of emulates the original code:
        mPlayerRoomColorGradentStops.resize(5);
        mPlayerRoomColorGradentStops[0] = QGradientStop(0, Qt::white);
        mPlayerRoomColorGradentStops[1] = QGradientStop(0.7, QColor(255, 0, 0, 200));
        mPlayerRoomColorGradentStops[2] = QGradientStop(0.799, QColor(150, 100, 100, 100));
        mPlayerRoomColorGradentStops[3] = QGradientStop(0.80, QColor(150, 100, 100, 150));
        mPlayerRoomColorGradentStops[4] = QGradientStop(0.95, QColor(255, 0, 0, 150));
    } // End of switch ()
}
