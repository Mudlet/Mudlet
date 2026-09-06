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
 * TSpellChecker::prepareProfileDictionary() returns nullptr when the profile's
 * dictionary cannot be read or rewritten - an unwritable profile directory, a
 * full disk, or an unreadable profile.dic. The Hunspell C API dereferences its
 * handle without checking it, so handing that nullptr on takes the whole client
 * down rather than refusing the word.
 *
 * This stands an unopenable dictionary in the way and asks the spell checker to
 * add and to remove a word: both have to come back with a false/reason pair.
 *
 * Run with: ctest -R SpellDictionaryFailureTest -V
 */

#include <QtTest/QtTest>

#include <QDir>
#include <QTemporaryDir>

#include "Host.h"
#include "HostManager.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "TSpellChecker.h"
#include "mudlet.h"

#include "GroupedTest.h"

class SpellDictionaryFailureTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    Host* mpHost = nullptr;
    const QString mProfileName = qsl("spell-dictionary-failure");

private slots:
    void initTestCase()
    {
        QVERIFY(mConfigDir.isValid());
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);

        if (portableMarkerPresent()) {
            QSKIP("portable.txt marker present - config dir cannot be redirected for this test");
        }
        QVERIFY2(MudletApp::getMudletPath(enums::profilesPath).startsWith(mConfigDir.path()), "test config dir redirection did not take effect");

        // A directory standing where profile.dic belongs: it exists, so the
        // scan does not skip it, and it can never be opened as a file. Made
        // before the profile so the first look at the dictionary already fails
        // - a handle opened once is cached and would mask this.
        QVERIFY(QDir().mkpath(MudletApp::getMudletPath(enums::profileDataItemPath, mProfileName, qsl("profile.dic"))));
        QVERIFY(QDir().mkpath(MudletApp::getMudletPath(enums::mainDataItemPath, qsl("mudlet.dic"))));

        QVERIFY(mudlet::self()->getHostManager().addHost(mProfileName, QString(), QString(), QString()));
        mpHost = mudlet::self()->getHostManager().getHost(mProfileName);
        QVERIFY(mpHost);

        // The profile's own dictionary, not the shared one:
        mpHost->setUserDictionaryOptions(true, false);
        QVERIFY2(!mpHost->spellChecker().userHandle(), "the rigged profile dictionary opened anyway, so this test proves nothing");
    }

    void cleanupTestCase()
    {
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
        delete mudlet::self();
    }

    void test_addWordRefusesWhenTheProfileDictionaryCannotBeOpened()
    {
        const auto result = mpHost->spellChecker().addWord(qsl("kalamazoo"));
        QVERIFY2(!result.first, "addWord() claimed to have added a word with no dictionary to add it to");
        QVERIFY2(!result.second.isEmpty(), "addWord() refused the word without saying why");
    }

    void test_removeWordRefusesWhenTheProfileDictionaryCannotBeOpened()
    {
        const auto result = mpHost->spellChecker().removeWord(qsl("kalamazoo"));
        QVERIFY2(!result.first, "removeWord() claimed to have removed a word with no dictionary to remove it from");
        QVERIFY2(!result.second.isEmpty(), "removeWord() refused the word without saying why");
    }

    // The shared dictionary fails the same three ways, and reaching it is a
    // matter of one profile setting, so these run last - they leave the profile
    // pointed at the shared dictionary.
    void test_addWordRefusesWhenTheSharedDictionaryCannotBeOpened()
    {
        mpHost->setUserDictionaryOptions(true, true);
        QVERIFY2(!TSpellChecker::sharedDictionary(), "the rigged shared dictionary opened anyway, so this test proves nothing");

        const auto result = mpHost->spellChecker().addWord(qsl("kalamazoo"));
        QVERIFY2(!result.first, "addWord() claimed to have added a word with no dictionary to add it to");
        QVERIFY2(result.second.contains(qsl("could not be opened")), qPrintable(qsl("addWord() blamed the wrong thing: '%1'").arg(result.second)));
    }

    void test_removeWordRefusesWhenTheSharedDictionaryCannotBeOpened()
    {
        mpHost->setUserDictionaryOptions(true, true);

        const auto result = mpHost->spellChecker().removeWord(qsl("kalamazoo"));
        QVERIFY2(!result.first, "removeWord() claimed to have removed a word with no dictionary to remove it from");
        QVERIFY2(result.second.contains(qsl("could not be opened")), qPrintable(qsl("removeWord() blamed the wrong thing: '%1'").arg(result.second)));
    }
};

#include "SpellDictionaryFailureTest.moc"
MUDLET_GROUPED_TEST_MAIN(SpellDictionaryFailureTest)
