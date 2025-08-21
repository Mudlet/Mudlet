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

    QSharedPointer<pcre> re = mpRegex;
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
    int namecount = 0;
    char* tabptr = nullptr;
    NameGroupMatches nameGroups;
    QMap<QString, QPair<int, int>> namePositions;
    std::list<std::string> captureList;
    std::list<int> posList;
    int name_entry_size = 0;
    int haystackCLength = strlen(haystackC);
    int rc = 0;
    int i = 0;
    int ovector[MAX_CAPTURE_GROUPS * 3] = {0};

    if (mRegexCode.isEmpty()) {
        goto MUD_ERROR;
    }

    rc = pcre_exec(re.data(), nullptr, haystackC, haystackCLength, 0, 0, ovector, MAX_CAPTURE_GROUPS * 3);

    if (rc < 0) {
        goto MUD_ERROR;
    }

    if (rc == 0) {
        if (mpHost->mpEditorDialog) {
            mpHost->mpEditorDialog->mpErrorConsole->print(
                qsl("%1\n").arg(tr("[Alias Error:] %1 capture group limit exceeded, capture less groups.").arg(MAX_CAPTURE_GROUPS)),
                QColor(255, 128, 0),
                QColor(Qt::black));
        }
        qWarning() << "CRITICAL ERROR: SHOULD NOT HAPPEN pcre_info() got wrong number of capture groups ovector only has room for" << MAX_CAPTURE_GROUPS << "captured substrings";
    } else {
        if (mudlet::smDebugMode) {
            TDebug(Qt::cyan, Qt::black) << "Alias name=" << mName << "(" << mRegexCode << ") matched.\n" >> mpHost;
        }
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

    pcre_fullinfo(re.data(), nullptr, PCRE_INFO_NAMECOUNT, &namecount);

    if (namecount > 0) {
        pcre_fullinfo(re.data(), nullptr, PCRE_INFO_NAMETABLE, &tabptr);
        pcre_fullinfo(re.data(), nullptr, PCRE_INFO_NAMEENTRYSIZE, &name_entry_size);
        for (i = 0; i < namecount; ++i) {
            const int n = (tabptr[0] << 8) | tabptr[1];
            auto name = QString::fromUtf8(&tabptr[2]).trimmed();
            auto* substring_start = haystackC + ovector[2*n];
            auto substring_length = ovector[2*n+1] - ovector[2*n];
            auto utf16_pos = haystack.indexOf(QString::fromUtf8(substring_start, substring_length));
            auto capture = QString::fromUtf8(substring_start, substring_length);
            nameGroups << qMakePair(name, capture);
            tabptr += name_entry_size;
            namePositions.insert(name, qMakePair(utf16_pos, substring_length));
        }
    }

    for (;;) {
        int options = 0;
        int start_offset = ovector[1];

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
                mpHost->mpEditorDialog->mpErrorConsole->print(
                    qsl("%1\n").arg(tr("[Alias Error:] %1 capture group limit exceeded, capture less groups.").arg(MAX_CAPTURE_GROUPS)),
                    QColor(255, 128, 0),
                    QColor(Qt::black));
            }
            qWarning() << "CRITICAL ERROR: SHOULD NOT HAPPEN pcre_info() got wrong number of capture groups ovector only has room for" << MAX_CAPTURE_GROUPS << "captured substrings";
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

MUD_ERROR:
    for (auto childAlias : *mpMyChildrenList) {
        if (childAlias->match(haystack)) {
            matchCondition = true;
        }
    }

    free(haystackC);
    return matchCondition;
}

static void pcre_deleter(pcre* pointer)
{
    pcre_free(pointer);
}

void TAlias::setRegexCode(const QString& code)
{
    mRegexCode = code;
    compileRegex();
}

void TAlias::compileRegex()
{
    const char* error;
    int erroffset;

    // PCRE_UTF8 needed to run compile in UTF-8 mode
    // PCRE_UCP needed for \d, \w etc. to use Unicode properties:
    QSharedPointer<pcre> re(pcre_compile(mRegexCode.toUtf8().constData(), PCRE_UTF8 | PCRE_UCP, &error, &erroffset, nullptr), pcre_deleter);

    if (re == nullptr) {
        mOK_init = false;
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
