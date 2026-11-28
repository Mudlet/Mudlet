/***************************************************************************
 *   Copyright (C) 2026 by Mike Conley - mike.conley@stickmud.com          *
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

#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "Host.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "TBuffer.h"
#include "TTextSelection.h"
#include "TelnetServerStub.h"
#include "mudlet.h"

#include "GroupedTest.h"

class TextSelectionTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    const QString mpHostname = qsl("Test-TextSelection");
    QString mpPort;
    const QString mpLocalhost = qsl("localhost");

    static void print(TBuffer& buffer, const QString& text) { buffer.append(text, 0, text.size(), Qt::white, Qt::black); }

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
    }

    void cleanupTestCase() { mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg); }

    void init()
    {
        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mpLocalhost, 0);
        mpPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(MudletApp::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        TestProfile::removeProfileDirectory(mpHostname);
    }

    void cleanup()
    {
        delete mudlet::self();
        delete mpServer;
        mpServer = nullptr;
        TestProfile::removeProfileDirectory(mpHostname);
    }

    void test_aBackwardDragIsOrderedTopLeftFirst()
    {
        Host* pHost = TestProfile::create(mpHostname, mpLocalhost, mpPort);
        QVERIFY(pHost);
        TBuffer buffer(pHost);
        TTextSelection selection(buffer, pHost);
        selection.mDragStart = QPoint(5, 1);
        selection.mDragSelectionEnd = QPoint(2, 0);
        selection.normalise();
        QCOMPARE(selection.mPA, QPoint(2, 0));
        QCOMPARE(selection.mPB, QPoint(5, 1));

        selection.mDragStart = QPoint(7, 3);
        selection.mDragSelectionEnd = QPoint(4, 3);
        selection.normalise();
        QCOMPARE(selection.mPA, QPoint(4, 3));
        QCOMPARE(selection.mPB, QPoint(7, 3));
    }

    void test_aWordStopsAtSpacesAndTheIgnoredCharacters()
    {
        Host* pHost = TestProfile::create(mpHostname, mpLocalhost, mpPort);
        QVERIFY(pHost);
        pHost->mDoubleClickIgnore = {QChar('\'')};
        TBuffer buffer(pHost);
        print(buffer, qsl("say 'hello' now\n"));
        TTextSelection selection(buffer, pHost);
        selection.mPA = QPoint(6, 0);
        selection.mPB = QPoint(6, 0);
        selection.expandToWords();
        QCOMPARE(selection.mPA, QPoint(5, 0));
        QCOMPARE(selection.mPB, QPoint(9, 0));
        QCOMPARE(selection.text(), qsl("hello"));
    }

    void test_theTextRejoinsALineWordWrappingSplit()
    {
        Host* pHost = TestProfile::create(mpHostname, mpLocalhost, mpPort);
        QVERIFY(pHost);
        TBuffer buffer(pHost);
        buffer.setWrapAt(16);
        print(buffer, qsl("the quick brown fox jumps over the lazy dog\n"));
        const int lastRow = static_cast<int>(buffer.lineBuffer.size()) - 2;
        QVERIFY2(lastRow > 0 && buffer.wrapsFromPreviousLine(lastRow), "the text did not wrap");
        TTextSelection selection(buffer, pHost);
        selection.mPA = QPoint(0, 0);
        selection.mPB = QPoint(static_cast<int>(buffer.lineBuffer.at(lastRow).size()) - 1, lastRow);
        QCOMPARE(selection.text(), qsl("the quick brown fox jumps over the lazy dog"));
    }

    // Lines trimmed off the front leave mPA and mPB on their old indexes, past the end of the buffer
    void test_theTextCatchesUpWithLinesTrimmedOffTheFront()
    {
        Host* pHost = TestProfile::create(mpHostname, mpLocalhost, mpPort);
        QVERIFY(pHost);
        TBuffer buffer(pHost);
        print(buffer, qsl("a\nb\nc\nd\ne\n"));
        buffer.mBatchDeleteSize = 2;
        TTextSelection selection(buffer, pHost);
        selection.mPA = QPoint(0, 6);
        selection.mPB = QPoint(0, 6);
        selection.mPBTakingNewline = selection.mPB;
        QCOMPARE(selection.linesTrimmed(), 2);
        QCOMPARE(selection.text(), qsl("e\n"));
        QCOMPARE(selection.mPA, QPoint(0, 4));
        QCOMPARE(selection.mPBTakingNewline, selection.mPB);
        QCOMPARE(selection.linesTrimmed(), 0);
    }

    void test_theAccessibleRangeCountsANewlineAfterEachLine()
    {
        Host* pHost = TestProfile::create(mpHostname, mpLocalhost, mpPort);
        QVERIFY(pHost);
        TBuffer buffer(pHost);
        print(buffer, qsl("ab\ncde\nf\n"));
        TTextSelection selection(buffer, pHost);
        selection.mPA = QPoint(1, 0);
        selection.mPB = QPoint(1, 1);
        QCOMPARE(selection.accessibleRange(), (std::pair<int, int>{0, 0}));
        selection.mSelectedRegion = QRegion(0, 0, 1, 1);
        QCOMPARE(selection.accessibleRange(), (std::pair<int, int>{1, 5}));
        QCOMPARE(selection.text(), qsl("b\ncd"));
    }

    void test_crossedEndpointsAreNoSelection()
    {
        Host* pHost = TestProfile::create(mpHostname, mpLocalhost, mpPort);
        QVERIFY(pHost);
        TBuffer buffer(pHost);
        print(buffer, qsl("a word\n"));
        TTextSelection selection(buffer, pHost);
        selection.mSelectedRegion = QRegion(0, 0, 1, 1);
        selection.mPA = QPoint(2, 0);
        selection.mPB = QPoint(5, 0);
        QVERIFY(selection.hasSelection());
        selection.mPA = QPoint(2, 0);
        selection.mPB = QPoint(1, 0);
        QVERIFY(!selection.hasSelection());
    }
};

#include "TextSelectionTest.moc"
MUDLET_GROUPED_TEST_MAIN(TextSelectionTest)
