/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Makers                                   *
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
 * Regression test for #10763: a word put into the user dictionary has to
 * survive being written to the profile's ".dic" file and read back again, or
 * the word list a script reads and the dictionary the spell checker actually
 * uses part company from the next start of the profile onwards.
 *
 * The pair that has to agree - mudlet::saveDictionary() and
 * mudlet::prepareProfileDictionary() - is only reachable from C++: the file is
 * written when the console is destroyed, and no Lua function triggers that. So
 * the Lua specs can only pin what TMainConsole::addWordToSet() refuses, and
 * this pins that the refusals are exactly the words the file cannot give back.
 *
 * Run with: ctest -R DictionaryRoundTripTest -V
 */

#include <QDir>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "TMainConsole.h"
#include "TelnetServerStub.h"
#include "mudlet.h"

#include "GroupedTest.h"

class DictionaryRoundTripTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mHostname = qsl("Dictionary-RoundTrip");
    // A profile folder that only ever holds a dictionary, so putting words
    // straight into a ".dic" file cannot disturb the open profile's own one:
    const QString mFileOnlyProfile = qsl("Dictionary-RoundTrip-file");
    QString mPort;
    const QString mLocalhost = qsl("localhost");

    QString dictionaryBase(const QString& profileName) const { return mudlet::getMudletPath(enums::profileDataItemPath, profileName, qsl("profile")); }

    // Writes wordSet out the way closing a profile does, then reads it back the
    // way opening one does. Returns the word list that came back; *handle takes
    // the dictionary hunspell built from the same file, for the caller to ask
    // what the spell checker itself ended up knowing.
    QSet<QString> roundTrip(QSet<QString> wordSet, Hunhandle** handle) const
    {
        *handle = nullptr;
        if (!mudlet::self()->saveDictionary(dictionaryBase(mFileOnlyProfile), wordSet)) {
            return {};
        }
        QSet<QString> reloaded;
        *handle = mudlet::self()->prepareProfileDictionary(mFileOnlyProfile, reloaded);
        return reloaded;
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

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0);
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        // never touch the developer's real profiles
        QVERIFY(mudlet::getMudletPath(enums::profilesPath).startsWith(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);

        mpHost = TestProfile::create(mHostname, mLocalhost, mPort);
        QVERIFY2(mpHost, "no active host after profile creation");
        QVERIFY(mpHost->mpConsole);

        // saveDictionary() reads the existing word count before writing, so the
        // file-only profile needs the pair of files a first load would make:
        QVERIFY(QDir().mkpath(mudlet::getMudletPath(enums::profileDataItemPath, mFileOnlyProfile, QString())));
        QSet<QString> empty;
        Hunhandle* seed = mudlet::self()->prepareProfileDictionary(mFileOnlyProfile, empty);
        QVERIFY2(seed, "could not prepare the dictionary of the file-only profile");
        Hunspell_destroy(seed);
    }

    void cleanupTestCase()
    {
        mpHost = nullptr;
        delete mpServer;
        mpServer = nullptr;
        if (mudlet::self()) {
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    // The words a user dictionary exists for - including the ones a narrower
    // rule would sweep up with the rejects - come back from the file as
    // themselves, and hunspell knows them afterwards.
    void test_theWordsTheApiAcceptsSurviveTheFile()
    {
        const QStringList words{qsl("mudletplain"), qsl("don't"), qsl("well-known"), QString::fromUtf8("naïve"), qsl("two words"), QString::fromUtf8("日本語")};

        QSet<QString> wordSet;
        for (const auto& word : words) {
            const QPair<bool, QString> result = mpHost->mpConsole->addWordToSet(word);
            QVERIFY2(result.first, qPrintable(qsl("addWordToSet() refused \"%1\": %2").arg(word, result.second)));
            wordSet.insert(word);
            mpHost->mpConsole->removeWordFromSet(word);
        }

        Hunhandle* handle = nullptr;
        const QSet<QString> reloaded = roundTrip(wordSet, &handle);
        QVERIFY2(handle, "the saved dictionary would not load again");
        QCOMPARE(reloaded, wordSet);
        for (const auto& word : words) {
            QVERIFY2(Hunspell_spell(handle, word.toUtf8().constData()), qPrintable(qsl("the reloaded dictionary did not know \"%1\"").arg(word)));
        }
        Hunspell_destroy(handle);
    }

    // Every word in the first list is one the file genuinely cannot give back:
    // either Mudlet's own reader loses it or changes it, or hunspell ends up
    // knowing something else. The second list is refused for not being a word
    // rather than for anything the format does to it - both of those do come
    // back from the file intact, so only the first list is held to that.
    void test_theWordsTheApiRefusesAreOnesTheFileCannotCarry()
    {
        const QStringList unstorable{qsl(""), qsl("qa\nword"), qsl("qa\rword"), qsl(" qapadded"), qsl("qapadded\t"), qsl("qatab\tword"), qsl("qaslash/word")};
        const QStringList notWords{qsl("   "), qsl("qapadded ")};

        const QStringList refused = unstorable + notWords;
        for (const auto& word : refused) {
            const QPair<bool, QString> result = mpHost->mpConsole->addWordToSet(word);
            QVERIFY2(!result.first, qPrintable(qsl("addWordToSet() took \"%1\"").arg(word)));
            QVERIFY(result.second.contains(QLatin1String("cannot be stored in the user dictionary")));
        }

        for (const auto& word : unstorable) {
            Hunhandle* handle = nullptr;
            const QSet<QString> reloaded = roundTrip(QSet<QString>{word}, &handle);
            QVERIFY2(handle, qPrintable(qsl("the dictionary holding \"%1\" would not load again").arg(word)));
            const bool wordListHasIt = reloaded.contains(word);
            const bool spellerHasIt = Hunspell_spell(handle, word.toUtf8().constData());
            Hunspell_destroy(handle);
            QVERIFY2(!wordListHasIt || !spellerHasIt, qPrintable(qsl("\"%1\" came back from the file intact, so refusing it is not warranted").arg(word)));
        }
    }

    // removeWordFromSet() says why an unstorable word can never be in the
    // dictionary, but must still try the removal: a word stored by a build
    // without this validation is in the word list, and refusing outright would
    // leave no way of taking it out. The shared dictionary has no validation of
    // its own, which is how one gets seeded here.
    void test_aWordStoredBeforeTheValidationCanStillBeTakenOut()
    {
        const QString whitespaceWord = qsl("   ");

        mpHost->setUserDictionaryOptions(true, true);
        QVERIFY2(mudlet::self()->prepareSharedDictionary(), "the shared dictionary would not load");
        mudlet::self()->addWordToSet(whitespaceWord);
        QVERIFY(mpHost->mpConsole->getWordSet().contains(whitespaceWord));

        QVERIFY2(!mpHost->mpConsole->addWordToSet(whitespaceWord).first, "addWordToSet() took a word of nothing but spaces");
        const QPair<bool, QString> removal = mpHost->mpConsole->removeWordFromSet(whitespaceWord);
        QVERIFY2(removal.first, qPrintable(qsl("a word stored before the validation existed could not be removed: %1").arg(removal.second)));
        QVERIFY(!mpHost->mpConsole->getWordSet().contains(whitespaceWord));

        mpHost->setUserDictionaryOptions(true, false);
    }
};

#include "DictionaryRoundTripTest.moc"
MUDLET_GROUPED_TEST_MAIN(DictionaryRoundTripTest)
