#ifndef MUDLET_TTRIGGER_H
#define MUDLET_TTRIGGER_H

/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2017-2018, 2026 by Stephen Lyons                        *
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


#include "TMatchState.h"
#include "Tree.h"
#include "utils.h" // For NameGroupMatches

#include <QColor>
#include <QDebug>
#include <QDebugStateSaver>
#include <QList>
#include <QMap>
#include <QPointer>
#include <QSharedPointer>
#include <QString>
#include <QStringList>
#include <QStringMatcher>
#include <QtGlobal>

#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

#include <list>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <QCoreApplication>

class Host;
class TLuaInterpreter;
class TMatchState;


#define REGEX_SUBSTRING 0
#define REGEX_PERL 1
#define REGEX_BEGIN_OF_LINE_SUBSTRING 2
#define REGEX_EXACT_MATCH 3
#define REGEX_LUA_CODE 4
#define REGEX_LINE_SPACER 5
#define REGEX_COLOR_PATTERN 6
#define REGEX_PROMPT 7

struct TColorTable
{
    int ansiFg;
    int ansiBg;
    QColor mFgColor;
    QColor mBgColor;
    // The same colors in the form the characters store theirs in, converted
    // once here rather than on every line. An ignored aspect, or an ANSI code
    // the table has no color for, leaves the QColor invalid and the flag clear
    QRgb mFgRgba = 0;
    QRgb mBgRgba = 0;
    bool mFgValid = false;
    bool mBgValid = false;
};

// A 256-bit Bloom filter over a line's adjacent character pairs: a substring pattern missing any of
// its own pairs cannot occur in the line. Holds only while the search compares exactly as bitsFor()
// does; a case-insensitive matcher would need a case-insensitive summary too.
class TBigramFilter
{
public:
    struct Bits
    {
        static constexpr int scmWords = 4;
        quint64 words[scmWords]{};
    };

    static Bits bitsFor(const QString& text);

    // Summarising a line costs about as much as searching it 3-4 times, so it only pays with several
    // substring patterns. The previous line's question count predicts this one's:
    static constexpr int scmQuestionsWorthSummarising = 5;

    TBigramFilter(const QString& line, const int questionsOnThePreviousLine)
    : mLine(line)
    , mSummarise(questionsOnThePreviousLine >= scmQuestionsWorthSummarising)
    {
    }
    TBigramFilter(QString&&, int) = delete;
    Q_DISABLE_COPY_MOVE(TBigramFilter)

    // haystack is only checked to be the line these bits describe: answering about any other string
    // (a capture, a slice of the line) would silently stop triggers firing.
    bool couldContain(const QString& haystack, const Bits& pattern) const
    {
        Q_ASSERT(haystack.constData() == mLine.constData());
        Q_UNUSED(haystack)
        ++mQuestionsAsked;
        if (!mSummarise) {
            return true;
        }
        if (!mBuilt) {
            mLineBits = bitsFor(mLine);
            mBuilt = true;
        }
        for (int i = 0; i < Bits::scmWords; ++i) {
            if (pattern.words[i] & ~mLineBits.words[i]) {
                return false;
            }
        }
        return true;
    }

    // Call on the main thread before couldContainShared() is asked from other threads, so nothing is
    // left to build lazily.
    void prepareForSharing() const
    {
        if (mSummarise && !mBuilt) {
            mLineBits = bitsFor(mLine);
            mBuilt = true;
        }
    }

    // couldContain() for other threads: read-only, so it neither builds the summary nor counts the
    // question. The main thread still asks about every pattern the prescan does not rule out, so few
    // go uncounted.
    bool couldContainShared(const QString& haystack, const Bits& pattern) const
    {
        Q_ASSERT(haystack.constData() == mLine.constData());
        Q_UNUSED(haystack)
        if (!mSummarise) {
            return true;
        }
        Q_ASSERT_X(mBuilt, "TBigramFilter::couldContainShared", "prepareForSharing() has to run on the main thread before another thread asks");
        for (int i = 0; i < Bits::scmWords; ++i) {
            if (pattern.words[i] & ~mLineBits.words[i]) {
                return false;
            }
        }
        return true;
    }

    int questionsAsked() const { return mQuestionsAsked; }

private:
    const QString& mLine;
    const bool mSummarise;
    mutable Bits mLineBits;
    mutable bool mBuilt = false;
    mutable int mQuestionsAsked = 0;
};

// What TriggerUnit copies out of a root trigger, so that a line can dismiss it
// without reading the trigger at all - see TTrigger::rootFilter(). A copy goes
// stale the moment the trigger changes, so whatever changes one of the things
// rootFilter() reads has to go through TTrigger::invalidatePrescan(), and
// whatever makes a trigger fire without matching through
// TriggerUnit::markPrescanStaleForLineInFlight(), which bumps mRootFilterEpoch.
struct TRootTriggerFilter
{
    enum class Kind : quint8 {
        // Nothing here can dismiss it, which leaves cannotMatch() and match()
        // to decide as they would have anyway
        Visit,
        // mText is all there is to cannotMatch(), so that need not be asked
        Text,
        // It can only match a line holding this color, which a line that is
        // one color throughout either is or is not
        Color
    };

    // Whether a line of just this one color pair has nothing for the trigger
    bool lacksColors(const QRgb foreground, const QRgb background) const { return (mForegroundWanted && foreground != mForeground) || (mBackgroundWanted && background != mBackground); }

    TBigramFilter::Bits mText;
    QRgb mForeground = 0;
    QRgb mBackground = 0;
    bool mForegroundWanted = false;
    bool mBackgroundWanted = false;
    Kind mKind = Kind::Visit;
};

// The UTF-8 bytes a perl pattern matches against. A line is encoded the first
// time one asks for them, which with the literal pre-check most never do, so
// a line no perl pattern gets as far as is not encoded at all. A filter's
// capture is bytes already and is handed over as it is.
class TUtf8Subject
{
public:
    // Encodes line into scratch when first asked; the line has to outlive
    // this object. Passing the storage in rather than allocating it is what
    // lets a line no longer than any before it allocate nothing.
    TUtf8Subject(const QString& line, QByteArray&& scratch)
    : mpLine(&line)
    , mpPendingLine(&line)
    , mScratch(std::move(scratch))
    {
    }
    TUtf8Subject(QString&&, QByteArray&&) = delete;
    // Already encoded bytes, which have to outlive this object
    TUtf8Subject(const char* data, const int length)
    : mData(data)
    , mLength(length)
    {
    }
    Q_DISABLE_COPY_MOVE(TUtf8Subject)

    const char* data() const
    {
        if (mpPendingLine) {
            encode();
        }
        return mData;
    }
    // Perl patterns see the line only as far as its first NUL byte, so this
    // is not the byte count
    int length() const
    {
        if (mpPendingLine) {
            encode();
        }
        return mLength;
    }
    // Hands the encoding storage back, to be lent to the next line
    QByteArray takeScratch() { return std::move(mScratch); }
    // An unpaired surrogate does not reach the UTF-8, which joins the text on
    // either side of it for pcre2, so searching the QString cannot rule a
    // pattern out on such a line
    bool dropsText() const
    {
        if (mDropsText < 0) {
            mDropsText = mpLine && !mpLine->isValidUtf16();
        }
        return mDropsText;
    }

private:
    void encode() const;

    const QString* const mpLine = nullptr;
    mutable qint8 mDropsText = -1;
    mutable const QString* mpPendingLine = nullptr;
    mutable QByteArray mScratch;
    mutable const char* mData = nullptr;
    mutable int mLength = 0;
};

class TTrigger : public Tree<TTrigger>
{
    Q_DECLARE_TR_FUNCTIONS(TTrigger) // Needed so we can use tr() even though TTrigger is NOT derived from QObject
    friend class CorruptTriggerPatternsTest;
    friend class XMLexport;
    friend class XMLimport;

public:
    virtual ~TTrigger();
    TTrigger(TTrigger* parent, Host* pHost);
    TTrigger(const QString& name, const QStringList& patterns, const QList<int>& patternKinds, bool isMultiline, Host* pHost); //throws exception ExObjNoCreate

    // Used as ANSI color code for either fore or back ground in color triggers
    // that is not considered when checking the color - both being set to this
    // is an error:
    static const int scmIgnored;
    // Used as ANSI color code for either fore or back ground in color triggers
    // and corresponds to matching against the "default" colour - e.g. whatever
    // is used immediately after a "<ESC>[0m" sequence - or "<ESC>[39m" for
    // foreground and "<ESC>[49m" for background:
    // It cannot be 0 because ANSI color 0 is "black" and that is chosen
    // by "<ESC>[30m" (foreground) or "<ESC>[40m" (background) and the default
    // need not be black on white / white on black.
    static const int scmDefault;

    QString getCommand() const { return mCommand; }
    void compileAll();
    void setCommand(const QString& b) { mCommand = b; }
    QString getName() const { return mName; }
    void setName(const QString& name);
    const QStringList& getPatternsList() const { return mPatterns; }
    QList<int> getRegexCodePropertyList() const { return mPatternKinds; }
    QColor getFgColor() const { return mFgColor; }
    QColor getBgColor() const { return mBgColor; }
    void setColorizerFgColor(const QColor& c) { mFgColor = c; }
    void setColorizerBgColor(const QColor& c) { mBgColor = c; }
    bool isColorizerTrigger() const { return mIsColorizerTrigger; }
    void setIsColorizerTrigger(const bool b) { mIsColorizerTrigger = b; }
    void compile();
    void execute();
    bool isFilterChain();
    bool setRegexCodeList(QStringList patterns, QList<int> patternKinds, bool existingTrigger = true);
    void rebuildPrescanGrams();
    QString getScript() const { return mScript; }
    bool setScript(const QString& script);
    bool compileScript();
    bool match(const TUtf8Subject& subject, const QString&, int line, int posOffset = 0, const TBigramFilter* pLineBigrams = nullptr);
    // Runs only the patterns that are a pure function of the line, just far enough for yes or no.
    // Called from helper threads, so it writes nothing shared: PCRE2 match data and the regexSearches
    // tally are the caller's own. Answers yes to anything it cannot decide, so false is a promise and
    // true only a maybe. The main thread must have called lineBigrams.prepareForSharing().
    // lineDropsText is the line's TUtf8Subject::dropsText(), asked on the main thread as it is cached
    // lazily; it gates match_perl()'s literal pre-check, which only holds while the UTF-8 and the
    // QString carry the same text.
    bool prescanMayFire(const char* haystackC, int haystackCLength, const QString& haystack, const TBigramFilter& lineBigrams, bool lineDropsText, pcre2_match_data* scratch, int& regexSearches) const;
    // Regex searches match() has run on the main thread across all profiles, counting only those a
    // prescan could have run instead (not a multiline trigger's). Read before and after a pass to cost it.
    static quint64 regexSearches() { return smRegexSearches; }
    // Written from a worker thread, and only for a trigger no other worker is holding.
    void setPrescanVerdict(const quint32 passId, const bool mayFire)
    {
        mPrescanPassId = passId;
        mPrescanMayFire = mayFire;
    }
    // Which pass's verdicts match() believes. Zero while no prescan is in force, which is also an
    // untouched trigger's id, so a trigger the prescan never visited is never mistaken for one it cleared.
    static quint32 prescanPassId() { return smPrescanPassId; }
    static void setPrescanPassId(const quint32 id) { smPrescanPassId = id; }
    // Zero is skipped on wrap so it keeps meaning "no prescan in force".
    static quint32 nextPrescanPassId()
    {
        if (++smPrescanPassIdCounter == 0) {
            ++smPrescanPassIdCounter;
        }
        return smPrescanPassIdCounter;
    }
    // Bumped when the tree changes shape or a pattern is recompiled, so a flattened list can tell it is stale
    static quint64 structureGeneration() { return smStructureGeneration; }
    static void bumpStructureGeneration() { ++smStructureGeneration; }
    bool checkIfNew();
    void unmarkAsNew();
    // Empty when the trigger cannot be decided from the line's text alone, in
    // which case TTriggerPrescan offers it every line.
    const std::vector<quint64>& prescanGrams() const;
    void invalidatePrescan(bool nowFiresWithoutMatching = false);
    // Whether the line's bigram summary already rules this trigger out, so match() need not be entered.
    // One-sided like the summary: false leaves match() to decide. Only a trigger whose every pattern
    // matches by containing text, and that never fires on a line it does not match - the cases
    // prescanGrams() reads, taken live so a script changing them mid-line is seen at once.
    bool cannotMatch(const TBigramFilter& lineBigrams, const QString& line) const
    {
        if (mPatternBigrams.empty() || mIsLineTrigger || mIsMultiline || mKeepFiring > 0) {
            return false;
        }
        for (const TBigramFilter::Bits& bits : mPatternBigrams) {
            if (lineBigrams.couldContain(line, bits)) {
                return false;
            }
        }
        return true;
    }
    // What a line can dismiss this trigger by without calling into it - the cases cannotMatch() and
    // match_color_pattern() decide, copied only for a single-pattern trigger that never fires on a
    // line it does not match.
    TRootTriggerFilter rootFilter() const;
    // The one color pair a root trigger's color pattern would find across the whole line, as
    // match_color_pattern() reads it; false when the line has more than one, or cannot be answered for.
    static bool uniformLineColors(Host* pHost, int line, int length, QRgb& foreground, QRgb& background);
    // Never filed by a snapshot: a child, which the index never files, or a root still queued for
    // appending. No pinned pass can hold a filter copy of it.
    static constexpr int scmNeverSnapshotted = -1;
    // Left the root list since last filed. A snapshot pinned before the removal still holds it and its
    // filter copy, so a change to it still has to be announced.
    static constexpr int scmSnapshotPositionDropped = -2;
    // Position in TriggerUnit's root-node snapshot, or one of the two sentinels above.
    // Only TriggerUnit may set it.
    int rootSnapshotPosition() const { return mRootSnapshotPosition; }
    void setRootSnapshotPosition(const int position) { mRootSnapshotPosition = position; }

    bool isMultiline() const { return mIsMultiline; }
    int getTriggerType() const { return mTriggerType; }
    bool isLineTrigger() const { return mIsLineTrigger; }
    void setIsLineTrigger(bool b)
    {
        mIsLineTrigger = b;
        invalidatePrescan(b);
    }
    void setStartOfLineDelta(int b) { mStartOfLineDelta = b; }
    void setLineDelta(int b) { mLineDelta = b; }
    void setTriggerType(int b) { mTriggerType = b; }
    void setIsMultiline(bool b)
    {
        mIsMultiline = b;
        invalidatePrescan(b);
    }
    void enableTrigger(const QString&);
    void disableTrigger(const QString&);
    TTrigger* killTrigger(const QString&);
    bool match_substring(const QString&, const QString&, int, int posOffset, int lineNumber, const TBigramFilter* pLineBigrams);
    bool match_perl(const TUtf8Subject& subject, const QString&, int, int posOffset, int lineNumber, const TBigramFilter* pLineBigrams = nullptr);
    bool match_exact_match(const QString&, const QString&, int, int posOffset, int lineNumber);
    bool match_begin_of_line_substring(const QString& haystack, const QString& needle, int patternNumber, int posOffset, int lineNumber);
    bool match_lua_code(int);
    bool match_line_spacer(int patternNumber);
    bool match_color_pattern(int line, int patternNumber, int posOffset, int length);
    bool match_prompt(int patternNumber);
    void setConditionLineDelta(int delta) { mConditionLineDelta = delta; }
    int getConditionLineDelta() const { return mConditionLineDelta; }
    bool registerTrigger();
    void setSound(const QString& file) { mSoundFile = file; }
    bool setupColorTrigger(int, int);
    bool setupTmpColorTrigger(int ansiFg, int ansiBg);
    std::unique_ptr<TColorTable> createColorPattern(int, int);
    static QString createColorPatternText(const int fgColorCode, const int bgColorCode);
    static void decodeColorPatternText(const QString& patternText, int& fgColorCode, int& bgColorCode);
    QString packageName(TTrigger* pTrigger);
    QString moduleName(TTrigger* pTrigger);


    bool mTriggerContainsPerlRegex = false;
    bool mPerlSlashGOption = false;
    bool mFilterTrigger = false;
    bool mSoundTrigger = false;
    QString mSoundFile;
    int mStayOpen = 0;
    bool mColorTrigger = false;
    std::vector<std::unique_ptr<TColorTable>> mColorPatternList;
    // The next four members refer to the details of the currently selected
    // color trigger pattern item - it is not obvious that they need to be
    // stored in the profile even though they are:
    QColor mColorTriggerFgColor;
    QColor mColorTriggerBgColor;
    int mColorTriggerFgAnsi = scmIgnored;
    int mColorTriggerBgAnsi = scmIgnored;
    int mKeepFiring = 0;
    QPointer<Host> mpHost;
    QString mName;
    QStringList mPatterns;
    std::vector<quint64> mPrescanGrams;
    int mRootSnapshotPosition = scmNeverSnapshotted;
    bool exportItem = true;
    bool mModuleMasterFolder = false;
    // specifies whenever the payload is Lua code as a string
    // or a function
    bool mRegisteredAnonymousLuaFunction = false;
    bool mIsNew = true;

    int getExpiryCount() const;
    void setExpiryCount(int expiryCount);

    // Set when the trigger is registered as a root node while a line is being
    // processed, and cleared when that line is done with - see TriggerUnit's
    // same-line creation chains. The id names the lineage this trigger belongs
    // to, the generation is how many creations deep in it this trigger sits;
    // everything its script creates during that line joins the same lineage one
    // generation further down.
    int sameLineChainId() const { return mSameLineChainId; }
    int sameLineGeneration() const { return mSameLineGeneration; }
    void setSameLineChain(const int chainId, const int generation)
    {
        mSameLineChainId = chainId;
        mSameLineGeneration = generation;
    }


private:
    TTrigger() = default;

    inline void updateMultistates(int regexNumber, std::list<std::string>& captureList, std::list<int>& posList, const NameGroupMatches* nameMatches = nullptr);
    inline void filter(std::string&, int&, int lineNumber);
    void processExactMatch(int patternNumber, int posOffset, int lineNumber);
    void processRegexMatch(const char* haystackC,
                           const QString& haystack,
                           int patternNumber,
                           int posOffset,
                           const QSharedPointer<pcre2_code>& re,
                           int haystackCLength,
                           pcre2_match_data* match_data,
                           int rc,
                           int lineNumber);
    void processBeginOfLine(int patternNumber, int posOffset, int lineNumber);
    void processSubstringMatch(const QString& haystack, const QString& needle, int regexNumber, int posOffset, int where, int lineNumber);
    void processColorPattern(int patternNumber, std::list<std::string>& captureList, std::list<int>& posList, int lineNumber);
    void processPromptMatch(int patternNumber);
    const std::string& patternUtf8(int patternNumber) const;


    QList<int> mPatternKinds;
    // matcher is null for non-substring patterns; kept beside its bits so the two can't fall out of step
    struct TSubstringPattern
    {
        std::unique_ptr<QStringMatcher> matcher;
        TBigramFilter::Bits bigrams;
    };
    // Indexed, not keyed, by pattern number: every line reaches these for every pattern of every trigger,
    // too hot for a tree lookup and a reference count
    std::vector<TSubstringPattern> mSubstringPatterns;
    // Text every match of a perl pattern has to contain, prepared like a
    // substring pattern so that a line without it is dismissed without asking
    // pcre2. A null matcher for every other pattern kind and for a perl pattern
    // that guarantees no text of two characters or more
    struct TRegexLiteral
    {
        std::unique_ptr<QStringMatcher> matcher;
        TBigramFilter::Bits bigrams;
    };
    std::vector<TRegexLiteral> mRegexLiterals;
    // One entry per pattern while every pattern of the trigger can be dismissed
    // by a line's bigram summary; empty as soon as one cannot, which hands every
    // line to match() - see cannotMatch()
    std::vector<TBigramFilter::Bits> mPatternBigrams;
    std::vector<QSharedPointer<pcre2_code>> mRegexes;
    std::vector<QSharedPointer<pcre2_match_data>> mMatchData;
    // char, not bool, to avoid the bit-packed vector<bool> specialisation
    std::vector<char> mRegexJitCompiled;
    std::vector<std::string> mPatternsUtf8;

    // Lua code as a string to run
    QString mScript;

    bool mNeedsToBeCompiled = true;
    int mTriggerType = REGEX_SUBSTRING;

    bool mIsLineTrigger = false;
    int mStartOfLineDelta = 0;
    int mLineDelta = 3;
    bool mIsMultiline = false;
    int mConditionLineDelta = 0;
    QString mCommand;
    // Key is the raw address of the owned TMatchState — stable once inserted and
    // used for O(1) lookup during the deferred-removal pass in match(). The map
    // is the sole owner; the raw pointer is never passed out as an observer.
    std::map<TMatchState*, std::unique_ptr<TMatchState>> mConditionMap;
    std::list<std::list<std::string>> mMultiCaptureGroupList;
    std::list<std::list<int>> mMultiCaptureGroupPosList;
    TLuaInterpreter* mpLua;
    std::map<int, std::string> mLuaConditionMap;
    QString mFuncName;
    // The colors to use if mIsColorizeTrigger is true:
    QColor mFgColor{Qt::red};
    QColor mBgColor{Qt::yellow};
    bool mIsColorizerTrigger = false;
    bool mModuleMember = false;
    // -1: don't self-destruct, 0: delete, 1+: number of times it can still fire
    int mExpiryCount = -1;
    int mSameLineChainId = 0;
    int mSameLineGeneration = 0;
    static quint64 smStructureGeneration;
    static quint64 smRegexSearches;
    static quint32 smPrescanPassId;
    static quint32 smPrescanPassIdCounter;
    quint32 mPrescanPassId = 0;
    bool mPrescanMayFire = true;
};

#ifndef QT_NO_DEBUG_STREAM
inline QDebug& operator<<(QDebug& debug, const TTrigger* trigger)
{
    QDebugStateSaver saver(debug);
    Q_UNUSED(saver)

    if (!trigger) {
        return debug << "TTrigger(0x0) ";
    }
    debug.nospace() << "TTrigger(" << trigger->getName() << ")";
    debug.nospace() << ", id=" << trigger->getID();
    debug.nospace() << ", isFolder=" << trigger->isFolder();
    debug.nospace() << ", isActive=" << trigger->isActive();
    debug.nospace() << ", isTemporary=" << trigger->isTemporary();
    debug.nospace() << ", isMultiline=" << trigger->isMultiline();
    debug.nospace() << ", patterns=" << trigger->getPatternsList();
    debug.nospace() << ", regexCodes=" << trigger->getRegexCodePropertyList();
    debug.nospace() << ", script is in: " << (trigger->mRegisteredAnonymousLuaFunction ? "string" : "Lua function");
    debug.nospace() << ", script=" << trigger->getScript();
    debug.nospace() << ')';
    return debug;
}
#endif // QT_NO_DEBUG_STREAM

#endif // MUDLET_TTRIGGER_H
