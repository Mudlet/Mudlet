/***************************************************************************
 *   Copyright (C) 2026 by Jay Howard - jay.patrick.howard@gmail.com       *
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

#include "TTextEdit.h"

#include <QTest>

#include <cmath>

class TTextEditPaintSurfaceTest : public QObject
{
    Q_OBJECT

private slots:
    void theSurfaceIsNeverSmallerThanTheCellsItHolds_data()
    {
        QTest::addColumn<int>("screenWidth");
        QTest::addColumn<int>("fontWidth");
        QTest::addColumn<int>("pixmapHeight");
        QTest::addColumn<qreal>("devicePixelRatio");
        QTest::addColumn<bool>("productIsFractional");

        // pixmapHeight is (rows + 1) * fontHeight, so its parity moves with both
        // the font size and the window height. Whether the device-pixel product
        // comes out whole depends on all three, and every combination has to be
        // accepted: the surface is allocated from the same numbers.
        // Windows reports fractional ratios directly at the common 125% and 150%
        // settings, and some ratios leave almost no window height with a whole
        // product at all. What matters is not the particular ratio but whether
        // pixmapHeight * ratio lands on a whole device pixel: when it does not,
        // the surface cannot be allocated at the size the old check demanded.
        // The flag is asserted below, so editing these numbers cannot quietly
        // leave the suite with nothing that exercises the defect.
        QTest::newRow("whole product, unscaled") << 148 << 8 << 782 << 1.0 << false;
        QTest::newRow("whole product, unscaled odd height") << 148 << 8 << 765 << 1.0 << false;
        QTest::newRow("whole product at a fractional ratio") << 148 << 8 << 784 << 1.25 << false;
        QTest::newRow("whole product, doubled ratio") << 148 << 8 << 765 << 2.0 << false;
        QTest::newRow("whole product, ratio 1.5") << 148 << 8 << 782 << 1.5 << false;
        QTest::newRow("fractional height product") << 148 << 8 << 765 << 1.25 << true;
        QTest::newRow("fractional height product, other parity") << 148 << 8 << 782 << 1.25 << true;
        QTest::newRow("fractional height product, ratio 1.5") << 148 << 8 << 765 << 1.5 << true;
        QTest::newRow("fractional height product, ratio 1.75") << 148 << 8 << 799 << 1.75 << true;
        QTest::newRow("fractional height product, doubled fractional ratio") << 148 << 8 << 765 << 2.5 << true;
        QTest::newRow("fractional on both axes") << 148 << 8 << 765 << 1.43 << true;
        QTest::newRow("fractional on both axes, doubled") << 148 << 8 << 714 << 2.86 << true;
        QTest::newRow("fractional product, odd cell metrics") << 101 << 7 << 663 << 1.25 << true;
    }

    // The invariant the paint path relies on. A surface short of the area it
    // holds cannot be matched by the cached screen kept from the previous paint,
    // so drawForeground() stops reusing it, falls through with drawFrom past
    // drawTo, and redraws nothing in the region that was damaged.
    void theSurfaceIsNeverSmallerThanTheCellsItHolds()
    {
        QFETCH(int, screenWidth);
        QFETCH(int, fontWidth);
        QFETCH(int, pixmapHeight);
        QFETCH(qreal, devicePixelRatio);
        QFETCH(bool, productIsFractional);

        // The row is only exercising the defect if the product really is
        // fractional, so check the data before trusting the result below.
        const qreal idealHeight = pixmapHeight * devicePixelRatio;
        const bool actuallyFractional = !qFuzzyCompare(idealHeight, std::floor(idealHeight));
        QCOMPARE(actuallyFractional, productIsFractional);

        const QSize surfaceSize = TTextEdit::renderSurfaceSize(screenWidth, fontWidth, pixmapHeight, devicePixelRatio);
        QVERIFY2(!surfaceSize.isEmpty(), "the surface came out empty, so this row proves nothing");

        // The surface has to be at least the area it is asked to hold. Truncating
        // leaves it a fraction of a pixel short whenever the product is not whole,
        // and then no cache can satisfy a check written against the full area.
        const qreal idealWidth = screenWidth * fontWidth * devicePixelRatio;
        QVERIFY2(surfaceSize.width() >= idealWidth, qPrintable(QStringLiteral("surface width %1 is short of the %2 it must cover").arg(surfaceSize.width()).arg(idealWidth)));
        QVERIFY2(surfaceSize.height() >= idealHeight, qPrintable(QStringLiteral("surface height %1 is short of the %2 it must cover").arg(surfaceSize.height()).arg(idealHeight)));
    }
};

QTEST_GUILESS_MAIN(TTextEditPaintSurfaceTest)
#include "TTextEditPaintSurfaceTest.moc"
