#ifndef MUDLET_TTRIGGER_H
#define MUDLET_TTRIGGER_H

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


#include "Tree.h"

#include "pre_guard.h"
#include <QApplication>
#include <QColor>
#include <QMap>
#include <QPointer>
#include <QSharedPointer>
#include "post_guard.h"

#include <pcre.h>

#include <map>
#include <string>

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

#define OVECCOUNT 30 // should be a multiple of 3

struct TColorTable
{
    int ansiFg;
    int ansiBg;
    int fgR;
    int fgG;
    int fgB;
    int bgR;
    int bgG;
    int bgB;
};

class TTrigger : public Tree<TTrigger>
{
    Q_DECLARE_TR_FUNCTIONS(TTrigger) // Needed so we can use tr() even though TTrigger is NOT derived from QObject
    friend class XMLexport;
    friend class XMLimport;

public:
    virtual ~TTrigger();
    TTrigger(TTrigger* parent, Host* pHost);
    TTrigger(const QString& name, QStringList regexList, QList<int> regexPorpertyList, bool isMultiline, Host* pHost); //throws exeption ExObjNoCreate
    QString getCommand() { return mCommand; }
    void compileAll();
    void setCommand(const QString& b) { mCommand = b; }
    QString getName() { return mName; }
    void setName(const QString& name);
    QStringList& getRegexCodeList() { return mRegexCodeList; }
    QList<int> getRegexCodePropertyList() { return mRegexCodePropertyList; }
    QColor getFgColor() { return mFgColor; }
    QColor getBgColor() { return mBgColor; }
    void setFgColor(QColor& c) { mFgColor = c; }
    void setBgColor(QColor& c) { mBgColor = c; }
    bool isColorizerTrigger() { return mIsColorizerTrigger; }
    void setIsColorizerTrigger(bool b) { mIsColorizerTrigger = b; }
    void compile();
    void execute();
    bool isFilterChain();
    bool setRegexCodeList(QStringList regex, QList<int> regexPorpertyList);
    QString getScript() { return mScript; }
    bool setScript(const QString& script);
    bool compileScript();
    bool match(char*, const QString&, int line, int posOffset = 0);

    bool isMultiline() { return mIsMultiline; }
    int getTriggerType() { return mTriggerType; }
    bool isLineTrigger() { return mIsLineTrigger; }
    void setIsLineTrigger(bool b) { mIsLineTrigger = b; }
    void setStartOfLineDelta(int b) { mStartOfLineDelta = b; }
    void setLineDelta(int b) { mLineDelta = b; }
    void setTriggerType(int b) { mTriggerType = b; }
    void setIsMultiline(bool b) { mIsMultiline = b; }
    void enableTrigger(const QString&);
    void disableTrigger(const QString&);
    TTrigger* killTrigger(const QString&);
    bool match_substring(const QString&, const QString&, int, int posOffset = 0);
    bool match_perl(char*, const QString&, int, int posOffset = 0);
    bool match_wildcard(const QString&, int);
    bool match_exact_match(const QString&, const QString&, int, int posOffset = 0);
    bool match_begin_of_line_substring(const QString& toMatch, const QString& regex, int regexNumber, int posOffset = 0);
    bool match_lua_code(int);
    bool match_line_spacer(int regexNumber);
    bool match_color_pattern(int, int);
    bool match_prompt(int patternNumber);
    void setConditionLineDelta(int delta) { mConditionLineDelta = delta; }
    int getConditionLineDelta() { return mConditionLineDelta; }
    bool registerTrigger();
    void setSound(const QString& file) { mSoundFile = file; }
    bool setupColorTrigger(int, int);
    bool setupTmpColorTrigger(int ansiFg, int ansiBg);
    TColorTable* createColorPattern(int, int);
    bool mTriggerContainsPerlRegex;
    bool mPerlSlashGOption;
    bool mFilterTrigger;
    bool mSoundTrigger;
    QString mSoundFile;
    int mStayOpen;
    bool mColorTrigger;
    QList<TColorTable*> mColorPatternList;
    bool mColorTriggerFg;
    bool mColorTriggerBg;
    QColor mColorTriggerFgColor;
    QColor mColorTriggerBgColor;
    int mColorTriggerFgAnsi;
    int mColorTriggerBgAnsi;
    int mKeepFiring;
    QPointer<Host> mpHost;
    QString mName;
    QStringList mRegexCodeList;
    bool exportItem;
    bool mModuleMasterFolder;
    // specifies whenever the payload is Lua code as a string
    // or a function
    bool mRegisteredAnonymousLuaFunction;

private:
    TTrigger() {}
    void updateMultistates(int regexNumber, std::list<std::string>& captureList, std::list<int>& posList);
    void filter(std::string&, int&);


    QList<int> mRegexCodePropertyList;
    QMap<int, QSharedPointer<pcre>> mRegexMap;

    // Lua code as a string to run
    QString mScript;

    bool mNeedsToBeCompiled;
    int mTriggerType;

    bool mIsLineTrigger;
    int mStartOfLineDelta;
    int mLineDelta;
    bool mIsMultiline;
    int mConditionLineDelta;
    QString mCommand;
    std::map<TMatchState*, TMatchState*> mConditionMap;
    std::list<std::list<std::string>> mMultiCaptureGroupList;
    std::list<std::list<int>> mMultiCaptureGroupPosList;
    TLuaInterpreter* mpLua;
    std::map<int, std::string> mLuaConditionMap;
    QString mFuncName;
    QColor mFgColor;
    QColor mBgColor;
    bool mIsColorizerTrigger;
    bool mModuleMember;
};

#endif // MUDLET_TTRIGGER_H
