/***************************************************************************
 *   Copyright (C) 2026 by Gesslar - karahd@gmail.com                      *
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
 * Guards the word count Mudlet writes on the first line of a profile's
 * ".dic" file. hunspell has refused to load a dictionary that declares no
 * words since 1.3.4 - the "missing or bad word count" guard is unchanged
 * between 1.7.2 and 1.7.3 - so the failure is neither new nor confined to one
 * release. 1.7.3 only made it audible, by emitting that message with fprintf
 * where earlier releases used HUNSPELL_WARNING, which compiles away unless
 * HUNSPELL_WARNING_ON is defined.
 *
 * The assertions are on the file rather than on hunspell's behaviour because
 * for an empty dictionary there is no behaviour to assert on: one that failed
 * to load and one that loaded with no words answer spell(), add() and
 * suggest() identically. The handle says nothing either - Hunspell_create()
 * is a new expression behind a cast, so it is non-null on every version and
 * for every outcome. The header is the only place the difference shows.
 *
 * The handle is still checked, but only for what it can actually prove:
 * prepareProfileDictionary() returns nullptr when Mudlet itself gives up
 * before reaching hunspell.
 *
 * Run with: ctest -R ProfileDictionaryTest -V
 */

#include "PortableModeTestHelper.h"
#include "MudletInstanceCoordinator.h"
#include "MudletApp.h"
#include "mudlet.h"

#include <QtTest/QtTest>

#include <algorithm>

#include "GroupedTest.h"

static QStringList sCapturedMessages;

static void captureMessage(QtMsgType, const QMessageLogContext&, const QString& message)
{
    sCapturedMessages << message;
}

class ProfileDictionaryTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mXdgDir;
    QByteArray mSavedXdg;

    const QString mEmptyProfile = qsl("dictionary empty");
    const QString mSingleWordProfile = qsl("dictionary single");
    const QString mStockedProfile = qsl("dictionary stocked");
    const QString mZeroCountProfile = qsl("dictionary zero");
    const QString mQuietProfile = qsl("dictionary quiet");
    const QString mAffixProfile = qsl("dictionary affix");

    QString dictionaryPath(const QString& profileName) const { return MudletApp::getMudletPath(enums::profileDataItemPath, profileName, qsl("profile.dic")); }

    QString affixPath(const QString& profileName) const { return MudletApp::getMudletPath(enums::profileDataItemPath, profileName, qsl("profile.aff")); }

    void makeProfileFolder(const QString& name) const { QVERIFY(QDir().mkpath(MudletApp::getMudletPath(enums::profileDataItemPath, name, QString()))); }

    void writeDictionary(const QString& profileName, const QString& contents) const
    {
        QFile dict(dictionaryPath(profileName));
        QVERIFY(dict.open(QFile::WriteOnly | QFile::Text));
        QCOMPARE(dict.write(contents.toUtf8()), contents.toUtf8().size());
    }

    // The count hunspell reads, and the words that follow it:
    QStringList dictionaryLines(const QString& profileName) const
    {
        QFile dict(dictionaryPath(profileName));
        if (!dict.open(QFile::ReadOnly | QFile::Text)) {
            return {};
        }
        return QString::fromUtf8(dict.readAll()).split(QChar::LineFeed, Qt::SkipEmptyParts);
    }

    // The directives hunspell parses, in the order they are written:
    QStringList affixLines(const QString& profileName) const
    {
        QFile aff(affixPath(profileName));
        if (!aff.open(QFile::ReadOnly | QFile::Text)) {
            return {};
        }
        return QString::fromUtf8(aff.readAll()).split(QChar::LineFeed, Qt::SkipEmptyParts);
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - cannot redirect the config dir for this test");
        }

        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        QVERIFY(mXdgDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mXdgDir.path()))); // profiles/ = XDG opt-in
        qputenv("XDG_CONFIG_HOME", mXdgDir.path().toUtf8());

        mudlet::start();
        mudlet::self()->setupConfig();
        // never touch the user's real profiles:
        QVERIFY(MudletApp::getMudletPath(enums::profilesPath).startsWith(mXdgDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();

        makeProfileFolder(mEmptyProfile);
        makeProfileFolder(mSingleWordProfile);
        makeProfileFolder(mStockedProfile);
        makeProfileFolder(mZeroCountProfile);
        makeProfileFolder(mQuietProfile);
        makeProfileFolder(mAffixProfile);
    }

    void cleanupTestCase()
    {
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
        delete mudlet::self();
    }

    // A brand new profile has no words to spell check against, which is the
    // case that used to write a zero count and produce a dictionary hunspell
    // would not load:
    void test_newProfileWritesALoadableWordCount()
    {
        QSet<QString> wordSet;
        Hunhandle* handle = mudlet::self()->prepareProfileDictionary(mEmptyProfile, wordSet);
        QVERIFY2(handle, "prepareProfileDictionary() gave up before reaching hunspell");
        Hunspell_destroy(handle);

        QVERIFY(wordSet.isEmpty());

        const QStringList lines = dictionaryLines(mEmptyProfile);
        QVERIFY2(!lines.isEmpty(), "the profile dictionary was not written at all");

        bool isNumber = false;
        const int declaredCount = lines.first().toInt(&isNumber);
        QVERIFY2(isNumber, qPrintable(qsl("first line of the dictionary is not a number: '%1'").arg(lines.first())));
        QVERIFY2(declaredCount >= 1, "an empty profile dictionary declared no words, which hunspell refuses to load");
    }

    // The padding for the empty case must not inflate a real word count, which
    // hunspell uses to size its hash table. A single word is the boundary that
    // shows it: a two word dictionary is written as "2" by an exact count and
    // by a qMax(2, ...) padding alike, so it cannot tell them apart.
    void test_storedWordsAreCountedExactly()
    {
        writeDictionary(mSingleWordProfile, qsl("1\nbrandish\n"));

        QSet<QString> wordSet;
        Hunhandle* handle = mudlet::self()->prepareProfileDictionary(mSingleWordProfile, wordSet);
        QVERIFY2(handle, "prepareProfileDictionary() gave up before reaching hunspell");
        Hunspell_destroy(handle);

        QCOMPARE(wordSet, QSet<QString>({qsl("brandish")}));
        QCOMPARE(dictionaryLines(mSingleWordProfile), QStringList({qsl("1"), qsl("brandish")}));

        writeDictionary(mStockedProfile, qsl("2\nbrandish\nquaff\n"));

        handle = mudlet::self()->prepareProfileDictionary(mStockedProfile, wordSet);
        QVERIFY2(handle, "prepareProfileDictionary() gave up before reaching hunspell");
        Hunspell_destroy(handle);

        QCOMPARE(wordSet, QSet<QString>({qsl("brandish"), qsl("quaff")}));
        QCOMPARE(dictionaryLines(mStockedProfile), QStringList({qsl("2"), qsl("brandish"), qsl("quaff")}));
    }

    // Every profile that has already hit this bug has a "0" on disk, so the
    // repair on load is the path that matters to them. A fix that only got the
    // creation path right would leave those dictionaries unloadable and still
    // pass every other test here:
    void test_existingZeroCountIsRepairedOnLoad()
    {
        writeDictionary(mZeroCountProfile, qsl("0\n"));

        QSet<QString> wordSet;
        Hunhandle* handle = mudlet::self()->prepareProfileDictionary(mZeroCountProfile, wordSet);
        QVERIFY2(handle, "prepareProfileDictionary() gave up before reaching hunspell");
        Hunspell_destroy(handle);

        QVERIFY(wordSet.isEmpty());
        // Repaired, and without a placeholder word being invented to justify it:
        QCOMPARE(dictionaryLines(mZeroCountProfile), QStringList({qsl("1")}));
    }

    // The padded count is a floor for hunspell, not a claim about how many
    // words are stored, so neither reader may report the padding as a word the
    // user has since removed:
    void test_emptyDictionaryReportsNoWordsLost()
    {
        QSet<QString> wordSet;
        Hunhandle* handle = mudlet::self()->prepareProfileDictionary(mQuietProfile, wordSet);
        QVERIFY2(handle, "prepareProfileDictionary() gave up before reaching hunspell");
        Hunspell_destroy(handle);
        QCOMPARE(dictionaryLines(mQuietProfile), QStringList({qsl("1")}));

        // Only the second pass sees the padded count written by the first:
        sCapturedMessages.clear();
        // The diagnostics are qDebug() lines, which some distributions - Fedora
        // among them - turn off by default in their shipped qtlogging.ini:
        QLoggingCategory::setFilterRules(qsl("default.debug=true"));
        QtMessageHandler previousHandler = qInstallMessageHandler(captureMessage);
        handle = mudlet::self()->prepareProfileDictionary(mQuietProfile, wordSet);
        const bool saved = mudlet::self()->saveDictionary(MudletApp::getMudletPath(enums::profileDataItemPath, mQuietProfile, qsl("profile")), wordSet);
        qInstallMessageHandler(previousHandler);
        QLoggingCategory::setFilterRules(QString());

        QVERIFY2(handle, "prepareProfileDictionary() gave up before reaching hunspell");
        Hunspell_destroy(handle);
        QVERIFY(saved);

        // The diagnostics under test are qDebug() lines: capturing nothing at
        // all would pass this test without exercising anything:
        QVERIFY2(!sCapturedMessages.isEmpty(), "no dictionary diagnostics were captured");

        for (const QString& message : sCapturedMessages) {
            QVERIFY2(!message.contains(qsl("fewer words")), qPrintable(qsl("an unchanged empty dictionary reported lost words: '%1'").arg(message)));
            QVERIFY2(!message.contains(qsl("Previously, there were")), qPrintable(qsl("an unchanged empty dictionary reported a differing stored count: '%1'").arg(message)));
        }
    }

    // The ".dic" half is only one of the pair. hunspell rejects the whole affix
    // file if TRY is written with nothing after it, so an empty profile - which
    // has no graphemes to offer - must not carry the line at all, or every load
    // prints "Failure loading aff file" no matter how sound the ".dic" is:
    void test_affixFileOmitsAnEmptyTryLine()
    {
        QSet<QString> wordSet;
        Hunhandle* handle = mudlet::self()->prepareProfileDictionary(mAffixProfile, wordSet);
        QVERIFY2(handle, "prepareProfileDictionary() gave up before reaching hunspell");
        Hunspell_destroy(handle);

        QCOMPARE(affixLines(mAffixProfile), QStringList({qsl("SET UTF-8")}));

        // ...and it comes back as soon as there is something to try:
        writeDictionary(mAffixProfile, qsl("1\nbrandish\n"));
        handle = mudlet::self()->prepareProfileDictionary(mAffixProfile, wordSet);
        QVERIFY2(handle, "prepareProfileDictionary() gave up before reaching hunspell");
        Hunspell_destroy(handle);

        const QStringList lines = affixLines(mAffixProfile);
        QCOMPARE(lines.size(), 2);
        QCOMPARE(lines.first(), qsl("SET UTF-8"));
        QVERIFY2(lines.at(1).startsWith(qsl("TRY ")), qPrintable(qsl("no TRY line for a stocked dictionary: '%1'").arg(lines.at(1))));
        // The graphemes are ordered by frequency, which ties for these, so
        // compare the set of them rather than the order:
        QString graphemes = lines.at(1).mid(4);
        std::sort(graphemes.begin(), graphemes.end());
        QCOMPARE(graphemes, qsl("abdhinrs"));
    }
};

#include "ProfileDictionaryTest.moc"
MUDLET_GROUPED_TEST_MAIN(ProfileDictionaryTest)
