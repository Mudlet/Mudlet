#ifndef MUDLET_TALIAS_H
#define MUDLET_TALIAS_H

/***************************************************************************
 *   Copyright (C) 2008-2012 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2017-2022 by Stephen Lyons - slysven@virginmedia.com    *
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
#include <QDebug>
#include <QPointer>
#include <QSharedPointer>
#include "post_guard.h"

#include <pcre.h>

class Host;

#define MAX_CAPTURE_GROUPS 33

using NameGroupMatches = QVector<QPair<QString, QString>>;

class TAlias : public Tree<TAlias>
{
    Q_DECLARE_TR_FUNCTIONS(TAlias) // Needed so we can use tr() even though TAlias is NOT derived from QObject
    friend class XMLexport;
    friend class XMLimport;

public:
    virtual ~TAlias();
    TAlias(TAlias* parent, Host* pHost);
    TAlias(const QString& name, Host* pHost);
    void compileAll();
    void compileRegex();
    QString getName() const { return mName; }
    void setName(const QString& name);
    void compile();
    bool compileScript();
    void execute();
    QString getScript() const { return mScript; }
    bool setScript(const QString& script);
    QString getRegexCode() const { return mRegexCode; }
    void setRegexCode(const QString&);
    void setCommand(const QString& command) { mCommand = command; }
    QString getCommand() const { return mCommand; }

    bool match(const QString& toMatch);
    bool registerAlias();

    TAlias() = default;

    QString mName;
    QString mCommand;
    QString mRegexCode;
    QSharedPointer<pcre> mpRegex;
    QString mScript;
    QPointer<Host> mpHost;
    bool mModuleMember = false;
    bool mModuleMasterFolder = false;
    QString mFuncName;
    bool exportItem = true;
    bool mRegisteredAnonymousLuaFunction = false;
    QVector<NameGroupMatches> nameCaptures;

private:
    bool mNeedsToBeCompiled = true;
};

#ifndef QT_NO_DEBUG_STREAM
inline QDebug& operator<<(QDebug& debug, const TAlias* alias)
{
    QDebugStateSaver saver(debug);
    Q_UNUSED(saver);

    if (!alias) {
        return debug << "TAlias(0x0) ";
    }
    debug.nospace() << "TAlias(" << alias->getName() << ")";
    debug.nospace() << ", command=" << alias->getCommand();
    debug.nospace() << ", regexCode=" << alias->getRegexCode();
    debug.nospace() << ", funcName=" << alias->mFuncName;
    debug.nospace() << ", script is in: " << (alias->mRegisteredAnonymousLuaFunction ? "string": "Lua function");
    debug.nospace() << ", script=" << alias->getScript();
    debug.nospace() << ", registeredAnonymousLuaFunction=" << alias->mRegisteredAnonymousLuaFunction;
    debug.nospace() << ')';
    return debug;
}
#endif // QT_NO_DEBUG_STREAM

#endif // MUDLET_TALIAS_H
