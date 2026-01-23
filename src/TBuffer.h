#ifndef MUDLET_TBUFFER_H
#define MUDLET_TBUFFER_H

/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2015, 2017-2018, 2020, 2022-2023 by Stephen Lyons       *
 *                                               - slysven@virginmedia.com *
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


#include "TTextCodec.h"

#include <QApplication>
#include <QChar>
#include <QColor>
#include <QDebug>
#include <QMap>
#include <QQueue>
#include <QPoint>
#include <QPointer>
#include <QString>
#include <QStringBuilder>
#include <QStringList>
#include <QTime>
#include <QVector>
#include "TEncodingTable.h"
#include "TLinkStore.h"
#include "TMxpMudlet.h"
#include "TMxpProcessor.h"

#include <deque>
#include <memory>
#include <string>

class Host;
class QTextCodec;
class TConsole;

// Enhanced OSC 8 hyperlink styling support with CSS link states
// Defined in Mudlet namespace to avoid circular dependencies
namespace Mudlet {

struct HyperlinkStyling {
    // Base styling properties
    QColor foregroundColor;
    QColor backgroundColor;
    bool hasForegroundColor = false;
    bool hasBackgroundColor = false;
    bool isBold = false;
    bool isItalic = false;
    bool isUnderlined = false; // OSC 8 hyperlinks default to no underline (unlike other Mudlet hyperlinks)
    bool isStrikeOut = false;
    bool isOverlined = false;
    bool hasCustomStyling = false; // Tracks if any custom styling was provided
    bool hasBaseCustomStyling = false; // Tracks if base (non-pseudo-class) styling was provided

    // Extended text decoration support
    enum UnderlineStyle {
        UnderlineNone,
        UnderlineSolid,     // Standard underline
        UnderlineWavy,      // Squiggly/wavy underline
        UnderlineDotted,    // Dotted underline
        UnderlineDashed     // Dashed underline
    };
    UnderlineStyle underlineStyle = UnderlineSolid;
    QColor underlineColor;
    QColor overlineColor;
    QColor strikeoutColor;
    bool hasUnderlineColor = false;
    bool hasOverlineColor = false;
    bool hasStrikeoutColor = false;

    // CSS Link State Support with Accessibility
    enum LinkState {
        StateDefault,       // Default/unvisited (:link)
        StateVisited,       // Visited link (:visited)
        StateHover,         // Mouse hover (:hover)
        StateActive,        // Mouse down/active (:active)
        StateFocus,         // Keyboard focus (:focus)
        StateFocusVisible   // Visible keyboard focus (:focus-visible)
    };

    // State-specific styling containers
    struct StateStyle {
        QColor foregroundColor;
        QColor backgroundColor;
        QColor underlineColor;
        QColor overlineColor;
        QColor strikeoutColor;
        bool hasForegroundColor = false;
        bool hasBackgroundColor = false;
        bool hasUnderlineColor = false;
        bool hasOverlineColor = false;
        bool hasStrikeoutColor = false;
        bool isBold = false;
        bool isItalic = false;
        bool isUnderlined = false;
        bool isStrikeOut = false;
        bool isOverlined = false;
        UnderlineStyle underlineStyle = UnderlineSolid;
        bool hasCustomStyling = false;
    };

    // State-specific styles
    StateStyle linkStyle;           // :link (unvisited)
    StateStyle visitedStyle;        // :visited
    StateStyle hoverStyle;          // :hover
    StateStyle activeStyle;         // :active
    StateStyle focusStyle;          // :focus
    StateStyle focusVisibleStyle;   // :focus-visible
    StateStyle anyLinkStyle;        // :any-link (applies to both :link and :visited)

    // State tracking
    LinkState currentState = StateDefault;

    // Methods to get effective styling for current state
    StateStyle getEffectiveStyle() const;
};

} // namespace Mudlet

class WrapInfo
{
    friend class TBuffer;

public:
    const bool isNewline;
    const bool needsIndent;
    const int firstChar;
    const int lastChar;
    WrapInfo(bool isNewline, bool needsIndent, int firstChar, int lastChar) : isNewline(isNewline), needsIndent(needsIndent), firstChar(firstChar), lastChar(lastChar) {}
};

class TChar
{
    friend class TBuffer;

public:
    enum AttributeFlag {
        None = 0x0,
        // Replaces TCHAR_BOLD 2
        Bold = 0x1,                   // 0000 0000 0000 0000 0000 0000 0000 0001
        // Replaces TCHAR_ITALICS 1
        Italic = 0x2,                 // 0000 0000 0000 0000 0000 0000 0000 0010
        // Replaces TCHAR_UNDERLINE 4
        Underline = 0x4,              // 0000 0000 0000 0000 0000 0000 0000 0100
        // ANSI CSI SGR Overline (53 on, 55 off)
        Overline = 0x8,               // 0000 0000 0000 0000 0000 0000 0000 1000
        // Replaces TCHAR_STRIKEOUT 32
        StrikeOut = 0x10,             // 0000 0000 0000 0000 0000 0000 0001 0000
        // Extended underline styles for enhanced OSC 8 hyperlink support
        UnderlineWavy = 0x400000,     // 0000 0000 0100 0000 0000 0000 0000 0000
        UnderlineDotted = 0x800000,   // 0000 0000 1000 0000 0000 0000 0000 0000
        UnderlineDashed = 0x1000000,  // 0000 0001 0000 0000 0000 0000 0000 0000
        // NOT a replacement for TCHAR_INVERSE, that is now covered by the
        // separate isSelected bool but they must be EX-ORed at the point of
        // painting the Character
        Reverse = 0x20,               // 0000 0000 0000 0000 0000 0000 0010 0000
        // Flashing less than 150 times a minute:
        Blink = 0x40,                 // 0000 0000 0000 0000 0000 0000 0100 0000
        // Flashing at least 150 times a minute:
        FastBlink = 0x80,             // 0000 0000 0000 0000 0000 0000 1000 0000
        // Alternate fonts 1 to 9 from SGR 11 m to SGR 19 m; we flag each one
        // separately so that trigger processing can select them individually
        // which could not be done should they be rolled up into just 4 bits.
        // As one can only be active at a time only the highest one should be
        // used if/when we can actually paint different fonts in a TConsole at
        // the same time; currently there is no MUD standard to specify what the
        // alternatives are:
        AltFont1 = 0x00100,           // 0000 0000 0000 0000 0000 0001 0000 0000
        AltFont2 = 0x00200,           // 0000 0000 0000 0000 0000 0010 0000 0000
        AltFont3 = 0x00400,           // 0000 0000 0000 0000 0000 0100 0000 0000
        AltFont4 = 0x00800,           // 0000 0000 0000 0000 0000 1000 0000 0000
        AltFont5 = 0x01000,           // 0000 0000 0000 0000 0001 0000 0000 0000
        AltFont6 = 0x02000,           // 0000 0000 0000 0000 0010 0000 0000 0000
        AltFont7 = 0x04000,           // 0000 0000 0000 0000 0100 0000 0000 0000
        AltFont8 = 0x08000,           // 0000 0000 0000 0000 1000 0000 0000 0000
        AltFont9 = 0x10000,           // 0000 0000 0000 0001 0000 0000 0000 0000
        // From SGR 8 m; however there is no MUD standard protocol to control
        // when we should show concealed text.
        Concealed = 0x20000,          // 0000 0000 0000 0010 0000 0000 0000 0000
        // Mask for "is flashing" at any rate - will return a logical true
        // should either of the above be set - should both be set then FastBlink
        // should take preference over Blink:
        BlinkMask = 0xC0,             // 0000 0000 0000 0000 0000 0000 1100 0000
        // Mask for "any alternate font" - only the most significant one should
        // be used if more than one is set:
        AltFontMask = 0x1ff00,        // 0000 0000 0000 0001 1111 1111 0000 0000
        TestMask = 0x1f3ffff,         // 0000 0001 1111 0011 1111 1111 1111 1111 (includes extended underline styles)
        // The remainder are internal use ones that do not related to SGR codes
        // that have been parsed from the incoming text.
        // Has been found in a search operation (currently Main Console only)
        // and has been given a highlight to indicate that:
        Found = 0x100000,             // 0000 0000 0001 0000 0000 0000 0000 0000
        // Replaces TCHAR_ECHO 16
        Echo = 0x200000               // 0000 0000 0010 0000 0000 0000 0000 0000
    };
    Q_DECLARE_FLAGS(AttributeFlags, AttributeFlag)

    // Not a default constructor - the defaulted argument means it could have
    // been used if supplied with no arguments, but the 'explicit' prevents
    // this:
    explicit TChar(TConsole* pC = nullptr);
    // Another non-default constructor:
    TChar(const QColor& foreground, const QColor& background, const TChar::AttributeFlags flags = TChar::None, const int linkIndex = 0);
    // User defined copy-constructor:
    TChar(const TChar&);
    // Under the rule of three, because we have a user defined copy-constructor,
    // we should also have a destructor and an assignment operator but they can,
    // in this case, be default ones:
    TChar& operator=(const TChar&) = default;
    ~TChar() = default;

    bool operator==(const TChar&);
    void setColors(const QColor& newForeGroundColor, const QColor& newBackGroundColor) {
        mFgColor = newForeGroundColor;
        mBgColor = newBackGroundColor;
    }
    // Only considers the following flags: AltFont#, Bold, Conceal,
    // FastBlink/Blink, Italic, Overline, Reverse, Strikeout, Underline,
    // - does not consider Echo or Found:
    void setAllDisplayAttributes(const AttributeFlags newDisplayAttributes) { mFlags = (mFlags & ~TestMask) | (newDisplayAttributes & TestMask); }
    void setForeground(const QColor& newColor) { mFgColor = newColor; }
    void setBackground(const QColor& newColor) { mBgColor = newColor; }
    void setTextFormat(const QColor& newFgColor, const QColor& newBgColor, const AttributeFlags newDisplayAttributes) {
        setColors(newFgColor, newBgColor);
        setAllDisplayAttributes(newDisplayAttributes);
    }

    const QColor& foreground() const { return mFgColor; }
    const QColor& background() const { return mBgColor; }
    AttributeFlags allDisplayAttributes() const { return mFlags & TestMask; }
    void select() { mIsSelected = true; }
    void deselect() { mIsSelected = false; }
    bool isSelected() const { return mIsSelected; }
    int linkIndex () const { return mLinkIndex; }
    bool isBold() const { return mFlags & Bold; }
    bool isItalic() const { return mFlags & Italic; }
    bool isUnderlined() const { return mFlags & Underline; }
    bool isOverlined() const { return mFlags & Overline; }
    bool isStruckOut() const { return mFlags & StrikeOut; }
    bool isReversed() const { return mFlags & Reverse; }
    bool isConcealed() const { return mFlags & Concealed; }
    bool isFound() const { return mFlags & Found; }
    // Extended underline style accessors for enhanced OSC 8 hyperlink support
    bool isUnderlineWavy() const { return mFlags & UnderlineWavy; }
    bool isUnderlineDotted() const { return mFlags & UnderlineDotted; }
    bool isUnderlineDashed() const { return mFlags & UnderlineDashed; }

    // Decoration color accessors
    const QColor& underlineColor() const { return mUnderlineColor; }
    const QColor& overlineColor() const { return mOverlineColor; }
    const QColor& strikeoutColor() const { return mStrikeoutColor; }
    bool hasCustomUnderlineColor() const { return mHasCustomUnderlineColor; }
    bool hasCustomOverlineColor() const { return mHasCustomOverlineColor; }
    bool hasCustomStrikeoutColor() const { return mHasCustomStrikeoutColor; }

    // Decoration color setters
    void setUnderlineColor(const QColor& color) { mUnderlineColor = color; mHasCustomUnderlineColor = true; }
    void setOverlineColor(const QColor& color) { mOverlineColor = color; mHasCustomOverlineColor = true; }
    void setStrikeoutColor(const QColor& color) { mStrikeoutColor = color; mHasCustomStrikeoutColor = true; }
    void clearCustomUnderlineColor() { mHasCustomUnderlineColor = false; }
    void clearCustomOverlineColor() { mHasCustomOverlineColor = false; }
    void clearCustomStrikeoutColor() { mHasCustomStrikeoutColor = false; }
    // Special case - if fast blink is set then do NOT say that blink is set to
    // preserve priority of the former over the latter:
    bool isBlinking() const { return (mFlags & FastBlink) ? false : (mFlags & Blink); }
    bool isFastBlinking() const { return mFlags & FastBlink; }
    quint8 alternateFont() const;
    static TChar::AttributeFlag alternateFontFlag(const quint8 altFontNumber) {
        switch (altFontNumber) {
        case 1: return AltFont1;
        case 2: return AltFont2;
        case 3: return AltFont3;
        case 4: return AltFont4;
        case 5: return AltFont5;
        case 6: return AltFont6;
        case 7: return AltFont7;
        case 8: return AltFont8;
        case 9: return AltFont9;
        default:
            Q_ASSERT_X(altFontNumber < 10, "alternateFontFlag", "value out of range 0 to 9");
            return None;
        }
    }
    static QString attributeType(const AttributeFlag flag) {
        switch (flag) {
        case None:
            return qsl("None");
        case Bold:
            return qsl("Bold");
        case Italic:
            return qsl("Italic");
        case Underline:
            return qsl("Underline");
        case Overline:
            return qsl("Overline");
        case StrikeOut:
            return qsl("StrikeOut");
        case Reverse:
            return qsl("Reverse");
        case Blink:
            return qsl("Blink");
        case FastBlink:
            return qsl("FastBlink");
        case AltFont1:
            return qsl("AltFont1");
        case AltFont2:
            return qsl("AltFont2");
        case AltFont3:
            return qsl("AltFont3");
        case AltFont4:
            return qsl("AltFont4");
        case AltFont5:
            return qsl("AltFont5");
        case AltFont6:
            return qsl("AltFont6");
        case AltFont7:
            return qsl("AltFont7");
        case AltFont8:
            return qsl("AltFont8");
        case AltFont9:
            return qsl("AltFont9");
        case Concealed:
            return qsl("Concealed");
        case UnderlineWavy:
            return qsl("UnderlineWavy");
        case UnderlineDotted:
            return qsl("UnderlineDotted");
        case UnderlineDashed:
            return qsl("UnderlineDashed");
        default:
            return qsl("Unknown");
        }
    }

private:
    QColor mFgColor;
    QColor mBgColor;
    AttributeFlags mFlags = None;
    // Kept as a separate flag because it must often be handled separately
    bool mIsSelected = false;
    int mLinkIndex = 0;

    // Enhanced decoration color support for OSC 8 hyperlinks
    QColor mUnderlineColor;
    QColor mOverlineColor;
    QColor mStrikeoutColor;
    bool mHasCustomUnderlineColor = false;
    bool mHasCustomOverlineColor = false;
    bool mHasCustomStrikeoutColor = false;
};
Q_DECLARE_OPERATORS_FOR_FLAGS(TChar::AttributeFlags)




class TBuffer
{
    inline static const TEncodingTable &csmEncodingTable = TEncodingTable::csmDefaultInstance;

    inline static const int TCHAR_IN_BYTES = sizeof(TChar);

    // limit on how many characters a single echo can accept for performance reasons
    inline static const int MAX_CHARACTERS_PER_ECHO = 1000000;

public:
    explicit TBuffer(Host* pH, TConsole* pConsole = nullptr);
    ~TBuffer();
    TBuffer(const TBuffer& other);
    TBuffer& operator=(const TBuffer& other);
    QPoint insert(QPoint&, const QString& text, int, int, int, int, int, int, bool bold, bool italics, bool underline, bool strikeout);
    bool insertInLine(QPoint& cursor, const QString& what, const TChar& format);
    void expandLine(int y, int count, TChar&);
    int wrapLine(int startLine, int maxWidth, int indentSize, int hangingIndentSize);
    void log(int, int);
    int skipSpacesAtBeginOfLine(const int row, const int column);
    void addLink(bool, const QString& text, QStringList& command, QStringList& hint, TChar format, QVector<int> luaReference = QVector<int>());
    QString bufferToHtml(const bool showTimeStamp = false, const int row = -1, const int endColumn = -1, const int startColumn = 0,  int spacePadding = 0);
    int size() { return static_cast<int>(buffer.size()); }
    bool isEmpty() const { return buffer.size() == 0; }
    QString& line(int lineNumber);
    int find(int line, const QString& what, int pos);
    QStringList split(int line, const QString& splitter);
    QStringList split(int line, const QRegularExpression& splitter);
    bool replaceInLine(QPoint& start, QPoint& end, const QString& with, TChar& format);
    bool deleteLine(int);
    bool deleteLines(int from, int to);
    bool applyAttribute(const QPoint& P_begin, const QPoint& P_end, const TChar::AttributeFlags attributes, const bool state);
    bool applyLink(const QPoint& P_begin, const QPoint& P_end, const QStringList& linkFunction, const QStringList& linkHist, QVector<int> luaReference = QVector<int>());
    bool applyFgColor(const QPoint&, const QPoint&, const QColor&);
    bool applyBgColor(const QPoint&, const QPoint&, const QColor&);
    void appendBuffer(const TBuffer& chunk);
    bool moveCursor(QPoint& where);
    int getLastLineNumber();
    QStringList getEndLines(int);
    void clear();
    QPoint getEndPos();
    void translateToPlainText(std::string& incoming, bool isFromServer = false);
    void append(const QString& chunk, int sub_start, int sub_end, const QColor& fg, const QColor& bg, const TChar::AttributeFlags flags = TChar::None, const int linkID = 0);
    // Only the bits within TChar::TestMask are considered for formatting:
    void append(const QString& chunk, const int sub_start, const int sub_end, const TChar format, const int linkID = 0);
    void appendLine(const QString& chunk, const int sub_start, const int sub_end, const QColor& fg, const QColor& bg, TChar::AttributeFlags flags = TChar::None, const int linkID = 0);
    void appendEmptyLine();
    void setWrapAt(int i) { mWrapAt = i; }
    void setWrapIndent(int i) { mWrapIndent = i; }
    void setWrapHangingIndent(int i) { mWrapHangingIndent = i; }
    void updateColors();
    TBuffer copy(QPoint&, QPoint&);
    TBuffer cut(QPoint&, QPoint&);
    void paste(QPoint&, const TBuffer&);
    void setBufferSize(int requestedLinesLimit, int batch);
    int getMaxBufferSize();
    static const QList<QByteArray> getEncodingNames();
    void logRemainingOutput();
    void appendLog(const QString &text);

    // OSC 8 hyperlink documentation examples - triggered by secret phrase
    void injectOSC8DocumentationExamples();

    // It would have been nice to do this with Qt's signals and slots but that
    // is apparently incompatible with using a default constructor - sigh!
    void encodingChanged(const QByteArray &);
    void clearSearchHighlights();

    static int lengthInGraphemes(const QString& text);


    std::deque<TChar> bufferLine;
    // stores the text attributes (TChars) that make up each line of text in the buffer
    std::deque<std::deque<TChar>> buffer;
    // stores the actual content of lines
    QStringList lineBuffer;
    // stores timestamps associated with lines
    QStringList timeBuffer;
    // stores a boolean whenever the line is a prompt one
    QList<bool> promptBuffer;
    TLinkStore mLinkStore;
    int mLinesLimit = 10000;
    int mBatchDeleteSize = 1000;
    int mWrapAt = 99999999;
    int mWrapIndent = 0;
    int mWrapHangingIndent = 0;
    int mCursorY = 0;
    bool mEchoingText = false;

private:
    inline QList<WrapInfo> getWrapInfo(const QString& lineText, bool isNewline, const int maxWidth, const int indent, const int hangingIndent);
    void shrinkBuffer();
    int calculateWrapPosition(int lineNumber, int begin, int end);
    void handleNewLine();
    bool processUtf8Sequence(const std::string&, bool, size_t, size_t&, bool&);
    bool processGBSequence(const std::string&, bool, bool, size_t, size_t&, bool&);
    bool processBig5Sequence(const std::string&, bool, size_t, size_t&, bool&);
    bool processEUC_KRSequence(const std::string&, bool, size_t, size_t&, bool&);
    void decodeSGR(const QString&);
    void decodeSGR38(const QStringList&, bool isColonSeparated = true);
    void decodeSGR48(const QStringList&, bool isColonSeparated = true);
    void decodeOSC(const QString&);
    void resetColors();
    bool commitLine(char ch, size_t& localBufferPosition);
    void processMxpWatchdogCallback();
    TChar::AttributeFlags computeCurrentAttributeFlags() const;

    // Helper function for parsing URI query parameters in OSC 8 hyperlinks
    QMap<QString, QString> parseUriQueryParameters(const QString& uri);
    // Helper function for parsing JSON format hyperlink configuration
    bool parseJsonHyperlinkConfig(const QString& jsonString, QMap<QString, QString>& parameters);
    // Helper functions for JSON to CSS conversion
    QString jsonStyleObjectToCss(const QJsonObject& styleObj);
    QString jsonMenuArrayToString(const QJsonArray& menuArray);
    // Helper function for appending query parameters to URIs (handles existing params)
    QString appendQueryParameters(const QString& uri, const QMap<QString, QString>& parameters);
    // Helper function for parsing CSS-like style strings
    void parseHyperlinkStyling(const QString& styleString, Mudlet::HyperlinkStyling& styling);
    // Helper function for parsing color values (hex, named, rgb)
    QColor parseColorValue(const QString& value);
    // CSS Link State parsing and management
    void parseHyperlinkStateStyle(const QString& pseudoClass, const QString& styleString, Mudlet::HyperlinkStyling& styling);
    void parseStateStyleProperties(const QString& styleString, Mudlet::HyperlinkStyling::StateStyle& stateStyle);
    void applyAccessibilityEnhancements(Mudlet::HyperlinkStyling& styling);


    QPointer<TConsole> mpConsole;

    // First stage in decoding SGR/OCS sequences - set true when we see the
    // ASCII ESC character:
    bool mGotESC = false;
    // Second stage in decoding SGR sequences - set true when we see the ASCII
    // ESC character followed by the '[' one:
    bool mGotCSI = false;
    // Second stage in decoding OSC sequences - set true when we see the ASCII
    // ESC character followed by the ']' one:
    bool mGotOSC = false;
    bool mIsDefaultColor = true;


    QColor mBlack;
    QColor mLightBlack;
    QColor mRed;
    QColor mLightRed;
    QColor mLightGreen;
    QColor mGreen;
    QColor mLightBlue;
    QColor mBlue;
    QColor mLightYellow;
    QColor mYellow;
    QColor mLightCyan;
    QColor mCyan;
    QColor mLightMagenta;
    QColor mMagenta;
    QColor mLightWhite;
    QColor mWhite;
    // These three replace three sets of three integers that were used to hold
    // colour components during the parsing of SGR sequences, they were called:
    // fgColor{R|G|B}, fgColorLight{R|G|B} and bgColor{R|G|B} apart from
    // anything else, the first and last sets had the same names as arguments
    // to several of the methods which meant the latter shadowed and masked
    // them off!
    QColor mForeGroundColor;
    QColor mForeGroundColorLight;
    QColor mBackGroundColor;

    QPointer<Host> mpHost;

    bool mBold = false;
    bool mItalics = false;
    bool mOverline = false;
    bool mReverse = false;
    bool mStrikeOut = false;
    bool mUnderline = false;
    bool mUnderlineWavy = false;
    bool mUnderlineDotted = false;
    bool mUnderlineDashed = false;
    // If BOTH of these ever get set than only mFastBlink is to be considered
    // set - when setting one ensure the other is reset:
    bool mBlink = false;
    bool mFastBlink = false;
    bool mConcealed = false;
    quint8 mAltFont = 0;

    QString mMudLine;
    std::deque<TChar> mMudBuffer;
    // Used to hold the unprocessed bytes that could be left at the end of a
    // packet if we detect that there should be more - will be prepended to the
    // next chunk of data - PROVIDED it is flagged as coming from the MUD Server
    // and is not generated locally {because both pass through
    // translateToPlainText()}:
    std::string mIncompleteSequenceBytes;

    // keeps track of the previously logged buffer lines to ensure no log duplication
    // happens when you enter a command
    int lastLoggedFromLine = 0;
    int lastloggedToLine = 0;
    QString lastTextToLog;

    QByteArray mEncoding;
    QTextCodec* mMainIncomingCodec = nullptr;

    // OSC 8 hyperlink tracking
    QStringList mCurrentHyperlinkCommand;
    QStringList mCurrentHyperlinkHint;
    int mCurrentHyperlinkLinkId = 0;
    bool mHyperlinkActive = false;

    enum class WatchdogPhase {
        Phase1_Snapshot,
        Phase2_Unfreeze,
        None
    };
    static constexpr int    MAX_TAG_TIMEOUT_MS = 1300;
    WatchdogPhase           mWatchdogPhase = WatchdogPhase::None;
    std::unique_ptr<QTimer> mTagWatchdog;
    std::string             mWatchdogTagSnapshot;

    // Enhanced OSC 8 hyperlink styling and menu support
    Mudlet::HyperlinkStyling mCurrentHyperlinkStyling;
    QStringList mCurrentHyperlinkMenu; // Format: "Label|Command|Label|Command..."

    // Link state tracking for interactive pseudo-classes
    QMap<int, Mudlet::HyperlinkStyling::LinkState> mLinkStates; // Track current state per linkIndex
    QMap<int, bool> mVisitedLinks; // Track which links have been visited (base state)
    QMap<int, QColor> mLinkOriginalBackgrounds; // Track original background color per link
    QMap<int, TChar> mLinkOriginalCharacters; // Track original ANSI formatting per link
    int mCurrentHoveredLinkIndex = 0;  // Which link is currently hovered (0 = none)
    int mCurrentActiveLinkIndex = 0;   // Which link is currently being clicked (0 = none)
    int mCurrentFocusedLinkIndex = 0;  // Which link has keyboard focus (0 = none)

public:
    // Methods for link state management (used by TTextEdit event handlers)
    void setLinkState(int linkIndex, Mudlet::HyperlinkStyling::LinkState state);
    Mudlet::HyperlinkStyling::LinkState getLinkState(int linkIndex) const;
    Mudlet::HyperlinkStyling getEffectiveHyperlinkStyling(int linkIndex) const;
    void setHoveredLink(int linkIndex);
    void setActiveLink(int linkIndex);
    void setFocusedLink(int linkIndex);
    void markLinkAsVisited(int linkIndex); // Mark a link as visited (base state)
    bool isLinkVisited(int linkIndex) const; // Check if link has been visited
    int getHoveredLink() const { return mCurrentHoveredLinkIndex; }
    int getActiveLink() const { return mCurrentActiveLinkIndex; }
    int getFocusedLink() const { return mCurrentFocusedLinkIndex; }
    int getLinkIndexAt(int line, int column) const; // Get link index at specific position

private:
    // Update all TChar objects that belong to a specific link with effective styling
    void updateLinkCharacters(int linkIndex);
};

#ifndef QT_NO_DEBUG_STREAM
// Dumper for the TChar::AttributeFlags - so that qDebug gives a detailed broken
// down results when presented with the value rather than just a hex value.
// Note "inline" is REQUIRED:
inline QDebug& operator<<(QDebug& debug, const TChar::AttributeFlags& attributes)
{
    const QDebugStateSaver saver(debug);
    QString result = QLatin1String("TChar::AttributeFlags(");
    QStringList presentAttributes;
    if (attributes & TChar::Bold) {
        presentAttributes << QLatin1String("Bold (0x01)");
    }
    if (attributes & TChar::Italic) {
        presentAttributes << QLatin1String("Italic (0x02)");
    }
    if (attributes & TChar::Underline) {
        presentAttributes << QLatin1String("Underline (0x04)");
    }
    if (attributes & TChar::Overline) {
        presentAttributes << QLatin1String("Overline (0x08)");
    }
    if (attributes & TChar::StrikeOut) {
        presentAttributes << QLatin1String("StrikeOut (0x10)");
    }
    if (attributes & TChar::Reverse) {
        presentAttributes << QLatin1String("Reverse (0x20)");
    }
    if (attributes & TChar::Blink) {
        presentAttributes << QLatin1String("Blink (0x40)");
    }
    if (attributes & TChar::FastBlink) {
        presentAttributes << QLatin1String("FastBlink (0x80)");
    }
    if (attributes & TChar::AltFont1) {
        presentAttributes << QLatin1String("AltFont1 (0x100)");
    }
    if (attributes & TChar::AltFont2) {
        presentAttributes << QLatin1String("AltFont2 (0x200)");
    }
    if (attributes & TChar::AltFont3) {
        presentAttributes << QLatin1String("AltFont3 (0x400)");
    }
    if (attributes & TChar::AltFont4) {
        presentAttributes << QLatin1String("AltFont4 (0x800)");
    }
    if (attributes & TChar::AltFont5) {
        presentAttributes << QLatin1String("AltFont5 (0x1000)");
    }
    if (attributes & TChar::AltFont6) {
        presentAttributes << QLatin1String("AltFont6 (0x2000)");
    }
    if (attributes & TChar::AltFont7) {
        presentAttributes << QLatin1String("AltFont7 (0x4000)");
    }
    if (attributes & TChar::AltFont8) {
        presentAttributes << QLatin1String("AltFont8 (0x8000)");
    }
    if (attributes & TChar::AltFont9) {
        presentAttributes << QLatin1String("AltFont9 (0x10000)");
    }
    if (attributes & TChar::Concealed) {
        presentAttributes << QLatin1String("Concealed (0x20000)");
    }
    if (attributes & TChar::Found) {
        presentAttributes << QLatin1String("Found (0x100000)");
    }
    if (attributes & TChar::Echo) {
        presentAttributes << QLatin1String("Echo (0x200000)");
    }
    if (attributes & TChar::UnderlineWavy) {
        presentAttributes << QLatin1String("UnderlineWavy (0x400000)");
    }
    if (attributes & TChar::UnderlineDotted) {
        presentAttributes << QLatin1String("UnderlineDotted (0x800000)");
    }
    if (attributes & TChar::UnderlineDashed) {
        presentAttributes << QLatin1String("UnderlineDashed (0x1000000)");
    }
    if (presentAttributes.isEmpty()) {
        result.append(QLatin1String("None (0x0))"));
    } else {
        result.append(presentAttributes.join(QLatin1String(", ")).append(QLatin1String(")")));
    }
    debug.nospace().noquote() << result;
    return debug;
}
#endif // QT_NO_DEBUG_STREAM

#endif // MUDLET_TBUFFER_H
