/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2017, 2021-2022 by Stephen Lyons                        *
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


#include "TAlias.h"


#include "Host.h"
#include "TConsole.h"
#include "TDebug.h"
#include "mudlet.h"

TAlias::TAlias(TAlias* parent, Host* pHost)
: Tree<TAlias>( parent )
, mpHost(pHost)
{
}

TAlias::TAlias(const QString& name, Host* pHost)
: Tree<TAlias>(nullptr)
, mName(name)
, mpHost(pHost)
{
}

TAlias::~TAlias()
{
    if (!mpHost) {
        return;
    }
    mpHost->getAliasUnit()->unregisterAlias(this);

    if (isTemporary()) {
        if (mScript.isEmpty()) {
            mpHost->mLuaInterpreter.delete_luafunction(this);
        } else {
            mpHost->mLuaInterpreter.delete_luafunction(mFuncName);
        }
    }
}

void TAlias::setName(const QString& name)
{
    if (!isTemporary()) {
        mpHost->getAliasUnit()->mLookupTable.remove(mName, this);
    }
    mName = name;
    mpHost->getAliasUnit()->mLookupTable.insert(name, this);
}

bool TAlias::match(const QString& haystack)
{
    bool matchCondition = false;
    if (!isActive()) {
        if (isFolder()) {
            if (shouldBeActive()) {
                for (auto alias : *mpMyChildrenList) {
                    if (alias->match(haystack)) {
                        matchCondition = true;
                    }
                }
                return matchCondition;
            }
        }
        return false;
    }

    QSharedPointer<pcre2_code> re = mpRegex;
    if (re == nullptr) {
        return false; //regex compile error
    }

#if defined(Q_OS_WINDOWS)
    // strndup(3) - a safe strdup(3) does not seem to be available in the
    // original Mingw or the replacement Mingw-w64 environment we use:
    char* haystackC = static_cast<char*>(malloc(strlen(haystack.toUtf8().constData()) + 1));
    strcpy(haystackC, haystack.toUtf8().constData());
#else
    char* haystackC = strndup(haystack.toUtf8().constData(), strlen(haystack.toUtf8().constData()));
#endif

    // These must be initialised before any goto so the latter does not jump
    // over them:
    uint32_t namecount = 0;
    PCRE2_SPTR tabptr = nullptr;
    NameGroupMatches nameGroups;
    QMap<QString, QPair<int, int>> namePositions;
    std::list<std::string> captureList;
    std::list<int> posList;
    uint32_t name_entry_size = 0;
    int haystackCLength = strlen(haystackC);
    int rc = 0;
    int i = 0;
    pcre2_match_data* match_data = nullptr;
    PCRE2_SIZE* ovector = nullptr;

    if (mRegexCode.isEmpty()) {
        goto MUD_ERROR;
    }

    match_data = pcre2_match_data_create_from_pattern(re.data(), nullptr);
    if (!match_data) {
        goto MUD_ERROR;
    }

    rc = pcre2_match(re.data(), reinterpret_cast<PCRE2_SPTR>(haystackC), haystackCLength, 0, 0, match_data, nullptr);

    if (rc < 0) {
        pcre2_match_data_free(match_data);
        goto MUD_ERROR;
    }

    ovector = pcre2_get_ovector_pointer(match_data);

    if (mudlet::smDebugMode) {
        TDebug(Qt::cyan, Qt::black) << "Alias name=" << mName << "(" << mRegexCode << ") matched.\n" >> mpHost;
    }

    matchCondition = true; // alias has matched

    for (i = 0; i < rc; i++) {
        char* substring_start = haystackC + ovector[2 * i];
        int substring_length = ovector[2 * i + 1] - ovector[2 * i];

        std::string match;
        if (substring_length < 1) {
            captureList.push_back(match);
            posList.push_back(-1);
            continue;
        }
        match.append(substring_start, substring_length);
        captureList.push_back(match);
        posList.push_back(ovector[2 * i]);
        if (mudlet::smDebugMode) {
            TDebug(Qt::darkCyan, Qt::black) << "Alias: capture group #" << (i + 1) << " = " >> mpHost;
            TDebug(Qt::darkMagenta, Qt::black) << TDebug::csmContinue << "<" << match.c_str() << ">\n" >> mpHost;
        }
    }

    pcre2_pattern_info(re.data(), PCRE2_INFO_NAMECOUNT, &namecount);

    if (namecount > 0) {
        pcre2_pattern_info(re.data(), PCRE2_INFO_NAMETABLE, &tabptr);
        pcre2_pattern_info(re.data(), PCRE2_INFO_NAMEENTRYSIZE, &name_entry_size);
        for (uint32_t j = 0; j < namecount; ++j) {
            const int n = (tabptr[0] << 8) | tabptr[1];
            auto name = QString::fromUtf8(reinterpret_cast<const char*>(&tabptr[2])).trimmed();
            auto* substring_start = haystackC + ovector[2*n];
            auto substring_length = ovector[2*n+1] - ovector[2*n];
            auto utf16_pos = haystack.indexOf(QString::fromUtf8(substring_start, substring_length));
            auto capture = QString::fromUtf8(substring_start, substring_length);
            nameGroups << qMakePair(name, capture);
            tabptr += name_entry_size;
            namePositions.insert(name, qMakePair(utf16_pos, static_cast<int>(substring_length)));
        }
    }

    for (;;) {
        uint32_t options = 0;
        PCRE2_SIZE start_offset = ovector[1];

        if (ovector[0] == ovector[1]) {
            if (ovector[0] >= static_cast<PCRE2_SIZE>(haystackCLength)) {
                goto END;
            }
            options = PCRE2_NOTEMPTY_ATSTART | PCRE2_ANCHORED;
        }

        rc = pcre2_match(re.data(), reinterpret_cast<PCRE2_SPTR>(haystackC), haystackCLength, start_offset, options, match_data, nullptr);
        if (rc == PCRE2_ERROR_NOMATCH) {
            if (options == 0) {
                break;
            }
            ovector[1] = start_offset + 1;
            continue;
        } else if (rc < 0) {
            goto END;
        }

        for (i = 0; i < rc; i++) {
            char* substring_start = haystackC + ovector[2 * i];
            int substring_length = ovector[2 * i + 1] - ovector[2 * i];
            std::string match;
            if (substring_length < 1) {
                captureList.push_back(match);
                posList.push_back(-1);
                continue;
            }
            match.append(substring_start, substring_length);
            captureList.push_back(match);
            posList.push_back(ovector[2 * i]);
            if (mudlet::smDebugMode) {
                TDebug(Qt::darkCyan, Qt::black) << "capture group #" << (i + 1) << " = " >> mpHost;
                TDebug(Qt::darkMagenta, Qt::black) << TDebug::csmContinue << "<" << match.c_str() << ">\n" >> mpHost;
            }
        }
    }

END : {
        TLuaInterpreter* pL = mpHost->getLuaInterpreter();
        pL->setCaptureGroups(captureList, posList);
        pL->setCaptureNameGroups(nameGroups, namePositions);
        // call lua trigger function with number of matches and matches itselves as arguments
        execute();
        pL->clearCaptureGroups();
    }

    if (match_data) {
        pcre2_match_data_free(match_data);
    }

MUD_ERROR:
    for (auto childAlias : *mpMyChildrenList) {
        if (childAlias->match(haystack)) {
            matchCondition = true;
        }
    }

    free(haystackC);
    return matchCondition;
}

static void pcre2_code_deleter(pcre2_code* pointer)
{
    pcre2_code_free(pointer);
}

void TAlias::setRegexCode(const QString& code)
{
    mRegexCode = code;
    compileRegex();
}

void TAlias::compileRegex()
{
    int errorcode;
    PCRE2_SIZE erroffset;

    // PCRE2_UTF needed to run compile in UTF-8 mode
    // PCRE2_UCP needed for \d, \w etc. to use Unicode properties:
    QSharedPointer<pcre2_code> re(pcre2_compile(
        reinterpret_cast<PCRE2_SPTR>(mRegexCode.toUtf8().constData()),
        PCRE2_ZERO_TERMINATED,
        PCRE2_UTF | PCRE2_UCP,
        &errorcode,
        &erroffset,
        nullptr), pcre2_code_deleter);

    if (re == nullptr) {
        mOK_init = false;
        PCRE2_UCHAR errorBuffer[256];
        pcre2_get_error_message(errorcode, errorBuffer, sizeof(errorBuffer));
        const char* error = reinterpret_cast<const char*>(errorBuffer);
        if (mudlet::smDebugMode) {
            TDebug(Qt::white, Qt::red) << "REGEX ERROR: failed to compile, reason:\n" << error << "\n" >> mpHost;
            TDebug(Qt::red, Qt::gray) << TDebug::csmContinue << R"(in: ")" << mRegexCode << "\"\n" >> mpHost;
        }
        setError(qsl("<b><font color='blue'>%1</font></b>").arg(tr(R"(Error: in "Pattern:", faulty regular expression, reason: "%1".)").arg(error)));
    } else {
        mOK_init = true;
    }

    mpRegex = re;
}

bool TAlias::registerAlias()
{
    if (!mpHost) {
        qDebug() << "ERROR: TAlias::registerTrigger() pHost=0";
        return false;
    }
    return mpHost->getAliasUnit()->registerAlias(this);
}

void TAlias::compileAll()
{
    mNeedsToBeCompiled = true;
    if (!compileScript()) {
        if (mudlet::smDebugMode) {
            TDebug(Qt::white, Qt::red) << "ERROR: Lua compile error. compiling script of alias:" << mName << "\n" >> mpHost;
        }
        mOK_code = false;
    }
    compileRegex(); // Effectively will repost the error if there was a problem in the regex
    for (auto alias : *mpMyChildrenList) {
        alias->compileAll();
    }
}

void TAlias::compile()
{
    if (mNeedsToBeCompiled) {
        if (!compileScript()) {
            if (mudlet::smDebugMode) {
                TDebug(Qt::white, Qt::red) << "ERROR: Lua compile error. compiling script of alias:" << mName << "\n" >> mpHost;
            }
            mOK_code = false;
        }
    }
    for (auto alias : *mpMyChildrenList) {
        alias->compile();
    }
}

bool TAlias::setScript(const QString& script)
{
    mScript = script;
    mNeedsToBeCompiled = true;
    mOK_code = compileScript();
    return mOK_code;
}

bool TAlias::compileScript()
{
    QString code = qsl("function Alias%1() %2\nend").arg(QString::number(mID), mScript);
    QString aliasName = qsl("Alias: %1").arg(getName());
    mFuncName = qsl("Alias%1").arg(QString::number(mID));
    QString error;

    if (mpHost->mLuaInterpreter.compile(code, error, aliasName)) {
        mNeedsToBeCompiled = false;
        mOK_code = true;
        return true;
    } else {
        mOK_code = false;
        setError(error);
        return false;
    }
}

void TAlias::execute()
{
    if (!mCommand.isEmpty()) {
        mpHost->send(mCommand);
    }
    if (mNeedsToBeCompiled) {
        if (!compileScript()) {
            return;
        }
    }

    if (mRegisteredAnonymousLuaFunction) {
        mpHost->mLuaInterpreter.call_luafunction(this);
        return;
    }

    if (mScript.isEmpty()) {
        return;
    }

    mpHost->mLuaInterpreter.call(mFuncName, mName);
}

QString TAlias::packageName(TAlias* pAlias)
{
    if (!pAlias) {
        return QString();
    }

    if (!pAlias->mPackageName.isEmpty()) {
        return !mpHost->mModuleInfo.contains(pAlias->mPackageName) ? pAlias->mPackageName : QString();
    }

    if (pAlias->getParent()) {
        return packageName(pAlias->getParent());
    }

    return QString();
}

QString TAlias::moduleName(TAlias* pAlias)
{
    if (!pAlias) {
        return QString();
    }

    if (!pAlias->mPackageName.isEmpty()) {
        return mpHost->mModuleInfo.contains(pAlias->mPackageName) ? pAlias->mPackageName : QString();
    }

    if (pAlias->getParent()) {
        return moduleName(pAlias->getParent());
    }

    return QString();
}

bool TAlias::checkIfNew()
{
    return mIsNew;
}

void TAlias::unmarkAsNew()
{
    mIsNew = false;
}
