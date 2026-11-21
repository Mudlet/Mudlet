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

#include <QStyleHints>
#include <QTreeWidget>
#include <QtTest/QtTest>

#include "DarkTheme.h"
#include "TConsoleModel.h"

#include "GroupedTest.h"

// Lua cannot see a widget's pixels, so the contrast is checked here rather than
// in a spec.
class DarkThemeItemCheckTest : public QObject
{
    Q_OBJECT

private:
    // The item has no text, so everything in its row that is not the row's own
    // background is the check indicator.
    static QRect indicatorRect(const QImage& shot, const QRect& row, const QColor& background)
    {
        QRect found;
        for (int y = row.top(); y <= row.bottom(); ++y) {
            for (int x = row.left(); x <= row.right(); ++x) {
                if (shot.pixelColor(x, y) != background) {
                    found |= QRect(x, y, 1, 1);
                }
            }
        }
        return found;
    }

    // Checks the weakest of the four sides, each sampled at its middle so an
    // antialiased corner cannot stand in for it.
    static void verifyOutlineStandsOut(const Qt::CheckState state, const bool selected, const QColor& foreground = QColor())
    {
        QTreeWidget tree;
        tree.setHeaderHidden(true);
        // Otherwise the current item's focus frame runs along the row as well
        tree.setFocusPolicy(Qt::NoFocus);
        tree.resize(200, 100);
        auto* pItem = new QTreeWidgetItem(&tree);
        pItem->setFlags(pItem->flags() | Qt::ItemIsUserCheckable);
        pItem->setCheckState(0, state);
        pItem->setSelected(selected);
        if (foreground.isValid()) {
            pItem->setForeground(0, foreground);
        }
        tree.show();
        QVERIFY(QTest::qWaitForWindowExposed(&tree));

        QImage shot(tree.viewport()->size(), QImage::Format_RGB32);
        shot.fill(tree.palette().color(QPalette::Base));
        tree.viewport()->render(&shot, QPoint(), QRegion(), QWidget::DrawChildren);

        const QRect row = tree.visualItemRect(pItem);
        const QColor background = shot.pixelColor(row.right(), row.center().y());
        QCOMPARE(background, tree.palette().color(selected ? QPalette::Highlight : QPalette::Base));
        const QRect indicator = indicatorRect(shot, row, background);
        QVERIFY2(indicator.width() >= 3 && indicator.height() >= 3, qPrintable(qsl("no check indicator was drawn, only %1x%2 pixels").arg(indicator.width()).arg(indicator.height())));

        const QPoint middle = indicator.center();
        const double contrast = std::min({TConsoleModel::contrastRatio(shot.pixelColor(indicator.left(), middle.y()), background),
                                          TConsoleModel::contrastRatio(shot.pixelColor(indicator.right(), middle.y()), background),
                                          TConsoleModel::contrastRatio(shot.pixelColor(middle.x(), indicator.top()), background),
                                          TConsoleModel::contrastRatio(shot.pixelColor(middle.x(), indicator.bottom()), background)});
        QVERIFY2(contrast >= 3.0, qPrintable(qsl("the check box outline was only %1:1 against the row").arg(contrast, 0, 'f', 2)));
    }

private slots:
    void initTestCase()
    {
        QGuiApplication::styleHints()->setColorScheme(Qt::ColorScheme::Dark);
        qApp->setStyle(new DarkTheme);
        QCOMPARE(qApp->palette().color(QPalette::Base), QColor(25, 25, 25));
    }

    void test_uncheckedItemOutlineIsVisible() { verifyOutlineStandsOut(Qt::Unchecked, false); }

    void test_checkedItemOutlineIsVisible() { verifyOutlineStandsOut(Qt::Checked, false); }

    // On Highlight rather than Base, which a lightened outline would fade into
    void test_selectedItemOutlineIsVisible() { verifyOutlineStandsOut(Qt::Checked, true); }

    // As the variables tree greys out the ones that cannot be saved
    void test_dimTextItemOutlineIsVisible() { verifyOutlineStandsOut(Qt::Unchecked, false, QColor(128, 128, 128)); }
};

#include "DarkThemeItemCheckTest.moc"
MUDLET_GROUPED_TEST_MAIN(DarkThemeItemCheckTest)
