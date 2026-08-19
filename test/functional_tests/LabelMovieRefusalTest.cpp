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
 * A setMovie() that refuses a file must leave the movie the label was already
 * playing alone. The gif count, startMovie() and setMovieFrame() all answer the
 * same for a label left holding a movie with no frames in it, so the QMovie is
 * what tells the two states apart.
 *
 * Run with: ctest -R LabelMovieRefusalTest -V
 */

#include <QMovie>
#include <QSignalSpy>
#include <QtTest/QtTest>
#include <chrono>

#include "PortableModeTestHelper.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TLabel.h"
#include "TMainConsole.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class LabelMovieRefusalTest : public QObject
{
    Q_OBJECT

private:
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mHostname = qsl("LabelMovieRefusal-Test-Host");
    QString mPort;
    const QString mLocalhost = qsl("localhost");
    const QString mLabelName = qsl("movieRefusalLabel");
    QTemporaryDir mFixtureDir;
    // a configuration directory of its own, so the profile this opens is never
    // one of the developer's
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdgConfigHome;
    QString mGifPath;
    QString mNotAGifPath;

    // three frames so the count is a distinctive thing to compare, and a 60
    // second frame delay so the animation never advances between two reads
    static QByteArray threeFrameGif()
    {
        QByteArray gif("GIF89a");
        gif.append(QByteArray::fromHex("01000100910000"));
        gif.append(QByteArray::fromHex("ff000000ff000000ff000000"));
        const QByteArray frame = QByteArray::fromHex("21f90400701700002c00000000010001000002024c0100");
        gif.append(frame).append(frame).append(frame);
        gif.append(QByteArray::fromHex("3b"));
        return gif;
    }

    static bool writeFixture(const QString& path, const QByteArray& contents)
    {
        QFile file(path);
        return file.open(QIODevice::WriteOnly) && file.write(contents) == contents.size();
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - cannot redirect the config dir for this test");
        }
        QVERIFY(mConfigDir.isValid());
        // setupConfig() only adopts $XDG_CONFIG_HOME once the profiles
        // directory under it is there to be adopted
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdgConfigHome = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        QVERIFY(mFixtureDir.isValid());
        mGifPath = qsl("%1/movie.gif").arg(mFixtureDir.path());
        mNotAGifPath = qsl("%1/notamovie.gif").arg(mFixtureDir.path());
        QVERIFY(writeFixture(mGifPath, threeFrameGif()));
        QVERIFY(writeFixture(mNotAGifPath, QByteArray("this is not a GIF at all")));
        QVERIFY2(QMovie(mGifPath).isValid(), "the generated fixture is not a movie Qt can read");

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0);
        QVERIFY2(mpServer->serverPort() != 0, "the telnet stub did not start listening");
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::getQSettings()->setValue(qsl("uiTourShown"), true);
        mudlet::getQSettings()->sync();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);

        const QString path = mudlet::getMudletPath(enums::profileHomePath, mHostname);
        QDir(path).removeRecursively();

        QTimer::singleShot(0ms, qApp, [this]() {
            mudlet::self()->startAutoLogin({});
            QTest::qWait(100ms);
            QTest::mouseClick(mudlet::self()->mpConnectionDialog->new_profile_button, Qt::LeftButton);
            QTest::qWait(100ms);
            QTest::keyClicks(QApplication::focusWidget(), mHostname);
            QTest::qWait(100ms);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
            QTest::qWait(100ms);
            QTest::keyClicks(QApplication::focusWidget(), mLocalhost);
            QTest::qWait(100ms);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
            QTest::qWait(100ms);
            QTest::keyClicks(QApplication::focusWidget(), mPort);
            QTest::qWait(100ms);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Return);
        });

        QSignalSpy spy(mudlet::self(), &mudlet::signal_profileLoaded);
        if (!spy.wait(1000)) {
            QFAIL("Profile took too long to load.");
        }
        mpHost = mudlet::self()->getActiveHost();
        if (!mpHost) {
            QFAIL("No active host available for the test.");
        }
    }

    void cleanupTestCase()
    {
        delete mpServer;
        mpServer = nullptr;
        mpHost = nullptr;
        // Null when initTestCase skipped or failed ahead of mudlet::start(), and
        // getMudletPath() dereferences the instance rather than checking it
        if (mudlet::self()) {
            const QString path = mudlet::getMudletPath(enums::profileHomePath, mHostname);
            QDir(path).removeRecursively();
            delete mudlet::self();
        }
        if (mSavedXdgConfigHome.isEmpty()) {
            qunsetenv("XDG_CONFIG_HOME");
        } else {
            qputenv("XDG_CONFIG_HOME", mSavedXdgConfigHome);
        }
    }

    void test_aRefusedMovieLeavesTheOneTheLabelWasPlaying()
    {
        auto [created, createMessage] = mpHost->createLabel(qsl("main"), mLabelName, 10, 10, 60, 30, true, false);
        QVERIFY2(created, qPrintable(createMessage));

        auto [loaded, loadMessage] = mpHost->setMovie(mLabelName, mGifPath);
        QVERIFY2(loaded, qPrintable(loadMessage));

        TLabel* pLabel = mpHost->mpConsole->mLabelMap.value(mLabelName);
        QVERIFY(pLabel);
        QVERIFY(pLabel->mpMovie);
        QVERIFY2(pLabel->mpMovie->isValid(), "the label did not end up with a movie it can play");
        const int frameCount = pLabel->mpMovie->frameCount();
        QVERIFY(frameCount > 0);

        auto [refused, refusalMessage] = mpHost->setMovie(mLabelName, mNotAGifPath);
        QVERIFY2(!refused, "setMovie() accepted a file that is not a movie");
        QCOMPARE(refusalMessage, qsl("no valid movie found at '%1'").arg(mNotAGifPath));

        QVERIFY(pLabel->mpMovie);
        // a refusal must not hand the label's own QMovie the new path: that
        // would leave it invalid, renamed after a file that is not a movie, and
        // with no frames
        QVERIFY2(pLabel->mpMovie->isValid(), "the refused file left the label with a movie it cannot play");
        QCOMPARE(pLabel->mpMovie->fileName(), mGifPath);
        QCOMPARE(pLabel->mpMovie->frameCount(), frameCount);
        // the label has to still be playing, which a QMovie handed a file that
        // is not one also reports, so this only holds the line
        QCOMPARE(pLabel->mpMovie->state(), QMovie::Running);

        mpHost->mpConsole->deleteLabel(mLabelName);
    }
};

#include "LabelMovieRefusalTest.moc"
MUDLET_GROUPED_TEST_MAIN(LabelMovieRefusalTest)
