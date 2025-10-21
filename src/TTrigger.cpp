/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2016 by Chris Leacy - cleacy1972@gmail.com              *
 *   Copyright (C) 2017-2018, 2021 by Stephen Lyons                        *
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


#include "TTrigger.h"


#include "Host.h"
#include "TConsole.h"
#include "TDebug.h"
#include "TMatchState.h"
#include "TMedia.h"
#include "mudlet.h"
#include "pre_guard.h"
#include <QRegularExpression>
#include "post_guard.h"

#include <cassert>
#include <sstream>

// Some extraordinary numbers outside of the range (0-255) used for ANSI colors:
// Changing them WILL modify the Lua API of TLuaInterpreter::tempColorTrigger
// and the replacement TLuaInterpreter::tempAnsiColorTrigger
const int TTrigger::scmDefault = -2;
const int TTrigger::scmIgnored = -1;

TTrigger::TTrigger( TTrigger * parent, Host * pHost )
: Tree<TTrigger>( parent )
, mpHost(pHost)
, mpLua(mpHost->getLuaInterpreter())
{
}

TTrigger::TTrigger(const QString& name, const QStringList& patterns, const QList<int>& patternKinds, bool isMultiline, Host* pHost)
: Tree<TTrigger>(nullptr)
, mpHost(pHost)
, mName(name)
, mPatterns(patterns)
, mPatternKinds(patternKinds)
, mIsMultiline(isMultiline)
, mpLua(mpHost->getLuaInterpreter())
{
    setRegexCodeList(patterns, patternKinds);
}

TTrigger::~TTrigger()
{
    QMutableListIterator<TColorTable*> itColorTable(mColorPatternList);
    while (itColorTable.hasNext()) {
        if (itColorTable.next()) {
//            qDebug() << "TTrigger::~TTrigger() INFO: removing TColorTable from mColorPatternList: (ansiFg:"
//                     << itColorTable.peekPrevious()->ansiFg
//                     << itColorTable.peekPrevious()->mFgColor
//                     << "ansiBg:"
//                     << itColorTable.peekPrevious()->ansiBg
//                     << itColorTable.peekPrevious()->mBgColor
//                     << ").";
            delete itColorTable.peekPrevious();
        }
        itColorTable.remove();
    }

    for (auto && [key, value] : mConditionMap) {
        delete value;
    }

    if (!mpHost) {
        return;
    }
    mpHost->getTriggerUnit()->unregisterTrigger(this);

    if (isTemporary()) {
        if (mScript.isEmpty()) {
            mpHost->mLuaInterpreter.delete_luafunction(this);
        } else {
            mpHost->mLuaInterpreter.delete_luafunction(mFuncName);
        }
    }
}

void TTrigger::setName(const QString& name)
{
    if (!isTemporary()) {
        mpHost->getTriggerUnit()->mLookupTable.remove( mName, this );
    }
    mName = name;
    mpHost->getTriggerUnit()->mLookupTable.insert(name, this);
}

static void pcre_deleter(pcre* pointer)
{
    pcre_free(pointer);
}

//FIXME: lock if code *OR* regex doesn't compile
bool TTrigger::setRegexCodeList(QStringList patterns, QList<int> patternKinds, bool existingTrigger)
{
    patterns.replaceInStrings("\n", "");
    mPatterns.clear();
    mRegexMap.clear();
    mPatternKinds.clear();
    mLuaConditionMap.clear();
    // mColorPatternList is filled with pointers to TColorTable instances that
    // were created with "new" and they need to be "delete"-d:
    QMutableListIterator<TColorTable*> itColorTable(mColorPatternList);
    while (itColorTable.hasNext()) {
        if (itColorTable.next()) {
//            qDebug() << "TTrigger::setRegexCodeList() INFO: removing TColorTable from mColorPatternList: (ansiFg:"
//                     << itColorTable.peekPrevious()->ansiFg
//                     << itColorTable.peekPrevious()->mFgColor
//                     << "ansiBg:"
//                     << itColorTable.peekPrevious()->ansiBg
//                     << itColorTable.peekPrevious()->mBgColor
//                     << ").";
            delete itColorTable.peekPrevious();
        }
        itColorTable.remove();
    }
    mTriggerContainsPerlRegex = false;

    if (patternKinds.size() != patterns.size()) {
        //FIXME: ronny managed to trigger this somehow
        qDebug() << "[CRITICAL ERROR (plz report):] Trigger name=" << mName << " aborting reason: patternKinds.size() != patterns.size()";
    }

    if (existingTrigger && (patternKinds.empty()) && (!isFolder()) && (!mColorTrigger)) {
        setError(tr("error: this trigger has no patterns defined"));
        mOK_init = false;
        return false;
    }

    bool state = true;

    if (existingTrigger) {
        for (int i = 0; i < patterns.size(); i++) {
            if (patterns.at(i).isEmpty() && patternKinds.at(i) != REGEX_PROMPT) {
                continue;
            }

            mPatterns.append(patterns.at(i));
            mPatternKinds.append(patternKinds.at(i));

            if (patternKinds.at(i) == REGEX_PERL) {
                const char* error;
                const QByteArray& regexp = patterns.at(i).toUtf8();

                int erroffset;

                // PCRE_UTF8 needed to run compile in UTF-8 mode
                // PCRE_UCP needed for \d, \w etc. to use Unicode properties:
                QSharedPointer<pcre> const re(pcre_compile(regexp.constData(), PCRE_UTF8 | PCRE_UCP, &error, &erroffset, nullptr), pcre_deleter);

                if (!re) {
                    if (mudlet::smDebugMode) {
                        TDebug(Qt::white, Qt::red) << "REGEX ERROR: failed to compile, reason:\n" << error << "\n" >> mpHost;
                        TDebug(Qt::red, Qt::gray) << TDebug::csmContinue << R"(in: ")" << regexp.constData() << "\"\n" >> mpHost;
                    }
                    setError(qsl("<b><font color='blue'>%1</font></b>")
                             .arg(tr(R"(Error: in item %1, perl regex "%2" failed to compile, reason: "%3".)")
                             .arg(QString::number(i + 1), QString(regexp.constData()).toHtmlEscaped(), QString(error).toHtmlEscaped())));
                    state = false;
                } else {
                    if (mudlet::smDebugMode) {
                        TDebug(Qt::white, Qt::darkGreen) << "[OK]: REGEX_COMPILE OK\n" >> mpHost;
                    }
                }
                mRegexMap[i] = re;
                mTriggerContainsPerlRegex = true;
            }

            if (patternKinds.at(i) == REGEX_LUA_CODE) {
                std::string funcName;
                std::stringstream func;
                func << "trigger" << mID << "condition" << i;
                funcName = func.str();
                const QString code = qsl("function %1() %2\nend").arg(funcName.c_str(), patterns[i]);
                QString error;
                if (!mpLua->compile(code, error, QString::fromStdString(funcName))) {
                    setError(qsl("<b><font color='blue'>%1</font></b>")
                             .arg(tr(R"(Error: in item %1, lua function "%2" failed to compile, reason: "%3".)")
                             .arg(QString::number(i + 1), patterns.at(i).toHtmlEscaped(), QString(error))));
                    state = false;
                    if (mudlet::smDebugMode) {
                        TDebug(Qt::white, Qt::red) << "LUA ERROR: failed to compile, reason:\n" << error << "\n" >> mpHost;
                        TDebug(Qt::red, Qt::gray) << TDebug::csmContinue << R"(in lua condition function: ")" << patterns.at(i) << "\"\n" >> mpHost;
                    }
                } else {
                    mLuaConditionMap[i] = funcName;
                }
            }

            if (patternKinds[i] == REGEX_COLOR_PATTERN) {
                int textAnsiFg = scmIgnored;
                int textAnsiBg = scmIgnored;
                // Decode the pattern string to the colour codes wanted:
                TTrigger::decodeColorPatternText(patterns.at(i), textAnsiFg, textAnsiBg);

                if (textAnsiBg == scmIgnored && textAnsiFg == scmIgnored) {
                    setError(qsl("<b><font color='blue'>%1</font></b>")
                                     .arg(tr("Error: in item %1, no colors to match were set - at least <i>one</i> of the foreground or background must not be <i>ignored</i>.")
                                          .arg(QString::number(i+1))));
                    state = false;
                    continue;
                }

                // The setupColorTrigger(...) method will push_back the created
                // TColorTable instance if it is successful:
                if (!setupColorTrigger(textAnsiFg, textAnsiBg)) {
                    mColorPatternList.push_back(nullptr);
                    state = false;
                    continue;
                }
            } else {
                mColorPatternList.push_back(nullptr);
            }
        }
    }

    mOK_init = state;
    return state;
}

bool TTrigger::match_perl(char* haystackC, const QString& haystack, int patternNumber, int posOffset)
{
    assert(mRegexMap.contains(patternNumber));

    QSharedPointer<pcre> const re = mRegexMap[patternNumber];

    if (!re) {
        if (mudlet::smDebugMode) {
            TDebug(Qt::white, Qt::red) << "ERROR:" >> mpHost;
            TDebug(Qt::darkRed, Qt::darkGray) << TDebug::csmContinue << " the regex of trigger " << mName
                                              << " does not compile. Please correct the expression. This trigger will never match until it is fixed.\n"
                    >> mpHost;
        }
        return false; //regex compile error
    }

    const int haystackCLength = strlen(haystackC);
    int rc = -1;
    int ovector[MAX_CAPTURE_GROUPS * 3];

    rc = pcre_exec(re.data(), nullptr, haystackC, haystackCLength, 0, 0, ovector, MAX_CAPTURE_GROUPS * 3);

    if (rc < 0) {
        return false;
    }

    processRegexMatch(haystackC, haystack, patternNumber, posOffset, re, haystackCLength, rc, ovector);

    return true;
}

void TTrigger::processRegexMatch(const char* haystackC, const QString& haystack, int patternNumber, int posOffset,
                                 const QSharedPointer<pcre>& re, int haystackCLength, int rc, int* ovector)
{
    if (rc == 0) {
        if (mpHost->mpEditorDialog) {
            mpHost->mpEditorDialog->mpErrorConsole->print(
                qsl("%1\n").arg(tr("[Trigger Error:] %1 capture group limit exceeded, capture less groups.").arg(MAX_CAPTURE_GROUPS)),
                QColor(255, 128, 0),
                QColor(Qt::black));
        }
        qWarning() << "CRITICAL ERROR: SHOULD NOT HAPPEN pcre_info() got wrong number of capture groups ovector only has room for" << MAX_CAPTURE_GROUPS << "captured substrings";
    }

    if (mudlet::smDebugMode) {
        TDebug(Qt::blue, Qt::black) << "Trigger name=" << mName << "(" << mPatterns.value(patternNumber) << ") matched.\n" >> mpHost;
    }

    int i = 0;
    int numberOfCaptureGroups = 0;
    std::list<std::string> captureList;
    std::list<int> posList;
    QMap<QString, QPair<int, int>> namePositions;
    NameGroupMatches nameGroups;
    for (i = 0; i < rc; i++) {
        const char *substring_start = haystackC + ovector[2 * i];
        const int substring_length = ovector[2 * i + 1] - ovector[2 * i];
        const int utf16_pos = haystack.indexOf(QString::fromUtf8(substring_start, substring_length));
        std::string match;
        if (substring_length < 1) {
            captureList.push_back(match);
            posList.push_back(-1);
            continue;
        }

        match.append(substring_start, substring_length);
        captureList.push_back(match);
        posList.push_back(utf16_pos + posOffset);
        if (mudlet::smDebugMode) {
            TDebug(Qt::darkCyan, Qt::black) << "capture group #" << (i + 1) << " = " >> mpHost;
            TDebug(Qt::darkMagenta, Qt::black) << TDebug::csmContinue << "<" << match.c_str() << ">\n" >> mpHost;
        }
    }

    int namecount = 0;
    int name_entry_size = 0;
    char* tabptr = nullptr;

    pcre_fullinfo(re.data(), nullptr, PCRE_INFO_NAMECOUNT, &namecount);

    if (namecount > 0) {
        // Based on snippet https://github.com/vmg/pcre/blob/master/pcredemo.c#L216
        // Retrieves char table end entry size and extracts name of group and captures from
        pcre_fullinfo(re.data(), nullptr, PCRE_INFO_NAMETABLE, &tabptr);
        pcre_fullinfo(re.data(), nullptr, PCRE_INFO_NAMEENTRYSIZE, &name_entry_size);
        for (i = 0; i < namecount; ++i) {
            const int n = (tabptr[0] << 8) | tabptr[1];
            auto name = QString::fromUtf8(&tabptr[2]).trimmed(); //NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic, cppcoreguidelines-pro-bounds-constant-array-index)
            auto* substring_start = haystackC + ovector[2*n]; //NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic, cppcoreguidelines-pro-bounds-constant-array-index)
            auto substring_length = ovector[2*n+1] - ovector[2*n]; //NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
            auto utf16_pos = haystack.indexOf(QString::fromUtf8(substring_start, substring_length));
            auto capture = QString::fromUtf8(substring_start, substring_length);
            nameGroups << qMakePair(name, capture);
            tabptr += name_entry_size;
            namePositions.insert(name, qMakePair(utf16_pos + posOffset, substring_length));
        }
    }
    if (mIsColorizerTrigger || mFilterTrigger) {
        numberOfCaptureGroups = captureList.size();
    }
    for (; mPerlSlashGOption;) {
        int options = 0;
        const int start_offset = ovector[1];

        if (ovector[0] == ovector[1]) {
            if (ovector[0] >= haystackCLength) {
                goto END;
            }
            options = PCRE_NOTEMPTY | PCRE_ANCHORED;
        }

        rc = pcre_exec(re.data(), nullptr, haystackC, haystackCLength, start_offset, options, ovector, MAX_CAPTURE_GROUPS * 3);

        if (rc == PCRE_ERROR_NOMATCH) {
            if (options == 0) {
                break;
            }
            ovector[1] = start_offset + 1;
            continue;
        } else if (rc < 0) {
            goto END;
        } else if (rc == 0) {
            if (mpHost->mpEditorDialog) {
                mpHost->mpEditorDialog->mpErrorConsole->print(tr("[Trigger Error:] %1 capture group limit exceeded, capture less groups.\n").arg(MAX_CAPTURE_GROUPS), QColor(255, 128, 0), QColor(Qt::black));
            }
            qWarning() << "CRITICAL ERROR: SHOULD NOT HAPPEN pcre_info() got wrong number of capture groups ovector only has room for" << MAX_CAPTURE_GROUPS << "captured substrings";
        }

        for (i = 0; i < rc; i++) {
            const char *substring_start = haystackC + ovector[2 * i];
            const int substring_length = ovector[2 * i + 1] - ovector[2 * i];
            const int utf16_pos = haystack.indexOf(QString::fromUtf8(substring_start, substring_length));

            std::string match;
            if (substring_length < 1) {
                captureList.push_back(match);
                posList.push_back(-1);
                continue;
            }
            match.append(substring_start, substring_length);
            captureList.push_back(match);
            posList.push_back(utf16_pos + posOffset);
            if (mudlet::smDebugMode) {
                TDebug(Qt::darkCyan, Qt::black) << "<regex mode: match all> capture group #" << (i + 1) << " = " >> mpHost;
                TDebug(Qt::darkMagenta, Qt::black) << "<" << match.c_str() << ">\n" >> mpHost;
            }
        }
    }

    END : {
        if (mIsColorizerTrigger) {
            const int r1 = mBgColor.red();
            const int g1 = mBgColor.green();
            const int b1 = mBgColor.blue();
            const int r2 = mFgColor.red();
            const int g2 = mFgColor.green();
            const int b2 = mFgColor.blue();
            const int total = captureList.size();
            TConsole* pC = mpHost->mpConsole;
            if (Q_UNLIKELY(!pC)) {
                return;
            }
            pC->deselect();
            auto its = captureList.begin();
            auto iti = posList.begin();
            for (int position = 1; iti != posList.end(); ++iti, ++its, position++) {
                const int begin = *iti;
                const std::string& s = *its;
                const int length = QString::fromStdString(s).size();
                if (total > 1) {
                    // skip complete match in Perl /g option type of triggers
                    // to enable people to highlight capture groups if there are any
                    // otherwise highlight complete expression match
                    if (position % numberOfCaptureGroups != 1) {
                        pC->selectSection(begin, length);
                        if (mBgColor != QColorConstants::Transparent) {
                            pC->setBgColor(r1, g1, b1, 255);
                        }
                        if (mFgColor != QColorConstants::Transparent) {
                            pC->setFgColor(r2, g2, b2);
                        }
                    }
                } else {
                    pC->selectSection(begin, length);
                    pC->setBgColor(r1, g1, b1, 255);
                    pC->setFgColor(r2, g2, b2);
                }
            }
            pC->reset();
        }
        if (mIsMultiline) {
            updateMultistates(patternNumber, captureList, posList, &nameGroups);
            return;
        } else {
            TLuaInterpreter* pL = mpHost->getLuaInterpreter();
            pL->setCaptureGroups(captureList, posList);
            pL->setCaptureNameGroups(nameGroups, namePositions);
            execute();
            pL->clearCaptureGroups();
            if (mFilterTrigger) {
                if (captureList.size() > 1) {
                    const int total = captureList.size();
                    auto its = captureList.begin();
                    auto iti = posList.begin();
                    for (int filterPosition = 1; iti != posList.end(); ++iti, ++its, filterPosition++) {
                        int begin = *iti;
                        std::string& s = *its;
                        if (total > 1 && numberOfCaptureGroups > 0) {
                            // skip complete match in Perl /g option type of triggers
                            // to enable people to highlight capture groups if there are any
                            // otherwise highlight complete expression match
                            if (filterPosition % numberOfCaptureGroups != 1) {
                                filter(s, begin);
                            }
                        } else {
                            filter(s, begin);
                        }
                    }
                }
            }
            return;
        }
    }
}

bool TTrigger::match_begin_of_line_substring(const QString& haystack, const QString& needle, int patternNumber, int posOffset)
{
    if (haystack.startsWith(needle)) {
        processBeginOfLine(needle, patternNumber, posOffset);
        return true;
    }
    return false;
}

void TTrigger::processBeginOfLine(const QString& needle, int patternNumber, int posOffset)
{
    std::list<std::string> captureList;
    std::list<int> posList;
    captureList.emplace_back(needle.toUtf8().constData());
    posList.push_back(0 + posOffset);
    if (mudlet::smDebugMode) {
        TDebug(Qt::darkCyan, Qt::black) << "Trigger name=" << mName << "(" << mPatterns.value(patternNumber) << ") matched.\n" >> mpHost;
    }
    if (mIsColorizerTrigger) {
        const int r1 = mBgColor.red();
        const int g1 = mBgColor.green();
        const int b1 = mBgColor.blue();
        const int r2 = mFgColor.red();
        const int g2 = mFgColor.green();
        const int b2 = mFgColor.blue();
        TConsole* pC = mpHost->mpConsole;
        if (Q_UNLIKELY(!pC)) {
            return;
        }
        auto its = captureList.begin();
        for (auto iti = posList.begin(); iti != posList.end(); ++iti, ++its) {
            const int begin = *iti;
            const std::string& s = *its;
            const int length = QString::fromStdString(s).size();
            pC->selectSection(begin, length);
            if (mBgColor != QColorConstants::Transparent) {
                pC->setBgColor(r1, g1, b1, 255);
            }
            if (mFgColor != QColorConstants::Transparent) {
                pC->setFgColor(r2, g2, b2);
            }
        }
        pC->reset();
    }
    if (mIsMultiline) {
        updateMultistates(patternNumber, captureList, posList);
        return;
    } else {
        TLuaInterpreter* pL = mpHost->getLuaInterpreter();
        pL->setCaptureGroups(captureList, posList);

        // call lua trigger function with number of matches and matches itselves as arguments
        execute();
        pL->clearCaptureGroups();
        if (mFilterTrigger) {
            if (!captureList.empty()) {
                filter(captureList.front(), posList.front());
            }
        }
    }
}

inline void TTrigger::updateMultistates(int regexNumber, std::list<std::string>& captureList, std::list<int>& posList, const NameGroupMatches* nameMatches)
{
    if (regexNumber == 0) {
        // automatically set to #1
        auto pCondition = new TMatchState(mPatterns.size(), mConditionLineDelta);
        mConditionMap[pCondition] = pCondition;
        pCondition->multiCaptureList.push_back(captureList);
        pCondition->multiCapturePosList.push_back(posList);
        if (nameMatches) {
            pCondition->nameCaptures.push_back(*nameMatches);
        } else {
            pCondition->nameCaptures.push_back(QVector<QPair<QString, QString>>());
        }
        if (mudlet::smDebugMode) {
            TDebug(Qt::darkYellow, Qt::black) << "match state " << mConditionMap.size() << "/" << mConditionMap.size() << " condition #" << regexNumber << "=true (" << regexNumber
                                              << "/" << mPatterns.size() << ") regex=" << mPatterns[regexNumber] << "\n"
                    >> mpHost;
        }
    } else {
        int k = 0;
        for (auto& matchStatePair : mConditionMap) {
            k++;
            if (matchStatePair.second->nextCondition() == regexNumber) {
                if (mudlet::smDebugMode) {
                    TDebug(Qt::darkYellow, Qt::black) << "match state " << k << "/" << mConditionMap.size() << " condition #" << regexNumber << "=true (" << regexNumber << "/"
                                                      << mPatterns.size() << ") regex=" << mPatterns[regexNumber] << "\n"
                            >> mpHost;
                }
                matchStatePair.second->conditionMatched();
                matchStatePair.second->multiCaptureList.push_back(captureList);
                matchStatePair.second->multiCapturePosList.push_back(posList);
                if (nameMatches != nullptr) {
                    matchStatePair.second->nameCaptures.push_back(*nameMatches);
                }
            }
        }
    }
}

inline void TTrigger::filter(std::string& capture, int& posOffset)
{
    if (capture.empty()) {
        return;
    }
    auto * filterSubject = static_cast<char*>(malloc(capture.size() + 2048));
    if (filterSubject) {
        strcpy(filterSubject, capture.c_str());
    } else {
        return;
    }
    const QString text = capture.c_str();
    for (auto& trigger : *mpMyChildrenList) {
        trigger->match(filterSubject, text, -1, posOffset);
    }
    free(filterSubject);
}

int TTrigger::getExpiryCount() const
{
    return mExpiryCount;
}

void TTrigger::setExpiryCount(int expiryCount)
{
    mExpiryCount = expiryCount;
}

bool TTrigger::match_substring(const QString& haystack, const QString& needle, int patternNumber, int posOffset)
{
    const int where = haystack.indexOf(needle);
    if (where != -1) {
        processSubstringMatch(haystack, needle, patternNumber, posOffset, where);
        return true;
    }
    return false;
}

void TTrigger::processSubstringMatch(const QString& haystack, const QString& needle, int regexNumber, int posOffset,
                                     int where)
{
    std::list<std::string> captureList;
    std::list<int> posList;
    captureList.emplace_back(needle.toUtf8().constData());
    posList.push_back(where + posOffset);
    if (mPerlSlashGOption) {
        while ((where = haystack.indexOf(needle, where + 1)) != -1) {
            captureList.emplace_back(needle.toUtf8().constData());
            posList.push_back(where + posOffset);
        }
    }
    if (mudlet::smDebugMode) {
        TDebug(Qt::cyan, Qt::black) << "Trigger name=" << mName << "(" << mPatterns.value(regexNumber) << ") matched.\n" >> mpHost;
    }
    if (mIsColorizerTrigger) {
        const int r1 = mBgColor.red();
        const int g1 = mBgColor.green();
        const int b1 = mBgColor.blue();
        const int r2 = mFgColor.red();
        const int g2 = mFgColor.green();
        const int b2 = mFgColor.blue();
        TConsole* pC = mpHost->mpConsole;
        if (Q_UNLIKELY(!pC)) {
            return;
        }
        pC->deselect();
        auto its = captureList.begin();
        for (auto iti = posList.begin(); iti != posList.end(); ++iti, ++its) {
            const int begin = *iti;
            const std::string& s = *its;
            const int length = QString::fromStdString(s).size();
            pC->selectSection(begin, length);
            if (mBgColor != QColorConstants::Transparent) {
                pC->setBgColor(r1, g1, b1, 255);
            }
            if (mFgColor != QColorConstants::Transparent) {
                pC->setFgColor(r2, g2, b2);
            }
        }
        pC->reset();
    }
    if (mIsMultiline) {
        updateMultistates(regexNumber, captureList, posList);
        return;
    } else {
        TLuaInterpreter* pL = mpHost->getLuaInterpreter();
        pL->setCaptureGroups(captureList, posList);

        // call lua trigger function with number of matches and matches itselves as arguments
        execute();
        pL->clearCaptureGroups();
        if (mFilterTrigger) {
            if (!captureList.empty()) {
                filter(captureList.front(), posList.front());
            }
        }
    }
}

bool TTrigger::match_color_pattern(int line, int patternNumber)
{
    if (patternNumber >= mColorPatternList.size()) {
        return false;
    }
    if (line == -1) {
        return false;
    }
    bool canExecute = false;
    std::list<std::string> captureList;
    std::list<int> posList;
    if (line >= static_cast<int>(mpHost->mpConsole->buffer.buffer.size())) {
        return false;
    }
    std::deque<TChar>& bufferLine = mpHost->mpConsole->buffer.buffer[line];
    const QString& lineBuffer = mpHost->mpConsole->buffer.lineBuffer[line];
    int pos = 0;
    int matchBegin = -1;
    bool matching = false;

    TColorTable* pCT = mColorPatternList[patternNumber];
    if (!pCT) {
        return false; // no color pattern created
    }

    if (pCT->ansiBg == scmIgnored && pCT->ansiFg == scmIgnored) {
        // BOTH the foreground AND the background colors are set to "ignore"
        // so this is not an active color trigger setup - so ignore it
        return false; // no color settings to match against
    }

    for (auto it = bufferLine.begin(); it != bufferLine.end(); ++it, ++pos) {
        // This now allows matching against the current default colours (-1) and
        // allows ONE of the foreground or background to NOT be considered (-2)
        // Ideally we should base the matching on only the ANSI code but not
        // all parts of the text come from the Server and can be determined to
        // have come from a decoded ANSI code number:
        if (((pCT->ansiFg == scmIgnored) || ((pCT->ansiFg == scmDefault) && mpHost->mpConsole->mFgColor == (*it).foreground()) || (pCT->mFgColor == (*it).foreground()))
            && ((pCT->ansiBg == scmIgnored) || ((pCT->ansiBg == scmDefault) && mpHost->mpConsole->mBgColor == (*it).background()) || (pCT->mBgColor == (*it).background()))) {

            if (matchBegin == -1) {
                matchBegin = pos;
            }
            matching = true;
        } else {
            matching = false;
        }

        if ((!matching) || (matching && (pos + 1 >= static_cast<int>(bufferLine.size())))) {
            if (matchBegin > -1) {
                std::string got;
                if (matching) {
                    got = lineBuffer.mid(matchBegin, pos - matchBegin + 1).toUtf8().constData();
                } else {
                    got = lineBuffer.mid(matchBegin, pos - matchBegin).toUtf8().constData();
                }
                captureList.push_back(got);
                posList.push_back(matchBegin);
                matchBegin = -1;
                canExecute = true;
                matching = false;
            }
        }
    }

    if (canExecute) {
        processColorPattern(patternNumber, captureList, posList);
        return true;
    }
    return false;
}

void TTrigger::processColorPattern(int patternNumber, std::list<std::string>& captureList, std::list<int>& posList)
{
    if (mIsColorizerTrigger) {
        const int r1 = mBgColor.red();
        const int g1 = mBgColor.green();
        const int b1 = mBgColor.blue();
        const int r2 = mFgColor.red();
        const int g2 = mFgColor.green();
        const int b2 = mFgColor.blue();
        TConsole* pC = mpHost->mpConsole;
        if (Q_UNLIKELY(!pC)) {
            return;
        }
        pC->deselect();
        auto its = captureList.begin();
        for (auto iti = posList.begin(); iti != posList.end(); ++iti, ++its) {
            const int begin = *iti;
            //                qDebug() << "TTrigger::match_color_pattern(" << line << "," << patternNumber << ") INFO - match found: " << (*its).c_str() << " size is:" << (*its).size();
            const std::string& s = *its;
            const int length = QString::fromStdString(s).size();
            pC->selectSection(begin, length);
            if (mBgColor != QColorConstants::Transparent) {
                pC->setBgColor(r1, g1, b1, 255);
            }
            if (mFgColor != QColorConstants::Transparent) {
                pC->setFgColor(r2, g2, b2);
            }
        }
        pC->reset();
    }
    if (mIsMultiline) {
        updateMultistates(patternNumber, captureList, posList);
        return;
    } else {
        TLuaInterpreter* pL = mpHost->getLuaInterpreter();
        pL->setCaptureGroups(captureList, posList);
        // call lua trigger function with number of matches and matches itselves as arguments
        execute();
        pL->clearCaptureGroups();
        if (mFilterTrigger) {
            if (!captureList.empty()) {
                auto it1 = captureList.begin();
                auto it2 = posList.begin();
                for (; it1 != captureList.end(); it1++, it2++) {
                    filter(*it1, *it2);
                }
            }
        }
    }
}

bool TTrigger::match_line_spacer(int patternNumber)
{
    if (mIsMultiline) {
        int k = 0;

        for (auto& matchStatePair : mConditionMap) {
            k++;
            if (matchStatePair.second->nextCondition() == patternNumber) {
                if (matchStatePair.second->lineSpacerMatch(mPatterns.value(patternNumber).toInt())) {
                    if (mudlet::smDebugMode) {
                        TDebug(Qt::yellow, Qt::black) << "Trigger name=" << mName << "(" << mPatterns.value(patternNumber) << ") condition #" << patternNumber << "=true " >> mpHost;
                        TDebug(Qt::darkYellow, Qt::black) << TDebug::csmContinue << "match state " << k << "/" << mConditionMap.size() << " condition #" << patternNumber << "=true (" << patternNumber + 1 << "/"
                                                          << mPatterns.size() << ") line spacer=" << mPatterns.value(patternNumber) << "lines\n"
                                                          >> mpHost;
                    }
                    matchStatePair.second->conditionMatched();
                    std::list<std::string> const captureList;
                    std::list<int> const posList;
                    matchStatePair.second->multiCaptureList.push_back(captureList);
                    matchStatePair.second->multiCapturePosList.push_back(posList);
                }
            }
        }
    }

    return true; //line spacers don't make sense outside of AND triggers -> ignore them
}

bool TTrigger::match_lua_code(int patternNumber)
{
    if (mLuaConditionMap.find(patternNumber) == mLuaConditionMap.end()) {
        return false;
    }

    if (mpLua->callConditionFunction(mLuaConditionMap[patternNumber], mName)) {
        if (mudlet::smDebugMode) {
            TDebug(Qt::yellow, Qt::black) << "Trigger name=" << mName << "(" << mPatterns.value(patternNumber) << ") matched.\n" >> mpHost;
        }
        if (mIsMultiline) {
            std::list<std::string> captureList;
            std::list<int> posList;
            updateMultistates(patternNumber, captureList, posList);
            return true;
        }
        execute();
        return true;
    }
    return false;
}

bool TTrigger::match_prompt(int patternNumber)
{
    if (mpHost->mpConsole->mIsPromptLine) {
        processPromptMatch(patternNumber);
        return true;
    }
    return false;
}

void TTrigger::processPromptMatch(int patternNumber)
{
    if (mudlet::smDebugMode) {
        TDebug(Qt::yellow, Qt::black) << "Trigger name=" << mName << "(" << mPatterns.value(patternNumber) << ") matched.\n" >> mpHost;
    }
    if (mIsMultiline) {
        std::list<std::string> captureList;
        std::list<int> posList;
        updateMultistates(patternNumber, captureList, posList);
        return;
    }
    execute();
}

bool TTrigger::match_exact_match(const QString& haystack, const QString& needle, int patternNumber, int posOffset)
{
    QString text = haystack;
    if (text.endsWith(QChar('\n'))) {
        text.chop(1);
    }

    if (text == needle) {
        processExactMatch(needle, patternNumber, posOffset);
        return true;
    }
    return false;
}

void TTrigger::processExactMatch(const QString& line, int patternNumber, int posOffset)
{
    std::list<std::string> captureList;
    std::list<int> posList;
    captureList.emplace_back(line.toUtf8().constData());
    posList.push_back(0 + posOffset);
    if (mudlet::smDebugMode) {
        TDebug(Qt::yellow, Qt::black) << "Trigger name=" << mName << "(" << mPatterns.value(patternNumber) << ") matched.\n" >> mpHost;
    }
    if (mIsColorizerTrigger) {
        const int r1 = mBgColor.red();
        const int g1 = mBgColor.green();
        const int b1 = mBgColor.blue();
        const int r2 = mFgColor.red();
        const int g2 = mFgColor.green();
        const int b2 = mFgColor.blue();
        TConsole* pC = mpHost->mpConsole;
        if (Q_UNLIKELY(!pC)) {
            return;
        }
        auto its = captureList.begin();
        for (auto iti = posList.begin(); iti != posList.end(); ++iti, ++its) {
            const int begin = *iti;
            const std::string& s = *its;
            const int length = QString::fromStdString(s).size();
            pC->selectSection(begin, length);
            if (mBgColor != QColorConstants::Transparent) {
                pC->setBgColor(r1, g1, b1, 255);
            }
            if (mFgColor != QColorConstants::Transparent) {
                pC->setFgColor(r2, g2, b2);
            }
        }
        pC->reset();
    }
    if (mIsMultiline) {
        updateMultistates(patternNumber, captureList, posList);
        return;
    } else {
        TLuaInterpreter* pL = mpHost->getLuaInterpreter();
        pL->setCaptureGroups(captureList, posList);
        // call lua trigger function with number of matches and matches themselves as arguments
        execute();
        pL->clearCaptureGroups();
        if (mFilterTrigger) {
            if (!captureList.empty()) {
                filter(captureList.front(), posList.front());
            }
        }
    }
}

// haystackC: string to match as a char*
// haystack: string to match as a QString
// line: line number in the buffer
// posOffset: position in the line to start matching from; used by child triggers
bool TTrigger::match(char* haystackC, const QString& haystack, int line, int posOffset)
{
    bool ret = false;
    if (isActive()) {
        if (mIsLineTrigger) {
            if (--mStartOfLineDelta < 0) {
                execute();
                if (--mLineDelta <= 0) {
                    deactivate();
                    mpHost->getTriggerUnit()->markCleanup(this);
                }
                return true;
            }
            return false;
        }

        if (haystack.isEmpty()) {
            return false;
        }

        bool conditionMet = false;

        int highestCondition = 0;
        if (mIsMultiline) {
            for (auto& matchStatePair : mConditionMap) {
                matchStatePair.second->newLineArrived();
                const int next = matchStatePair.second->nextCondition();
                if (next > highestCondition) {
                    highestCondition = next;
                }
            }
        }

        const int size = mPatternKinds.size();
        for (int patternNumber = 0;; patternNumber++) {
            if (patternNumber >= size) {
                break;
            }
            ret = false;
            switch (mPatternKinds.value(patternNumber)) {
            case REGEX_SUBSTRING:
                ret = match_substring(haystack, mPatterns.at(patternNumber), patternNumber, posOffset);
                break;

            case REGEX_PERL:
                ret = match_perl(haystackC, haystack, patternNumber, posOffset);
                break;

            case REGEX_BEGIN_OF_LINE_SUBSTRING:
                ret = match_begin_of_line_substring(haystack, mPatterns.at(patternNumber), patternNumber, posOffset);
                break;

            case REGEX_EXACT_MATCH:
                ret = match_exact_match(haystack, mPatterns.at(patternNumber), patternNumber, posOffset);
                break;

            case REGEX_LUA_CODE:
                ret = match_lua_code(patternNumber);
                break;

            case REGEX_LINE_SPACER:
                ret = match_line_spacer(patternNumber);
                break;

            case REGEX_COLOR_PATTERN:
                ret = match_color_pattern(line, patternNumber);
                break;

            case REGEX_PROMPT:
                ret = match_prompt(patternNumber);
                break;
            }
            // policy: one match is enough to fire on OR-trigger, but in the case of
            //         an AND-trigger all conditions have to be met in order to fire the trigger
            if (!mIsMultiline) {
                if (ret) {
                    conditionMet = true;
                    mKeepFiring = mStayOpen;
                    break;
                }
            } else {
                if ((!ret) && (patternNumber >= highestCondition)) {
                    break;
                }
            }
        }

        // in the case of multiline triggers: check our state
        if (mIsMultiline) {
            int k = 0;
            conditionMet = false; //invalidate conditionMet as it has no meaning for multiline triggers
            std::list<TMatchState*> removeList;

            for (auto& matchStatePair : mConditionMap) {
                k++;
                if (matchStatePair.second->isComplete()) {
                    mKeepFiring = mStayOpen;
                    if (mudlet::smDebugMode) {
                        TDebug(Qt::yellow, Qt::darkMagenta) << "multiline trigger name=" << mName << " *FIRES* all conditions are fulfilled. Executing script.\n" >> mpHost;
                    }
                    removeList.push_back(matchStatePair.first);
                    conditionMet = true;
                    TLuaInterpreter* pL = mpHost->getLuaInterpreter();
                    pL->setMultiCaptureGroups(matchStatePair.second->multiCaptureList, matchStatePair.second->multiCapturePosList, matchStatePair.second->nameCaptures);
                    execute();
                    pL->clearCaptureGroups();
                    if (mFilterTrigger) {
                        std::list<std::list<std::string>> multiCaptureList;
                        multiCaptureList = matchStatePair.second->multiCaptureList;
                        if (!multiCaptureList.empty()) {
                            for (auto mit = multiCaptureList.begin(); mit != multiCaptureList.end(); mit++, k++) {
                                const int total = (*mit).size();
                                auto its = (*mit).begin();
                                for (int i = 1; its != (*mit).end(); ++its, i++) {
                                    std::string s = *its;
                                    int p = 0;
                                    if (total > 1) {
                                        if (i % total != 1) {
                                            filter(s, p);
                                        }
                                    } else {
                                        filter(s, p);
                                    }
                                }
                            }
                        }
                    }
                }

                if (!matchStatePair.second->newLine()) {
                    removeList.push_back(matchStatePair.first);
                }
            }
            for (auto& matchState : removeList) {
                if (mConditionMap.find(matchState) != mConditionMap.end()) {
                    delete mConditionMap[matchState];
                    if (mudlet::smDebugMode) {
                        TDebug(Qt::darkBlue, Qt::black) << "removing condition from condition table.\n" >> mpHost;
                    }
                    mConditionMap.erase(matchState);
                }
            }
        }


        // definition trigger chain: a folder is part of a trigger chain if it has a regex defined
        // a trigger chain only lets data pass if the condition matches or in case of multiline all
        // all conditions are fulfilled
        //
        // a folder can also be a simple structural element in which case all data passes through
        // if at least one regex is defined a folder is considered a trigger chain otherwise a structural element
        if (!mFilterTrigger) {
            if (conditionMet || (mPatterns.empty())) {
                for (auto trigger : *mpMyChildrenList) {
                    ret = trigger->match(haystackC, haystack, line, posOffset);
                    if (ret) {
                        conditionMet = true;
                    }
                }
            }
        }

        if ((mKeepFiring > 0) && (!conditionMet)) {
            mKeepFiring--;
            if ((mKeepFiring == mStayOpen) || (mpMyChildrenList->empty())) {
                execute();
            }
            for (auto trigger : *mpMyChildrenList) {
                ret = trigger->match(haystackC, haystack, line, posOffset);
                if (ret) {
                    conditionMet = true;
                }
            }
            return true;
        }

        if (conditionMet && mExpiryCount > -1) {
            mExpiryCount--;

            if (mExpiryCount == 0) {
                mpHost->getTriggerUnit()->markCleanup(this);

                if (mudlet::smDebugMode) {
                    // FIXME: This message is translated - but most other TDebug ones are not!
                    TDebug(Qt::yellow, Qt::darkMagenta) << qsl("%1\n").arg(tr("Trigger name=%1 expired.").arg(mName)) >> mpHost;
                }

            } else if (mudlet::smDebugMode) {
                // FIXME: This message is translated - but most other TDebug ones are not!
                TDebug(Qt::yellow, Qt::darkMagenta) << qsl("%1\n").arg(tr("Trigger name=%1 will fire %n more time(s).", "", mExpiryCount).arg(mName)) >> mpHost;
            }
        }

        return conditionMet;
    }
    return false;
}

bool TTrigger::checkIfNew()
{
    return mIsNew;
}

void TTrigger::unmarkAsNew()
{
    mIsNew = false;
}


// This NOW uses proper ANSI numbers
// A TColorTable is a simple struct that stores four values, the two given ANSI
// colors for foreground and background (proper ANSI indexes) and what they look
// like give the current Host settings (the first 16 ANSI ones and the default
// fore and background colors can be changed by the user and since OSC P/R
// support has been implemented - by the MUD Server!)
TColorTable* TTrigger::createColorPattern(int ansiFg, int ansiBg)
{
    /*
     *  OLD Mudlet simplified ANSI color codes
     *                       -> proper ANSI numbers
     * ---------------------------------------------
     *  0  default text color -> (-1 special value!)
     *  1  light black        ->  8
     *  2  dark black         ->  0
     *  3  light red          ->  9
     *  4  dark red           ->  1
     *  5  light green        -> 10
     *  6  dark green         ->  2
     *  7  light yellow       -> 11
     *  8  dark yellow        ->  3
     *  9  light blue         -> 12
     * 10 dark blue           ->  4
     * 11 light magenta       -> 13
     * 12 dark magenta        ->  5
     * 13 light cyan          -> 14
     * 14 dark cyan           ->  6
     * 15 light white         -> 15
     * 16 dark white          ->  7
     */

    const QColor fgColor = mpHost->getAnsiColor(ansiFg, false);
    const QColor bgColor = mpHost->getAnsiColor(ansiBg, true);

    // If BOTH ansiFg AND ansiBg are scmIgnored then the color pattern is
    // totally unset
    if (!(fgColor.isValid() || bgColor.isValid())) {
        return nullptr;
    }

    auto pCT = new (std::nothrow) TColorTable;
    if (!pCT) {
        return nullptr;
    }

    pCT->ansiBg = ansiBg;
    pCT->ansiFg = ansiFg;
    pCT->mBgColor = bgColor;
    pCT->mFgColor = fgColor;
    return pCT;
}

bool TTrigger::setupColorTrigger(int ansiFg, int ansiBg)
{
    TColorTable* pCT = createColorPattern(ansiFg, ansiBg);
    if (!pCT) {
        // This can be caused by both ansiFg and ansiBg being scmIgnored
        return false;
    }
//    qDebug() << "TTrigger::setupColorTrigger(" << ansiFg
//             << ", "
//             << ansiBg
//             << ") INFO: adding TColorTable to mColorPatternList: (ansiFg:"
//             << pCT->ansiFg
//             << pCT->mFgColor
//             << "ansiBg:"
//             << pCT->ansiBg
//             << pCT->mBgColor
//             << ").";
    mColorPatternList.push_back(pCT);
    return true;
}

// The numbers here are NOW the proper ANSI colour codes (0 to 255)
// or scmDefault for default fore/background or scmIgnored for ignored
bool TTrigger::setupTmpColorTrigger(int ansiFg, int ansiBg)
{
    TColorTable* pCT = createColorPattern(ansiFg, ansiBg);
    if (!pCT) {
        return false;
    }

    // createColorPatternText(...) now returns an empty string if BOTH color
    // codes are the scmIgnored ones:
    mPatterns << createColorPatternText(ansiFg, ansiBg);
    mPatternKinds << REGEX_COLOR_PATTERN;
    mColorPatternList.push_back(pCT);
    return true;
}

bool TTrigger::isFilterChain()
{
    return (!mPatterns.empty()) && (hasChildren());
}

bool TTrigger::registerTrigger()
{
    if (!mpHost) {
        return false;
    }
    return mpHost->getTriggerUnit()->registerTrigger(this);
}

void TTrigger::compileAll()
{
    mNeedsToBeCompiled = true;
    if (!compileScript()) {
        if (mudlet::smDebugMode) {
            TDebug(Qt::white, Qt::red) << "ERROR: Lua compile error. compiling script of Trigger:" << mName << "\n" >> mpHost;
        }
        mOK_code = false;
    }
    setRegexCodeList(mPatterns, mPatternKinds);
    for (auto trigger : *mpMyChildrenList) {
        trigger->compileAll();
    }
}


void TTrigger::compile()
{
    if (mNeedsToBeCompiled) {
        if (!compileScript()) {
            if (mudlet::smDebugMode) {
                TDebug(Qt::white, Qt::red) << "ERROR: Lua compile error. compiling script of Trigger:" << mName << "\n" >> mpHost;
            }
            mOK_code = false;
        }
    }
    for (auto trigger : *mpMyChildrenList) {
        trigger->compile();
    }
}

bool TTrigger::setScript(const QString& script)
{
    mScript = script;
    if (script.isEmpty()) {
        mNeedsToBeCompiled = false;
        mOK_code = true;
    } else {
        mNeedsToBeCompiled = true;
        mOK_code = compileScript();
    }
    return mOK_code;
}

bool TTrigger::compileScript()
{
    mFuncName = qsl("Trigger%1").arg(QString::number(mID));
    const QString code = qsl("function %1() %2\nend").arg(mFuncName, mScript);
    QString error;
    if (mpLua->compile(code, error, qsl("Trigger: %1").arg(getName()))) {
        mNeedsToBeCompiled = false;
        mOK_code = true;
        return true;
    } else {
        mOK_code = false;
        setError(error);
        return false;
    }
}

void TTrigger::execute()
{
    if (mSoundTrigger) { /* eventually something should be added to the gui to change sound volumes. 100=full volume */
        QString mediaFileName = mSoundFile;

        if (QDir::homePath().contains('\\')) {
            mediaFileName.replace('/', R"(\)");
        } else {
            mediaFileName.replace('\\', "/");
        }

        TMediaData mediaData{};
        mediaData.setMediaProtocol(TMediaData::MediaProtocolAPI);
        mediaData.setMediaType(TMediaData::MediaTypeSound);
        mediaData.setMediaFileName(mediaFileName);
        mediaData.setMediaVolume(TMediaData::MediaVolumeMax);
        mpHost->mpMedia->playMedia(mediaData);
    }
    if (!mCommand.isEmpty()) {
        mpHost->send(mCommand);
    }
    if (mNeedsToBeCompiled) {
        if (!compileScript()) {
            return;
        }
    }

    if (mRegisteredAnonymousLuaFunction) {
        if (Q_LIKELY(mExpiryCount <= 0)) {
            mpLua->call_luafunction(this);
        } else {
            // if the trigger is a temporary expiring one,
            // don't expire if it returned true
            auto result = mpLua->callLuaFunctionReturnBool(this);
            // if the function ran okay and returned true, it wants to extend the expiry count
            if (result.first && result.second) {
                mExpiryCount++;
            }
        }
        return;
    }

    if (mScript.isEmpty()) {
        return;
    }

    if (mIsMultiline) {
        if (Q_LIKELY(mExpiryCount <= 0)) {
            mpLua->callMulti(mFuncName, mName);
        } else {
            // if the trigger is a temporary expiring one,
            // don't expire if it returned true
            auto result = mpLua->callMultiReturnBool(mFuncName, mName);
            if (result.second) {
                mExpiryCount++;
            }
        }
    } else {
        if (Q_LIKELY(mExpiryCount <= 0)) {
            mpLua->call(mFuncName, mName);
        } else {
            // if the trigger is a temporary expiring one,
            // don't expire if it returned true
            auto result = mpLua->callReturnBool(mFuncName, mName);
            if (result.second) {
                mExpiryCount++;
            }
        }
    }
}

void TTrigger::enableTrigger(const QString& name)
{
    if (mName == name) {
        setIsActive(true);
    }
    for (auto trigger : *mpMyChildrenList) {
        trigger->enableTrigger(name);
    }
}

void TTrigger::disableTrigger(const QString& name)
{
    if (mName == name) {
        setIsActive(false);
    }
    for (auto trigger : *mpMyChildrenList) {
        trigger->disableTrigger(name);
    }
}

TTrigger* TTrigger::killTrigger(const QString& name)
{
    if (mName == name) {
        setIsActive(false);
    }
    for (auto trigger : *mpMyChildrenList) {
        trigger->killTrigger(name);
    }
    return nullptr;
}

// Provide a pair of helpers which means we can provide a slightly more readable
// pattern text for the odd cases - and do it in one place:
QString TTrigger::createColorPatternText(const int fgColorCode, const int bgColorCode)
{
    QString fgText;
    QString bgText;
    if (fgColorCode == scmIgnored) {
        fgText = QLatin1String("IGNORE");
    } else if (fgColorCode == scmDefault) {
        fgText = QLatin1String("DEFAULT");
    } else {
        fgText = qsl("%1").arg(fgColorCode, 3, 10, QLatin1Char('0'));
    }

    if (bgColorCode == scmIgnored) {
        bgText = QLatin1String("IGNORE");
    } else if (bgColorCode == scmDefault) {
        bgText = QLatin1String("DEFAULT");
    } else {
        bgText = qsl("%1").arg(bgColorCode, 3, 10, QLatin1Char('0'));
    }

    // QString::compare(...) returns zero (boolean false) on a match, or a
    // non-zero (boolean true) on no match - and we want to detect when BOTH
    // texts are IGNORE so we return an empty string in that case only - so that
    // it is equivalent to an empty other trigger type:
    return (fgText.compare(QLatin1String("IGNORE")) || bgText.compare(QLatin1String("IGNORE"))) ? qsl("ANSI_COLORS_F{%1}_B{%2}").arg(fgText, bgText) : QString();
}

void TTrigger::decodeColorPatternText(const QString& patternText, int& fgColorCode, int& bgColorCode)
{
    // The numbers used for the text have changed - see table in:
    // TColorTable* TTrigger::createColorPattern(int ansiFg, int ansiBg)
    const QRegularExpression regex = QRegularExpression(qsl("^ANSI_COLORS_F{(\\d+|DEFAULT|IGNORE)}_B{(\\d+|DEFAULT|IGNORE)}$"));
    // Was QRegularExpression regex = QRegularExpression(qsl(R"(FG(\d+)BG(\d+))"));
    const QRegularExpressionMatch match = regex.match(patternText);
    // scmDefault is the new code for "default" colour (as 0 is a valid ANSI color number!)
    // scmIgnored is the new code for "reset" i.e. NOT set color trigger (i.e. don't
    // bother with checking this part of the colour)

    if (match.capturedStart() > -1) {
        // There *is* a match - now examine what was found:
        if (match.captured(1) == QLatin1String("DEFAULT")) {
            fgColorCode = scmDefault;
        } else if (match.captured(1) == QLatin1String("IGNORE")) {
            fgColorCode = scmIgnored;
        } else {
            fgColorCode = match.captured(1).toInt();
        }

        if (match.captured(2) == QLatin1String("DEFAULT")) {
            bgColorCode = scmDefault;
        } else if (match.captured(2) == QLatin1String("IGNORE")) {
            bgColorCode = scmIgnored;
        } else {
            bgColorCode = match.captured(2).toInt();
        }
    } else {
        fgColorCode = scmIgnored;
        bgColorCode = scmIgnored;
    }
}

QString TTrigger::packageName(TTrigger* pTrigger)
{
    if (!pTrigger) {
        return QString();
    }

    if (!pTrigger->mPackageName.isEmpty()) {
        return !mpHost->mModuleInfo.contains(pTrigger->mPackageName) ? pTrigger->mPackageName : QString();
    }

    if (pTrigger->getParent()) {
        return packageName(pTrigger->getParent());
    }

    return QString();
}

QString TTrigger::moduleName(TTrigger* pTrigger)
{
    if (!pTrigger) {
        return QString();
    }

    if (!pTrigger->mPackageName.isEmpty()) {
        return mpHost->mModuleInfo.contains(pTrigger->mPackageName) ? pTrigger->mPackageName : QString();
    }

    if (pTrigger->getParent()) {
        return moduleName(pTrigger->getParent());
    }

    return QString();
}
