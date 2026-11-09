#ifndef MUDLET_PIXELTESTHELPER_H
#define MUDLET_PIXELTESTHELPER_H

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

#include <chrono>
#include <cstdlib>

#include <QColor>
#include <QCoreApplication>
#include <QIcon>
#include <QImage>
#include <QPoint>
#include <QRect>
#include <QSize>
#include <QString>
#include <QTreeWidgetItem>
#include <QWidget>
#include <QtTest/QTest>

#include "enums.h"
#include "mudlet.h"
#include "utils.h"

// Free inline functions, like TestSettings: nothing runs moc over a header that
// CMakeLists.txt does not list. Nothing here may use QVERIFY or QFAIL - both
// expand to a bare return, ill-formed in a function returning anything.
namespace TestPixels {

// Each is the longest wait any of the tests that shared the helper needed
inline constexpr std::chrono::milliseconds scmSettleWait{80};
inline constexpr std::chrono::milliseconds scmRestyleWait{150};

// Summed rather than per channel: at a pixel blending two flat fills the
// nearer fill is the one that was painted there
inline int distance(const QColor& one, const QColor& other)
{
    return std::abs(one.red() - other.red()) + std::abs(one.green() - other.green()) + std::abs(one.blue() - other.blue());
}

inline bool readsAs(const QColor& read, const QColor& wanted, const int tolerance)
{
    return std::abs(read.red() - wanted.red()) <= tolerance && std::abs(read.green() - wanted.green()) <= tolerance && std::abs(read.blue() - wanted.blue()) <= tolerance;
}

// Qt answers some palette roles with an alpha (PlaceholderText is 128), so what
// a reader sees is this mix over the surface, not the colour the palette holds
inline QColor composited(const QColor& ink, const QColor& surface)
{
    if (ink.alpha() == 255) {
        return ink;
    }
    const qreal weight = ink.alphaF();
    return QColor::fromRgbF(ink.redF() * weight + surface.redF() * (1.0 - weight), ink.greenF() * weight + surface.greenF() * (1.0 - weight), ink.blueF() * weight + surface.blueF() * (1.0 - weight));
}

// A grab is in device pixels while widget geometry is logical. A point off the
// grab reads as an invalid colour rather than the nearest edge pixel: it is a
// wrong coordinate, and the edge pixel could pass a check the right one fails.
inline QColor sampled(const QImage& grab, const QPoint& at)
{
    const qreal ratio = grab.devicePixelRatio();
    const QPoint device(qRound(at.x() * ratio), qRound(at.y() * ratio));
    if (!grab.rect().contains(device)) {
        return QColor();
    }
    return grab.pixelColor(device);
}

// A glyph is a tinted stroke over nothing, so its most opaque pixel carries the ink
inline QString inkOf(const QImage& glyph)
{
    QColor strongestColour;
    int strongest = -1;
    for (int y = 0; y < glyph.height(); ++y) {
        for (int x = 0; x < glyph.width(); ++x) {
            const QColor pixel = glyph.pixelColor(x, y);
            if (pixel.alpha() > strongest) {
                strongest = pixel.alpha();
                strongestColour = pixel;
            }
        }
    }
    return strongest <= 0 ? qsl("nothing at all") : qsl("%1 at alpha %2").arg(strongestColour.name(), QString::number(strongest));
}

inline QImage drawnAt(const QIcon& icon, const int size, const QIcon::State state = QIcon::Off)
{
    return icon.pixmap(QSize(size, size), QIcon::Normal, state).toImage();
}

// The posted events first, then the timers a relayout or restyle queues, then
// what those posted in turn
inline void settle()
{
    QCoreApplication::processEvents();
    QTest::qWait(scmSettleWait);
    QCoreApplication::processEvents();
}

// setAppearance() swaps the application style and widgets restyle on the
// StyleChange that follows. Under the offscreen platform nothing paints unless
// grabbed, and painting is what settles a widget's palette against the application's.
inline void setAppearanceAndWait(const enums::Appearance appearance, QWidget* pToPaint = nullptr)
{
    mudlet::self()->setAppearance(appearance);
    QCoreApplication::processEvents();
    QTest::qWait(scmRestyleWait);
    QCoreApplication::processEvents();
    if (pToPaint) {
        pToPaint->grab();
    }
}

// Through the widget, since offscreen there is no screen to grab. Pass the
// window rather than a see-through child: a widget grabbed on its own comes back
// without the pane that shows through it.
inline QImage shot(QWidget* pWidget)
{
    QCoreApplication::processEvents();
    return pWidget->grab().toImage().convertToFormat(QImage::Format_ARGB32);
}

// The editor's trees keep an item's id in Qt::UserRole of its first column
inline QTreeWidgetItem* rowFor(QTreeWidgetItem* pParent, const int id)
{
    for (int row = 0, rows = pParent->childCount(); row < rows; ++row) {
        QTreeWidgetItem* pChild = pParent->child(row);
        if (pChild->data(0, Qt::UserRole).toInt() == id) {
            return pChild;
        }
        if (QTreeWidgetItem* pFound = rowFor(pChild, id)) {
            return pFound;
        }
    }
    return nullptr;
}

inline QString describe(const QRect& rect)
{
    return qsl("%1,%2 %3x%4").arg(QString::number(rect.x()), QString::number(rect.y()), QString::number(rect.width()), QString::number(rect.height()));
}

inline QString describe(const QSize& size)
{
    return qsl("%1x%2").arg(QString::number(size.width()), QString::number(size.height()));
}

} // namespace TestPixels

#endif // MUDLET_PIXELTESTHELPER_H
