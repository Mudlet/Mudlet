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

#include <QDir>
#include <QProxyStyle>
#include <QScopeGuard>
#include <QStyleFactory>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <memory>

#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "mudlet.h"

#include "GroupedTest.h"

// On macOS and Windows both appearances are drawn on Fusion, so a dialog keeps its sizes and spacing when the
// appearance changes, unless the player chose a style
class AppearanceBaseStyleTest : public QObject
{
    Q_OBJECT

    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;

    // DarkTheme stands over a MudletProxyStyle, which stands over the base
    static QString baseStyleName()
    {
        const QStyle* pStyle = QApplication::style();
        while (const auto* pProxy = qobject_cast<const QProxyStyle*>(pStyle)) {
            pStyle = pProxy->baseStyle();
        }
        return pStyle->name().toLower();
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }
        // Stands in for the platform style, which is Fusion already under the offscreen platform
        QStyle* pPlatformStyle = QStyleFactory::create(qsl("Windows"));
        QVERIFY2(pPlatformStyle, "SETUP: this Qt has no Windows style to stand in for the platform's");
        QApplication::setStyle(pPlatformStyle);

        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>(qsl("MudletInstanceCoordinator")));
        mudlet::self()->init();
    }

    void cleanupTestCase()
    {
        delete mudlet::self();
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    // Can only fail on macOS and Windows; Linux keeps the recorded style for the light appearance
    void test_theLightAppearanceIsDrawnOnTheDarkOnesBaseStyle()
    {
        mudlet::self()->setAppearance(enums::Appearance::dark);
        const QString dark = baseStyleName();
        mudlet::self()->setAppearance(enums::Appearance::light);
        const QString light = baseStyleName();
        QCOMPARE(dark, qsl("fusion"));
#if defined(Q_OS_MACOS) || defined(Q_OS_WINDOWS)
        QCOMPARE(light, dark);
#else
        // The desktop's own style is kept for the light appearance
        QCOMPARE(light, qsl("windows"));
#endif
    }

    // The Windows style from initTestCase stands in for one given with -style
    void test_aStyleThePlayerChoseIsKeptForTheLightAppearance()
    {
        mudlet::smStyleChosenByPlayer = true;
        auto reset = qScopeGuard([]() {
            mudlet::smStyleChosenByPlayer = false;
        });

        mudlet::self()->setAppearance(enums::Appearance::dark);
        const QString dark = baseStyleName();
        mudlet::self()->setAppearance(enums::Appearance::light);
        const QString light = baseStyleName();
        QCOMPARE(dark, qsl("fusion"));
        QCOMPARE(light, qsl("windows"));
    }
};

#include "AppearanceBaseStyleTest.moc"
MUDLET_GROUPED_TEST_MAIN(AppearanceBaseStyleTest)
