/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2013-2016, 2018-2026 by Stephen Lyons                   *
 *                                               - slysven@virginmedia.com *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2021-2022 by Piotr Wilczynski - delwing@gmail.com       *
 *   Copyright (C) 2022-2025 by Lecker Kebap - Leris@mudlet.org            *
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

#include "TMapPainter.h"

#include "ExitAppearance.h"
#include "Host.h"
#include "RoomAppearance.h"
#include "TArea.h"
#include "TEvent.h"
#include "TMap.h"
#include "TMapLabel.h"
#include "TRoom.h"
#include "TRoomDB.h"
#include "mapInfoContributorManager.h"
#include "utils.h"

#include <QCoreApplication>
#include <QDebug>
#include <QElapsedTimer>
#include <QFontMetrics>
#include <QFutureWatcher>
#include <QImage>
#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>
#include <QPointer>
#include <QRadialGradient>
#include <QSetIterator>
#include <QVector3D>
#if defined(Q_OS_LINUX) || defined(Q_OS_MACOS)
/* The Devuan package for qt6-base-dev (for Qt 6.8.2) - and presumably
 * Debian and Ubuntu are missing the
 * ./include/QtConcurrent/QtConcurrentTask file that contains the single
 * line:
 * #include "qtconcurrenttask.h" // IWYU pragma: export
 *
 * that is present in the Qt on-line installer install (for Qt 6.10.1)
 * - so building Mudlet using the latter succeed whereas it breaks on the
 * GitHub CI runners (qt 6.9.0). So use the less "pretty" direct include.
 * It appears that this is an upstream Qt bug as the MacOS builds (also
 * for Qt 6.9.0) are failing in the same manner. Yet the Windows build
 * using the later Qt 6.11.0 succeed!*/
#include <qtconcurrenttask.h>
#else
#include <QtConcurrentTask>
#endif

#include <algorithm>
#include <climits>
#include <cmath>
#include <numbers>

namespace {
const QString& key_n = qsl("n");
const QString& key_ne = qsl("ne");
const QString& key_e = qsl("e");
const QString& key_se = qsl("se");
const QString& key_s = qsl("s");
const QString& key_sw = qsl("sw");
const QString& key_w = qsl("w");
const QString& key_nw = qsl("nw");
const QString& key_up = qsl("up");
const QString& key_down = qsl("down");
const QString& key_in = qsl("in");
const QString& key_out = qsl("out");

// The room an XY-plane exit leads to, or 0 for any other exit.
int xyExitFor(const TRoom& room, const QString& key)
{
    if (key == key_n) {
        return room.getNorth();
    }
    if (key == key_ne) {
        return room.getNortheast();
    }
    if (key == key_e) {
        return room.getEast();
    }
    if (key == key_se) {
        return room.getSoutheast();
    }
    if (key == key_s) {
        return room.getSouth();
    }
    if (key == key_sw) {
        return room.getSouthwest();
    }
    if (key == key_w) {
        return room.getWest();
    }
    if (key == key_nw) {
        return room.getNorthwest();
    }
    return 0;
}
} // namespace

// A special exit can be named like a normal one ("n", "up"...); a custom line by
// that name is the special exit's when the room has no such normal exit.
int TMapPainter::customLineDestination(const TRoom& room, const QString& key)
{
    for (int direction = DIR_NORTH; direction <= DIR_OUT; ++direction) {
        if (TRoom::dirCodeToShortString(direction) == key) {
            const int destination = room.getExit(direction);
            if (destination > 0 || !room.getSpecialExits().contains(key)) {
                return destination;
            }
            break;
        }
    }
    return room.getSpecialExits().value(key);
}

// Whether owner has a custom line for an exit to other that runs straight to its centre, so a plain line between them
// would cover it, show through its gaps or run beside it. The line's fixed first half-room segment may leave that line.
bool TMapPainter::customLineRunsStraightTo(const TRoom& owner, const TRoom& other)
{
    if (owner.customLines.empty()) {
        return false;
    }
    const QPointF from(owner.x(), owner.y());
    const QPointF to(other.x(), other.y());
    const QPointF along = to - from;
    const qreal lengthSquared = QPointF::dotProduct(along, along);
    if (lengthSquared <= 0.0) {
        return false;
    }
    constexpr qreal tolerance = 0.1;
    const auto onThePlainLine = [&](const QPointF& point) {
        const qreal fraction = std::clamp(QPointF::dotProduct(point - from, along) / lengthSquared, 0.0, 1.0);
        return QLineF(point, from + along * fraction).length() <= tolerance;
    };
    for (const auto& [key, points] : owner.customLines.asKeyValueRange()) {
        if (points.isEmpty() || QLineF(points.last(), to).length() > tolerance || xyExitFor(owner, key) != other.getId()) {
            continue;
        }
        if (std::all_of(points.cbegin(), points.cend(), onThePlainLine)) {
            return true;
        }
    }
    return false;
}

// key format: <QColor.name()><QString of one or more QChars>
void TMapPainter::addSymbolToPixmapCache(const QString key, const QString text, const QColor symbolColor, const bool gridMode)
{
    // Prevents small, unreadable symbols:
    static const float symbolLowerSizeLimit = 8.0;

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

    QPainter symbolPainter(pixmap);
    RoomAppearance::paintSymbol(symbolPainter, pixmap->rect(), text, symbolColor, mpMap->mMapSymbolFont, mpMap->mMapSymbolFontFudgeFactor, mSymbolFontSize);
    symbolPainter.end();

    if (!mSymbolPixmapCache.insert(key, pixmap)) {
        qDebug("TMapPainter::addSymbolToPixmapCache() ALERT: Map Room Symbol Pixmap cache is full...!");
    }
}

bool TMapPainter::labelPixmapIsCurrent(const LabelPixmap& cached, const TMapLabel& label, const qreal devicePixelRatio)
{
    return cached.devicePixelRatio == devicePixelRatio && cached.sourcePixmapKey == label.pix.cacheKey() && cached.fgColor == label.fgColor.rgba() && cached.bgColor == label.bgColor.rgba()
           && cached.outlineColor == label.outlineColor.rgba() && cached.text == label.text && cached.font == label.font;
}

const TMapPainter::LabelPixmap* TMapPainter::addTextLabelToCache(const LabelPixmapKey& key, const TMapLabel& label, const qreal devicePixelRatio)
{
    const QSize& targetSize = key.size;
    // Drawn with every pixel the screen has for the label, or painting it scales it up and blurs its text
    const QSize pixelSize = (QSizeF(targetSize) * devicePixelRatio).toSize();
    auto entry = std::make_unique<LabelPixmap>();
    entry->devicePixelRatio = devicePixelRatio;
    entry->text = label.text;
    entry->font = label.font;
    entry->fgColor = label.fgColor.rgba();
    entry->bgColor = label.bgColor.rgba();
    entry->outlineColor = label.outlineColor.rgba();
    entry->sourcePixmapKey = label.pix.cacheKey();
    auto insert = [this, &key](std::unique_ptr<LabelPixmap> made) -> const LabelPixmap* {
        made->costKiB = std::max<qint64>(1, qint64(made->pixmap.width()) * made->pixmap.height() * 4 / 1024);
        // Room for what the last frame and this one drew, so a label coming into
        // view never evicts one still on screen
        if (const qsizetype needed = std::min(mTextLabelPixmapLastFrameKiB + mTextLabelPixmapFrameKiB + made->costKiB, mTextLabelPixmapCeilingKiB); needed > mTextLabelPixmapCache.maxCost()) {
            mTextLabelPixmapCache.setMaxCost(needed);
        }
        const qsizetype costKiB = made->costKiB;
        const LabelPixmap* cached = made.get();
        if (!mTextLabelPixmapCache.insert(key, made.release(), costKiB)) {
            qWarning("TMapPainter::addTextLabelToCache() ALERT: Text Label Pixmap cache is full for label %d in area %d", key.labelId, key.areaId);
            return nullptr;
        }
        return cached;
    };

    QFont scaledFont = label.font;
    QRectF targetRect(0, 0, targetSize.width(), targetSize.height());
    if (label.text.isEmpty() || label.font.family().isEmpty() || !sizeFontToFitTextInRect(scaledFont, targetRect, label.text, 5, 4.0)) {
        // An image label, a text label with no font, or a font too small to be
        // legible - smooth-scaled original pixmap
        entry->pixmap = label.pix.scaled(pixelSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        entry->pixmap.setDevicePixelRatio(devicePixelRatio);
        return insert(std::move(entry));
    }

    entry->pixmap = QPixmap(pixelSize);
    entry->pixmap.setDevicePixelRatio(devicePixelRatio);
    entry->pixmap.fill(label.bgColor);

    QPainter painter(&entry->pixmap);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing, mMapperUseAntiAlias);
    painter.setFont(scaledFont);

    const int labelWidth = targetSize.width();
    const int labelHeight = targetSize.height();

    if (label.fgColor != label.outlineColor) {
        QPen outlinePen(label.outlineColor);
        outlinePen.setCosmetic(mMapperUseAntiAlias);
        outlinePen.setWidth(1);
        painter.setPen(outlinePen);

        painter.drawText(QRect(-1, 0, labelWidth, labelHeight), Qt::AlignHCenter | Qt::AlignVCenter, label.text);
        painter.drawText(QRect(1, 0, labelWidth, labelHeight), Qt::AlignHCenter | Qt::AlignVCenter, label.text);
        painter.drawText(QRect(0, -1, labelWidth, labelHeight), Qt::AlignHCenter | Qt::AlignVCenter, label.text);
        painter.drawText(QRect(0, 1, labelWidth, labelHeight), Qt::AlignHCenter | Qt::AlignVCenter, label.text);
    }

    painter.setPen(label.fgColor);
    painter.drawText(QRect(0, 0, labelWidth, labelHeight), Qt::AlignHCenter | Qt::AlignVCenter, label.text);

    painter.end();
    return insert(std::move(entry));
}

// A label rendered whole at its zoomed size needs gigabytes once a big one is
// zoomed in on, so past this QPainter scales only the part of it that is painted.
// An area rather than a side, so a long thin title stays rendered crisply.
static constexpr qint64 cMaxScaledLabelPixels = 4096 * 4096;

static bool fitsScaledLabelLimit(const QSize& size)
{
    return qint64(size.width()) * size.height() <= cMaxScaledLabelPixels;
}

void TMapPainter::drawPixmapScaledTo(QPainter& painter, const QRectF& target, const QPixmap& pixmap)
{
    const QSize targetSize = target.size().toSize();
    if (fitsScaledLabelLimit((target.size() * painter.device()->devicePixelRatio()).toSize())) {
        painter.drawPixmap(target.topLeft(), pixmap.scaled(targetSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
        return;
    }
    painter.save();
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.drawPixmap(target, pixmap, QRectF(pixmap.rect()));
    painter.restore();
}

void TMapPainter::startTextLabelFrame()
{
    mTextLabelPixmapLastFrameKiB = mTextLabelPixmapFrameKiB;
    mTextLabelPixmapFrameKiB = 0;
    // Shrinks back once fewer labels are on screen, e.g. after switching area
    mTextLabelPixmapCache.setMaxCost(std::max(mTextLabelPixmapBudgetKiB, std::min(mTextLabelPixmapLastFrameKiB, mTextLabelPixmapCeilingKiB)));
}

void TMapPainter::drawScaledLabel(QPainter& painter, const QPointF& position, TMapLabel& label, int labelKey, const QRectF& paintRect)
{
    const QSize targetSize = paintRect.size().toSize();
    const qreal devicePixelRatio = painter.device()->devicePixelRatio();
    const QSize pixelSize = (QSizeF(targetSize) * devicePixelRatio).toSize();
    if (fitsScaledLabelLimit(pixelSize)) {
        const LabelPixmapKey key{mAreaID, labelKey, targetSize};
        const LabelPixmap* cached = mTextLabelPixmapCache.object(key);
        if (!cached || !labelPixmapIsCurrent(*cached, label, devicePixelRatio)) {
            // One that would take the labels on screen past the cache's ceiling would evict another, which would
            // in turn evict the next, every frame; on a dense display a label costs the square of its ratio more.
            // Only what this frame has drawn is known to be on screen: after a zoom, none of the last frame is.
            const qint64 costKiB = std::max<qint64>(1, qint64(pixelSize.width()) * pixelSize.height() * 4 / 1024);
            if (mTextLabelPixmapFrameKiB + costKiB > mTextLabelPixmapCeilingKiB) {
                // An out-of-date one left behind would hold memory no frame counts, so trimming the
                // cache to the frame would evict a label on screen in its place
                mTextLabelPixmapCache.remove(key);
                cached = nullptr;
            } else {
                cached = addTextLabelToCache(key, label, devicePixelRatio);
            }
        }
        if (cached) {
            mTextLabelPixmapFrameKiB += cached->costKiB;
            painter.drawPixmap(position, cached->pixmap);
        } else {
            drawPixmapScaledTo(painter, QRectF(position, QSizeF(targetSize)), label.pix);
        }
    } else {
        drawPixmapScaledTo(painter, QRectF(position, QSizeF(targetSize)), label.pix);
    }
    label.clickSize = QSizeF(paintRect.width(), paintRect.height());
}

/*
 * Helper used to size fonts establishes the font size to use to draw the given
 * sample text centralized inside the given boundary (or rather the boundary
 * reduced by the margin (0-40) as a percentage). This margin is defaulted to
 * 10%.
 */
bool TMapPainter::sizeFontToFitTextInRect(QFont& font, const QRectF& boundaryRect, const QString& text, const quint8 percentageMargin, qreal minFontSize)
{
    QFont _font = font;

    if (percentageMargin > 40) {
        qWarning() << "TMapPainter::sizeFontToFitTextInRect(...) percentage margin" << percentageMargin << "exceeded recommended maximum (40%) !";
    }
    if (text.isEmpty()) {
        qWarning() << "TMapPainter::sizeFontToFitTextInRect(...) called with no sample text!";
        return false;
    }

    qreal fontSize = qMax(minFontSize, font.pointSizeF()); // protect against too-small initial value
    const QRectF testRect(boundaryRect.width() * (100 - percentageMargin) / 200.0,
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

        neededRect = _painter.boundingRect(testRect, Qt::AlignCenter | Qt::TextIncludeTrailingSpaces, text);
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

        neededRect = _painter.boundingRect(testRect, Qt::AlignCenter | Qt::TextIncludeTrailingSpaces, text);
    } while ((!isSizeTooSmall) && (!testRect.contains(neededRect)));

    if (isSizeTooSmall) {
        return false;
    }

    font.setPointSizeF(fontSize);
    return true;
}

// This has been refactored to a separate function out of the paintEven() code
// because we need to use it in two places - one for every room that is not the
// player's room and then, AFTER all those have been drawn, once for the
// player's room if it is visible. This is so it is drawn LAST (and any effects,
// or extra markings for it do not get overwritten by the drawing of the other
// rooms)... only the other rooms' highlights, which reach over every room, go over its body.
void TMapPainter::drawRoom(QPainter& painter,
                           QFont& roomVNumFont,
                           QFont& mapNameFont,
                           QPen& pen,
                           TRoom* pRoom,
                           const bool isGridMode,
                           const bool areRoomIdsLegible,
                           const bool showRoomName,
                           const int speedWalkStartRoomId,
                           const float rx,
                           const float ry,
                           const bool showRoomCollision,
                           const RoomDrawPass pass)
{
    const int currentRoomId = pRoom->getId();
    if (pRoom->isHidden()) {
        return;
    }
    pRoom->rendered = false;
    QRectF roomRectangle;
    QRectF roomNameRectangle;
    double realHeight;
    const int borderWidth = roomBorderWidth(*pRoom);
    const bool shouldDrawBorder = roomBorderShown(*pRoom) && !isGridMode;
    const bool showThisRoomName = showRoomName && RoomAppearance::nameShown(*pRoom);
    if (isGridMode) {
        realHeight = mRoomHeight;
        roomRectangle = QRectF(rx - mRoomWidth / 2.0, ry - mRoomHeight / 2.0, mRoomWidth, mRoomHeight);
    } else {
        // this dance is necessary to put the name just below the room rect, later
        // but we only do this when NOT in grid mode
        realHeight = mRoomHeight * rSize;
        roomRectangle = QRectF(rx - (mRoomWidth * rSize) / 2.0, ry - realHeight / 2.0, mRoomWidth * rSize, realHeight);
        roomNameRectangle = roomRectangle.adjusted(-2000, realHeight, 2000, realHeight);
    }

    // We should be using the full area for testing for clicks even though
    // we only show a smaller one if the user has dialed down the room size
    // on NON-grid mode areas:
    const QRectF roomClickTestRectangle(QRectF(static_cast<qreal>(rx) - (static_cast<qreal>(mRoomWidth) / 2.0),
                                               static_cast<qreal>(ry) - (static_cast<qreal>(mRoomHeight) / 2.0),
                                               static_cast<qreal>(mRoomWidth),
                                               static_cast<qreal>(mRoomHeight)));

    const QColor roomColor = environmentColor(pRoom->environment);

    if (pass == RoomDrawPass::Whole || pass == RoomDrawPass::Body) {
        const bool isRoomSelected = (mPick && roomClickTestRectangle.contains(mPHighlight)) || mMultiSelectionSet.contains(currentRoomId);

        QPen roomPen(Qt::transparent);
        roomPen.setJoinStyle(Qt::MiterJoin);
        painter.setBrush(roomColor);

        // Determine if we're actually drawing a border
        bool isDrawingBorder = false;
        qreal borderInset = 0.0;

        if (showRoomCollision) {
            roomPen.setColor(mpHost->mRoomCollisionBorderColor);
            roomPen.setWidth(borderWidth);
            borderInset = borderWidth / 2.0;
            isDrawingBorder = true;
        } else if (shouldDrawBorder) {
            roomPen.setWidth(borderWidth);
            borderInset = borderWidth / 2.0;
            isDrawingBorder = true;
            QColor borderColor = pRoom->mBorderColor.isValid() ? pRoom->mBorderColor : mpHost->mRoomBorderColor;
            if (mRoomWidth >= 12) {
                roomPen.setColor(borderColor);
            } else {
                auto fadingColor = QColor(borderColor);
                fadingColor.setAlpha(255 * (mRoomWidth / 12));
                roomPen.setColor(fadingColor);
            }
        }

        // Expand the drawing rectangle outward by half the border width so that,
        // with the border centered on its edge, the visible fill area matches the
        // original room size and the border grows purely outward
        const QRectF roomDrawRectangle = roomRectangle.adjusted(-borderInset, -borderInset, borderInset, borderInset);

        if (isRoomSelected) {
            QLinearGradient selectionBg(roomDrawRectangle.topLeft(), roomDrawRectangle.bottomRight());
            selectionBg.setColorAt(0.2, roomColor);
            selectionBg.setColorAt(1, Qt::blue);
            if (!showRoomCollision && isDrawingBorder) {
                roomPen.setColor(QColor(255, 50, 50));
            }
            painter.setBrush(selectionBg);
        }

        painter.setPen(roomPen);

        if (mBubbleMode) {
            // Expand radius by half border width so visible fill stays at original size
            const float roomRadius = (0.5 * rSize * mRoomWidth) + borderInset;
            const QPointF roomCenter = QPointF(rx, ry);
            QPainterPath diameterPath;
            diameterPath.addEllipse(roomCenter, roomRadius, roomRadius);
            painter.drawPath(diameterPath);
        } else if (isDrawingBorder) {
            // The border is filled rather than stroked, as Qt's raster engine strokes a rect several times slower than it fills one
            painter.setPen(Qt::NoPen);
            painter.drawRect(roomDrawRectangle);
            const QRectF outer = roomRectangle.adjusted(-borderWidth, -borderWidth, borderWidth, borderWidth);
            // In device pixels, which HiDPI screens and exportAreaToImage() scale the border to
            if (borderWidth * painter.deviceTransform().m11() > 1.0 && painter.testRenderHint(QPainter::Antialiasing)) {
                // Antialiased strips leave a visible seam across a wider border where they meet at each corner
                QPainterPath ring;
                ring.setFillRule(Qt::OddEvenFill);
                ring.addRect(outer);
                ring.addRect(roomRectangle);
                painter.fillPath(ring, roomPen.color());
            } else {
                // Strips, as a ring is no faster than the stroke for a border one device pixel wide
                const QRectF borderStrips[] = {QRectF(outer.left(), outer.top(), outer.width(), borderWidth),
                                               QRectF(outer.left(), roomRectangle.bottom(), outer.width(), borderWidth),
                                               QRectF(outer.left(), roomRectangle.top(), borderWidth, roomRectangle.height()),
                                               QRectF(roomRectangle.right(), roomRectangle.top(), borderWidth, roomRectangle.height())};
                const QBrush roomBrush = painter.brush();
                painter.setBrush(roomPen.color());
                painter.drawRects(borderStrips, 4);
                painter.setBrush(roomBrush);
            }
            // ExitAppearance::paintExitMarkers() derives its pens from the painter's current one
            painter.setPen(roomPen);
        } else {
            painter.drawRect(roomDrawRectangle);
        }

        if (isRoomSelected) {
            mPick = false;
            if (mStartSpeedWalk) {
                mStartSpeedWalk = false;
                // This draws a red circle around the room that was chosen as
                // the target for the speedwalk, but it is only shown for one
                // paintEvent call and it is not obvious that it is useful, note
                // that this is the code for a room being clicked on that is
                // within the area - there is a separate block of code further down
                // in this method that handles clicking on the out of area exit so
                // that a speed walk is done to the room in the OTHER area:
                const float roomRadius = 0.4 * mRoomWidth;
                const QPointF roomCenter = QPointF(rx, ry);
                QRadialGradient gradient(roomCenter, roomRadius);
                gradient.setColorAt(0.95, QColor(255, 0, 0, 150));
                gradient.setColorAt(0.80, QColor(150, 100, 100, 150));
                gradient.setColorAt(0.799, QColor(150, 100, 100, 100));
                gradient.setColorAt(0.7, QColor(255, 0, 0, 200));
                gradient.setColorAt(0, Qt::white);
                const QPen transparentPen(Qt::transparent);
                QPainterPath diameterPath;
                painter.setBrush(gradient);
                painter.setPen(transparentPen);
                diameterPath.addEllipse(roomCenter, roomRadius, roomRadius);
                painter.drawPath(diameterPath);

                mPendingSpeedWalk = {speedWalkStartRoomId, currentRoomId};
            }
        }
    }

    if (pass == RoomDrawPass::Body) {
        return;
    }

    // Do we need to draw the custom (user specified) highlight
    if (pRoom->highlight && pass != RoomDrawPass::Labels) {
        const float roomRadius = (pRoom->highlightRadius * mRoomWidth) / 2.0;
        const QPointF roomCenter = QPointF(rx, ry);
        QRadialGradient gradient(roomCenter, roomRadius);
        gradient.setStops(RoomAppearance::highlightStops(pRoom->highlightColor, pRoom->highlightColor2));
        const QPen transparentPen(Qt::transparent);
        QPainterPath diameterPath;
        painter.setBrush(gradient);
        painter.setPen(transparentPen);
        diameterPath.addEllipse(roomCenter, roomRadius, roomRadius);
        painter.drawPath(diameterPath);
    }

    if (pass == RoomDrawPass::Highlight) {
        return;
    }

    // Do we need to draw the room symbol:
    if (!(mShowRoomID && areRoomIdsLegible) && !pRoom->mSymbol.isEmpty()) {
        const QColor symbolColor = RoomAppearance::symbolColor(pRoom->mSymbolColor, roomColor);
        auto pixmapKey = qsl("%1_%2").arg(symbolColor.name(), pRoom->mSymbol);
        if (!mSymbolPixmapCache.contains(pixmapKey)) {
            addSymbolToPixmapCache(pixmapKey, pRoom->mSymbol, symbolColor, isGridMode);
        }

        painter.save();
        painter.setBackgroundMode(Qt::TransparentMode);

        QPixmap* pix = mSymbolPixmapCache.object(pixmapKey);
        if (!pix) {
            qWarning("TMapPainter::drawRoom() Alert: mSymbolPixmapCache failure, too many items to cache all of them for: \"%s\"", pRoom->mSymbol.toUtf8().constData());
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

            painter.drawPixmap(QPoint(qRound(roomRectangle.left() + ((roomRectangle.width() - pix->width()) / 2.0)), qRound(roomRectangle.top() + ((roomRectangle.height() - pix->height()) / 2.0))),
                               *pix);
        }

        painter.restore();
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

    if (showThisRoomName) {
        painter.save();

        const QPointF nameOffset = RoomAppearance::nameOffset(pRoom->userData.value(ROOM_UI_NAMEPOS));
        roomNameRectangle.adjust(mRoomWidth * nameOffset.x(), mRoomHeight * nameOffset.y(), mRoomWidth * nameOffset.x(), mRoomHeight * nameOffset.y());
        painter.setPen(QPen(RoomAppearance::nameColor(pRoom->userData.value(ROOM_UI_NAMECOLOR), mpHost->mBgColor_2)));
        painter.setFont(mapNameFont);
        painter.drawText(roomNameRectangle, Qt::AlignCenter | Qt::TextDontClip, pRoom->name);
        painter.restore();
    }

    ExitAppearance::paintExitMarkers(painter, pen, *pRoom, rx, ry, mRoomWidth * rSize, mRoomHeight * rSize, roomColor, mMapperUseAntiAlias);
}

bool TMapPainter::roomBorderShown(const TRoom& room) const
{
    return mpHost->mMapperShowRoomBorders || room.mBorderColor.isValid() || room.mBorderThickness > 0;
}

int TMapPainter::roomBorderWidth(const TRoom& room) const
{
    const double thickness = room.mBorderThickness > 0 ? room.mBorderThickness : 1.0;
    return qMax(1, static_cast<int>(thickness / mBSize * mRoomWidth * rSize));
}

void TMapPainter::drawRoomHighlightsAndLabels(QPainter& painter,
                                              QFont& roomVNumFont,
                                              QFont& mapNameFont,
                                              QPen& pen,
                                              const QList<QPair<TRoom*, QPointF>>& rooms,
                                              const bool isGridMode,
                                              const bool areRoomIdsLegible,
                                              const bool showRoomNames,
                                              const int speedWalkStartRoomId)
{
    for (const auto& [room, roomCenter] : rooms) {
        if (room->highlight) {
            drawRoom(painter,
                     roomVNumFont,
                     mapNameFont,
                     pen,
                     room,
                     isGridMode,
                     areRoomIdsLegible,
                     showRoomNames,
                     speedWalkStartRoomId,
                     static_cast<float>(roomCenter.x()),
                     static_cast<float>(roomCenter.y()),
                     false,
                     RoomDrawPass::Highlight);
        }
    }
    for (const auto& [room, roomCenter] : rooms) {
        drawRoom(painter,
                 roomVNumFont,
                 mapNameFont,
                 pen,
                 room,
                 isGridMode,
                 areRoomIdsLegible,
                 showRoomNames,
                 speedWalkStartRoomId,
                 static_cast<float>(roomCenter.x()),
                 static_cast<float>(roomCenter.y()),
                 false,
                 RoomDrawPass::Labels);
    }
}

void TMapPainter::drawPlayerRoomBody(QPainter& painter, TRoom* pPlayerRoom, const QPointF& roomCenter, const bool isGridMode, const bool showRoomCollision)
{
    // The body pass reads none of the fonts or the exit pen
    QFont unusedFont = painter.font();
    QPen unusedPen = painter.pen();
    // The grid and reduced-detail loops resolve a double-click themselves, after this
    const bool pick = std::exchange(mPick, false);
    const bool startSpeedWalk = std::exchange(mStartSpeedWalk, false);
    drawRoom(painter,
             unusedFont,
             unusedFont,
             unusedPen,
             pPlayerRoom,
             isGridMode,
             false,
             false,
             pPlayerRoom->getId(),
             static_cast<float>(roomCenter.x()),
             static_cast<float>(roomCenter.y()),
             showRoomCollision,
             RoomDrawPass::Body);
    mPick = pick;
    mStartSpeedWalk = startSpeedWalk;
}

bool TMapPainter::anyRoomHighlighted(const QList<int>& roomIds) const
{
    return std::any_of(roomIds.cbegin(), roomIds.cend(), [this](const int roomId) {
        const TRoom* pRoom = mpMap->mpRoomDB->getRoom(roomId);
        return pRoom && pRoom->highlight && !pRoom->isHidden();
    });
}

QColor TMapPainter::environmentColor(int env) const
{
    return RoomAppearance::environmentColor(*mpMap, *mpHost, env);
}

// Also decides which exits that tier drops: rooms are painted over exits, so one whose ends are within
// a blob on both axes is hidden (bar its door tick, so this simplifies rather than matches).
// At least one pixel, or rooms would not be drawn.
QSize TMapPainter::lodRoomBlobSize() const
{
    return QSize(qMax(1, qRound(mRoomWidth * rSize)), qMax(1, qRound(mRoomHeight * rSize)));
}

// Casting a double outside int range is undefined, and callers exceed it: zoom is unbounded, and the
// pixel origin scales a centre coordinate that can be INT_MAX. NaN (0 * inf at extreme zoom) maps to INT_MIN.
int TMapPainter::clampedToIntRange(const double value)
{
    return static_cast<int>(qBound(static_cast<double>(INT_MIN), value, static_cast<double>(INT_MAX)));
}

// Inverts rx = x * roomWidth + rx0, ry = -y * roomHeight + ry0, rounded outwards plus a cell of margin
// so float rounding can't lose a drawable room; both room loops cull more tightly afterwards.
QRect TMapPainter::viewportRoomBounds(const float rx0, const float ry0, const float roomWidth, const float roomHeight, const float widgetWidth, const float widgetHeight)
{
    // A zero-pixel room would divide by zero below, and clamping can't rescue that: NaN fails every
    // qBound comparison and int(NaN) differs between x86-64 and ARM64. Return the whole coordinate range.
    if (!(roomWidth > 0.0f) || !(roomHeight > 0.0f)) {
        return QRect(QPoint(INT_MIN, INT_MIN), QPoint(INT_MAX, INT_MAX));
    }
    const int minX = clampedToIntRange(std::floor(static_cast<double>(-rx0) / roomWidth) - 1.0);
    const int maxX = clampedToIntRange(std::ceil(static_cast<double>(widgetWidth - rx0) / roomWidth) + 1.0);
    const int minY = clampedToIntRange(std::floor(static_cast<double>(ry0 - widgetHeight) / roomHeight) - 1.0);
    const int maxY = clampedToIntRange(std::ceil(static_cast<double>(ry0) / roomHeight) + 1.0);
    return QRect(QPoint(minX, minY), QPoint(maxX, maxY));
}

// Fetches only viewport cells from the grid index: O(visible cells), not O(rooms on the level).
void TMapPainter::drawGridModeRooms(QPainter& painter,
                                    const TArea* pDrawnArea,
                                    const int zLevel,
                                    const int playerRoomId,
                                    const float mRX,
                                    const float mRY,
                                    const float mRoomWidth,
                                    const float mRoomHeight,
                                    const float widgetWidth,
                                    const float widgetHeight,
                                    QFont& roomVNumFont,
                                    bool& isPlayerRoomVisible,
                                    QPointF& playerRoomOnWidgetCoordinates,
                                    bool& isPlayerRoomBodyDrawn,
                                    bool areRoomIdsLegible,
                                    QString* profileOutput)
{
    QElapsedTimer timer;
    timer.start();

    const QRect roomBounds = viewportRoomBounds(mRX, mRY, mRoomWidth, mRoomHeight, widgetWidth, widgetHeight);
    const int minX = roomBounds.left();
    const int maxX = roomBounds.right();
    const int minY = roomBounds.top();
    const int maxY = roomBounds.bottom();

    const qreal timeIndex = timer.nsecsElapsed() / 1000000.0;
    timer.start();

    const TAreaGridIndex& gridIndex = pDrawnArea->getGridIndex();
    int roomCount = 0;

    // LOD fast path: when rooms are smaller than this pixel threshold their
    // individual details (collision borders, symbols, vnums) are invisible.
    // QPainterPath::addRect() costs ~3.6 us per room regardless of room size,
    // making the normal path unusable at high zoom-out with thousands of rooms.
    // Writing directly to a QImage scanline is ~10x faster overall.
    if (mRoomWidth < cLodPixelThreshold) {
        const int imgW = static_cast<int>(std::ceil(static_cast<double>(widgetWidth)));
        const int imgH = static_cast<int>(std::ceil(static_cast<double>(widgetHeight)));
        QImage lodImage(imgW, imgH, QImage::Format_ARGB32_Premultiplied);
        lodImage.fill(0); // transparent: background was already rendered beneath us

        // Premultiplied, as these go straight into a premultiplied image's scanlines: a translucent
        // setCustomEnvColor() left unpremultiplied would composite at full strength.
        QHash<int, QRgb> envColorCache;

        if (mRoomWidth < 1.0f) {
            // Sub-pixel LOD: each output pixel may cover multiple rooms.
            // Iterate output pixels and look up the representative room per unique
            // world cell. Run-length optimizations skip redundant lookups:
            //   - Row runs: when the same worldY repeats across consecutive output
            //     rows (common at extreme zoom-out where 1 room spans ~5.8 rows at
            //     roomSizePx=0.171), duplicate the previous scanline with memcpy.
            //   - Column runs: when the same worldX repeats across consecutive pixels
            //     in a row, reuse the previously computed color.
            // Combined, these reduce unique lookups from ~5.36M to ~4,600 at the
            // reported zoom level, yielding a ~700x speedup in this branch.
            int lastWorldY = INT_MIN;
            for (int py = 0; py < imgH; ++py) {
                QRgb* line = reinterpret_cast<QRgb*>(lodImage.scanLine(py));
                const int worldY = static_cast<int>(std::floor(static_cast<double>(mRY - py) / mRoomHeight));

                if (worldY == lastWorldY && py > 0) {
                    // Same world row as the previous scanline: just copy it.
                    const QRgb* prevLine = reinterpret_cast<const QRgb*>(lodImage.constScanLine(py - 1));
                    std::copy_n(prevLine, imgW, line);
                    continue;
                }
                lastWorldY = worldY;

                int lastWorldX = INT_MIN;
                QRgb currentColor = 0;

                for (int px = 0; px < imgW; ++px) {
                    const int worldX = static_cast<int>(std::floor(static_cast<double>(px - mRX) / mRoomWidth));

                    if (worldX != lastWorldX) {
                        lastWorldX = worldX;
                        currentColor = 0;

                        const TAreaGridIndex::RoomIds& rooms = gridIndex.roomsAt(zLevel, worldX, worldY);
                        // Iterate to find the first non-hidden room at this cell.
                        for (const int roomId : rooms) {
                            TRoom* room = mpMap->mpRoomDB->getRoom(roomId);
                            if (!room || room->isHidden()) {
                                continue;
                            }
                            ++roomCount;
                            if (roomId == playerRoomId) {
                                isPlayerRoomVisible = true;
                                playerRoomOnWidgetCoordinates = QPointF(static_cast<qreal>(worldX * mRoomWidth + mRX), static_cast<qreal>(worldY * -1.0f * mRoomHeight + mRY));
                                // Player room drawn by paintEvent; leave pixel transparent.
                            } else {
                                const auto colorIt = envColorCache.constFind(room->environment);
                                if (colorIt != envColorCache.constEnd()) {
                                    currentColor = *colorIt;
                                } else {
                                    currentColor = qPremultiply(environmentColor(room->environment).rgba());
                                    envColorCache.insert(room->environment, currentColor);
                                }
                            }
                            break; // Use only the first non-hidden room at this cell.
                        }
                    }

                    line[px] = currentColor;
                }
            }
        } else {
            // Room-driven LOD path for 1.0 <= mRoomWidth < cLodPixelThreshold.
            // Each room spans >= 1 output pixel; iterate rooms from the grid index.
            for (const int roomId : gridIndex.roomsInViewport(zLevel, minX, maxX, minY, maxY)) {
                TRoom* room = mpMap->mpRoomDB->getRoom(roomId);
                if (!room || room->isHidden()) {
                    continue;
                }

                ++roomCount;

                const float rx = room->x() * mRoomWidth + mRX;
                const float ry = room->y() * -1.0f * mRoomHeight + mRY;

                if (playerRoomId == roomId) {
                    isPlayerRoomVisible = true;
                    playerRoomOnWidgetCoordinates = QPointF(static_cast<qreal>(rx), static_cast<qreal>(ry));
                    continue;
                }

                // Map the room center to a pixel rect, clamped to image bounds.
                const int px = static_cast<int>(rx - mRoomWidth * 0.5f);
                const int py = static_cast<int>(ry - mRoomHeight * 0.5f);
                const int pw = qMax(1, static_cast<int>(std::ceil(static_cast<double>(mRoomWidth))));
                const int ph = qMax(1, static_cast<int>(std::ceil(static_cast<double>(mRoomHeight))));
                const int xStart = qBound(0, px, imgW);
                const int xEnd = qBound(0, px + pw, imgW);
                const int yStart = qBound(0, py, imgH);
                const int yEnd = qBound(0, py + ph, imgH);
                if (xStart >= xEnd || yStart >= yEnd) {
                    continue;
                }

                QRgb colorRgb;
                const auto colorIt = envColorCache.constFind(room->environment);
                if (colorIt != envColorCache.constEnd()) {
                    colorRgb = *colorIt;
                } else {
                    colorRgb = qPremultiply(environmentColor(room->environment).rgba());
                    envColorCache.insert(room->environment, colorRgb);
                }

                // Direct scanline writes bypass QPainterPath construction.
                for (int dy = yStart; dy < yEnd; ++dy) {
                    QRgb* scanLine = reinterpret_cast<QRgb*>(lodImage.scanLine(dy));
                    for (int dx = xStart; dx < xEnd; ++dx) {
                        scanLine[dx] = colorRgb;
                    }
                }
            }
        }

        const qreal timeCollect = timer.nsecsElapsed() / 1000000.0;
        timer.start();

        painter.drawImage(0, 0, lodImage);

        // Overlay selected rooms with a blue tint so the selection highlight
        // is preserved in both LOD paths.  At LOD scales (rooms are < 4px wide)
        // a real gradient is imperceptible, so we paint a semi-transparent blue
        // rect over the env-colored pixel(s) to produce the same "env + blue"
        // appearance as the full-detail selection gradient.
        if (!mMultiSelectionSet.isEmpty()) {
            painter.save();
            painter.setPen(Qt::NoPen);
            const int roomW = qMax(1, qRound(static_cast<double>(mRoomWidth)));
            const int roomH = qMax(1, qRound(static_cast<double>(mRoomHeight)));
            // Semi-transparent blue matches the blue endpoint of the full-detail
            // QLinearGradient (Qt::blue at alpha ~180 gives an equivalent look
            // over the env base colour that was already written into lodImage).
            const QColor selectionTint(0, 0, 255, 180);
            for (const int selId : std::as_const(mMultiSelectionSet)) {
                TRoom* room = mpMap->mpRoomDB->getRoom(selId);
                if (!room || room->z() != zLevel) {
                    continue;
                }
                const int rx = qRound(room->x() * mRoomWidth + mRX);
                const int ry = qRound(room->y() * -1.0 * mRoomHeight + mRY);
                const QRect selRect(rx - roomW / 2, ry - roomH / 2, roomW, roomH);
                if (selRect.intersects(QRect(0, 0, static_cast<int>(widgetWidth), static_cast<int>(widgetHeight)))) {
                    painter.fillRect(selRect, selectionTint);
                }
            }
            painter.restore();
        }

        const qreal timeBlit = timer.nsecsElapsed() / 1000000.0;

        if (qEnvironmentVariableIsSet("MUDLET_PROFILE_MAP") && profileOutput) {
            // Build the profiling string and return it to paintEvent so it can
            // be printed AFTER phase6Time is captured.  Calling qDebug() here
            // would inflate phase6 time by 100-200 ms on Windows debug builds.
            const char* lodPathName = (mRoomWidth < 1.0f) ? "LOD sub-pixel" : "LOD pixel";
            QDebug dbg(profileOutput);
            dbg.noquote().nospace() << "drawGridModeRooms (" << lodPathName << ") timing (ms):" << " total:" << (timeIndex + timeCollect + timeBlit) << " indexSetup:" << timeIndex
                                    << " collect+pixelWrite:" << timeCollect << " imageBlit:" << timeBlit << " visibleRooms:" << roomCount
                                    << " viewportColumns:" << (static_cast<qint64>(maxX) - minX + 1) << " viewportRows:" << (static_cast<qint64>(maxY) - minY + 1) << " viewportBounds: x[" << minX
                                    << "," << maxX << "] y[" << minY << "," << maxY << "]" << " roomSizePx:" << mRoomWidth << " gridIndexRooms:" << gridIndex.size()
                                    << " gridIndexBytes:" << gridIndex.memoryEstimateBytes();
        }

        // Handle double-click speedwalk via grid-cell lookup.  The pixel-level
        // hit test inside drawRoom() is never reached in the LOD path, so we
        // must resolve the clicked cell here before returning.
        if (mPick) {
            const int clickWorldX = qRound(static_cast<double>(mPHighlight.x() - mRX) / mRoomWidth);
            const int clickWorldY = qRound(static_cast<double>(mRY - mPHighlight.y()) / mRoomHeight);
            const TAreaGridIndex::RoomIds& clickedRooms = gridIndex.roomsAt(zLevel, clickWorldX, clickWorldY);
            if (!clickedRooms.isEmpty()) {
                const int clickedRoomId = *clickedRooms.constBegin();
                mPick = false;
                if (mStartSpeedWalk) {
                    mStartSpeedWalk = false;
                    mPendingSpeedWalk = {playerRoomId, clickedRoomId};
                }
            } else {
                mPick = false;
            }
        }
        return;
    }

    // Full detail path (mRoomWidth >= cLodPixelThreshold): collect rooms grouped
    // by environment color, then batch-draw using QPainterPath per color group.
    QHash<int, QList<QRectF>> roomsByEnvironment;
    QHash<int, QList<QRectF>> collisionRoomsByEnvironment;
    QList<QPair<TRoom*, QRectF>> decorationRooms;
    QList<QPair<TRoom*, QRectF>> selectedRooms;
    QList<QPair<TRoom*, QRectF>> highlightedRooms;
    QList<QPair<TRoom*, QPointF>> verticalExitRooms;
    TRoom* pPlayerRoom = nullptr;
    bool playerRoomCollision = false;

    const bool showVnums = mShowRoomID && areRoomIdsLegible && mMaxRoomIdDigits > 0;

    for (const auto& [roomId, roomCollision] : gridIndex.roomsInViewportWithCollisions(zLevel, minX, maxX, minY, maxY)) {
        TRoom* room = mpMap->mpRoomDB->getRoom(roomId);
        if (!room || room->isHidden()) {
            continue;
        }

        const float rx = room->x() * mRoomWidth + mRX;
        const float ry = room->y() * -1 * mRoomHeight + mRY;

        // Belt-and-suspenders: the viewport bounds above should make this pass
        // always, but guard against floating-point edge cases at the margin.
        if (rx < -mRoomWidth || ry < -mRoomHeight || rx > widgetWidth + mRoomWidth || ry > widgetHeight + mRoomHeight) {
            continue;
        }

        ++roomCount;

        if (playerRoomId == roomId) {
            isPlayerRoomVisible = true;
            playerRoomOnWidgetCoordinates = QPointF(static_cast<qreal>(rx), static_cast<qreal>(ry));
            pPlayerRoom = room;
            playerRoomCollision = roomCollision;
            continue;
        }

        const QRectF roomRect(rx - mRoomWidth / 2.0, ry - mRoomHeight / 2.0, mRoomWidth, mRoomHeight);

        if (mMultiSelectionSet.contains(roomId)) {
            selectedRooms.append({room, roomRect});
        } else if (roomCollision) {
            collisionRoomsByEnvironment[room->environment].append(roomRect);
        } else {
            roomsByEnvironment[room->environment].append(roomRect);
        }

        if (room->highlight) {
            highlightedRooms.append({room, roomRect});
        }

        if (showVnums || !room->mSymbol.isEmpty()) {
            decorationRooms.append({room, roomRect});
        }

        if (room->getUp() > 0 || room->exitStubs.contains(DIR_UP) || room->getDown() > 0 || room->exitStubs.contains(DIR_DOWN) || room->getIn() > 0 || room->exitStubs.contains(DIR_IN)
            || room->getOut() > 0 || room->exitStubs.contains(DIR_OUT)) {
            verticalExitRooms.append({room, QPointF(rx, ry)});
        }
    }

    const qreal timeCollect = timer.nsecsElapsed() / 1000000.0;
    timer.start();

    // Batch draw normal rooms by environment color.
    painter.setPen(Qt::NoPen);
    for (auto it = roomsByEnvironment.constBegin(); it != roomsByEnvironment.constEnd(); ++it) {
        QPainterPath path;
        for (const auto& rect : it.value()) {
            if (mBubbleMode) {
                path.addEllipse(rect);
            } else {
                path.addRect(rect);
            }
        }
        painter.fillPath(path, environmentColor(it.key()));
    }

    const qreal timeBatchDraw = timer.nsecsElapsed() / 1000000.0;
    timer.start();

    // Batch draw collision rooms with a colored border.
    const int collisionBorderWidth = qMax(1, static_cast<int>(mRoomWidth / 50.0));
    for (auto it = collisionRoomsByEnvironment.constBegin(); it != collisionRoomsByEnvironment.constEnd(); ++it) {
        const QColor color = environmentColor(it.key());
        QPainterPath fillPath;
        for (const auto& rect : it.value()) {
            if (mBubbleMode) {
                fillPath.addEllipse(rect);
            } else {
                fillPath.addRect(rect);
            }
        }
        painter.fillPath(fillPath, color);

        QPen collisionPen(mpHost->mRoomCollisionBorderColor, collisionBorderWidth);
        painter.setPen(collisionPen);
        for (const auto& rect : it.value()) {
            if (mBubbleMode) {
                painter.drawEllipse(rect);
            } else {
                painter.drawRect(rect);
            }
        }
    }

    // Draw selected rooms with a gradient fill so they stand out in editing mode.
    // This mirrors the selection highlight in drawRoom() which is never called for
    // grid mode rooms.
    painter.setPen(Qt::NoPen);
    for (const auto& [room, roomRect] : selectedRooms) {
        const QColor roomColor = environmentColor(room->environment);
        QLinearGradient selectionBg(roomRect.topLeft(), roomRect.bottomRight());
        selectionBg.setColorAt(0.2, roomColor);
        selectionBg.setColorAt(1, Qt::blue);
        painter.setBrush(selectionBg);
        if (mBubbleMode) {
            painter.drawEllipse(roomRect);
        } else {
            painter.drawRect(roomRect);
        }
    }

    if (pPlayerRoom && !highlightedRooms.isEmpty()) {
        drawPlayerRoomBody(painter, pPlayerRoom, playerRoomOnWidgetCoordinates, true, playerRoomCollision);
        isPlayerRoomBodyDrawn = true;
    }

    // Draw custom per-room radial highlights (pRoom->highlight flag).
    for (const auto& [room, roomRect] : highlightedRooms) {
        const float roomRadius = (room->highlightRadius * mRoomWidth) / 2.0f;
        const QPointF roomCenter = roomRect.center();
        QRadialGradient gradient(roomCenter, roomRadius);
        gradient.setStops(RoomAppearance::highlightStops(room->highlightColor, room->highlightColor2));
        const QPen transparentPen(Qt::transparent);
        QPainterPath diameterPath;
        painter.setBrush(gradient);
        painter.setPen(transparentPen);
        diameterPath.addEllipse(roomCenter, roomRadius, roomRadius);
        painter.drawPath(diameterPath);
    }

    // Draw vertical exits (up/down/in/out) for grid mode rooms, mirroring drawRoom().
    if (!verticalExitRooms.isEmpty()) {
        constexpr float allInsideTipOffsetFactor = 1 / 20.0f;
        constexpr float upDownXOrYFactor = 1 / 3.1f;
        constexpr float inOuterXFactor = 1 / 4.5f;
        constexpr float inUpDownYFactor = 1 / 7.0f;
        constexpr float outOuterXFactor = 1 / 2.2f;
        constexpr float outUpDownYFactor = 1 / 5.5f;
        constexpr float outInterXFactor = 1 / 3.5f;
        painter.save();
        for (const auto& [room, center] : verticalExitRooms) {
            const float rx = center.x();
            const float ry = center.y();
            const QColor roomColor = environmentColor(room->environment);
            const QColor lc = (roomColor.lightness() > 127) ? QColorConstants::Black : QColorConstants::White;
            QPen exitPen;
            exitPen.setColor(lc);
            exitPen.setCosmetic(mMapperUseAntiAlias);
            exitPen.setCapStyle(Qt::RoundCap);
            exitPen.setJoinStyle(Qt::RoundJoin);
            QPen innerPen = exitPen;
            QBrush innerBrush = painter.brush();
            innerBrush.setStyle(Qt::NoBrush);
            // Draws one or two polygons with door colouring, matching drawRoom().
            auto drawExitPoly = [&](const QList<QPolygonF>& polys, int doorVal, bool hasExit) {
                bool isDoor = true;
                QBrush brush;
                switch (doorVal) {
                case 1:
                case 2:
                case 3:
                    brush.setColor(ExitAppearance::doorColor(doorVal));
                    innerPen.setColor(ExitAppearance::doorColor(doorVal));
                    break;
                default:
                    brush.setColor(lc);
                    isDoor = false;
                    break;
                }
                if (hasExit) {
                    exitPen.setWidthF(mRoomWidth * rSize * 0.050f);
                    innerPen.setWidthF(mRoomWidth * rSize * 0.025f);
                    brush.setStyle(Qt::Dense4Pattern);
                } else {
                    exitPen.setWidthF(mRoomWidth * rSize * 0.025f);
                    innerPen.setWidthF(mRoomWidth * rSize * 0.0125f);
                    brush.setStyle(Qt::DiagCrossPattern);
                }
                painter.setPen(exitPen);
                painter.setBrush(brush);
                for (const auto& poly : polys) {
                    painter.drawPolygon(poly);
                }
                if (isDoor) {
                    painter.setPen(innerPen);
                    painter.setBrush(innerBrush);
                    for (const auto& poly : polys) {
                        painter.drawPolygon(poly);
                    }
                }
            };

            if (room->getUp() > 0 || room->exitStubs.contains(DIR_UP)) {
                QPolygonF poly;
                poly << QPointF(rx, ry + (mRoomHeight * rSize * allInsideTipOffsetFactor)) << QPointF(rx - (mRoomWidth * rSize * upDownXOrYFactor), ry + (mRoomHeight * rSize * upDownXOrYFactor))
                     << QPointF(rx + (mRoomWidth * rSize * upDownXOrYFactor), ry + (mRoomHeight * rSize * upDownXOrYFactor));
                drawExitPoly({poly}, room->doors.value(key_up), room->getUp() > 0);
            }
            if (room->getDown() > 0 || room->exitStubs.contains(DIR_DOWN)) {
                QPolygonF poly;
                poly << QPointF(rx, ry - (mRoomHeight * rSize * allInsideTipOffsetFactor)) << QPointF(rx - (mRoomWidth * rSize * upDownXOrYFactor), ry - (mRoomHeight * rSize * upDownXOrYFactor))
                     << QPointF(rx + (mRoomWidth * rSize * upDownXOrYFactor), ry - (mRoomHeight * rSize * upDownXOrYFactor));
                drawExitPoly({poly}, room->doors.value(key_down), room->getDown() > 0);
            }
            if (room->getIn() > 0 || room->exitStubs.contains(DIR_IN)) {
                QPolygonF left, right;
                left << QPointF(rx - (mRoomWidth * rSize * allInsideTipOffsetFactor), ry) << QPointF(rx - (mRoomWidth * rSize * inOuterXFactor), ry + (mRoomHeight * rSize * inUpDownYFactor))
                     << QPointF(rx - (mRoomWidth * rSize * inOuterXFactor), ry - (mRoomHeight * rSize * inUpDownYFactor));
                right << QPointF(rx + (mRoomWidth * rSize * allInsideTipOffsetFactor), ry) << QPointF(rx + (mRoomWidth * rSize * inOuterXFactor), ry + (mRoomHeight * rSize * inUpDownYFactor))
                      << QPointF(rx + (mRoomWidth * rSize * inOuterXFactor), ry - (mRoomHeight * rSize * inUpDownYFactor));
                drawExitPoly({left, right}, room->doors.value(key_in), room->getIn() > 0);
            }
            if (room->getOut() > 0 || room->exitStubs.contains(DIR_OUT)) {
                QPolygonF left, right;
                left << QPointF(rx - (mRoomWidth * rSize * outOuterXFactor), ry) << QPointF(rx - (mRoomWidth * rSize * outInterXFactor), ry + (mRoomHeight * rSize * outUpDownYFactor))
                     << QPointF(rx - (mRoomWidth * rSize * outInterXFactor), ry - (mRoomHeight * rSize * outUpDownYFactor));
                right << QPointF(rx + (mRoomWidth * rSize * outOuterXFactor), ry) << QPointF(rx + (mRoomWidth * rSize * outInterXFactor), ry + (mRoomHeight * rSize * outUpDownYFactor))
                      << QPointF(rx + (mRoomWidth * rSize * outInterXFactor), ry - (mRoomHeight * rSize * outUpDownYFactor));
                drawExitPoly({left, right}, room->doors.value(key_out), room->getOut() > 0);
            }
        }
        painter.restore();
    }

    const qreal timeCollision = timer.nsecsElapsed() / 1000000.0;
    timer.start();
    // Decoration pass: symbols and V-numbers for rooms that need them.
    // Runs over the pre-filtered decoration list built during the collect pass,
    // so no second viewport scan is needed.
    for (const auto& entry : decorationRooms) {
        TRoom* room = entry.first;
        const QRectF& roomRect = entry.second;

        if (!showVnums && !room->mSymbol.isEmpty()) {
            const QColor roomColor = environmentColor(room->environment);
            const QColor symbolColor = RoomAppearance::symbolColor(room->mSymbolColor, roomColor);
            const QString pixmapKey = qsl("%1_%2").arg(symbolColor.name(), room->mSymbol);
            if (!mSymbolPixmapCache.contains(pixmapKey)) {
                addSymbolToPixmapCache(pixmapKey, room->mSymbol, symbolColor, true);
            }
            if (const QPixmap* pix = mSymbolPixmapCache.object(pixmapKey); pix && !pix->isNull()) {
                painter.save();
                painter.setBackgroundMode(Qt::TransparentMode);
                painter.drawPixmap(QPoint(qRound(roomRect.left() + (roomRect.width() - pix->width()) / 2.0), qRound(roomRect.top() + (roomRect.height() - pix->height()) / 2.0)), *pix);
                painter.restore();
            }
        }

        if (showVnums) {
            const QColor roomColor = environmentColor(room->environment);
            const QColor textColor = (roomColor.lightness() > 127) ? QColor(Qt::black) : QColor(Qt::white);
            painter.save();
            painter.setPen(QPen(textColor));
            painter.setFont(roomVNumFont);
            painter.drawText(roomRect, Qt::AlignCenter, QString::number(room->getId()));
            painter.restore();
        }
    }

    const qreal timeDecor = timer.nsecsElapsed() / 1000000.0;

    // Handle mPick (double-click speedwalk) for grid mode (full-detail path).
    // Use rounding to snap the click to the nearest room centre — floor() would
    // miss rooms clicked in the left or bottom half of their cell.
    if (mPick) {
        const int clickWorldX = qRound(static_cast<double>(mPHighlight.x() - mRX) / mRoomWidth);
        const int clickWorldY = qRound(static_cast<double>(mRY - mPHighlight.y()) / mRoomHeight);
        const TAreaGridIndex::RoomIds& clickedRooms = gridIndex.roomsAt(zLevel, clickWorldX, clickWorldY);
        if (!clickedRooms.isEmpty()) {
            const int clickedRoomId = *clickedRooms.constBegin();
            mPick = false;
            if (mStartSpeedWalk) {
                mStartSpeedWalk = false;
                mPendingSpeedWalk = {playerRoomId, clickedRoomId};
            }
        } else {
            mPick = false;
        }
    }

    if (qEnvironmentVariableIsSet("MUDLET_PROFILE_MAP") && profileOutput) {
        // Build the profiling string and return it to paintEvent so it can
        // be printed AFTER phase6Time is captured.  Calling qDebug() here
        // would inflate phase6 time by 100-200 ms on Windows debug builds.
        QDebug dbg(profileOutput);
        dbg.noquote().nospace() << "drawGridModeRooms timing (ms):" << " total:" << (timeIndex + timeCollect + timeBatchDraw + timeCollision + timeDecor) << " indexSetup:" << timeIndex
                                << " collect(gridIndex):" << timeCollect << " batchDraw:" << timeBatchDraw << " collision:" << timeCollision << " decor:" << timeDecor << " visibleRooms:" << roomCount
                                << " viewportColumns:" << (static_cast<qint64>(maxX) - minX + 1) << " viewportRows:" << (static_cast<qint64>(maxY) - minY + 1) << " viewportBounds: x[" << minX << ","
                                << maxX << "] y[" << minY << "," << maxY << "]" << " gridIndexRooms:" << gridIndex.size() << " gridIndexBytes:" << gridIndex.memoryEstimateBytes();
    }
}

// Rooms are written as environment colours into scanlines and blitted once. Kept: the selection tint
// (selecting a region and zooming out is why you'd be here) and highlights, whose script-set radius
// can far exceed the room.
void TMapPainter::drawNonGridModeRoomsLod(QPainter& painter,
                                          const TArea* pDrawnArea,
                                          const int zLevel,
                                          const int playerRoomId,
                                          const QList<int>& viewportRooms,
                                          const float widgetWidth,
                                          const float widgetHeight,
                                          bool& isPlayerRoomVisible,
                                          QPointF& playerRoomOnWidgetCoordinates,
                                          bool& isPlayerRoomBodyDrawn,
                                          QString* profileOutput)
{
    QElapsedTimer timer;
    timer.start();

    const int imgW = static_cast<int>(std::ceil(static_cast<double>(widgetWidth)));
    const int imgH = static_cast<int>(std::ceil(static_cast<double>(widgetHeight)));
    QImage lodImage(imgW, imgH, QImage::Format_ARGB32_Premultiplied);
    lodImage.fill(0); // transparent: the background and the exit lines are already under us

    // Non-grid rooms are drawn at rSize of their cell.
    const float blobWidth = mRoomWidth * static_cast<float>(rSize);
    const float blobHeight = mRoomHeight * static_cast<float>(rSize);
    const QSize blobSize = lodRoomBlobSize();
    const int blobPixelWidth = blobSize.width();
    const int blobPixelHeight = blobSize.height();

    // Premultiplied: see drawGridModeRooms().
    QHash<int, QRgb> envColorCache;
    QList<QPair<TRoom*, QPointF>> highlightedRooms;
    QList<QRect> selectedRoomRects;
    TRoom* pPlayerRoom = nullptr;

    int roomCount = 0;

    // Zoomed out this far every room on screen is visited, and each visit is a
    // chain of dependent loads that miss main memory: the hash lookup for the
    // id, then the room it points at. Interleaved with the drawing work, the
    // chains barely overlap; resolved first in a tight loop of their own they
    // do, and the drawing loop then prefetches the rooms ahead (16 measured as
    // good as 8 or 32 on a 2.3 million room map, and better than 64). The
    // fields read below sit in the first 16 bytes of TRoom, so one prefetch
    // covers them.
    std::vector<TRoom*> resolvedRooms;
    resolvedRooms.reserve(viewportRooms.size());
    for (const int roomId : viewportRooms) {
        resolvedRooms.push_back(mpMap->mpRoomDB->getRoom(roomId));
    }
    constexpr qsizetype prefetchDistance = 16;
    const qsizetype candidateCount = viewportRooms.size();

    for (qsizetype index = 0; index < candidateCount; ++index) {
        if (index + prefetchDistance < candidateCount) {
            __builtin_prefetch(resolvedRooms[index + prefetchDistance]);
        }
        TRoom* room = resolvedRooms[index];
        if (!room) {
            continue;
        }
        const int roomId = viewportRooms.at(index);

        const float rx = room->x() * mRoomWidth + static_cast<float>(mRX);
        const float ry = room->y() * -1 * mRoomHeight + static_cast<float>(mRY);
        if (rx < 0 || ry < 0 || rx > widgetWidth || ry > widgetHeight) {
            continue;
        }

        if (playerRoomId == roomId) {
            // Drawn in full at the end of the phase, as it is at any zoom.
            isPlayerRoomVisible = true;
            playerRoomOnWidgetCoordinates = QPointF(static_cast<qreal>(rx), static_cast<qreal>(ry));
            pPlayerRoom = room;
            continue;
        }

        if (room->isHidden()) {
            continue;
        }

        ++roomCount;
        const int px = qRound(static_cast<double>(rx - blobWidth * 0.5f));
        const int py = qRound(static_cast<double>(ry - blobHeight * 0.5f));
        const int xStart = qBound(0, px, imgW);
        const int xEnd = qBound(0, px + blobPixelWidth, imgW);
        const int yStart = qBound(0, py, imgH);
        const int yEnd = qBound(0, py + blobPixelHeight, imgH);
        if (xStart >= xEnd || yStart >= yEnd) {
            continue;
        }

        QRgb colorRgb;
        const auto colorIt = envColorCache.constFind(room->environment);
        if (colorIt != envColorCache.constEnd()) {
            colorRgb = *colorIt;
        } else {
            colorRgb = qPremultiply(environmentColor(room->environment).rgba());
            envColorCache.insert(room->environment, colorRgb);
        }

        for (int dy = yStart; dy < yEnd; ++dy) {
            QRgb* scanLine = reinterpret_cast<QRgb*>(lodImage.scanLine(dy));
            for (int dx = xStart; dx < xEnd; ++dx) {
                scanLine[dx] = colorRgb;
            }
        }

        if (!mMultiSelectionSet.isEmpty() && mMultiSelectionSet.contains(roomId)) {
            selectedRoomRects.append(QRect(xStart, yStart, xEnd - xStart, yEnd - yStart));
        }
        if (room->highlight) {
            highlightedRooms.append({room, QPointF(static_cast<qreal>(rx), static_cast<qreal>(ry))});
        }
    }

    const qreal timeCollect = timer.nsecsElapsed() / 1000000.0;
    timer.start();

    painter.drawImage(0, 0, lodImage);

    // Stands in for the full-detail selection gradient, whose two ends are one pixel at this size.
    if (!selectedRoomRects.isEmpty()) {
        painter.save();
        painter.setPen(Qt::NoPen);
        const QColor selectionTint(0, 0, 255, 180);
        for (const QRect& selectedRect : std::as_const(selectedRoomRects)) {
            painter.fillRect(selectedRect, selectionTint);
        }
        painter.restore();
    }

    if (pPlayerRoom && !highlightedRooms.isEmpty()) {
        const bool playerRoomCollision = pDrawnArea->getGridIndex().roomsAt(zLevel, pPlayerRoom->x(), pPlayerRoom->y()).size() > 1;
        drawPlayerRoomBody(painter, pPlayerRoom, playerRoomOnWidgetCoordinates, false, playerRoomCollision);
        isPlayerRoomBodyDrawn = true;
    }

    for (const auto& [room, roomCenter] : std::as_const(highlightedRooms)) {
        const float roomRadius = (room->highlightRadius * mRoomWidth) / 2.0f;
        QRadialGradient gradient(roomCenter, roomRadius);
        gradient.setStops(RoomAppearance::highlightStops(room->highlightColor, room->highlightColor2));
        painter.save();
        painter.setBrush(gradient);
        painter.setPen(QPen(Qt::transparent));
        QPainterPath highlightPath;
        highlightPath.addEllipse(roomCenter, roomRadius, roomRadius);
        painter.drawPath(highlightPath);
        painter.restore();
    }

    const qreal timeBlit = timer.nsecsElapsed() / 1000000.0;

    if (qEnvironmentVariableIsSet("MUDLET_PROFILE_MAP") && profileOutput) {
        // paintEvent prints it, keeping qDebug() out of the timed phase.
        QDebug dbg(profileOutput);
        dbg.noquote().nospace() << "drawNonGridModeRoomsLod timing (ms):" << " total:" << (timeCollect + timeBlit) << " collect+pixelWrite:" << timeCollect << " imageBlit:" << timeBlit
                                << " candidates:" << viewportRooms.size() << " drawnRooms:" << roomCount << " roomSizePx:" << mRoomWidth << " blobPx:" << blobPixelWidth << "x" << blobPixelHeight;
    }

    // drawRoom(), which resolves speed-walk clicks, isn't called here, so use the grid. Clear mPick only
    // on a hit: paintEvent() then offers the click to the area exit markers.
    if (mPick) {
        const int clickWorldX = qRound(static_cast<double>(mPHighlight.x() - mRX) / mRoomWidth);
        const int clickWorldY = qRound(static_cast<double>(mRY - mPHighlight.y()) / mRoomHeight);
        const TAreaGridIndex::RoomIds& clickedRooms = pDrawnArea->getGridIndex().roomsAt(zLevel, clickWorldX, clickWorldY);
        for (const int clickedRoomId : clickedRooms) {
            // The index includes hidden rooms, which drawRoom() never hit-tests either.
            const TRoom* pClickedRoom = mpMap->mpRoomDB->getRoom(clickedRoomId);
            if (!pClickedRoom || pClickedRoom->isHidden()) {
                continue;
            }
            mPick = false;
            if (mStartSpeedWalk) {
                mStartSpeedWalk = false;
                mPendingSpeedWalk = {playerRoomId, clickedRoomId};
            }
            break;
        }
    }
}

// This draws two lines at angles to the "exitLine" so as to form what would be
// an "arrow head" if they were to be extended so as to meet (at the "end" of
// the "exitLine". Various features of the QPen that is used are redefined
// as appropriate - but they are restored afterwards so there should be
// no change to the QPainter as a result of calling this method.
void TMapPainter::drawDoor(QPainter& painter, const TRoom& room, const QString& dirKey, const QLineF& exitLine)
{
    const float doorWidthFactor = 1.5;
    const auto [strokeA, strokeB] = ExitAppearance::doorStrokes(exitLine, mRoomWidth + mRoomHeight);

    painter.save();
    QPen doorPen = painter.pen();
    doorPen.setWidthF(painter.pen().widthF() * doorWidthFactor);
    doorPen.setCosmetic(mMapperUseAntiAlias);
    doorPen.setStyle(Qt::SolidLine);
    doorPen.setCapStyle(Qt::RoundCap);
    doorPen.setColor(ExitAppearance::doorColor(room.doors.value(dirKey)));
    painter.setPen(doorPen);
    painter.drawLine(strokeA);
    painter.drawLine(strokeB);
    painter.restore();
}

// Not QRect::contains(): it computes left - 1, which overflows at the coordinate limit and inverts the test.
static bool withinViewportBounds(const QRect& bounds, const int x, const int y)
{
    return x >= bounds.left() && x <= bounds.right() && y >= bounds.top() && y <= bounds.bottom();
}

// Of the dot on the end of an exit stub, as a fraction of a room's spacing
static constexpr qreal cStubDotRadius = 0.1;

void TMapPainter::paintRoomExits(QPainter& painter,
                                 QPen& pen,
                                 QList<ExitToPaint>& exitList,
                                 const TArea* pArea,
                                 int zLevel,
                                 const QRect& roomBounds,
                                 const QList<int>& viewportRooms,
                                 float exitWidth,
                                 QMap<int, QPointF>& areaExitsMap)
{
    const float exitArrowScale = (mLargeAreaExitArrows ? 2.0f : 1.0f);
    const float widgetWidth = width();
    const float widgetHeight = height();
    // Rooms are drawn as blobs over these lines; keepExit() drops lines hidden under their end blobs.
    const bool lodTier = mRoomWidth < cLodPixelThreshold;
    const QSize lodBlobSize = lodRoomBlobSize();

    // Largest per-axis delta (room units) that keepExit() surely drops, settled with keepExit()'s own
    // float test (monotonic in delta) as the exit index may only err towards too many rooms. The loops
    // fix the division's rounding.
    int maxSkippableSpan = 0;
    if (lodTier && mRoomWidth > 0.0f && mRoomHeight > 0.0f) {
        const auto exitDropped = [&](const int span) {
            return float(span) * mRoomWidth <= lodBlobSize.width() && float(span) * mRoomHeight <= lodBlobSize.height();
        };
        const double unitsAcrossBlob = qMin(lodBlobSize.width() / double(mRoomWidth), lodBlobSize.height() / double(mRoomHeight));
        maxSkippableSpan = int(qBound(0.0, std::floor(unitsAcrossBlob), double(INT_MAX / 2)));
        while (maxSkippableSpan > 0 && !exitDropped(maxSkippableSpan)) {
            --maxSkippableSpan;
        }
        if (exitDropped(maxSkippableSpan + 1)) {
            ++maxSkippableSpan;
        }
    }

    int customLineDestinationTarget = 0;
    if (mCustomLinesRoomTo > 0) {
        customLineDestinationTarget = mCustomLinesRoomTo;
    } else if (mCustomLineSelectedRoom > 0 && !mCustomLineSelectedExit.isEmpty()) {
        TRoom* pSR = mpMap->mpRoomDB->getRoom(mCustomLineSelectedRoom);
        if (pSR) {
            customLineDestinationTarget = customLineDestination(*pSR, mCustomLineSelectedExit);
        }
    }
    // Viewport rooms plus every custom-line room on the level, as such a line can cross the whole level.
    // The copy is free unless appended to. In the reduced tier the exit index can instead give just the
    // rooms whose exits survive (a handful vs hundreds of thousands); used only when smaller, as it
    // isn't viewport-culled.
    QList<int> roomsToPaint;
    mLodExitIndexRoomsHandedOver = -1;
    if (!mLodExitIndexDisabled && maxSkippableSpan > 0 && pArea->lodVisibleExitRoomCount(zLevel, maxSkippableSpan) < viewportRooms.size()) {
        roomsToPaint = pArea->lodVisibleExitRooms(zLevel, maxSkippableSpan);
        mLodExitIndexRoomsHandedOver = int(roomsToPaint.size());
        QSet<int> alreadyListed{roomsToPaint.cbegin(), roomsToPaint.cend()};
        for (const int customLineRoomId : pArea->getCustomLineRoomsForZ(zLevel)) {
            // All of them: the index only covers exits drawn as plain lines.
            if (!alreadyListed.contains(customLineRoomId)) {
                alreadyListed.insert(customLineRoomId);
                roomsToPaint.append(customLineRoomId);
            }
        }
        // The loop draws a target ring on whatever room the in-progress custom line leads to:
        if (customLineDestinationTarget > 0 && !alreadyListed.contains(customLineDestinationTarget)) {
            const TRoom* pTargetRoom = mpMap->mpRoomDB->getRoom(customLineDestinationTarget);
            if (pTargetRoom && pTargetRoom->getArea() == mAreaID && pTargetRoom->z() == zLevel && withinViewportBounds(roomBounds, pTargetRoom->x(), pTargetRoom->y())) {
                roomsToPaint.append(customLineDestinationTarget);
            }
        }
    } else {
        roomsToPaint = viewportRooms;
        for (const int customLineRoomId : pArea->getCustomLineRoomsForZ(zLevel)) {
            TRoom* pRoomWithCustomLines = mpMap->mpRoomDB->getRoom(customLineRoomId);
            // Already listed, and painting exits twice doesn't look like painting them once.
            if (!pRoomWithCustomLines || withinViewportBounds(roomBounds, pRoomWithCustomLines->x(), pRoomWithCustomLines->y())) {
                continue;
            }
            roomsToPaint.append(customLineRoomId);
        }
    }

    const auto exitsReachWidget = [&](const TRoom* pRoom) {
        if (pRoom->customLines.empty()) {
            const float roomX = pRoom->x() * mRoomWidth + mRX;
            const float roomY = pRoom->y() * -1 * mRoomHeight + mRY;
            return !(roomX < 0 || roomY < 0 || roomX > widgetWidth || roomY > widgetHeight);
        }
        const float miny = pRoom->min_y * -1 * mRoomHeight + static_cast<float>(mRY);
        const float maxy = pRoom->max_y * -1 * mRoomHeight + static_cast<float>(mRY);
        const float minx = pRoom->min_x * mRoomWidth + static_cast<float>(mRX);
        const float maxx = pRoom->max_x * mRoomWidth + static_cast<float>(mRX);
        return (minx > 0.0 || maxx > 0.0) && (static_cast<float>(widgetWidth) > minx || static_cast<float>(widgetWidth) > maxx) && (miny > 0.0 || maxy > 0.0)
               && (static_cast<float>(widgetHeight) > miny || static_cast<float>(widgetHeight) > maxy);
    };

    // Every room with custom lines on this level, wherever it is; roomsToPaint holds them all, and the
    // loop below paints those that are not hidden and whose lines reach the widget.
    const QSet<int>& customLineRooms = pArea->getCustomLineRoomsForZ(zLevel);
    for (const int _id : std::as_const(roomsToPaint)) {
        TRoom* room = mpMap->mpRoomDB->getRoom(_id);
        if (!room) {
            continue;
        }
        if (room->isHidden()) {
            continue;
        }
        const float rx = room->x() * mRoomWidth + mRX;
        const float ry = room->y() * -1 * mRoomHeight + mRY;
        if (!exitsReachWidget(room)) {
            continue;
        }

        room->rendered = true;

        // Drop lines hidden under the blobs before gathering: at the furthest zoom that is every exit, and
        // gathering them cost ~98ms a frame. Per axis because blobs are rectangles. Area exits stay: their
        // fixed-size marker is the speed-walk click target. A delta scaled once, not two rounded screen
        // positions, so maxSkippableSpan can reason about it exactly.
        const auto keepExit = [&](const TRoom* pDestination) {
            if (!lodTier || pDestination->getArea() != mAreaID) {
                return true;
            }
            const float xReach = float(qAbs(qint64{pDestination->x()} - room->x())) * mRoomWidth;
            const float yReach = float(qAbs(qint64{pDestination->y()} - room->y())) * mRoomHeight;
            return xReach > lodBlobSize.width() || yReach > lodBlobSize.height();
        };

        // exitList is a list of the destination rooms reached by exit lines
        // that are NOT custom exit lines from this room so are places to
        // which a straight line is to be drawn from the centre of this room
        // to the center of the exit room, unless a straight custom line
        // already draws it; this does mean that multiple exits to the same
        // room are drawn on top of each other and that there is no
        // indication from which exit direction they are for...!
        exitList.clear();
        if (!room->customLines.empty()) {
            // This room has custom exit lines:
            if (!room->customLines.contains(key_n)) {
                TRoom* pER = mpMap->mpRoomDB->getRoom(room->getNorth());
                if (pER && !pER->isHidden() && keepExit(pER)) {
                    exitList.push_back({pER, room->getNorth(), pER->getSouth() != _id});
                }
            }
            if (!room->customLines.contains(key_ne)) {
                TRoom* pER = mpMap->mpRoomDB->getRoom(room->getNortheast());
                if (pER && !pER->isHidden() && keepExit(pER)) {
                    exitList.push_back({pER, room->getNortheast(), pER->getSouthwest() != _id});
                }
            }
            if (!room->customLines.contains(key_e)) {
                TRoom* pER = mpMap->mpRoomDB->getRoom(room->getEast());
                if (pER && !pER->isHidden() && keepExit(pER)) {
                    exitList.push_back({pER, room->getEast(), pER->getWest() != _id});
                }
            }
            if (!room->customLines.contains(key_se)) {
                TRoom* pER = mpMap->mpRoomDB->getRoom(room->getSoutheast());
                if (pER && !pER->isHidden() && keepExit(pER)) {
                    exitList.push_back({pER, room->getSoutheast(), pER->getNorthwest() != _id});
                }
            }
            if (!room->customLines.contains(key_s)) {
                TRoom* pER = mpMap->mpRoomDB->getRoom(room->getSouth());
                if (pER && !pER->isHidden() && keepExit(pER)) {
                    exitList.push_back({pER, room->getSouth(), pER->getNorth() != _id});
                }
            }
            if (!room->customLines.contains(key_sw)) {
                TRoom* pER = mpMap->mpRoomDB->getRoom(room->getSouthwest());
                if (pER && !pER->isHidden() && keepExit(pER)) {
                    exitList.push_back({pER, room->getSouthwest(), pER->getNortheast() != _id});
                }
            }
            if (!room->customLines.contains(key_w)) {
                TRoom* pER = mpMap->mpRoomDB->getRoom(room->getWest());
                if (pER && !pER->isHidden() && keepExit(pER)) {
                    exitList.push_back({pER, room->getWest(), pER->getEast() != _id});
                }
            }
            if (!room->customLines.contains(key_nw)) {
                TRoom* pER = mpMap->mpRoomDB->getRoom(room->getNorthwest());
                if (pER && !pER->isHidden() && keepExit(pER)) {
                    exitList.push_back({pER, room->getNorthwest(), pER->getSoutheast() != _id});
                }
            }
        } else {
            int exitRoomId = room->getNorth();
            if (exitRoomId > 0) {
                TRoom* pER = mpMap->mpRoomDB->getRoom(exitRoomId);
                if (pER && !pER->isHidden() && keepExit(pER)) {
                    exitList.push_back({pER, exitRoomId, pER->getSouth() != _id});
                }
            }
            exitRoomId = room->getNortheast();
            if (exitRoomId > 0) {
                TRoom* pER = mpMap->mpRoomDB->getRoom(exitRoomId);
                if (pER && !pER->isHidden() && keepExit(pER)) {
                    exitList.push_back({pER, exitRoomId, pER->getSouthwest() != _id});
                }
            }
            exitRoomId = room->getEast();
            if (exitRoomId > 0) {
                TRoom* pER = mpMap->mpRoomDB->getRoom(exitRoomId);
                if (pER && !pER->isHidden() && keepExit(pER)) {
                    exitList.push_back({pER, exitRoomId, pER->getWest() != _id});
                }
            }
            exitRoomId = room->getSoutheast();
            if (exitRoomId > 0) {
                TRoom* pER = mpMap->mpRoomDB->getRoom(exitRoomId);
                if (pER && !pER->isHidden() && keepExit(pER)) {
                    exitList.push_back({pER, exitRoomId, pER->getNorthwest() != _id});
                }
            }
            exitRoomId = room->getSouth();
            if (exitRoomId > 0) {
                TRoom* pER = mpMap->mpRoomDB->getRoom(exitRoomId);
                if (pER && !pER->isHidden() && keepExit(pER)) {
                    exitList.push_back({pER, exitRoomId, pER->getNorth() != _id});
                }
            }
            exitRoomId = room->getSouthwest();
            if (exitRoomId > 0) {
                TRoom* pER = mpMap->mpRoomDB->getRoom(exitRoomId);
                if (pER && !pER->isHidden() && keepExit(pER)) {
                    exitList.push_back({pER, exitRoomId, pER->getNortheast() != _id});
                }
            }
            exitRoomId = room->getWest();
            if (exitRoomId > 0) {
                TRoom* pER = mpMap->mpRoomDB->getRoom(exitRoomId);
                if (pER && !pER->isHidden() && keepExit(pER)) {
                    exitList.push_back({pER, exitRoomId, pER->getEast() != _id});
                }
            }
            exitRoomId = room->getNorthwest();
            if (exitRoomId > 0) {
                TRoom* pER = mpMap->mpRoomDB->getRoom(exitRoomId);
                if (pER && !pER->isHidden() && keepExit(pER)) {
                    exitList.push_back({pER, exitRoomId, pER->getSoutheast() != _id});
                }
            }
        }
        // A straight custom line between the two rooms, from either end, already draws a two-way exit; a plain
        // line would land on top of it or under it, whichever room paints last. Only where that line is painted.
        for (ExitToPaint& exit : exitList) {
            const TRoom* pDestination = exit.destination;
            exit.drawnByCustomLine = !exit.oneWay
                                     && (customLineRunsStraightTo(*room, *pDestination)
                                         || (customLineRooms.contains(exit.destinationId) && exitsReachWidget(pDestination) && customLineRunsStraightTo(*pDestination, *room)));
        }

        if (!room->customLines.empty()) {
            painter.save();
            QMapIterator<QString, QList<QPointF>> itk(room->customLines);
            while (itk.hasNext()) {
                itk.next();
                QColor customLineColor;
                if (_id == mCustomLineSelectedRoom && itk.key() == mCustomLineSelectedExit) {
                    customLineColor = QColor(255, 155, 55);
                } else {
                    customLineColor = room->customLinesColor.value(itk.key(), Qt::red);
                }

                const float ex = room->x() * mRoomWidth + mRX;
                const float ey = room->y() * mRoomHeight * -1 + mRY;
                const QPointF origin = QPointF(ex, ey);
                // The following sets a point offset from the room center
                // that depends on the exit direction that the custom line
                // heads to from the room center - it forms a fixed segment
                // that cannot be moved - for XY-plane exits:
                QPointF fixedOffsetPoint;
                bool isXYPlainExit = false;
                if (itk.key() == key_n) {
                    fixedOffsetPoint = QPointF(ex, ey - mRoomHeight / 2.0);
                    isXYPlainExit = true;
                } else if (itk.key() == key_ne) {
                    fixedOffsetPoint = QPointF(ex + mRoomWidth / 2.0, ey - mRoomHeight / 2.0);
                    isXYPlainExit = true;
                } else if (itk.key() == key_e) {
                    fixedOffsetPoint = QPointF(ex + mRoomWidth / 2.0, ey);
                    isXYPlainExit = true;
                } else if (itk.key() == key_se) {
                    fixedOffsetPoint = QPointF(ex + mRoomWidth / 2.0, ey + mRoomHeight / 2.0);
                    isXYPlainExit = true;
                } else if (itk.key() == key_s) {
                    fixedOffsetPoint = QPointF(ex, ey + mRoomHeight / 2.0);
                    isXYPlainExit = true;
                } else if (itk.key() == key_sw) {
                    fixedOffsetPoint = QPointF(ex - mRoomWidth / 2.0, ey + mRoomHeight / 2.0);
                    isXYPlainExit = true;
                } else if (itk.key() == key_w) {
                    fixedOffsetPoint = QPointF(ex - mRoomWidth / 2.0, ey);
                    isXYPlainExit = true;
                } else if (itk.key() == key_nw) {
                    fixedOffsetPoint = QPointF(ex - mRoomWidth / 2.0, ey - mRoomHeight / 2.0);
                    isXYPlainExit = true;
                } else {
                    fixedOffsetPoint = QPointF(ex, ey);
                }
                QPen customLinePen = painter.pen();
                customLinePen.setCosmetic(mMapperUseAntiAlias);
                customLinePen.setWidthF(exitWidth);
                customLinePen.setCapStyle(Qt::RoundCap);
                customLinePen.setJoinStyle(Qt::RoundJoin);
                customLinePen.setColor(customLineColor);
                customLinePen.setStyle(room->customLinesStyle.value(itk.key()));

                QVector<QPointF> polyLinePoints;
                QList<QPointF> const customLinePoints = itk.value();
                QLineF doorLineSegment;
                if (!customLinePoints.empty()) {
                    painter.setPen(customLinePen);
                    polyLinePoints << origin;
                    polyLinePoints << fixedOffsetPoint;
                    for (const QPointF& customLinePoint : customLinePoints) {
                        polyLinePoints << QPointF(customLinePoint.x() * mRoomWidth + mRX, customLinePoint.y() * mRoomHeight * -1 + mRY);
                    }
                    if (polyLinePoints.size() > 2) {
                        if (isXYPlainExit) {
                            doorLineSegment = QLineF{polyLinePoints.at(0), polyLinePoints.at(1)};
                        } else {
                            // Non-XY-Plane exits have the first two points
                            // being coincident:
                            doorLineSegment = QLineF{polyLinePoints.at(1), polyLinePoints.at(2)};
                        }
                    } else {
                        // There must be 2 points ...
                        doorLineSegment = QLineF{polyLinePoints.at(0), polyLinePoints.at(1)};
                    }
                    painter.drawPolyline(polyLinePoints.data(), polyLinePoints.size());

                    if (room->customLinesArrow.value(itk.key())) {
                        QLineF l0 = QLineF(polyLinePoints.last(), polyLinePoints.at(polyLinePoints.size() - 2));
                        l0.setLength(exitWidth * 5.0);
                        const QPointF _p1 = l0.p1();
                        const QPointF _p2 = l0.p2();
                        const QLineF l1 = QLineF(l0);
                        const qreal w1 = l1.angle() - 90.0;
                        QLineF l2;
                        l2.setP1(_p2);
                        l2.setAngle(w1);
                        l2.setLength(exitWidth * 2.0);
                        const QPointF _p3 = l2.p2();
                        l2.setAngle(l2.angle() + 180.0);
                        const QPointF _p4 = l2.p2();
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
                        const QPen _savedPen = painter.pen();
                        QPen _pen;
                        const QBrush _brush = painter.brush();
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

                if (room->doors.value(itk.key())) {
                    drawDoor(painter, *room, itk.key(), doorLineSegment);
                }
            }
            painter.restore();
        }

        // draw exit stubs
        for (const int direction : std::as_const(room->exitStubs)) {
            if (direction >= DIR_NORTH && direction <= DIR_SOUTHWEST) {
                // Stubs on non-XY plane exits are handled differently and we
                // do not support special exit stubs (yet?)
                const QVector3D uDirection = mpMap->scmUnitVectors.value(direction);
                const bool diagonal = uDirection.x() != 0.0f && uDirection.y() != 0.0f;
                const int borderWidth = !lodTier && roomBorderShown(*room) ? roomBorderWidth(*room) : 0;
                // The room and its border are painted over the stub later, so past its half cell it reaches
                // only far enough for the dot to clear them: any further puts the dot under a room next door
                const auto stubReach = [&](const qreal cellSize) {
                    const qreal roomEdge = rSize * cellSize / 2.0 + borderWidth;
                    const qreal dotRadius = cStubDotRadius * cellSize;
                    if (!diagonal) {
                        return qMax(0.5 * cellSize, roomEdge + dotRadius);
                    }
                    if (mBubbleMode) {
                        return qMax(0.5 * cellSize, (roomEdge + dotRadius) / std::numbers::sqrt2);
                    }
                    return qMax(0.5 * cellSize, roomEdge + dotRadius / std::numbers::sqrt2);
                };
                const QLineF stubLine(rx, ry, rx + uDirection.x() * stubReach(mRoomWidth), ry + uDirection.y() * stubReach(mRoomHeight));
                const QString doorKey{TRoom::dirCodeToShortString(direction)};
                // Draw the door lines before we draw the stub or the filled
                // circle on the end - so that the latter overlays the doors
                // if they get a bit large (at low exit size numbers)
                if (room->doors.value(doorKey)) {
                    drawDoor(painter, *room, doorKey, stubLine);
                }
                painter.save();
                painter.drawLine(stubLine);
                // Set the fill colour to be what is used for exit lines
                painter.setBrush(painter.pen().color());
                // And turn off drawing the border (outline):
                painter.setPen(Qt::NoPen);
                QPainterPath stubMarkingCirclePath;
                const QRectF surroundingRectF(
                        stubLine.p2().x() - cStubDotRadius * mRoomWidth, stubLine.p2().y() - cStubDotRadius * mRoomHeight, 2.0 * cStubDotRadius * mRoomWidth, 2.0 * cStubDotRadius * mRoomHeight);
                stubMarkingCirclePath.arcTo(surroundingRectF, 0.0, 360.0);
                // So this should draw a solid filled circle whose diameter
                // is fixed and not dependent on the exit line thickness:
                painter.drawPath(stubMarkingCirclePath);
                painter.restore();
            }
        }

        for (const ExitToPaint& exit : exitList) {
            const int rID = exit.destinationId;
            // Defensive: TRoomDB::addRoom() rejects ids <= 0.
            if (rID <= 0) {
                continue;
            }

            const TRoom* pE = exit.destination;
            const bool areaExit = pE->getArea() != mAreaID;
            const float ex = pE->x() * mRoomWidth + mRX;
            const float ey = pE->y() * mRoomHeight * -1 + mRY;
            const int ez = pE->z();

            const QVector3D p1(ex, ey, ez);
            const int rz = mMapCenterZ;
            const QVector3D p2(rx, ry, rz);
            // This was a QLine (so used integer coordinates), but lets
            // try with a QLineF as we are using floating point numbers:
            QLineF line;
            if (!areaExit && !exit.drawnByCustomLine) {
                // Non-area exit:
                // Exits to the same room overlap, so one being one-way arrows them all.
                bool oneWayToDestination = false;
                for (const ExitToPaint& other : exitList) {
                    if (other.oneWay && other.destinationId == rID) {
                        oneWayToDestination = true;
                        break;
                    }
                }
                if (!oneWayToDestination) {
                    // Two way exit. drawLine() does not itself touch any
                    // painter state, so there is nothing here for save()
                    // /restore() to protect - removed along with the QPen
                    // copy that was never installed via setPen() and so
                    // never actually took effect.
                    const QLineF l0 = QLineF(p2.toPointF(), p1.toPointF());
                    painter.drawLine(l0);
                } else {
                    // one way non-area exit - draw arrow
                    QLineF l0 = QLineF(p2.toPointF(), p1.toPointF());
                    QLineF k0 = l0;
                    k0.setLength((l0.length() - exitWidth * 5.0) / 2.0);
                    const qreal dx = k0.dx();
                    const qreal dy = k0.dy();
                    painter.save();
                    QPen arrowPen = painter.pen();
                    QPen oneWayLinePen = painter.pen();
                    QBrush brush = painter.brush();

                    oneWayLinePen.setStyle(Qt::DotLine);
                    oneWayLinePen.setCapStyle(Qt::SquareCap);
                    painter.setPen(oneWayLinePen);
                    painter.drawLine(l0);

                    l0.setLength(exitWidth * 5.0);
                    const QPointF _p1 = l0.p2();
                    const QPointF _p2 = l0.p1();
                    const QLineF l1 = QLineF(l0);
                    const qreal w1 = l1.angle() - 90.0;
                    QLineF l2;
                    l2.setP1(_p2);
                    l2.setAngle(w1);
                    l2.setLength(exitWidth * 2.0);
                    const QPointF _p3 = l2.p2();
                    l2.setAngle(l2.angle() + 180.0);
                    const QPointF _p4 = l2.p2();
                    QPolygonF poly;
                    poly.append(_p1);
                    poly.append(_p3);
                    poly.append(_p4);
                    arrowPen.setCosmetic(mMapperUseAntiAlias);
                    arrowPen.setStyle(Qt::SolidLine);
                    arrowPen.setJoinStyle(Qt::RoundJoin);
                    arrowPen.setCapStyle(Qt::RoundCap);
                    brush.setColor(ExitAppearance::oneWayArrowColor());
                    brush.setStyle(Qt::SolidPattern);
                    painter.setPen(arrowPen);
                    painter.setBrush(brush);
                    painter.drawPolygon(poly.translated(dx, dy));

                    painter.restore();
                }

            } else if (areaExit) {
                // Area exit:
                painter.save();
                QPointF clickPoint;
                pen = painter.pen();
                pen.setWidthF(exitWidth);
                pen.setCapStyle(Qt::RoundCap);
                pen.setCosmetic(mMapperUseAntiAlias);
                pen.setColor(mpMap->getColor(rID));
                painter.setPen(pen);
                if (room->getSouth() == rID) {
                    line = QLineF(p2.x(), p2.y() + exitArrowScale * mRoomHeight, p2.x(), p2.y());
                    clickPoint = QPointF(p2.x(), p2.y() + mRoomHeight);
                } else if (room->getNorth() == rID) {
                    line = QLineF(p2.x(), p2.y() - exitArrowScale * mRoomHeight, p2.x(), p2.y());
                    clickPoint = QPointF(p2.x(), p2.y() - mRoomHeight);
                } else if (room->getWest() == rID) {
                    line = QLineF(p2.x() - exitArrowScale * mRoomWidth, p2.y(), p2.x(), p2.y());
                    clickPoint = QPointF(p2.x() - mRoomWidth, p2.y());
                } else if (room->getEast() == rID) {
                    line = QLineF(p2.x() + exitArrowScale * mRoomWidth, p2.y(), p2.x(), p2.y());
                    clickPoint = QPointF(p2.x() + mRoomWidth, p2.y());
                } else if (room->getNorthwest() == rID) {
                    line = QLineF(p2.x() - exitArrowScale * mRoomWidth, p2.y() - exitArrowScale * mRoomHeight, p2.x(), p2.y());
                    clickPoint = QPointF(p2.x() - mRoomWidth, p2.y() - mRoomHeight);
                } else if (room->getNortheast() == rID) {
                    line = QLineF(p2.x() + exitArrowScale * mRoomWidth, p2.y() - exitArrowScale * mRoomHeight, p2.x(), p2.y());
                    clickPoint = QPointF(p2.x() + mRoomWidth, p2.y() - mRoomHeight);
                } else if (room->getSoutheast() == rID) {
                    line = QLineF(p2.x() + exitArrowScale * mRoomWidth, p2.y() + exitArrowScale * mRoomHeight, p2.x(), p2.y());
                    clickPoint = QPointF(p2.x() + mRoomWidth, p2.y() + mRoomHeight);
                } else if (room->getSouthwest() == rID) {
                    line = QLineF(p2.x() - exitArrowScale * mRoomWidth, p2.y() + exitArrowScale * mRoomHeight, p2.x(), p2.y());
                    clickPoint = QPointF(p2.x() - mRoomWidth, p2.y() + mRoomHeight);
                }
                areaExitsMap[rID] = clickPoint;
                // line ENDS at the center of the room, and the START sticks out
                // in the appropriate direction

                // Draw outline first for visibility on matching backgrounds
                QPen outlinePen = pen;
                outlinePen.setWidthF(exitWidth + 2.0);
                outlinePen.setColor((mpHost->mBgColor_2.lightness() > 127) ? Qt::black : Qt::white);
                painter.setPen(outlinePen);
                painter.drawLine(line);

                // Draw colored line on top
                painter.setPen(pen);
                painter.drawLine(line);
                QLineF l0 = QLineF(line);
                if (mLargeAreaExitArrows) {
                    l0.setLength((mRoomWidth + mRoomHeight) * 0.4);
                } else {
                    l0.setLength(exitWidth * 5.0);
                }
                const QPointF p1 = l0.p1();
                const QPointF p2 = l0.p2();
                const QLineF l1 = QLineF(l0);
                const qreal w1 = l1.angle() - 90.0;
                QLineF l2;
                l2.setP1(p2);
                l2.setAngle(w1);
                if (mLargeAreaExitArrows) {
                    l2.setLength((mRoomWidth + mRoomHeight) * 0.15);
                } else {
                    l2.setLength(exitWidth * 2.0);
                }
                const QPointF p3 = l2.p2();
                l2.setAngle(l2.angle() + 180.0);
                const QPointF p4 = l2.p2();
                QPolygonF polygon;
                polygon.append(p1);
                polygon.append(p3);
                polygon.append(p4);
                QBrush brush = painter.brush();
                brush.setColor(mpMap->getColor(rID));
                brush.setStyle(Qt::SolidPattern);
                QPen arrowPen = painter.pen();
                arrowPen.setJoinStyle(Qt::RoundJoin);
                arrowPen.setCapStyle(Qt::RoundCap);
                arrowPen.setCosmetic(mMapperUseAntiAlias);

                // Draw outline first for visibility
                QPen arrowOutlinePen = arrowPen;
                arrowOutlinePen.setWidthF(2.0);
                arrowOutlinePen.setColor((mpHost->mBgColor_2.lightness() > 127) ? Qt::black : Qt::white);
                painter.setPen(arrowOutlinePen);
                painter.setBrush(Qt::NoBrush);
                painter.drawPolygon(polygon);

                // Draw filled arrow head on top
                painter.setPen(Qt::NoPen);
                painter.setBrush(brush);
                painter.drawPolygon(polygon);
                painter.restore();
            }

            // doors
            if (!room->doors.empty()) {
                QString doorKey;
                int doorStatus = 0;
                if (room->getSouth() == rID && room->doors.contains(key_s)) {
                    doorKey = key_s;
                    doorStatus = room->doors.value(doorKey);
                } else if (room->getNorth() == rID && room->doors.contains(key_n)) {
                    doorKey = key_n;
                    doorStatus = room->doors.value(doorKey);
                } else if (room->getSouthwest() == rID && room->doors.contains(key_sw)) {
                    doorKey = key_sw;
                    doorStatus = room->doors.value(doorKey);
                } else if (room->getSoutheast() == rID && room->doors.contains(key_se)) {
                    doorKey = key_se;
                    doorStatus = room->doors.value(doorKey);
                } else if (room->getNortheast() == rID && room->doors.contains(key_ne)) {
                    doorKey = key_ne;
                    doorStatus = room->doors.value(doorKey);
                } else if (room->getNorthwest() == rID && room->doors.contains(key_nw)) {
                    doorKey = key_nw;
                    doorStatus = room->doors.value(doorKey);
                } else if (room->getWest() == rID && room->doors.contains(key_w)) {
                    doorKey = key_w;
                    doorStatus = room->doors.value(doorKey);
                } else if (room->getEast() == rID && room->doors.contains(key_e)) {
                    doorKey = key_e;
                    doorStatus = room->doors.value(doorKey);
                }
                // Else not an XY-plane exit and doorStatus/doorKey will not be
                // set to a usable value:
                if (doorStatus && !doorKey.isEmpty()) {
                    QLineF doorBaseLine;
                    if (areaExit) {
                        doorBaseLine = QLineF(line.p2(), line.p1());
                    } else {
                        doorBaseLine = QLineF(p2.toPointF(), p1.toPointF());
                    }
                    doorBaseLine.setLength(doorBaseLine.length() / 2.0);
                    drawDoor(painter, *room, doorKey, doorBaseLine);
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
            const QPen savePen = painter.pen();
            const QBrush saveBrush = painter.brush();
            const float roomRadius = mRoomWidth * 1.2;
            const float roomDiagonal = mRoomWidth * 1.2;
            const QPointF roomCenter = QPointF(rx, ry);

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

void TMapPainter::paintLabelHighlight(QPainter& painter, QRectF labelPaintRectangle, const QSizeF& clickSize)
{
    labelPaintRectangle.setSize(clickSize);
    painter.save();
    painter.setPen(QPen(QColor(255, 155, 55), 2));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(labelPaintRectangle);
    painter.restore();
}

void TMapPainter::paintView(QPainter& painter, const View& view, PaintResult& result, const std::function<void(const View&)>& viewSettled)
{
    setView(view);
    result = PaintResult{};
    paintFrame(painter, result, viewSettled);
    result.view = *this;
    result.pendingSpeedWalk = takePendingSpeedWalk();
    result.lodExitIndexRoomsHandedOver = mLodExitIndexRoomsHandedOver;
}

// Revised to use a QCache to hold QPixmap * to generated images for room symbols
void TMapPainter::paintFrame(QPainter& painter, PaintResult& result, const std::function<void(const View&)>& viewSettled)
{
    if (!mpMap || !mpHost) {
        return;
    }
    QElapsedTimer renderTimer;
    renderTimer.start();

    // Single phase timer restarted at each phase boundary.
    QElapsedTimer phaseTimer;
    phaseTimer.start();
    qreal phase1Time = 0, phase2Time = 0, phase3Time = 0, phase4Time = 0, phase5Time = 0, phase6Time = 0, phase7Time = 0;

    // This is needed so that clicking on an area exit can instigate a
    // speed-walk to the room in the linked to area...
    QMap<int, QPointF> areaExitsMap;

    const float widgetWidth = width();
    const float widgetHeight = height();

    if (widgetWidth < 10 || widgetHeight < 10) {
        return;
    }

    if (mCachedBubbleMode != mBubbleMode) {
        // If the round/square room selection has changed this will invalidate
        // all the previously generated pixmaps:
        flushSymbolPixmapCache();
        mCachedBubbleMode = mBubbleMode;
    }

    resetSymbolFontSize();
    mMapSymbolFont = mpMap->mMapSymbolFont;
    mMapSymbolFont.setBold(false);
    mMapSymbolFont.setItalic(false);
    mMapSymbolFont.setUnderline(false);
    mMapSymbolFont.setOverline(false);
    mMapSymbolFont.setStrikeOut(false);

    // the room name's font defaults to the symbol's
    // but may be overridden
    auto mapNameFont = mpMap->mMapSymbolFont;
    const QString fontName = mpMap->mUserData.value(ROOM_UI_NAMEFONT);
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

    QList<ExitToPaint> exitList;
    int playerRoomId = mpMap->mRoomIdHash.value(mpMap->mProfileName);
    TRoom* pPlayerRoom = mpMap->mpRoomDB->getRoom(playerRoomId);

    // try and set the player to a room if we don't have a known location
    if (!pPlayerRoom && !mpMap->mpRoomDB->isEmpty()) {
        const int randomRoom = mpMap->mpRoomDB->getRoomMap().constBegin().key();
        pPlayerRoom = mpMap->mpRoomDB->getRoom(randomRoom);
        playerRoomId = pPlayerRoom->getId();
    }
    // no rooms at all, let's show an information message instead
    if (!pPlayerRoom) {
        spendClick();
        if (mSuppressEmptyStateMessage) {
            return;
        }
        painter.save();
        painter.fillRect(0, 0, width(), height(), Qt::transparent);
        auto font(painter.font());
        font.setPointSize(10);
        painter.setFont(font);

        QString message;
        if (mpMap->mpRoomDB) {
            if (mpMap->mpRoomDB->isEmpty()) {
                message = QCoreApplication::translate("T2DMap", "No rooms in the map - load another one, or start mapping from scratch to begin.");
            } else {
                message = QCoreApplication::translate("T2DMap", "You have a map loaded (%n room(s)), but Mudlet does not know where you are at the moment.", nullptr, mpMap->mpRoomDB->size());
            }
        } else {
            message = QCoreApplication::translate("T2DMap", "You do not have a map yet - load one, or start mapping from scratch to begin.");
        }

        painter.drawText(0, 0, widgetWidth, widgetHeight, Qt::AlignCenter | Qt::TextWordWrap, message);
        painter.restore();
        return;
    }

    phase1Time = phaseTimer.nsecsElapsed() / 1000000.0;
    phaseTimer.restart();

    // This is only a safety check to avoid a probably impossible condition, we
    // have already established that pPlayerRoom is valid so for it to NOT be
    // in a valid area is unlikely:
    if (Q_UNLIKELY(!mpMap->mpRoomDB->getArea(pPlayerRoom->getArea()))) {
        spendClick();
        return;
    }

    if (mRoomID != playerRoomId && mShiftMode) {
        // Reset the flag if it is set but current player room (playerRoomId)
        // is not the same as was last used for that (mRoomID):
        mShiftMode = false;
    }

    // Secondary views don't follow the player - they maintain their own independent view
    // Track if we're auto-centering on the player (vs user manually panning)
    const bool centeringOnPlayer = !mIsSecondaryView && ((!mPick && !mShiftMode) || mpMap->mNewMove);

    if (centeringOnPlayer) {
        mShiftMode = true;
        // that's of interest only here because the map editor is here ->
        // map might not be updated, thus I force a map update on centerview()
        // with mNewRoom
        mpMap->mNewMove = false;

        mRoomID = playerRoomId;
        const int playerAreaID = pPlayerRoom->getArea();
        if (mAreaID != playerAreaID && pruneSelectionToArea(mpMap, mMultiSelectionSet, mMultiSelectionHighlightRoomId, playerAreaID)) {
            mSelectionListVisible = false;
            result.selectionPruned = true;
        }
        mAreaID = playerAreaID;
        if (mLastViewedAreaID != mAreaID) {
            result.areaChanged = {mAreaID, mLastViewedAreaID};
            mLastViewedAreaID = mAreaID;
        }

        mMapCenterX = pPlayerRoom->x();
        mMapCenterY = pPlayerRoom->y() * -1;
        mMapCenterZ = pPlayerRoom->z();
    }

    TArea* pDrawnArea = mpMap->mpRoomDB->getArea(mAreaID);
    if (!pDrawnArea) {
        spendClick();
        return;
    }

    phase2Time = phaseTimer.nsecsElapsed() / 1000000.0;
    phaseTimer.restart();

    if (!mKeepZoom) {
        xyzoom = pDrawnArea->get2DMapZoom();
    }
    if (widgetWidth > widgetHeight) {
        xspan = xyzoom * (widgetWidth / widgetHeight);
        yspan = xyzoom;
    } else {
        xspan = xyzoom;
        yspan = xyzoom * (widgetHeight / widgetWidth);
    }

    // Center map on area when it fits entirely in viewport, but only when
    // following player movement - not during manual panning or editing.
    // Off by default; opt in via the mapCenterSmallAreas key in Mudlet.ini.
    if (mpHost->mMapperCenterSmallAreas && centeringOnPlayer && !mRoomBeingMoved && !mMultiSelection) {
        const int zLevel = mMapCenterZ;

        // Get area bounds for current Z level (use overall bounds as fallback)
        const qreal areaMinX = pDrawnArea->xminForZ.value(zLevel, pDrawnArea->min_x);
        const qreal areaMaxX = pDrawnArea->xmaxForZ.value(zLevel, pDrawnArea->max_x);
        // Y is inverted in map coordinates
        const qreal areaMinY = -pDrawnArea->ymaxForZ.value(zLevel, pDrawnArea->max_y);
        const qreal areaMaxY = -pDrawnArea->yminForZ.value(zLevel, pDrawnArea->min_y);

        const qreal areaWidth = areaMaxX - areaMinX;
        const qreal areaHeight = areaMaxY - areaMinY;

        if (areaWidth <= xspan && areaHeight <= yspan) {
            mMapCenterX = (areaMinX + areaMaxX) / 2.0;
            mMapCenterY = -((areaMinY + areaMaxY) / 2.0);
        }
    }

    mRoomWidth = widgetWidth / xspan;
    mRoomHeight = widgetHeight / yspan;

    if (!qFuzzyCompare(1.0f + mPrevRoomWidth, 1.0f + mRoomWidth) || !qFuzzyCompare(1.0f + mPrevRoomHeight, 1.0f + mRoomHeight)) {
        flushSymbolPixmapCache();
        flushTextLabelPixmapCache();
        mPrevRoomWidth = mRoomWidth;
        mPrevRoomHeight = mRoomHeight;
    }

    // Not qRound(): it returns int, overflowing before the clamp.
    mRX = clampedToIntRange(std::round(mRoomWidth * ((xspan / 2.0) - mMapCenterX)));
    mRY = clampedToIntRange(std::round(mRoomHeight * ((yspan / 2.0) - mMapCenterY)));
    const QRect roomBounds = viewportRoomBounds(mRX, mRY, mRoomWidth, mRoomHeight, widgetWidth, widgetHeight);
    QFont roomVNumFont = mpMap->mMapSymbolFont;

    bool isFontBigEnoughToShowRoomVnum = false;
    if (mShowRoomID) {
        // Find out the number of digits the biggest room Id number in the area
        // needs; also useful for the room selection widget. Rescanning every
        // room here on every single paint made a large area's room-ID display
        // cost more than everything else in the frame put together, so this is
        // only redone when the area or its room membership has actually moved
        // on since the value was last cached.
        if (mCachedRoomIdDigitsVersion != pDrawnArea->getRoomsVersion()) {
            int maxUsedRoomId = 0;
            QSetIterator<int> itRoomId(pDrawnArea->getAreaRooms());
            while (itRoomId.hasNext()) {
                maxUsedRoomId = qMax(maxUsedRoomId, itRoomId.next());
            }
            mMaxRoomIdDigits = static_cast<quint8>(QString::number(maxUsedRoomId).length());
            mCachedRoomIdDigitsVersion = pDrawnArea->getRoomsVersion();
        }

        QRectF roomTestRect;
        if (pDrawnArea->gridMode) {
            roomTestRect = QRectF(0, 0, static_cast<qreal>(mRoomWidth), static_cast<qreal>(mRoomHeight));
        } else {
            roomTestRect = QRectF(0, 0, static_cast<qreal>(mRoomWidth) * rSize, static_cast<qreal>(mRoomHeight) * rSize);
        }
        static quint8 const roomVnumMargin = 10;
        roomVNumFont.setBold(true);

        // QFont::PreferOutline will help to select a font that will scale to any
        // size - which is important for good rendering over a range of sizes
        // QFont::PreferAntialias will look better - except perhaps at very small
        // sizes (but we prevent that by checking in the method call afterwards):
        roomVNumFont.setStyleStrategy(QFont::StyleStrategy(QFont::PreferNoShaping | QFont::PreferAntialias | QFont::PreferOutline));

        isFontBigEnoughToShowRoomVnum = sizeFontToFitTextInRect(roomVNumFont, roomTestRect, qsl("8").repeated(mMaxRoomIdDigits), roomVnumMargin);
    }

    bool showRoomNames = mpMap->getRoomNamesShown() && !pDrawnArea->gridMode;
    if (showRoomNames) {
        /*
         * Like above, except that we use the room height as the font size.
         */
        mapNameFont.setBold(true);

        mapNameFont.setStyleStrategy(QFont::StyleStrategy(QFont::PreferNoShaping | QFont::PreferAntialias | QFont::PreferOutline));

        const double sizeAdjust = 0;
        mapNameFont.setPointSizeF(static_cast<qreal>(mRoomWidth) * rSize * pow(1.1, sizeAdjust) / 2.0);
        showRoomNames = (mapNameFont.pointSizeF() > 3.0);
    }

    const int zLevel = mMapCenterZ;

    const float exitWidth = 1 / eSize * mRoomWidth * rSize;

    painter.fillRect(0, 0, width(), height(), mpHost->mBgColor_2);

    auto pen = painter.pen();
    pen.setColor(mpHost->mFgColor_2);
    pen.setWidthF(exitWidth);
    painter.setRenderHint(QPainter::Antialiasing, mMapperUseAntiAlias);
    painter.setPen(pen);

    if (mShowGrid && mRoomWidth > 0.0f && mRoomHeight > 0.0f) {
        painter.save();

        QColor gridColor = mpHost->mMapGridColor;

        QPen gridPen(gridColor);
        const qreal gridWidth = static_cast<qreal>(mpHost->mMapGridLineSize) * mRoomWidth / 50.0;
        gridPen.setWidthF(gridWidth);
        gridPen.setCosmetic(mMapperUseAntiAlias);
        painter.setPen(gridPen);

        const qreal visibleMinX = mMapCenterX - xspan / 2.0;
        const qreal visibleMaxX = mMapCenterX + xspan / 2.0;
        const qreal visibleMinY = -((yspan / 2.0) + mMapCenterY);
        const qreal visibleMaxY = (yspan / 2.0) - mMapCenterY;

        const int startX = static_cast<int>(std::floor(visibleMinX));
        const int endX = static_cast<int>(std::ceil(visibleMaxX));
        for (int gridX = startX; gridX <= endX; ++gridX) {
            const qreal widgetX = static_cast<qreal>(gridX) * static_cast<qreal>(mRoomWidth) + static_cast<qreal>(mRX);
            painter.drawLine(QPointF(widgetX, 0.0), QPointF(widgetX, static_cast<qreal>(widgetHeight)));
        }

        const int startY = static_cast<int>(std::floor(visibleMinY));
        const int endY = static_cast<int>(std::ceil(visibleMaxY));
        for (int gridY = startY; gridY <= endY; ++gridY) {
            const qreal widgetY = static_cast<qreal>(gridY) * -static_cast<qreal>(mRoomHeight) + static_cast<qreal>(mRY);
            painter.drawLine(QPointF(0.0, widgetY), QPointF(static_cast<qreal>(widgetWidth), widgetY));
        }

        painter.restore();
    }

    phase3Time = phaseTimer.nsecsElapsed() / 1000000.0;
    phaseTimer.restart();

    // Draw label sizing or group selection box
    if (mSizeLabel) {
        painter.fillRect(mMultiRect, QColor(250, 190, 0, 190));
    } else {
        painter.fillRect(mMultiRect, QColor(190, 190, 190, 60));
    }

    QPointF playerRoomOnWidgetCoordinates;
    bool isPlayerRoomVisible = false;
    // QPoint doesn't work here as the key as it can't be hashed...!
    QSet<QPair<int, int>> usedRoomPositions;

    if (mDrawUpperLowerLevels) {
        const TAreaGridIndex& gridIndex = pDrawnArea->getGridIndex();
        // Zoomed out over the whole area, walking the index costs more than the flat room set.
        // The area keeps its y extremes negated, unlike roomBounds.
        const bool viewportHoldsArea =
                roomBounds.left() <= pDrawnArea->min_x && roomBounds.right() >= pDrawnArea->max_x && roomBounds.top() <= -pDrawnArea->max_y && roomBounds.bottom() >= -pDrawnArea->min_y;
        auto neighbouringLevelRooms = [&](const int z) {
            if (viewportHoldsArea) {
                const QSet<int>& rooms = pDrawnArea->getRoomsForZ(z);
                return QList<int>(rooms.cbegin(), rooms.cend());
            }
            return gridIndex.roomsInViewport(z, roomBounds.left(), roomBounds.right(), roomBounds.top(), roomBounds.bottom());
        };
        const QList<int> lowerLevelRooms = neighbouringLevelRooms(zLevel - 1);
        for (const int currentAreaRoom : lowerLevelRooms) {
            TRoom* room = mpMap->mpRoomDB->getRoom(currentAreaRoom);
            if (!room) {
                continue;
            }

            const float rx = room->x() * mRoomWidth + static_cast<float>(mRX);
            const float ry = room->y() * -1 * mRoomHeight + static_cast<float>(mRY);
            if (rx >= 0 && ry >= 0 && rx <= widgetWidth && ry <= widgetHeight) {
                painter.save();
                painter.setPen(Qt::NoPen);
                painter.setBrush(mpHost->mLowerLevelColor);
                if (mBubbleMode) {
                    const float roomRadius = 0.5 * rSize * mRoomWidth;
                    const QPointF roomCenter = QPointF(rx - (roomRadius * rSize * 0.5), ry + (roomRadius * rSize * 0.5));
                    QPainterPath diameterPath;
                    diameterPath.addEllipse(roomCenter, roomRadius, roomRadius);
                    painter.drawPath(diameterPath);
                } else {
                    painter.drawRect(rx - (mRoomWidth * rSize * 0.8), ry - (mRoomHeight * rSize * 0.2), mRoomWidth * rSize, mRoomHeight * rSize);
                }
                painter.restore();
            }
        }

        const QList<int> upperLevelRooms = neighbouringLevelRooms(zLevel + 1);
        for (const int currentAreaRoom : upperLevelRooms) {
            TRoom* room = mpMap->mpRoomDB->getRoom(currentAreaRoom);
            if (!room) {
                continue;
            }

            const float rx = room->x() * mRoomWidth + static_cast<float>(mRX);
            const float ry = room->y() * -1 * mRoomHeight + static_cast<float>(mRY);
            if (rx >= 0 && ry >= 0 && rx <= widgetWidth && ry <= widgetHeight) {
                painter.save();
                painter.setPen(QPen(mpHost->mUpperLevelColor, 1));
                painter.setBrush(Qt::transparent);
                if (mBubbleMode) {
                    const float roomRadius = 0.5 * rSize * mRoomWidth;
                    const QPointF roomCenter = QPointF(rx + (roomRadius * rSize * 0.5), ry - (roomRadius * rSize * 0.5));
                    QPainterPath diameterPath;
                    diameterPath.addEllipse(roomCenter, roomRadius, roomRadius);
                    painter.drawPath(diameterPath);
                } else {
                    painter.drawRect(rx - (mRoomWidth * rSize * 0.2), ry - (mRoomHeight * rSize * 0.8), mRoomWidth * rSize, mRoomHeight * rSize);
                }
                painter.restore();
            }
        }
    }

    // Draw the ("background") labels that are on the bottom of the map:
    startTextLabelFrame();
    QMutableMapIterator<int, TMapLabel> itMapLabel(pDrawnArea->mMapLabels);
    while (itMapLabel.hasNext()) {
        itMapLabel.next();
        TMapLabel& mapLabel = itMapLabel.value();
        if (mapLabel.pos.z() != mMapCenterZ) {
            continue;
        }
        QPointF labelPosition;
        const int labelX = mapLabel.pos.x() * mRoomWidth + mRX;
        const int labelY = mapLabel.pos.y() * mRoomHeight * -1 + mRY;

        labelPosition.setX(labelX);
        labelPosition.setY(labelY);
        const int labelWidth = abs(qRound(mapLabel.size.width() * mRoomWidth));
        const int labelHeight = abs(qRound(mapLabel.size.height() * mRoomHeight));
        if (!((0 < labelX || 0 < labelX + labelWidth) && (widgetWidth > labelX || widgetWidth > labelX + labelWidth))) {
            continue;
        }
        if (!((0 < labelY || 0 < labelY + labelHeight) && (widgetHeight > labelY || widgetHeight > labelY + labelHeight))) {
            continue;
        }

        QRectF labelPaintRectangle = QRect(mapLabel.pos.x() * mRoomWidth + mRX, mapLabel.pos.y() * mRoomHeight * -1 + mRY, labelWidth, labelHeight);
        if (!mapLabel.showOnTop) {
            if (!mapLabel.noScaling) {
                drawScaledLabel(painter, labelPosition, mapLabel, itMapLabel.key(), labelPaintRectangle);
            } else {
                painter.drawPixmap(labelPosition, mapLabel.pix);
                mapLabel.clickSize = QSizeF(mapLabel.pix.width(), mapLabel.pix.height());
            }
        }

        if (mapLabel.highlight) {
            paintLabelHighlight(painter, labelPaintRectangle, mapLabel.clickSize);
        }
    }

    phase4Time = phaseTimer.nsecsElapsed() / 1000000.0;
    phaseTimer.restart();

    // draw room exits
    qreal timeViewportQuery = 0.0;
    QList<int> viewportRooms;
    if (!pDrawnArea->gridMode) {
        // Shared by the exit and room passes, as it's the costliest part of each. Not for grid mode: it
        // paints no exits and needs the collision flag only the other query returns.
        QElapsedTimer viewportQueryTimer;
        viewportQueryTimer.start();
        viewportRooms = pDrawnArea->getGridIndex().roomsInViewport(zLevel, roomBounds.left(), roomBounds.right(), roomBounds.top(), roomBounds.bottom());
        timeViewportQuery = viewportQueryTimer.nsecsElapsed() / 1000000.0;
        paintRoomExits(painter, pen, exitList, pDrawnArea, zLevel, roomBounds, viewportRooms, exitWidth, areaExitsMap);
    }

    phase5Time = phaseTimer.nsecsElapsed() / 1000000.0;
    phaseTimer.restart();

    // Draw rooms on selected z-level - use batch rendering for grid mode
    // Room collection time is printed at the end, keeping qDebug() overhead (large on Windows) out of phase6Time.
    QString roomLoopProfileOutput;
    QElapsedTimer phase6Sub;
    phase6Sub.start();
    // Only the full-detail loop layers its rooms, and only with a highlight in view
    bool layerRooms = false;
    // Gathered before drawing so each room is looked up once
    QList<QPair<TRoom*, QPointF>> drawnRooms;
    bool isPlayerRoomBodyDrawn = false;
    painter.save(); // prevent room-drawing state from leaking into labels/info box
    if (pDrawnArea->gridMode) {
        // Grid mode: use spatial grid index for O(visible cells) rendering
        drawGridModeRooms(painter,
                          pDrawnArea,
                          zLevel,
                          playerRoomId,
                          mRX,
                          mRY,
                          mRoomWidth,
                          mRoomHeight,
                          widgetWidth,
                          widgetHeight,
                          roomVNumFont,
                          isPlayerRoomVisible,
                          playerRoomOnWidgetCoordinates,
                          isPlayerRoomBodyDrawn,
                          isFontBigEnoughToShowRoomVnum,
                          &roomLoopProfileOutput);
    } else if (mRoomWidth < cLodPixelThreshold) {
        drawNonGridModeRoomsLod(
                painter, pDrawnArea, zLevel, playerRoomId, viewportRooms, widgetWidth, widgetHeight, isPlayerRoomVisible, playerRoomOnWidgetCoordinates, isPlayerRoomBodyDrawn, &roomLoopProfileOutput);
    } else {
        // Non-grid mode: use full-featured per-room rendering
        drawnRooms.reserve(viewportRooms.size());
        for (const int currentAreaRoom : viewportRooms) {
            TRoom* room = mpMap->mpRoomDB->getRoom(currentAreaRoom);
            if (!room) {
                continue;
            }

            const float rx = room->x() * mRoomWidth + static_cast<float>(mRX);
            const float ry = room->y() * -1 * mRoomHeight + static_cast<float>(mRY);
            if (rx < 0 || ry < 0 || rx > widgetWidth || ry > widgetHeight) {
                continue;
            }

            layerRooms = layerRooms || (room->highlight && !room->isHidden());
            if (playerRoomId == currentAreaRoom) {
                // We defer drawing THIS (the player's room) until the end
                isPlayerRoomVisible = true;
                playerRoomOnWidgetCoordinates = QPointF(static_cast<qreal>(rx), static_cast<qreal>(ry));
            } else {
                drawnRooms.append({room, QPointF(rx, ry)});
            }
        }
        for (const auto& [room, roomCenter] : std::as_const(drawnRooms)) {
            const QPair<int, int> roomPos{room->x(), room->y()};
            const bool roomCollision = usedRoomPositions.contains(roomPos);
            usedRoomPositions.insert(roomPos);
            drawRoom(painter,
                     roomVNumFont,
                     mapNameFont,
                     pen,
                     room,
                     pDrawnArea->gridMode,
                     isFontBigEnoughToShowRoomVnum,
                     showRoomNames,
                     playerRoomId,
                     static_cast<float>(roomCenter.x()),
                     static_cast<float>(roomCenter.y()),
                     roomCollision,
                     layerRooms ? RoomDrawPass::Body : RoomDrawPass::Whole);
        }
    }
    painter.restore(); // end of room-drawing save

    qreal timePhase6RoomsDraw = phase6Sub.nsecsElapsed() / 1000000.0;
    phase6Sub.restart();

    qreal timePhase6DrawRoom = 0.0;
    qreal timePhase6Gradient = 0.0;
    painter.save(); // prevent player-room drawing state from leaking into labels/info box
    if (isPlayerRoomVisible) {
        // Only the full-detail non-grid loop fills usedRoomPositions. The index also counts off-screen and hidden rooms.
        const bool roomCollision = (pDrawnArea->gridMode || mRoomWidth < cLodPixelThreshold) ? pDrawnArea->getGridIndex().roomsAt(zLevel, pPlayerRoom->x(), pPlayerRoom->y()).size() > 1
                                                                                             : usedRoomPositions.contains({pPlayerRoom->x(), pPlayerRoom->y()});
        const QList<RoomDrawPass> playerRoomPasses =
                isPlayerRoomBodyDrawn ? QList<RoomDrawPass>{RoomDrawPass::Highlight, RoomDrawPass::Labels} : QList<RoomDrawPass>{layerRooms ? RoomDrawPass::Body : RoomDrawPass::Whole};
        for (const RoomDrawPass pass : playerRoomPasses) {
            drawRoom(painter,
                     roomVNumFont,
                     mapNameFont,
                     pen,
                     pPlayerRoom,
                     pDrawnArea->gridMode,
                     isFontBigEnoughToShowRoomVnum,
                     showRoomNames,
                     playerRoomId,
                     static_cast<float>(playerRoomOnWidgetCoordinates.x()),
                     static_cast<float>(playerRoomOnWidgetCoordinates.y()),
                     roomCollision,
                     pass);
        }
        if (layerRooms) {
            drawnRooms.append({pPlayerRoom, playerRoomOnWidgetCoordinates});
        }
        timePhase6DrawRoom = phase6Sub.nsecsElapsed() / 1000000.0;
        phase6Sub.restart();
    }
    if (layerRooms) {
        drawRoomHighlightsAndLabels(painter, roomVNumFont, mapNameFont, pen, drawnRooms, pDrawnArea->gridMode, isFontBigEnoughToShowRoomVnum, showRoomNames, playerRoomId);
        timePhase6RoomsDraw += phase6Sub.nsecsElapsed() / 1000000.0;
        phase6Sub.restart();
    }
    if (isPlayerRoomVisible) {
        painter.save();
        const QPen transparentPen(Qt::transparent);
        QPainterPath myPath;
        // Use the room+border diagonal so 100% fully encompasses the square
        // (including corners), not just touching the midpoints of the sides
        const int borderWidth = qMax(1, static_cast<int>(1.0 / mBSize * mRoomWidth * rSize));
        const double roomVisualSize = static_cast<double>(mRoomWidth) * rSize + 2.0 * borderWidth;
        const double roomRadius = RoomAppearance::playerRoomRadius(mpMap->mPlayerRoomOuterDiameterPercentage, roomVisualSize);
        QRadialGradient gradient(playerRoomOnWidgetCoordinates, roomRadius);
        if (mpHost->mMapStrongHighlight) {
            // Never set, no means to except via XMLImport, as dlgMapper class's
            // slot_toggleStrongHighlight is not wired up to anything
            const QRectF dr = QRectF(playerRoomOnWidgetCoordinates.x() - (static_cast<double>(mRoomWidth) * rSize) / 2.0,
                                     playerRoomOnWidgetCoordinates.y() - (static_cast<double>(mRoomHeight) * rSize) / 2.0,
                                     static_cast<double>(mRoomWidth) * rSize,
                                     static_cast<double>(mRoomHeight) * rSize);
            painter.fillRect(dr, RoomAppearance::strongHighlightFillColor());

            gradient.setColorAt(0.95, QColor(255, 0, 0, 150));
            gradient.setColorAt(0.80, QColor(150, 100, 100, 150));
            gradient.setColorAt(0.799, QColor(150, 100, 100, 100));
            gradient.setColorAt(0.7, QColor(255, 0, 0, 200));
            gradient.setColorAt(0, Qt::white);
            painter.setBrush(gradient);
            painter.setPen(transparentPen);
            myPath.addEllipse(playerRoomOnWidgetCoordinates, roomRadius, roomRadius);
        } else {
            gradient.setStops(mPlayerRoomColorGradientStops);
            painter.setBrush(gradient);
            painter.setPen(transparentPen);
            myPath.addEllipse(playerRoomOnWidgetCoordinates, roomRadius, roomRadius);
        }
        painter.drawPath(myPath);
        painter.restore();
        timePhase6Gradient = phase6Sub.nsecsElapsed() / 1000000.0;
    }
    painter.restore(); // end of player-room save

    resolveAreaExitClick(painter, areaExitsMap, playerRoomId);
    spendClick();

    phase6Time = phaseTimer.nsecsElapsed() / 1000000.0;
    phaseTimer.restart();

    // Draw the ("foreground") labels that are on the top of the map:
    itMapLabel.toFront();
    while (itMapLabel.hasNext()) {
        itMapLabel.next();
        TMapLabel& mapLabel = itMapLabel.value();

        if (mapLabel.pos.z() != mMapCenterZ) {
            continue;
        }
        QPointF labelPosition;
        const int labelX = mapLabel.pos.x() * mRoomWidth + mRX;
        const int labelY = mapLabel.pos.y() * mRoomHeight * -1 + mRY;

        labelPosition.setX(labelX);
        labelPosition.setY(labelY);
        const int labelWidth = abs(qRound(mapLabel.size.width() * mRoomWidth));
        const int labelHeight = abs(qRound(mapLabel.size.height() * mRoomHeight));

        if (!((0 < labelX || 0 < labelX + labelWidth) && (widgetWidth > labelX || widgetWidth > labelX + labelWidth))) {
            continue;
        }
        if (!((0 < labelY || 0 < labelY + labelHeight) && (widgetHeight > labelY || widgetHeight > labelY + labelHeight))) {
            continue;
        }
        QRectF labelPaintRectangle = QRect(mapLabel.pos.x() * mRoomWidth + mRX, mapLabel.pos.y() * mRoomHeight * -1 + mRY, labelWidth, labelHeight);
        if (mapLabel.showOnTop) {
            if (!mapLabel.noScaling) {
                drawScaledLabel(painter, labelPosition, mapLabel, itMapLabel.key(), labelPaintRectangle);
            } else {
                painter.drawPixmap(labelPosition, mapLabel.pix);
                mapLabel.clickSize = QSizeF(mapLabel.pix.width(), mapLabel.pix.height());
            }
        }
        if (mapLabel.highlight) {
            paintLabelHighlight(painter, labelPaintRectangle, mapLabel.clickSize);
        }
    }

    // Draw an indication of the central room of a multi-room selection.
    // Similar code was used to indicate target of custom exit line selected for
    // editing but this could not be done there because gridmode areas don't hit
    // that bit of code and later rooms would overwrite the target...
    if (mMultiSelectionHighlightRoomId > 0 && mMultiSelectionSet.size() > 1) {
        TRoom* pR_multiSelectionHighlight = mpMap->mpRoomDB->getRoom(mMultiSelectionHighlightRoomId);
        if (pR_multiSelectionHighlight) {
            const float r_mSx = pR_multiSelectionHighlight->x() * mRoomWidth + mRX;
            const float r_mSy = pR_multiSelectionHighlight->y() * -1 * mRoomHeight + mRY;
            const QPen savePen = painter.pen();
            const QBrush saveBrush = painter.brush();
            const float roomRadius = mRoomWidth * 1.2;
            const float roomDiagonal = mRoomWidth * 1.2;
            const QPointF roomCenter = QPointF(r_mSx, r_mSy);

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

    if (mPaintBelowInfoBox) {
        mPaintBelowInfoBox(painter);
    }

    QColor infoColor;
    if (mpHost->mBgColor_2.lightness() > 127) {
        infoColor = QColor(Qt::black);
    } else {
        infoColor = QColor(Qt::white);
    }

    // The info box always shows the player room unless rooms are explicitly
    // selected.  playerRoomId is fresh each frame; mRoomID is only updated when
    // centeringOnPlayer fires and can be stale or zero before first centering.
    int roomID = playerRoomId;
    if (!mMultiSelectionSet.empty() && mMultiSelectionHighlightRoomId > 0 && mpMap->mpRoomDB->getRoom(mMultiSelectionHighlightRoomId)) {
        roomID = mMultiSelectionHighlightRoomId;
    }

    int xOffset = 10;
    if (mSelectionListVisible) {
        xOffset += mSelectionListX + mSelectionListWidth;
    }

    phase7Time = phaseTimer.nsecsElapsed() / 1000000.0;

    if (qEnvironmentVariableIsSet("MUDLET_PROFILE_MAP")) {
        // Accumulate all profiling text into ONE string then emit a single
        // qDebug() call.  On Windows each qDebug() call invokes OutputDebugString
        // which can take 5-30 ms when a debugger is attached; batching to one
        // call reduces that to a fixed per-frame cost of one call.
        QString profileOut;
        QDebug pdbg(&profileOut);
        pdbg.nospace().noquote() << "viewportQuery: " << timeViewportQuery << "ms (phase5, shared with phase6)\n";
        if (!roomLoopProfileOutput.isEmpty()) {
            pdbg << roomLoopProfileOutput << "\n";
        }
        pdbg << "phase6 subs: roomsDraw=" << timePhase6RoomsDraw << " playerRoom=" << timePhase6DrawRoom << " gradient=" << timePhase6Gradient << " playerVisible=" << isPlayerRoomVisible << "\n";
        // Comprehensive infoBox diagnostic: shows every condition paintMapInfo
        // checks so we can see exactly why it produces blank output.
        auto* mgr = mpMap->mMapInfoContributorManager;
        const bool roomIDValid = mpMap->mpRoomDB->getRoom(roomID) != nullptr;
        const bool contribOk = mgr != nullptr && mpHost->mMapInfoContributors.intersects(QSet<QString>(mgr->getContributorKeys().constBegin(), mgr->getContributorKeys().constEnd()));
        pdbg << "infoBox: roomID=" << roomID << " player=" << playerRoomId << " mRoomID=" << mRoomID << " areaID=" << mAreaID << " selectionSize=" << static_cast<int>(mMultiSelectionSet.size())
             << " xOffset=" << xOffset << " listWidgetVisible=" << mSelectionListVisible << " listWidgetGeom=" << mSelectionListX << "+" << mSelectionListWidth << " widgetWidth=" << width()
             << " shift=" << mShiftMode << " centerView=" << isCenterViewCall << " playerVisible=" << isPlayerRoomVisible << " roomValid=" << roomIDValid << " contribOk=" << contribOk
             << " painterHasClip=" << painter.hasClipping() << " painterClipRect=" << painter.clipRegion().boundingRect() << " fontHeight=" << mFontHeight << "\n"
             << "paintEvent: total=" << renderTimer.elapsed() << " p1=" << phase1Time << " p2=" << phase2Time << " p3=" << phase3Time << " p4=" << phase4Time << " p5=" << phase5Time
             << " p6=" << phase6Time << " p7=" << phase7Time;
        qDebug().noquote() << profileOut;
    }

    if (viewSettled) {
        viewSettled(*this);
    }
    paintMapInfo(renderTimer, painter, mpHost, mpMap, roomID, mAreaID, mMultiSelectionSet.size(), infoColor, xOffset, 20, width(), mFontHeight);

    if (!mHelpMsg.isEmpty()) {
        painter.setPen(QColor(255, 155, 50));
        QFont _f = painter.font();
        const QFont _f2 = _f;
        _f.setPointSize(12); // 20 was a little large
        _f.setBold(true);
        painter.setFont(_f);
        const QRect _r = QRect(0, 0, widgetWidth, widgetHeight);
        painter.drawText(_r, Qt::AlignHCenter | Qt::AlignBottom | Qt::TextWordWrap, mHelpMsg);
        // Now draw text centered at bottom, so it does not clash with info window
        painter.setFont(_f2);
    }
    if (!mKeepZoom) {
        // Save the current map zoom for this area
        pDrawnArea->set2DMapZoom(xyzoom);
    }
}


// A click no room or exit took is spent all the same, or a later frame would act on whatever moved under it
void TMapPainter::spendClick()
{
    mPick = false;
    mStartSpeedWalk = false;
}

// Markers are gathered from every drawn room, so hit-test once after the room passes rather than
// from a room loop: below four pixels a room, only the player's room is drawn in full.
void TMapPainter::resolveAreaExitClick(QPainter& painter, const QMap<int, QPointF>& areaExitsMap, const int speedWalkStartRoomId)
{
    if (!mPick || !mStartSpeedWalk) {
        return;
    }
    QMapIterator<int, QPointF> it(areaExitsMap);
    while (it.hasNext()) {
        it.next();
        const QPointF roomCenter = it.value();
        const QRectF dr = QRectF(roomCenter.x(), roomCenter.y(), mRoomWidth * rSize, mRoomHeight * rSize);

        // clang-format off
        if (mPHighlight.x() < (dr.x() - mRoomWidth / 3.0)
            || mPHighlight.x() > (dr.x() + mRoomWidth / 3.0)
            || mPHighlight.y() < (dr.y() - mRoomHeight / 3.0)
            || mPHighlight.y() > (dr.y() + mRoomHeight / 3.0)) {
            // clang-format on
            continue;
        }

        mStartSpeedWalk = false;
        // This draws a red circle around the out of area exit that
        // was chosen as the target for the speedwalk, but it is
        // only shown for one paintEvent call and it is not obvious
        // that it is useful, note that there is similar code for a
        // room being clicked on that is WITHIN the area, in drawRoom():
        painter.save();
        const float roomRadius = (0.8 * mRoomWidth) / 2.0;
        QRadialGradient gradient(roomCenter, roomRadius);
        gradient.setColorAt(0.95, QColor(255, 0, 0, 150));
        gradient.setColorAt(0.80, QColor(150, 100, 100, 150));
        gradient.setColorAt(0.799, QColor(150, 100, 100, 100));
        gradient.setColorAt(0.7, QColor(255, 0, 0, 200));
        gradient.setColorAt(0, Qt::white);
        const QPen transparentPen(Qt::transparent);
        QPainterPath myPath;
        painter.setBrush(gradient);
        painter.setPen(transparentPen);
        myPath.addEllipse(roomCenter, roomRadius, roomRadius);
        painter.drawPath(myPath);
        painter.restore();

        mPick = false;
        mPendingSpeedWalk = {speedWalkStartRoomId, it.key()};
        return;
    }
}

int TMapPainter::paintMapInfo(const QElapsedTimer& renderTimer,
                              QPainter& painter,
                              Host* pHost,
                              TMap* pMap,
                              int roomID,
                              int displayAreaId,
                              int selectionSize,
                              QColor& infoColor,
                              int xOffset,
                              int yOffset,
                              int widgetWidth,
                              int fontHeight,
                              const QColor& background,
                              const QString& fontFamily)
{
    if (!pMap || !pMap->mMapInfoContributorManager || !pHost) {
        return yOffset;
    }

    QList<QString> contributorList = pMap->mMapInfoContributorManager->getContributorKeys();
    QSet<QString> const contributorKeys{contributorList.begin(), contributorList.end()};
    if (!contributorKeys.intersects(pHost->mMapInfoContributors)) {
        return yOffset;
    }

    TRoom* pRoom = pMap->mpRoomDB->getRoom(roomID);
    if (!pRoom) {
        return yOffset;
    }

    const int initialYOffset = yOffset;
    const QColor infoBackground = background.isValid() ? background : pHost->mMapInfoBg;

    painter.save();
    QFont font = pHost->getDisplayFont();
    if (!fontFamily.isEmpty()) {
        font.setFamily(fontFamily);
        fontHeight = QFontMetrics(font).height();
    }
    painter.setFont(font);

    // The caller holds raw TRoom/TArea pointers across this, and a Lua contributor can
    // register or kill contributors, so walk a copy and skip any that went away.
    const TMap::ScriptCallbackScope callbackScope(pMap);
    const QList<QString> keys = pMap->mMapInfoContributorManager->getContributorKeys();
    for (const auto& key : keys) {
        if (pHost->mMapInfoContributors.contains(key)) {
            const MapInfoCallback contributor = pMap->mMapInfoContributorManager->getContributor(key);
            if (!contributor) {
                continue;
            }
            auto properties = contributor(roomID, selectionSize, pRoom->getArea(), displayAreaId, infoColor);
            if (!properties.color.isValid()) {
                properties.color = infoColor;
            }
            yOffset += paintMapInfoContributor(painter, xOffset, yOffset, properties, infoBackground, fontHeight, widgetWidth);
        }
    }

#ifdef QT_DEBUG
    yOffset += paintMapInfoContributor(
            painter, xOffset, yOffset, {false, false, QObject::tr("render time: %1S").arg(renderTimer.nsecsElapsed() * 1.0e-9, 0, 'f', 3), infoColor}, infoBackground, fontHeight, widgetWidth);
#else
    Q_UNUSED(renderTimer)
#endif

    painter.restore();

    if (yOffset > initialYOffset) {
        painter.fillRect(xOffset, initialYOffset - 10, widgetWidth - 10 - xOffset, 10, infoBackground);
    }
    return yOffset;
}

int TMapPainter::paintMapInfoContributor(QPainter& painter, int xOffset, int yOffset, const MapInfoProperties& properties, QColor bgColor, int fontHeight, int widgetWidth)
{
    if (properties.text.isEmpty()) {
        return 0;
    }

    painter.save();
    auto infoText = properties.text.trimmed();
    auto font = painter.font();
    font.setBold(properties.isBold);
    font.setItalic(properties.isItalic);
    painter.setFont(font);
    // Set the pen before calling boundingRect: Qt6's drawTextItem early-returns
    // when pen().style() == Qt::NoPen, which causes boundingRect to return
    // height=0 and makes drawText render nothing (grey box with no text).
    painter.setPen(properties.color);
    const int infoHeight = fontHeight;
    QRect testRect;
    QRect mapInfoRect = QRect(xOffset, yOffset, widgetWidth - 10 - xOffset, infoHeight);
    testRect = painter.boundingRect(mapInfoRect.left() + 10,
                                    mapInfoRect.top(),
                                    mapInfoRect.width() - 20,
                                    mapInfoRect.height() - 20,
                                    Qt::Alignment(Qt::AlignTop | Qt::AlignLeft) | Qt::TextFlag(Qt::TextWordWrap | Qt::TextIncludeTrailingSpaces),
                                    infoText);
    mapInfoRect.setHeight(testRect.height() + 10);
    painter.fillRect(mapInfoRect, bgColor);
    painter.drawText(mapInfoRect.left() + 10,
                     mapInfoRect.top(),
                     mapInfoRect.width() - 20,
                     mapInfoRect.height() - 10,
                     Qt::Alignment(Qt::AlignTop | Qt::AlignLeft) | Qt::TextFlag(Qt::TextWordWrap | Qt::TextIncludeTrailingSpaces),
                     infoText);
    painter.restore();
    return mapInfoRect.height();
}

// Drops the selected rooms that are not in the area that is about to be shown.
// Without this a selection would survive an area change but be off-screen, so
// anything done to it - deleting those rooms, say - would happen unseen.
bool TMapPainter::pruneSelectionToArea(TMap* pMap, QSet<int>& selection, int& highlightRoomId, const int areaId)
{
    if (selection.isEmpty() || !pMap || !pMap->mpRoomDB) {
        return false;
    }

    QSet<int> roomsInNewArea;
    for (const int roomId : std::as_const(selection)) {
        TRoom* pRoom = pMap->mpRoomDB->getRoom(roomId);
        if (pRoom && pRoom->getArea() == areaId) {
            roomsInNewArea.insert(roomId);
        }
    }

    if (roomsInNewArea.size() == selection.size()) {
        // Everything selected has come along to the new area - as happens when
        // the rooms themselves were just moved into it:
        return false;
    }

    selection = roomsInNewArea;
    if (!selection.contains(highlightRoomId)) {
        switch (selection.size()) {
        case 0:
            highlightRoomId = 0;
            break;
        case 1:
            highlightRoomId = *(selection.constBegin());
            break;
        default:
            highlightRoomId = selectionCentreRoom(pMap, selection);
        }
    }
    return true;
}

// Replacement for getTopLeftCenter - determines a room closest to geometrical
// mean of all the room selected
int TMapPainter::selectionCentreRoom(TMap* pMap, const QSet<int>& selection)
{
    int centreRoomId = 0;
    if (selection.isEmpty()) {
        return centreRoomId;
    }

    QSetIterator<int> itRoom = selection;
    float mean_x = 0.0;
    float mean_y = 0.0;
    float mean_z = 0.0;
    uint processedRoomCount = 0;
    while (itRoom.hasNext()) {
        const int currentRoomId = itRoom.next();
        TRoom* room = pMap->mpRoomDB->getRoom(currentRoomId);
        if (!room) {
            continue;
        }

        mean_x += (static_cast<float>(room->x() - mean_x)) / ++processedRoomCount;
        mean_y += (static_cast<float>(room->y() - mean_y)) / processedRoomCount;
        mean_z += (static_cast<float>(room->z() - mean_z)) / processedRoomCount;
    }

    if (processedRoomCount) {
        itRoom.toFront();
        float closestSquareDistance = -1.0;
        while (itRoom.hasNext()) {
            const int currentRoomId = itRoom.next();
            TRoom* room = pMap->mpRoomDB->getRoom(currentRoomId);
            if (!room) {
                continue;
            }

            const QVector3D meanToRoom(static_cast<float>(room->x()) - mean_x, static_cast<float>(room->y()) - mean_y, static_cast<float>(room->z()) - mean_z);
            if (closestSquareDistance < -0.5) {
                // Don't use an equality to zero test, we are using floats so
                // need to allow for a little bit of fuzzzyness!
                closestSquareDistance = meanToRoom.lengthSquared();
                centreRoomId = currentRoomId;
            } else {
                const float currentRoomSquareDistance = meanToRoom.lengthSquared();
                if (closestSquareDistance > currentRoomSquareDistance) {
                    closestSquareDistance = currentRoomSquareDistance;
                    centreRoomId = currentRoomId;
                }
            }
        }
    }
    return centreRoomId;
}

QList<int> TMapPainter::roomsAtPoint(const View& view, const TArea& area, const QPoint& position)
{
    const float fx = ((view.xspan / 2.0f) - view.mMapCenterX) * view.mRoomWidth;
    const float fy = ((view.yspan / 2.0f) - view.mMapCenterY) * view.mRoomHeight;

    const int mx = position.x();
    const int my = position.y();
    const int mz = view.mMapCenterZ;

    // Build the list of rooms under the cursor.  Grid mode uses the spatial
    // index for an exact cell lookup (works at all zoom levels including
    // extreme zoom-out where rooms are sub-pixel sized).  Non-grid mode uses
    // a pixel-distance test with a minimum 1-pixel radius.
    QList<int> hitRooms;
    if (area.gridMode) {
        const int worldX = qRound(static_cast<double>(mx - fx) / view.mRoomWidth);
        const int worldY = qRound(static_cast<double>(fy - my) / view.mRoomHeight);
        for (const int roomId : area.getGridIndex().roomsAt(mz, worldX, worldY)) {
            hitRooms.append(roomId);
        }
    } else {
        const int halfW = qMax(1, qRound(view.mRoomWidth * view.rSize / 2.0));
        const int halfH = qMax(1, qRound(view.mRoomHeight * view.rSize / 2.0));
        QSetIterator<int> roomIterator(area.getRoomsForZ(mz));
        while (roomIterator.hasNext()) {
            const int roomId = roomIterator.next();
            TRoom* room = view.mpMap->mpRoomDB->getRoom(roomId);
            if (!room) {
                continue;
            }
            const int rx = room->x() * view.mRoomWidth + fx;
            const int ry = room->y() * -1 * view.mRoomHeight + fy;
            if (qAbs(mx - rx) < halfW && qAbs(my - ry) < halfH) {
                hitRooms.append(roomId);
            }
        }
    }
    return hitRooms;
}

void TMapPainter::addRoomsToSelection(TMap* pMap, QSet<int>& selection, bool& multiSelection, int& highlightRoomId, const QList<int>& rooms, const Qt::KeyboardModifiers modifiers)
{
    const bool hasShift = modifiers.testFlag(Qt::ShiftModifier);
    const bool hasCtrl = modifiers.testFlag(Qt::ControlModifier);
    for (const int roomId : rooms) {
        const bool isAlreadySelected = selection.contains(roomId);

        if (hasCtrl) {
            if (isAlreadySelected && !hasShift) {
                selection.remove(roomId);
            } else {
                selection.insert(roomId);
            }
        } else {
            selection.insert(roomId);
        }

        if (!selection.empty()) {
            multiSelection = false;
        }
    }

    const int selectionSize = selection.size();
    switch (selectionSize) {
    case 0:
        highlightRoomId = 0;
        break;
    case 1:
        multiSelection = false;
        highlightRoomId = *(selection.begin());
        break;
    default:
        multiSelection = false;
        highlightRoomId = selectionCentreRoom(pMap, selection);
        break;
    }
}

std::optional<bool> TMapPainter::zoomAtPoint(TArea& area,
                                             qreal& xyzoom,
                                             qreal& mapCenterX,
                                             qreal& mapCenterY,
                                             const QSize& size,
                                             const QPointF& position,
                                             const QPoint& angleDelta,
                                             const Qt::KeyboardModifiers modifiers,
                                             const bool invertZoom)
{
    // Increase rate if control key down - it makes scrolling through
    // a large number of items in a listwidget's contents easier (that happens
    // automagically) AND this make it easier to zoom in and out on LARGE area
    // maps
    const int yDelta = qRound(angleDelta.y() * (modifiers & Qt::ControlModifier ? 5.0 : 1.0) / (8.0 * 15.0));
    if (!yDelta) {
        return std::nullopt;
    }
    const qreal oldZoom = xyzoom;
    // If invert zoom is enabled, use the traditional (inverted) behavior
    // Otherwise, use modern behavior (non-inverted)
    const int adjustedYDelta = invertZoom ? yDelta : -yDelta;
    xyzoom = qMax(TMap::scmMinXYZoom, xyzoom * pow(1.07, adjustedYDelta));
    area.set2DMapZoom(xyzoom);

    if (qFuzzyCompare(1.0 + oldZoom, 1.0 + xyzoom)) {
        return false;
    }
    const float widgetWidth = size.width();
    const float widgetHeight = size.height();
    float xs = 1.0;
    float ys = 1.0;
    if (widgetWidth > 10 && widgetHeight > 10) {
        if (widgetWidth > widgetHeight) {
            xs = (widgetWidth / widgetHeight);
        } else {
            ys = (widgetHeight / widgetWidth);
        }
    }

    // Position of the mouse within the map, scaled -1 .. +1
    // i.e. if the mouse is in the center, nothing changes
    const float dx = 2.0 * position.x() / widgetWidth - 1.0;
    const float dy = 2.0 * position.y() / widgetHeight - 1.0;

    // now shift the origin by that, scaled by the difference in
    // zoom factors. Thus the point under the mouse stays in place.
    mapCenterX += dx * (oldZoom - xyzoom) / 2.0 * xs;
    mapCenterY += dy * (oldZoom - xyzoom) / 2.0 * ys;
    return true;
}

void TMapPainter::startSpeedWalk(Host* pHost, TMap* pMap, const int fromRoomId, const int toRoomId)
{
    if (pMap->mpRoomDB->getRoom(toRoomId)) {
        pMap->mTargetID = toRoomId;

        if (pHost->checkForCustomSpeedwalk()) {
            pHost->startSpeedWalk(fromRoomId, toRoomId);
        } else if (pMap->findPath(fromRoomId, toRoomId)) {
            pHost->startSpeedWalk();
        } else {
            pHost->printSystemMessage(
                    qsl("%1\n").arg(QCoreApplication::translate("T2DMap", "Mapper: Cannot find a path from %1 to %2 using known exits.").arg(QString::number(fromRoomId), QString::number(toRoomId))));
        }
    }
}

void TMapPainter::raiseMousePressEvent(Host* pHost)
{
    TEvent sysMapWindowMousePressEvent{};
    sysMapWindowMousePressEvent.mArgumentList.append(QLatin1String("sysMapWindowMousePressEvent"));
    sysMapWindowMousePressEvent.mArgumentTypeList.append(ARGUMENT_TYPE_STRING);
    pHost->raiseEvent(sysMapWindowMousePressEvent);
}

std::pair<bool, QString> TMapPainter::exportArea(int areaId, const QString& filePath, std::optional<int> zLevel, qreal zoom, bool exportAllZLevels, QObject* saveOwner)
{
    QList<std::pair<int, QString>> levelExports;
    if (const QString problem = mpMap->areaExportProblem(areaId, filePath, zoom, exportAllZLevels, levelExports); !problem.isEmpty()) {
        return {false, problem};
    }

    // Handle exporting all Z levels if requested
    if (exportAllZLevels) {
        for (const auto& [currentZLevel, levelFileName] : levelExports) {
            // Recursively call this function for each Z level (without exportAllZLevels flag)
            auto [success, message] = exportArea(areaId, levelFileName, currentZLevel, zoom, false, saveOwner);
            if (!success) {
                return {false, qsl("Failed to export Z level %1: %2").arg(currentZLevel).arg(message)};
            }
        }

        return {true, {}};
    }

    TArea* pArea = mpMap->mpRoomDB->getArea(areaId);
    // Calculate area bounds
    pArea->calcSpan();

    const int areaWidth = pArea->max_x - pArea->min_x + 1;
    const int areaHeight = pArea->max_y - pArea->min_y + 1;


    if (areaWidth <= 0 || areaHeight <= 0) {
        return {false, qsl("Area %1 has invalid dimensions").arg(areaId)};
    }

    // Calculate room size based on area dimensions - auto-sizing approach
    // Aim for reasonable room sizes that fit well regardless of area size
    const int targetImageDimension = 1024; // Target around 1024 pixels for the larger dimension
    const int maxDimension = qMax(areaWidth, areaHeight);
    const int roomSize = targetImageDimension / maxDimension;
    const int minRoomSize = 8;
    const int maxRoomSize = 50; // Prevent rooms from being too large
    const int finalRoomSize = qBound(minRoomSize, roomSize, maxRoomSize);

    // Calculate image size based on actual area content
    const int padding = finalRoomSize * 2;
    const int imageWidth = areaWidth * finalRoomSize + padding * 2;
    const int imageHeight = areaHeight * finalRoomSize + padding * 2;

    // Calculate offsets to position rooms correctly in the image
    // Map the area bounds to the image with padding, accounting for Y-axis flip
    const int exportRX = padding - (pArea->min_x * finalRoomSize);
    const int exportRY = padding - (pArea->min_y * finalRoomSize);


    // Create high-quality image for crisp exports using specified zoom
    QPixmap pixmap(imageWidth * zoom, imageHeight * zoom);
    pixmap.setDevicePixelRatio(zoom);
    pixmap.fill(mpHost->mBgColor_2);

    QPainter painter(&pixmap);
    if (!painter.isActive()) {
        return {false, qsl("Failed to create image painter")};
    }

    // Configure painter
    if (mMapperUseAntiAlias) {
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
        painter.setRenderHint(QPainter::TextAntialiasing, true);
    }

    // Store original coordinate system values to restore later
    const int originalRX = mRX;
    const int originalRY = mRY;
    const float originalRoomWidth = mRoomWidth;
    const float originalRoomHeight = mRoomHeight;

    // Set class member variables to export coordinate system (Qt handles scaling via devicePixelRatio)
    mRX = exportRX;
    mRY = exportRY;
    // Set mRoomWidth/mRoomHeight for high-quality rendering (Qt handles scaling)
    mRoomWidth = finalRoomSize;
    mRoomHeight = finalRoomSize;

    // Set up fonts and pens - use similar approach to paintEvent
    QFont roomVNumFont = mpMap->mMapSymbolFont;
    QFont mapNameFont = mpMap->mMapSymbolFont;

    // Get room size factors from host
    const float eSize = mpHost->mLineSize;
    const float rSize = mpHost->mRoomSize;

    // Calculate font size based on room dimensions
    const int fontPixelSize = qMax(6, static_cast<int>(mRoomHeight * rSize / 4.0));
    roomVNumFont.setPixelSize(fontPixelSize);
    mapNameFont.setPixelSize(fontPixelSize);

    // Reset symbol font size to match paintEvent behavior (line 1185)
    resetSymbolFontSize();

    // Flush symbol cache to ensure symbols are regenerated with export dimensions
    flushSymbolPixmapCache();

    // Calculate if room IDs should be legible based on export room size
    // Similar to logic in paintEvent, but using export dimensions
    QRectF roomRect(0, 0, finalRoomSize, finalRoomSize);
    QFont testFont = roomVNumFont;
    bool areRoomIdsLegible = mShowRoomID && sizeFontToFitTextInRect(testFont, roomRect, qsl("12345"), 10, 7.0);

    QPen pen;
    pen.setColor(mpHost->mFgColor_2);
    // Use the same exitWidth calculation as paintEvent (Qt handles scaling via devicePixelRatio)
    const float exitWidth = 1 / eSize * finalRoomSize * rSize;
    pen.setWidthF(exitWidth);

    // Use the same Z-level filtering and drawing approach as paintEvent
    const int exportZLevel = zLevel.has_value() ? zLevel.value() : mMapCenterZ;

    // Draw the ("background") labels that are on the bottom of the map - same as paintEvent
    QMutableMapIterator<int, TMapLabel> itMapLabel(pArea->mMapLabels);
    while (itMapLabel.hasNext()) {
        itMapLabel.next();
        auto mapLabel = itMapLabel.value();
        if (mapLabel.pos.z() != exportZLevel) {
            continue;
        }
        // Use export coordinate system for label positioning
        const int exportRX = padding - (pArea->min_x * finalRoomSize);
        const int exportRY = padding - (pArea->min_y * finalRoomSize);
        QPointF labelPosition;
        const int labelX = mapLabel.pos.x() * finalRoomSize + exportRX;
        const int labelY = mapLabel.pos.y() * finalRoomSize * -1 + exportRY;

        labelPosition.setX(labelX);
        labelPosition.setY(labelY);
        const int labelWidth = abs(qRound(mapLabel.size.width() * finalRoomSize));
        const int labelHeight = abs(qRound(mapLabel.size.height() * finalRoomSize));
        if (!((0 < labelX || 0 < labelX + labelWidth) && (imageWidth > labelX || imageWidth > labelX + labelWidth))) {
            continue;
        }
        if (!((0 < labelY || 0 < labelY + labelHeight) && (imageHeight > labelY || imageHeight > labelY + labelHeight))) {
            continue;
        }

        QRectF labelPaintRectangle = QRect(labelX, labelY, labelWidth, labelHeight);
        if (!mapLabel.showOnTop) {
            if (!mapLabel.noScaling) {
                drawPixmapScaledTo(painter, QRectF(labelPosition, labelPaintRectangle.size()), mapLabel.pix);
                mapLabel.clickSize = QSizeF(labelPaintRectangle.width(), labelPaintRectangle.height());
            } else {
                painter.drawPixmap(labelPosition, mapLabel.pix);
                mapLabel.clickSize = QSizeF(mapLabel.pix.width(), mapLabel.pix.height());
            }
            pArea->mMapLabels[itMapLabel.key()] = mapLabel;
        }

        if (mapLabel.highlight) {
            paintLabelHighlight(painter, labelPaintRectangle, mapLabel.clickSize);
        }
    }

    QList<int> exitList;
    QList<int> oneWayExits;

    // Build exit lists from rooms on current Z-level (like paintEvent does)
    for (const int roomId : pArea->getRoomsForZ(exportZLevel)) {
        TRoom* pRoom = mpMap->mpRoomDB->getRoom(roomId);
        if (!pRoom) {
            continue;
        }
        exitList << roomId;
    }


    // Now draw rooms on Z-level and below (like paintEvent does)
    int roomsDrawn = 0;
    int roomsSkipped = 0;

    // First pass: draw rooms on level below (like paintEvent shadow rooms)
    for (const int roomId : pArea->getRoomsForZ(exportZLevel - 1)) {
        TRoom* pRoom = mpMap->mpRoomDB->getRoom(roomId);
        if (!pRoom) {
            continue;
        }

        // Use export coordinate system for room positioning
        const int exportRX = padding - (pArea->min_x * finalRoomSize);
        const int exportRY = padding - (pArea->min_y * finalRoomSize);
        const float rx = pRoom->x() * finalRoomSize + exportRX;
        const float ry = pRoom->y() * -1 * finalRoomSize + exportRY;

        if (rx >= 0 && ry >= 0 && rx <= imageWidth && ry <= imageHeight) {
            painter.save();
            painter.setPen(Qt::NoPen);
            painter.setBrush(QBrush(mpHost->mLowerLevelColor));

            // Draw shadow room using the same approach as paintEvent (lines 1426-1442)
            const qreal scaledRoomSize = static_cast<qreal>(finalRoomSize) * rSize;
            if (mBubbleMode) {
                const QPointF roomCenter(rx, ry);
                const qreal roomRadius = scaledRoomSize / 2.0;
                QPainterPath diameterPath;
                diameterPath.addEllipse(roomCenter, roomRadius, roomRadius);
                painter.drawPath(diameterPath);
            } else {
                const QRectF shadowRect(rx - (scaledRoomSize * 0.8), ry - (scaledRoomSize * 0.2), scaledRoomSize, scaledRoomSize);
                painter.drawRect(shadowRect);
            }
            painter.restore();
            roomsDrawn++;
        }
    }

    // Second pass: draw rooms on level above (like paintEvent upper level rooms)
    for (const int roomId : pArea->getRoomsForZ(exportZLevel + 1)) {
        TRoom* pRoom = mpMap->mpRoomDB->getRoom(roomId);
        if (!pRoom) {
            continue;
        }

        // Use export coordinate system for room positioning
        const int exportRX = padding - (pArea->min_x * finalRoomSize);
        const int exportRY = padding - (pArea->min_y * finalRoomSize);
        const float rx = pRoom->x() * finalRoomSize + exportRX;
        const float ry = pRoom->y() * -1 * finalRoomSize + exportRY;

        if (rx >= 0 && ry >= 0 && rx <= imageWidth && ry <= imageHeight) {
            painter.save();
            painter.setPen(QPen(mpHost->mUpperLevelColor, 1));
            painter.setBrush(Qt::transparent);

            // Draw outline room using the same approach as paintEvent (lines 1456-1471)
            if (mBubbleMode) {
                const float roomRadius = 0.5 * rSize * finalRoomSize;
                const QPointF roomCenter = QPointF(rx + (roomRadius * rSize * 0.5), ry - (roomRadius * rSize * 0.5));
                QPainterPath diameterPath;
                diameterPath.addEllipse(roomCenter, roomRadius, roomRadius);
                painter.drawPath(diameterPath);
            } else {
                painter.drawRect(rx - (finalRoomSize * rSize * 0.2), ry - (finalRoomSize * rSize * 0.8), finalRoomSize * rSize, finalRoomSize * rSize);
            }
            painter.restore();
            roomsDrawn++;
        }
    }

    // Third pass: draw room exits (moved to correct position after upper-level rooms)
    if (!pArea->gridMode) {
        // paintRoomExits uses width() and height() for visibility filtering, but we need to use imageWidth/imageHeight
        // Since we can't easily override those methods, we'll create a custom export-specific exit drawing

        // Draw exits using similar logic to paintRoomExits but with export-specific coordinates
        QSetIterator<int> itRoom2(pArea->getRoomsForZ(exportZLevel));
        while (itRoom2.hasNext()) {
            const int roomId = itRoom2.next();
            TRoom* room = mpMap->mpRoomDB->getRoom(roomId);
            if (!room) {
                continue;
            }

            const float rx = room->x() * finalRoomSize + (padding - (pArea->min_x * finalRoomSize));
            const float ry = room->y() * -1 * finalRoomSize + (padding - (pArea->min_y * finalRoomSize));

            // Skip rooms that are outside the image bounds
            if (rx < 0 || ry < 0 || rx > imageWidth || ry > imageHeight) {
                continue;
            }

            room->rendered = true;

            // First, draw custom exit lines (if any) - exact logic from paintRoomExits
            if (!room->customLines.empty()) {
                painter.save();
                QMapIterator<QString, QList<QPointF>> itk(room->customLines);
                while (itk.hasNext()) {
                    itk.next();
                    const QString exitKey = itk.key();
                    QColor customLineColor;
                    if (roomId == mCustomLineSelectedRoom && exitKey == mCustomLineSelectedExit) {
                        customLineColor = QColor(255, 155, 55);
                    } else {
                        customLineColor = room->customLinesColor.value(exitKey, Qt::red);
                    }

                    const QPointF origin = QPointF(rx, ry);

                    // Calculate fixed offset point based on exit direction (same logic as paintRoomExits)
                    QPointF fixedOffsetPoint;
                    bool isXYPlainExit = false;
                    if (exitKey == "n") {
                        fixedOffsetPoint = QPointF(rx, ry - finalRoomSize / 2.0);
                        isXYPlainExit = true;
                    } else if (exitKey == "ne") {
                        fixedOffsetPoint = QPointF(rx + finalRoomSize / 2.0, ry - finalRoomSize / 2.0);
                        isXYPlainExit = true;
                    } else if (exitKey == "e") {
                        fixedOffsetPoint = QPointF(rx + finalRoomSize / 2.0, ry);
                        isXYPlainExit = true;
                    } else if (exitKey == "se") {
                        fixedOffsetPoint = QPointF(rx + finalRoomSize / 2.0, ry + finalRoomSize / 2.0);
                        isXYPlainExit = true;
                    } else if (exitKey == "s") {
                        fixedOffsetPoint = QPointF(rx, ry + finalRoomSize / 2.0);
                        isXYPlainExit = true;
                    } else if (exitKey == "sw") {
                        fixedOffsetPoint = QPointF(rx - finalRoomSize / 2.0, ry + finalRoomSize / 2.0);
                        isXYPlainExit = true;
                    } else if (exitKey == "w") {
                        fixedOffsetPoint = QPointF(rx - finalRoomSize / 2.0, ry);
                        isXYPlainExit = true;
                    } else if (exitKey == "nw") {
                        fixedOffsetPoint = QPointF(rx - finalRoomSize / 2.0, ry - finalRoomSize / 2.0);
                        isXYPlainExit = true;
                    } else {
                        fixedOffsetPoint = QPointF(rx, ry);
                    }

                    QPen customLinePen = painter.pen();
                    customLinePen.setCosmetic(mMapperUseAntiAlias);
                    customLinePen.setWidthF(exitWidth);
                    customLinePen.setCapStyle(Qt::RoundCap);
                    customLinePen.setJoinStyle(Qt::RoundJoin);
                    customLinePen.setColor(customLineColor);
                    customLinePen.setStyle(room->customLinesStyle.value(exitKey, Qt::SolidLine));

                    QVector<QPointF> polyLinePoints;
                    const QList<QPointF> customLinePoints = itk.value();
                    QLineF doorLineSegment;

                    if (!customLinePoints.empty()) {
                        painter.setPen(customLinePen);
                        polyLinePoints << origin;
                        polyLinePoints << fixedOffsetPoint;

                        // Transform custom line points (same as original: customLinePoint.x() * mRoomWidth + mRX)
                        for (const QPointF& customLinePoint : customLinePoints) {
                            const float pointX = customLinePoint.x() * finalRoomSize + (padding - (pArea->min_x * finalRoomSize));
                            const float pointY = customLinePoint.y() * finalRoomSize * -1 + (padding - (pArea->min_y * finalRoomSize));
                            polyLinePoints << QPointF(pointX, pointY);
                        }

                        // Determine door line segment for door drawing (same logic as original)
                        if (polyLinePoints.size() > 2) {
                            if (isXYPlainExit) {
                                doorLineSegment = QLineF{polyLinePoints.at(0), polyLinePoints.at(1)};
                            } else {
                                // Non-XY-Plane exits have the first two points being coincident:
                                doorLineSegment = QLineF{polyLinePoints.at(1), polyLinePoints.at(2)};
                            }
                        } else {
                            // There must be 2 points ...
                            doorLineSegment = QLineF{polyLinePoints.at(0), polyLinePoints.at(1)};
                        }

                        // Draw the custom polyline
                        painter.drawPolyline(polyLinePoints.data(), polyLinePoints.size());

                        // Draw arrow if needed (same logic as original)
                        if (room->customLinesArrow.value(exitKey, false)) {
                            QLineF arrowLine = QLineF(polyLinePoints.last(), polyLinePoints.at(polyLinePoints.size() - 2));
                            arrowLine.setLength(exitWidth * 5.0);

                            QLineF arrowHead1 = arrowLine;
                            arrowHead1.setLength(exitWidth * 3.0);
                            arrowHead1.setAngle(arrowLine.angle() + 20);

                            QLineF arrowHead2 = arrowLine;
                            arrowHead2.setLength(exitWidth * 3.0);
                            arrowHead2.setAngle(arrowLine.angle() - 20);

                            painter.drawLine(arrowHead1);
                            painter.drawLine(arrowHead2);
                        }

                        // Draw door if exists on this custom line
                        if (room->doors.value(exitKey, 0) > 0) {
                            drawDoor(painter, *room, exitKey, doorLineSegment);
                        }
                    }
                }
                painter.restore();
            }

            // Now draw regular exits - use exact same logic as paintRoomExits
            QList<int> roomExitList;
            QList<int> roomOneWayExits;

            // Helper function to add exit and check for one-way
            auto addExitAndCheckOneWay = [&](const QString& dirKey, int exitRoomId) {
                if (exitRoomId > 0) {
                    roomExitList.push_back(exitRoomId);
                    TRoom* exitRoom = mpMap->mpRoomDB->getRoom(exitRoomId);
                    if (exitRoom) {
                        // Check if it's a one-way exit (return direction doesn't point back)
                        bool isOneWay = false;
                        if (dirKey == "n" && exitRoom->getSouth() != roomId)
                            isOneWay = true;
                        else if (dirKey == "s" && exitRoom->getNorth() != roomId)
                            isOneWay = true;
                        else if (dirKey == "e" && exitRoom->getWest() != roomId)
                            isOneWay = true;
                        else if (dirKey == "w" && exitRoom->getEast() != roomId)
                            isOneWay = true;
                        else if (dirKey == "ne" && exitRoom->getSouthwest() != roomId)
                            isOneWay = true;
                        else if (dirKey == "nw" && exitRoom->getSoutheast() != roomId)
                            isOneWay = true;
                        else if (dirKey == "se" && exitRoom->getNorthwest() != roomId)
                            isOneWay = true;
                        else if (dirKey == "sw" && exitRoom->getNortheast() != roomId)
                            isOneWay = true;

                        if (isOneWay) {
                            roomOneWayExits.push_back(exitRoomId);
                        }
                    }
                }
            };

            // Only add regular exits for directions that don't have custom lines (like paintRoomExits)
            if (!room->customLines.empty()) {
                // This room has custom lines - only add regular exits for directions without custom lines
                if (!room->customLines.contains("n")) {
                    addExitAndCheckOneWay("n", room->getNorth());
                }
                if (!room->customLines.contains("ne")) {
                    addExitAndCheckOneWay("ne", room->getNortheast());
                }
                if (!room->customLines.contains("e")) {
                    addExitAndCheckOneWay("e", room->getEast());
                }
                if (!room->customLines.contains("se")) {
                    addExitAndCheckOneWay("se", room->getSoutheast());
                }
                if (!room->customLines.contains("s")) {
                    addExitAndCheckOneWay("s", room->getSouth());
                }
                if (!room->customLines.contains("sw")) {
                    addExitAndCheckOneWay("sw", room->getSouthwest());
                }
                if (!room->customLines.contains("w")) {
                    addExitAndCheckOneWay("w", room->getWest());
                }
                if (!room->customLines.contains("nw")) {
                    addExitAndCheckOneWay("nw", room->getNorthwest());
                }
            } else {
                // This room has no custom lines - add all regular exits (only 8 cardinal/diagonal directions)
                addExitAndCheckOneWay("n", room->getNorth());
                addExitAndCheckOneWay("s", room->getSouth());
                addExitAndCheckOneWay("e", room->getEast());
                addExitAndCheckOneWay("w", room->getWest());
                addExitAndCheckOneWay("ne", room->getNortheast());
                addExitAndCheckOneWay("nw", room->getNorthwest());
                addExitAndCheckOneWay("se", room->getSoutheast());
                addExitAndCheckOneWay("sw", room->getSouthwest());
            }

            // Draw regular exit lines to destination rooms, leaving out those a straight custom line already draws
            for (const int exitRoomId : roomExitList) {
                if (exitRoomId <= 0)
                    continue;

                TRoom* exitRoom = mpMap->mpRoomDB->getRoom(exitRoomId);
                if (!exitRoom)
                    continue;

                // Determine if this is an area exit (like original paintRoomExits)
                const bool areaExit = exitRoom->getArea() != areaId;
                const float exitArrowScale = (mLargeAreaExitArrows ? 2.0f : 1.0f);

                QLineF exitLine;
                QString doorKey;

                // Determine door key and exit line based on direction
                if (room->getNorth() == exitRoomId) {
                    doorKey = "n";
                    if (areaExit) {
                        exitLine = QLineF(rx, ry - exitArrowScale * finalRoomSize, rx, ry);
                    }
                } else if (room->getSouth() == exitRoomId) {
                    doorKey = "s";
                    if (areaExit) {
                        exitLine = QLineF(rx, ry + exitArrowScale * finalRoomSize, rx, ry);
                    }
                } else if (room->getEast() == exitRoomId) {
                    doorKey = "e";
                    if (areaExit) {
                        exitLine = QLineF(rx + exitArrowScale * finalRoomSize, ry, rx, ry);
                    }
                } else if (room->getWest() == exitRoomId) {
                    doorKey = "w";
                    if (areaExit) {
                        exitLine = QLineF(rx - exitArrowScale * finalRoomSize, ry, rx, ry);
                    }
                } else if (room->getNortheast() == exitRoomId) {
                    doorKey = "ne";
                    if (areaExit) {
                        exitLine = QLineF(rx + exitArrowScale * finalRoomSize, ry - exitArrowScale * finalRoomSize, rx, ry);
                    }
                } else if (room->getNorthwest() == exitRoomId) {
                    doorKey = "nw";
                    if (areaExit) {
                        exitLine = QLineF(rx - exitArrowScale * finalRoomSize, ry - exitArrowScale * finalRoomSize, rx, ry);
                    }
                } else if (room->getSoutheast() == exitRoomId) {
                    doorKey = "se";
                    if (areaExit) {
                        exitLine = QLineF(rx + exitArrowScale * finalRoomSize, ry + exitArrowScale * finalRoomSize, rx, ry);
                    }
                } else if (room->getSouthwest() == exitRoomId) {
                    doorKey = "sw";
                    if (areaExit) {
                        exitLine = QLineF(rx - exitArrowScale * finalRoomSize, ry + exitArrowScale * finalRoomSize, rx, ry);
                    }
                }

                if (!areaExit) {
                    // Same-area exit: draw line to destination room (original logic)
                    const float exitRoomX = exitRoom->x() * finalRoomSize + (padding - (pArea->min_x * finalRoomSize));
                    const float exitRoomY = exitRoom->y() * -1 * finalRoomSize + (padding - (pArea->min_y * finalRoomSize));

                    // Skip if destination room is outside the image bounds
                    if (exitRoomX < 0 || exitRoomY < 0 || exitRoomX > imageWidth || exitRoomY > imageHeight) {
                        continue;
                    }

                    exitLine = QLineF(rx, ry, exitRoomX, exitRoomY);

                    if (roomOneWayExits.contains(exitRoomId)) {
                        // Draw one-way exit with dashed line
                        QPen oneWayPen = pen;
                        oneWayPen.setStyle(Qt::DashLine);
                        painter.setPen(oneWayPen);
                        painter.drawLine(exitLine);
                    } else {
                        // Also for a line left out: the door drawn below takes its width from this pen
                        painter.setPen(pen);
                        if (!customLineRunsStraightTo(*room, *exitRoom) && !(exitRoom->z() == exportZLevel && customLineRunsStraightTo(*exitRoom, *room))) {
                            painter.drawLine(exitLine);
                        }
                    }

                } else {
                    // Area exit: draw directional stub with arrow (like original paintRoomExits)
                    painter.save();
                    QPen areaPen = pen;
                    areaPen.setWidthF(exitWidth);
                    areaPen.setCapStyle(Qt::RoundCap);
                    areaPen.setCosmetic(mMapperUseAntiAlias);
                    areaPen.setColor(mpMap->getColor(exitRoomId));

                    // Draw outline first for visibility on matching backgrounds
                    QPen outlinePen = areaPen;
                    outlinePen.setWidthF(exitWidth + 2.0);
                    outlinePen.setColor((mpHost->mBgColor_2.lightness() > 127) ? Qt::black : Qt::white);
                    painter.setPen(outlinePen);
                    painter.drawLine(exitLine);

                    // Draw colored line on top
                    painter.setPen(areaPen);
                    painter.drawLine(exitLine);

                    // Draw arrow head (same logic as original paintRoomExits lines 2298-2333)
                    QLineF arrowLine = QLineF(exitLine);
                    if (mLargeAreaExitArrows) {
                        arrowLine.setLength((finalRoomSize * 2.0) * 0.4);
                    } else {
                        arrowLine.setLength(exitWidth * 5.0);
                    }
                    const QPointF arrowTip = arrowLine.p1();
                    const QPointF arrowBase = arrowLine.p2();
                    const QLineF arrowBaseLine = QLineF(arrowLine);
                    const qreal arrowAngle = arrowBaseLine.angle() - 90.0;

                    QLineF arrowSide;
                    arrowSide.setP1(arrowBase);
                    arrowSide.setAngle(arrowAngle);
                    if (mLargeAreaExitArrows) {
                        arrowSide.setLength((finalRoomSize * 2.0) * 0.15);
                    } else {
                        arrowSide.setLength(exitWidth * 2.0);
                    }
                    const QPointF arrowLeft = arrowSide.p2();
                    arrowSide.setAngle(arrowSide.angle() + 180.0);
                    const QPointF arrowRight = arrowSide.p2();

                    QPolygonF arrowHead;
                    arrowHead.append(arrowTip);
                    arrowHead.append(arrowLeft);
                    arrowHead.append(arrowRight);

                    QBrush arrowBrush = painter.brush();
                    arrowBrush.setColor(mpMap->getColor(exitRoomId));
                    arrowBrush.setStyle(Qt::SolidPattern);
                    QPen arrowPen = painter.pen();
                    arrowPen.setJoinStyle(Qt::RoundJoin);
                    arrowPen.setCapStyle(Qt::RoundCap);
                    arrowPen.setCosmetic(mMapperUseAntiAlias);

                    // Draw outline first for visibility
                    QPen arrowOutlinePen = arrowPen;
                    arrowOutlinePen.setWidthF(2.0);
                    arrowOutlinePen.setColor((mpHost->mBgColor_2.lightness() > 127) ? Qt::black : Qt::white);
                    painter.setPen(arrowOutlinePen);
                    painter.setBrush(Qt::NoBrush);
                    painter.drawPolygon(arrowHead);

                    // Draw filled arrow head on top
                    painter.setPen(Qt::NoPen);
                    painter.setBrush(arrowBrush);
                    painter.drawPolygon(arrowHead);
                    painter.restore();
                }

                // Draw door if exists on this exit (both area and same-area exits)
                if (!doorKey.isEmpty() && room->doors.value(doorKey, 0) > 0) {
                    // Apply half-length reduction like original paintRoomExits
                    QLineF halfExitLine = exitLine;
                    halfExitLine.setLength(halfExitLine.length() / 2.0);
                    drawDoor(painter, *room, doorKey, halfExitLine);
                }
            }
        }
    }

    // Fourth pass: draw main rooms on current level using existing drawRoom method
    const QList<int> roomsOnLevel = pArea->getRoomsForZ(exportZLevel).values();
    const bool layerRooms = anyRoomHighlighted(roomsOnLevel);
    QList<QPair<TRoom*, QPointF>> layeredRooms;
    for (const int roomId : roomsOnLevel) {
        TRoom* pRoom = mpMap->mpRoomDB->getRoom(roomId);
        if (!pRoom) {
            roomsSkipped++;
            continue;
        }

        // Use export coordinate system for room positioning
        const int exportRX = padding - (pArea->min_x * finalRoomSize);
        const int exportRY = padding - (pArea->min_y * finalRoomSize);
        const float rx = pRoom->x() * finalRoomSize + exportRX;
        const float ry = pRoom->y() * -1 * finalRoomSize + exportRY;

        // Debug: Check which rooms are being filtered out
        if (rx < 0 || ry < 0 || rx > imageWidth || ry > imageHeight) {
            roomsSkipped++;
            continue;
        }

        // Use the existing drawRoom method!
        drawRoom(painter,
                 roomVNumFont,
                 mapNameFont,
                 pen,
                 pRoom,
                 pArea->gridMode,
                 areRoomIdsLegible,
                 false /* showRoomNames */,
                 -1 /* speedWalkStartRoomId */,
                 rx,
                 ry,
                 false /* showRoomCollision */,
                 layerRooms ? RoomDrawPass::Body : RoomDrawPass::Whole);
        if (layerRooms) {
            layeredRooms.append({pRoom, QPointF(rx, ry)});
        }

        roomsDrawn++;
    }
    if (layerRooms) {
        drawRoomHighlightsAndLabels(painter, roomVNumFont, mapNameFont, pen, layeredRooms, pArea->gridMode, areRoomIdsLegible, false /* showRoomNames */, -1 /* speedWalkStartRoomId */);
    }


    // Draw the ("foreground") labels that are on the top of the map - same as paintEvent
    itMapLabel.toFront(); // Reset the iterator
    while (itMapLabel.hasNext()) {
        itMapLabel.next();
        auto mapLabel = itMapLabel.value();
        if (mapLabel.pos.z() != exportZLevel) {
            continue;
        }
        // Use export coordinate system for label positioning
        const int exportRX = padding - (pArea->min_x * finalRoomSize);
        const int exportRY = padding - (pArea->min_y * finalRoomSize);
        const int labelX = mapLabel.pos.x() * finalRoomSize + exportRX;
        const int labelY = mapLabel.pos.y() * finalRoomSize * -1 + exportRY;
        const int labelWidth = abs(qRound(mapLabel.size.width() * finalRoomSize));
        const int labelHeight = abs(qRound(mapLabel.size.height() * finalRoomSize));

        if (!((0 < labelX || 0 < labelX + labelWidth) && (imageWidth > labelX || imageWidth > labelX + labelWidth))) {
            continue;
        }
        if (!((0 < labelY || 0 < labelY + labelHeight) && (imageHeight > labelY || imageHeight > labelY + labelHeight))) {
            continue;
        }
        QRectF labelPaintRectangle = QRect(labelX, labelY, labelWidth, labelHeight);
        if (mapLabel.showOnTop) {
            QPointF labelPosition(labelX, labelY);
            if (!mapLabel.noScaling) {
                drawPixmapScaledTo(painter, QRectF(labelPosition, labelPaintRectangle.size()), mapLabel.pix);
                mapLabel.clickSize = QSizeF(labelPaintRectangle.width(), labelPaintRectangle.height());
            } else {
                painter.drawPixmap(labelPosition, mapLabel.pix);
                mapLabel.clickSize = QSize(mapLabel.pix.width(), mapLabel.pix.height());
            }
            pArea->mMapLabels[itMapLabel.key()] = mapLabel;
        }
        if (mapLabel.highlight) {
            paintLabelHighlight(painter, labelPaintRectangle, mapLabel.clickSize);
        }
    }

    painter.end();

    // Restore the original coordinate system
    mRX = originalRX;
    mRY = originalRY;
    mRoomWidth = originalRoomWidth;
    mRoomHeight = originalRoomHeight;
    // The symbols were drawn at the export's room size, which the view's next frame would otherwise reuse as its own
    flushSymbolPixmapCache();

    // Each export has a watcher of its own: several can be in flight at once,
    // from calls made back to back or one per z level
    auto* pWatcher = new QFutureWatcher<std::pair<bool, QString>>(saveOwner);
    QObject::connect(pWatcher, &QFutureWatcher<std::pair<bool, QString>>::finished, saveOwner, [pWatcher, pHost = QPointer<Host>(mpHost), profileName = mpHost->getName()]() {
        const auto [saved, errorMessage] = pWatcher->result();
        if (!saved) {
            // This view can outlive its profile, which may have closed before the save reported back
            if (pHost) {
                pHost->postMessage(QCoreApplication::translate("T2DMap", "[MAP]: %1").arg(errorMessage));
            } else {
                qWarning().noquote().nospace() << "TMapPainter::exportArea() WARNING - profile \"" << profileName << "\" closed before this export finished: " << errorMessage;
            }
        }
        pWatcher->deleteLater();
    });

    auto future = QtConcurrent::task(&TMapPainter::performImageSave).withArguments(pixmap, filePath, TMap::exportImageFormat(filePath)).spawn();
    pWatcher->setFuture(future);

    return {true, {}};
}

std::pair<bool, QString> TMapPainter::performImageSave(const QPixmap& pixmap, const QString& filePath, const QString& format)
{
    if (!pixmap.save(filePath, format.toLocal8Bit().data())) {
        return {false, qsl("Failed to save image to %1").arg(filePath)};
    }

    return {true, {}};
}
