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

#include "../src/updater/SemVer.h"

#include <QtTest/QtTest>

/*
 * Covers the version comparison every update offer is decided by. Release's
 * operators delegate to SemVer, so which release the updater treats as newer -
 * and therefore whether a user is offered an update at all - comes down to what
 * is asserted here.
 *
 * Two of these properties have already cost a release. A version SemVer cannot
 * read makes Release fall back to comparing build dates, so tagging a release
 * "Mudlet-5.0" rather than "Mudlet-5.0.0" silently stops it being offered by
 * version at all (CI/check-release-tag.sh now rejects such a tag). And the
 * components are numbers rather than text, so 4.22.0 has to outrank 4.9.0 -
 * comparing the version as a string gets that backwards, and would strand every
 * user on the older release.
 *
 * Built from src/updater/SemVer.cpp rather than linked against the Mudlet
 * library, because the library only contains it when configured with
 * USE_UPDATER - and CI's -testing builds are configured without it. Like
 * ReleaseChangelogSpanTest this file includes no utils.h of its own, so qsl()
 * is out of scope here and the literals below are QStringLiteral. SemVer.cpp
 * does include it, which is what the target's Qt6::Gui link is for.
 *
 * Run with: ctest -R SemVerTest -V
 */

namespace {
dblsqd::SemVer version(const QString& text)
{
    return dblsqd::SemVer(text);
}

// Both directions of a comparison, since an ordering that says "a is older than
// b" and "b is older than a" at once is as broken as one that says neither
void assertOlder(const QString& older, const QString& newer)
{
    QVERIFY2(version(older) < version(newer), qPrintable(QStringLiteral("%1 should sort below %2").arg(older, newer)));
    QVERIFY2(!(version(newer) < version(older)), qPrintable(QStringLiteral("%1 should not sort below %2").arg(newer, older)));
    QVERIFY2(!(version(older) == version(newer)), qPrintable(QStringLiteral("%1 and %2 should not be equal").arg(older, newer)));
}
} // namespace

class SemVerTest : public QObject
{
    Q_OBJECT

private slots:
    void aThreeComponentVersionIsReadable();
    void aTwoComponentVersionIsNotSemVerAtAll();
    void malformedVersionsAreRejected();
    void trailingCharactersAreNotPartOfAVersion();
    void aComponentTooLargeToStoreIsNotAVersion();
    void componentsAreComparedAsNumbersAndNotAsText();
    void majorOutranksMinorOutranksPatch();
    void aPrereleaseSortsBelowTheReleaseItLeadsTo();
    void ptbBuildsOfOneVersionAreOrderedByTheirDate();
    void buildMetadataDoesNotChangeAVersion();
    void buildMetadataCannotBeEmpty();
    void anUnreadableVersionNeverCompares();
};

void SemVerTest::aThreeComponentVersionIsReadable()
{
    QVERIFY(version(QStringLiteral("1.2.3")).isValid());
    QVERIFY(version(QStringLiteral("0.0.0")).isValid());
    QVERIFY(version(QStringLiteral("4.22.0")).isValid());
    QVERIFY(version(QStringLiteral("4.22.0-ptb-2026-08-08-7c6b5a49")).isValid());
}

// The shape of tag that stopped 4.22.0 reaching anyone by version: two
// components read as no version at all, so Release falls back to build dates
void SemVerTest::aTwoComponentVersionIsNotSemVerAtAll()
{
    QVERIFY(!version(QStringLiteral("4.22")).isValid());
    QVERIFY(!version(QStringLiteral("5.0")).isValid());
    QVERIFY(!version(QStringLiteral("5.0-ptb-2026-08-08-7c6b5a49")).isValid());
}

void SemVerTest::malformedVersionsAreRejected()
{
    QVERIFY(!version(QString()).isValid());
    QVERIFY(!version(QStringLiteral("v1.2.3")).isValid());
    QVERIFY(!version(QStringLiteral("Mudlet-1.2.3")).isValid());
    QVERIFY(!version(QStringLiteral("1.2.3.4")).isValid());
    QVERIFY(!version(QStringLiteral("1.2.3-")).isValid());
    // Leading zeroes are a different version to SemVer, so they are not one at all
    QVERIFY(!version(QStringLiteral("01.2.3")).isValid());
    QVERIFY(!version(QStringLiteral("1.02.3")).isValid());
    QVERIFY(!version(QStringLiteral("1.2.03")).isValid());
}

// A version is the whole of the string or it is not that version at all. The
// regexp is what decides this, and a '$' anchor there matches just before a
// trailing newline as well as at the end - so a version with a line ending left on
// it would read as valid and then be offered under a spelling nothing is published
// under.
void SemVerTest::trailingCharactersAreNotPartOfAVersion()
{
    QVERIFY(!version(QStringLiteral("1.2.3\n")).isValid());
    QVERIFY(!version(QStringLiteral("1.2.3-ptb-2026-08-08-7c6b5a49\n")).isValid());
    QVERIFY(!version(QStringLiteral("1.2.3+build.1\n")).isValid());
    QVERIFY(!version(QStringLiteral("1.2.3\r\n")).isValid());
    QVERIFY(!version(QStringLiteral("1.2.3 ")).isValid());
    QVERIFY(!version(QStringLiteral("\n1.2.3")).isValid());
}

// A component is read into an int, and a number too big for one reads back as 0 -
// so without a range check the largest version there is sorts below every other
// one instead of above them, and whichever side of the comparison it lands on gets
// the update decision backwards
void SemVerTest::aComponentTooLargeToStoreIsNotAVersion()
{
    QVERIFY(!version(QStringLiteral("2147483648.0.0")).isValid());
    QVERIFY(!version(QStringLiteral("0.2147483648.0")).isValid());
    QVERIFY(!version(QStringLiteral("0.0.2147483648")).isValid());
    QVERIFY(!version(QStringLiteral("99999999999.0.0")).isValid());

    // the largest one that does fit is still a version, and still ordered as a number
    QVERIFY(version(QStringLiteral("2147483647.0.0")).isValid());
    assertOlder(QStringLiteral("2147483646.0.0"), QStringLiteral("2147483647.0.0"));

    // and one that does not fit compares with nothing, rather than sorting below
    // the versions it is larger than
    QVERIFY(!(version(QStringLiteral("2147483648.0.0")) < version(QStringLiteral("2147483647.0.0"))));
    QVERIFY(!(version(QStringLiteral("2147483647.0.0")) < version(QStringLiteral("2147483648.0.0"))));
    QVERIFY(!(version(QStringLiteral("99999999999.0.0")) < version(QStringLiteral("1.2.3"))));
}

// Text ordering puts "4.22.0" below "4.9.0", which would offer every 4.22.0 user
// a downgrade and leave every 4.9.0 user believing they are current
void SemVerTest::componentsAreComparedAsNumbersAndNotAsText()
{
    assertOlder(QStringLiteral("4.9.0"), QStringLiteral("4.22.0"));
    assertOlder(QStringLiteral("1.2.9"), QStringLiteral("1.2.10"));
    assertOlder(QStringLiteral("9.0.0"), QStringLiteral("10.0.0"));
}

void SemVerTest::majorOutranksMinorOutranksPatch()
{
    assertOlder(QStringLiteral("1.9.9"), QStringLiteral("2.0.0"));
    assertOlder(QStringLiteral("1.2.9"), QStringLiteral("1.3.0"));
    assertOlder(QStringLiteral("1.2.3"), QStringLiteral("1.2.4"));

    QVERIFY(version(QStringLiteral("1.2.3")) == version(QStringLiteral("1.2.3")));
    QVERIFY(!(version(QStringLiteral("1.2.3")) < version(QStringLiteral("1.2.3"))));
}

// A public test build is a prerelease of the version it carries, so somebody
// running 5.0.0-ptb-... is behind 5.0.0 and has to be offered it
void SemVerTest::aPrereleaseSortsBelowTheReleaseItLeadsTo()
{
    assertOlder(QStringLiteral("5.0.0-ptb-2026-08-08-7c6b5a49"), QStringLiteral("5.0.0"));
    assertOlder(QStringLiteral("1.0.0-alpha"), QStringLiteral("1.0.0"));
    assertOlder(QStringLiteral("4.22.0"), QStringLiteral("5.0.0-ptb-2026-08-08-7c6b5a49"));
}

// The date in a PTB tag is zero-padded and most-significant first, so ordering
// the prerelease identifier as text orders the builds chronologically
void SemVerTest::ptbBuildsOfOneVersionAreOrderedByTheirDate()
{
    assertOlder(QStringLiteral("4.22.0-ptb-2026-08-07-eaa991b9"), QStringLiteral("4.22.0-ptb-2026-08-08-7c6b5a49"));
    assertOlder(QStringLiteral("4.22.0-ptb-2026-07-31-1a2b3c4d"), QStringLiteral("4.22.0-ptb-2026-08-01-dfdcb137"));
    assertOlder(QStringLiteral("4.22.0-ptb-2025-12-31-1a2b3c4d"), QStringLiteral("4.22.0-ptb-2026-01-01-dfdcb137"));
}

// Build metadata identifies the build, not the version, so two builds of one
// version are the same version and neither is an update to the other
void SemVerTest::buildMetadataDoesNotChangeAVersion()
{
    QVERIFY(version(QStringLiteral("1.2.3+build.1")).isValid());
    QVERIFY(version(QStringLiteral("1.2.3+build.1")) == version(QStringLiteral("1.2.3+build.2")));
    QVERIFY(version(QStringLiteral("1.2.3+build.1")) == version(QStringLiteral("1.2.3")));
    QVERIFY(!(version(QStringLiteral("1.2.3+build.1")) < version(QStringLiteral("1.2.3+build.2"))));
    QVERIFY(!(version(QStringLiteral("1.2.3+build.2")) < version(QStringLiteral("1.2.3+build.1"))));

    // The prerelease still counts when metadata is attached to it
    assertOlder(QStringLiteral("1.2.3-alpha+build.1"), QStringLiteral("1.2.3+build.1"));
}

// SemVer 2.0 has build metadata as one or more identifiers of at least one
// character each, so a '+' with nothing after it is a typo rather than a version
void SemVerTest::buildMetadataCannotBeEmpty()
{
    QVERIFY(!version(QStringLiteral("1.2.3+")).isValid());
    QVERIFY(!version(QStringLiteral("1.2.3+.build")).isValid());
    QVERIFY(!version(QStringLiteral("1.2.3+build.")).isValid());
    QVERIFY(!version(QStringLiteral("1.2.3+build..1")).isValid());
    QVERIFY(!version(QStringLiteral("1.2.3-alpha+")).isValid());

    // metadata that is actually there stays acceptable
    QVERIFY(version(QStringLiteral("1.2.3+build")).isValid());
    QVERIFY(version(QStringLiteral("1.2.3+build.1.2")).isValid());
    QVERIFY(version(QStringLiteral("1.2.3+21AF26D3----117B344092BD")).isValid());
    QVERIFY(version(QStringLiteral("1.2.3-alpha+build")).isValid());
}

// std::sort needs a strict weak ordering, and an unreadable version has no place
// in one - so it compares below nothing, above nothing and equal to nothing, its
// own spelling included. Release::operator< and operator== exist to give such a
// version a total order anyway, by date and by text respectively.
void SemVerTest::anUnreadableVersionNeverCompares()
{
    const auto unreadable = QStringLiteral("4.22");
    const auto readable = QStringLiteral("4.22.0");

    QVERIFY(!(version(unreadable) < version(readable)));
    QVERIFY(!(version(readable) < version(unreadable)));
    QVERIFY(!(version(unreadable) == version(readable)));

    QVERIFY(!(version(unreadable) < version(unreadable)));
    QVERIFY(!(version(unreadable) == version(unreadable)));
}

QTEST_GUILESS_MAIN(SemVerTest)

#include "SemVerTest.moc"
