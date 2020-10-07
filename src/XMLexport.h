#ifndef MUDLET_XMLEXPORT_H
#define MUDLET_XMLEXPORT_H

/***************************************************************************
 *   Copyright (C) 2008-2011 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2017-2019 by Stephen Lyons - slysven@virginmedia.com    *
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
#include <QClipboard>
#include <QPointer>
#include <QFuture>
#include <pugixml.hpp>
#include "post_guard.h"

class QFile;
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

    void writeHost(Host*, pugi::xml_node hostPackage);
    void writeTrigger(TTrigger*, pugi::xml_node xmlParent);
    void writeTimer(TTimer*, pugi::xml_node xmlParent);
    void writeAlias(TAlias*, pugi::xml_node xmlParent);
    void writeAction(TAction*, pugi::xml_node xmlParent);
    void writeScript(TScript*, pugi::xml_node xmlParent);
    void writeKey(TKey*, pugi::xml_node xmlParent);
    void writeVariable(TVar*, LuaInterface*, VarUnit*, pugi::xml_node xmlParent);
    void writeModuleXML(const QString& moduleName, const QString& fileName);

    void exportHost(const QString& filename_pugi_xml);
    bool writeGenericPackage(Host* pHost, pugi::xml_node& mMudletPackage);
    bool exportProfile(const QString& exportFileName);
    bool exportPackage(const QString &exportFileName);
    bool exportTrigger(const QString& fileName);
    bool exportTimer(const QString& fileName);
    bool exportAlias(const QString& fileName);
    bool exportAction(const QString& fileName);
    bool exportScript(const QString& fileName);
    bool exportKey(const QString& fileName);

    void exportToClipboard(TTrigger*);
    void exportToClipboard(TTimer*);
    void exportToClipboard(TAlias*);
    void exportToClipboard(TAction*);
    void exportToClipboard(TScript*);
    void exportToClipboard(TKey*);

    void writeScriptElement(const QString&, pugi::xml_node xmlElement);

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

    void writeTriggerPackage(const Host* pHost, pugi::xml_node& mMudletPackage, bool skipModuleMembers);
    void writeTimerPackage(const Host* pHost, pugi::xml_node& mMudletPackage, bool skipModuleMembers);
    void writeAliasPackage(const Host* pHost, pugi::xml_node& mMudletPackage, bool skipModuleMembers);
    void writeActionPackage(const Host* pHost, pugi::xml_node& mMudletPackage, bool skipModuleMembers);
    void writeScriptPackage(const Host* pHost, pugi::xml_node& mMudletPackage, bool skipModuleMembers);
    void writeKeyPackage(const Host* pHost, pugi::xml_node& mMudletPackage, bool skipModuleMembers);
    void writeVariablePackage(Host* pHost, pugi::xml_node& mMudletPackage);
    void inline replaceAll(std::string& source, const std::string& from, const std::string& to);
    bool saveXmlFile(QFile&);
    bool saveXml(const QString&);
    pugi::xml_node writeXmlHeader();
    void sanitizeForQxml(std::string& output);
    QString saveXml();
    QStringList remapAnsiToColorNumber(const QStringList&, const QList<int>&);
};

#endif // MUDLET_XMLEXPORT_H
