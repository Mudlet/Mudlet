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
 * A test that drives setupConfig() has to point XDG_CONFIG_HOME at a temporary
 * directory and opt that directory in, or it runs against the developer's real
 * ~/.config/mudlet. Since #9712 the opt-in marker is
 * $XDG_CONFIG_HOME/mudlet/profiles - the mudlet directory on its own no longer
 * counts, because other tooling creates that by accident.
 *
 * Two tests kept seeding the old recipe (#9810). That was caught only because
 * both happened to assert their config root afterwards; a test that seeds the
 * stale recipe and asserts nothing quietly reads and writes the user's own
 * profiles, and one of those two is a test that deletes profiles.
 *
 * So creating a directory whose path ends in /mudlet is an error here, unless
 * the same file also creates the profiles/ opt-in beneath it. A test that means
 * it says so with an "xdg-recipe-guard: allow" comment on the call or the line
 * above it.
 *
 * What that catches is the recipe copied from another test, which is how both of
 * those two got it: the path has to be spelled in the call. A file that builds
 * the config root through a helper or a local first is out of range, and
 * ConfigDirOverrideTest - which creates every shape of config root deliberately,
 * because the resolution rules are its subject - is the only one that does.
 *
 * The test directory is provided at configure time via MUDLET_TEST_DIR. Like
 * CMakeListsConsistencyTest this pulls in no Mudlet headers, hence QStringLiteral
 * rather than utils.h's qsl().
 *
 * Run with: ctest -R XdgRecipeConsistencyTest -V
 */

#include <QtTest/QtTest>

#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QVector>

class XdgRecipeConsistencyTest : public QObject
{
    Q_OBJECT

    struct DirectoryCreation
    {
        int line = 0;
        QString argument;
    };

    static QString testDir() { return QStringLiteral(MUDLET_TEST_DIR); }

    static QString allowToken() { return QStringLiteral("xdg-recipe-guard: allow"); }

    // Blanks out comment bodies so a recipe quoted in prose cannot read as code,
    // keeping the newlines so line numbers survive. The lines spanned by a
    // comment holding the allow token are collected on the way through.
    static QString withoutComments(const QString& source, QSet<int>& allowedLines)
    {
        enum class State { code, lineComment, blockComment, string, character };
        State state = State::code;
        QString stripped;
        stripped.reserve(source.size());
        QString comment;
        int line = 1;
        int commentStart = 1;

        auto endComment = [&]() {
            if (comment.contains(allowToken())) {
                for (int marked = commentStart; marked <= line; ++marked) {
                    allowedLines.insert(marked);
                }
            }
            comment.clear();
        };

        for (qsizetype i = 0; i < source.size(); ++i) {
            const QChar current = source.at(i);
            const QChar next = i + 1 < source.size() ? source.at(i + 1) : QChar(u'\0');
            switch (state) {
            case State::code:
                if (current == u'/' && (next == u'/' || next == u'*')) {
                    state = next == u'/' ? State::lineComment : State::blockComment;
                    commentStart = line;
                    stripped.append(QStringLiteral("  "));
                    ++i;
                    continue;
                }
                if (current == u'R' && next == u'"') {
                    // A raw string carries unbalanced quotes as ordinary text, so
                    // one read as a normal string desynchronises everything after
                    // it. Blanked whole rather than parsed: no path is spelled
                    // this way, and a missed one is only a missed report.
                    const qsizetype open = source.indexOf(u'(', i + 2);
                    const QString terminator = open < 0 ? QString() : QStringLiteral(")%1\"").arg(source.mid(i + 2, open - i - 2));
                    const qsizetype close = open < 0 ? -1 : source.indexOf(terminator, open);
                    if (close >= 0) {
                        for (const qsizetype end = close + terminator.size(); i < end; ++i) {
                            const QChar skipped = source.at(i);
                            stripped.append(skipped == u'\n' ? skipped : QChar(u' '));
                            if (skipped == u'\n') {
                                ++line;
                            }
                        }
                        --i;
                        continue;
                    }
                }
                if (current == u'"') {
                    state = State::string;
                } else if (current == u'\'') {
                    state = State::character;
                }
                stripped.append(current);
                break;
            case State::string:
            case State::character:
                stripped.append(current);
                if (current == u'\\' && i + 1 < source.size()) {
                    stripped.append(next);
                    ++i;
                    if (next == u'\n') {
                        ++line;
                    }
                    continue;
                }
                if ((state == State::string && current == u'"') || (state == State::character && current == u'\'')) {
                    state = State::code;
                }
                break;
            case State::lineComment:
                if (current == u'\n') {
                    endComment();
                    state = State::code;
                    stripped.append(current);
                } else {
                    comment.append(current);
                    stripped.append(u' ');
                }
                break;
            case State::blockComment:
                if (current == u'*' && next == u'/') {
                    endComment();
                    state = State::code;
                    stripped.append(QStringLiteral("  "));
                    ++i;
                    continue;
                }
                comment.append(current);
                stripped.append(current == u'\n' ? current : QChar(u' '));
                break;
            }
            if (current == u'\n') {
                ++line;
            }
        }
        if (state == State::lineComment || state == State::blockComment) {
            endComment();
        }
        return stripped;
    }

    static int lineOf(const QString& code, qsizetype offset) { return static_cast<int>(QStringView(code).left(offset).count(u'\n')) + 1; }

    // The argument text of every mkpath()/mkdir() call, found by matching
    // parentheses rather than by line, so a call wrapped over several lines and
    // one nesting further calls both come out whole.
    static QVector<DirectoryCreation> directoryCreations(const QString& code)
    {
        static const QRegularExpression call(QStringLiteral("\\b(?:mkpath|mkdir)\\s*\\("));
        QVector<DirectoryCreation> creations;
        auto matches = call.globalMatch(code);
        while (matches.hasNext()) {
            const QRegularExpressionMatch match = matches.next();
            const qsizetype start = match.capturedEnd();
            int depth = 1;
            QChar quote(u'\0');
            qsizetype end = start;
            for (; end < code.size() && depth > 0; ++end) {
                const QChar current = code.at(end);
                if (quote != QChar(u'\0')) {
                    if (current == u'\\') {
                        ++end;
                    } else if (current == quote) {
                        quote = QChar(u'\0');
                    }
                } else if (current == u'"' || current == u'\'') {
                    quote = current;
                } else if (current == u'(') {
                    ++depth;
                } else if (current == u')') {
                    --depth;
                }
            }
            if (depth > 0) {
                continue;
            }
            creations.append({lineOf(code, match.capturedStart()), code.mid(start, end - 1 - start)});
        }
        return creations;
    }

    static bool mentionsConfigRoot(const QString& argument)
    {
        static const QRegularExpression configRoot(QStringLiteral("\"(?:[^\"]*/)?mudlet\""));
        return argument.contains(configRoot);
    }

    // The opt-in either spelled in one literal or assembled from two, so that
    // filePath("profiles") off a config root counts as much as "%1/mudlet/profiles"
    static bool createsOptIn(const QString& argument)
    {
        static const QRegularExpression optIn(QStringLiteral("\"(?:[^\"]*/)?mudlet/profiles(?:/[^\"]*)?\""));
        static const QRegularExpression profiles(QStringLiteral("\"(?:[^\"]*/)?profiles(?:/[^\"]*)?\""));
        return argument.contains(optIn) || (mentionsConfigRoot(argument) && argument.contains(profiles));
    }

    static bool createsConfigRootOnly(const QString& argument) { return mentionsConfigRoot(argument) && !createsOptIn(argument); }

    static QStringList staleRecipes(const QString& source)
    {
        QSet<int> allowedLines;
        const QString code = withoutComments(source, allowedLines);
        const QVector<DirectoryCreation> creations = directoryCreations(code);

        bool optedIn = false;
        for (const DirectoryCreation& creation : creations) {
            if (createsOptIn(creation.argument)) {
                optedIn = true;
                break;
            }
        }
        if (optedIn) {
            return {};
        }

        QStringList problems;
        for (const DirectoryCreation& creation : creations) {
            if (!createsConfigRootOnly(creation.argument) || allowedLines.contains(creation.line) || allowedLines.contains(creation.line - 1)) {
                continue;
            }
            problems.append(QStringLiteral("line %1 creates the config root itself (%2) - create its profiles/ subdirectory instead, that is the opt-in")
                                    .arg(QString::number(creation.line), creation.argument.simplified()));
        }
        return problems;
    }

private slots:
    void test_theStaleRecipeIsFlagged()
    {
        const QString source = QStringLiteral("void initTestCase()\n{\n    QVERIFY(QDir().mkpath(qsl(\"%1/mudlet\").arg(mConfigDir.path())));\n}\n");
        const QStringList problems = staleRecipes(source);
        QCOMPARE(problems.size(), 1);
        QVERIFY2(problems.first().startsWith(QStringLiteral("line 3 ")), qPrintable(problems.first()));
    }

    void test_theCurrentRecipeIsAccepted()
    {
        const QString recipe = QStringLiteral("qsl(\"%1/mudlet/profiles\").arg(mConfigDir.path())");
        QVERIFY(createsOptIn(recipe));
        QVERIFY(!createsConfigRootOnly(recipe));
        const QString source = QStringLiteral("QVERIFY(QDir().mkpath(%1));\n").arg(recipe);
        QVERIFY2(staleRecipes(source).isEmpty(), qPrintable(staleRecipes(source).join(QChar(u'\n'))));
    }

    void test_aProfileUnderTheOptInIsAccepted()
    {
        const QString source = QStringLiteral("QVERIFY(QDir().mkpath(qsl(\"%1/mudlet\").arg(dir)));\nQVERIFY(QDir().mkpath(qsl(\"%1/mudlet/profiles/%2\").arg(dir, name)));\n");
        QVERIFY2(staleRecipes(source).isEmpty(), qPrintable(staleRecipes(source).join(QChar(u'\n'))));
    }

    // The opt-in reached by naming its parts separately, which is as much an
    // opt-in as spelling the whole path in one literal
    void test_theOptInAssembledFromTwoLiteralsCounts()
    {
        const QString relative = QStringLiteral("QVERIFY(QDir(root).mkdir(qsl(\"mudlet\")));\nQVERIFY(QDir(root).mkpath(qsl(\"mudlet/profiles\")));\n");
        QVERIFY2(staleRecipes(relative).isEmpty(), qPrintable(staleRecipes(relative).join(QChar(u'\n'))));

        const QString inOneCall = QStringLiteral("QVERIFY(QDir().mkpath(QDir(qsl(\"%1/mudlet\").arg(dir)).filePath(qsl(\"profiles\"))));\n");
        QVERIFY2(staleRecipes(inOneCall).isEmpty(), qPrintable(staleRecipes(inOneCall).join(QChar(u'\n'))));
    }

    // ProfileDeletionSafetyTest compares the resolved config root against a
    // "%1/mudlet" literal, which creates nothing
    void test_anAssertionOnTheConfigRootIsNotSeeding()
    {
        const QString source = QStringLiteral("QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl(\"%1/mudlet\").arg(mConfigDir.path()));\n");
        QVERIFY2(staleRecipes(source).isEmpty(), qPrintable(staleRecipes(source).join(QChar(u'\n'))));
    }

    void test_theRecipeQuotedInACommentIsNotCode()
    {
        const QString source = QStringLiteral("// never QDir().mkpath(qsl(\"%1/mudlet\").arg(dir))\n/* nor QDir().mkdir(qsl(\"%1/mudlet\")) */\n");
        QVERIFY2(staleRecipes(source).isEmpty(), qPrintable(staleRecipes(source).join(QChar(u'\n'))));
    }

    void test_aRawStringCannotDesynchroniseTheScan()
    {
        const QString source = QStringLiteral("const auto text = R\"(he said \"hi)\";\n"
                                              "// QDir().mkpath(qsl(\"%1/mudlet\").arg(dir))\n"
                                              "QVERIFY(QDir().mkpath(qsl(\"%1/mudlet\").arg(dir)));\n");
        const QStringList problems = staleRecipes(source);
        QCOMPARE(problems.size(), 1);
        QVERIFY2(problems.first().startsWith(QStringLiteral("line 3 ")), qPrintable(problems.first()));
    }

    void test_theOptInElsewhereInTheFileForgivesTheSeed()
    {
        const QString source = QStringLiteral("QVERIFY(QDir().mkpath(qsl(\"%1/mudlet\").arg(dir)));\nQVERIFY(QDir().mkpath(qsl(\"%1/mudlet/profiles\").arg(dir)));\n");
        QVERIFY2(staleRecipes(source).isEmpty(), qPrintable(staleRecipes(source).join(QChar(u'\n'))));
    }

    void test_theAllowTokenExemptsTheCallItSitsOn()
    {
        const QString onTheLine = QStringLiteral("QVERIFY(QDir().mkpath(qsl(\"%1/mudlet\").arg(dir))); // xdg-recipe-guard: allow, the legacy branch is the subject here\n");
        QVERIFY2(staleRecipes(onTheLine).isEmpty(), qPrintable(staleRecipes(onTheLine).join(QChar(u'\n'))));

        const QString aboveTheLine = QStringLiteral("// xdg-recipe-guard: allow, the legacy branch is the subject here\nQVERIFY(QDir().mkpath(qsl(\"%1/mudlet\").arg(dir)));\n");
        QVERIFY2(staleRecipes(aboveTheLine).isEmpty(), qPrintable(staleRecipes(aboveTheLine).join(QChar(u'\n'))));

        const QString twoLinesAbove = QStringLiteral("// xdg-recipe-guard: allow\n\nQVERIFY(QDir().mkpath(qsl(\"%1/mudlet\").arg(dir)));\n");
        QCOMPARE(staleRecipes(twoLinesAbove).size(), 1);
    }

    void test_aMultiLineCallIsStillOneCall()
    {
        const QString source = QStringLiteral("QVERIFY(QDir().mkpath(\n        qsl(\"%1/mudlet\")\n            .arg(mConfigDir.path())));\n");
        const QStringList problems = staleRecipes(source);
        QCOMPARE(problems.size(), 1);
        QVERIFY2(problems.first().startsWith(QStringLiteral("line 1 ")), qPrintable(problems.first()));
    }

    void test_everyTestSourceOptsInTheCurrentWay()
    {
        const QStringList directories = {testDir(), QStringLiteral("%1/functional_tests").arg(testDir())};
        QStringList problems;
        int scanned = 0;
        for (const QString& directory : directories) {
            const QDir dir(directory);
            QVERIFY2(dir.exists(), qPrintable(QStringLiteral("no such directory: %1 - is MUDLET_TEST_DIR right?").arg(directory)));
            const QStringList sources = dir.entryList({QStringLiteral("*.cpp")}, QDir::Files, QDir::Name);
            for (const QString& name : sources) {
                // its own fixtures spell the stale recipe out on purpose
                if (name == QStringLiteral("XdgRecipeConsistencyTest.cpp")) {
                    continue;
                }
                QFile source(dir.filePath(name));
                QVERIFY2(source.open(QIODevice::ReadOnly | QIODevice::Text), qPrintable(source.fileName()));
                ++scanned;
                const QStringList stale = staleRecipes(QString::fromUtf8(source.readAll()));
                for (const QString& problem : stale) {
                    problems.append(QStringLiteral("%1 %2").arg(name, problem));
                }
            }
        }
        QVERIFY2(scanned > 50, qPrintable(QStringLiteral("only %1 sources scanned, so this test would pass whatever they hold").arg(scanned)));
        QVERIFY2(problems.isEmpty(), qPrintable(QStringLiteral("tests seeding the pre-#9712 XDG opt-in, which resolves to the real ~/.config/mudlet:\n%1").arg(problems.join(QChar(u'\n')))));
    }
};

QTEST_GUILESS_MAIN(XdgRecipeConsistencyTest)

#include "XdgRecipeConsistencyTest.moc"
