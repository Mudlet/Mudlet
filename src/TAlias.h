#ifndef MUDLET_TALIAS_H
#define MUDLET_TALIAS_H

/***************************************************************************
 *   Copyright (C) 2008-2012 by Heiko Koehn - KoehnHeiko@googlemail.com    *
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
#include <QPointer>
#include <QSharedPointer>
#include "post_guard.h"

#include <pcre.h>

class Host;


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
    QString getName() { return mName; }
    void setName(const QString& name);
    void compile();
    bool compileScript();
    void execute();
    QString getScript() { return mScript; }
    bool setScript(const QString& script);
    QString getRegexCode() { return mRegexCode; }
    void setRegexCode(const QString&);
    void setCommand(const QString& command) { mCommand = command; }
    QString getCommand() { return mCommand; }

    bool match(const QString& toMatch);
    bool registerAlias();

    TAlias() = default;

    QString mName;
    QString mCommand;
    QString mRegexCode;
    QSharedPointer<pcre> mpRegex;
    QString mScript;
    QPointer<Host> mpHost;
    bool mNeedsToBeCompiled;
    bool mModuleMember;
    bool mModuleMasterFolder;
    QString mFuncName;
    bool exportItem;
};

#endif // MUDLET_TALIAS_H
