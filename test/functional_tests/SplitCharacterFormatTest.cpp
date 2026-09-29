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
#include "TEncodingHelper.h"
#include "TMainConsole.h"
#include "TTextEdit.h"
#include "TelnetServerStub.h"
#include "mudlet.h"

#include <QFontDatabase>
#include <QFontInfo>
#include <QPainter>
#include <QTemporaryDir>
#include <QtTest>

// Half-character styles and the two paint passes have no Lua accessor. Text
// decoding and trigger-visible output are covered by TBufferEncoding_spec.lua.
class SplitCharacterFormatTest : public QObject
{
    Q_OBJECT
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;

    TBuffer& buffer() const { return mpHost->mpConsole->buffer; }

    void feed(const QByteArray& bytes, bool server = true)
    {
        std::string data(bytes.constData(), bytes.size());
        buffer().translateToPlainText(data, server);
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

    QImage render(const TChar& format)
    {
        auto* pane = mpHost->mpConsole->mUpperPane;
        TTextEdit::LineLayout layout;
        pane->layoutGrapheme(layout, QPoint(1, 1), qsl("中"), 0, -1, format);
        QImage image(4 * pane->mFontWidth, 3 * pane->mFontHeight, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::black);
        QPainter painter(&image);
        painter.setFont(pane->font());
        pane->paintBackgrounds(painter, layout);
        pane->paintForegrounds(painter, layout);
        painter.end();
        return image;
    }

private slots:
    void initTestCase()
    {
        QVERIFY(!portableMarkerPresent());
        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(mConfigDir.path() + qsl("/mudlet/profiles")));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());
#ifdef INCLUDE_FONTS
        QVERIFY(QFontDatabase::addApplicationFont(qsl(":/fonts/ttf-bitstream-vera-1.10/VeraMono.ttf")) >= 0);
#endif
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
#ifdef INCLUDE_FONTS
        QFont font(qsl("Bitstream Vera Sans Mono"), 18);
        QCOMPARE(QFontInfo(font).family(), font.family());
        mpHost->mpConsole->mUpperPane->setFont(font);
#endif
    }

    void cleanupTestCase()
    {
        delete mudlet::self();
        delete mpServer;
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void init()
    {
        buffer().resetSequenceParserState();
        feed("\033[0m\n");
    }

    void splitFormatSurvivesEveryPacketBoundary_data()
    {
        QTest::addColumn<QByteArray>("encoding");
        QTest::addColumn<QByteArray>("bytes");
        QTest::addColumn<QString>("text");
        QTest::newRow("traditional") << QByteArray("BIG5") << QByteArray::fromHex("a4a4") << qsl("中");
        QTest::newRow("hong-kong") << QByteArray("BIG5-HKSCS") << QByteArray::fromHex("a741") << qsl("你");
        QTest::newRow("simplified") << QByteArray("GBK") << QByteArray::fromHex("d6d0") << qsl("中");
        QTest::newRow("extended-chinese") << QByteArray("GB18030") << QByteArray::fromHex("d6d0") << qsl("中");
        QTest::newRow("korean") << QByteArray("EUC-KR") << QByteArray::fromHex("c7d1") << qsl("한");
        QTest::newRow("japanese-shift") << QByteArray("SHIFT_JIS") << QByteArray::fromHex("93fa") << qsl("日");
        QTest::newRow("japanese-euc") << QByteArray("EUC-JP") << QByteArray::fromHex("c6fc") << qsl("日");
    }

    void splitFormatSurvivesEveryPacketBoundary()
    {
        QFETCH(QByteArray, encoding);
        QFETCH(QByteArray, bytes);
        QFETCH(QString, text);
        QVERIFY(mpHost->mTelnet.setEncoding(encoding, false).first);
        const QByteArray data = QByteArray("\033[0;38;2;255;0;0;48;2;0;0;64m") + bytes.left(1) + "\033[38:2::0:255:0m\033[48;2;64;0;0m" + bytes.mid(1) + "X\n";
        for (qsizetype cut = 0; cut <= data.size(); ++cut) {
            feed(data.left(cut));
            feed(data.mid(cut));
            const int line = lastTextLine();
            QVERIFY(line >= 0);
            QCOMPARE(buffer().line(line), text + qsl("X"));
            const auto& chars = buffer().buffer.at(line);
            QCOMPARE(chars.size(), size_t(text.size() + 1));
            const TChar& format = chars.front();
            QVERIFY(format.hasSplitFormat());
            QCOMPARE(format.foreground(), QColor(255, 0, 0));
            QCOMPARE(format.background(), QColor(0, 0, 64));
            QCOMPARE(format.rightHalfFormat().foreground(), QColor(0, 255, 0));
            QCOMPARE(format.rightHalfFormat().background(), QColor(64, 0, 0));
            QVERIFY(!chars.back().hasSplitFormat());
            QCOMPARE(chars.back().foreground(), QColor(0, 255, 0));
        }
    }

    void japaneseAsciiKeepsCodecMappings_data()
    {
        QTest::addColumn<QByteArray>("encoding");
        QTest::newRow("shift-jis") << QByteArray("SHIFT_JIS");
        QTest::newRow("euc-jp") << QByteArray("EUC-JP");
    }

    void japaneseAsciiKeepsCodecMappings()
    {
        QFETCH(QByteArray, encoding);
        QVERIFY(mpHost->mTelnet.setEncoding(encoding, false).first);
        for (int value = 0x1a; value < 0x80; ++value) {
            if (value < 0x20 && value != 0x1a && value != 0x1c) {
                continue;
            }
            const QByteArray byte(1, static_cast<char>(value));
            feed("ascii:");
            feed(byte);
            feed(":end\n");
            const QString actual = buffer().line(lastTextLine());
            const QString expected = qsl("ascii:") + TEncodingHelper::decode(byte, encoding) + qsl(":end");
            QCOMPARE(actual, expected);
        }
    }

    void localFeedDoesNotConsumePendingCharacter()
    {
        QVERIFY(mpHost->mTelnet.setEncoding("BIG5", false).first);
        feed(QByteArray("\033[31m") + QByteArray::fromHex("a4") + "\r");
        feed("local\n", false);
        feed(QByteArray("\033[32m") + QByteArray::fromHex("a4") + "\n");
        const int line = lastTextLine();
        QCOMPARE(buffer().line(line), qsl("中"));
        QVERIFY(buffer().buffer.at(line).front().hasSplitFormat());
    }

    void compactStoragePreservesCopiesAndMoves()
    {
        QCOMPARE(sizeof(TChar), size_t(16));
        TChar original(Qt::red, Qt::black, TChar::Bold, 23);
        original.setRightHalfFormat(TChar(Qt::green, Qt::blue, TChar::Italic));
        original.select();
        TChar assigned(Qt::white, Qt::black);
        assigned = original;
        QVERIFY(assigned.isSelected());
        assigned.setBackground(Qt::yellow);
        QCOMPARE(original.background(), QColor(Qt::black));
        QCOMPARE(original.rightHalfFormat().background(), QColor(Qt::blue));
        QCOMPARE(assigned.rightHalfFormat().background(), QColor(Qt::yellow));
        TChar moved(std::move(assigned));
        QVERIFY(moved.isSelected());
        QCOMPARE(moved.linkIndex(), 23);
        QVERIFY(!assigned.hasSplitFormat());
        assigned = original;
        moved = std::move(assigned);
        QVERIFY(moved == original);
        QVERIFY(!assigned.hasSplitFormat());
        moved.setRightHalfFormat(moved);
        QVERIFY(!moved.hasSplitFormat());
        QCOMPARE(moved.foreground(), QColor(Qt::red));
        QVERIFY(original.hasSplitFormat());
        moved = TChar(Qt::white, Qt::black);
        QVERIFY(!moved.hasSplitFormat());
        original = moved;
        QVERIFY(!original.hasSplitFormat());
        QCOMPARE(original.foreground(), QColor(Qt::white));
    }

    void copiesAndRecolouringKeepIndependentHalves()
    {
        TChar original(Qt::red, Qt::black);
        original.setRightHalfFormat(TChar(Qt::green, Qt::blue, TChar::Reverse));
        original.select();
        TChar copy(original);
        QVERIFY(!copy.isSelected());
        QVERIFY(original.rightHalfFormat().isSelected());
        QVERIFY(copy.rightHalfFormat().isReversed());
        copy.setForeground(Qt::yellow);
        QCOMPARE(copy.foreground(), QColor(Qt::yellow));
        QCOMPARE(copy.rightHalfFormat().foreground(), QColor(Qt::yellow));
        QCOMPARE(original.rightHalfFormat().foreground(), QColor(Qt::green));
        copy.setBackground(Qt::cyan);
        QCOMPARE(copy.rightHalfFormat().background(), QColor(Qt::cyan));
        copy.setAllDisplayAttributes(TChar::Bold);
        QVERIFY(copy.rightHalfFormat().isBold());
        QVERIFY(!copy.rightHalfFormat().isReversed());
    }

    void copyingBufferAndRecolouringPreserveTheOtherCopy()
    {
        QVERIFY(mpHost->mTelnet.setEncoding("BIG5", false).first);
        feed(QByteArray("\033[31m") + QByteArray::fromHex("a4") + "\033[32m" + QByteArray::fromHex("a4") + "X\n");
        const int line = lastTextLine();
        QCOMPARE(buffer().line(line), qsl("中X"));
        QPoint start(0, line);
        QPoint end(1, line);
        TBuffer copied = buffer().copy(start, end);
        QCOMPARE(copied.line(0), qsl("中"));
        QVERIFY(copied.buffer.front().front().hasSplitFormat());
        const QColor originalRight = copied.buffer.front().front().rightHalfFormat().foreground();
        QVERIFY(buffer().applyFgColor(start, end, QColor(Qt::cyan)));
        QCOMPARE(buffer().buffer.at(line).front().rightHalfFormat().foreground(), QColor(Qt::cyan));
        QCOMPARE(copied.buffer.front().front().rightHalfFormat().foreground(), originalRight);
        QVERIFY(buffer().applyAttribute(start, end, TChar::Underline, true));
        QVERIFY(buffer().buffer.at(line).front().rightHalfFormat().isUnderlined());
        QVERIFY(!copied.buffer.front().front().rightHalfFormat().isUnderlined());
    }

    void clearingSearchRemovesBothHalves()
    {
        QVERIFY(mpHost->mTelnet.setEncoding("BIG5", false).first);
        feed(QByteArray("\033[31m") + QByteArray::fromHex("a4") + "\033[32m" + QByteArray::fromHex("a4") + "\n");
        const int line = lastTextLine();
        const auto& character = buffer().buffer.at(line).front();
        QVERIFY(!character.isFound());
        QVERIFY(!character.rightHalfFormat().isFound());
        QVERIFY(buffer().applyAttribute(QPoint(0, line), QPoint(1, line), TChar::Found, true));
        QVERIFY(character.isFound());
        QVERIFY(character.rightHalfFormat().isFound());
        buffer().clearSearchHighlights();
        QVERIFY(!character.isFound());
        QVERIFY(!character.rightHalfFormat().isFound());
    }

    void pastePreservesSplitFormat()
    {
        QVERIFY(mpHost->mTelnet.setEncoding("BIG5", false).first);
        feed(QByteArray("\033[31m") + QByteArray::fromHex("a4") + "\033[32m" + QByteArray::fromHex("a4") + "X\n");
        const int line = lastTextLine();
        QPoint start(0, line);
        QPoint end(1, line);
        TBuffer copied = buffer().copy(start, end);
        const TChar expected = copied.buffer.front().front();
        QVERIFY(expected.hasSplitFormat());
        QPoint destination(1, line);
        buffer().paste(destination, copied);
        QCOMPARE(buffer().line(line), qsl("中中X"));
        QVERIFY(buffer().buffer.at(line).at(1) == expected);
        buffer().appendBuffer(copied);
        QVERIFY(buffer().buffer.at(lastTextLine()).front() == expected);
    }

    void japaneseEncodingNames_data()
    {
        QTest::addColumn<QByteArray>("encoding");
        QTest::newRow("shift-jis") << QByteArray("SHIFT_JIS");
        QTest::newRow("euc-jp") << QByteArray("EUC-JP");
    }

    void japaneseEncodingNames()
    {
        QFETCH(QByteArray, encoding);
        QVERIFY(mudlet::self()->getEncodingNamesMap().contains(encoding));
    }

    void japaneseAutomaticWidth_data() { japaneseEncodingNames_data(); }

    void japaneseAutomaticWidth()
    {
        QFETCH(QByteArray, encoding);
        QVERIFY(mpHost->mTelnet.setEncoding(encoding, false).first);
        mpHost->setWideAmbiguousEAsianGlyphs(Qt::Unchecked);
        QVERIFY(!mpHost->wideAmbiguousEAsianGlyphs());
        mpHost->setWideAmbiguousEAsianGlyphs(Qt::PartiallyChecked);
        QVERIFY(mpHost->wideAmbiguousEAsianGlyphs());
        QVERIFY(mpHost->mTelnet.setEncoding("UTF-8", false).first);
        mpHost->setWideAmbiguousEAsianGlyphs(Qt::PartiallyChecked);
        QVERIFY(!mpHost->wideAmbiguousEAsianGlyphs());
    }

    void resettingTheParserDropsThePendingCharacter()
    {
        QVERIFY(mpHost->mTelnet.setEncoding("BIG5", false).first);
        feed(QByteArray::fromHex("a4") + "\033[31");
        buffer().resetSequenceParserState();
        feed("OK\n");
        QCOMPARE(buffer().line(lastTextLine()), qsl("OK"));
        QVERIFY(!buffer().buffer.at(lastTextLine()).front().hasSplitFormat());
    }

    void oscLinkDecorationsReachBothHalves()
    {
        QVERIFY(mpHost->mTelnet.setEncoding("BIG5", false).first);
        mpHost->mEnableOSC8Hyperlinks = true;
        const QByteArray open = "\033]8;;https://example.com/?config={\"style\":{\"bold\":true,\"italic\":true,\"underline\":\"wavy\",\"overline\":true,\"strikethrough\":true}}\033\\";
        feed(open + "\033[31m" + QByteArray::fromHex("a4") + "\033[32m" + QByteArray::fromHex("a4") + "\033]8;;\033\\\n");
        QCOMPARE(buffer().line(lastTextLine()), qsl("中"));
        const TChar character = buffer().buffer.at(lastTextLine()).front();
        QVERIFY(character.linkIndex() > 0);
        const auto decorations = TChar::Bold | TChar::Italic | TChar::Underline | TChar::UnderlineWavy | TChar::Overline | TChar::StrikeOut;
        QCOMPARE(character.allDisplayAttributes() & decorations, decorations);
        QCOMPARE(character.rightHalfFormat().allDisplayAttributes() & decorations, decorations);
        QVERIFY(character.foreground() != character.rightHalfFormat().foreground());
    }

    void linkStateDecorationsReachBothHalves()
    {
        QVERIFY(mpHost->mTelnet.setEncoding("BIG5", false).first);
        mpHost->mEnableOSC8Hyperlinks = true;
        feed(QByteArray("\033]8;;https://example.com/\033\\\033[31;7m") + QByteArray::fromHex("a4") + "\033[27;32;5m" + QByteArray::fromHex("a4") + "\033]8;;\033\\\n");
        const int line = lastTextLine();
        const int link = buffer().buffer.at(line).front().linkIndex();
        QVERIFY(link > 0);
        QVERIFY(!buffer().buffer.at(line).front().rightHalfFormat().isBold());
        Mudlet::HyperlinkStyling styling;
        styling.hasCustomStyling = true;
        styling.hoverStyle.hasCustomStyling = true;
        styling.hoverStyle.isBold = true;
        styling.hoverStyle.isItalic = true;
        styling.hoverStyle.isOverlined = true;
        styling.hoverStyle.isStrikeOut = true;
        styling.hoverStyle.isUnderlined = true;
        styling.hoverStyle.underlineStyle = Mudlet::HyperlinkStyling::UnderlineDotted;
        mpHost->mpConsole->getLinkStore().setStyling(link, styling);
        buffer().setLinkState(link, Mudlet::HyperlinkStyling::StateHover);
        buffer().updateLinkCharacters(link);
        const TChar styled = buffer().buffer.at(line).front();
        QVERIFY(styled.isBold());
        QVERIFY(styled.rightHalfFormat().isBold());
        const auto decorations = TChar::Bold | TChar::Italic | TChar::Underline | TChar::UnderlineDotted | TChar::Overline | TChar::StrikeOut;
        QCOMPARE(styled.allDisplayAttributes() & decorations, decorations);
        QCOMPARE(styled.rightHalfFormat().allDisplayAttributes() & decorations, decorations);
        buffer().setLinkState(link, Mudlet::HyperlinkStyling::StateDefault);
        buffer().updateLinkCharacters(link);
        const TChar cleared = buffer().buffer.at(line).front();
        QVERIFY(!cleared.isBold());
        QVERIFY(!cleared.rightHalfFormat().isBold());
        QVERIFY(!cleared.rightHalfFormat().isUnderlined());
        QCOMPARE(cleared.allDisplayAttributes(), TChar::AttributeFlags(TChar::Reverse));
        QCOMPARE(cleared.rightHalfFormat().allDisplayAttributes(), TChar::AttributeFlags(TChar::Blink));
        QVERIFY(styled.rightHalfFormat().isBold());
        QCOMPARE(cleared.foreground(), styled.foreground());
        QCOMPARE(cleared.rightHalfFormat().foreground(), styled.rightHalfFormat().foreground());
    }

    void mxpLinkUnderlineReachesBothHalves()
    {
        QVERIFY(mpHost->mTelnet.setEncoding("BIG5", false).first);
        mpHost->setForceMXPProcessorOn(true);
        feed(QByteArray("<SEND \"test\">\033[31m") + QByteArray::fromHex("a4") + "\033[32m" + QByteArray::fromHex("a4") + "</SEND>\n");
        mpHost->setForceMXPProcessorOn(false);
        QCOMPARE(buffer().line(lastTextLine()), qsl("中"));
        const TChar character = buffer().buffer.at(lastTextLine()).front();
        QVERIFY(character.linkIndex() > 0);
        QVERIFY(character.isUnderlined());
        QVERIFY(character.rightHalfFormat().isUnderlined());
        QVERIFY(character.foreground() != character.rightHalfFormat().foreground());
    }

    void sgrResetPreservesHalfAttributes()
    {
        QVERIFY(mpHost->mTelnet.setEncoding("BIG5", false).first);
        // The first glyph starts in ANSI white; SGR 0 returns subsequent glyphs to the default color.
        const QByteArray byte = QByteArray::fromHex("a4");
        const QByteArray glyph = QByteArray("\033[1m") + byte + "\033[0m" + byte;
        const QByteArray data = QByteArray("\033[0m\033[37m") + glyph + glyph + glyph + "\033[37;0m/test\n";
        for (qsizetype cut = 0; cut <= data.size(); ++cut) {
            feed(data.left(cut));
            feed(data.mid(cut));
            QCOMPARE(buffer().line(lastTextLine()), qsl("中中中/test"));
            const auto& line = buffer().buffer.at(lastTextLine());
            for (int index = 0; index < 3; ++index) {
                const TChar& left = line.at(index);
                QVERIFY(left.hasSplitFormat());
                const TChar right = left.rightHalfFormat();
                QCOMPARE(left.foreground(), index == 0 ? mpHost->mLightWhite : mpHost->mFgColor);
                QCOMPARE(right.foreground(), mpHost->mFgColor);
                QCOMPARE(left.isBold(), index != 0);
                QVERIFY(!right.isBold());
            }
        }
    }

    void sgrAttributesMatchUnsplitCharacters()
    {
        const QList<QPair<QByteArray, QByteArray>> characters = {{"BIG5", QByteArray::fromHex("a4a4")},
                                                                 {"BIG5-HKSCS", QByteArray::fromHex("a741")},
                                                                 {"GBK", QByteArray::fromHex("d6d0")},
                                                                 {"GB18030", QByteArray::fromHex("d6d0")},
                                                                 {"EUC-KR", QByteArray::fromHex("c7d1")},
                                                                 {"SHIFT_JIS", QByteArray::fromHex("93fa")},
                                                                 {"EUC-JP", QByteArray::fromHex("c6fc")}};
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
        for (const auto& character : characters) {
            QVERIFY(mpHost->mTelnet.setEncoding(character.first, false).first);
            for (const auto& rendition : renditions) {
                const QByteArray before = "\033[0m\033[" + rendition.first + "m";
                const QByteArray between = "\033[" + rendition.second + "m";
                feed(before + character.second + between + character.second + "\033[0mX\n");
                const auto reference = buffer().buffer.at(lastTextLine());
                QVERIFY(reference.at(0).foreground() != reference.at(1).foreground() || reference.at(0).allDisplayAttributes() != reference.at(1).allDisplayAttributes());
                const QByteArray data = before + character.second.left(1) + between + character.second.mid(1) + "Y\033[0mX\n";
                for (qsizetype cut = 0; cut <= data.size(); ++cut) {
                    feed(data.left(cut));
                    feed(data.mid(cut));
                    const auto& actual = buffer().buffer.at(lastTextLine());
                    QCOMPARE(actual.size(), 3);
                    QVERIFY(actual.at(0).hasSplitFormat());
                    const TChar right = actual.at(0).rightHalfFormat();
                    QCOMPARE(actual.at(0).foreground(), reference.at(0).foreground());
                    QCOMPARE(actual.at(0).background(), reference.at(0).background());
                    QCOMPARE(actual.at(0).allDisplayAttributes(), reference.at(0).allDisplayAttributes());
                    QCOMPARE(right.foreground(), reference.at(1).foreground());
                    QCOMPARE(right.background(), reference.at(1).background());
                    QCOMPARE(right.allDisplayAttributes(), reference.at(1).allDisplayAttributes());
                    QCOMPARE(actual.at(1).allDisplayAttributes(), reference.at(1).allDisplayAttributes());
                    QCOMPARE(actual.at(1).foreground(), reference.at(1).foreground());
                    QCOMPARE(actual.at(2).allDisplayAttributes(), reference.at(2).allDisplayAttributes());
                    QCOMPARE(actual.at(2).foreground(), reference.at(2).foreground());
                }
            }
        }
    }

    void blinkingOnlyChangesTheBlinkingHalf()
    {
#ifndef INCLUDE_FONTS
        QSKIP("Rendering checks require bundled fonts (WITH_FONTS=YES)");
#endif
        auto* pane = mpHost->mpConsole->mUpperPane;
        pane->mEnableBlinkText = true;
        for (const auto flag : {TChar::Blink, TChar::FastBlink}) {
            TChar split(Qt::red, Qt::black, TChar::None);
            split.setRightHalfFormat(TChar(Qt::green, Qt::black, flag));
            pane->mHasBlinkingContent = false;
            const QImage before = render(split);
            QVERIFY(pane->mHasBlinkingContent);
            mudlet::self()->registerBlinkClient();
            QImage after;
            const bool changed = QTest::qWaitFor(
                    [&]() {
                        after = render(split);
                        return after != before;
                    },
                    3000);
            mudlet::self()->unregisterBlinkClient();
            QVERIFY(changed);
            const int width = pane->mFontWidth;
            QCOMPARE(before.copy(width, 0, width, before.height()), after.copy(width, 0, width, after.height()));
            QVERIFY(before.copy(2 * width, 0, width, before.height()) != after.copy(2 * width, 0, width, after.height()));
        }
    }

    void paintingMatchesEachHalfOfTheUnsplitGlyph_data()
    {
        QTest::addColumn<bool>("selected");
        QTest::addColumn<TChar::AttributeFlags>("attributes");
        QTest::addColumn<bool>("blinkEnabled");
        const QList<TChar::AttributeFlag> flags = {TChar::Bold,
                                                   TChar::Italic,
                                                   TChar::Underline,
                                                   TChar::Overline,
                                                   TChar::StrikeOut,
                                                   TChar::Reverse,
                                                   TChar::Blink,
                                                   TChar::FastBlink,
                                                   TChar::UnderlineWavy,
                                                   TChar::UnderlineDotted,
                                                   TChar::UnderlineDashed};
        for (const auto flag : flags) {
            for (const bool selected : {false, true}) {
                for (const bool blinkEnabled : {false, true}) {
                    const QByteArray name = QByteArray::number(flag) + (selected ? "-selected" : "-normal") + (blinkEnabled ? "-blink" : "-static");
                    QTest::newRow(name.constData()) << selected << TChar::AttributeFlags(flag) << blinkEnabled;
                }
            }
        }
    }

    void paintingMatchesEachHalfOfTheUnsplitGlyph()
    {
#ifndef INCLUDE_FONTS
        QSKIP("Rendering checks require bundled fonts (WITH_FONTS=YES)");
#endif
        QFETCH(bool, selected);
        QFETCH(TChar::AttributeFlags, attributes);
        QFETCH(bool, blinkEnabled);
        mpHost->mpConsole->mUpperPane->mEnableBlinkText = blinkEnabled;
        TChar left(Qt::red, QColor(0, 0, 64), TChar::None);
        TChar right(Qt::green, QColor(64, 0, 0), attributes);
        TChar split(left);
        split.setRightHalfFormat(right);
        if (selected) {
            left.select();
            right.select();
            split.select();
        }
        const QImage leftImage = render(left);
        const QImage rightImage = render(right);
        const QImage splitImage = render(split);
        QVERIFY(leftImage != rightImage);
        const int midpoint = 2 * mpHost->mpConsole->mUpperPane->mFontWidth;
        for (int y = 0; y < splitImage.height(); ++y) {
            for (int x = mpHost->mpConsole->mUpperPane->mFontWidth; x < 3 * mpHost->mpConsole->mUpperPane->mFontWidth; ++x) {
                const QColor actual = splitImage.pixelColor(x, y);
                const QColor expected = (x < midpoint ? leftImage : rightImage).pixelColor(x, y);
                // Clipping Qt's antialiased decorations can round a channel by one.
                QVERIFY(qAbs(actual.red() - expected.red()) <= 1);
                QVERIFY(qAbs(actual.green() - expected.green()) <= 1);
                QVERIFY(qAbs(actual.blue() - expected.blue()) <= 1);
                QCOMPARE(actual.alpha(), expected.alpha());
            }
        }
    }
};

#include "SplitCharacterFormatTest.moc"
MUDLET_GROUPED_TEST_MAIN(SplitCharacterFormatTest)
