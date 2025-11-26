/***************************************************************************
 *   Copyright (C) 2025 by Vadim Peretokin - vadim.peretokin@mudlet.org    *
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

#include "EditorItemXMLHelpers.h"

#include "Host.h"
#include "TTrigger.h"
#include "TAlias.h"
#include "TTimer.h"
#include "TScript.h"
#include "TKey.h"
#include "TAction.h"
#include "TriggerUnit.h"
#include "AliasUnit.h"
#include "TimerUnit.h"
#include "ScriptUnit.h"
#include "KeyUnit.h"
#include "ActionUnit.h"
#include "XMLexport.h"
#include "XMLimport.h"
#include "utils.h"

#include <QDebug>
#include <sstream>

// =============================================================================
// Helper functions for XML serialization and compression
// =============================================================================

// Internal helper functions for compression (static to keep them local to this file)
static QString compressXML(const QString& xml) {
    if (xml.isEmpty()) {
        return QString();
    }

    QByteArray compressed = qCompress(xml.toUtf8(), 9);
    return QString::fromLatin1(compressed.toBase64());
}

static QString decompressXML(const QString& data) {
    if (data.isEmpty()) {
        return QString();
    }

    if (data.startsWith('<')) {
        return data;
    }

    QByteArray compressed = QByteArray::fromBase64(data.toLatin1());
    QByteArray decompressed = qUncompress(compressed);
    if (decompressed.isEmpty()) {
        qWarning() << "EditorUndoSystem: Failed to decompress XML data";
        return QString();
    }

    return QString::fromUtf8(decompressed);
}

// XML Export/Import functions - used by both EditorUndoSystem and EditorAddItemCommand
QString exportTriggerToXML(TTrigger* trigger) {
    if (!trigger) {
        return QString();
    }

    pugi::xml_document doc;
    auto root = doc.append_child("TriggerSnapshot");

    XMLexport exporter(trigger);
    exporter.writeTrigger(trigger, root);

    std::ostringstream oss;
    doc.save(oss);
    return compressXML(QString::fromStdString(oss.str()));
}

QString exportAliasToXML(TAlias* alias) {
    if (!alias) {
        return QString();
    }

    pugi::xml_document doc;
    auto root = doc.append_child("AliasSnapshot");

    XMLexport exporter(alias);
    exporter.writeAlias(alias, root);

    std::ostringstream oss;
    doc.save(oss);
    return compressXML(QString::fromStdString(oss.str()));
}

QString exportTimerToXML(TTimer* timer) {
    if (!timer) {
        return QString();
    }

    pugi::xml_document doc;
    auto root = doc.append_child("TimerSnapshot");

    XMLexport exporter(timer);
    exporter.writeTimer(timer, root);

    std::ostringstream oss;
    doc.save(oss);
    return compressXML(QString::fromStdString(oss.str()));
}

QString exportScriptToXML(TScript* script) {
    if (!script) {
        return QString();
    }

    pugi::xml_document doc;
    auto root = doc.append_child("ScriptSnapshot");

    XMLexport exporter(script);
    exporter.writeScript(script, root);

    std::ostringstream oss;
    doc.save(oss);
    return compressXML(QString::fromStdString(oss.str()));
}

QString exportKeyToXML(TKey* key) {
    if (!key) {
        return QString();
    }

    pugi::xml_document doc;
    auto root = doc.append_child("KeySnapshot");

    XMLexport exporter(key);
    exporter.writeKey(key, root);

    std::ostringstream oss;
    doc.save(oss);
    return compressXML(QString::fromStdString(oss.str()));
}

QString exportActionToXML(TAction* action) {
    if (!action) {
        return QString();
    }

    pugi::xml_document doc;
    auto root = doc.append_child("ActionSnapshot");

    XMLexport exporter(action);
    exporter.writeAction(action, root);

    std::ostringstream oss;
    doc.save(oss);
    return compressXML(QString::fromStdString(oss.str()));
}

QString getItemName(EditorViewType viewType, int itemID, Host* host) {
    switch (viewType) {
    case EditorViewType::cmTriggerView: {
        TTrigger* trigger = host->getTriggerUnit()->getTrigger(itemID);
        return trigger ? trigger->getName() : QString();
    }
    case EditorViewType::cmAliasView: {
        TAlias* alias = host->getAliasUnit()->getAlias(itemID);
        return alias ? alias->getName() : QString();
    }
    case EditorViewType::cmTimerView: {
        TTimer* timer = host->getTimerUnit()->getTimer(itemID);
        return timer ? timer->getName() : QString();
    }
    case EditorViewType::cmScriptView: {
        TScript* script = host->getScriptUnit()->getScript(itemID);
        return script ? script->getName() : QString();
    }
    case EditorViewType::cmKeysView: {
        TKey* key = host->getKeyUnit()->getKey(itemID);
        return key ? key->getName() : QString();
    }
    case EditorViewType::cmActionView: {
        TAction* action = host->getActionUnit()->getAction(itemID);
        return action ? action->getName() : QString();
    }
    default:
        return QString();
    }
}

QString getViewTypeName(EditorViewType viewType) {
    switch (viewType) {
    case EditorViewType::cmTriggerView:
        //: Display name for trigger items in editor
        return QObject::tr("Trigger");
    case EditorViewType::cmAliasView:
        //: Display name for alias items in editor
        return QObject::tr("Alias");
    case EditorViewType::cmTimerView:
        //: Display name for timer items in editor
        return QObject::tr("Timer");
    case EditorViewType::cmScriptView:
        //: Display name for script items in editor
        return QObject::tr("Script");
    case EditorViewType::cmKeysView:
        //: Display name for key binding items in editor
        return QObject::tr("Key");
    case EditorViewType::cmActionView:
        //: Display name for action/button items in editor
        return QObject::tr("Action");
    default:
        //: Display name for unknown items in editor
        return QObject::tr("Item");
    }
}

TTrigger* importTriggerFromXML(const QString& xmlSnapshot, TTrigger* pParent, Host* host, int position) {
    if (xmlSnapshot.isEmpty() || !host) {
        return nullptr;
    }

    QString xml = decompressXML(xmlSnapshot);
    if (xml.isEmpty()) {
        qWarning() << "importTriggerFromXML: Failed to decompress XML";
        return nullptr;
    }

    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_string(xml.toStdString().c_str());
    if (!result) {
        qWarning() << "importTriggerFromXML: Failed to parse XML:" << result.description();
        return nullptr;
    }

    auto root = doc.child("TriggerSnapshot");
    if (!root) {
        qWarning() << "importTriggerFromXML: No TriggerSnapshot root element found";
        return nullptr;
    }

    auto triggerNode = root.child("Trigger");
    if (!triggerNode) {
        triggerNode = root.child("TriggerGroup");
    }
    if (!triggerNode) {
        qWarning() << "importTriggerFromXML: No Trigger/TriggerGroup element found";
        return nullptr;
    }

    auto pT = new TTrigger(nullptr, host);

    if (pParent) {
        pT->setParent(pParent);
        // Use explicit enum mode for clarity
        pParent->addChild(pT, (position >= 0) ? TreeItemInsertMode::AtPosition : TreeItemInsertMode::Append, position);
        host->getTriggerUnit()->registerTrigger(pT); // This will call addTrigger() since pT has a parent
    } else {
        // Root level trigger - register first (adds to end of root list)
        host->getTriggerUnit()->registerTrigger(pT);

        auto rootListSize = host->getTriggerUnit()->getTriggerRootNodeList().size();

        if (position != -1 && position < rootListSize) {
            // Use reParentTrigger with explicit AtPosition mode to insert at specific position
            host->getTriggerUnit()->reParentTrigger(pT->getID(), -1, -1, TreeItemInsertMode::AtPosition, position);
        }
    }

    pT->setIsActive(QString::fromStdString(triggerNode.attribute("isActive").value()) == "yes");
    pT->setIsFolder(QString::fromStdString(triggerNode.attribute("isFolder").value()) == "yes");
    pT->setTemporary(QString::fromStdString(triggerNode.attribute("isTempTrigger").value()) == "yes");
    pT->setIsMultiline(QString::fromStdString(triggerNode.attribute("isMultiline").value()) == "yes");
    pT->mPerlSlashGOption = QString::fromStdString(triggerNode.attribute("isPerlSlashGOption").value()) == "yes";
    pT->setIsColorizerTrigger(QString::fromStdString(triggerNode.attribute("isColorizerTrigger").value()) == "yes");
    pT->mFilterTrigger = QString::fromStdString(triggerNode.attribute("isFilterTrigger").value()) == "yes";
    pT->mSoundTrigger = QString::fromStdString(triggerNode.attribute("isSoundTrigger").value()) == "yes";
    pT->mColorTrigger = QString::fromStdString(triggerNode.attribute("isColorTrigger").value()) == "yes";

    QStringList patterns;
    QList<int> patternKinds;

    for (auto node : triggerNode.children()) {
        QString nodeName = QString::fromStdString(node.name());
        QString nodeValue = QString::fromStdString(node.child_value());

        if (nodeName == "name") {
            pT->setName(nodeValue);
        } else if (nodeName == "script") {
            pT->setScript(nodeValue);
        } else if (nodeName == "packageName") {
            pT->mPackageName = nodeValue;
        } else if (nodeName == "triggerType") {
            pT->setTriggerType(nodeValue.toInt());
        } else if (nodeName == "conditonLineDelta") {
            pT->setConditionLineDelta(nodeValue.toInt());
        } else if (nodeName == "mStayOpen") {
            pT->mStayOpen = nodeValue.toInt();
        } else if (nodeName == "mCommand") {
            pT->setCommand(nodeValue);
        } else if (nodeName == "mFgColor") {
            pT->setColorizerFgColor(QColor::fromString(nodeValue));
        } else if (nodeName == "mBgColor") {
            pT->setColorizerBgColor(QColor::fromString(nodeValue));
        } else if (nodeName == "colorTriggerFgColor") {
            pT->mColorTriggerFgColor = QColor::fromString(nodeValue);
        } else if (nodeName == "colorTriggerBgColor") {
            pT->mColorTriggerBgColor = QColor::fromString(nodeValue);
        } else if (nodeName == "mSoundFile") {
            pT->setSound(nodeValue);
        } else if (nodeName == "regexCodeList") {
            for (auto patternNode : node.children("string")) {
                patterns << QString::fromStdString(patternNode.child_value());
            }
        } else if (nodeName == "regexCodePropertyList") {
            for (auto propertyNode : node.children("integer")) {
                patternKinds << QString::fromStdString(propertyNode.child_value()).toInt();
            }
        }
    }

    if (!patterns.isEmpty()) {
        pT->setRegexCodeList(patterns, patternKinds);
    }

    pT->compileAll();

    for (auto childNode : triggerNode.children()) {
        QString childNodeName = QString::fromStdString(childNode.name());
        if (childNodeName == "Trigger" || childNodeName == "TriggerGroup") {
            pugi::xml_document childDoc;
            auto childRoot = childDoc.append_child("TriggerSnapshot");
            childRoot.append_copy(childNode);

            std::ostringstream oss;
            childDoc.save(oss);
            QString childXML = QString::fromStdString(oss.str());

            // Recursively import the child with current trigger as parent (position -1 = append to end)
            importTriggerFromXML(childXML, pT, host, -1);
        }
    }

    return pT;
}

bool updateTriggerFromXML(TTrigger* pT, const QString& xmlSnapshot) {
    if (!pT || xmlSnapshot.isEmpty()) {
        return false;
    }

    QString xml = decompressXML(xmlSnapshot);
    if (xml.isEmpty()) {
        qWarning() << "updateTriggerFromXML: Failed to decompress XML";
        return false;
    }

    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_string(xml.toStdString().c_str());
    if (!result) {
        qWarning() << "updateTriggerFromXML: Failed to parse XML:" << result.description();
        return false;
    }

    auto root = doc.child("TriggerSnapshot");
    if (!root) {
        return false;
    }

    auto triggerNode = root.child("Trigger");
    if (!triggerNode) {
        triggerNode = root.child("TriggerGroup");
    }
    if (!triggerNode) {
        return false;
    }

    pT->setIsActive(QString::fromStdString(triggerNode.attribute("isActive").value()) == "yes");
    pT->setIsFolder(QString::fromStdString(triggerNode.attribute("isFolder").value()) == "yes");
    pT->setTemporary(QString::fromStdString(triggerNode.attribute("isTempTrigger").value()) == "yes");
    pT->setIsMultiline(QString::fromStdString(triggerNode.attribute("isMultiline").value()) == "yes");
    pT->mPerlSlashGOption = QString::fromStdString(triggerNode.attribute("isPerlSlashGOption").value()) == "yes";
    pT->setIsColorizerTrigger(QString::fromStdString(triggerNode.attribute("isColorizerTrigger").value()) == "yes");
    pT->mFilterTrigger = QString::fromStdString(triggerNode.attribute("isFilterTrigger").value()) == "yes";
    pT->mSoundTrigger = QString::fromStdString(triggerNode.attribute("isSoundTrigger").value()) == "yes";
    pT->mColorTrigger = QString::fromStdString(triggerNode.attribute("isColorTrigger").value()) == "yes";

    QStringList patterns;
    QList<int> patternKinds;

    for (auto node : triggerNode.children()) {
        QString nodeName = QString::fromStdString(node.name());
        QString nodeValue = QString::fromStdString(node.child_value());

        if (nodeName == "name") {
            pT->setName(nodeValue);
        } else if (nodeName == "script") {
            pT->setScript(nodeValue);
        } else if (nodeName == "packageName") {
            pT->mPackageName = nodeValue;
        } else if (nodeName == "triggerType") {
            pT->setTriggerType(nodeValue.toInt());
        } else if (nodeName == "conditonLineDelta") {
            pT->setConditionLineDelta(nodeValue.toInt());
        } else if (nodeName == "mStayOpen") {
            pT->mStayOpen = nodeValue.toInt();
        } else if (nodeName == "mCommand") {
            pT->setCommand(nodeValue);
        } else if (nodeName == "mFgColor") {
            pT->setColorizerFgColor(QColor::fromString(nodeValue));
        } else if (nodeName == "mBgColor") {
            pT->setColorizerBgColor(QColor::fromString(nodeValue));
        } else if (nodeName == "colorTriggerFgColor") {
            pT->mColorTriggerFgColor = QColor::fromString(nodeValue);
        } else if (nodeName == "colorTriggerBgColor") {
            pT->mColorTriggerBgColor = QColor::fromString(nodeValue);
        } else if (nodeName == "mSoundFile") {
            pT->setSound(nodeValue);
        } else if (nodeName == "regexCodeList") {
            for (auto patternNode : node.children("string")) {
                patterns << QString::fromStdString(patternNode.child_value());
            }
        } else if (nodeName == "regexCodePropertyList") {
            for (auto propertyNode : node.children("integer")) {
                patternKinds << QString::fromStdString(propertyNode.child_value()).toInt();
            }
        }
    }

    if (!patterns.isEmpty()) {
        pT->setRegexCodeList(patterns, patternKinds);
    }

    pT->compileAll();

    return true;
}

// =============================================================================
// ALIAS import/update functions
// =============================================================================

// Import a single alias from XML string
TAlias* importAliasFromXML(const QString& xmlSnapshot, TAlias* pParent, Host* host, int position) {
    if (xmlSnapshot.isEmpty() || !host) {
        return nullptr;
    }

    QString xml = decompressXML(xmlSnapshot);
    if (xml.isEmpty()) {
        qWarning() << "importAliasFromXML: Failed to decompress XML";
        return nullptr;
    }

    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_string(xml.toStdString().c_str());
    if (!result) {
        qWarning() << "importAliasFromXML: Failed to parse XML:" << result.description();
        return nullptr;
    }

    auto root = doc.child("AliasSnapshot");
    if (!root) {
        qWarning() << "importAliasFromXML: No AliasSnapshot root element found";
        return nullptr;
    }

    auto aliasNode = root.child("Alias");
    if (!aliasNode) {
        aliasNode = root.child("AliasGroup");
    }
    if (!aliasNode) {
        qWarning() << "importAliasFromXML: No Alias/AliasGroup element found";
        return nullptr;
    }

    // Create new alias without parent (so it doesn't auto-add to end)
    auto pA = new TAlias(nullptr, host);

    // Manually add to parent at the correct position
    if (pParent) {
        pA->setParent(pParent);
        pParent->addChild(pA, (position >= 0) ? TreeItemInsertMode::AtPosition : TreeItemInsertMode::Append, position);
        host->getAliasUnit()->registerAlias(pA); // This will call addAlias() since pA has a parent
    } else {
        // Root level alias - register first (adds to end of root list)
        host->getAliasUnit()->registerAlias(pA);

        // Now reposition it if a specific position was requested
        if (position != -1) {
            host->getAliasUnit()->reParentAlias(pA->getID(), -1, -1, TreeItemInsertMode::AtPosition, position);
        }
    }

    // Read attributes
    pA->setIsActive(QString::fromStdString(aliasNode.attribute("isActive").value()) == "yes");
    pA->setIsFolder(QString::fromStdString(aliasNode.attribute("isFolder").value()) == "yes");
    pA->setTemporary(QString::fromStdString(aliasNode.attribute("isTempAlias").value()) == "yes");

    // Read child elements
    for (auto node : aliasNode.children()) {
        QString nodeName = QString::fromStdString(node.name());
        QString nodeValue = QString::fromStdString(node.child_value());

        if (nodeName == "name") {
            pA->setName(nodeValue);
        } else if (nodeName == "script") {
            pA->setScript(nodeValue);
        } else if (nodeName == "packageName") {
            pA->mPackageName = nodeValue;
        } else if (nodeName == "regex") {
            pA->setRegexCode(nodeValue);
        } else if (nodeName == "command") {
            pA->setCommand(nodeValue);
        }
    }

    pA->compileAll();

    // Recursively import child aliases
    for (auto childNode : aliasNode.children()) {
        QString childNodeName = QString::fromStdString(childNode.name());
        if (childNodeName == "Alias" || childNodeName == "AliasGroup") {
            pugi::xml_document childDoc;
            auto childRoot = childDoc.append_child("AliasSnapshot");
            childRoot.append_copy(childNode);

            std::ostringstream oss;
            childDoc.save(oss);
            QString childXML = QString::fromStdString(oss.str());

            importAliasFromXML(childXML, pA, host);
        }
    }

    return pA;
}

// Update an existing alias from XML string
bool updateAliasFromXML(TAlias* pA, const QString& xmlSnapshot) {
    if (!pA || xmlSnapshot.isEmpty()) {
        return false;
    }

    QString xml = decompressXML(xmlSnapshot);
    if (xml.isEmpty()) {
        qWarning() << "updateAliasFromXML: Failed to decompress XML";
        return false;
    }

    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_string(xml.toStdString().c_str());
    if (!result) {
        qWarning() << "updateAliasFromXML: Failed to parse XML:" << result.description();
        return false;
    }

    auto root = doc.child("AliasSnapshot");
    if (!root) {
        return false;
    }

    auto aliasNode = root.child("Alias");
    if (!aliasNode) {
        aliasNode = root.child("AliasGroup");
    }
    if (!aliasNode) {
        return false;
    }

    // Update attributes
    pA->setIsActive(QString::fromStdString(aliasNode.attribute("isActive").value()) == "yes");
    pA->setIsFolder(QString::fromStdString(aliasNode.attribute("isFolder").value()) == "yes");
    pA->setTemporary(QString::fromStdString(aliasNode.attribute("isTempAlias").value()) == "yes");

    // Update child elements
    for (auto node : aliasNode.children()) {
        QString nodeName = QString::fromStdString(node.name());
        QString nodeValue = QString::fromStdString(node.child_value());

        if (nodeName == "name") {
            pA->setName(nodeValue);
        } else if (nodeName == "script") {
            pA->setScript(nodeValue);
        } else if (nodeName == "regex") {
            pA->setRegexCode(nodeValue);
        } else if (nodeName == "command") {
            pA->setCommand(nodeValue);
        } else if (nodeName == "packageName") {
            pA->mPackageName = nodeValue;
        }
    }

    pA->compileAll();

    return true;
}

// =============================================================================
// TIMER import/update functions
// =============================================================================

// Import a single timer from XML string
TTimer* importTimerFromXML(const QString& xmlSnapshot, TTimer* pParent, Host* host, int position) {
    if (xmlSnapshot.isEmpty() || !host) {
        return nullptr;
    }

    QString xml = decompressXML(xmlSnapshot);
    if (xml.isEmpty()) {
        qWarning() << "importTimerFromXML: Failed to decompress XML";
        return nullptr;
    }

    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_string(xml.toStdString().c_str());
    if (!result) {
        qWarning() << "importTimerFromXML: Failed to parse XML:" << result.description();
        return nullptr;
    }

    auto root = doc.child("TimerSnapshot");
    if (!root) {
        qWarning() << "importTimerFromXML: No TimerSnapshot root element found";
        return nullptr;
    }

    auto timerNode = root.child("Timer");
    if (!timerNode) {
        timerNode = root.child("TimerGroup");
    }
    if (!timerNode) {
        qWarning() << "importTimerFromXML: No Timer/TimerGroup element found";
        return nullptr;
    }

    // Create new timer without parent (so it doesn't auto-add to end)
    auto pT = new TTimer(nullptr, host);

    // Manually add to parent at the correct position
    if (pParent) {
        pT->setParent(pParent);
        pParent->addChild(pT, (position >= 0) ? TreeItemInsertMode::AtPosition : TreeItemInsertMode::Append, position);
        host->getTimerUnit()->registerTimer(pT); // This will call addTimer() since pT has a parent
    } else {
        // Root level timer - register first (adds to end of root list)
        host->getTimerUnit()->registerTimer(pT);

        // Reposition it if a specific position was requested
        if (position != -1) {
            host->getTimerUnit()->reParentTimer(pT->getID(), -1, -1, TreeItemInsertMode::AtPosition, position);
        }
    }

    // Read attributes
    pT->setShouldBeActive(QString::fromStdString(timerNode.attribute("isActive").value()) == "yes");
    pT->setIsFolder(QString::fromStdString(timerNode.attribute("isFolder").value()) == "yes");
    pT->setTemporary(QString::fromStdString(timerNode.attribute("isTempTimer").value()) == "yes");

    // Read child elements
    for (auto node : timerNode.children()) {
        QString nodeName = QString::fromStdString(node.name());
        QString nodeValue = QString::fromStdString(node.child_value());

        if (nodeName == "name") {
            pT->setName(nodeValue);
        } else if (nodeName == "script") {
            pT->setScript(nodeValue);
        } else if (nodeName == "packageName") {
            pT->mPackageName = nodeValue;
        } else if (nodeName == "command") {
            pT->setCommand(nodeValue);
        } else if (nodeName == "time") {
            pT->setTime(QTime::fromString(nodeValue, "hh:mm:ss.zzz"));
        }
    }

    pT->compileAll();

    // Recursively import child timers
    for (auto childNode : timerNode.children()) {
        QString childNodeName = QString::fromStdString(childNode.name());
        if (childNodeName == "Timer" || childNodeName == "TimerGroup") {
            pugi::xml_document childDoc;
            auto childRoot = childDoc.append_child("TimerSnapshot");
            childRoot.append_copy(childNode);

            std::ostringstream oss;
            childDoc.save(oss);
            QString childXML = QString::fromStdString(oss.str());

            importTimerFromXML(childXML, pT, host);
        }
    }

    return pT;
}

// Update an existing timer from XML string
bool updateTimerFromXML(TTimer* pT, const QString& xmlSnapshot) {
    if (!pT || xmlSnapshot.isEmpty()) {
        return false;
    }

    QString xml = decompressXML(xmlSnapshot);
    if (xml.isEmpty()) {
        qWarning() << "updateTimerFromXML: Failed to decompress XML";
        return false;
    }

    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_string(xml.toStdString().c_str());
    if (!result) {
        qWarning() << "updateTimerFromXML: Failed to parse XML:" << result.description();
        return false;
    }

    auto root = doc.child("TimerSnapshot");
    if (!root) {
        return false;
    }

    auto timerNode = root.child("Timer");
    if (!timerNode) {
        timerNode = root.child("TimerGroup");
    }
    if (!timerNode) {
        return false;
    }

    // Update attributes
    pT->setShouldBeActive(QString::fromStdString(timerNode.attribute("isActive").value()) == "yes");
    pT->setIsFolder(QString::fromStdString(timerNode.attribute("isFolder").value()) == "yes");
    pT->setTemporary(QString::fromStdString(timerNode.attribute("isTempTimer").value()) == "yes");

    // Update child elements
    for (auto node : timerNode.children()) {
        QString nodeName = QString::fromStdString(node.name());
        QString nodeValue = QString::fromStdString(node.child_value());

        if (nodeName == "name") {
            pT->setName(nodeValue);
        } else if (nodeName == "script") {
            pT->setScript(nodeValue);
        } else if (nodeName == "command") {
            pT->setCommand(nodeValue);
        } else if (nodeName == "time") {
            pT->setTime(QTime::fromString(nodeValue, "hh:mm:ss.zzz"));
        } else if (nodeName == "packageName") {
            pT->mPackageName = nodeValue;
        }
    }

    pT->compileAll();

    return true;
}

// =============================================================================
// SCRIPT import/update functions
// =============================================================================

// Import a single script from XML string
TScript* importScriptFromXML(const QString& xmlSnapshot, TScript* pParent, Host* host, int position) {
    if (xmlSnapshot.isEmpty() || !host) {
        return nullptr;
    }

    QString xml = decompressXML(xmlSnapshot);
    if (xml.isEmpty()) {
        qWarning() << "importScriptFromXML: Failed to decompress XML";
        return nullptr;
    }

    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_string(xml.toStdString().c_str());
    if (!result) {
        qWarning() << "importScriptFromXML: Failed to parse XML:" << result.description();
        return nullptr;
    }

    auto root = doc.child("ScriptSnapshot");
    if (!root) {
        qWarning() << "importScriptFromXML: No ScriptSnapshot root element found";
        return nullptr;
    }

    auto scriptNode = root.child("Script");
    if (!scriptNode) {
        scriptNode = root.child("ScriptGroup");
    }
    if (!scriptNode) {
        qWarning() << "importScriptFromXML: No Script/ScriptGroup element found";
        return nullptr;
    }

    // Create new script without parent (so it doesn't auto-add to end)
    auto pS = new TScript(nullptr, host);

    // Manually add to parent at the correct position
    if (pParent) {
        pS->setParent(pParent);
        pParent->addChild(pS, (position >= 0) ? TreeItemInsertMode::AtPosition : TreeItemInsertMode::Append, position);
        host->getScriptUnit()->registerScript(pS); // This will call addScript() since pS has a parent
    } else {
        // Root level script - register first (adds to end of root list)
        host->getScriptUnit()->registerScript(pS);

        // Reposition it if a specific position was requested
        if (position != -1) {
            host->getScriptUnit()->reParentScript(pS->getID(), -1, -1, TreeItemInsertMode::AtPosition, position);
        }
    }

    // Read attributes
    pS->setIsActive(QString::fromStdString(scriptNode.attribute("isActive").value()) == "yes");
    pS->setIsFolder(QString::fromStdString(scriptNode.attribute("isFolder").value()) == "yes");

    // Read child elements
    QStringList eventHandlers;
    for (auto node : scriptNode.children()) {
        QString nodeName = QString::fromStdString(node.name());
        QString nodeValue = QString::fromStdString(node.child_value());

        if (nodeName == "name") {
            pS->setName(nodeValue);
        } else if (nodeName == "packageName") {
            pS->mPackageName = nodeValue;
        } else if (nodeName == "script") {
            pS->setScript(nodeValue);
        } else if (nodeName == "eventHandlerList") {
            for (auto eventNode : node.children("string")) {
                eventHandlers << QString::fromStdString(eventNode.child_value());
            }
        }
    }

    // Set event handlers
    if (!eventHandlers.isEmpty()) {
        pS->setEventHandlerList(eventHandlers);
    }

    pS->compileAll();

    // Recursively import child scripts
    for (auto childNode : scriptNode.children()) {
        QString childNodeName = QString::fromStdString(childNode.name());
        if (childNodeName == "Script" || childNodeName == "ScriptGroup") {
            pugi::xml_document childDoc;
            auto childRoot = childDoc.append_child("ScriptSnapshot");
            childRoot.append_copy(childNode);

            std::ostringstream oss;
            childDoc.save(oss);
            QString childXML = QString::fromStdString(oss.str());

            importScriptFromXML(childXML, pS, host);
        }
    }

    return pS;
}

// Update an existing script from XML string
bool updateScriptFromXML(TScript* pS, const QString& xmlSnapshot) {
    if (!pS || xmlSnapshot.isEmpty()) {
        return false;
    }

    QString xml = decompressXML(xmlSnapshot);
    if (xml.isEmpty()) {
        qWarning() << "updateScriptFromXML: Failed to decompress XML";
        return false;
    }

    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_string(xml.toStdString().c_str());
    if (!result) {
        qWarning() << "updateScriptFromXML: Failed to parse XML:" << result.description();
        return false;
    }

    auto root = doc.child("ScriptSnapshot");
    if (!root) {
        return false;
    }

    auto scriptNode = root.child("Script");
    if (!scriptNode) {
        scriptNode = root.child("ScriptGroup");
    }
    if (!scriptNode) {
        return false;
    }

    // Update attributes
    pS->setIsActive(QString::fromStdString(scriptNode.attribute("isActive").value()) == "yes");
    pS->setIsFolder(QString::fromStdString(scriptNode.attribute("isFolder").value()) == "yes");

    // Update child elements
    QStringList eventHandlers;
    for (auto node : scriptNode.children()) {
        QString nodeName = QString::fromStdString(node.name());
        QString nodeValue = QString::fromStdString(node.child_value());

        if (nodeName == "name") {
            pS->setName(nodeValue);
        } else if (nodeName == "script") {
            pS->setScript(nodeValue);
        } else if (nodeName == "packageName") {
            pS->mPackageName = nodeValue;
        } else if (nodeName == "eventHandlerList") {
            for (auto eventNode : node.children("string")) {
                eventHandlers << QString::fromStdString(eventNode.child_value());
            }
        }
    }

    // Set event handlers
    if (!eventHandlers.isEmpty()) {
        pS->setEventHandlerList(eventHandlers);
    }

    pS->compileAll();

    return true;
}

// =============================================================================
// KEY import/update functions
// =============================================================================

// Import a single key from XML string
TKey* importKeyFromXML(const QString& xmlSnapshot, TKey* pParent, Host* host, int position) {
    if (xmlSnapshot.isEmpty() || !host) {
        return nullptr;
    }

    QString xml = decompressXML(xmlSnapshot);
    if (xml.isEmpty()) {
        qWarning() << "importKeyFromXML: Failed to decompress XML";
        return nullptr;
    }

    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_string(xml.toStdString().c_str());
    if (!result) {
        qWarning() << "importKeyFromXML: Failed to parse XML:" << result.description();
        return nullptr;
    }

    auto root = doc.child("KeySnapshot");
    if (!root) {
        qWarning() << "importKeyFromXML: No KeySnapshot root element found";
        return nullptr;
    }

    auto keyNode = root.child("Key");
    if (!keyNode) {
        keyNode = root.child("KeyGroup");
    }
    if (!keyNode) {
        qWarning() << "importKeyFromXML: No Key/KeyGroup element found";
        return nullptr;
    }

    // Create new key without parent (so it doesn't auto-add to end)
    auto pK = new TKey(nullptr, host);

    // Manually add to parent at the correct position
    if (pParent) {
        pK->setParent(pParent);
        pParent->addChild(pK, (position >= 0) ? TreeItemInsertMode::AtPosition : TreeItemInsertMode::Append, position);
        host->getKeyUnit()->registerKey(pK); // This will call addKey() since pK has a parent
    } else {
        // Root level key - register first (adds to end of root list)
        host->getKeyUnit()->registerKey(pK);

        // Reposition it if a specific position was requested
        if (position != -1) {
            host->getKeyUnit()->reParentKey(pK->getID(), -1, -1, TreeItemInsertMode::AtPosition, position);
        }
    }

    // Read attributes
    pK->setIsActive(QString::fromStdString(keyNode.attribute("isActive").value()) == "yes");
    pK->setIsFolder(QString::fromStdString(keyNode.attribute("isFolder").value()) == "yes");

    // Read child elements
    for (auto node : keyNode.children()) {
        QString nodeName = QString::fromStdString(node.name());
        QString nodeValue = QString::fromStdString(node.child_value());

        if (nodeName == "name") {
            pK->setName(nodeValue);
        } else if (nodeName == "packageName") {
            pK->mPackageName = nodeValue;
        } else if (nodeName == "script") {
            pK->setScript(nodeValue);
        } else if (nodeName == "command") {
            pK->setCommand(nodeValue);
        } else if (nodeName == "keyCode") {
            pK->setKeyCode(nodeValue.toInt());
        } else if (nodeName == "keyModifier") {
            pK->setKeyModifiers(nodeValue.toInt());
        }
    }

    pK->compileAll();

    // Recursively import child keys
    for (auto childNode : keyNode.children()) {
        QString childNodeName = QString::fromStdString(childNode.name());
        if (childNodeName == "Key" || childNodeName == "KeyGroup") {
            pugi::xml_document childDoc;
            auto childRoot = childDoc.append_child("KeySnapshot");
            childRoot.append_copy(childNode);

            std::ostringstream oss;
            childDoc.save(oss);
            QString childXML = QString::fromStdString(oss.str());

            importKeyFromXML(childXML, pK, host);
        }
    }

    return pK;
}

// Update an existing key from XML string
bool updateKeyFromXML(TKey* pK, const QString& xmlSnapshot) {
    if (!pK || xmlSnapshot.isEmpty()) {
        return false;
    }

    QString xml = decompressXML(xmlSnapshot);
    if (xml.isEmpty()) {
        qWarning() << "updateKeyFromXML: Failed to decompress XML";
        return false;
    }

    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_string(xml.toStdString().c_str());
    if (!result) {
        qWarning() << "updateKeyFromXML: Failed to parse XML:" << result.description();
        return false;
    }

    auto root = doc.child("KeySnapshot");
    if (!root) {
        return false;
    }

    auto keyNode = root.child("Key");
    if (!keyNode) {
        keyNode = root.child("KeyGroup");
    }
    if (!keyNode) {
        return false;
    }

    // Update attributes
    pK->setIsActive(QString::fromStdString(keyNode.attribute("isActive").value()) == "yes");
    pK->setIsFolder(QString::fromStdString(keyNode.attribute("isFolder").value()) == "yes");

    // Update child elements
    for (auto node : keyNode.children()) {
        QString nodeName = QString::fromStdString(node.name());
        QString nodeValue = QString::fromStdString(node.child_value());

        if (nodeName == "name") {
            pK->setName(nodeValue);
        } else if (nodeName == "script") {
            pK->setScript(nodeValue);
        } else if (nodeName == "command") {
            pK->setCommand(nodeValue);
        } else if (nodeName == "keyCode") {
            pK->setKeyCode(nodeValue.toInt());
        } else if (nodeName == "keyModifier") {
            pK->setKeyModifiers(nodeValue.toInt());
        } else if (nodeName == "packageName") {
            pK->mPackageName = nodeValue;
        }
    }

    pK->compileAll();

    return true;
}

// =============================================================================
// ACTION import/update functions
// =============================================================================

// Import a single action from XML string
TAction* importActionFromXML(const QString& xmlSnapshot, TAction* pParent, Host* host, int position) {
    if (xmlSnapshot.isEmpty() || !host) {
        return nullptr;
    }

    QString xml = decompressXML(xmlSnapshot);
    if (xml.isEmpty()) {
        qWarning() << "importActionFromXML: Failed to decompress XML";
        return nullptr;
    }

    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_string(xml.toStdString().c_str());
    if (!result) {
        qWarning() << "importActionFromXML: Failed to parse XML:" << result.description();
        return nullptr;
    }

    auto root = doc.child("ActionSnapshot");
    if (!root) {
        qWarning() << "importActionFromXML: No ActionSnapshot root element found";
        return nullptr;
    }

    auto actionNode = root.child("Action");
    if (!actionNode) {
        actionNode = root.child("ActionGroup");
    }
    if (!actionNode) {
        qWarning() << "importActionFromXML: No Action/ActionGroup element found";
        return nullptr;
    }

    // Create new action without parent (so it doesn't auto-add to end)
    auto pA = new TAction(nullptr, host);

    // Manually add to parent at the correct position
    if (pParent) {
        pA->Tree<TAction>::setParent(pParent);
        pParent->addChild(pA, (position >= 0) ? TreeItemInsertMode::AtPosition : TreeItemInsertMode::Append, position);
        host->getActionUnit()->registerAction(pA); // This will call addAction() since pA has a parent
    } else {
        // Root level action - register first (adds to end of root list)
        host->getActionUnit()->registerAction(pA);

        // Reposition it if a specific position was requested
        if (position != -1) {
            host->getActionUnit()->reParentAction(pA->getID(), -1, -1, TreeItemInsertMode::AtPosition, position);
        }
    }

    // Read attributes
    pA->setIsActive(QString::fromStdString(actionNode.attribute("isActive").value()) == "yes");
    pA->setIsFolder(QString::fromStdString(actionNode.attribute("isFolder").value()) == "yes");
    pA->mIsPushDownButton = QString::fromStdString(actionNode.attribute("isPushButton").value()) == "yes";
    pA->mButtonFlat = QString::fromStdString(actionNode.attribute("isFlatButton").value()) == "yes";
    pA->mUseCustomLayout = QString::fromStdString(actionNode.attribute("useCustomLayout").value()) == "yes";

    // Read child elements
    for (auto node : actionNode.children()) {
        QString nodeName = QString::fromStdString(node.name());
        QString nodeValue = QString::fromStdString(node.child_value());

        if (nodeName == "name") {
            pA->setName(nodeValue);
        } else if (nodeName == "packageName") {
            pA->mPackageName = nodeValue;
        } else if (nodeName == "script") {
            pA->setScript(nodeValue);
        } else if (nodeName == "css") {
            pA->css = nodeValue;
        } else if (nodeName == "commandButtonUp") {
            pA->setCommandButtonUp(nodeValue);
        } else if (nodeName == "commandButtonDown") {
            pA->setCommandButtonDown(nodeValue);
        } else if (nodeName == "icon") {
            pA->setIcon(nodeValue);
        } else if (nodeName == "orientation") {
            pA->mOrientation = nodeValue.toInt();
        } else if (nodeName == "location") {
            pA->mLocation = nodeValue.toInt();
        } else if (nodeName == "buttonRotation") {
            pA->mButtonRotation = nodeValue.toInt();
        } else if (nodeName == "sizeX") {
            pA->mSizeX = nodeValue.toInt();
        } else if (nodeName == "sizeY") {
            pA->mSizeY = nodeValue.toInt();
        } else if (nodeName == "buttonColumn") {
            pA->mButtonColumns = nodeValue.toInt();
        } else if (nodeName == "buttonColor") {
            // Deprecated - skip this element
        } else if (nodeName == "posX") {
            pA->mPosX = nodeValue.toInt();
        } else if (nodeName == "posY") {
            pA->mPosY = nodeValue.toInt();
        }
    }

    pA->compileAll();

    // Recursively import child actions
    for (auto childNode : actionNode.children()) {
        QString childNodeName = QString::fromStdString(childNode.name());
        if (childNodeName == "Action" || childNodeName == "ActionGroup") {
            pugi::xml_document childDoc;
            auto childRoot = childDoc.append_child("ActionSnapshot");
            childRoot.append_copy(childNode);

            std::ostringstream oss;
            childDoc.save(oss);
            QString childXML = QString::fromStdString(oss.str());

            importActionFromXML(childXML, pA, host);
        }
    }

    return pA;
}

// Update an existing action from XML string
bool updateActionFromXML(TAction* pA, const QString& xmlSnapshot) {
    if (!pA || xmlSnapshot.isEmpty()) {
        return false;
    }

    QString xml = decompressXML(xmlSnapshot);
    if (xml.isEmpty()) {
        qWarning() << "updateActionFromXML: Failed to decompress XML";
        return false;
    }

    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_string(xml.toStdString().c_str());
    if (!result) {
        qWarning() << "updateActionFromXML: Failed to parse XML:" << result.description();
        return false;
    }

    auto root = doc.child("ActionSnapshot");
    if (!root) {
        return false;
    }

    auto actionNode = root.child("Action");
    if (!actionNode) {
        actionNode = root.child("ActionGroup");
    }
    if (!actionNode) {
        return false;
    }

    // Update attributes
    pA->setIsActive(QString::fromStdString(actionNode.attribute("isActive").value()) == "yes");
    pA->setIsFolder(QString::fromStdString(actionNode.attribute("isFolder").value()) == "yes");
    pA->mIsPushDownButton = QString::fromStdString(actionNode.attribute("isPushButton").value()) == "yes";
    pA->mButtonFlat = QString::fromStdString(actionNode.attribute("isFlatButton").value()) == "yes";
    pA->mUseCustomLayout = QString::fromStdString(actionNode.attribute("useCustomLayout").value()) == "yes";

    // Update child elements
    for (auto node : actionNode.children()) {
        QString nodeName = QString::fromStdString(node.name());
        QString nodeValue = QString::fromStdString(node.child_value());

        if (nodeName == "name") {
            pA->setName(nodeValue);
        } else if (nodeName == "script") {
            pA->setScript(nodeValue);
        } else if (nodeName == "css") {
            pA->css = nodeValue;
        } else if (nodeName == "commandButtonUp") {
            pA->setCommandButtonUp(nodeValue);
        } else if (nodeName == "commandButtonDown") {
            pA->setCommandButtonDown(nodeValue);
        } else if (nodeName == "icon") {
            pA->setIcon(nodeValue);
        } else if (nodeName == "orientation") {
            pA->mOrientation = nodeValue.toInt();
        } else if (nodeName == "location") {
            pA->mLocation = nodeValue.toInt();
        } else if (nodeName == "buttonRotation") {
            pA->mButtonRotation = nodeValue.toInt();
        } else if (nodeName == "sizeX") {
            pA->mSizeX = nodeValue.toInt();
        } else if (nodeName == "sizeY") {
            pA->mSizeY = nodeValue.toInt();
        } else if (nodeName == "buttonColumn") {
            pA->mButtonColumns = nodeValue.toInt();
        } else if (nodeName == "buttonColor") {
            // Deprecated - skip this element
        } else if (nodeName == "posX") {
            pA->mPosX = nodeValue.toInt();
        } else if (nodeName == "posY") {
            pA->mPosY = nodeValue.toInt();
        } else if (nodeName == "packageName") {
            pA->mPackageName = nodeValue;
        }
    }

    pA->compileAll();

    return true;
}


// =============================================================================
// AddItemCommand implementation
// =============================================================================
