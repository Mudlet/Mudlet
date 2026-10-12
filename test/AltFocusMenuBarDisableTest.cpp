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

#include "AltFocusMenuBarDisable.h"

#include <QPainter>
#include <QStyleOption>
#include <QtTest/QtTest>

/*
 * Mudlet's application style keeps the two Fusion title bar icons that a dock
 * widget's title paint measures, rather than letting Fusion load them from files
 * on every paint. The title must paint exactly as Fusion paints it, and every
 * other request for an icon must still get a new one from Fusion.
 */
class RecordingStyle : public AltFocusMenuBarDisable
{
public:
    using AltFocusMenuBarDisable::AltFocusMenuBarDisable;
    mutable QList<qint64> mCloseIconKeys;

    QIcon standardIcon(StandardPixmap standardIcon, const QStyleOption* option = nullptr, const QWidget* widget = nullptr) const override
    {
        const QIcon icon = AltFocusMenuBarDisable::standardIcon(standardIcon, option, widget);
        if (standardIcon == SP_TitleBarCloseButton) {
            mCloseIconKeys.append(icon.cacheKey());
        }
        return icon;
    }
};

class AltFocusMenuBarDisableTest : public QObject
{
    Q_OBJECT

    static QStyleOptionDockWidget titleOption(bool closable, bool floatable, bool vertical, Qt::LayoutDirection direction, const QString& title)
    {
        QStyleOptionDockWidget option;
        option.rect = vertical ? QRect(0, 0, 24, 300) : QRect(0, 0, 300, 24);
        option.title = title;
        option.closable = closable;
        option.floatable = floatable;
        option.verticalTitleBar = vertical;
        option.direction = direction;
        option.state = QStyle::State_Enabled;
        option.palette = QApplication::palette();
        option.fontMetrics = QFontMetrics(QApplication::font());
        return option;
    }

    static QImage paintTitle(const QStyle& style, const QStyleOptionDockWidget& option)
    {
        QImage image(option.rect.size(), QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);
        QPainter painter(&image);
        style.drawControl(QStyle::CE_DockWidgetTitle, &option, &painter);
        return image;
    }

private slots:
    void dockTitlesPaintAsFusionPaintsThem_data()
    {
        QTest::addColumn<bool>("closable");
        QTest::addColumn<bool>("floatable");
        QTest::addColumn<bool>("vertical");
        QTest::addColumn<int>("direction");
        QTest::addColumn<QString>("title");

        const QString longTitle = QStringLiteral("A title long enough that the buttons have to cut it short somewhere");
        for (const bool closable : {true, false}) {
            for (const bool floatable : {true, false}) {
                for (const bool vertical : {false, true}) {
                    for (const Qt::LayoutDirection direction : {Qt::LeftToRight, Qt::RightToLeft}) {
                        for (const QString& title : {QStringLiteral("Map - profile"), longTitle}) {
                            QTest::addRow("closable %d floatable %d vertical %d rtl %d %s", closable, floatable, vertical, direction == Qt::RightToLeft, title.size() > 20 ? "long" : "short")
                                    << closable << floatable << vertical << static_cast<int>(direction) << title;
                        }
                    }
                }
            }
        }
    }

    void dockTitlesPaintAsFusionPaintsThem()
    {
        QFETCH(bool, closable);
        QFETCH(bool, floatable);
        QFETCH(bool, vertical);
        QFETCH(int, direction);
        QFETCH(QString, title);

        const QStyleOptionDockWidget option = titleOption(closable, floatable, vertical, static_cast<Qt::LayoutDirection>(direction), title);
        AltFocusMenuBarDisable style(QStringLiteral("Fusion"));
        QProxyStyle fusion(QStyleFactory::create(QStringLiteral("Fusion")));
        const QImage expected = paintTitle(fusion, option);

        QCOMPARE(paintTitle(style, option), expected);
        // The kept icons have been measured by now; measuring them again changes nothing
        QCOMPARE(paintTitle(style, option), expected);
        QCOMPARE(style.subElementRect(QStyle::SE_DockWidgetTitleBarText, &option, nullptr), fusion.subElementRect(QStyle::SE_DockWidgetTitleBarText, &option, nullptr));
    }

    void dockTitlePaintsShareOneIcon()
    {
        RecordingStyle style(QStringLiteral("Fusion"));
        const QStyleOptionDockWidget option = titleOption(true, true, false, Qt::LeftToRight, QStringLiteral("Map - profile"));
        paintTitle(style, option);
        paintTitle(style, option);
        QVERIFY(style.mCloseIconKeys.size() >= 2);
        for (const qint64 key : std::as_const(style.mCloseIconKeys)) {
            QCOMPARE(key, style.mCloseIconKeys.constFirst());
        }
    }

    void dockTitlePaintsShareOneIconBehindAnotherProxy()
    {
        // As in dark mode, where DarkTheme wraps this style and Fusion's proxy() is the outer style
        auto* inner = new RecordingStyle(QStringLiteral("Fusion"));
        QProxyStyle outer(inner);
        const QStyleOptionDockWidget option = titleOption(true, true, false, Qt::LeftToRight, QStringLiteral("Map - profile"));
        paintTitle(outer, option);
        paintTitle(outer, option);
        QVERIFY(inner->mCloseIconKeys.size() >= 2);
        for (const qint64 key : std::as_const(inner->mCloseIconKeys)) {
            QCOMPARE(key, inner->mCloseIconKeys.constFirst());
        }
    }

    void otherRequestsGetANewIcon()
    {
        RecordingStyle style(QStringLiteral("Fusion"));
        const QStyleOptionDockWidget option = titleOption(true, true, false, Qt::LeftToRight, QStringLiteral("Map - profile"));
        paintTitle(style, option);
        const qint64 keptKey = style.mCloseIconKeys.constFirst();

        const QIcon first = style.standardIcon(QStyle::SP_TitleBarCloseButton);
        const QIcon second = style.standardIcon(QStyle::SP_TitleBarCloseButton, &option);
        QVERIFY(first.cacheKey() != keptKey);
        QVERIFY(second.cacheKey() != keptKey);
        QVERIFY(second.cacheKey() != first.cacheKey());
    }

    void otherBaseStylesAreLeftAlone()
    {
        AltFocusMenuBarDisable style(QStringLiteral("Windows"));
        if (style.baseStyle()->name().compare(QLatin1String("windows"), Qt::CaseInsensitive)) {
            QSKIP("the Windows style is not available");
        }
        QProxyStyle windows(QStyleFactory::create(QStringLiteral("Windows")));
        const QStyleOptionDockWidget option = titleOption(true, true, false, Qt::LeftToRight, QStringLiteral("Map - profile"));
        QCOMPARE(paintTitle(style, option), paintTitle(windows, option));
    }
};

QTEST_MAIN(AltFocusMenuBarDisableTest)
#include "AltFocusMenuBarDisableTest.moc"
