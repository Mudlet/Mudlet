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

/*
 * When an update is found the toolbar's About button says so, but the
 * "Check for updates..." item in the menubar's About menu kept saying
 * "Check for updates..." (#5110). The toolbar button itself becomes a green "!"
 * and the word "Update", in place of the old colour picture.
 *
 * Run with: ctest -R AboutMenuReleaseNoticeTest -V
 */

#include <QAction>
#include <QDir>
#include <QImage>
#include <QTemporaryDir>
#include <QToolBar>
#include <QToolButton>
#include <QtTest/QtTest>

#include <memory>

#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "mudlet.h"
#include "uiDesign.h"
#if defined(INCLUDE_UPDATER)
#include "updater.h"
#endif

#include "GroupedTest.h"

class AboutMenuReleaseNoticeTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;

#if defined(INCLUDE_UPDATER)
    // By the actions on the bar: each check leaves the button before it hidden among the bar's children
    static QAction* updateAction()
    {
        QToolBar* pBar = mudlet::self()->mpMainToolBar;
        for (QAction* pAction : pBar->actions()) {
            if (const QWidget* pWidget = pBar->widgetForAction(pAction); pWidget && pWidget->objectName() == qsl("aboutUpdateButton")) {
                return pAction;
            }
        }
        return nullptr;
    }

    static QToolButton* updateButton()
    {
        QAction* pAction = updateAction();
        return pAction ? qobject_cast<QToolButton*>(mudlet::self()->mpMainToolBar->widgetForAction(pAction)) : nullptr;
    }

    // The most heavily inked pixel below the glyph, which is the middle of a stroke of the word. At its own size:
    // on a bar too narrow for it, the button sits in the overflow at the 640x480 of a widget never laid out
    static QColor wordInk(QToolButton* pButton, const QColor& page)
    {
        pButton->resize(pButton->sizeHint());
        const QImage image = pButton->grab().toImage().convertToFormat(QImage::Format_ARGB32);
        const auto distance = [](const QColor& a, const QColor& b) {
            return std::abs(a.red() - b.red()) + std::abs(a.green() - b.green()) + std::abs(a.blue() - b.blue());
        };
        QColor best = page;
        for (int y = image.height() * 2 / 3; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                const QColor colour = image.pixelColor(x, y);
                if (colour.alpha() > 127 && distance(colour, page) > distance(best, page)) {
                    best = colour;
                }
            }
        }
        return best;
    }
#endif

private slots:
    void initTestCase()
    {
#if !defined(INCLUDE_UPDATER)
        QSKIP("built without the updater");
#else
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }
        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>(qsl("MudletInstanceCoordinator")));
        mudlet::self()->init();
        QVERIFY(mudlet::self()->pUpdater);
#endif
    }

#if defined(INCLUDE_UPDATER)
    void test_menuItemAnnouncesAvailableUpdates()
    {
        auto* pMudlet = mudlet::self();
        QCOMPARE(pMudlet->dactionUpdate->text(), qsl("Check for updates..."));

        emit pMudlet->pUpdater->signal_updateAvailable(2);

        QCOMPARE(pMudlet->dactionUpdate->text(), mudlet::tr("Review %n update(s)...", nullptr, 2));
    }

    void test_toolbarButtonSaysUpdateInGreen()
    {
        auto* pMudlet = mudlet::self();
        pMudlet->show();
        QVERIFY(QTest::qWaitForWindowExposed(pMudlet));
        emit pMudlet->pUpdater->signal_updateAvailable(2);

        const auto glyphInUpdateInk = [](QToolButton* pButton) {
            const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
            const QColor ink = uiDesign::readableOn(tokens.page, uiDesign::stateColor(uiDesign::scmStateHue_update, tokens.darkPage), tokens.text, uiDesign::scmTextMinimumRatio);
            const QImage expected = uiDesign::tintedGlyph(uiDesign::glyphPixmap(qsl(":/icons/toolbar-about-update.svg")), ink).toImage().convertToFormat(QImage::Format_ARGB32);
            return !expected.isNull() && pButton->icon().pixmap(expected.size()).toImage().convertToFormat(QImage::Format_ARGB32) == expected;
        };
        // Before any appearance change, which draws the glyph again by another route
        QVERIFY2(updateButton() && glyphInUpdateInk(updateButton()), "the button a found update puts on the bar does not show the \"!\" in the update ink");

        for (const auto appearance : {enums::Appearance::dark, enums::Appearance::light}) {
            pMudlet->setAppearance(appearance);
            QCoreApplication::processEvents();
            QToolButton* pButton = updateButton();
            QVERIFY2(pButton, "no button named aboutUpdateButton is on the toolbar after an update was found");
            QCOMPARE(pButton->text(), mudlet::tr("Update"));

            const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
            const QColor ink = uiDesign::readableOn(tokens.page, uiDesign::stateColor(uiDesign::scmStateHue_update, tokens.darkPage), tokens.text, uiDesign::scmTextMinimumRatio);
            QVERIFY2(uiDesign::contrastRatio(ink, tokens.page) >= uiDesign::scmTextMinimumRatio, "the word cannot be read on the bar");

            QVERIFY2(glyphInUpdateInk(pButton), qPrintable(qsl("the button's glyph is not the \"!\" in %1").arg(ink.name())));

            // A blend of the word's ink with the grey bar keeps the ink's hue, and a grey word has none
            const QColor word = wordInk(pButton, tokens.page);
            QVERIFY2(std::abs(word.hueF() - ink.hueF()) < 0.03, qPrintable(qsl("the word is drawn in %1 where the ink is %2").arg(word.name(), ink.name())));

            const QString sheet = pMudlet->mpMainToolBar->styleSheet();
            QVERIFY2(sheet.contains(qsl("QToolButton#aboutUpdateButton:hover { color: %1; }").arg(ink.name())), "the word loses the ink under the pointer");
            QVERIFY2(sheet.contains(qsl("QToolButton#aboutUpdateButton::menu-indicator { image: none; }")), "Qt's menu arrow is still drawn over the word");
        }
        pMudlet->setAppearance(enums::Appearance::dark);
    }

    void test_aLaterCheckLeavesTheButtonWhereItWas()
    {
        auto* pMudlet = mudlet::self();
        emit pMudlet->pUpdater->signal_updateAvailable(1);
        emit pMudlet->pUpdater->signal_updateAvailable(3);

        const QList<QAction*> actions = pMudlet->mpMainToolBar->actions();
        const int position = actions.indexOf(updateAction());
        QVERIFY2(position >= 0, "no update button is on the toolbar");
        QVERIFY2(position + 1 < actions.size(), "a later check sent the button to the far end of the bar");
        QCOMPARE(actions.at(position + 1)->objectName(), qsl("fullscreen_action"));
        QVERIFY2(!actions.contains(pMudlet->findChild<QAction*>(qsl("about_action"))), "the plain About action is back on the bar beside the update button");
    }

    void test_buttonFollowsTheToolbarIconSize()
    {
        auto* pMudlet = mudlet::self();
        emit pMudlet->pUpdater->signal_updateAvailable(1);
        pMudlet->setToolBarIconSize(4);
        QCOMPARE(updateButton()->iconSize(), pMudlet->mpMainToolBar->iconSize());

        emit pMudlet->pUpdater->signal_updateAvailable(2);
        QCOMPARE(updateButton()->iconSize(), pMudlet->mpMainToolBar->iconSize());
        pMudlet->setToolBarIconSize(3);
    }

    // The feed keeps listing an installed update until Mudlet restarts into it
    void test_laterCheckKeepsRestartPrompt()
    {
        // Checked inside the slot: with some Qt installs moc does not see Q_OS_MACOS, so a guard around it leaves moc calling a slot the compiler never declares
#if defined(Q_OS_MACOS)
        QSKIP("Sparkle installs and relaunches updates itself on macOS, so the menu has no restart prompt");
#else
        auto* pMudlet = mudlet::self();
        emit pMudlet->pUpdater->signal_updateInstalled();
        QCOMPARE(pMudlet->dactionUpdate->text(), qsl("Update installed - restart to apply"));

        emit pMudlet->pUpdater->signal_updateAvailable(1);

        QCOMPARE(pMudlet->dactionUpdate->text(), qsl("Update installed - restart to apply"));
#endif
    }
#endif
};

#include "AboutMenuReleaseNoticeTest.moc"
MUDLET_GROUPED_TEST_MAIN(AboutMenuReleaseNoticeTest)
