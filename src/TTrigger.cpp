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
#include <QRegularExpression>
#include "TConsole.h"
#include "TDebug.h"
#include "TMatchState.h"
#include "mudlet.h"

#include <cassert>
#include <sstream>

// Some extraordinary numbers outside of the range (0-255) used for ANSI colors:
// Changing them WILL modify the Lua API of TLuaInterpreter::tempColorTrigger
// and the replacement TLuaInterpreter::tempAnsiColorTrigger
const int TTrigger::scmDefault = -2;
const int TTrigger::scmIgnored = -1;

TTrigger::TTrigger( TTrigger * parent, Host * pHost )
: Tree<TTrigger>( parent )
, mTriggerContainsPerlRegex( false )
, mPerlSlashGOption( false )
, mFilterTrigger( false )
, mSoundTrigger( false )
, mStayOpen( 0 )
, mColorTrigger( false )
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
, mColorTriggerFgAnsi(scmIgnored)
, mColorTriggerBgAnsi(scmIgnored)
, mRegisteredAnonymousLuaFunction(false)
, mExpiryCount(-1)
{
}

TTrigger::TTrigger(const QString& name, const QStringList& regexList, const QList<int>& regexProperyList, bool isMultiline, Host* pHost)
: Tree<TTrigger>(nullptr)
, mTriggerContainsPerlRegex( false )
, mPerlSlashGOption( false )
, mFilterTrigger( false )
, mSoundTrigger( false )
, mStayOpen( 0 )
, mColorTrigger( false )
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
, mColorTriggerFgAnsi(scmIgnored)
, mColorTriggerBgAnsi(scmIgnored)
, mRegisteredAnonymousLuaFunction(false)
, mExpiryCount(-1)
{
    setRegexCodeList(regexList, regexProperyList);
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
    if (!mpHost) {
        return;
    }
    mpHost->getTriggerUnit()->unregisterTrigger(this);
}

void TTrigger::setName(const QString& name)
{
    if (!isTemporary()) {
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

    if (propertyList.size() != regexList.size()) {
        //FIXME: ronny hat das irgendwie geschafft
        qDebug() << "[CRITICAL ERROR (plz report):] Trigger name=" << mName << " aborting reason: propertyList.size() != regexList.size()";
    }

    if ((propertyList.empty()) && (!isFolder()) && (!mColorTrigger)) {
        setError(QStringLiteral("<b><font color='blue'>%1</font></b>")
                .arg(tr("Error: This trigger has no patterns defined, yet. Add some to activate it.")));
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
                         .arg(tr(R"(Error: in item %1, perl regex "%2" failed to compile, reason: "%3".)")
                         .arg(QString::number(i + 1), regexp.constData(), error)));
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
                         .arg(tr(R"(Error: in item %1, lua function "%2" failed to compile, reason: "%3".)")
                         .arg(QString::number(i + 1), regexList.at(i), error)));
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
            int textAnsiFg = scmIgnored;
            int textAnsiBg = scmIgnored;
            // Decode the pattern string to the colour codes wanted:
            TTrigger::decodeColorPatternText(regexList.at(i), textAnsiFg, textAnsiBg);

            if (textAnsiBg == scmIgnored && textAnsiFg == scmIgnored) {
                setError(QStringLiteral("<b><font color='blue'>%1</font></b>")
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

    mOK_init = state;
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
        int utf16_pos = toMatch.indexOf(QString(substring_start));
        std::string match;
        if (substring_length < 1) {
            captureList.push_back(match);
            posList.push_back(-1);
            continue;
        }

        match.append(substring_start, substring_length);
        captureList.push_back(match);
        posList.push_back(utf16_pos + posOffset);
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
            int utf16_pos = toMatch.indexOf(QString(substring_start));

            std::string match;
            if (substring_length < 1) {
                captureList.push_back(match);
                posList.push_back(-1);
                continue;
            }
            match.append(substring_start, substring_length);
            captureList.push_back(match);
            posList.push_back(utf16_pos + posOffset);
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
        if (Q_UNLIKELY(!pC)) {
            return true;
        }
        pC->deselect();
        auto its = captureList.begin();
        auto iti = posList.begin();
        for (int i = 1; iti != posList.end(); ++iti, ++its, i++) {
            int begin = *iti;
            std::string& s = *its;
            int length = QString::fromStdString(s).size();
            if (total > 1) {
                // skip complete match in Perl /g option type of triggers
                // to enable people to highlight capture groups if there are any
                // otherwise highlight complete expression match
                if (i % numberOfCaptureGroups != 1) {
                    pC->selectSection(begin, length);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
                    if (mBgColor != QColorConstants::Transparent) {
#else
                    if (mBgColor != QColor("transparent")) {
#endif
                        pC->setBgColor(r1, g1, b1);
                    }
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
                    if (mFgColor != QColorConstants::Transparent) {
#else
                    if (mFgColor != QColor("transparent")) {
#endif
                        pC->setFgColor(r2, g2, b2);
                    }
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
            if (Q_UNLIKELY(!pC)) {
                return true;
            }
            auto its = captureList.begin();
            for (auto iti = posList.begin(); iti != posList.end(); ++iti, ++its) {
                int begin = *iti;
                std::string& s = *its;
                int length = QString::fromStdString(s).size();
                pC->selectSection(begin, length);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
                if (mBgColor != QColorConstants::Transparent) {
#else
                if (mBgColor != QColor("transparent")) {
#endif
                    pC->setBgColor(r1, g1, b1);
                }
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
                if (mFgColor != QColorConstants::Transparent) {
#else
                if (mFgColor != QColor("transparent")) {
#endif
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

            // call lua trigger function with number of matches and matches itselves as arguments
            execute();
            pL->clearCaptureGroups();
            if (mFilterTrigger) {
                if (!captureList.empty()) {
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
    if (capture.empty()) {
        return;
    }
    auto * filterSubject = (char*)malloc(capture.size() + 2048);
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

int TTrigger::getExpiryCount() const
{
    return mExpiryCount;
}

void TTrigger::setExpiryCount(int expiryCount)
{
    mExpiryCount = expiryCount;
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
            if (Q_UNLIKELY(!pC)) {
                return true;
            }
            pC->deselect();
            auto its = captureList.begin();
            for (auto iti = posList.begin(); iti != posList.end(); ++iti, ++its) {
                int begin = *iti;
                std::string& s = *its;
                int length = QString::fromStdString(s).size();
                pC->selectSection(begin, length);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
                if (mBgColor != QColorConstants::Transparent) {
#else
                if (mBgColor != QColor("transparent")) {
#endif
                    pC->setBgColor(r1, g1, b1);
                }
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
                if (mFgColor != QColorConstants::Transparent) {
#else
                if (mFgColor != QColor("transparent")) {
#endif
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

            // call lua trigger function with number of matches and matches itselves as arguments
            execute();
            pL->clearCaptureGroups();
            if (mFilterTrigger) {
                if (!captureList.empty()) {
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
        if (mIsColorizerTrigger) {
            int r1 = mBgColor.red();
            int g1 = mBgColor.green();
            int b1 = mBgColor.blue();
            int r2 = mFgColor.red();
            int g2 = mFgColor.green();
            int b2 = mFgColor.blue();
            TConsole* pC = mpHost->mpConsole;
            if (Q_UNLIKELY(!pC)) {
                return true;
            }
            pC->deselect();
            auto its = captureList.begin();
            for (auto iti = posList.begin(); iti != posList.end(); ++iti, ++its) {
                int begin = *iti;
//                qDebug() << "TTrigger::match_color_pattern(" << line << "," << regexNumber << ") INFO - match found: " << (*its).c_str() << " size is:" << (*its).size();
                std::string& s = *its;
                int length = QString::fromStdString(s).size();
                pC->selectSection(begin, length);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
                if (mBgColor != QColorConstants::Transparent) {
#else
                if (mBgColor != QColor("transparent")) {
#endif
                    pC->setBgColor(r1, g1, b1);
                }
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
                if (mFgColor != QColorConstants::Transparent) {
#else
                if (mFgColor != QColor("transparent")) {
#endif
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
                    std::list<std::string> captureList;
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
            if (Q_UNLIKELY(!pC)) {
                return true;
            }
            auto its = captureList.begin();
            for (auto iti = posList.begin(); iti != posList.end(); ++iti, ++its) {
                int begin = *iti;
                std::string& s = *its;
                int length = QString::fromStdString(s).size();
                pC->selectSection(begin, length);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
                if (mBgColor != QColorConstants::Transparent) {
#else
                if (mBgColor != QColor("transparent")) {
#endif
                    pC->setBgColor(r1, g1, b1);
                }
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
                if (mFgColor != QColorConstants::Transparent) {
#else
                if (mFgColor != QColor("transparent")) {
#endif
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
            // call lua trigger function with number of matches and matches itselves as arguments
            execute();
            pL->clearCaptureGroups();
            if (mFilterTrigger) {
                if (!captureList.empty()) {
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
            std::list<TMatchState*> removeList;

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
                        if (!multiCaptureList.empty()) {
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
            if (conditionMet || (mRegexCodeList.empty())) {
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
            if ((mKeepFiring == mStayOpen) || (mpMyChildrenList->empty())) {
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

        if (conditionMet && mExpiryCount > -1) {
            mExpiryCount--;

            if (mExpiryCount == 0) {
                mpHost->getTriggerUnit()->markCleanup(this);

                if (mudlet::debugMode) {
                    TDebug(QColor(Qt::yellow), QColor(Qt::darkMagenta)) << tr("Trigger name=%1 expired.\n").arg(mName) >> 0;
                }
            } else if (mudlet::debugMode) {
                    TDebug(QColor(Qt::yellow), QColor(Qt::darkMagenta)) << tr("Trigger name=%1 will fire %n more time(s).\n", "", mExpiryCount).arg(mName) >> 0;
            }
        }

        return conditionMet;
    }
    return false;
}


// Die Musternummer wird ID im color-pattern lookup table
// This NOW uses proper ANSI numbers
// A TColorTable is a simple struct that stores four values, the two given ANSI
// colors for foreground and background (proper ANSI indexes) and what they look
// like give the current Host settings (the first 16 ANSI ones and the default
// fore and background colors can be changed by the user and since OSC P/R
// support has been implimented - by the MUD Server!)
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

    QColor fgColor = mpHost->getAnsiColor(ansiFg, false);
    QColor bgColor = mpHost->getAnsiColor(ansiBg, true);

    // If BOTH ansiFg AND ansiBg are scmIgnored then the color pattern is
    // totally unset
    if (!(fgColor.isValid() || bgColor.isValid())) {
        return nullptr;
    }

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

    mRegexCodeList << createColorPatternText(ansiFg, ansiBg);
    mRegexCodePropertyList << REGEX_COLOR_PATTERN;
    mColorPatternList.push_back(pCT);
    return true;
}

bool TTrigger::isFilterChain()
{
    return (!mRegexCodeList.empty()) && (hasChildren());
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
    mFuncName = QStringLiteral("Trigger%1").arg(QString::number(mID));
    QString code = QStringLiteral("function %1()\n%2\nend\n").arg(mFuncName, mScript);
    QString error;
    if (mpLua->compile(code, error, QStringLiteral("Trigger: %1").arg(getName()))) {
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
        fgText = QStringLiteral("%1").arg(fgColorCode, 3, 10, QLatin1Char('0'));
    }

    if (bgColorCode == scmIgnored) {
        bgText = QLatin1String("IGNORE");
    } else if (bgColorCode == scmDefault) {
        bgText = QLatin1String("DEFAULT");
    } else {
        bgText = QStringLiteral("%1").arg(bgColorCode, 3, 10, QLatin1Char('0'));
    }

    return QStringLiteral("ANSI_COLORS_F{%1}_B{%2}").arg(fgText, bgText);
}

void TTrigger::decodeColorPatternText(const QString& patternText, int& fgColorCode, int& bgColorCode)
{
    // The numbers used for the text have changed - see table in:
    // TColorTable* TTrigger::createColorPattern(int ansiFg, int ansiBg)
    QRegularExpression regex = QRegularExpression(QStringLiteral("^ANSI_COLORS_F{(\\d+|DEFAULT|IGNORE)}_B{(\\d+|DEFAULT|IGNORE)}$"));
    // Was QRegularExpression regex = QRegularExpression(QStringLiteral(R"(FG(\d+)BG(\d+))"));
    QRegularExpressionMatch match = regex.match(patternText);
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
