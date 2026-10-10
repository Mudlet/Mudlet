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
 * The system spell dictionary is read once the profile has finished loading,
 * and only for a profile that has spell check on. Turning spell check on in the
 * preferences is therefore another way the dictionary goes from unused to
 * used mid-session, and it has to be read then as well - otherwise the first
 * word typed pays for it.
 *
 * Run with: ctest -R SettingsSpellCheckEnableTest -V
 */

#include <QDir>
#include <QScopeGuard>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <QCheckBox>
#include <QComboBox>
#include <QSignalSpy>

#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "SettingsTestHelper.h"
#include "Host.h"
#include "MudletApp.h"
#include "TCommandLine.h"
#include "TMainConsole.h"
#include "MudletInstanceCoordinator.h"
#include "TelnetServerStub.h"
#include "dlgProfilePreferences.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

// Not the starting dictionary, so that reading it can only be down to the
// change made below. Hunspell_create() hands back a handle whether or not the
// files exist, so the load is logged on every platform.
static const QString scmDictionary = qsl("en_GB");

// loadSystemSpellDictionary() logs every read it makes, which is the only way to
// see one without asking for the handle - and asking for it would make the read.
static QtMessageHandler previousMessageHandler = nullptr;
static int dictionaryReads = 0;

static void countDictionaryReads(QtMsgType type, const QMessageLogContext& context, const QString& message)
{
    // A dictionary that is not on this machine is still looked for, which is the read asked for
    if (message.contains(qsl("System Hunspell dictionary \"%1\" loaded").arg(scmDictionary)) || message.contains(qsl("the Hunspell dictionary \"%1\" is not available").arg(scmDictionary))) {
        ++dictionaryReads;
    }
    if (previousMessageHandler) {
        previousMessageHandler(type, context, message);
    }
}

class SettingsSpellCheckEnableTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    dlgProfilePreferences* mpPreferences = nullptr;
    const QString mProfileName = qsl("SettingsSpellCheckEnable-Test");
    const QString mLocalhost = qsl("localhost");
    QString mPort; // the stub's actual ephemeral port

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        // A config root of this process's own - see the same block in
        // DialogTeardownTest for why sharing the developer's one does not work
        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0); // ephemeral OS-assigned port avoids collisions across concurrent test runs
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>(qsl("MudletInstanceCoordinator")));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        TestSettings::deleteProfileDirectory(mProfileName);

        previousMessageHandler = qInstallMessageHandler(countDictionaryReads);
        mpHost = TestProfile::create(mProfileName, mLocalhost, mPort);
        QVERIFY2(mpHost, "No active host after profile creation");
    }

    void cleanupTestCase()
    {
        qInstallMessageHandler(previousMessageHandler);
        previousMessageHandler = nullptr;
        delete mpPreferences;
        mpPreferences = nullptr;
        mpHost = nullptr;
        delete mpServer;
        mpServer = nullptr;
        // Null when initTestCase skipped or failed ahead of mudlet::start(), and
        // getMudletPath() dereferences the instance rather than checking it
        if (mudlet::self()) {
            TestSettings::deleteProfileDirectory(mProfileName);
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void test_tickingSpellCheckReadsTheDictionary()
    {
        // Picking a dictionary queues a read too; the flag makes the slot skip it
        mpHost->setEnableSpellCheck(false);
        mpHost->setSpellDic(scmDictionary);
        QTest::qWait(TestSettings::scmQuietWindow);
        QCOMPARE(dictionaryReads, 0);

        mpPreferences = new dlgProfilePreferences(mudlet::self(), mpHost);
        mpPreferences->resize(1060, 760);
        mpPreferences->show();
        QVERIFY(QTest::qWaitForWindowExposed(mpPreferences));
        QVERIFY2(mpPreferences->checkBox_spellCheck->isEnabled(), "the spell check box cannot be ticked");
        QVERIFY2(!mpPreferences->checkBox_spellCheck->isChecked(), "the spell check box does not show the profile's setting");

        QSignalSpy applySpy(mpPreferences, &dlgProfilePreferences::signal_preferencesSaved);
        mpPreferences->checkBox_spellCheck->click();
        QVERIFY2(TestSettings::waitForApply(applySpy), "the tick never reached the Host");
        QVERIFY2(mpHost->getEnableSpellCheck(), "the tick did not turn spell check on");

        // Nothing here has asked for the handle, so a read now can only be the
        // one the tick queued
        QTRY_VERIFY2_WITH_TIMEOUT(dictionaryReads == 1, "turning spell check on left the dictionary unread, for the first word typed to pay for", 5s);
    }

    // Words already in the input lines are checked again, not only ones typed after
    void test_switchingSpellCheckRechecksTheInputLine()
    {
        QVERIFY2(mpPreferences && mpHost->getEnableSpellCheck(), "the case above left spell check off");
        TCommandLine* pCommandLine = mpHost->mainConsoleView()->mpCommandLine;
        QVERIFY(pCommandLine);
        const auto [created, message] = mpHost->mainConsoleView()->createCommandLine(QString(), qsl("spellCheckLine"), 0, 0, 100, 30);
        QVERIFY2(created, qPrintable(message));
        TCommandLine* pSubCommandLine = mpHost->mainConsoleView()->subCommandLineWidget(qsl("spellCheckLine"));
        QVERIFY(pSubCommandLine);
        const auto cleanup = qScopeGuard([this, pCommandLine]() {
            pCommandLine->clear();
            mpHost->mainConsoleView()->deleteCommandLine(qsl("spellCheckLine"));
        });

        pCommandLine->setPlainText(qsl("helo wrld "));
        pCommandLine->recheckWholeLine();
        pSubCommandLine->setPlainText(qsl("helo wrld "));
        pSubCommandLine->recheckWholeLine();
        if (!marked(pCommandLine, qsl("helo"))) {
            QSKIP("no dictionary here marks \"helo\" as misspelt");
        }
        QVERIFY(marked(pSubCommandLine, qsl("helo")));

        QSignalSpy applySpy(mpPreferences, &dlgProfilePreferences::signal_preferencesSaved);
        mpPreferences->checkBox_spellCheck->click();
        QVERIFY2(TestSettings::waitForApply(applySpy), "the untick never reached the Host");
        QVERIFY2(!marked(pCommandLine, qsl("helo")), "switching spell check off left the marks on the input line");
        QVERIFY2(!marked(pSubCommandLine, qsl("helo")), "switching spell check off left the marks on a createCommandLine() line");

        applySpy.clear();
        mpPreferences->checkBox_spellCheck->click();
        QVERIFY2(TestSettings::waitForApply(applySpy), "the tick never reached the Host");
        QVERIFY2(marked(pCommandLine, qsl("helo")), "switching spell check back on did not mark the input line again");
        QVERIFY2(marked(pSubCommandLine, qsl("helo")), "switching spell check back on did not mark a createCommandLine() line again");
    }

    void test_pickingAnotherDictionaryRechecksTheInputLine()
    {
        QVERIFY2(mpHost->getEnableSpellCheck(), "the case above left spell check off");
        TCommandLine* pCommandLine = mpHost->mainConsoleView()->mpCommandLine;
        QVERIFY(pCommandLine);
        const auto cleanup = qScopeGuard([pCommandLine]() {
            pCommandLine->clear();
        });

        // The first case picked British English
        pCommandLine->setPlainText(qsl("colour color "));
        pCommandLine->recheckWholeLine();
        if (!marked(pCommandLine, qsl("color")) || marked(pCommandLine, qsl("colour"))) {
            QSKIP("no en_GB dictionary here tells \"colour\" from \"color\"");
        }

        mpHost->setSpellDic(qsl("en_US"));
        const bool rechecked = marked(pCommandLine, qsl("colour")) && !marked(pCommandLine, qsl("color"));
        mpHost->setSpellDic(scmDictionary);
        QVERIFY2(rechecked, "picking another dictionary did not check the input line again");
    }

    // Until its files are all there the preferences show the dictionary as not available, and
    // picking it there again once they are retries it, as nothing else would (#10153)
    void test_aMissingDictionaryIsShownAsSuchAndRetriedWhenPickedAgain()
    {
        const QString dictionary = qsl("mudlet_retried_dictionary");
        // Found nowhere, the name falls back to a folder of Mudlet's own, which takes the files below
        const QString folder = MudletApp::getMudletPath(enums::hunspellDictionaryPath, dictionary);
        QVERIFY(QDir().mkpath(folder));
        const QString affixPath = qsl("%1%2.aff").arg(folder, dictionary);
        const QString wordsPath = qsl("%1%2.dic").arg(folder, dictionary);
        const auto cleanup = qScopeGuard([this, affixPath, wordsPath]() {
            QFile::remove(affixPath);
            QFile::remove(wordsPath);
            mpHost->setSpellDic(scmDictionary);
        });
        QVERIFY(writeFile(affixPath, "SET UTF-8\n"));

        mpHost->setSpellDic(dictionary);
        QVERIFY2(!mpHost->spellChecker().systemHandle(), "a dictionary with no word list was loaded, and would mark every word as misspelt");
        const QString shown = mpHost->mpConsole->buffer.lineBuffer.join(QChar::Space).simplified();
        QVERIFY2(shown.contains(qsl("[ WARN ]")) && shown.contains(dictionary), qPrintable(qsl("the player was not told the dictionary is missing; the console holds: %1").arg(shown)));

        delete mpPreferences;
        mpPreferences = new dlgProfilePreferences(mudlet::self(), mpHost);
        QComboBox* pDictionaries = mpPreferences->comboBox_dictionary;
        QCOMPARE(pDictionaries->currentData().toString(), dictionary);
        QCOMPARE(pDictionaries->currentText(), qsl("%1 - not available").arg(dictionary));

        QVERIFY(writeFile(wordsPath, "1\nkalamazoo\n"));
        emit pDictionaries->activated(pDictionaries->currentIndex());
        QVERIFY2(mpHost->spellChecker().systemHandle(), "picking the dictionary again once its files were there did not retry it");
    }

private:
    static bool writeFile(const QString& path, const QByteArray& contents)
    {
        QFile file(path);
        return file.open(QIODevice::WriteOnly) && file.write(contents) == contents.size();
    }

    static bool marked(TCommandLine* pCommandLine, const QString& word)
    {
        const QTextCursor found = pCommandLine->document()->find(word);
        QTextCursor at(pCommandLine->document());
        at.setPosition(found.selectionStart() + 1);
        return !found.isNull() && at.charFormat().underlineStyle() == QTextCharFormat::SpellCheckUnderline;
    }
};

#include "SettingsSpellCheckEnableTest.moc"
MUDLET_GROUPED_TEST_MAIN(SettingsSpellCheckEnableTest)
