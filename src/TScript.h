#ifndef MUDLET_TSCRIPT_H
#define MUDLET_TSCRIPT_H

/***************************************************************************
 *   Copyright (C) 2008-2012 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
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

#include <QDebug>
#include <QPointer>
#include <QStringList>
#include <optional>

class Host;
class TEvent;


class TScript : public Tree<TScript>
{
    friend class XMLexport;
    friend class XMLimport;
    friend class DeleteItemCommand;
    friend class MudletDeleteItemCommand;

public:
    virtual ~TScript();
    TScript(TScript* parent, Host* pHost);
    TScript(const QString& name, Host* pHost);

    QString getName() const { return mName; }
    void setName(const QString& name) { mName = name; }
    void compile(bool saveLoadingError = false);
    void compileAll(bool saveLoadingError = false);
    bool compileScript(bool saveLoadingError = false);
    void execute();
    QString getScript() const { return mScript; }
    bool setScript(const QString& script);
    bool registerScript();
    void callEventHandler(const TEvent&);
    void setEventHandlerList(QStringList handlerList);
    QStringList getEventHandlerList() const { return mEventHandlerList; }
    std::optional<QString> getLoadingError();
    void setLoadingError(const QString& error);
    void clearLoadingError();
    QString packageName(TScript* pScript);
    QString moduleName(TScript* pScript);
    bool checkIfNew();
    void unmarkAsNew();

    bool exportItem = true;
    bool mModuleMasterFolder = false;
    bool mIsNew = true;

private:
    TScript() = default;
    QString mName;
    QString mScript;
    QString mFuncName;
    bool mNeedsToBeCompiled = true;
    QStringList mEventHandlerList;
    QPointer<Host> mpHost;
    bool mModuleMember = false;
    std::optional<QString> mLoadingError;
};

#ifndef QT_NO_DEBUG_STREAM
// Note "inline" is REQUIRED:
inline QDebug& operator<<(QDebug& debug, const TScript* script)
{
    QDebugStateSaver saver(debug);
    Q_UNUSED(saver)
    debug.nospace() << "TScript(" << script->getName() << ")";
    debug.nospace() << ", script=" << script->getScript();
    debug.nospace() << ", event handlers=" << script->getEventHandlerList();
    debug.nospace() << ')';
    return debug;
}
#endif // QT_NO_DEBUG_STREAM

#endif // MUDLET_TSCRIPT_H
