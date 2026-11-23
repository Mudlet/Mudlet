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

/*
 * Every entry of the main window's menu bar carries a glyph drawn by the toolbar's recipe: an
 * entry that does what a toolbar button does carries that button's picture, in colour by default
 * and grey once the icons are set to grayscale, and an unavailable one takes the menu's disabled
 * ink rather than the bar's. A switched-on entry shows its glyph where the tick would be, so it
 * has to be told from one that is off by something else, under a script's sheet too. Under the
 * pointer the glyph takes the ink of the menus' own palette, or stays as at rest while a script's
 * sheet, whose colours for that row are unknown, styles the menus. Changing "Show icons on menus"
 * tells every entry, so a native menu takes or drops its icon at once.
 *
 * Run with: ctest -R MenuBarGlyphTest -V
 */

#include <QAction>
#include <QApplication>
#include <QImage>
#include <QMenu>
#include <QMenuBar>
#include <QPalette>
#include <QScopeGuard>
#include <QSet>
#include <QSignalBlocker>
#include <QtTest/QtTest>

#include "PixelTestHelper.h"
#include "ProfileSessionTestHelper.h"
#include "mudlet.h"
#include "uiDesign.h"
#if defined(Q_OS_MACOS)
#include "MacMenuImageVisibility.h"
#endif

#include "GroupedTest.h"

class MenuBarGlyphTest : public QObject
{
    Q_OBJECT

private:
    TestProfile::Session mSession;
    const QString mHostname = qsl("MenuBarGlyph");

    static constexpr int scmComparisonSize = 24;
    // A wash is composited rather than blitted, so it agrees with the token mix to a level or two
    static constexpr int scmInkTolerance = 2;
    // Summed over the channels: a line about a pixel wide is scaled down onto the row, so no pixel of it is solid ink
    static constexpr int scmDrawnInkTolerance = 60;
    // Clear of the cursor, which offscreen rests at the top left and would light a row it lands on
    static constexpr QPoint scmMenuAt{300, 100};

    static QList<QMenu*> menuBarMenus()
    {
        mudlet* pMudlet = mudlet::self();
        return {pMudlet->menuGames, pMudlet->menuEditor, pMudlet->menuOptions, pMudlet->menuWindow, pMudlet->menuHelp, pMudlet->menuAbout};
    }

    static QString saidAs(const QAction* pAction) { return qsl("%1 (\"%2\")").arg(pAction->objectName(), pAction->text()); }

    static QImage drawnAt(const QIcon& icon, const QIcon::Mode mode, const QIcon::State state)
    {
        return icon.pixmap(QSize(scmComparisonSize, scmComparisonSize), mode, state).toImage().convertToFormat(QImage::Format_ARGB32);
    }

    // TestPixels::inkOf() cannot report a difference in shape alone between two glyphs of one ink
    static int pixelsApart(const QImage& first, const QImage& second)
    {
        if (first.size() != second.size()) {
            return first.width() * first.height();
        }
        int apart = 0;
        for (int y = 0; y < first.height(); ++y) {
            for (int x = 0; x < first.width(); ++x) {
                apart += first.pixel(x, y) != second.pixel(x, y) ? 1 : 0;
            }
        }
        return apart;
    }

    static int inkedPixels(const QImage& image)
    {
        int inked = 0;
        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                inked += qAlpha(image.pixel(x, y)) > 32 ? 1 : 0;
            }
        }
        return inked;
    }

    // The strongest pixel of the glyph, whose antialiased edge is lighter than its ink
    static QColor strongestInk(const QImage& image)
    {
        QColor strongest;
        int strongestAlpha = 0;
        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                if (const QColor pixel = image.pixelColor(x, y); pixel.alpha() > strongestAlpha) {
                    strongestAlpha = pixel.alpha();
                    strongest = pixel;
                }
            }
        }
        return strongest;
    }

    // The most saturated of the glyph's solid pixels
    struct Hue
    {
        qreal hue = -1.0;
        qreal saturation = 0.0;
    };
    static Hue mostSaturated(const QImage& image)
    {
        Hue seen;
        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                const QColor pixel = image.pixelColor(x, y);
                if (pixel.alphaF() > 0.95 && pixel.hslSaturationF() > seen.saturation) {
                    seen.saturation = pixel.hslSaturationF();
                    seen.hue = pixel.hslHueF();
                }
            }
        }
        return seen;
    }

    static QString appearanceSaidAs(const enums::Appearance appearance) { return appearance == enums::Appearance::dark ? qsl("dark") : qsl("light"); }

    // Popped up rather than grabbed while hidden: a menu lays its rows out as it is shown
    static QImage shotOf(QMenu* pMenu, QAction* pUnderThePointer = nullptr)
    {
        pMenu->popup(scmMenuAt);
        if (!QTest::qWaitForWindowExposed(pMenu)) {
            return QImage();
        }
        if (pUnderThePointer) {
            pMenu->setActiveAction(pUnderThePointer);
        }
        const QImage shot = TestPixels::shot(pMenu);
        pMenu->hide();
        QCoreApplication::processEvents();
        return shot;
    }

    // From the row's leading edge to well past the glyph, the whole of the column a tick would take
    static QRect leadingColumnOf(const QRect& row) { return QRect(row.left(), row.top(), qMin(row.width(), 2 * row.height()), row.height()); }

    static int differingPixels(const QImage& first, const QImage& second, const QRect& within)
    {
        int differing = 0;
        for (int y = within.top(); y <= within.bottom(); ++y) {
            for (int x = within.left(); x <= within.right(); ++x) {
                const QPoint at(x, y);
                differing += TestPixels::sampled(first, at) != TestPixels::sampled(second, at) ? 1 : 0;
            }
        }
        return differing;
    }

    static int pixelsReading(const QImage& shot, const QRect& within, const QColor& wanted)
    {
        int reading = 0;
        for (int y = within.top(); y <= within.bottom(); ++y) {
            for (int x = within.left(); x <= within.right(); ++x) {
                const QColor pixel = TestPixels::sampled(shot, QPoint(x, y));
                reading += pixel.isValid() && TestPixels::readsAs(pixel, wanted, scmInkTolerance) ? 1 : 0;
            }
        }
        return reading;
    }

    // Those taking the colour between the two shots, so a glyph already in it is not counted
    static int pixelsTurning(const QImage& before, const QImage& after, const QRect& within, const QColor& wanted)
    {
        int turning = 0;
        for (int y = within.top(); y <= within.bottom(); ++y) {
            for (int x = within.left(); x <= within.right(); ++x) {
                const QColor was = TestPixels::sampled(before, QPoint(x, y));
                const QColor is = TestPixels::sampled(after, QPoint(x, y));
                turning += is.isValid() && TestPixels::readsAs(is, wanted, scmInkTolerance) && !TestPixels::readsAs(was, wanted, scmInkTolerance) ? 1 : 0;
            }
        }
        return turning;
    }

    // On its own, a sheet that colours only the row under the pointer leaves the rows to the base style; one giving the rows a box
    // has every row drawn by Qt's stylesheet style (QStyleSheetStyle::drawControl(), CE_MenuItem)
    static QString hoveredRowSheet() { return qsl("QMenu::item:selected { background-color: #444444; }"); }
    static QString paddedRowsSheet() { return qsl("QMenu { background: #222; color: #ddd; } QMenu::item { padding: 4px 24px; } QMenu::item:selected { background: #444; }"); }

    static QColor scriptsWash() { return QColor(200, 40, 40); }

    static void addBothRoads(const QString& what, const QString& sheet)
    {
        QTest::addRow("setAppStyleSheet, %s", qPrintable(what)) << true << sheet;
        QTest::addRow("setProfileStyleSheet, %s", qPrintable(what)) << false << sheet;
    }

    // The two roads a script's sheet reaches the menu bar's menus by: the application's, and the profile's, which is
    // set on the menu bar
    static void useScriptSheet(const bool application, const QString& sheet)
    {
        if (application) {
            mudlet::self()->setAppStyleSheet(sheet);
        } else {
            mudlet::self()->getActiveHost()->setProfileStyleSheet(sheet);
        }
        QCoreApplication::processEvents();
    }

    static void dropScriptSheets()
    {
        mudlet::self()->setAppStyleSheet(QString());
        mudlet::self()->getActiveHost()->setProfileStyleSheet(QString());
        QCoreApplication::processEvents();
    }

    static QList<QAction*> switchesOfTheMenuBar()
    {
        QList<QAction*> switches;
        for (QMenu* pMenu : menuBarMenus()) {
            for (QAction* pAction : pMenu->actions()) {
                if (pAction->isCheckable()) {
                    switches << pAction;
                }
            }
        }
        return switches;
    }

    static QMenu* menuHolding(QAction* pAction)
    {
        for (QMenu* pMenu : menuBarMenus()) {
            if (pMenu->actions().contains(pAction)) {
                return pMenu;
            }
        }
        return nullptr;
    }

    // What the base style draws the row under the pointer in, read from the palette the menus are drawn with
    static QColor chosenRowInk(const QPalette& palette)
    {
        const QColor words = palette.color(QPalette::HighlightedText);
        return uiDesign::readableOn(palette.color(QPalette::Highlight), words, words, uiDesign::scmQuietMinimumRatio);
    }

private slots:
    void initTestCase()
    {
        if (TestProfile::Session::configCannotBeRedirected()) {
            QSKIP(TestProfile::Session::configCannotBeRedirectedReason());
        }

        QVERIFY2(mSession.startMudlet(), qPrintable(mSession.why()));
        mudlet::self()->setInterfaceLanguage(qsl("en_US"));
        // One, so the Window menu holds no list of profiles, whose rows carry a tick rather than a glyph
        QVERIFY2(mSession.openProfile(mHostname), qPrintable(mSession.why()));
        mudlet::self()->resize(1200, 600);
        mudlet::self()->show();
        QVERIFY(QTest::qWaitForWindowExposed(mudlet::self()));
    }

    void cleanupTestCase()
    {
        if (mudlet::self()) {
            mudlet::self()->setAppearance(enums::Appearance::systemSetting);
        }
        mSession.stop();
    }

    // Hidden ones too: Check for updates, Show changelog and Report an issue show only in some builds
    void test_everyEntryOfTheMenuBarCarriesAGlyph()
    {
        QStringList bare;
        int entries = 0;
        for (QMenu* pMenu : menuBarMenus()) {
            QVERIFY2(pMenu, "the main window is missing one of its six menu bar menus");
            for (QAction* pAction : pMenu->actions()) {
                if (pAction->isSeparator()) {
                    continue;
                }
                ++entries;
                if (pAction->icon().isNull() || inkedPixels(drawnAt(pAction->icon(), QIcon::Normal, QIcon::Off)) == 0) {
                    bare << saidAs(pAction);
                }
            }
        }
        qInfo().noquote() << qsl("  %1 entries in the six menus, %2 of them without a glyph").arg(QString::number(entries), QString::number(bare.size()));

        QVERIFY2(entries > 0, "the six menus hold no entries at all, so nothing below was checked");
        QVERIFY2(bare.isEmpty(), qPrintable(qsl("these menu bar entries have no picture: %1").arg(bare.join(qsl(", ")))));
    }

    // At rest only: unavailable and under the pointer, a menu's row is drawn on another surface than a bar's button
    void test_anEntryWithAToolBarTwinCarriesItsPicture()
    {
        mudlet* pMudlet = mudlet::self();
        const QList<std::pair<QAction*, QString>> twins{{pMudlet->dactionConnect, qsl("connect")},
                                                        {pMudlet->dactionDisconnect, qsl("disconnect")},
                                                        {pMudlet->dactionReconnect, qsl("reconnect_action")},
                                                        {pMudlet->dactionCloseProfile, qsl("close_profile")},
                                                        {pMudlet->dactionCloseApplication, qsl("close_application")},
                                                        {pMudlet->dactionShowMap, qsl("map_action")},
                                                        {pMudlet->dactionNotepad, qsl("notepad_action")},
                                                        {pMudlet->dactionPackageManager, qsl("package_manager")},
                                                        {pMudlet->dactionReplay, qsl("replay_action")},
                                                        {pMudlet->dactionModuleManager, qsl("module_manager")},
                                                        {pMudlet->dactionPackageExporter, qsl("package_exporter")},
                                                        {pMudlet->dactionOptions, qsl("settings_action")},
                                                        {pMudlet->dactionMuteMedia, qsl("muteMedia")},
                                                        {pMudlet->dactionMuteAPI, qsl("muteAPI")},
                                                        {pMudlet->dactionMuteGame, qsl("muteGame")},
                                                        {pMudlet->dactionToggleFullScreen, qsl("fullscreen_action")},
                                                        {pMudlet->dactionMultiView, qsl("multiview_action")},
                                                        {pMudlet->dactionHelp, qsl("manual_action")},
                                                        {pMudlet->dactionDiscord, qsl("openDiscord")},
                                                        {pMudlet->dactionAbout, qsl("about_action")}};
        auto restore = qScopeGuard([]() {
            TestPixels::setAppearanceAndWait(enums::Appearance::systemSetting);
        });

        // Both, so a menu entry left out of an appearance change's restyle keeps the other one's ink and fails here
        for (const enums::Appearance appearance : {enums::Appearance::dark, enums::Appearance::light}) {
            TestPixels::setAppearanceAndWait(appearance);
            const QString said = appearanceSaidAs(appearance);
            for (const auto& [pEntry, twinName] : twins) {
                const QAction* pTwin = pMudlet->findChild<QAction*>(twinName);
                QVERIFY2(pTwin, qPrintable(qsl("the toolbar has no action named %1 to compare %2 with").arg(twinName, saidAs(pEntry))));
                for (const QIcon::State state : {QIcon::Off, QIcon::On}) {
                    const QImage inTheMenu = drawnAt(pEntry->icon(), QIcon::Normal, state);
                    const QImage onTheBar = drawnAt(pTwin->icon(), QIcon::Normal, state);
                    QVERIFY2(inTheMenu == onTheBar,
                             qPrintable(qsl("%1: %2 %3 is not the picture of the toolbar's %4: inked %5 where the toolbar's is %6, and %7 pixels differ")
                                                .arg(said,
                                                     saidAs(pEntry),
                                                     state == QIcon::On ? qsl("switched on") : qsl("switched off"),
                                                     twinName,
                                                     TestPixels::inkOf(inTheMenu),
                                                     TestPixels::inkOf(onTheBar),
                                                     QString::number(pixelsApart(inTheMenu, onTheBar)))));
                }
            }
        }
    }

    // The bar's disabled ink is walked on the page alone; a menu's is walked on the card and the field as well
    void test_anUnavailableEntryTakesTheMenusDisabledInk()
    {
        mudlet* pMudlet = mudlet::self();
        const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
        // One coloured entry and one grey one, as the two are built by different recipes
        for (const QAction* pEntry : {pMudlet->dactionConnect, pMudlet->dactionOptions}) {
            const QColor ink = strongestInk(drawnAt(pEntry->icon(), QIcon::Disabled, QIcon::Off));
            qInfo().noquote() << qsl("  unavailable, %1 is inked %2 against the menu's disabled ink %3 and the bar's %4")
                                         .arg(saidAs(pEntry), ink.name(), tokens.disabledText.name(), tokens.disabledBarText.name());
            QVERIFY2(TestPixels::readsAs(ink, tokens.disabledText, scmInkTolerance),
                     qPrintable(qsl("unavailable, %1 is inked %2 rather than the menu's disabled ink %3").arg(saidAs(pEntry), ink.name(), tokens.disabledText.name())));
        }
    }

    // The base style fills the row under the pointer with Highlight and writes its words in HighlightedText; Fusion asks
    // the icon for Active there, Breeze for Selected while the row is pressed
    void test_theRowUnderThePointerInksItsGlyphAsItsWords()
    {
        mudlet* pMudlet = mudlet::self();
        const bool iconsHidden = qApp->testAttribute(Qt::AA_DontShowIconsInMenus);
        qApp->setAttribute(Qt::AA_DontShowIconsInMenus, false);
        auto restore = qScopeGuard([iconsHidden]() {
            qApp->setAttribute(Qt::AA_DontShowIconsInMenus, iconsHidden);
            mudlet::self()->setColouredIcons(true);
            TestPixels::setAppearanceAndWait(enums::Appearance::systemSetting);
        });

        for (const enums::Appearance appearance : {enums::Appearance::dark, enums::Appearance::light}) {
            TestPixels::setAppearanceAndWait(appearance);
            for (const bool coloured : {true, false}) {
                pMudlet->setColouredIcons(coloured);
                const QString said = qsl("%1, %2").arg(appearanceSaidAs(appearance), coloured ? qsl("in colour") : qsl("grey"));
                const QColor highlight = QApplication::palette().color(QPalette::Highlight);
                const QColor words = QApplication::palette().color(QPalette::HighlightedText);
                const QColor wanted = uiDesign::readableOn(highlight, words, words, uiDesign::scmQuietMinimumRatio);

                QStringList astray;
                for (QMenu* pMenu : menuBarMenus()) {
                    for (QAction* pAction : pMenu->actions()) {
                        if (pAction->isSeparator()) {
                            continue;
                        }
                        for (const QIcon::Mode mode : {QIcon::Active, QIcon::Selected}) {
                            for (const QIcon::State state : {QIcon::Off, QIcon::On}) {
                                if (const QColor ink = strongestInk(drawnAt(pAction->icon(), mode, state)); !TestPixels::readsAs(ink, wanted, scmInkTolerance)) {
                                    astray << qsl("%1 %2 %3 in %4")
                                                      .arg(saidAs(pAction), mode == QIcon::Active ? qsl("Active") : qsl("Selected"), state == QIcon::On ? qsl("On") : qsl("Off"), ink.name());
                                }
                            }
                        }
                    }
                }

                // Read off the menu as the base style draws it, so a style asking for another mode would show here
                QMenu* pMenu = pMudlet->menuGames;
                QAction* pEntry = pMudlet->dactionConnect;
                pMenu->popup(scmMenuAt);
                QVERIFY(QTest::qWaitForWindowExposed(pMenu));
                pMenu->setActiveAction(pEntry);
                const QImage shot = TestPixels::shot(pMenu);
                pMenu->hide();
                QCoreApplication::processEvents();
                const QRect row = pMenu->actionGeometry(pEntry);
                // Inside the fill's outline, so neither it nor the menu's own surface round the row is read
                const QRect glyphColumn = QRect(row.left(), row.top(), row.height(), row.height()).adjusted(2, 2, -2, -2);
                QColor nearest;
                for (int y = glyphColumn.top(); y <= glyphColumn.bottom(); ++y) {
                    for (int x = glyphColumn.left(); x <= glyphColumn.right(); ++x) {
                        const QColor pixel = TestPixels::sampled(shot, QPoint(x, y));
                        if (pixel.isValid() && (!nearest.isValid() || TestPixels::distance(pixel, wanted) < TestPixels::distance(nearest, wanted))) {
                            nearest = pixel;
                        }
                    }
                }
                const qreal ratio = uiDesign::contrastRatio(wanted, highlight);
                const qreal wordsRatio = uiDesign::contrastRatio(words, highlight);
                qInfo().noquote() << qsl("  %1: on Highlight %2 the glyph is inked %3 at %4:1 (the words' %5 at %6:1); the hovered Play row's nearest pixel to it is %7")
                                             .arg(said, highlight.name(), wanted.name(), QString::number(ratio, 'f', 2), words.name(), QString::number(wordsRatio, 'f', 2))
                                             .arg(nearest.name());

                QVERIFY2(astray.isEmpty(), qPrintable(qsl("%1: under the pointer these glyphs are not in the row's ink %2: %3").arg(said, wanted.name(), astray.join(qsl(", ")))));
                QVERIFY2(nearest.isValid() && TestPixels::distance(nearest, wanted) <= scmDrawnInkTolerance,
                         qPrintable(qsl("%1: the hovered Play row shows no glyph in the row's ink %2; the nearest pixel in its glyph column at %3 is %4")
                                            .arg(said, wanted.name(), TestPixels::describe(glyphColumn), nearest.name())));
                // The glyph is a line, held to the floor identityInk() holds one to, and never fainter than the words beside it
                QVERIFY2(ratio >= uiDesign::scmQuietMinimumRatio && ratio >= wordsRatio,
                         qPrintable(qsl("%1: the glyph's ink %2 reads at %3:1 on Highlight %4, where the row's words read at %5:1")
                                            .arg(said, wanted.name(), QString::number(ratio, 'f', 2), highlight.name(), QString::number(wordsRatio, 'f', 2))));
            }
        }
    }

    void test_grayscaleIconsLeaveEveryEntryGrey()
    {
        mudlet* pMudlet = mudlet::self();
        auto restore = qScopeGuard([]() {
            mudlet::self()->setColouredIcons(true);
            TestPixels::setAppearanceAndWait(enums::Appearance::systemSetting);
        });

        for (const enums::Appearance appearance : {enums::Appearance::dark, enums::Appearance::light}) {
            TestPixels::setAppearanceAndWait(appearance);
            const QString said = appearanceSaidAs(appearance);

            pMudlet->setColouredIcons(false);
            QStringList coloured;
            for (QMenu* pMenu : menuBarMenus()) {
                for (QAction* pAction : pMenu->actions()) {
                    if (!pAction->isSeparator() && mostSaturated(drawnAt(pAction->icon(), QIcon::Normal, QIcon::Off)).saturation >= 0.1) {
                        coloured << saidAs(pAction);
                    }
                }
            }
            QVERIFY2(coloured.isEmpty(), qPrintable(qsl("%1: with the icons set to grayscale these entries are still in colour: %2").arg(said, coloured.join(qsl(", ")))));

            pMudlet->setColouredIcons(true);
            for (const auto& [pEntry, identity] :
                 QList<std::pair<QAction*, uiDesign::IdentityColour>>{{pMudlet->dactionConnect, uiDesign::scmIdentity_connect}, {pMudlet->dactionMudletDiscord, uiDesign::scmIdentity_discord}}) {
                const Hue read = mostSaturated(drawnAt(pEntry->icon(), QIcon::Normal, QIcon::Off));
                QVERIFY2(read.saturation > 0.3 && std::abs(read.hue - identity.hue) < 0.03,
                         qPrintable(qsl("%1: in colour, %2 is hue %3 at saturation %4 rather than the hue %5 of its picture")
                                            .arg(said, saidAs(pEntry), QString::number(read.hue), QString::number(read.saturation), QString::number(identity.hue))));
            }
            QVERIFY2(mostSaturated(drawnAt(pMudlet->dactionOptions->icon(), QIcon::Normal, QIcon::Off)).saturation < 0.1,
                     qPrintable(qsl("%1: Preferences took a colour, though the toolbar's Settings is grey").arg(said)));
            QVERIFY2(drawnAt(pMudlet->dactionMudletDiscord->icon(), QIcon::Normal, QIcon::Off) != drawnAt(pMudlet->dactionDiscord->icon(), QIcon::Normal, QIcon::Off),
                     qPrintable(qsl("%1: the two Discord entries of the Help menu carry the same picture").arg(said)));
        }
    }

    // As the menu bar's menus are drawn today: by the base style, with no sheet of the design's
    void test_aSwitchedOnEntryIsToldFromOneThatIsOff()
    {
        QAction* pEntry = mudlet::self()->dactionInputLine;
        QMenu* pMenu = mudlet::self()->menuEditor;
        QVERIFY2(pEntry->isCheckable() && pEntry->isEnabled(), "Compact input line is not a switch that can be used, so there is no switched-on entry to read");
        const bool iconsHidden = qApp->testAttribute(Qt::AA_DontShowIconsInMenus);
        const bool wasChecked = pEntry->isChecked();
        qApp->setAttribute(Qt::AA_DontShowIconsInMenus, false);
        auto restore = qScopeGuard([pEntry, iconsHidden, wasChecked]() {
            pEntry->setChecked(wasChecked);
            qApp->setAttribute(Qt::AA_DontShowIconsInMenus, iconsHidden);
        });
        // Coloured, the glyph is the same picture either way, so whatever differs is the state drawn round it
        QVERIFY2(drawnAt(pEntry->icon(), QIcon::Normal, QIcon::Off) == drawnAt(pEntry->icon(), QIcon::Normal, QIcon::On),
                 "Compact input line's glyph changes with its state, so this case would pass on the glyph alone");

        pEntry->setChecked(false);
        const QImage off = shotOf(pMenu);
        pEntry->setChecked(true);
        const QImage on = shotOf(pMenu);
        QVERIFY2(!off.isNull() && !on.isNull(), "the Toolbox menu never came up to be grabbed");

        const QRect column = leadingColumnOf(pMenu->actionGeometry(pEntry));
        const int differing = differingPixels(off, on, column);
        qInfo().noquote()
                << qsl("  %1 of the %2 pixels of Compact input line's leading column change as it is switched on").arg(QString::number(differing), QString::number(column.width() * column.height()));
        QVERIFY2(differing > column.height(),
                 qPrintable(qsl("Compact input line looks the same switched on as off: %1 pixels of its leading column at %2 differ").arg(QString::number(differing), TestPixels::describe(column))));
    }

    // Any menu drawn by menuStyleSheet() - the toolbar's overflow menu holds Full Screen and MultiView
    void test_aSwitchedOnEntryOnTheDesignsMenuIsWashed()
    {
        const bool iconsHidden = qApp->testAttribute(Qt::AA_DontShowIconsInMenus);
        qApp->setAttribute(Qt::AA_DontShowIconsInMenus, false);
        auto restore = qScopeGuard([iconsHidden]() {
            qApp->setAttribute(Qt::AA_DontShowIconsInMenus, iconsHidden);
        });
        const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();
        QMenu menu;
        menu.setStyleSheet(uiDesign::menuStyleSheet(tokens));
        uiDesign::letPopupsTakeTheFieldsCorner(&menu);
        const QString glyphFile = qsl(":/icons/settings-input.svg");
        QAction* pWithGlyph = menu.addAction(uiDesign::identityIcon(glyphFile, glyphFile, uiDesign::scmIdentity_input, tokens, tokens.disabledText), qsl("With a glyph"));
        pWithGlyph->setCheckable(true);
        QAction* pWithout = menu.addAction(qsl("Without a glyph"));
        pWithout->setCheckable(true);

        const QImage off = shotOf(&menu);
        pWithGlyph->setChecked(true);
        pWithout->setChecked(true);
        const QImage on = shotOf(&menu);
        QVERIFY2(!off.isNull() && !on.isNull(), "the menu never came up to be grabbed");

        const QRect glyphRow = menu.actionGeometry(pWithGlyph);
        // Between the row's edge and the glyph's own, where only the wash is drawn
        const QPoint washSpot(glyphRow.left() + 2, glyphRow.center().y());
        const QColor washed = TestPixels::sampled(on, washSpot);
        const QColor unwashed = TestPixels::sampled(off, washSpot);
        const QColor wanted = TestPixels::composited(tokens.accentWash, tokens.card);
        const int tickDiffering = differingPixels(off, on, leadingColumnOf(menu.actionGeometry(pWithout)));
        qInfo().noquote()
                << qsl("  at %1 the switched-on glyph's row reads %2 against an expected %3, and %4 switched off; the row without a glyph changes in %5 pixels")
                           .arg(qsl("%1,%2").arg(QString::number(washSpot.x()), QString::number(washSpot.y())), washed.name(), wanted.name(), unwashed.name(), QString::number(tickDiffering));

        QVERIFY2(TestPixels::readsAs(unwashed, tokens.card, scmInkTolerance),
                 qPrintable(qsl("switched off, the row reads %1 beside its glyph rather than the menu's card %2").arg(unwashed.name(), tokens.card.name())));
        QVERIFY2(TestPixels::readsAs(washed, wanted, scmInkTolerance),
                 qPrintable(qsl("switched on, the row reads %1 beside its glyph rather than the accent's wash over the card, %2 - nothing says it is on").arg(washed.name(), wanted.name())));
        QVERIFY2(tickDiffering > glyphRow.height(), "a switched-on row without a glyph no longer shows its tick");
    }

    void test_aSwitchedOnEntryIsToldFromOneThatIsOffUnderAScriptsSheet_data()
    {
        QTest::addColumn<bool>("application");
        QTest::addColumn<QString>("sheet");
        addBothRoads(qsl("hovered row coloured"), hoveredRowSheet());
        addBothRoads(qsl("rows padded"), paddedRowsSheet());
    }

    // Under a sheet giving the rows a box, Qt's stylesheet style draws an entry's icon in place of its tick
    // (QStyleSheetStyle::renderMenuItemIcon()), so a glyph that is one picture either way needs something round it
    void test_aSwitchedOnEntryIsToldFromOneThatIsOffUnderAScriptsSheet()
    {
        QFETCH(bool, application);
        QFETCH(QString, sheet);
        const QList<QAction*> switches = switchesOfTheMenuBar();
        QList<bool> wereChecked;
        for (const QAction* pSwitch : switches) {
            wereChecked << pSwitch->isChecked();
        }
        const bool iconsHidden = qApp->testAttribute(Qt::AA_DontShowIconsInMenus);
        qApp->setAttribute(Qt::AA_DontShowIconsInMenus, false);
        auto restore = qScopeGuard([switches, wereChecked, iconsHidden]() {
            for (qsizetype i = 0; i < switches.size(); ++i) {
                const QSignalBlocker quiet(switches.at(i));
                switches.at(i)->setChecked(wereChecked.at(i));
            }
            qApp->setAttribute(Qt::AA_DontShowIconsInMenus, iconsHidden);
            dropScriptSheets();
        });
        useScriptSheet(application, sheet);
        // On a script's dark menu the wash alone reads near 1.2:1 against the row, so the outline is what has to show
        const QColor outline = uiDesign::themeTokens().accent;

        QStringList alike;
        QStringList unoutlined;
        for (QAction* pSwitch : switches) {
            QMenu* pMenu = menuHolding(pSwitch);
            // Blocked, so switching Full Screen or Always on Top here changes nothing but the mark
            const QSignalBlocker quiet(pSwitch);
            for (const bool hovered : {false, true}) {
                pSwitch->setChecked(false);
                const QImage off = shotOf(pMenu, hovered ? pSwitch : nullptr);
                pSwitch->setChecked(true);
                const QImage on = shotOf(pMenu, hovered ? pSwitch : nullptr);
                QVERIFY2(!off.isNull() && !on.isNull(), qPrintable(qsl("the menu holding %1 never came up to be grabbed").arg(saidAs(pSwitch))));
                const QRect column = leadingColumnOf(pMenu->actionGeometry(pSwitch));
                const QString said = qsl("%1 %2").arg(saidAs(pSwitch), hovered ? qsl("under the pointer") : qsl("at rest"));
                if (const int differing = differingPixels(off, on, column); differing <= column.height()) {
                    alike << qsl("%1 (%2 pixels differ)").arg(said, QString::number(differing));
                }
                if (const int outlined = pixelsTurning(off, on, column, outline); outlined <= column.height()) {
                    unoutlined << qsl("%1 (%2 pixels)").arg(said, QString::number(outlined));
                }
            }
        }
        qInfo().noquote() << qsl("  %1 switches; in one state or the other %2 of them look alike switched on and off, and %3 show no outline in %4 switched on")
                                     .arg(QString::number(switches.size()), QString::number(alike.size()), QString::number(unoutlined.size()), outline.name());

        QVERIFY2(!switches.isEmpty(), "the six menus hold no entry that switches on and off, so nothing below was checked");
        QVERIFY2(alike.isEmpty(), qPrintable(qsl("these entries look the same switched on as off: %1").arg(alike.join(qsl(", ")))));
        QVERIFY2(unoutlined.isEmpty(), qPrintable(qsl("switched on, these entries show no outline in the accent %1 beside their glyph: %2").arg(outline.name(), unoutlined.join(qsl(", ")))));
    }

    void test_aScriptsOwnIconRuleIsKept_data()
    {
        QTest::addColumn<bool>("application");
        QTest::addColumn<QString>("sheet");
        // The second is less specific than a design rule for a switched-on icon would be, so it shows whether one is set
        addBothRoads(qsl("switched-on icon"), paddedRowsSheet() + qsl(" QMenu::icon:checked { background-color: %1; }").arg(scriptsWash().name()));
        addBothRoads(qsl("every icon"), paddedRowsSheet() + qsl(" QMenu::icon { background-color: %1; }").arg(scriptsWash().name()));
    }

    void test_aScriptsOwnIconRuleIsKept()
    {
        QFETCH(bool, application);
        QFETCH(QString, sheet);
        QAction* pEntry = mudlet::self()->dactionInputLine;
        QMenu* pMenu = mudlet::self()->menuEditor;
        const bool wasChecked = pEntry->isChecked();
        const bool iconsHidden = qApp->testAttribute(Qt::AA_DontShowIconsInMenus);
        qApp->setAttribute(Qt::AA_DontShowIconsInMenus, false);
        auto restore = qScopeGuard([pEntry, wasChecked, iconsHidden]() {
            const QSignalBlocker quiet(pEntry);
            pEntry->setChecked(wasChecked);
            qApp->setAttribute(Qt::AA_DontShowIconsInMenus, iconsHidden);
            dropScriptSheets();
        });
        useScriptSheet(application, sheet);
        {
            const QSignalBlocker quiet(pEntry);
            pEntry->setChecked(true);
        }

        const QImage shot = shotOf(pMenu);
        QVERIFY2(!shot.isNull(), "the Toolbox menu never came up to be grabbed");
        const QRect row = pMenu->actionGeometry(pEntry);
        const int washed = pixelsReading(shot, leadingColumnOf(row), scriptsWash());
        qInfo().noquote() << qsl("  %1 pixels of the switched-on Compact input line's leading column are the script's wash").arg(QString::number(washed));
        QVERIFY2(washed > row.height(),
                 qPrintable(qsl("the script's own icon rule no longer washes the switched-on Compact input line: %1 pixels at %2 are in its colour")
                                    .arg(QString::number(washed), TestPixels::describe(row))));
    }

    void test_aScriptsSheetLeavesAGlyphUnderThePointerAsAtRest_data()
    {
        QTest::addColumn<bool>("application");
        QTest::addColumn<QString>("sheet");
        QTest::addColumn<bool>("stylesMenus");
        for (const bool application : {true, false}) {
            const char* road = application ? "setAppStyleSheet" : "setProfileStyleSheet";
            QTest::addRow("%s, hovered row coloured", road) << application << qsl("QMenu::item:selected { background-color: #f0f0f0; color: #141414; }") << true;
            QTest::addRow("%s, rows padded", road) << application << paddedRowsSheet() << true;
            QTest::addRow("%s, the bar alone", road) << application << qsl("QMenuBar { background-color: #102030; }") << false;
        }
    }

    // Qt has no way to read what a script's sheet fills the row under the pointer with, so the glyph stays as at rest
    // there; a sheet that names only the bar leaves the menus to the base style
    void test_aScriptsSheetLeavesAGlyphUnderThePointerAsAtRest()
    {
        QFETCH(bool, application);
        QFETCH(QString, sheet);
        QFETCH(bool, stylesMenus);
        auto restore = qScopeGuard([]() {
            dropScriptSheets();
        });
        const auto astrayFrom = [](const bool asAtRest) {
            QStringList astray;
            for (QMenu* pMenu : menuBarMenus()) {
                const QColor wanted = chosenRowInk(QApplication::palette(pMenu));
                for (QAction* pAction : pMenu->actions()) {
                    if (pAction->isSeparator()) {
                        continue;
                    }
                    for (const QIcon::Mode mode : {QIcon::Active, QIcon::Selected}) {
                        for (const QIcon::State state : {QIcon::Off, QIcon::On}) {
                            const QImage chosen = drawnAt(pAction->icon(), mode, state);
                            const bool astrayHere = asAtRest ? chosen != drawnAt(pAction->icon(), QIcon::Normal, state) : !TestPixels::readsAs(strongestInk(chosen), wanted, scmInkTolerance);
                            if (astrayHere) {
                                astray << qsl("%1 %2 %3 in %4")
                                                  .arg(saidAs(pAction),
                                                       mode == QIcon::Active ? qsl("Active") : qsl("Selected"),
                                                       state == QIcon::On ? qsl("On") : qsl("Off"),
                                                       strongestInk(chosen).name());
                            }
                        }
                    }
                }
            }
            return astray;
        };

        useScriptSheet(application, sheet);
        const QStringList underTheSheet = astrayFrom(stylesMenus);
        const QString menuBarSheet = mudlet::self()->menuBar()->styleSheet();
        dropScriptSheets();
        const QStringList afterTheSheet = astrayFrom(false);
        qInfo().noquote() << qsl("  under the sheet %1 glyphs are astray, and %2 once it is gone").arg(QString::number(underTheSheet.size()), QString::number(afterTheSheet.size()));

        QVERIFY2(underTheSheet.isEmpty(),
                 qPrintable(qsl("under the sheet, these glyphs are not %1 under the pointer: %2")
                                    .arg(stylesMenus ? qsl("as they are at rest") : qsl("in the base style's chosen-row ink"), underTheSheet.join(qsl(", ")))));
        QVERIFY2(afterTheSheet.isEmpty(), qPrintable(qsl("with the sheet gone, these glyphs are not back in the chosen-row ink: %1").arg(afterTheSheet.join(qsl(", ")))));
        QVERIFY2(stylesMenus || !menuBarSheet.contains(qsl("QMenu::icon")),
                 qPrintable(qsl("a sheet naming only the bar put an icon rule on the menu bar, which hands the menus' glyphs to the stylesheet style: %1").arg(menuBarSheet)));
    }

    // The Windows platform theme gives QMenu a palette of its own, apart from the application's
    void test_theRowUnderThePointerFollowsTheMenusClassPalette()
    {
        mudlet* pMudlet = mudlet::self();
        const bool iconsHidden = qApp->testAttribute(Qt::AA_DontShowIconsInMenus);
        qApp->setAttribute(Qt::AA_DontShowIconsInMenus, false);
        TestPixels::setAppearanceAndWait(enums::Appearance::light);
        const QPalette before = QApplication::palette("QMenu");
        auto restore = qScopeGuard([before, iconsHidden]() {
            QApplication::setPalette(before, "QMenu");
            qApp->setAttribute(Qt::AA_DontShowIconsInMenus, iconsHidden);
            // A change of the application's palette clears every class palette
            TestPixels::setAppearanceAndWait(enums::Appearance::dark);
            TestPixels::setAppearanceAndWait(enums::Appearance::systemSetting);
        });

        QPalette menus = before;
        menus.setColor(QPalette::Highlight, QColor(250, 226, 120));
        menus.setColor(QPalette::HighlightedText, QColor(48, 24, 0));
        QApplication::setPalette(menus, "QMenu");
        TestPixels::settle();
        pMudlet->restyleToolBarIcons();
        const QPalette read = QApplication::palette("QMenu");
        const QColor wanted = chosenRowInk(read);
        const QColor applications = chosenRowInk(QApplication::palette());
        QVERIFY2(TestPixels::distance(wanted, applications) > scmDrawnInkTolerance,
                 qPrintable(qsl("the menus' palette reads %1 under the pointer, the application's %2: too alike for this case to tell them apart").arg(wanted.name(), applications.name())));

        QStringList astray;
        for (QMenu* pMenu : menuBarMenus()) {
            for (QAction* pAction : pMenu->actions()) {
                if (pAction->isSeparator()) {
                    continue;
                }
                for (const QIcon::State state : {QIcon::Off, QIcon::On}) {
                    if (const QColor ink = strongestInk(drawnAt(pAction->icon(), QIcon::Active, state)); !TestPixels::readsAs(ink, wanted, scmInkTolerance)) {
                        astray << qsl("%1 %2 in %3").arg(saidAs(pAction), state == QIcon::On ? qsl("On") : qsl("Off"), ink.name());
                    }
                }
            }
        }

        QMenu* pMenu = pMudlet->menuGames;
        QAction* pEntry = pMudlet->dactionConnect;
        const QImage shot = shotOf(pMenu, pEntry);
        const QRect row = pMenu->actionGeometry(pEntry);
        const QRect glyphColumn = QRect(row.left(), row.top(), row.height(), row.height()).adjusted(2, 2, -2, -2);
        const int filled = pixelsReading(shot, row, read.color(QPalette::Highlight));
        QColor nearest;
        for (int y = glyphColumn.top(); y <= glyphColumn.bottom(); ++y) {
            for (int x = glyphColumn.left(); x <= glyphColumn.right(); ++x) {
                const QColor pixel = TestPixels::sampled(shot, QPoint(x, y));
                if (pixel.isValid() && (!nearest.isValid() || TestPixels::distance(pixel, wanted) < TestPixels::distance(nearest, wanted))) {
                    nearest = pixel;
                }
            }
        }
        qInfo().noquote() << qsl("  on the menus' Highlight %1 the glyph is inked %2 (the application's palette would give %3); the hovered Play row has %4 pixels of that fill and its glyph's "
                                 "nearest pixel is %5")
                                     .arg(read.color(QPalette::Highlight).name(), wanted.name(), applications.name(), QString::number(filled), nearest.name());

        QVERIFY2(filled > row.height(),
                 qPrintable(qsl("the hovered Play row is not filled with the menus' own Highlight %1, so the menu is not drawn with that palette").arg(read.color(QPalette::Highlight).name())));
        QVERIFY2(astray.isEmpty(), qPrintable(qsl("under the pointer these glyphs are not in the ink of the menus' own palette, %1: %2").arg(wanted.name(), astray.join(qsl(", ")))));
        QVERIFY2(nearest.isValid() && TestPixels::distance(nearest, wanted) <= scmDrawnInkTolerance,
                 qPrintable(qsl("the hovered Play row shows no glyph in the ink of the menus' own palette %1; the nearest pixel in its glyph column is %2").arg(wanted.name(), nearest.name())));
    }

    void test_theRowUnderThePointerFollowsAPaletteSetOnTheMenu()
    {
        QMenu menu;
        QPalette own = QApplication::palette(&menu);
        own.setColor(QPalette::Highlight, QColor(250, 226, 120));
        own.setColor(QPalette::HighlightedText, QColor(48, 24, 0));
        menu.setPalette(own);
        QAction* pEntry = menu.addAction(qsl("Play"));
        const QString glyphFile = qsl(":/icons/toolbar-connect.svg");
        uiDesign::restyleMenuGlyphs({{pEntry, glyphFile, QString(), uiDesign::scmIdentity_connect}}, uiDesign::themeTokens(), true, false);

        const QColor wanted = chosenRowInk(menu.palette());
        const QColor applications = chosenRowInk(QApplication::palette());
        QVERIFY2(TestPixels::distance(wanted, applications) > scmDrawnInkTolerance,
                 qPrintable(qsl("the menu's own palette reads %1 under the pointer, the application's %2: too alike for this case to tell them apart").arg(wanted.name(), applications.name())));
        const QColor ink = strongestInk(drawnAt(pEntry->icon(), QIcon::Active, QIcon::Off));
        QVERIFY2(TestPixels::readsAs(ink, wanted, scmInkTolerance),
                 qPrintable(qsl("under the pointer the glyph is inked %1 rather than %2, the ink of the palette set on its menu").arg(ink.name(), wanted.name())));
    }

    // A native menu - the macOS menu bar, a desktop's global menu - holds a copy of an entry's icon, taken as the entry changes
    void test_changingShowIconsOnMenusTellsEveryEntry()
    {
        mudlet* pMudlet = mudlet::self();
        const Qt::CheckState wasState = pMudlet->mShowIconsOnMenuCheckedState;
        auto restore = qScopeGuard([wasState]() {
            mudlet::self()->setShowIconsOnMenu(wasState);
        });
        QSet<QAction*> told;
        QList<QAction*> entries;
        QObject listener;
        for (QMenu* pMenu : menuBarMenus()) {
            for (QAction* pAction : pMenu->actions()) {
                if (pAction->isSeparator()) {
                    continue;
                }
                entries << pAction;
                connect(pAction, &QAction::changed, &listener, [&told, pAction]() {
                    told.insert(pAction);
                });
            }
        }
        QVERIFY2(!entries.isEmpty(), "the six menus hold no entries at all, so nothing below was checked");

        pMudlet->setShowIconsOnMenu(wasState == Qt::Unchecked ? Qt::Checked : Qt::Unchecked);

        QStringList untold;
        for (QAction* pAction : std::as_const(entries)) {
            if (!told.contains(pAction)) {
                untold << saidAs(pAction);
            }
        }
        qInfo().noquote() << qsl("  %1 of %2 entries were told the setting changed").arg(QString::number(entries.size() - untold.size()), QString::number(entries.size()));
        QVERIFY2(untold.isEmpty(), qPrintable(qsl("these entries were not told, so a native menu goes on showing or hiding their icon as before: %1").arg(untold.join(qsl(", ")))));
    }

#if defined(Q_OS_MACOS)
    // Offscreen the bar is drawn by Qt, and neither it nor its menus has a native menu to set
    void test_menuImageVisibilityLeavesABarWithoutNativeMenusAlone()
    {
        QMenuBar* pBar = mudlet::self()->menuBar();
        QVERIFY2(!pBar->toNSMenu(), "the bar has a native menu here, so this case does not run what it is named for");

        setMacMenuImagesAlwaysVisible(nullptr, true);
        setMacMenuImagesAlwaysVisible(pBar, true);
        setMacMenuImagesAlwaysVisible(pBar, false);
    }
#endif
};

#include "MenuBarGlyphTest.moc"
MUDLET_GROUPED_TEST_MAIN(MenuBarGlyphTest)
