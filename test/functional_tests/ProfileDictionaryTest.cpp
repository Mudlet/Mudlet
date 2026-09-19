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
 * ".dic" file. hunspell refuses to load a dictionary that declares no words -
 * 1.7.3 reports "missing or bad word count" where 1.7.2 accepted it - and
 * Hunspell_create() returns a non-null handle either way, so checking the
 * handle alone does not detect the dictionary-loading failure.
 *
 * The assertions are on the file rather than on hunspell's behaviour: a test
 * that only checked whether the dictionary loaded would pass on a 1.7.2 build
 * whether or not the bug was present.
 *
 * Run with: ctest -R ProfileDictionaryTest -V
 */

#include "PortableModeTestHelper.h"
#include "MudletInstanceCoordinator.h"
#include "mudlet.h"

#include <QtTest/QtTest>

#include "GroupedTest.h"

class ProfileDictionaryTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mXdgDir;
    QByteArray mSavedXdg;

    const QString mEmptyProfile = qsl("dictionary empty");
    const QString mStockedProfile = qsl("dictionary stocked");

    QString dictionaryPath(const QString& profileName) const { return mudlet::getMudletPath(enums::profileDataItemPath, profileName, qsl("profile.dic")); }

    void makeProfileFolder(const QString& name) const { QVERIFY(QDir().mkpath(mudlet::getMudletPath(enums::profileDataItemPath, name, QString()))); }

    // The count hunspell reads, and the words that follow it:
    QStringList dictionaryLines(const QString& profileName) const
    {
        QFile dict(dictionaryPath(profileName));
        if (!dict.open(QFile::ReadOnly | QFile::Text)) {
            return {};
        }
        return QString::fromUtf8(dict.readAll()).split(QChar::LineFeed, Qt::SkipEmptyParts);
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
        QVERIFY(mudlet::getMudletPath(enums::profilesPath).startsWith(mXdgDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();

        makeProfileFolder(mEmptyProfile);
        makeProfileFolder(mStockedProfile);
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
        QVERIFY2(handle, "no dictionary was prepared for a new profile");
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
    // hunspell uses to size its hash table:
    void test_storedWordsAreCountedExactly()
    {
        QFile dict(dictionaryPath(mStockedProfile));
        QVERIFY(dict.open(QFile::WriteOnly | QFile::Text));
        dict.write(QString(qsl("2\nbrandish\nquaff\n")).toUtf8());
        dict.close();

        QSet<QString> wordSet;
        Hunhandle* handle = mudlet::self()->prepareProfileDictionary(mStockedProfile, wordSet);
        QVERIFY2(handle, "no dictionary was prepared for a profile with stored words");
        Hunspell_destroy(handle);

        QCOMPARE(wordSet, QSet<QString>({qsl("brandish"), qsl("quaff")}));

        const QStringList lines = dictionaryLines(mStockedProfile);
        QCOMPARE(lines.size(), 3);
        QCOMPARE(lines.first(), qsl("2"));
        QCOMPARE(lines.mid(1), QStringList({qsl("brandish"), qsl("quaff")}));
    }
};

#include "ProfileDictionaryTest.moc"
MUDLET_GROUPED_TEST_MAIN(ProfileDictionaryTest)
