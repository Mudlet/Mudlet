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
#include "../3rdparty/pugixml/src/pugiconfig.hpp"
#include "../3rdparty/pugixml/src/pugixml.hpp"
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

    bool writeHost(Host*, pugi::xml_node hostPackage);

    bool writeTrigger(TTrigger*, pugi::xml_node xmlParent);
    bool writeTimer(TTimer*, pugi::xml_node xmlParent);
    bool writeAlias(TAlias*, pugi::xml_node xmlParent);
    bool writeAction(TAction*, pugi::xml_node xmlParent);
    bool writeScript(TScript*, pugi::xml_node xmlParent);
    bool writeKey(TKey*, pugi::xml_node xmlParent);
    bool writeVariable(TVar*, LuaInterface*, VarUnit*, pugi::xml_node xmlParent);
    bool writeModuleXML(const QString& moduleName, const QString& fileName);

    bool exportHost(const QString& filename_pugi_xml);
    bool writeGenericPackage(Host* pHost, pugi::xml_node& mMudletPackage);
    bool exportGenericPackage(const QString& exportFileName);
    bool exportTrigger(const QString& fileName);
    bool exportTimer(const QString& fileName);
    bool exportAlias(const QString& fileName);
    bool exportAction(const QString& fileName);
    bool exportScript(const QString& fileName);
    bool exportKey(const QString& fileName);

    bool exportToClipboard(TTrigger*);
    bool exportToClipboard(TTimer*);
    bool exportToClipboard(TAlias*);
    bool exportToClipboard(TAction*);
    bool exportToClipboard(TScript*);
    bool exportToClipboard(TKey*);

    bool writeScriptElement(const QString&, pugi::xml_node xmlElement);

    QVector<QFuture<bool>> saveFutures;

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
    void writeTriggerPackage(const Host* pHost, pugi::xml_node& mMudletPackage, bool includeModuleMembers);
    void writeTimerPackage(const Host* pHost, pugi::xml_node& mMudletPackage, bool includeModuleMembers);
    void writeAliasPackage(const Host* pHost, pugi::xml_node& mMudletPackage, bool includeModuleMembers);
    void writeActionPackage(const Host* pHost, pugi::xml_node& mMudletPackage, bool includeModuleMembers);
    void writeScriptPackage(const Host* pHost, pugi::xml_node& mMudletPackage, bool includeModuleMembers);
    void writeKeyPackage(const Host* pHost, pugi::xml_node& mMudletPackage, bool includeModuleMembers);
    void writeVariablePackage(Host* pHost, pugi::xml_node& mMudletPackage);
    void inline replaceAll(std::string& source, const char from, const std::string& to);
    void inline replaceAll(std::string& source, const std::string& from, const std::string& to);
    bool saveXml(const QString& fileName);
    pugi::xml_node writeXmlHeader();
};

#endif // MUDLET_XMLEXPORT_H
