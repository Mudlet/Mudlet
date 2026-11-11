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

#include <QToolBar>
#include <QToolButton>
#include <QtTest/QtTest>
#include <algorithm>
#include <cmath>

#include "DarkTheme.h"
#include "MudletProxyStyle.h"

#include "GroupedTest.h"

namespace {

double relativeLuminance(const QColor& colour)
{
    const auto channel = [](const double value) {
        return value <= 0.03928 ? value / 12.92 : std::pow((value + 0.055) / 1.055, 2.4);
    };
    return 0.2126 * channel(colour.redF()) + 0.7152 * channel(colour.greenF()) + 0.0722 * channel(colour.blueF());
}

double contrastRatio(const QColor& first, const QColor& second)
{
    const double a = relativeLuminance(first);
    const double b = relativeLuminance(second);
    return (std::max(a, b) + 0.05) / (std::min(a, b) + 0.05);
}

// A faint pixel is as hard to see as its colour blended into what lies under it
double bestContrastAgainst(const QImage& image, const QColor& background)
{
    double best = 1.0;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            const QColor pixel = image.pixelColor(x, y);
            const double alpha = pixel.alphaF();
            const QColor seen = QColor::fromRgbF(
                    pixel.redF() * alpha + background.redF() * (1.0 - alpha), pixel.greenF() * alpha + background.greenF() * (1.0 - alpha), pixel.blueF() * alpha + background.blueF() * (1.0 - alpha));
            best = std::max(best, contrastRatio(seen, background));
        }
    }
    return best;
}

void applyAppearance(const bool dark)
{
    if (dark) {
        // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDeleteLeaks)
        qApp->setStyle(new DarkTheme);
    } else {
        // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDeleteLeaks)
        qApp->setStyle(new MudletProxyStyle(qsl("Fusion")));
    }
}

int brightestLightness(const QPixmap& pixmap)
{
    const QImage image = pixmap.toImage().convertToFormat(QImage::Format_ARGB32);
    int brightest = 0;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            const QColor pixel = image.pixelColor(x, y);
            if (pixel.alpha() > 128) {
                brightest = std::max(brightest, pixel.lightness());
            }
        }
    }
    return brightest;
}

} // namespace

// The overflow button is the only way to reach the actions a short window
// leaves off a toolbar, so it has to be seen to be used.
class DarkThemeToolBarExtensionTest : public QObject
{
    Q_OBJECT

private slots:
    void overflowButtonStandsOutFromTheToolBar_data()
    {
        QTest::addColumn<Qt::Orientation>("orientation");
        QTest::addColumn<bool>("builtDark");
        QTest::addColumn<bool>("shownDark");
        QTest::newRow("dark, horizontal") << Qt::Horizontal << true << true;
        QTest::newRow("dark, vertical") << Qt::Vertical << true << true;
        QTest::newRow("light, horizontal") << Qt::Horizontal << false << false;
        QTest::newRow("built light, switched to dark") << Qt::Horizontal << false << true;
        QTest::newRow("built dark, switched to light") << Qt::Vertical << true << false;
    }

    void overflowButtonStandsOutFromTheToolBar()
    {
        QFETCH(Qt::Orientation, orientation);
        QFETCH(bool, builtDark);
        QFETCH(bool, shownDark);

        applyAppearance(builtDark);
        QToolBar toolBar;
        toolBar.setOrientation(orientation);
        toolBar.setToolButtonStyle(Qt::ToolButtonTextOnly);
        for (int i = 0; i < 10; ++i) {
            toolBar.addAction(qsl("Action %1").arg(i));
        }
        toolBar.resize(orientation == Qt::Horizontal ? QSize(120, 40) : QSize(40, 120));
        toolBar.show();
        QVERIFY(QTest::qWaitForWindowExposed(&toolBar));
        if (shownDark != builtDark) {
            applyAppearance(shownDark);
        }

        auto* pExtension = toolBar.findChild<QToolButton*>(qsl("qt_toolbar_ext_button"));
        QVERIFY(pExtension);
        QTRY_VERIFY(pExtension->isVisible());

        // The application palette reaches widgets through posted events
        QTRY_COMPARE(toolBar.palette().color(QPalette::Window).lightness() < 128, shownDark);
        const QColor background = toolBar.palette().color(QPalette::Window);
        const QImage image = pExtension->grab().toImage().convertToFormat(QImage::Format_ARGB32);
        const double contrast = bestContrastAgainst(image, background);
        QVERIFY2(contrast >= 3.0, qPrintable(qsl("the overflow button's best contrast against the toolbar is only %1:1").arg(contrast, 0, 'f', 2)));
    }

    // The button is disabled when everything it would offer is a widget, and
    // then must not look like it does something
    void disabledOverflowButtonLooksDisabled()
    {
        applyAppearance(true);
        const QIcon icon = qApp->style()->standardIcon(QStyle::SP_ToolBarHorizontalExtensionButton);
        const int enabled = brightestLightness(icon.pixmap(QSize(16, 16), 1.0));
        const int disabled = brightestLightness(icon.pixmap(QSize(16, 16), 1.0, QIcon::Disabled));
        QVERIFY2(enabled - disabled >= 64, qPrintable(qsl("enabled lightness %1, disabled %2").arg(enabled).arg(disabled)));
    }
};

#include "DarkThemeToolBarExtensionTest.moc"
MUDLET_GROUPED_TEST_MAIN(DarkThemeToolBarExtensionTest)
