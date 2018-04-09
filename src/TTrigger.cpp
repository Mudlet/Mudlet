/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2016 by Chris Leacy - cleacy1972@gmail.com              *
 *   Copyright (C) 2017-2018 by Stephen Lyons - slysven@virginmedia.com    *
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
#include "mudlet.h"

#include "pre_guard.h"
#include <QRegularExpression>
#include "post_guard.h"

#include <assert.h>

#include <sstream>


using namespace std;

const QString nothing = "";

TTrigger::TTrigger( TTrigger * parent, Host * pHost )
: Tree<TTrigger>( parent )
, mTriggerContainsPerlRegex( false )
, mPerlSlashGOption( false )
, mFilterTrigger( false )
, mSoundTrigger( false )
, mStayOpen( 0 )
, mColorTrigger( false )
, mColorTriggerFg( false )
, mColorTriggerBg( false )
, mKeepFiring( 0 )
, mpHost( pHost )
, exportItem(true)
, mModuleMasterFolder(false)
, mNeedsToBeCompiled(true)
, mTriggerType(REGEX_SUBSTRING)

, mIsLineTrigger(false)
, mStartOfLineDelta(0)
, mLineDelta(3)
, mIsMultiline(false)
, mConditionLineDelta(0)
, mpLua(mpHost->getLuaInterpreter())
, mFgColor(QColor(Qt::red))
, mBgColor(QColor(Qt::yellow))
, mIsColorizerTrigger(false)
, mModuleMember(false)
, mColorTriggerFgAnsi()
, mColorTriggerBgAnsi()
, mRegisteredAnonymousLuaFunction(false)
{
}

TTrigger::TTrigger(const QString& name, QStringList regexList, QList<int> regexProperyList, bool isMultiline, Host* pHost)
: Tree<TTrigger>(nullptr)
, mTriggerContainsPerlRegex( false )
, mPerlSlashGOption( false )
, mFilterTrigger( false )
, mSoundTrigger( false )
, mStayOpen( 0 )
, mColorTrigger( false )
, mColorTriggerFg( false )
, mColorTriggerBg( false )
, mKeepFiring( 0 )
, mpHost( pHost )
, mName( name )
, mRegexCodeList( regexList )
, exportItem(true)
, mModuleMasterFolder(false)
, mRegexCodePropertyList(regexProperyList)
, mNeedsToBeCompiled(true)
, mTriggerType(REGEX_SUBSTRING)
, mIsLineTrigger(false)
, mStartOfLineDelta(0)
, mLineDelta(3)
, mIsMultiline(isMultiline)
, mConditionLineDelta(0)
, mpLua(mpHost->getLuaInterpreter())
, mFgColor(QColor(Qt::red))
, mBgColor(QColor(Qt::yellow))
, mIsColorizerTrigger(false)
, mModuleMember(false)
, mColorTriggerFgAnsi()
, mColorTriggerBgAnsi()
, mRegisteredAnonymousLuaFunction(false)
{
    setRegexCodeList(regexList, regexProperyList);
}

TTrigger::~TTrigger()
{
    if (!mpHost) {
        return;
    }
    mpHost->getTriggerUnit()->unregisterTrigger(this);
}

void TTrigger::setName(const QString& name)
{
    if( ! isTemporary() )
    {
        mpHost->getTriggerUnit()->mLookupTable.remove( mName, this );
    }
    mName = name;
    mpHost->getTriggerUnit()->mLookupTable.insertMulti(name, this);
}

static void pcre_deleter(pcre* pointer)
{
    pcre_free(pointer);
}

//FIXME: sperren, wenn code nicht compiliert werden kann *ODER* regex falsch
bool TTrigger::setRegexCodeList(QStringList regexList, QList<int> propertyList)
{
    regexList.replaceInStrings("\n", "");
    mRegexCodeList.clear();
    mRegexMap.clear();
    mRegexCodePropertyList.clear();
    mLuaConditionMap.clear();
    mColorPatternList.clear();
    mTriggerContainsPerlRegex = false;

    if (propertyList.size() != regexList.size()) {
        //FIXME: ronny hat das irgendwie geschafft
        qDebug() << "[CRITICAL ERROR (plz report):] Trigger name=" << mName << " aborting reason: propertyList.size() != regexList.size()";
    }

    if ((propertyList.size() < 1) && (!isFolder()) && (!mColorTrigger)) {
        setError("No patterns defined.");
        mOK_init = false;
        return false;
    }

    bool state = true;

    for (int i = 0; i < regexList.size(); i++) {
        if (regexList.at(i).isEmpty() && propertyList.at(i) != REGEX_PROMPT) {
            continue;
        }

        mRegexCodeList.append(regexList.at(i));
        mRegexCodePropertyList.append(propertyList.at(i));

        if (propertyList.at(i) == REGEX_PERL) {
            const char* error;
            const QByteArray& regexp = regexList.at(i).toUtf8();

            int erroffset;

            // PCRE_UTF8 needed to run compile in UTF-8 mode
            // PCRE_UCP needed for \d, \w etc. to use Unicode properties:
            QSharedPointer<pcre> re(pcre_compile(regexp.constData(), PCRE_UTF8 | PCRE_UCP, &error, &erroffset, nullptr), pcre_deleter);

            if (!re) {
                if (mudlet::debugMode) {
                    TDebug(QColor(Qt::white), QColor(Qt::red)) << "REGEX ERROR: failed to compile, reason:\n" << error << "\n" >> 0;
                    TDebug(QColor(Qt::red), QColor(Qt::gray)) << R"(in: ")" << regexp.constData() << "\"\n" >> 0;
                }
                setError(QStringLiteral("<b><font color='blue'>%1</font></b>")
                                 .arg(tr(R"(Error: in item %1, perl regex: "%2", it failed to compile, reason: "%3".)").arg(QString::number(i), regexp.constData(), error)));
                state = false;
            } else {
                if (mudlet::debugMode) {
                    TDebug(QColor(Qt::white), QColor(Qt::darkGreen)) << "[OK]: REGEX_COMPILE OK\n" >> 0;
                }
            }
            mRegexMap[i] = re;
            mTriggerContainsPerlRegex = true;
        }

        if (propertyList.at(i) == REGEX_LUA_CODE) {
            std::string funcName;
            std::stringstream func;
            func << "trigger" << mID << "condition" << i;
            funcName = func.str();
            QString code = QStringLiteral("function %1()\n%2\nend\n").arg(funcName.c_str(), regexList[i]);
            QString error;
            if (!mpLua->compile(code, error, QString::fromStdString(funcName))) {
                setError(QStringLiteral("<b><font color='blue'>%1</font></b>")
                                 .arg(tr(R"(Error: in item %1, lua condition function "%2" failed to compile, reason:%3.)").arg(QString::number(i), regexList.at(i), error)));
                state = false;
                if (mudlet::debugMode) {
                    TDebug(QColor(Qt::white), QColor(Qt::red)) << "LUA ERROR: failed to compile, reason:\n" << error << "\n" >> 0;
                    TDebug(QColor(Qt::red), QColor(Qt::gray)) << R"(in lua condition function: ")" << regexList.at(i) << "\"\n" >> 0;
                }
            } else {
                mLuaConditionMap[i] = funcName;
            }
        }

        if (propertyList[i] == REGEX_COLOR_PATTERN) {
            QRegularExpression regex = QRegularExpression(QStringLiteral(R"(FG(\d+)BG(\d+))"));
            QRegularExpressionMatch match = regex.match(regexList[i]);

            int _pos = match.capturedStart();

            if (_pos == -1) {
                mColorPatternList.push_back(nullptr);
                state = false;
                continue;
            }

            int ansiFg = match.captured(1).toInt();
            int ansiBg = match.captured(2).toInt();

            if (!setupColorTrigger(ansiFg, ansiBg)) {
                mColorPatternList.push_back(nullptr);
                state = false;
                continue;
            }
        } else {
            mColorPatternList.push_back(nullptr);
        }
    }
    if (!state) {
        mOK_init = false;
    } else {
        mOK_init = true;
    }
    return state;
}

bool TTrigger::match_perl(char* subject, const QString& toMatch, int regexNumber, int posOffset)
{
    assert(mRegexMap.contains(regexNumber));

    QSharedPointer<pcre> re = mRegexMap[regexNumber];

    if (!re) {
        if (mudlet::debugMode) {
            TDebug(QColor(Qt::white), QColor(Qt::red)) << "ERROR:" << 0;
            TDebug(QColor(Qt::darkRed), QColor(Qt::darkGray)) << " the regex of trigger " << mName
                                                              << " does not compile. Please correct the expression. This trigger will never match until it is fixed.\n"
                    >> 0;
        }
        return false; //regex compile error
    }

    int numberOfCaptureGroups = 0;
    unsigned char* name_table;
    int namecount;
    int name_entry_size;

    int subject_length = strlen(subject);
    int rc, i;
    std::list<std::string> captureList;
    std::list<int> posList;
    int ovector[300]; // 100 capture groups max (can be increase nbGroups=1/3 ovector

    rc = pcre_exec(re.data(), nullptr, subject, subject_length, 0, 0, ovector, 100);

    if (rc < 0) {
        return false;
    } else if (rc == 0) {
        qDebug() << "CRITICAL ERROR: SHOULD NOT HAPPEN->pcre_info() got wrong num of cap groups ovector only has room for %d captured substrings\n";
    } else {
        if (mudlet::debugMode) {
            TDebug(QColor(Qt::blue), QColor(Qt::black)) << "Trigger name=" << mName << "(" << mRegexCodeList.value(regexNumber) << ") matched.\n" >> 0;
        }
    }

    for (i = 0; i < rc; i++) {
        char* substring_start = subject + ovector[2 * i];
        int substring_length = ovector[2 * i + 1] - ovector[2 * i];
        std::string match;
        if (substring_length < 1) {
            captureList.push_back(match);
            posList.push_back(-1);
            continue;
        }

        match.append(substring_start, substring_length);
        captureList.push_back(match);
        posList.push_back(ovector[2 * i] + posOffset);
        if (mudlet::debugMode) {
            TDebug(QColor(Qt::darkCyan), QColor(Qt::black)) << "capture group #" << (i + 1) << " = " >> 0;
            TDebug(QColor(Qt::darkMagenta), QColor(Qt::black)) << "<" << match.c_str() << ">\n" >> 0;
        }
    }
    pcre_fullinfo(re.data(), nullptr, PCRE_INFO_NAMECOUNT, &namecount);

    if (namecount <= 0) {
        ;// Do something?
    } else {
        unsigned char* tabptr;
        pcre_fullinfo(re.data(), nullptr, PCRE_INFO_NAMETABLE, &name_table);

        pcre_fullinfo(re.data(), nullptr, PCRE_INFO_NAMEENTRYSIZE, &name_entry_size);

        tabptr = name_table;
        for (i = 0; i < namecount; i++) {
            tabptr += name_entry_size;
        }
    }
    //TODO: add named groups seperately later as Lua::namedGroups
    if (mIsColorizerTrigger || mFilterTrigger) {
        numberOfCaptureGroups = captureList.size();
    }
    for (; mPerlSlashGOption;) {
        int options = 0;
        int start_offset = ovector[1];

        if (ovector[0] == ovector[1]) {
            if (ovector[0] >= subject_length) {
                goto END;
            }
            options = PCRE_NOTEMPTY | PCRE_ANCHORED;
        }

        rc = pcre_exec(re.data(), nullptr, subject, subject_length, start_offset, options, ovector, 30);

        if (rc == PCRE_ERROR_NOMATCH) {
            if (options == 0) {
                break;
            }
            ovector[1] = start_offset + 1;
            continue;
        } else if (rc < 0) {
            goto END;
        } else if (rc == 0) {
            qDebug() << "CRITICAL ERROR: SHOULD NOT HAPPEN->pcre_info() got wrong num of cap groups ovector only has room for %d captured substrings\n";
        }

        for (i = 0; i < rc; i++) {
            char* substring_start = subject + ovector[2 * i];
            int substring_length = ovector[2 * i + 1] - ovector[2 * i];

            std::string match;
            if (substring_length < 1) {
                captureList.push_back(match);
                posList.push_back(-1);
                continue;
            }
            match.append(substring_start, substring_length);
            captureList.push_back(match);
            posList.push_back(ovector[2 * i] + posOffset);
            if (mudlet::debugMode) {
                TDebug(QColor(Qt::darkCyan), QColor(Qt::black)) << "<regex mode: match all> capture group #" << (i + 1) << " = " >> 0;
                TDebug(QColor(Qt::darkMagenta), QColor(Qt::black)) << "<" << match.c_str() << ">\n" >> 0;
            }
        }
    }

END : {
    if (mIsColorizerTrigger) {
        int r1 = mBgColor.red();
        int g1 = mBgColor.green();
        int b1 = mBgColor.blue();
        int r2 = mFgColor.red();
        int g2 = mFgColor.green();
        int b2 = mFgColor.blue();
        int total = captureList.size();
        TConsole* pC = mpHost->mpConsole;
        pC->deselect();
        auto its = captureList.begin();
        auto iti = posList.begin();
        for (int i = 1; iti != posList.end(); ++iti, ++its, i++) {
            int begin = *iti;
            std::string& s = *its;
            int length = s.size();
            if (total > 1) {
                // skip complete match in Perl /g option type of triggers
                // to enable people to highlight capture groups if there are any
                // otherwise highlight complete expression match
                if (i % numberOfCaptureGroups != 1) {
                    pC->selectSection(begin, length);
                    pC->setBgColor(r1, g1, b1);
                    pC->setFgColor(r2, g2, b2);
                }
            } else {
                pC->selectSection(begin, length);
                pC->setBgColor(r1, g1, b1);
                pC->setFgColor(r2, g2, b2);
            }
        }
        pC->reset();
    }
    if (mIsMultiline) {
        updateMultistates(regexNumber, captureList, posList);
        return true;
    } else {
        TLuaInterpreter* pL = mpHost->getLuaInterpreter();
        pL->setCaptureGroups(captureList, posList);
        execute();
        pL->clearCaptureGroups();
        if (mFilterTrigger) {
            if (captureList.size() > 1) {
                int total = captureList.size();
                auto its = captureList.begin();
                auto iti = posList.begin();
                for (int i = 1; iti != posList.end(); ++iti, ++its, i++) {
                    int begin = *iti;
                    std::string& s = *its;
                    if (total > 1) {
                        // skip complete match in Perl /g option type of triggers
                        // to enable people to highlight capture groups if there are any
                        // otherwise highlight complete expression match
                        if (i % numberOfCaptureGroups != 1) {
                            filter(s, begin);
                        }
                    } else {
                        filter(s, begin);
                    }
                }
            }
        }
        return true;
    }
}
    return true;
}

bool TTrigger::match_begin_of_line_substring(const QString& toMatch, const QString& regex, int regexNumber, int posOffset)
{
    if (toMatch.startsWith(regex)) {
        std::list<std::string> captureList;
        std::list<int> posList;
        captureList.emplace_back(regex.toUtf8().constData());
        posList.push_back(0 + posOffset);
        if (mudlet::debugMode) {
            TDebug(QColor(Qt::darkCyan), QColor(Qt::black)) << "Trigger name=" << mName << "(" << mRegexCodeList.value(regexNumber) << ") matched.\n" >> 0;
        }
        if (mIsColorizerTrigger) {
            int r1 = mBgColor.red();
            int g1 = mBgColor.green();
            int b1 = mBgColor.blue();
            int r2 = mFgColor.red();
            int g2 = mFgColor.green();
            int b2 = mFgColor.blue();
            TConsole* pC = mpHost->mpConsole;
            auto its = captureList.begin();
            for (auto iti = posList.begin(); iti != posList.end(); ++iti, ++its) {
                int begin = *iti;
                std::string& s = *its;
                int length = s.size();
                pC->selectSection(begin, length);
                pC->setBgColor(r1, g1, b1);
                pC->setFgColor(r2, g2, b2);
            }
            pC->reset();
        }
        if (mIsMultiline) {
            updateMultistates(regexNumber, captureList, posList);
            return true;
        } else {
            TLuaInterpreter* pL = mpHost->getLuaInterpreter();
            pL->setCaptureGroups(captureList, posList);

            // call lua trigger function with number of matches and matches itselves as arguments
            execute();
            pL->clearCaptureGroups();
            if (mFilterTrigger) {
                if (captureList.size() > 0) {
                    filter(captureList.front(), posList.front());
                }
            }
            return true;
        }
    }
    return false;
}

inline void TTrigger::updateMultistates(int regexNumber, std::list<std::string>& captureList, std::list<int>& posList)
{
    if (regexNumber == 0) {
        // wird automatisch auf #1 gesetzt
        auto pCondition = new TMatchState(mRegexCodeList.size(), mConditionLineDelta);
        mConditionMap[pCondition] = pCondition;
        pCondition->multiCaptureList.push_back(captureList);
        pCondition->multiCapturePosList.push_back(posList);
        if (mudlet::debugMode) {
            TDebug(QColor(Qt::darkYellow), QColor(Qt::black)) << "match state " << mConditionMap.size() << "/" << mConditionMap.size() << " condition #" << regexNumber << "=true (" << regexNumber
                                                              << "/" << mRegexCodeList.size() << ") regex=" << mRegexCodeList[regexNumber] << "\n"
                    >> 0;
        }
    } else {
        int k = 0;
        for (auto& matchStatePair : mConditionMap) {
            k++;
            if (matchStatePair.second->nextCondition() == regexNumber) {
                if (mudlet::debugMode) {
                    TDebug(QColor(Qt::darkYellow), QColor(Qt::black)) << "match state " << k << "/" << mConditionMap.size() << " condition #" << regexNumber << "=true (" << regexNumber << "/"
                                                                      << mRegexCodeList.size() << ") regex=" << mRegexCodeList[regexNumber] << "\n"
                            >> 0;
                }
                matchStatePair.second->conditionMatched();
                matchStatePair.second->multiCaptureList.push_back(captureList);
                matchStatePair.second->multiCapturePosList.push_back(posList);
            }
        }
    }
}

inline void TTrigger::filter(std::string& capture, int& posOffset)
{
    if (capture.size() < 1) {
        return;
    }
    char* filterSubject = (char*)malloc(capture.size() + 2048);
    if (filterSubject) {
        strcpy(filterSubject, capture.c_str());
    } else {
        return;
    }
    QString text = capture.c_str();
    for (auto& trigger : *mpMyChildrenList) {
        trigger->match(filterSubject, text, -1, posOffset);
    }
    free(filterSubject);
}

bool TTrigger::match_substring(const QString& toMatch, const QString& regex, int regexNumber, int posOffset)
{
    int where = toMatch.indexOf(regex);
    if (where != -1) {
        std::list<std::string> captureList;
        std::list<int> posList;
        captureList.emplace_back(regex.toUtf8().constData());
        posList.push_back(where + posOffset);
        if (mPerlSlashGOption) {
            while ((where = toMatch.indexOf(regex, where + 1)) != -1) {
                captureList.emplace_back(regex.toUtf8().constData());
                posList.push_back(where + posOffset);
            }
        }
        if (mudlet::debugMode) {
            TDebug(QColor(Qt::cyan), QColor(Qt::black)) << "Trigger name=" << mName << "(" << mRegexCodeList.value(regexNumber) << ") matched.\n" >> 0;
        }
        if (mIsColorizerTrigger) {
            int r1 = mBgColor.red();
            int g1 = mBgColor.green();
            int b1 = mBgColor.blue();
            int r2 = mFgColor.red();
            int g2 = mFgColor.green();
            int b2 = mFgColor.blue();
            TConsole* pC = mpHost->mpConsole;
            pC->deselect();
            auto its = captureList.begin();
            for (auto iti = posList.begin(); iti != posList.end(); ++iti, ++its) {
                int begin = *iti;
                std::string& s = *its;
                int length = s.size();
                pC->selectSection(begin, length);
                pC->setBgColor(r1, g1, b1);
                pC->setFgColor(r2, g2, b2);
            }
            pC->reset();
        }
        if (mIsMultiline) {
            updateMultistates(regexNumber, captureList, posList);
            return true;
        } else {
            TLuaInterpreter* pL = mpHost->getLuaInterpreter();
            pL->setCaptureGroups(captureList, posList);

            // call lua trigger function with number of matches and matches itselves as arguments
            execute();
            pL->clearCaptureGroups();
            if (mFilterTrigger) {
                if (captureList.size() > 0) {
                    filter(captureList.front(), posList.front());
                }
            }
            return true;
        }
    }
    return false;
}

bool TTrigger::match_color_pattern(int line, int regexNumber)
{
    if (regexNumber >= mColorPatternList.size()) {
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
    QString& lineBuffer = mpHost->mpConsole->buffer.lineBuffer[line];
    int pos = 0;
    int matchBegin = -1;
    bool matching = false;

    TColorTable* pCT = mColorPatternList[regexNumber];
    if (!pCT) {
        return false; //no color pattern created
    }

    for (auto it = bufferLine.begin(); it != bufferLine.end(); it++, pos++) {
        if (pCT->mFgColor == (*it).foreground() && pCT->mBgColor == (*it).background()) {
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
        if (mIsColorizerTrigger) {
            int r1 = mBgColor.red();
            int g1 = mBgColor.green();
            int b1 = mBgColor.blue();
            int r2 = mFgColor.red();
            int g2 = mFgColor.green();
            int b2 = mFgColor.blue();
            TConsole* pC = mpHost->mpConsole;
            pC->deselect();
            auto its = captureList.begin();
            for (auto iti = posList.begin(); iti != posList.end(); ++iti, ++its) {
                int begin = *iti;
                qDebug() << "TTrigger::match_color_pattern(" << line << "," << regexNumber << ") INFO - match found: " << (*its).c_str() << " size is:" << (*its).size();
                int length = (*its).size();
                pC->selectSection(begin, length);
                pC->setBgColor(r1, g1, b1);
                pC->setFgColor(r2, g2, b2);
            }
            pC->reset();
        }
        if (mIsMultiline) {
            updateMultistates(regexNumber, captureList, posList);
            return true;
        } else {
            TLuaInterpreter* pL = mpHost->getLuaInterpreter();
            pL->setCaptureGroups(captureList, posList);
            // call lua trigger function with number of matches and matches itselves as arguments
            execute();
            pL->clearCaptureGroups();
            if (mFilterTrigger) {
                if (captureList.size() > 0) {
                    auto it1 = captureList.begin();
                    auto it2 = posList.begin();
                    for (; it1 != captureList.end(); it1++, it2++) {
                        filter(*it1, *it2);
                    }
                }
            }
            return true;
        }
    }
    return false;
}

bool TTrigger::match_line_spacer(int regexNumber)
{
    if (mIsMultiline) {
        int k = 0;

        for (auto& matchStatePair : mConditionMap) {
            k++;
            if (matchStatePair.second->nextCondition() == regexNumber) {
                if (matchStatePair.second->lineSpacerMatch(mRegexCodeList.value(regexNumber).toInt())) {
                    if (mudlet::debugMode) {
                        TDebug(QColor(Qt::yellow), QColor(Qt::black)) << "Trigger name=" << mName << "(" << mRegexCodeList.value(regexNumber) << ") condition #" << regexNumber << "=true " >> 0;
                        TDebug(QColor(Qt::darkYellow), QColor(Qt::black)) << "match state " << k << "/" << mConditionMap.size() << " condition #" << regexNumber << "=true (" << regexNumber + 1 << "/"
                                                                          << mRegexCodeList.size() << ") line spacer=" << mRegexCodeList.value(regexNumber) << "lines\n"
                                >> 0;
                    }
                    matchStatePair.second->conditionMatched();
                    std::list<string> captureList;
                    std::list<int> posList;
                    matchStatePair.second->multiCaptureList.push_back(captureList);
                    matchStatePair.second->multiCapturePosList.push_back(posList);
                }
            }
        }
    }

    return true; //line spacers don't make sense outside of AND triggers -> ignore them
}

bool TTrigger::match_lua_code(int regexNumber)
{
    if (mLuaConditionMap.find(regexNumber) == mLuaConditionMap.end()) {
        return false;
    }

    if (mpLua->callConditionFunction(mLuaConditionMap[regexNumber], mName)) {
        if (mudlet::debugMode) {
            TDebug(QColor(Qt::yellow), QColor(Qt::black)) << "Trigger name=" << mName << "(" << mRegexCodeList.value(regexNumber) << ") matched.\n" >> 0;
        }
        if (mIsMultiline) {
            std::list<std::string> captureList;
            std::list<int> posList;
            updateMultistates(regexNumber, captureList, posList);
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
        if (mudlet::debugMode) {
            TDebug(QColor(Qt::yellow), QColor(Qt::black)) << "Trigger name=" << mName << "(" << mRegexCodeList.value(patternNumber) << ") matched.\n" >> 0;
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

bool TTrigger::match_exact_match(const QString& toMatch, const QString& line, int regexNumber, int posOffset)
{
    QString text = toMatch;
    if (text.endsWith(QChar('\n'))) {
        text.chop(1); //TODO: speed optimization
    }
    if (text == line) {
        std::list<std::string> captureList;
        std::list<int> posList;
        captureList.emplace_back(line.toUtf8().constData());
        posList.push_back(0 + posOffset);
        if (mudlet::debugMode) {
            TDebug(QColor(Qt::yellow), QColor(Qt::black)) << "Trigger name=" << mName << "(" << mRegexCodeList.value(regexNumber) << ") matched.\n" >> 0;
        }
        if (mIsColorizerTrigger) {
            int r1 = mBgColor.red();
            int g1 = mBgColor.green();
            int b1 = mBgColor.blue();
            int r2 = mFgColor.red();
            int g2 = mFgColor.green();
            int b2 = mFgColor.blue();
            TConsole* pC = mpHost->mpConsole;
            auto its = captureList.begin();
            for (auto iti = posList.begin(); iti != posList.end(); ++iti, ++its) {
                int begin = *iti;
                std::string& s = *its;
                int length = s.size();
                pC->selectSection(begin, length);
                pC->setBgColor(r1, g1, b1);
                pC->setFgColor(r2, g2, b2);
            }
            pC->reset();
        }
        if (mIsMultiline) {
            updateMultistates(regexNumber, captureList, posList);
            return true;
        } else {
            TLuaInterpreter* pL = mpHost->getLuaInterpreter();
            pL->setCaptureGroups(captureList, posList);
            // call lua trigger function with number of matches and matches itselves as arguments
            execute();
            pL->clearCaptureGroups();
            if (mFilterTrigger) {
                if (captureList.size() > 0) {
                    filter(captureList.front(), posList.front());
                }
            }
            return true;
        }
    }
    return false;
}

bool TTrigger::match(char* subject, const QString& toMatch, int line, int posOffset)
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

        if (toMatch.size() < 1) {
            return false;
        }

        bool conditionMet = false;

        int highestCondition = 0;
        if (mIsMultiline) {
            for (auto& matchStatePair : mConditionMap) {
                matchStatePair.second->newLineArrived();
                int next = matchStatePair.second->nextCondition();
                if (next > highestCondition) {
                    highestCondition = next;
                }
            }
        }

        int size = mRegexCodePropertyList.size();
        for (int patternNumber = 0;; patternNumber++) {
            if (patternNumber >= size) {
                break;
            }
            ret = false;
            switch (mRegexCodePropertyList.value(patternNumber)) {
            case REGEX_SUBSTRING:
                ret = match_substring(toMatch, mRegexCodeList[patternNumber], patternNumber, posOffset);
                break;

            case REGEX_PERL:
                ret = match_perl(subject, toMatch, patternNumber, posOffset);
                break;

            case REGEX_BEGIN_OF_LINE_SUBSTRING:
                ret = match_begin_of_line_substring(toMatch, mRegexCodeList[patternNumber], patternNumber, posOffset);
                break;

            case REGEX_EXACT_MATCH:
                ret = match_exact_match(toMatch, mRegexCodeList[patternNumber], patternNumber, posOffset);
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
            list<TMatchState*> removeList;

            for (auto& matchStatePair : mConditionMap) {
                k++;
                if (matchStatePair.second->isComplete()) {
                    mKeepFiring = mStayOpen;
                    if (mudlet::debugMode) {
                        TDebug(QColor(Qt::yellow), QColor(Qt::darkMagenta)) << "multiline trigger name=" << mName << " *FIRES* all conditons are fullfilled. Executing script.\n" >> 0;
                    }
                    removeList.push_back(matchStatePair.first);
                    conditionMet = true;
                    TLuaInterpreter* pL = mpHost->getLuaInterpreter();
                    pL->setMultiCaptureGroups(matchStatePair.second->multiCaptureList, matchStatePair.second->multiCapturePosList);
                    execute();
                    pL->clearCaptureGroups();
                    if (mFilterTrigger) {
                        std::list<std::list<std::string>> multiCaptureList;
                        multiCaptureList = matchStatePair.second->multiCaptureList;
                        if (multiCaptureList.size() > 0) {
                            for (auto mit = multiCaptureList.begin(); mit != multiCaptureList.end(); mit++, k++) {
                                int total = (*mit).size();
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
                    if (mudlet::debugMode) {
                        TDebug(QColor(Qt::darkBlue), QColor(Qt::black)) << "removing condition from conditon table.\n" >> 0;
                    }
                    mConditionMap.erase(matchState);
                }
            }
        }


        // definition trigger chain: a folder is part of a trigger chain if it has a regex defined
        // a trigger chain only lets data pass if the condition matches or in case of multiline all
        // all conditions are fullfilled
        //
        // a folder can also be a simple structural element in which case all data passes through
        // if at least one regex is defined a folder is considered a trigger chain otherwise a structural element
        if (!mFilterTrigger) {
            if (conditionMet || (mRegexCodeList.size() < 1)) {
                for (auto trigger : *mpMyChildrenList) {
                    ret = trigger->match(subject, toMatch, line);
                    if (ret) {
                        conditionMet = true;
                    }
                }
            }
        }

        if ((mKeepFiring > 0) && (!conditionMet)) {
            mKeepFiring--;
            if ((mKeepFiring == mStayOpen) || (mpMyChildrenList->size() == 0)) {
                execute();
            }
            for (auto trigger : *mpMyChildrenList) {
                ret = trigger->match(subject, toMatch, line);
                if (ret) {
                    conditionMet = true;
                }
            }
            return true;
        }


        return conditionMet;
    }
    return false;
}


// Die Musternummer wird ID im color-pattern lookup table
// This NOW uses proper ANSI numbers
TColorTable* TTrigger::createColorPattern(int ansiFg, int ansiBg)
{
    /* OLD Mudlet simplified ANSI color codes
                            -> proper ANSI numbers
      ---------------------------------------
      0  default text color -> -1 (NEW code)
      1  light black        ->  8
      2  dark black         ->  0
      3  light red          ->  9
      4  dark red           ->  1
      5  light green        -> 10
      6  dark green         ->  2
      7  light yellow       -> 11
      8  dark yellow        ->  3
      9  light blue         -> 12
      10 dark blue          ->  4
      11 light magenta      -> 13
      12 dark magenta       ->  5
      13 light cyan         -> 14
      14 dark cyan          ->  6
      15 light white        -> 15
      16 dark white         ->  7
 */

    // clang-format off
    QColor fgColor;
    switch (ansiFg) {
    case -1:        fgColor = mpHost->mFgColor;         break;
    case 0:         fgColor = mpHost->mBlack;           break;
    case 1:         fgColor = mpHost->mRed;             break;
    case 2:         fgColor = mpHost->mGreen;           break;
    case 3:         fgColor = mpHost->mYellow;          break;
    case 4:         fgColor = mpHost->mBlue;            break;
    case 5:         fgColor = mpHost->mMagenta;         break;
    case 6:         fgColor = mpHost->mCyan;            break;
    case 7:         fgColor = mpHost->mWhite;           break;
    case 8:         fgColor = mpHost->mLightBlack;      break;
    case 9:         fgColor = mpHost->mLightRed;        break;
    case 10:        fgColor = mpHost->mLightGreen;      break;
    case 11:        fgColor = mpHost->mLightYellow;     break;
    case 12:        fgColor = mpHost->mLightBlue;       break;
    case 13:        fgColor = mpHost->mLightMagenta;    break;
    case 14:        fgColor = mpHost->mLightCyan;       break;
    case 15:        fgColor = mpHost->mLightWhite;      break;
    // Grey scale divided into 24 values:
    case 232:       fgColor = QColor(  0,   0,   0);    break; //   0.000
    case 233:       fgColor = QColor( 11,  11,  11);    break; //  11.087
    case 234:       fgColor = QColor( 22,  22,  22);    break; //  22.174
    case 235:       fgColor = QColor( 33,  33,  33);    break; //  33.261
    case 236:       fgColor = QColor( 44,  44,  44);    break; //  44.348
    case 237:       fgColor = QColor( 55,  55,  55);    break; //  55.435
    case 238:       fgColor = QColor( 67,  67,  67);    break; //  66.522
    case 239:       fgColor = QColor( 78,  78,  78);    break; //  77.609
    case 240:       fgColor = QColor( 89,  89,  89);    break; //  88.696
    case 241:       fgColor = QColor(100, 100, 100);    break; //  99.783
    case 242:       fgColor = QColor(111, 111, 111);    break; // 110.870
    case 243:       fgColor = QColor(122, 122, 122);    break; // 121.957
    case 244:       fgColor = QColor(133, 133, 133);    break; // 133.043
    case 245:       fgColor = QColor(144, 144, 144);    break; // 144.130
    case 246:       fgColor = QColor(155, 155, 155);    break; // 155.217
    case 247:       fgColor = QColor(166, 166, 166);    break; // 166.304
    case 248:       fgColor = QColor(177, 177, 177);    break; // 177.391
    case 249:       fgColor = QColor(188, 188, 188);    break; // 188.478
    case 250:       fgColor = QColor(200, 200, 200);    break; // 199.565
    case 251:       fgColor = QColor(211, 211, 211);    break; // 210.652
    case 252:       fgColor = QColor(222, 222, 222);    break; // 221.739
    case 253:       fgColor = QColor(233, 233, 233);    break; // 232.826
    case 254:       fgColor = QColor(244, 244, 244);    break; // 243.913
    case 255:       fgColor = QColor(255, 255, 255);    break; // 255.000
    default:
        if (ansiFg >= 16 && ansiFg <= 231) {
            // because color 1-15 behave like normal ANSI colors we need to subtract 16
            // 6x6 RGB color space
            int r = (ansiFg - 16) / 36;
            int g = (ansiFg - 16 - (r * 36)) / 6;
            int b = (ansiFg - 16 - (r * 36)) - (g * 6);
            // The following WERE using 42 as factor but that does not reflect
            // changes already made in TBuffer::translateToPlainText a while ago:
            fgColor = QColor(r * 51, g * 51, b * 51);
        } else {
            return nullptr;
        }
    }

    QColor bgColor;
    switch (ansiBg) {
    case -1:        bgColor = mpHost->mBgColor;         break;
    case 0:         bgColor = mpHost->mBlack;           break;
    case 1:         bgColor = mpHost->mRed;             break;
    case 2:         bgColor = mpHost->mGreen;           break;
    case 3:         bgColor = mpHost->mYellow;          break;
    case 4:         bgColor = mpHost->mBlue;            break;
    case 5:         bgColor = mpHost->mMagenta;         break;
    case 6:         bgColor = mpHost->mCyan;            break;
    case 7:         bgColor = mpHost->mWhite;           break;
    case 8:         bgColor = mpHost->mLightBlack;      break;
    case 9:         bgColor = mpHost->mLightRed;        break;
    case 10:        bgColor = mpHost->mLightGreen;      break;
    case 11:        bgColor = mpHost->mLightYellow;     break;
    case 12:        bgColor = mpHost->mLightBlue;       break;
    case 13:        bgColor = mpHost->mLightMagenta;    break;
    case 14:        bgColor = mpHost->mLightCyan;       break;
    case 15:        bgColor = mpHost->mLightWhite;      break;
    // Grey scale divided into 24 values:
    case 232:       bgColor = QColor(  0,   0,   0);    break; //   0.000
    case 233:       bgColor = QColor( 11,  11,  11);    break; //  11.087
    case 234:       bgColor = QColor( 22,  22,  22);    break; //  22.174
    case 235:       bgColor = QColor( 33,  33,  33);    break; //  33.261
    case 236:       bgColor = QColor( 44,  44,  44);    break; //  44.348
    case 237:       bgColor = QColor( 55,  55,  55);    break; //  55.435
    case 238:       bgColor = QColor( 67,  67,  67);    break; //  66.522
    case 239:       bgColor = QColor( 78,  78,  78);    break; //  77.609
    case 240:       bgColor = QColor( 89,  89,  89);    break; //  88.696
    case 241:       bgColor = QColor(100, 100, 100);    break; //  99.783
    case 242:       bgColor = QColor(111, 111, 111);    break; // 110.870
    case 243:       bgColor = QColor(122, 122, 122);    break; // 121.957
    case 244:       bgColor = QColor(133, 133, 133);    break; // 133.043
    case 245:       bgColor = QColor(144, 144, 144);    break; // 144.130
    case 246:       bgColor = QColor(155, 155, 155);    break; // 155.217
    case 247:       bgColor = QColor(166, 166, 166);    break; // 166.304
    case 248:       bgColor = QColor(177, 177, 177);    break; // 177.391
    case 249:       bgColor = QColor(188, 188, 188);    break; // 188.478
    case 250:       bgColor = QColor(200, 200, 200);    break; // 199.565
    case 251:       bgColor = QColor(211, 211, 211);    break; // 210.652
    case 252:       bgColor = QColor(222, 222, 222);    break; // 221.739
    case 253:       bgColor = QColor(233, 233, 233);    break; // 232.826
    case 254:       bgColor = QColor(244, 244, 244);    break; // 243.913
    case 255:       bgColor = QColor(255, 255, 255);    break; // 255.000
    default:
        if (ansiBg >= 16 && ansiBg <= 231) {
            // because color 1-15 behave like normal ANSI colors we need to subtract 16
            // 6x6 RGB color space
            int r = (ansiBg - 16)  / 36;
            int g = (ansiBg - 16 - (r * 36)) / 6;
            int b = (ansiBg - 16 - (r * 36)) - (g * 6);
            bgColor = QColor(r * 51, g * 51, b * 51);
        } else {
            return nullptr;
        }
    }
    // clang-format on

    auto pCT = new TColorTable;
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
        return false;
    }
    mColorPatternList.push_back(pCT);
    return true;
}

bool TTrigger::setupTmpColorTrigger(int ansiFg, int ansiBg)
{
    TColorTable* pCT = createColorPattern(ansiFg, ansiBg);
    if (!pCT) {
        return false;
    }
    QString code;
    code = QString("FG%1BG%2").arg(QString::number(ansiFg), QString::number(ansiBg));
    mRegexCodeList << code;
    mRegexCodePropertyList << REGEX_COLOR_PATTERN;
    mColorPatternList.push_back(pCT);
    return true;
}

bool TTrigger::isFilterChain()
{
    if ((mRegexCodeList.size() > 0) && (hasChildren())) {
        return true;
    } else {
        return false;
    }
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
        if (mudlet::debugMode) {
            TDebug(QColor(Qt::white), QColor(Qt::red)) << "ERROR: Lua compile error. compiling script of Trigger:" << mName << "\n" >> 0;
        }
        mOK_code = false;
    }
    setRegexCodeList(mRegexCodeList, mRegexCodePropertyList);
    for (auto trigger : *mpMyChildrenList) {
        trigger->compileAll();
    }
}


void TTrigger::compile()
{
    if (mNeedsToBeCompiled) {
        if (!compileScript()) {
            if (mudlet::debugMode) {
                TDebug(QColor(Qt::white), QColor(Qt::red)) << "ERROR: Lua compile error. compiling script of Trigger:" << mName << "\n" >> 0;
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
    mFuncName = QString("Trigger") + QString::number(mID);
    QString code = QString("function ") + mFuncName + QString("()\n") + mScript + QString("\nend\n");
    QString error;
    if (mpLua->compile(code, error, QString("Trigger: ") + getName())) {
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
        mudlet::self()->playSound(mSoundFile, 100);
    }
    if (mCommand.size() > 0) {
        mpHost->send(mCommand);
    }
    if (mNeedsToBeCompiled) {
        if (!compileScript()) {
            return;
        }
    }

    if (mRegisteredAnonymousLuaFunction) {
        mpLua->call_luafunction(this);
        return;
    }

    if (mScript.isEmpty()) {
        return;
    }

    if (mIsMultiline) {
        mpLua->callMulti(mFuncName, mName);
    } else {
        mpLua->call(mFuncName, mName);
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
