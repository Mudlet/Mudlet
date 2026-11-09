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
 * A colour trigger names an ANSI colour, but the text it is matched against
 * carries only the RGB that colour had in the profile's palette when the text
 * arrived. The palette can change while the profile runs - the server can
 * redefine it (<OSC>P) and reset it (<OSC>R), and the player can in the
 * preferences - so a trigger has to follow it, or it goes on looking for the
 * old RGB and never fires on the game's text again.
 *
 * This is a functional test rather than a spec because only the preferences
 * can let a server redefine the palette; no script can.
 *
 * Run with: ctest -R ColorTriggerPaletteTest -V
 */

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "Host.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "TLuaInterpreter.h"
#include "TTrigger.h"
#include "TriggerUnit.h"
#include "mudlet.h"

#include "GroupedTest.h"

class ColorTriggerPaletteTest : public QObject
{
    Q_OBJECT

private:
    const QString mProfileName = qsl("ColorTriggerPalette-Test");
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    Host* mpHost = nullptr;

    // Past the floor of root triggers the root filter needs before it switches
    // on, so that the filter's copy of the trigger's colours is tested too
    static constexpr int scmFillerTriggers = 80;

    bool runLua(const QString& script) { return mpHost->mLuaInterpreter.compileAndExecuteScript(script); }

    int luaInteger(const char* name)
    {
        lua_State* L = mpHost->getLuaInterpreter()->getLuaGlobalState();
        lua_getglobal(L, name);
        const int value = static_cast<int>(lua_tointeger(L, -1));
        lua_pop(L, 1);
        return value;
    }

    // "31" for red text, "41" for text on red
    QString mRedSgr = qsl("31");

    int redLineFires()
    {
        const int before = luaInteger("paletteFires");
        runLua(qsl("feedTriggers('\\27[%1mpalette red words\\27[0m\\n')").arg(mRedSgr));
        return luaInteger("paletteFires") - before;
    }

    void checkTriggerFollowsThePalette(const QString& redSgr = qsl("31"))
    {
        mRedSgr = redSgr;
        const QColor originalRed = mpHost->mRed;
        const QColor redefinedRed(0x12, 0x34, 0x56);
        QVERIFY2(originalRed != redefinedRed, "the profile's red is already the redefined one, so the assertions below cannot fail");

        QCOMPARE(redLineFires(), 1);

        runLua(qsl("feedTriggers('\\27]P1123456\\27\\\\\\n')"));
        QCOMPARE(mpHost->mRed, redefinedRed);
        QVERIFY2(redLineFires() == 1, "a red colour trigger stopped firing on red text once the server redefined red");

        runLua(qsl("feedTriggers('\\27]R\\27\\\\\\n')"));
        QCOMPARE(mpHost->mRed, originalRed);
        QVERIFY2(redLineFires() == 1, "a red colour trigger stopped firing on red text once the server reset the palette");

        // What the preferences do when the player picks a new red
        mpHost->mRed = redefinedRed;
        mpHost->applyMainConsoleColors();
        QVERIFY2(redLineFires() == 1, "a red colour trigger stopped firing on red text once the player changed red");
        mpHost->mRed = originalRed;
        mpHost->applyMainConsoleColors();
        QCOMPARE(redLineFires(), 1);
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }
        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);

        // An empty save, so no default packages install and the only triggers
        // in the profile are this test's
        const QString folder = MudletApp::getMudletPath(enums::profileXmlFilesPath, mProfileName);
        QVERIFY(QDir().mkpath(folder));
        QFile save(qsl("%1/2020-01-01#00-00-00.xml").arg(folder));
        QVERIFY(save.open(QIODevice::WriteOnly | QIODevice::Text));
        QVERIFY(save.write("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                           "<!DOCTYPE MudletPackage>\n"
                           "<MudletPackage version=\"1.001\">\n"
                           "<HostPackage><Host></Host></HostPackage>\n"
                           "</MudletPackage>\n")
                > 0);
        save.close();

        mpHost = mudlet::self()->loadProfile(mProfileName, false);
        QVERIFY(mpHost);
        QVERIFY(mpHost->mLoadedOk);
        mudlet::self()->slot_connectionDialogueFinished(mProfileName, false);
        QVERIFY(mpHost->mainConsoleView());
        mpHost->setMayRedefineColors(true);
    }

    void cleanupTestCase()
    {
        mpHost = nullptr;
        delete mudlet::self();
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    // A failed check returns early, so this puts the palette and the triggers back for the next case
    void cleanup()
    {
        // killTrigger() refuses permanent triggers
        delete mpHost->getTriggerUnit()->findTrigger(qsl("paletteFolder"));
        runLua(qsl("feedTriggers('\\27]R\\27\\\\\\n')"));
        runLua(qsl("if paletteTrigger then killTrigger(paletteTrigger) end\n"
                   "for _, id in ipairs(paletteFillers or {}) do killTrigger(id) end\n"
                   "paletteTrigger, paletteFillers = nil, nil\n"
                   "feedTriggers('palette teardown line\\n')"));
    }

    void colorTriggerFollowsThePalette()
    {
        QVERIFY(runLua(qsl("paletteFires = 0\n"
                           "paletteTrigger = tempAnsiColorTrigger(1, -1, function() paletteFires = paletteFires + 1 end)")));
        checkTriggerFollowsThePalette();
    }

    void rootFilteredColorTriggerFollowsThePalette()
    {
        QString setup;
        for (int i = 1; i <= scmFillerTriggers; ++i) {
            setup += qsl("table.insert(paletteFillers, tempTrigger('qqpalettefiller%1zz', ''))\n").arg(i);
        }
        QVERIFY(runLua(qsl("paletteFires = 0\npaletteFillers = {}\n%1"
                           "paletteTrigger = tempAnsiColorTrigger(1, -1, function() paletteFires = paletteFires + 1 end)\n"
                           "feedTriggers('palette warmup line\\n')")
                               .arg(setup)));
        QVERIFY2(mpHost->getTriggerUnit()->rootPrescanActive(), "the root filter has not engaged, so this tests nothing the case above does not");
        checkTriggerFollowsThePalette();
    }

    void backgroundColorTriggerFollowsThePalette()
    {
        QVERIFY(runLua(qsl("paletteFires = 0\n"
                           "paletteTrigger = tempAnsiColorTrigger(-1, 1, function() paletteFires = paletteFires + 1 end)")));
        checkTriggerFollowsThePalette(qsl("41"));
    }

    // A child is in no root snapshot, so it is only reached through the unit's map of every trigger
    void childColorTriggerFollowsThePalette()
    {
        QVERIFY(runLua(qsl("paletteFires = 0\npermGroup('paletteFolder', 'trigger')")));
        TTrigger* pFolder = mpHost->getTriggerUnit()->findTrigger(qsl("paletteFolder"));
        QVERIFY(pFolder);
        auto* pChild = new TTrigger(pFolder, mpHost);
        pChild->setName(qsl("paletteChild"));
        QVERIFY(pChild->setRegexCodeList({TTrigger::createColorPatternText(1, TTrigger::scmIgnored)}, {REGEX_COLOR_PATTERN}));
        pChild->setScript(qsl("paletteFires = paletteFires + 1"));
        pChild->setIsActive(true);
        pChild->registerTrigger();
        checkTriggerFollowsThePalette();
    }

    // Servers send the redefinition inline with their text, so it can land between two lines of one chunk
    void paletteChangeBetweenLinesOfOneChunk()
    {
        QVERIFY(runLua(qsl("paletteFires = 0\n"
                           "paletteTrigger = tempAnsiColorTrigger(1, -1, function() paletteFires = paletteFires + 1 end)\n"
                           "feedTriggers('\\27[31mbefore\\27[0m\\n\\27]P1123456\\27\\\\\\27[31mafter\\27[0m\\n')")));
        QCOMPARE(mpHost->mRed, QColor(0x12, 0x34, 0x56));
        QCOMPARE(luaInteger("paletteFires"), 2);
    }
};

#include "ColorTriggerPaletteTest.moc"
MUDLET_GROUPED_TEST_MAIN(ColorTriggerPaletteTest)
