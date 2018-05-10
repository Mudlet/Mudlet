/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2017 by Stephen Lyons - slysven@virginmedia.com         *
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
#include "TDebug.h"
#include "mudlet.h"


using namespace std;

TAlias::TAlias(TAlias* parent, Host* pHost)
: Tree<TAlias>( parent )
, mpHost( pHost )
, mNeedsToBeCompiled( true )
, mModuleMember(false)
, mModuleMasterFolder(false)
, exportItem(true)
{
}

TAlias::TAlias(const QString& name, Host* pHost)
: Tree<TAlias>(nullptr)
, mName( name )
, mpHost( pHost )
, mNeedsToBeCompiled( true )
, mModuleMember(false)
, mModuleMasterFolder(false)
, exportItem(true)
{
}

TAlias::~TAlias()
{
    if (!mpHost) {
        return;
    }
    mpHost->getAliasUnit()->unregisterAlias(this);
}

void TAlias::setName(const QString& name)
{
    if (!isTemporary()) {
        mpHost->getAliasUnit()->mLookupTable.remove(mName, this);
    }
    mName = name;
    mpHost->getAliasUnit()->mLookupTable.insertMulti(name, this);
}

bool TAlias::match(const QString& toMatch)
{
    if (!isActive()) {
        if (isFolder()) {
            if (shouldBeActive()) {
                bool matchCondition = false;
                for (auto alias : *mpMyChildrenList) {
                    if (alias->match(toMatch)) {
                        matchCondition = true;
                    }
                }
                return matchCondition;
            }
        }
        return false;
    }

    bool matchCondition = false;
    //bool ret = false;
    //bool conditionMet = false;
    QSharedPointer<pcre> re = mpRegex;
    if (re == nullptr) {
        return false; //regex compile error
    }

#if defined(Q_OS_WIN32)
    // strndup(3) - a safe strdup(3) does not seem to be available on mingw32 with GCC-4.9.2
    char* subject = (char*)malloc(strlen(toMatch.toUtf8().constData()) + 1);
    strcpy(subject, toMatch.toUtf8().constData());
#else
    char* subject = strndup(toMatch.toUtf8().constData(), strlen(toMatch.toUtf8().constData()));
#endif
    unsigned char* name_table;
    int namecount;
    int name_entry_size;

    int subject_length = strlen(subject);
    int rc, i;
    std::list<std::string> captureList;
    std::list<int> posList;
    int ovector[300]; // 100 capture groups max (can be increase nbGroups=1/3 ovector

    //cout <<" LINE="<<subject<<endl;
    if (mRegexCode.size() > 0) {
        rc = pcre_exec(re.data(), nullptr, subject, subject_length, 0, 0, ovector, 100);
    } else {
        goto MUD_ERROR;
    }

    if (rc < 0) {
        goto MUD_ERROR;
    } else if (rc == 0) {
        qDebug() << "CRITICAL ERROR: SHOULD NOT HAPPEN->pcre_info() got wrong num of cap groups ovector only has room for %d captured substrings\n";
    } else {
        if (mudlet::debugMode) {
            TDebug(QColor(Qt::cyan), QColor(Qt::black)) << "Alias name=" << mName << "(" << mRegexCode << ") matched.\n" >> 0;
        }
    }

    matchCondition = true; // alias has matched

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
        posList.push_back(ovector[2 * i]);
        if (mudlet::debugMode) {
            TDebug(QColor(Qt::darkCyan), QColor(Qt::black)) << "Alias: capture group #" << (i + 1) << " = " >> 0;
            TDebug(QColor(Qt::darkMagenta), QColor(Qt::black)) << "<" << match.c_str() << ">\n" >> 0;
        }
    }
    pcre_fullinfo(re.data(), nullptr, PCRE_INFO_NAMECOUNT, &namecount);

    if (namecount <= 0) {
        //cout << "no named substrings detected" << endl;
    } else {
        unsigned char* tabptr;
        pcre_fullinfo(re.data(), nullptr, PCRE_INFO_NAMETABLE, &name_table);

        pcre_fullinfo(re.data(), nullptr, PCRE_INFO_NAMEENTRYSIZE, &name_entry_size);

        tabptr = name_table;
        for (i = 0; i < namecount; i++) {
            //int n = (tabptr[0] << 8) | tabptr[1];
            tabptr += name_entry_size;
        }
    }
    //TODO: add named groups seperately later as Lua::namedGroups
    for (;;) {
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
            posList.push_back(ovector[2 * i]);
            if (mudlet::debugMode) {
                TDebug(QColor(Qt::darkCyan), QColor(Qt::black)) << "capture group #" << (i + 1) << " = " >> 0;
                TDebug(QColor(Qt::darkMagenta), QColor(Qt::black)) << "<" << match.c_str() << ">\n" >> 0;
            }
        }
    }

END : {
        TLuaInterpreter* pL = mpHost->getLuaInterpreter();
        pL->setCaptureGroups(captureList, posList);
        // call lua trigger function with number of matches and matches itselves as arguments
        execute();
        pL->clearCaptureGroups();
    }

MUD_ERROR:
    for (auto childAlias : *mpMyChildrenList) {
        if (childAlias->match(toMatch)) {
            matchCondition = true;
        }
    }

    free(subject);
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
        if (mudlet::debugMode) {
            TDebug(QColor(Qt::white), QColor(Qt::red)) << "REGEX ERROR: failed to compile, reason:\n" << error << "\n" >> 0;
            TDebug(QColor(Qt::red), QColor(Qt::gray)) << R"(in: ")" << mRegexCode << "\"\n" >> 0;
        }
        setError(QStringLiteral("<b><font color='blue'>%1</font></b>").arg(tr(R"(Error: in "Pattern:", faulty regular expression, reason: "%1".)", error)));
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
        if (mudlet::debugMode) {
            TDebug(QColor(Qt::white), QColor(Qt::red)) << "ERROR: Lua compile error. compiling script of alias:" << mName << "\n" >> 0;
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
            if (mudlet::debugMode) {
                TDebug(QColor(Qt::white), QColor(Qt::red)) << "ERROR: Lua compile error. compiling script of alias:" << mName << "\n" >> 0;
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
    QString code = QStringLiteral("function Alias%1() %2\nend").arg(QString::number(mID), mScript);
    QString aliasName = QStringLiteral("Alias: %1").arg(getName());
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
    if (mCommand.size() > 0) {
        mpHost->send(mCommand);
    }
    if (mNeedsToBeCompiled) {
        if (!compileScript()) {
            return;
        }
    }
    mpHost->mLuaInterpreter.call(mFuncName, mName);
}
