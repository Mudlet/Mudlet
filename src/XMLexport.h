#ifndef MUDLET_XMLEXPORT_H
#define MUDLET_XMLEXPORT_H

/***************************************************************************
 *   Copyright (C) 2008-2011 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2017 by Stephen Lyons - slysven@virginmedia.com         *
 *   Copyright (C) 2017 by Ian Adkins - ieadkins@gmail.com                 *
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


#include "pre_guard.h"
#include <QApplication>
#include <QBuffer>
#include <QClipboard>
#include <QPointer>
#include <QXmlStreamWriter>
#include <QObject>
#include <QFuture>
#include "<pugiconfig>"
#include "<pugixml>"
#include "post_guard.h"

class Host;
class LuaInterface;
class TAction;
class TAlias;
class TKey;
class TScript;
class TTimer;
class TTrigger;
class TVar;
class VarUnit;


class XMLexport : public QObject
{
    Q_OBJECT

public:
    XMLexport(Host*);
    XMLexport(TTrigger*);
    XMLexport(TTimer*);
    XMLexport(TAlias*);
    XMLexport(TAction*);
    XMLexport(TScript*);
    XMLexport(TKey*);
    ~XMLexport();

    bool writeHost(Host *, pugi::xml_node hostPackage);

    bool writeTrigger(TTrigger *, pugi::xml_node xmlParent);
    bool writeTimer(TTimer *, pugi::xml_node xmlParent);
    bool writeAlias(TAlias *, pugi::xml_node xmlParent);
    bool writeAction(TAction *, pugi::xml_node xmlParent);
    bool writeScript(TScript *, pugi::xml_node xmlParent);
    bool writeKey(TKey *, pugi::xml_node xmlParent);
    bool writeVariable(TVar *, LuaInterface *, VarUnit *, pugi::xml_node xmlParent);
    bool writeModuleXML(const QString &moduleName, const QString &fileName);

    bool exportHost(const QString &filename_pugi_xml);
    bool exportGenericPackage(QIODevice* device);
    bool writeGenericPackage(Host*);
    bool exportTrigger(QIODevice*);
    bool exportTimer(QIODevice*);
    bool exportAlias(QIODevice*);
    bool exportAction(QIODevice*);
    bool exportScript(QIODevice*);
    bool exportKey(QIODevice*);

    bool exportToClipboard(TTrigger*);
    bool exportToClipboard(TTimer*);
    bool exportToClipboard(TAlias*);
    bool exportToClipboard(TAction*);
    bool exportToClipboard(TScript*);
    bool exportToClipboard(TKey*);

    bool writeScriptElement(const QString &, pugi::xml_node xmlElement);

    QFuture<bool> savingFuture;

signals:
    void saveCompleted();

private:
    QPointer<Host> mpHost;
    TTrigger* mpTrigger;
    TTimer* mpTimer;
    TAlias* mpAlias;
    TAction* mpAction;
    TScript* mpScript;
    TKey* mpKey;
    pugi::xml_document mExportDoc;

    void showXmlDebug();
    void writeTriggerPackage(const Host* pHost, pugi::xml_node& mMudletPackageNode, bool ignoreModuleMember);
    void writeTimerPackage(const Host* pHost, pugi::xml_node& mMudletPackageNode, bool ignoreModuleMember);
    void writeAliasPackage(const Host* pHost, pugi::xml_node& mMudletPackageNode, bool ignoreModuleMember);
    void writeActionPackage(const Host* pHost, pugi::xml_node& mMudletPackageNode, bool ignoreModuleMember);
    void writeScriptPackage(const Host* pHost, pugi::xml_node& mMudletPackageNode, bool ignoreModuleMember);
    void writeKeyPackage(const Host* pHost, pugi::xml_node& mMudletPackageNode, bool ignoreModuleMember);
    void writeVariablePackage(Host* pHost, pugi::xml_node& mMudletPackageNode);
    bool saveXml(const QString& fileName);
    void inline replaceAll(std::string& source, const char from, const std::string& to);
};

#endif // MUDLET_XMLEXPORT_H
