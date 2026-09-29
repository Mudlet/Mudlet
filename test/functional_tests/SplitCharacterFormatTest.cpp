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

#include "GroupedTest.h"
#include "Host.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "TMainConsole.h"
#include "TTextEdit.h"
#include "TelnetServerStub.h"
#include "mudlet.h"

#include <QPainter>
#include <QTemporaryDir>
#include <QtTest>

// The right half of a split double-byte character is not visible to Lua, so its
// storage and painting are tested here; the decoded text, trigger matching and
// recovery from a broken character are in TBufferEncoding_spec.lua.
class SplitCharacterFormatTest : public QObject
{
    Q_OBJECT
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;

    TBuffer& buffer() const { return mpHost->mpConsole->buffer; }

    void feed(const QByteArray& bytes, const bool fromServer = true)
    {
        std::string data(bytes.constData(), bytes.size());
        buffer().translateToPlainText(data, fromServer);
    }

    int lastTextLine() const
    {
        for (int line = buffer().getLastLineNumber(); line >= 0; --line) {
            if (!buffer().line(line).isEmpty()) {
                return line;
            }
        }
        return -1;
    }

    // Big5 中 with a red on blue left half and a green on yellow right half:
    static QByteArray splitCharacter() { return QByteArray("\033[31;44m\xa4\033[32;43m\xa4"); }

    const TChar& splitCharacterOnNewLine()
    {
        feed(splitCharacter() + "\033[0m\n");
        const int line = lastTextLine();
        return buffer().buffer.at(line).front();
    }

private slots:
    void initTestCase()
    {
        QVERIFY(!portableMarkerPresent());
        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(mConfigDir.path() + qsl("/mudlet/profiles")));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());
        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        mpServer = new TelnetServerStub(qApp);
        mpServer->start(qsl("localhost"), 0);
        QVERIFY(mpServer->isListening());
        mpHost = TestProfile::create(qsl("Test-SplitCharacterFormat"), qsl("localhost"), QString::number(mpServer->serverPort()));
        QVERIFY(mpHost);
        QVERIFY(mpHost->mpConsole);
    }

    void cleanupTestCase()
    {
        delete mudlet::self();
        delete mpServer;
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void init()
    {
        QVERIFY(mpHost->mTelnet.setEncoding("BIG5", false).first);
        buffer().resetSequenceParserState();
        feed("\033[0m\n");
    }

    void halvesSurviveEveryPacketBoundary_data()
    {
        QTest::addColumn<QByteArray>("encoding");
        QTest::addColumn<QByteArray>("bytes");
        QTest::addColumn<QString>("text");
        QTest::newRow("Big5") << QByteArray("BIG5") << QByteArray("\xa4\xa4") << qsl("中");
        QTest::newRow("Big5-HKSCS") << QByteArray("BIG5-HKSCS") << QByteArray("\xa7\x41") << qsl("你");
        QTest::newRow("GBK") << QByteArray("GBK") << QByteArray("\xd6\xd0") << qsl("中");
        QTest::newRow("GB18030") << QByteArray("GB18030") << QByteArray("\xd6\xd0") << qsl("中");
        QTest::newRow("EUC-KR") << QByteArray("EUC-KR") << QByteArray("\xc7\xd1") << qsl("한");
    }

    void halvesSurviveEveryPacketBoundary()
    {
        QFETCH(QByteArray, encoding);
        QFETCH(QByteArray, bytes);
        QFETCH(QString, text);
        QVERIFY(mpHost->mTelnet.setEncoding(encoding, false).first);
        const QByteArray data = QByteArray("\033[38;2;255;0;0;48;2;0;0;64m") + bytes.left(1) + "\033[38:2::0:255:0m\033[48;2;64;0;0m" + bytes.mid(1) + "X\033[0m\n";
        for (qsizetype cut = 0; cut <= data.size(); ++cut) {
            feed(data.left(cut));
            feed(data.mid(cut));
            const int line = lastTextLine();
            QCOMPARE(buffer().line(line), text + qsl("X"));
            const auto& characters = buffer().buffer.at(line);
            QCOMPARE(characters.size(), size_t(2));
            const TChar& character = characters.front();
            QVERIFY2(character.hasSplitFormat(), qPrintable(qsl("cut at %1").arg(cut)));
            QCOMPARE(character.foreground(), QColor(255, 0, 0));
            QCOMPARE(character.background(), QColor(0, 0, 64));
            QCOMPARE(character.rightHalf().foreground(), QColor(0, 255, 0));
            QCOMPARE(character.rightHalf().background(), QColor(64, 0, 0));
            // The rendition carries on in one colour:
            QVERIFY(!characters.back().hasSplitFormat());
            QCOMPARE(characters.back().foreground(), QColor(0, 255, 0));
        }
    }

    void halvesSurviveAPartialLineFlush()
    {
        // cTelnet ends a chunk with a carriage return to show a line that is still arriving:
        feed(QByteArray("\033[31m\xa4\033[32m\r"));
        feed(QByteArray("\xa4\033[0m\n"));
        const int line = lastTextLine();
        QCOMPARE(buffer().line(line), qsl("中"));
        QVERIFY(buffer().buffer.at(line).front().hasSplitFormat());
    }

    void unchangedRenditionIsNotSplit()
    {
        feed(QByteArray("\033[31m\xa4\033[31m\xa4\033[0m\n"));
        QVERIFY(!buffer().buffer.at(lastTextLine()).front().hasSplitFormat());
    }

    void localFeedLeavesThePendingLeadAlone()
    {
        feed(QByteArray("\033[31m\xa4\033[32m\r"));
        // Every byte of this is a valid Big5 trail byte:
        feed("local\n", false);
        feed(QByteArray("\xa4\033[0m\n"));
        const TChar& character = buffer().buffer.at(lastTextLine()).front();
        QCOMPARE(buffer().line(lastTextLine()), qsl("中"));
        QVERIFY(character.hasSplitFormat());
        QCOMPARE(character.foreground(), mpHost->mRed);
        QCOMPARE(character.rightHalf().foreground(), mpHost->mGreen);
    }

    void recolouringAppliesToBothHalves()
    {
        const TChar& character = splitCharacterOnNewLine();
        const int line = lastTextLine();
        QVERIFY(buffer().applyAttribute(QPoint(0, line), QPoint(1, line), TChar::Bold, true));
        QVERIFY(character.isBold());
        QVERIFY(character.rightHalf().isBold());
        QVERIFY(buffer().applyFgColor(QPoint(0, line), QPoint(1, line), Qt::white));
        QCOMPARE(character.rightHalf().foreground(), QColor(Qt::white));
        QVERIFY2(character.hasSplitFormat(), "the backgrounds still differ");
        QVERIFY(buffer().applyBgColor(QPoint(0, line), QPoint(1, line), Qt::black));
        QVERIFY2(!character.hasSplitFormat(), "both halves are now alike");
    }

    void searchHighlightClearsFromBothHalves()
    {
        const TChar& character = splitCharacterOnNewLine();
        const int line = lastTextLine();
        QVERIFY(buffer().applyAttribute(QPoint(0, line), QPoint(1, line), TChar::Found, true));
        QVERIFY(character.rightHalf().isFound());
        buffer().clearSearchHighlights();
        QVERIFY(!character.isFound());
        QVERIFY(!character.rightHalf().isFound());
        QVERIFY(character.hasSplitFormat());
    }

    void selectionCoversBothHalves()
    {
        TChar character(splitCharacterOnNewLine());
        character.select();
        QVERIFY(character.rightHalf().isSelected());
    }

    void copyingKeepsTheRightHalf()
    {
        feed("paste here\n");
        const int target = lastTextLine();
        splitCharacterOnNewLine();
        const int line = lastTextLine();
        QPoint from(0, line);
        QPoint to(1, line);
        const TBuffer slice = buffer().copy(from, to);
        QVERIFY(slice.buffer.at(0).front().hasSplitFormat());

        QPoint at(0, target);
        buffer().paste(at, slice);
        QCOMPARE(buffer().line(target), qsl("中paste here"));
        QVERIFY2(buffer().buffer.at(target).front().hasSplitFormat(), "paste() lost it");

        buffer().appendBuffer(slice);
        QVERIFY2(buffer().buffer.at(lastTextLine()).front().hasSplitFormat(), "appendBuffer() lost it");
    }

    void paintsEachHalfInItsOwnColours()
    {
        auto* pane = mpHost->mpConsole->mUpperPane;
        const TChar& character = splitCharacterOnNewLine();
        TTextEdit::LineLayout layout;
        QCOMPARE(pane->layoutGrapheme(layout, QPoint(0, 0), qsl("中"), 0, -1, character), 2);
        QCOMPARE(layout.size(), size_t(2));

        QImage image(2 * pane->mFontWidth, pane->mFontHeight, QImage::Format_RGB32);
        image.fill(Qt::black);
        QPainter painter(&image);
        painter.setFont(pane->font());
        pane->paintBackgrounds(painter, layout);
        pane->paintForegrounds(painter, layout);
        painter.end();

        // Anti-aliased ink blends its colour into the cell's background, so it
        // lies between the two in every channel - unless it is the other half's:
        const auto checkCell = [&image, pane](const int fromX, const QColor& ink, const QColor& background) {
            bool inked = false;
            for (int y = 0; y < image.height(); ++y) {
                for (int x = fromX; x < fromX + pane->mFontWidth; ++x) {
                    const QColor pixel = image.pixelColor(x, y);
                    inked |= pixel != background;
                    const auto between = [](int value, int one, int other) {
                        return value >= qMin(one, other) && value <= qMax(one, other);
                    };
                    if (!between(pixel.red(), ink.red(), background.red()) || !between(pixel.green(), ink.green(), background.green()) || !between(pixel.blue(), ink.blue(), background.blue())) {
                        return qsl("%1 at (%2, %3)").arg(pixel.name()).arg(x).arg(y);
                    }
                }
            }
            return inked ? QString() : qsl("no ink");
        };
        QCOMPARE(image.pixelColor(0, 0), mpHost->mBlue);
        QCOMPARE(image.pixelColor(pane->mFontWidth, 0), mpHost->mYellow);
        QCOMPARE(checkCell(0, mpHost->mRed, mpHost->mBlue), QString());
        QCOMPARE(checkCell(pane->mFontWidth, mpHost->mGreen, mpHost->mYellow), QString());
    }
};

#include "SplitCharacterFormatTest.moc"
MUDLET_GROUPED_TEST_MAIN(SplitCharacterFormatTest)
