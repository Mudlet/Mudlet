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
#include <QPlainTextEdit>
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
#include "dlgComposer.h"
#include "dlgProfilePreferences.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

// Not the starting dictionary, so that reading it can only be down to the
// change made below.
static const QString scmDictionary = qsl("en_GB");

// loadSystemDictionary() logs every read it makes - as "loaded", or as "is not
// available" where the files are not installed - which is the only way to see
// one without asking for the handle, and asking for it would make the read.
static QtMessageHandler previousMessageHandler = nullptr;
static int dictionaryReads = 0;

static void countDictionaryReads(QtMsgType type, const QMessageLogContext& context, const QString& message)
{
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
    QString mStartingDictionary;
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
        mStartingDictionary = mpHost->getSpellDic();
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
    // picking it there again once they are retries it
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
        const QString shown = mpHost->mainConsoleView()->buffer.lineBuffer.join(QChar::Space).simplified();
        QVERIFY2(shown.contains(qsl("[ WARN ]")) && shown.contains(dictionary), qPrintable(qsl("the player was not told the dictionary is missing; the console holds: %1").arg(shown)));
        // Asked for again with every word checked
        mpHost->spellChecker().systemHandle();
        mpHost->spellChecker().systemHandle();
        QCOMPARE(warningsShownFor(dictionary), 1);

        delete mpPreferences;
        mpPreferences = new dlgProfilePreferences(mudlet::self(), mpHost);
        QComboBox* pDictionaries = mpPreferences->comboBox_dictionary;
        QCOMPARE(pDictionaries->currentData().toString(), dictionary);
        QCOMPARE(pDictionaries->currentText(), qsl("%1 - not available").arg(dictionary));

        QVERIFY(writeFile(wordsPath, "1\nkalamazoo\n"));
        emit pDictionaries->activated(pDictionaries->currentIndex());
        QVERIFY2(mpHost->spellChecker().systemHandle(), "picking the dictionary again once its files were there did not retry it");
        QCOMPARE(pDictionaries->currentText(), qsl("%1 - not recognised").arg(dictionary));
    }

    // The folder listed is the one the profile's dictionary would be in, which for one that is
    // nowhere is a fallback that need hold no dictionaries at all
    void test_aMissingDictionaryLeavesTheOthersToPickFrom()
    {
        const QString dictionary = qsl("mudlet_dictionary_found_nowhere");
        const QString fallbackFolder = MudletApp::getMudletPath(enums::hunspellDictionaryPath, dictionary);
        const QString siblingFolder = qsl("%1/../src/").arg(QCoreApplication::applicationDirPath());
        const QString affixPath = qsl("%1%2.aff").arg(siblingFolder, mStartingDictionary);
        const QString wordsPath = qsl("%1%2.dic").arg(siblingFolder, mStartingDictionary);
        bool wroteStartingDictionary = false;
        bool madeSiblingFolder = false;
        const auto cleanup = qScopeGuard([&]() {
            if (wroteStartingDictionary) {
                QFile::remove(affixPath);
                QFile::remove(wordsPath);
            }
            if (madeSiblingFolder) {
                QDir().rmdir(siblingFolder);
            }
            mpHost->setSpellDic(scmDictionary);
        });
#if !defined(Q_OS_MACOS)
        // Looked in ahead of the fallback everywhere but macOS, which keeps every dictionary in one folder
        if (MudletApp::getMudletPath(enums::hunspellDictionaryPath, mStartingDictionary) == fallbackFolder) {
            madeSiblingFolder = !QDir(siblingFolder).exists();
            QVERIFY(QDir().mkpath(siblingFolder));
            wroteStartingDictionary = true;
            QVERIFY(writeFile(affixPath, "SET UTF-8\n"));
            QVERIFY(writeFile(wordsPath, "1\nthe\n"));
        }
#endif
        if (MudletApp::getMudletPath(enums::hunspellDictionaryPath, mStartingDictionary) == fallbackFolder) {
            QSKIP("the starting dictionary is looked for in the same folder as one that is nowhere");
        }

        mpHost->setSpellDic(dictionary);
        delete mpPreferences;
        mpPreferences = new dlgProfilePreferences(mudlet::self(), mpHost);
        QComboBox* pDictionaries = mpPreferences->comboBox_dictionary;
        QVERIFY2(pDictionaries->isEnabled() && pDictionaries->findData(mStartingDictionary) >= 0,
                 qPrintable(qsl("the preferences offered no other dictionary to pick, although \"%1\" is installed").arg(mStartingDictionary)));
        QCOMPARE(pDictionaries->currentText(), qsl("%1 - not available").arg(dictionary));
    }

    void test_aDictionaryInstalledSinceIsReadWhenThePreferencesOpen()
    {
        const QString dictionary = qsl("mudlet_installed_later");
        const QString folder = MudletApp::getMudletPath(enums::hunspellDictionaryPath, dictionary);
        QVERIFY(QDir().mkpath(folder));
        const QString affixPath = qsl("%1%2.aff").arg(folder, dictionary);
        const QString wordsPath = qsl("%1%2.dic").arg(folder, dictionary);
        const auto cleanup = qScopeGuard([this, affixPath, wordsPath]() {
            QFile::remove(affixPath);
            QFile::remove(wordsPath);
            mpHost->setSpellDic(scmDictionary);
        });

        mpHost->setSpellDic(dictionary);
        QVERIFY(!mpHost->spellChecker().systemHandle());
        QVERIFY(writeFile(affixPath, "SET UTF-8\n"));
        QVERIFY(writeFile(wordsPath, "1\nkalamazoo\n"));

        delete mpPreferences;
        mpPreferences = new dlgProfilePreferences(mudlet::self(), mpHost);
        QCOMPARE(mpPreferences->comboBox_dictionary->currentText(), qsl("%1 - not recognised").arg(dictionary));
        QVERIFY2(mpHost->spellChecker().systemHandle(), "the preferences listed the dictionary as there while it was still not read, so nothing was spell-checked");
    }

    // Names ending in "med" are left out of the list, as supplements to another dictionary
    void test_aDictionaryThatLoadsIsNotShownAsMissing()
    {
        const QString dictionary = qsl("mudlet_listedmed");
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
        QVERIFY(writeFile(wordsPath, "1\nkalamazoo\n"));

        mpHost->setSpellDic(dictionary);
        QVERIFY(mpHost->spellChecker().systemHandle());
        delete mpPreferences;
        mpPreferences = new dlgProfilePreferences(mudlet::self(), mpHost);
        QCOMPARE(mpPreferences->comboBox_dictionary->currentText(), qsl("%1 - not recognised").arg(dictionary));
    }

    // Hunspell_create() reads an empty word list as a dictionary of no words
    void test_anEmptyWordListIsNotLoaded()
    {
        const QString dictionary = qsl("mudlet_empty_word_list");
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
        QVERIFY(writeFile(wordsPath, QByteArray()));

        mpHost->setSpellDic(dictionary);
        QVERIFY2(!mpHost->spellChecker().systemHandle(), "an empty word list was loaded, and would mark every word as misspelt");
        QCOMPARE(warningsShownFor(dictionary), 1);
    }

    // Nothing is marked against a dictionary that is not there, which includes the marks made against the one before
    void test_aMissingDictionaryLeavesNoMarksBehind()
    {
        const QString dictionary = qsl("mudlet_marking_dictionary");
        const QString folder = MudletApp::getMudletPath(enums::hunspellDictionaryPath, dictionary);
        QVERIFY(QDir().mkpath(folder));
        const QString affixPath = qsl("%1%2.aff").arg(folder, dictionary);
        const QString wordsPath = qsl("%1%2.dic").arg(folder, dictionary);
        TCommandLine* pCommandLine = mpHost->mainConsoleView()->mpCommandLine;
        QVERIFY(pCommandLine);
        auto* pComposer = new dlgComposer(mpHost);
        const auto cleanup = qScopeGuard([this, affixPath, wordsPath, pCommandLine, pComposer]() {
            delete pComposer;
            pCommandLine->clear();
            QFile::remove(affixPath);
            QFile::remove(wordsPath);
            mpHost->setSpellDic(scmDictionary);
        });
        QVERIFY(writeFile(affixPath, "SET UTF-8\n"));
        QVERIFY(writeFile(wordsPath, "1\nthe\n"));
        QVERIFY(mpHost->getEnableSpellCheck());

        mpHost->setSpellDic(dictionary);
        pCommandLine->setPlainText(qsl("helo wrld "));
        pCommandLine->recheckWholeLine();
        pComposer->init(qsl("composer"), qsl("helo wrld"));
        pComposer->show();
        QVERIFY(QTest::qWaitForWindowExposed(pComposer));
        QVERIFY2(marked(pCommandLine->document(), qsl("helo")) && marked(pComposer->edit->document(), qsl("helo")), "the dictionary written here marked nothing, so this proves nothing");

        mpHost->setSpellDic(qsl("mudlet_no_such_dictionary"));
        QVERIFY2(!marked(pCommandLine->document(), qsl("helo")), "the input line kept the marks of the dictionary before");
        // The composer checks a word as it is typed after
        QTextCursor cursor = pComposer->edit->textCursor();
        cursor.setPosition(4);
        pComposer->edit->setTextCursor(cursor);
        QTest::keyClick(pComposer->edit, Qt::Key_Space);
        QVERIFY2(!marked(pComposer->edit->document(), qsl("helo")), "the composer kept the mark of the dictionary before on a word checked again");
    }

private:
    static bool writeFile(const QString& path, const QByteArray& contents)
    {
        QFile file(path);
        return file.open(QIODevice::WriteOnly) && file.write(contents) == contents.size();
    }

    int warningsShownFor(const QString& dictionary) const
    {
        const QString warning = qsl("The spell-check dictionary \"%1\"").arg(dictionary);
        int count = 0;
        for (const auto& line : mpHost->mainConsoleView()->buffer.lineBuffer) {
            count += line.count(warning);
        }
        return count;
    }

    static bool marked(TCommandLine* pCommandLine, const QString& word) { return marked(pCommandLine->document(), word); }

    static bool marked(QTextDocument* pDocument, const QString& word)
    {
        const QTextCursor found = pDocument->find(word);
        QTextCursor at(pDocument);
        at.setPosition(found.selectionStart() + 1);
        return !found.isNull() && at.charFormat().underlineStyle() == QTextCharFormat::SpellCheckUnderline;
    }
};

#include "SettingsSpellCheckEnableTest.moc"
MUDLET_GROUPED_TEST_MAIN(SettingsSpellCheckEnableTest)
