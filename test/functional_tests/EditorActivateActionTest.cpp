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
 * The editor's Activate button is on the toolbar and in the trees' menu, worded
 * for what a click does to the chosen row, and held to one width so the buttons
 * after it do not move as rows are chosen.
 */

#include <QToolBar>
#include <QToolButton>
#include <QtTest/QtTest>

#include "Host.h"
#include "HostDialogs.h"
#include "ProfileSessionTestHelper.h"
#include "ScriptUnit.h"
#include "TLuaInterpreter.h"
#include "TScript.h"
#include "dlgTriggerEditor.h"
#include "uiDesign.h"
#include "mudlet.h"

#include "GroupedTest.h"

class EditorActivateActionTest : public QObject
{
    Q_OBJECT

private:
    TestProfile::Session mSession;
    dlgTriggerEditor* mpEditor = nullptr;
    Host* mpHost = nullptr;
    const QString mProfileName = qsl("EditorActivateAction-Test-Profile");
    const QString mScriptName = qsl("activateWordingProbe");

    static constexpr int scmItemGroup = 1;

    QAction* action() const { return mpEditor->mpAction_toggleActive; }
    QToolButton* button() const { return qobject_cast<QToolButton*>(mpEditor->toolBar->widgetForAction(action())); }
    QTreeWidget* tree() const { return mpEditor->treeWidget_scripts; }

    TScript* chosenScript() const
    {
        QTreeWidgetItem* pItem = tree()->currentItem();
        return pItem ? mpHost->getScriptUnit()->getScript(pItem->data(0, Qt::UserRole).toInt()) : nullptr;
    }

    QString wordingFor(const TScript* pScript) const { return pScript->shouldBeActive() ? mpEditor->tr("Deactivate") : mpEditor->tr("Activate"); }

    // What the bar's layout gives the button
    int heldWidth() const { return button()->sizeHint().expandedTo(button()->minimumSize()).width(); }

    void runLua(const QString& code) { QVERIFY2(mpHost->getLuaInterpreter()->compileAndExecuteScript(code), qPrintable(code)); }

private slots:
    void initTestCase()
    {
        if (TestProfile::Session::configCannotBeRedirected()) {
            QSKIP(TestProfile::Session::configCannotBeRedirectedReason());
        }

        QVERIFY2(mSession.start(mProfileName), qPrintable(mSession.why()));
        mpHost = mSession.host();

        mudlet::self()->slot_showEditorDialog();
        QCoreApplication::processEvents();
        mpEditor = HostDialogs::of(mpHost).mpEditorDialog;
        QVERIFY2(mpEditor != nullptr, "Editor dialog should be created");
        // Wide enough that every button keeps its name
        mpEditor->resize(1600, 700);
        mpEditor->slot_showScripts();
        QCoreApplication::processEvents();
    }

    void cleanupTestCase()
    {
        mpEditor = nullptr;
        mpHost = nullptr;
        mSession.stop();
    }

    void init()
    {
        mpEditor->slot_showScripts();
        tree()->setCurrentItem(mpEditor->mpScriptsBaseItem);
        mpEditor->addScript(false);
        QCoreApplication::processEvents();
        TScript* pScript = chosenScript();
        QVERIFY2(pScript, "Adding a script should choose it");
        pScript->setName(mScriptName);
    }

    void cleanup()
    {
        if (chosenScript()) {
            mpEditor->slot_deleteItemOrGroup();
        }
        QCoreApplication::processEvents();
    }

    void test_theBarCarriesItAmongTheItemButtons()
    {
        QVERIFY2(button() != nullptr, "Activate has no button on the editor's toolbar");
        QVERIFY2(mpEditor->mEditorToolBarGroups.at(scmItemGroup).actions.contains(action()), "Activate is not in the item group, so it would keep its name after the buttons beside it gave theirs up");
        QVERIFY2(!action()->icon().isNull(), "Activate has no glyph to stand as once the bar drops its names");
    }

    void test_aClickRewordsItForTheRowItLeaves()
    {
        TScript* pScript = chosenScript();
        QVERIFY(pScript);
        QVERIFY2(action()->isEnabled(), "A chosen script should be switchable");

        for (int click = 0; click < 2; ++click) {
            const bool wasOn = pScript->shouldBeActive();
            QCOMPARE(action()->text(), wordingFor(pScript));
            action()->trigger();
            QCoreApplication::processEvents();
            QVERIFY2(pScript->shouldBeActive() != wasOn, "The click did not switch the chosen script");
            QCOMPARE(action()->text(), wordingFor(pScript));
            QVERIFY2(action()->toolTip().contains(action()->text()), "The tooltip should lead with the button's wording");
        }
    }

    void test_undoRewordsIt()
    {
        TScript* pScript = chosenScript();
        QVERIFY(pScript);
        const QString before = action()->text();
        action()->trigger();
        QCoreApplication::processEvents();
        QVERIFY(action()->text() != before);

        mpEditor->mpUndoStack->undo();
        QCoreApplication::processEvents();
        QCOMPARE(action()->text(), wordingFor(pScript));
        QCOMPARE(action()->text(), before);
    }

    void test_aScriptSwitchingTheRowRewordsIt()
    {
        TScript* pScript = chosenScript();
        QVERIFY(pScript);

        runLua(qsl("enableScript('%1')").arg(mScriptName));
        QCoreApplication::processEvents();
        QVERIFY(pScript->shouldBeActive());
        QCOMPARE(action()->text(), mpEditor->tr("Deactivate"));

        runLua(qsl("disableScript('%1')").arg(mScriptName));
        QCoreApplication::processEvents();
        QVERIFY(!pScript->shouldBeActive());
        QCOMPARE(action()->text(), mpEditor->tr("Activate"));
    }

    // A hidden editor skips the tree repaint a script's toggle asks for, so only the direct hook rewords it
    void test_aScriptSwitchingTheRowWhileTheEditorIsHiddenRewordsIt()
    {
        TScript* pScript = chosenScript();
        QVERIFY(pScript);
        runLua(qsl("enableScript('%1')").arg(mScriptName));
        QCoreApplication::processEvents();
        QCOMPARE(action()->text(), mpEditor->tr("Deactivate"));

        mpEditor->hide();
        QCoreApplication::processEvents();
        runLua(qsl("disableScript('%1')").arg(mScriptName));
        QCoreApplication::processEvents();
        const QString hiddenWording = action()->text();
        mpEditor->show();
        QCoreApplication::processEvents();

        QVERIFY(!pScript->shouldBeActive());
        QCOMPARE(hiddenWording, mpEditor->tr("Activate"));
    }

    void test_bothWordingsTakeTheSameWidth()
    {
        QVERIFY(button() != nullptr);
        QCOMPARE(button()->toolButtonStyle(), Qt::ToolButtonTextBesideIcon);

        runLua(qsl("disableScript('%1')").arg(mScriptName));
        QCoreApplication::processEvents();
        const int offWidth = heldWidth();
        runLua(qsl("enableScript('%1')").arg(mScriptName));
        QCoreApplication::processEvents();
        const int onWidth = heldWidth();

        QVERIFY2(offWidth == onWidth,
                 qPrintable(qsl("\"Activate\" takes %1px and \"Deactivate\" %2px, so the buttons after it move as rows are chosen").arg(QString::number(offWidth), QString::number(onWidth))));
        QVERIFY2(onWidth >= button()->sizeHint().width(), "The held width is narrower than the wording it carries");
    }

    void test_asAPictureItIsNotHeldToItsWords()
    {
        QVERIFY(button() != nullptr);
        auto& group = mpEditor->mEditorToolBarGroups[scmItemGroup];
        group.labelsShown = false;
        mpEditor->applyEditorToolbarButtonStyles();
        QCoreApplication::processEvents();
        const int pictureMinimum = button()->minimumWidth();

        group.labelsShown = true;
        mpEditor->applyEditorToolbarButtonStyles();
        QCoreApplication::processEvents();

        QCOMPARE(pictureMinimum, 0);
    }

    // disabledText has to read on a card too, which on dark leaves it 1.5:1 from the bar's own words
    void test_unavailableItIsPlainlyDimmerThanTheButtonsBesideIt()
    {
        const QPalette savedPalette = QApplication::palette();
        QPalette darkPalette(savedPalette);
        darkPalette.setColor(QPalette::Window, QColor(0x35, 0x35, 0x35));
        darkPalette.setColor(QPalette::WindowText, QColor(Qt::white));
        darkPalette.setColor(QPalette::Base, QColor(0x19, 0x19, 0x19));
        QPalette lightPalette(savedPalette);
        lightPalette.setColor(QPalette::Window, QColor(0xec, 0xec, 0xec));
        lightPalette.setColor(QPalette::WindowText, QColor(Qt::black));
        lightPalette.setColor(QPalette::Base, QColor(Qt::white));

        QStringList misses;
        for (const QPalette& palette : {darkPalette, lightPalette}) {
            const uiDesign::ThemeTokens tokens = uiDesign::themeTokens(palette);
            const QString theme = tokens.darkPage ? qsl("dark") : qsl("light");
            const qreal onThePage = uiDesign::contrastRatio(tokens.disabledBarText, tokens.page);
            const qreal fromTheWords = uiDesign::contrastRatio(tokens.disabledBarText, tokens.mutedText);
            qInfo().noquote() << qsl("  %1: an unavailable bar button is %2, %3:1 on the page and %4:1 from the %5 of the buttons beside it")
                                         .arg(theme, tokens.disabledBarText.name(), QString::number(onThePage, 'f', 2), QString::number(fromTheWords, 'f', 2), tokens.mutedText.name());
            if (onThePage < uiDesign::scmDisabledBarMinimumRatio) {
                misses << qsl("%1: %2 reads %3:1 on the page, under the floor").arg(theme, tokens.disabledBarText.name(), QString::number(onThePage, 'f', 2));
            }
            if (fromTheWords < 2.0) {
                misses << qsl("%1: %2 is %3:1 from the available buttons' %4, too close to tell at a glance")
                                  .arg(theme, tokens.disabledBarText.name(), QString::number(fromTheWords, 'f', 2), tokens.mutedText.name());
            }
        }
        QVERIFY2(misses.isEmpty(), qPrintable(misses.join(qsl("\n"))));

        tree()->setCurrentItem(mpEditor->mpScriptsBaseItem);
        QCoreApplication::processEvents();
        QVERIFY(!action()->isEnabled());
        const uiDesign::ThemeTokens tokens = uiDesign::themeTokens();

        const QColor word = button()->palette().color(QPalette::Disabled, button()->foregroundRole());
        QVERIFY2(word.rgb() == tokens.disabledBarText.rgb(), qPrintable(qsl("the unavailable button's word is %1, not the bar's %2").arg(word.name(), tokens.disabledBarText.name())));

        // A 1.5px stroke has no fully covered pixel at this size; the most covered one carries the ink unpremultiplied
        const QImage glyph = action()->icon().pixmap(button()->iconSize(), QIcon::Disabled).toImage().convertToFormat(QImage::Format_ARGB32);
        QRgb densest = 0;
        for (int y = 0; y < glyph.height(); ++y) {
            for (int x = 0; x < glyph.width(); ++x) {
                if (qAlpha(glyph.pixel(x, y)) > qAlpha(densest)) {
                    densest = glyph.pixel(x, y);
                }
            }
        }
        QVERIFY2(qAlpha(densest) > 128, "the unavailable glyph has nothing drawn to read");
        const QColor ink(densest);
        const auto closeTo = [](const QColor& a, const QColor& b) {
            return std::abs(a.red() - b.red()) <= 2 && std::abs(a.green() - b.green()) <= 2 && std::abs(a.blue() - b.blue()) <= 2;
        };
        QVERIFY2(closeTo(ink, tokens.disabledBarText), qPrintable(qsl("the unavailable button's glyph is %1, not the bar's %2").arg(ink.name(), tokens.disabledBarText.name())));
    }

    void test_nothingToSwitchLeavesItUnavailable()
    {
        tree()->setCurrentItem(mpEditor->mpScriptsBaseItem);
        QCoreApplication::processEvents();
        QVERIFY2(!action()->isEnabled(), "The Scripts heading is chosen, which has nothing to switch");

        mpEditor->slot_showVariables();
        QCoreApplication::processEvents();
        QVERIFY2(!action()->isEnabled(), "The Variables view has nothing to switch");

        mpEditor->slot_showScripts();
        tree()->setCurrentItem(mpEditor->mpScriptsBaseItem->child(mpEditor->mpScriptsBaseItem->childCount() - 1));
        QCoreApplication::processEvents();
        QVERIFY2(action()->isEnabled(), "Choosing a script again should make it switchable again");
    }
};

#include "EditorActivateActionTest.moc"
MUDLET_GROUPED_TEST_MAIN(EditorActivateActionTest)
