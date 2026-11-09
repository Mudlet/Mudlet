/***************************************************************************
 *   Copyright (C) 2026 by Morquin - morquin@morquin.dk                    *
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

// Arabic and Hebrew turn the interface right to left, where Qt's own layouts mirror their items and these two have to
// as well. Neither needs a profile, so this links the two layouts alone.

#include "FlowLayout.h"
#include "PairGridLayout.h"

#include <QHash>
#include <QStyle>
#include <QtTest/QtTest>

// A fixed hint, so no font or style moves the numbers
class Block : public QWidget
{
public:
    Block(QWidget* pParent, const QSize& hint)
    : QWidget(pParent)
    , mHint(hint)
    {
    }
    QSize sizeHint() const override { return mHint; }

private:
    QSize mHint;
};

class LayoutMirrorTest : public QObject
{
    Q_OBJECT

private slots:
    void test_aFlowStartsAtTheRightEdgeRightToLeft()
    {
        QWidget row;
        auto* pFlow = new uiDesign::FlowLayout(&row, 10, 10);
        pFlow->setContentsMargins(0, 0, 0, 0);
        const QList<QWidget*> blocks{new Block(&row, {50, 20}), new Block(&row, {70, 20}), new Block(&row, {60, 20})};
        for (QWidget* pBlock : blocks) {
            pFlow->addWidget(pBlock);
        }
        row.resize(400, 200);
        // A layout skips the children of a window never shown, which count as hidden
        row.show();
        QVERIFY(QTest::qWaitForWindowExposed(&row));
        // Offset and narrower than the window, so mirroring around the window's rectangle would fail
        const QRect area(5, 0, 190, pFlow->heightForWidth(190));
        pFlow->setGeometry(area);
        QHash<QWidget*, QRect> leftToRight;
        for (QWidget* pBlock : blocks) {
            leftToRight.insert(pBlock, pBlock->geometry());
        }

        row.setLayoutDirection(Qt::RightToLeft);
        pFlow->setGeometry(area);
        QCOMPARE(blocks.at(0)->geometry().right(), area.right());
        QVERIFY2(blocks.at(1)->geometry().right() < blocks.at(0)->geometry().left(), "the second block is not to the left of the first");
        QVERIFY2(blocks.at(2)->geometry().top() > blocks.at(0)->geometry().bottom(), "SETUP: the third block did not wrap onto a line of its own");
        QCOMPARE(blocks.at(2)->geometry().right(), area.right());
        for (auto it = leftToRight.cbegin(); it != leftToRight.cend(); ++it) {
            QCOMPARE(it.key()->geometry(), QStyle::visualRect(Qt::RightToLeft, area, it.value()));
        }
    }

    void test_aPairGridLeadsFromTheRightEdgeRightToLeft()
    {
        QWidget grid;
        grid.setLayoutDirection(Qt::RightToLeft);
        auto* pGrid = new uiDesign::PairGridLayout(&grid, 6, 6);
        pGrid->setContentsMargins(0, 0, 0, 0);
        auto* pLabel1 = new Block(&grid, {40, 20});
        auto* pControl1 = new Block(&grid, {30, 20});
        auto* pLabel2 = new Block(&grid, {50, 20});
        auto* pControl2 = new Block(&grid, {30, 20});
        auto* pReset = new Block(&grid, {45, 20});
        pGrid->addPair(pLabel1, pControl1);
        pGrid->addPair(pLabel2, pControl2);
        pGrid->addFooter(nullptr, pReset);
        grid.resize(400, 200);
        grid.show();
        QVERIFY(QTest::qWaitForWindowExposed(&grid));

        // Two pairs to a row, with an odd pixel to share
        const int wide = pGrid->sizeHint().width() + 1;
        const QRect area(7, 0, wide, pGrid->heightForWidth(wide));
        pGrid->setGeometry(area);
        QVERIFY2(pLabel2->geometry().top() == pLabel1->geometry().top(), "SETUP: the two pairs are not on one row");
        QCOMPARE(pLabel1->geometry().right(), area.right());
        QVERIFY2(pControl1->geometry().right() < pLabel1->geometry().left(), "the first control is not to the left of its label");
        QVERIFY2(pLabel2->geometry().right() < pControl1->geometry().left(), "the second pair is not to the left of the first");
        QCOMPARE(pControl2->geometry().left(), area.left());
        QCOMPARE(pReset->geometry().left(), area.left());
        QCOMPARE(pReset->geometry().right(), pLabel2->geometry().right());

        // One pair to a row: the button keeps its width at the trailing edge, the left one
        const int narrow = pGrid->sizeHint().width() - 1;
        pGrid->setGeometry(QRect(7, 0, narrow, pGrid->heightForWidth(narrow)));
        QVERIFY2(pLabel2->geometry().top() > pLabel1->geometry().bottom(), "SETUP: the pairs still share a row");
        QCOMPARE(pLabel2->geometry().right(), 7 + narrow - 1);
        QCOMPARE(pReset->geometry().left(), 7);
        QCOMPARE(pReset->width(), 45);
    }
};

QTEST_MAIN(LayoutMirrorTest)
#include "LayoutMirrorTest.moc"
