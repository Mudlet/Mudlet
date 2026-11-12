/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Developers - mudlet@mudlet.org           *
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

#include <QAbstractItemView>
#include <QComboBox>
#include <QScrollBar>
#include <QWheelEvent>
#include <QtTest/QtTest>

#include "DarkTheme.h"
#include "MudletProxyStyle.h"
#include "utils.h"

#include "GroupedTest.h"

class ComboListWheelTest : public QObject
{
    Q_OBJECT

    static void applyAppearance(const bool dark)
    {
        if (dark) {
            // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDeleteLeaks)
            qApp->setStyle(new DarkTheme);
        } else {
            // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDeleteLeaks)
            qApp->setStyle(new MudletProxyStyle(qsl("Fusion")));
        }
    }

private slots:
    void theHighlightFollowsThePointerAsTheWheelScrolls_data()
    {
        QTest::addColumn<bool>("dark");
        QTest::newRow("light") << false;
        QTest::newRow("dark") << true;
    }

    void theHighlightFollowsThePointerAsTheWheelScrolls()
    {
        QFETCH(bool, dark);
        applyAppearance(dark);
        QComboBox comboBox;
        for (int i = 0; i < 60; ++i) {
            comboBox.addItem(qsl("Item %1").arg(i));
        }
        comboBox.setMaxVisibleItems(10);
        comboBox.show();
        QVERIFY(QTest::qWaitForWindowExposed(&comboBox));
        comboBox.showPopup();
        QAbstractItemView* pList = comboBox.view();
        QTRY_VERIFY(pList->isVisible());
        QVERIFY2(comboBox.style()->styleHint(QStyle::SH_ComboBox_ListMouseTracking, nullptr, &comboBox), "SETUP: this style does not move the highlight with the pointer");

        // The pointer rests on the third row, which its last move highlighted
        const QModelIndex pointedAt = pList->model()->index(2, 0);
        const QPoint pointer = pList->visualRect(pointedAt).center();
        pList->setCurrentIndex(pointedAt);

        QWheelEvent wheel(pointer, pList->viewport()->mapToGlobal(pointer), QPoint(), QPoint(0, -120), Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
        QApplication::sendEvent(pList->viewport(), &wheel);
        QVERIFY2(pList->verticalScrollBar()->value() > 0, "SETUP: the wheel did not scroll the list");
        const QModelIndex underThePointer = pList->indexAt(pointer);
        QVERIFY2(underThePointer.isValid() && underThePointer != pointedAt, "SETUP: the scroll left the same item under the pointer");
        QTRY_COMPARE(pList->currentIndex(), underThePointer);
        comboBox.hidePopup();
    }
};

#include "ComboListWheelTest.moc"
MUDLET_GROUPED_TEST_MAIN(ComboListWheelTest)
