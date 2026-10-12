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
#include "TEncodingHelper.h"
#include "TGlyphCache.h"
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

    TBuffer& buffer() const { return mpHost->mainConsoleView()->buffer; }

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
        QVERIFY(mpHost->mainConsoleView());
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
        QTest::newRow("Shift JIS") << QByteArray("SHIFT_JIS") << QByteArray("\x93\xfa") << qsl("日");
        QTest::newRow("EUC-JP") << QByteArray("EUC-JP") << QByteArray("\xc6\xfc") << qsl("日");
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
        TBuffer slice(mpHost);
        buffer().copyInto(from, to, slice);
        QVERIFY(slice.buffer.at(0).front().hasSplitFormat());

        QPoint at(0, target);
        buffer().paste(at, slice);
        QCOMPARE(buffer().line(target), qsl("中paste here"));
        QVERIFY2(buffer().buffer.at(target).front().hasSplitFormat(), "paste() lost it");

        buffer().appendBuffer(slice);
        QVERIFY2(buffer().buffer.at(lastTextLine()).front().hasSplitFormat(), "appendBuffer() lost it");
    }

    // Every rendition an SGR can give a character must reach the right half as
    // it would reach an ordinary character, whichever packet boundary falls in it:
    void sgrAttributesMatchUnsplitCharacters()
    {
        const QList<QPair<QByteArray, QByteArray>> characters = {
                {"BIG5", "\xa4\xa4"}, {"BIG5-HKSCS", "\xa7\x41"}, {"GBK", "\xd6\xd0"}, {"GB18030", "\xd6\xd0"}, {"EUC-KR", "\xc7\xd1"}, {"SHIFT_JIS", "\x93\xfa"}, {"EUC-JP", "\xc6\xfc"}};
        const QList<QPair<QByteArray, QByteArray>> renditions = {{"1", "22"},
                                                                 {"1", "31"},
                                                                 {"31;1", "39"},
                                                                 {"31;1", "22;32"},
                                                                 {"91", "32"},
                                                                 {"3", "23"},
                                                                 {"4", "24"},
                                                                 {"5", "25"},
                                                                 {"6", "25"},
                                                                 {"5", "6"},
                                                                 {"7", "27"},
                                                                 {"8", "28"},
                                                                 {"9", "29"},
                                                                 {"53", "55"},
                                                                 {"4:3", "24"},
                                                                 {"4:4", "24"},
                                                                 {"4:5", "24"},
                                                                 {"1;3;4;5;7;9;53", "0"}};
        for (const auto& [encoding, bytes] : characters) {
            QVERIFY(mpHost->mTelnet.setEncoding(encoding, false).first);
            for (const auto& [before, between] : renditions) {
                const QByteArray first = "\033[0m\033[" + before + "m";
                const QByteArray second = "\033[" + between + "m";
                // The same two renditions on two whole characters are the reference:
                feed(first + bytes + second + bytes + "\033[0mX\n");
                const auto reference = buffer().buffer.at(lastTextLine());
                QVERIFY(!(reference.at(0) == reference.at(1)));
                const QByteArray data = first + bytes.left(1) + second + bytes.mid(1) + "Y\033[0mX\n";
                for (qsizetype cut = 0; cut <= data.size(); ++cut) {
                    feed(data.left(cut));
                    feed(data.mid(cut));
                    const auto& actual = buffer().buffer.at(lastTextLine());
                    QCOMPARE(actual.size(), size_t(3));
                    const TChar right = actual.at(0).rightHalf();
                    QCOMPARE(actual.at(0).foregroundRgba(), reference.at(0).foregroundRgba());
                    QCOMPARE(actual.at(0).backgroundRgba(), reference.at(0).backgroundRgba());
                    QCOMPARE(actual.at(0).allDisplayAttributes(), reference.at(0).allDisplayAttributes());
                    QCOMPARE(right.foregroundRgba(), reference.at(1).foregroundRgba());
                    QCOMPARE(right.backgroundRgba(), reference.at(1).backgroundRgba());
                    QCOMPARE(right.allDisplayAttributes(), reference.at(1).allDisplayAttributes());
                    QVERIFY(actual.at(1) == reference.at(1));
                    QVERIFY(actual.at(2) == reference.at(2));
                }
            }
        }
    }

    void sgrResetKeepsEachHalfsBoldness()
    {
        const QByteArray glyph = QByteArray("\033[1m\xa4\033[0m\xa4");
        const QByteArray data = QByteArray("\033[0m\033[37m") + glyph + glyph + glyph + "\033[37;0m/test\n";
        for (qsizetype cut = 0; cut <= data.size(); ++cut) {
            feed(data.left(cut));
            feed(data.mid(cut));
            QCOMPARE(buffer().line(lastTextLine()), qsl("中中中/test"));
            const auto& line = buffer().buffer.at(lastTextLine());
            for (int index = 0; index < 3; ++index) {
                const TChar& left = line.at(index);
                QVERIFY(left.hasSplitFormat());
                // Bold on ANSI white brightens it instead; SGR 0 took the white away after the first glyph:
                QCOMPARE(left.foreground(), index == 0 ? mpHost->mLightWhite : mpHost->mFgColor);
                QCOMPARE(left.rightHalf().foreground(), mpHost->mFgColor);
                QCOMPARE(left.isBold(), index != 0);
                QVERIFY(!left.rightHalf().isBold());
            }
        }
    }

    void resettingTheParserDropsThePendingLead()
    {
        feed("\033[31m\xa4\033[3");
        buffer().resetSequenceParserState();
        feed("\033[0mOK\n");
        QCOMPARE(buffer().line(lastTextLine()), qsl("OK"));
        QVERIFY(!buffer().buffer.at(lastTextLine()).front().hasSplitFormat());
    }

    void oscLinkStylingReachesBothHalves()
    {
        mpHost->mEnableOSC8Hyperlinks = true;
        const QByteArray open = "\033]8;;https://example.com/?config={\"style\":{\"bold\":true,\"italic\":true,\"underline\":\"wavy\",\"overline\":true,\"strikethrough\":true}}\033\\";
        feed(open + "\033[31m\xa4\033[32m\xa4\033]8;;\033\\\n");
        QCOMPARE(buffer().line(lastTextLine()), qsl("中"));
        const TChar character = buffer().buffer.at(lastTextLine()).front();
        QVERIFY(character.linkIndex() > 0);
        const auto decorations = TChar::Bold | TChar::Italic | TChar::Underline | TChar::UnderlineWavy | TChar::Overline | TChar::StrikeOut;
        QCOMPARE(character.allDisplayAttributes() & decorations, decorations);
        QCOMPARE(character.rightHalf().allDisplayAttributes() & decorations, decorations);
        QVERIFY(character.foreground() != character.rightHalf().foreground());
    }

    void linkStateStylingReachesBothHalves()
    {
        mpHost->mEnableOSC8Hyperlinks = true;
        feed("\033]8;;https://example.com/\033\\\033[31;7m\xa4\033[27;32;5m\xa4\033]8;;\033\\\n");
        const int line = lastTextLine();
        const int link = buffer().buffer.at(line).front().linkIndex();
        QVERIFY(link > 0);
        Mudlet::HyperlinkStyling styling;
        styling.hasCustomStyling = true;
        styling.hoverStyle.hasCustomStyling = true;
        styling.hoverStyle.isBold = true;
        styling.hoverStyle.isItalic = true;
        styling.hoverStyle.isOverlined = true;
        styling.hoverStyle.isStrikeOut = true;
        styling.hoverStyle.isUnderlined = true;
        styling.hoverStyle.underlineStyle = Mudlet::HyperlinkStyling::UnderlineDotted;
        mpHost->mainConsoleView()->getLinkStore().setStyling(link, styling);
        buffer().setLinkState(link, Mudlet::HyperlinkStyling::StateHover);
        buffer().updateLinkCharacters(link);
        const TChar styled = buffer().buffer.at(line).front();
        const auto decorations = TChar::Bold | TChar::Italic | TChar::Underline | TChar::UnderlineDotted | TChar::Overline | TChar::StrikeOut;
        QCOMPARE(styled.allDisplayAttributes() & decorations, decorations);
        QCOMPARE(styled.rightHalf().allDisplayAttributes() & decorations, decorations);
        buffer().setLinkState(link, Mudlet::HyperlinkStyling::StateDefault);
        buffer().updateLinkCharacters(link);
        const TChar cleared = buffer().buffer.at(line).front();
        QCOMPARE(cleared.allDisplayAttributes(), TChar::AttributeFlags(TChar::Reverse));
        QCOMPARE(cleared.rightHalf().allDisplayAttributes(), TChar::AttributeFlags(TChar::Blink));
        QCOMPARE(cleared.rightHalf().foreground(), styled.rightHalf().foreground());
    }

    void mxpLinkUnderlineReachesBothHalves()
    {
        mpHost->setForceMXPProcessorOn(true);
        feed("<SEND \"test\">\033[31m\xa4\033[32m\xa4</SEND>\n");
        mpHost->setForceMXPProcessorOn(false);
        QCOMPARE(buffer().line(lastTextLine()), qsl("中"));
        const TChar character = buffer().buffer.at(lastTextLine()).front();
        QVERIFY(character.linkIndex() > 0);
        QVERIFY(character.isUnderlined());
        QVERIFY(character.rightHalf().isUnderlined());
        QVERIFY(character.foreground() != character.rightHalf().foreground());
    }

    // A lead byte the game never completed still belongs to the link it was sent in:
    void brokenCharacterStaysInItsLink()
    {
        mpHost->mEnableOSC8Hyperlinks = true;
        feed("\033]8;;https://example.com/\033\\A\xa4\033[31m\033[2KB\033]8;;\033\\\033[0m\n");
        const int line = lastTextLine();
        QCOMPARE(buffer().line(line), qsl("A\uFFFDB"));
        const auto& characters = buffer().buffer.at(line);
        QVERIFY(characters.at(0).linkIndex() > 0);
        QCOMPARE(characters.at(1).linkIndex(), characters.at(0).linkIndex());
        QCOMPARE(characters.at(2).linkIndex(), characters.at(0).linkIndex());

        mpHost->setForceMXPProcessorOn(true);
        feed("<SEND \"test\">A\xa4\033[31m\033[2KB</SEND>\033[0m\n");
        mpHost->setForceMXPProcessorOn(false);
        const auto& mxpCharacters = buffer().buffer.at(lastTextLine());
        QCOMPARE(buffer().line(lastTextLine()), qsl("A\uFFFDB"));
        QVERIFY(mxpCharacters.at(0).linkIndex() > 0);
        QCOMPARE(mxpCharacters.at(1).linkIndex(), mxpCharacters.at(0).linkIndex());
        QVERIFY(mxpCharacters.at(1).isUnderlined());
    }

    void japaneseAsciiKeepsCodecMappings_data()
    {
        QTest::addColumn<QByteArray>("encoding");
        QTest::newRow("Shift JIS") << QByteArray("SHIFT_JIS");
        QTest::newRow("EUC-JP") << QByteArray("EUC-JP");
    }

    void japaneseAsciiKeepsCodecMappings()
    {
        QFETCH(QByteArray, encoding);
        QVERIFY(mpHost->mTelnet.setEncoding(encoding, false).first);
        for (int value = 0x1a; value < 0x80; ++value) {
            if (value < 0x20 && value != 0x1a && value != 0x1c) {
                continue;
            }
            // Fed alone, so it reaches the decoder rather than a bulk-copied run:
            const QByteArray byte(1, static_cast<char>(value));
            feed("ascii:");
            feed(byte);
            feed(":end\n");
            QCOMPARE(buffer().line(lastTextLine()), qsl("ascii:") + TEncodingHelper::decode(byte, encoding) + qsl(":end"));
        }
    }

    void japaneseEncodingsAreOffered_data() { japaneseAsciiKeepsCodecMappings_data(); }

    void japaneseEncodingsAreOffered()
    {
        QFETCH(QByteArray, encoding);
        QVERIFY(mudlet::self()->getEncodingNamesMap().contains(encoding));
        QVERIFY(mpHost->mTelnet.getEncodingsList().contains(encoding));
        QVERIFY(mpHost->mTelnet.setEncoding(encoding, false).first);
        mpHost->setWideAmbiguousEAsianGlyphs(Qt::PartiallyChecked);
        QVERIFY2(mpHost->wideAmbiguousEAsianGlyphs(), "ambiguous width glyphs are wide in CJK encodings");
        QVERIFY(mpHost->mTelnet.setEncoding("BIG5", false).first);
    }

    void japaneseTextStillDecodes()
    {
        QVERIFY(mpHost->mTelnet.setEncoding("SHIFT_JIS", false).first);
        feed("\x93\xfa\xb1\n");
        QCOMPARE(buffer().line(lastTextLine()), qsl("日ｱ"));
        QVERIFY(mpHost->mTelnet.setEncoding("EUC-JP", false).first);
        feed("\xc6\xfc\x8e\xb1\x8f\xb0\xa1\n");
        QCOMPARE(buffer().line(lastTextLine()), qsl("日ｱ丂"));
    }

    void paintsEachHalfInItsOwnColours()
    {
        auto* pane = mpHost->mainConsoleView()->mUpperPane;
        const TChar& character = splitCharacterOnNewLine();
        TTextEdit::LineLayout layout;
        QCOMPARE(pane->layoutGrapheme(layout, QPoint(0, 0), qsl("中"), 0, -1, character), 2);
        QCOMPARE(layout.size(), size_t(2));

        QImage image(2 * pane->mFontWidth, pane->mFontHeight, QImage::Format_RGB32);
        image.fill(Qt::black);
        QPainter painter(&image);
        painter.setFont(pane->font());
        TGlyphCache glyphCache;
        glyphCache.setFont(painter.font(), *painter.device());
        pane->paintBackgrounds(painter, layout);
        pane->paintForegrounds(painter, glyphCache, layout);
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
