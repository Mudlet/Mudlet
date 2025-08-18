/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2016-2023 by Stephen Lyons - slysven@virginmedia.com    *
 *   Copyright (C) 2016-2017 by Ian Adkins - ieadkins@gmail.com            *
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

#include "XMLimport.h"


#include "dlgMapper.h"
#include "LuaInterface.h"
#include "CredentialManager.h"
#include "SecureStringUtils.h"
#include "TConsole.h"
#include "TMap.h"
#include "TRoomDB.h"
#include "TRoom.h"
#include "VarUnit.h"
#include "mudlet.h"

#include "pre_guard.h"
#include <QBuffer>
#include <QtMath>
#include <QVersionNumber>
#include "post_guard.h"

XMLimport::XMLimport(Host* pH)
: mpHost(pH)
, mPackageName(QString())
, mpTrigger(nullptr)
, mpTimer(nullptr)
, mpAlias(nullptr)
, mpKey(nullptr)
, mpAction(nullptr)
, mpScript(nullptr)
, mpVar(nullptr)
, gotTrigger(false)
, gotTimer(false)
, gotAlias(false)
, gotKey(false)
, gotAction(false)
, gotScript(false)
, module(0)
, mMaxRoomId(0)
, mVersionMajor(1) // 0 to 255
, mVersionMinor(0) // 0 to 999 for 3 digit decimal value
{
}

std::pair<bool, QString> XMLimport::importPackage(QFile* pfile, QString packName, int moduleFlag, QString* pVersionString)
{
    mPackageName = packName;
    setDevice(pfile);

    module = moduleFlag;

    if (!packName.isEmpty()) {
        mpKey = new TKey(nullptr, mpHost);
        if (module) {
            mpKey->mModuleMasterFolder = true;
            mpKey->mModuleMember = true;
        }
        mpKey->mPackageName = mPackageName;
        mpKey->setIsActive(true);
        mpKey->setName(mPackageName);
        mpKey->setIsFolder(true);

        mpTrigger = new TTrigger(nullptr, mpHost);
        if (module) {
            mpTrigger->mModuleMasterFolder = true;
            mpTrigger->mModuleMember = true;
        }
        mpTrigger->mPackageName = mPackageName;
        mpTrigger->setIsActive(true);
        mpTrigger->setName(mPackageName);
        mpTrigger->setIsFolder(true);

        mpTimer = new TTimer(nullptr, mpHost);
        if (module) {
            mpTimer->mModuleMasterFolder = true;
            mpTimer->mModuleMember = true;
        }
        mpTimer->mPackageName = mPackageName;
        mpTimer->setIsActive(true);
        mpTimer->setName(mPackageName);
        mpTimer->setIsFolder(true);

        mpAlias = new TAlias(nullptr, mpHost);
        if (module) {
            mpAlias->mModuleMasterFolder = true;
            mpAlias->mModuleMember = true;
        }
        mpAlias->mPackageName = mPackageName;
        mpAlias->setIsActive(true);
        mpAlias->setName(mPackageName);
        mpAlias->setScript(QString());
        mpAlias->setRegexCode(QString());
        mpAlias->setIsFolder(true);

        mpAction = new TAction(nullptr, mpHost);
        if (module) {
            mpAction->mModuleMasterFolder = true;
            mpAction->mModuleMember = true;
        }
        mpAction->mPackageName = mPackageName;
        mpAction->setIsActive(true);
        mpAction->setName(mPackageName);
        mpAction->setIsFolder(true);

        mpScript = new TScript(nullptr, mpHost);
        if (module) {
            mpScript->mModuleMasterFolder = true;
            mpScript->mModuleMember = true;
        }
        mpScript->mPackageName = mPackageName;
        mpScript->setIsActive(true);
        mpScript->setName(mPackageName);
        mpScript->setIsFolder(true);

        mpHost->getTriggerUnit()->registerTrigger(mpTrigger);
        mpHost->getTimerUnit()->registerTimer(mpTimer);
        mpHost->getAliasUnit()->registerAlias(mpAlias);
        mpHost->getActionUnit()->registerAction(mpAction);
        mpHost->getKeyUnit()->registerKey(mpKey);
        mpHost->getScriptUnit()->registerScript(mpScript);
    }

    while (!atEnd()) {
        readNext();

        if (isStartElement()) {
            if (name() == qsl("MudletPackage")) {
                QString versionString;
                if (attributes().hasAttribute(qsl("version"))) {
                    versionString = attributes().value(qsl("version")).toString();
                    if (!versionString.isEmpty()) {
                        bool isOk = false;
                        const float versionNumber = versionString.toFloat(&isOk);
                        if (isOk) {
                            mVersionMajor = qFloor(versionNumber);
                            mVersionMinor = qRound(1000.0 * versionNumber) - (1000 * mVersionMajor);
                        }
                        if (pVersionString) {
                            *pVersionString = versionString;
                        }
                    }
                }

                if (mVersionMajor > 1
                    /*||(mVersionMajor==1&&mVersionMinor)*/) {
                    // Minor check is not currently relevant, just abort on 2.000f or more

                    const QString moanMsg = tr("[ ALERT ] - Sorry, the file being read:\n"
                                         "\"%1\"\n"
                                         "reports it has a version (%2) it must have come from a later Mudlet version,\n"
                                         "and this one cannot read it, you need a newer Mudlet!")
                                              .arg(pfile->fileName(), versionString);
                    mpHost->postMessage(moanMsg);
                    return {false, moanMsg};
                }

                readPackage();
            } else if (name() == qsl("map")) {
                readMap();
                mpHost->mpMap->audit();
                mpHost->mpMap->mpMapper->mp2dMap->init();
                mpHost->mpMap->mpMapper->updateAreaComboBox();
                mpHost->mpMap->mpMapper->resetAreaComboBoxToPlayerRoomArea();
                mpHost->mpMap->mpMapper->show();
            } else {
                qDebug().nospace() << "XMLimport::importPackage(...) ERROR: "
                                      "unrecognised element with name: "
                                   << name().toString() << " and content: " << text().toString();
            }
        }
    }

    if (!packName.isEmpty()) {
        if (!gotTrigger) {
            mpHost->getTriggerUnit()->unregisterTrigger(mpTrigger);
            delete mpTrigger;
        }

        if (gotTimer) { // packName is NOT empty for modules...!
            mpTimer->setIsActive(true);
            mpTimer->enableTimer(mpTimer->getID());
        } else {
            mpHost->getTimerUnit()->unregisterTimer(mpTimer);
            delete mpTimer;
        }

        if (gotAlias) {
            mpAlias->setIsActive(true);
        } else {
            mpHost->getAliasUnit()->unregisterAlias(mpAlias);
            delete mpAlias;
        }

        if (gotAction) {
            mpHost->getActionUnit()->updateToolbar();
        } else {
            mpHost->getActionUnit()->unregisterAction(mpAction);
            delete mpAction;
        }

        if (!gotKey) {
            mpHost->getKeyUnit()->unregisterKey(mpKey);
            delete mpKey;
        }

        if (!gotScript) {
            mpHost->getScriptUnit()->unregisterScript(mpScript);
            delete mpScript;
        }
    }

    return {!hasError(), errorString()};
}

// returns the type of item and ID of the first (root) element
std::pair<dlgTriggerEditor::EditorViewType, int> XMLimport::importFromClipboard()
{
    QString xml;
    QClipboard* clipboard = QApplication::clipboard();
    std::pair<dlgTriggerEditor::EditorViewType, int> result;

    xml = clipboard->text(QClipboard::Clipboard);

    QByteArray ba = xml.toUtf8();
    QBuffer xmlBuffer(&ba);

    setDevice(&xmlBuffer);
    xmlBuffer.open(QIODevice::ReadOnly);

    while (!atEnd()) {
        readNext();

        if (isStartElement()) {
            if (name() == qsl("MudletPackage")) {
                result = readPackage();
            } else {
                qDebug() << "ERROR:name=" << name().toString() << "text:" << text().toString();
            }
        }
    }

    return result;
}

void XMLimport::readVariable(TVar* pParent)
{
    auto var = new TVar(pParent);

    LuaInterface* lI = mpHost->getLuaInterface();
    VarUnit* vu = lI->getVarUnit();
    QString keyName, value;
    int keyType = 0;
    int valueType;

    const QString what = name().toString();
    while (!atEnd()) {
        readNext();
        if (isEndElement()) {
            break;
        }

        if (isStartElement()) {
            if (name() == qsl("name")) {
                keyName = readElementText();
                continue;
            } else if (name() == qsl("value")) {
                value = readElementText();
                continue;
            } else if (name() == qsl("keyType")) {
                keyType = readElementText().toInt();
                continue;
            } else if (name() == qsl("valueType")) {
                valueType = readElementText().toInt();
                var->setName(keyName, keyType);
                var->setValue(value, valueType);
                vu->addSavedVar(var);
                lI->setValue(var);
                continue;
            } else if (name() == qsl("VariableGroup") || name() == qsl("Variable")) {
                readVariable(var);
            } else {
                readUnknownElement(what);
            }
        }
    }

    delete var;
}

void XMLimport::readHiddenVariables()
{
    LuaInterface* lI = mpHost->getLuaInterface();
    VarUnit* vu = lI->getVarUnit();

    while (!atEnd()) {
        readNext();
        if (isEndElement()) {
            break;
        }

        if (isStartElement()) {
            if (name() == qsl("name")) {
                const QString var = readElementText();
                vu->addHidden(var);
                continue;
            }
        }
    }
}

void XMLimport::readVariablePackage()
{
    LuaInterface* lI = mpHost->getLuaInterface();
    VarUnit* vu = lI->getVarUnit();

    mpVar = vu->getBase();
    while (!atEnd()) {
        readNext();
        if (isStartElement()) {
            if (name() == qsl("VariableGroup") || name() == qsl("Variable")) {
                readVariable(mpVar);
            } else if (name() == qsl("HiddenVariables")) {
                readHiddenVariables();
            } else {
                readUnknownElement(qsl("VariablePackage"));
            }
        }
    }
}

void XMLimport::readMap()
{
    QMultiHash<int, int> tempAreaRoomsHash; // Keys: area id, Values: a room id in that area

    while (!atEnd()) {
        readNext();

        if (isStartElement()) {
            if (name() == qsl("areas")) {
                mpHost->mpMap->mpRoomDB->clearMapDB();
                mpHost->mpMap->reportStringToProgressDialog(tr("Parsing area data..."));
                mpHost->mpMap->reportProgressToProgressDialog(0, 3);
                readAreas();
            } else if (name() == qsl("rooms")) {
                mpHost->mpMap->reportStringToProgressDialog(tr("Parsing room data..."));
                mpHost->mpMap->reportProgressToProgressDialog(1, 3);
                readRooms(tempAreaRoomsHash);
            } else if (name() == qsl("environments")) {
                mpHost->mpMap->reportStringToProgressDialog(tr("Parsing environment data..."));
                mpHost->mpMap->reportProgressToProgressDialog(2, 3);
                readEnvColors();
            }
            mpHost->mpMap->reportProgressToProgressDialog(3, 3);
        }
    }

    mpHost->mpMap->reportStringToProgressDialog(tr("Assigning rooms to their areas..."));
    const int roomTotal = tempAreaRoomsHash.count();
    int currentRoomCount = 0;

    QListIterator<int> itAreaWithRooms(tempAreaRoomsHash.uniqueKeys());
    while (itAreaWithRooms.hasNext()) {
        const int areaId = itAreaWithRooms.next();
        auto values = tempAreaRoomsHash.values(areaId);
        QSet<int> const areaRoomsSet{values.begin(), values.end()};

        if (!mpHost->mpMap->mpRoomDB->areas.contains(areaId)) {
            // It is known for map files to have rooms with area Ids that are
            // not in the listed areas - this cures that:
            mpHost->mpMap->mpRoomDB->addArea(areaId);
        }

        mpHost->mpMap->mpRoomDB->setAreaRooms(areaId, areaRoomsSet);
        currentRoomCount += areaRoomsSet.count();
        mpHost->mpMap->reportProgressToProgressDialog(currentRoomCount, roomTotal);
    }
}

void XMLimport::readEnvColors()
{
    while (!atEnd()) {
        readNext();

        if (name() == qsl("environment")) {
            readEnvColor();
        }
    }
}

void XMLimport::readEnvColor()
{
    const int id = attributes().value(qsl("id")).toString().toInt();
    const int color = attributes().value(qsl("color")).toString().toInt();

    mpHost->mpMap->mEnvColors[id] = color;
}

void XMLimport::readAreas()
{
    while (!atEnd()) {
        readNext();

        if (name() == qsl("areas")) {
            break;
        } else if (name() == qsl("area")) {
            readArea();
        }
    }
}

void XMLimport::readArea()
{
    if (attributes().hasAttribute(qsl("id"))) {
        const int id = attributes().value(qsl("id")).toString().toInt();
        const QString name = attributes().value(qsl("name")).toString();

        mpHost->mpMap->mpRoomDB->addArea(id, name);
    }
}

void XMLimport::readRooms(QMultiHash<int, int>& areaRoomsHash)
{
    unsigned int roomCount = 0;

    while (!atEnd()) {
        readNext();

        if (Q_LIKELY(isStartElement())) {
            if (Q_LIKELY(name() == qsl("room"))) {
                readRoom(areaRoomsHash, &roomCount);
            } else {
                readUnknownMapElement();
            }
        } else if (isEndElement() && name() == qsl("rooms")) {
            break;
        }
    }
}

void XMLimport::readRoomFeatures(TRoom* pR)
{
    while (!atEnd()) {
        readNext();

        if (Q_LIKELY(isStartElement())) {
            if (name() == qsl("features")) {
                continue;
            } else if (Q_LIKELY(name() == qsl("feature"))) {
                readRoomFeature(pR);
            }
        } else if (isEndElement() && name() == qsl("features")) {
            break;
        }
    }
}

void XMLimport::readRoomFeature(TRoom* pR)
{
    if (Q_LIKELY(attributes().hasAttribute(qsl("type")))) {
        pR->userData.insert(qsl("feature-%1").arg(attributes().value(qsl("type"))), qsl("true"));
    }
}

// This is a CPU/Time hog without the non-default (true) third argument to
// TRoomDB::addRoom(...)
void XMLimport::readRoom(QMultiHash<int, int>& areamRoomMultiHash, unsigned int* roomCount)
{
    auto pT = new TRoom(mpHost->mpMap->mpRoomDB);

    pT->id = attributes().value(qsl("id")).toString().toInt();
    pT->area = attributes().value(qsl("area")).toString().toInt();
    pT->name = attributes().value(qsl("title")).toString();
    pT->environment = attributes().value(qsl("environment")).toString().toInt();

    while (!atEnd()) {
        readNext();

        if (Q_UNLIKELY(pT->id < 1)) {
            continue; // Skip further tests on exits as we'd have to throw away
                      // this invalid room and it would mess up the
                      // entranceMultiHash
        } else if (Q_LIKELY(name() == qsl("exit"))) {
            QString dir = attributes().value(qsl("direction")).toString();
            const int e = attributes().value(qsl("target")).toString().toInt();
            // If there is a "hidden" exit mark it as a locked door, otherwise
            // if there is a "door" mark it as an open/closed/locked door
            // depending on the value (I.R.E. MUD maps always uses "1" for "door"
            // and/or "hidden" - though the latter does not always appear with
            // former):
            const int door = (attributes().hasAttribute(qsl("hidden")) && attributes().value(qsl("hidden")).toString().toInt() == 1)
                    ? 3
                    : (attributes().hasAttribute(qsl("door")) && attributes().value(qsl("door")).toString().toInt() >= 0 && attributes().value(qsl("door")).toString().toInt() <= 3)
                      ? attributes().value(qsl("door")).toString().toInt()
                      : 0;
            if (dir.isEmpty()) {
                if (attributes().value(qsl("special")).toString().toInt() == 1 && !attributes().value(qsl("command")).toString().isEmpty()) {
                    // This is how IRE XML maps mark special exits, rather than
                    // by just using a different string for the direction!
                    dir = attributes().value(qsl("command")).toString();
                    pT->setSpecialExit(e, dir);
                    pT->setDoor(dir, door);
                } else {
                    continue;
                }
            } else if (dir == qsl("north")) {
                pT->north = e;
                pT->setDoor(qsl("n"), door);
            } else if (dir == qsl("east")) {
                pT->east = e;
                pT->setDoor(qsl("e"), door);
            } else if (dir == qsl("south")) {
                pT->south = e;
                pT->setDoor(qsl("s"), door);
            } else if (dir == qsl("west")) {
                pT->west = e;
                pT->setDoor(qsl("w"), door);
            } else if (dir == qsl("up")) {
                pT->up = e;
                pT->setDoor(qsl("up"), door);
            } else if (dir == qsl("down")) {
                pT->down = e;
                pT->setDoor(qsl("down"), door);
            } else if (dir == qsl("northeast")) {
                pT->northeast = e;
                pT->setDoor(qsl("ne"), door);
            } else if (dir == qsl("southwest")) {
                pT->southwest = e;
                pT->setDoor(qsl("sw"), door);
            } else if (dir == qsl("southeast")) {
                pT->southeast = e;
                pT->setDoor(qsl("se"), door);
            } else if (dir == qsl("northwest")) {
                pT->northwest = e;
                pT->setDoor(qsl("nw"), door);
            } else if (dir == qsl("in")) {
                pT->in = e;
                pT->setDoor(qsl("in"), door);
            } else if (dir == qsl("out")) {
                pT->out = e;
                pT->setDoor(qsl("out"), door);
            } else {
                // TODO: Handle Special Exits
            }
        } else if (name() == qsl("coord")) {
            if (attributes().value(qsl("x")).toString().isEmpty()) {
                continue;
            }

            pT->setCoordinates(attributes().value(qsl("x")).toString().toInt(),
                               attributes().value(qsl("y")).toString().toInt(),
                               attributes().value(qsl("z")).toString().toInt());
            continue;
        } else if (name() == qsl("features")) {
            readRoomFeatures(pT);
        } else if (Q_UNLIKELY(name().isEmpty())) {
            continue;
        }

        if (isEndElement() && name() == qsl("room")) {
            break;
        }
    }

    if (pT->id > 0) {
        if (++(*roomCount) % 100 == 0) {
            mpHost->mpMap->reportStringToProgressDialog(tr("Parsing room data [count: %1]...").arg(*roomCount));
        }
        areamRoomMultiHash.insert(pT->area, pT->id);
        // We are loading a map so can make some optimisation by setting the
        // third argument as true:
        mpHost->mpMap->mpRoomDB->addRoom(pT->id, pT, true);
        mMaxRoomId = qMax(mMaxRoomId, pT->id); // Wasn't used but now maintains max Room Id
    } else {
        delete pT;
    }
}

void XMLimport::readUnknownMapElement()
{
    while (!atEnd()) {
        readNext();

        if (isEndElement()) {
            break;
        } else if (isStartElement()) {
            readUnknownMapElement();
        }
    }
}

// returns the type of item and ID of the first (root) element
std::pair<dlgTriggerEditor::EditorViewType, int> XMLimport::readPackage()
{
    dlgTriggerEditor::EditorViewType objectType = dlgTriggerEditor::EditorViewType::cmUnknownView;
    int rootItemID = -1;
    while (!atEnd()) {
        readNext();

        if (isEndElement()) {
            break;
        } else if (isStartElement()) {
            if (name() == qsl("HostPackage")) {
                readHostPackage();
            } else if (name() == qsl("TriggerPackage")) {
                objectType = dlgTriggerEditor::EditorViewType::cmTriggerView;
                rootItemID = readTriggerPackage();
            } else if (name() == qsl("TimerPackage")) {
                objectType = dlgTriggerEditor::EditorViewType::cmTimerView;
                rootItemID = readTimerPackage();
            } else if (name() == qsl("AliasPackage")) {
                objectType = dlgTriggerEditor::EditorViewType::cmAliasView;
                rootItemID = readAliasPackage();
            } else if (name() == qsl("ActionPackage")) {
                objectType = dlgTriggerEditor::EditorViewType::cmActionView;
                rootItemID = readActionPackage();
            } else if (name() == qsl("ScriptPackage")) {
                objectType = dlgTriggerEditor::EditorViewType::cmScriptView;
                rootItemID = readScriptPackage();
            } else if (name() == qsl("KeyPackage")) {
                objectType = dlgTriggerEditor::EditorViewType::cmKeysView;
                rootItemID = readKeyPackage();
            } else if (name() == qsl("HelpPackage")) {
                readHelpPackage();
            } else if (name() == qsl("VariablePackage")) {
                objectType = dlgTriggerEditor::EditorViewType::cmVarsView;
                readVariablePackage();
            } else {
                readUnknownElement(qsl("MudletPackage"));
            }
        }
    }
    return {objectType, rootItemID};
}

void XMLimport::readHelpPackage()
{
    while (!atEnd()) {
        readNext();

        if (isEndElement()) {
            break;
        } else if (isStartElement()) {
            if (name() == qsl("helpURL")) {
                const QString contents = readElementText();
                mpHost->moduleHelp[mPackageName].insert("helpURL", contents);
            }
        }
    }
}

// Will be on a startElement on entry, and on the matching endElement
// at exit:
void XMLimport::readUnknownElement(const QString& what)
{
    if (!atEnd()) {
        qDebug().nospace().noquote() << "XMLimport::readUnknownElement(\"" << what << "\") ERROR - UNKNOWN Package Element name: \"" << name().toString() << "\".";
        qDebug().nospace().noquote() << "    This is at byte offset: " << characterOffset() << ", which is (line:column): " << lineNumber() << ":" << columnNumber() << ".";
#if !defined(QT_STRICT_ITERATORS)
        if (attributes().isEmpty()) {
            qDebug().nospace().noquote() << "    It has no attributes.";
        } else {
            // This can fail if QT_STRICT_ITERATORS is defined.
            // See https://bugreports.qt.io/browse/QTBUG-45368
            QVectorIterator<QXmlStreamAttribute> itAttribute(attributes());
            qDebug().nospace().noquote() << "    It has the following attributes:";
            while (itAttribute.hasNext()) {
                const auto attribute = itAttribute.next();
                qDebug().nospace().noquote() << "        name: \"" << attribute.name() << "\", value: \"" << attribute.value() << "\".";
            }
        }
#endif
        // The argument to readElementText(...) is required otherwise it stops
        // if a child element is encountered, the third alternative
        // "IncludeChildElements" is not so helpful as it might seem as it only
        // includes some of the intervening content from sub-elements. As it is
        // this should advance the current position to the EndElement of the
        // unexpected startElement:
        qDebug().nospace().noquote() << "    The (text) content is: \"" << readElementText(QXmlStreamReader::SkipChildElements) << "\"";
    }
}

void XMLimport::readHostPackage()
{
    while (!atEnd()) {
        readNext();
        if (isEndElement()) {
            break;
        } else if (isStartElement()) {
            if (name() == qsl("Host")) {
                readHost(mpHost);
            } else {
                readUnknownElement(qsl("HostPackage"));
            }
        }
    }
}

void XMLimport::readHost(Host* pHost)
{
    // This is an inline helper function to get a boolean value from a legacy attribute
    // or return a default value. It also allows for inverting the result which is useful
    // for attributes that have been negated in the past (e.g., mFORCE_MXP_NEGOTIATION_OFF
    // which is now mEnableMXP).
    auto getBoolValueFromLegacyAttributeOrDefault = [&](const QString& legacyAttribute, const bool defaultsTo, bool invert = false) -> bool {
        if (attributes().hasAttribute(legacyAttribute)) {
            bool value = attributes().value(legacyAttribute) == YES;
            return invert ? !value : value;
        } else {
            return defaultsTo;
        }
    };

    auto setBoolAttributeWithDefault = [&](const QString& attribute, bool& target, const bool defaultsTo) {
        target = attributes().hasAttribute(attribute) ? attributes().value(attribute) == YES : defaultsTo;
    };

    auto setBoolAttribute = [&](const QString& attribute, bool& target) {
        target = attributes().value(attribute) == YES;
    };

    setBoolAttributeWithDefault(qsl("announceIncomingText"), pHost->mAnnounceIncomingText, true);
    setBoolAttributeWithDefault(qsl("advertiseScreenReader"), pHost->mAdvertiseScreenReader, false);
    setBoolAttributeWithDefault(qsl("enableClosedCaption"), pHost->mEnableClosedCaption, false);
    setBoolAttributeWithDefault(qsl("mEnableMTTS"), pHost->mEnableMTTS, true);
    setBoolAttributeWithDefault(qsl("mEnableMNES"), pHost->mEnableMNES, false);
    setBoolAttributeWithDefault(qsl("mEnableMXP"), pHost->mEnableMXP, getBoolValueFromLegacyAttributeOrDefault(qsl("mFORCE_MXP_NEGOTIATION_OFF"), true, true));
    setBoolAttributeWithDefault(qsl("forceNewEnvironNegotiationOff"), pHost->mForceNewEnvironNegotiationOff, false);

    setBoolAttribute(qsl("autoClearCommandLineAfterSend"), pHost->mAutoClearCommandLineAfterSend);
    
    // Handle command echo mode with backward compatibility
    if (attributes().hasAttribute(qsl("commandEchoMode"))) {
        // New tri-state attribute
        int echoMode = attributes().value(qsl("commandEchoMode")).toInt();
        pHost->mCommandEchoMode = static_cast<Host::CommandEchoMode>(qBound(0, echoMode, 2));
    } else {
        // Legacy boolean attribute - convert to new enum
        bool legacyPrintCommand = getBoolValueFromLegacyAttributeOrDefault(qsl("printCommand"), true);
        pHost->mCommandEchoMode = legacyPrintCommand ? Host::CommandEchoMode::ScriptControl : Host::CommandEchoMode::Never;
    }
    setBoolAttribute(qsl("mUSE_FORCE_LF_AFTER_PROMPT"), pHost->mUSE_FORCE_LF_AFTER_PROMPT);
    setBoolAttribute(qsl("mUSE_UNIX_EOL"), pHost->mUSE_UNIX_EOL);
    setBoolAttribute(qsl("runAllKeyMatches"), pHost->getKeyUnit()->mRunAllKeyMatches);
    setBoolAttribute(qsl("mNoAntiAlias"), pHost->mNoAntiAlias);
    setBoolAttribute(qsl("mEchoLuaErrors"), pHost->mEchoLuaErrors);
    setBoolAttribute(qsl("mRawStreamDump"), pHost->mIsNextLogFileInHtmlFormat);
    setBoolAttribute(qsl("mIsLoggingTimestamps"), pHost->mIsLoggingTimestamps);
    setBoolAttribute(qsl("mAlertOnNewData"), pHost->mAlertOnNewData);
    setBoolAttribute(qsl("mFORCE_NO_COMPRESSION"), pHost->mFORCE_NO_COMPRESSION);
    setBoolAttribute(qsl("mFORCE_GA_OFF"), pHost->mFORCE_GA_OFF);
    setBoolAttribute(qsl("mEnableGMCP"), pHost->mEnableGMCP);
    setBoolAttribute(qsl("mEnableMSSP"), pHost->mEnableMSSP);
    setBoolAttribute(qsl("mEnableMSDP"), pHost->mEnableMSDP);
    setBoolAttribute(qsl("mEnableMSP"), pHost->mEnableMSP);
    setBoolAttribute(qsl("mMapStrongHighlight"), pHost->mMapStrongHighlight);
    setBoolAttribute(qsl("mEnableSpellCheck"), pHost->mEnableSpellCheck);
    setBoolAttribute(qsl("mAcceptServerGUI"), pHost->mAcceptServerGUI);
    setBoolAttribute(qsl("mAcceptServerMedia"), pHost->mAcceptServerMedia);
    setBoolAttribute(qsl("mMapperUseAntiAlias"), pHost->mMapperUseAntiAlias);
    setBoolAttribute(qsl("mEditorAutoComplete"), pHost->mEditorAutoComplete);
    setBoolAttribute(qsl("mFORCE_CHARSET_NEGOTIATION_OFF"), pHost->mFORCE_CHARSET_NEGOTIATION_OFF);
    setBoolAttribute(qsl("mVersionInTTYPE"), pHost->mVersionInTTYPE);
    setBoolAttribute(qsl("mPromptedForVersionInTTYPE"), pHost->mPromptedForVersionInTTYPE);
    setBoolAttribute(qsl("mPromptedForMXPProcessorOn"), pHost->mPromptedForMXPProcessorOn);
    setBoolAttribute(qsl("enableTextAnalyzer"), pHost->mEnableTextAnalyzer);
    setBoolAttribute(qsl("mBubbleMode"), pHost->mBubbleMode);
    setBoolAttribute(qsl("mMapViewOnly"), pHost->mMapViewOnly);
    setBoolAttribute(qsl("mShowRoomIDs"), pHost->mShowRoomID);
    setBoolAttribute(qsl("mShowPanel"), pHost->mShowPanel);
    setBoolAttribute(qsl("mHaveMapperScript"), pHost->mHaveMapperScript);
    setBoolAttribute(qsl("mSslTsl"), pHost->mSslTsl);
    setBoolAttribute(qsl("mSslIgnoreExpired"), pHost->mSslIgnoreExpired);
    setBoolAttribute(qsl("mSslIgnoreSelfSigned"), pHost->mSslIgnoreSelfSigned);
    setBoolAttribute(qsl("mSslIgnoreAll"), pHost->mSslIgnoreAll);
    setBoolAttribute(qsl("mAskTlsAvailable"), pHost->mAskTlsAvailable);
    setBoolAttribute(qsl("mUseProxy"), pHost->mUseProxy);
    setBoolAttribute(qsl("f3SearchEnabled"), pHost->mF3SearchEnabled);

    pHost->setForceMXPProcessorOn(attributes().value(qsl("mForceMXPProcessorOn")) == YES);
    pHost->mProxyAddress = attributes().value(qsl("mProxyAddress")).toString();

    if (attributes().hasAttribute(QLatin1String("mProxyPort"))) {
        pHost->mProxyPort = attributes().value(qsl("mProxyPort")).toInt();
    } else {
        pHost->mProxyPort = 0;
    }

    pHost->mProxyUsername = attributes().value(qsl("mProxyUsername")).toString();
    
    // Handle backward compatibility based on application version, not profile version
    QString storedProxyPassword = attributes().value(qsl("mProxyPassword")).toString();
    
    // For version 4.20.0+, use secure storage; for older versions, maintain plaintext in XML
    // Use current application version for consistency with XMLexport behavior
    const QString currentAppVersion = QString(APP_VERSION);
    const QVersionNumber appVersion = QVersionNumber::fromString(currentAppVersion);
    const QVersionNumber secureStorageVersion = QVersionNumber(4, 20, 0);
    const bool useSecureStorage = appVersion >= secureStorageVersion;
    
    if (!storedProxyPassword.isEmpty()) {
        if (useSecureStorage) {
            // Modern application: migrate plaintext password to secure storage and clear from XML
            CredentialManager::storeCredential(pHost->getName(), "proxy", storedProxyPassword);
            pHost->mProxyPassword = storedProxyPassword;
            SecureStringUtils::secureStringClear(storedProxyPassword); // Clear after migration
        } else {
            // Legacy application: keep plaintext password for backward compatibility
            pHost->mProxyPassword = storedProxyPassword;
        }
    } else if (useSecureStorage) {
        // Modern application: load from secure storage if available
        pHost->mProxyPassword = CredentialManager::retrieveCredential(pHost->getName(), "proxy");
    }

    pHost->set_USE_IRE_DRIVER_BUGFIX(attributes().value(qsl("USE_IRE_DRIVER_BUGFIX")) == YES);
    pHost->mHighlightHistory = readDefaultTrueBool(qsl("HighlightHistory"));
    pHost->mLogDir = attributes().value(qsl("logDirectory")).toString();
    pHost->mFORCE_SAVE_ON_EXIT = readDefaultTrueBool(qsl("mFORCE_SAVE_ON_EXIT"));
    const bool enableUserDictionary = attributes().value(qsl("mEnableUserDictionary")) == YES;
    const bool useSharedDictionary = attributes().value(qsl("mUseSharedDictionary")) == YES;
    pHost->setUserDictionaryOptions(enableUserDictionary, useSharedDictionary);
    pHost->mMapperShowRoomBorders = readDefaultTrueBool(qsl("mMapperShowRoomBorders"));
    pHost->mEditorTheme = attributes().value(QLatin1String("mEditorTheme")).toString();
    pHost->mEditorThemeFile = attributes().value(QLatin1String("mEditorThemeFile")).toString();
    if (pHost->mEditorTheme.isEmpty() || pHost->mEditorThemeFile.isEmpty()) {
        pHost->mEditorTheme = qsl("Mudlet");
        pHost->mEditorThemeFile = qsl("Mudlet.tmTheme");
    }
    pHost->mThemePreviewItemID = attributes().value(QLatin1String("mThemePreviewItemID")).toInt();
    pHost->mThemePreviewType = attributes().value(QLatin1String("mThemePreviewType")).toString();
    pHost->setHaveColorSpaceId(attributes().value(QLatin1String("mSGRCodeHasColSpaceId")).toString() == QLatin1String("yes"));
    pHost->setMayRedefineColors(attributes().value(QLatin1String("mServerMayRedefineColors")).toString() == QLatin1String("yes"));

    if (attributes().hasAttribute("AmbigousWidthGlyphsToBeWide")) {
        const QStringView ambiguousWidthSetting(attributes().value(qsl("AmbigousWidthGlyphsToBeWide")));

        if (ambiguousWidthSetting == YES) {
            pHost->setWideAmbiguousEAsianGlyphs(Qt::Checked);
        } else if (ambiguousWidthSetting == qsl("auto")) {
            pHost->setWideAmbiguousEAsianGlyphs(Qt::PartiallyChecked);
        } else {
            pHost->setWideAmbiguousEAsianGlyphs(Qt::Unchecked);
        }
    } else {
        // The encoding setting is stored as part of the profile details and NOT
        // in the save file - probably because it is needed before the
        // connection to the Server is initiated so it will already be in place
        // which is just as well as it is needed for the automatic case...
        pHost->setWideAmbiguousEAsianGlyphs(Qt::PartiallyChecked);
    }

    if (attributes().hasAttribute("logFileNameFormat")) {
        // We previously mixed "yyyy-MM-dd{#|T}hh-MM-ss" with "yyyy-MM-dd{#|T}HH-MM-ss"
        // which is slightly different {always use 24-hour clock even if AM/PM is
        // present (it isn't)} and that broke some code that requires an exact
        // string to work with - now always change it to "HH":
        pHost->mLogFileNameFormat = attributes().value(qsl("logFileNameFormat")).toString().replace(QLatin1String("hh"), QLatin1String("HH"), Qt::CaseSensitive);
        pHost->mLogFileName = attributes().value(qsl("logFileName")).toString();
    }

    if (attributes().hasAttribute("mEditorShowBidi")) {
        pHost->setEditorShowBidi(attributes().value(qsl("mEditorShowBidi")) == YES);
    } else {
        pHost->setEditorShowBidi(true);
    }

    if (attributes().hasAttribute("caretShortcut")) {
        const QStringView caretShortcut(attributes().value(qsl("caretShortcut")));
        if (caretShortcut == qsl("None")) {
            pHost->mCaretShortcut = Host::CaretShortcut::None;
        } else if (caretShortcut == qsl("Tab")) {
            pHost->mCaretShortcut = Host::CaretShortcut::Tab;
        } else if (caretShortcut == qsl("CtrlTab")) {
            pHost->mCaretShortcut = Host::CaretShortcut::CtrlTab;
        } else if (caretShortcut == qsl("F6")) {
            pHost->mCaretShortcut = Host::CaretShortcut::F6;
        }
    }

    if (attributes().hasAttribute("blankLineBehaviour")) {
        const QStringView blankLineBehaviour(attributes().value(qsl("blankLineBehaviour")));
        if (blankLineBehaviour == qsl("Hide")) {
            pHost->mBlankLineBehaviour = Host::BlankLineBehaviour::Hide;
        } else if (blankLineBehaviour == qsl("Show")) {
            pHost->mBlankLineBehaviour = Host::BlankLineBehaviour::Show;
        } else if (blankLineBehaviour == qsl("ReplaceWithSpace")) {
            pHost->mBlankLineBehaviour = Host::BlankLineBehaviour::ReplaceWithSpace;
        }
    }

    if (attributes().hasAttribute(QLatin1String("mSearchEngineName"))) {
        pHost->mSearchEngineName = attributes().value(QLatin1String("mSearchEngineName")).toString();
    } else {
        pHost->mSearchEngineName = QString("Google");
    }

    if (attributes().hasAttribute(QLatin1String("mTimerSupressionInterval"))) {
        pHost->mTimerDebugOutputSuppressionInterval = QTime::fromString(attributes().value(QLatin1String("mTimerSupressionInterval")).toString(), QLatin1String("hh:mm:ss.zzz"));
    } else {
        pHost->mTimerDebugOutputSuppressionInterval = QTime();
    }

    if (attributes().hasAttribute(QLatin1String("mDiscordAccessFlags"))) {
        pHost->mDiscordAccessFlags = static_cast<Host::DiscordOptionFlags>(attributes().value(qsl("mDiscordAccessFlags")).toString().toInt());
    }

    if (attributes().hasAttribute(QLatin1String("mRequiredDiscordUserName"))) {
        pHost->mRequiredDiscordUserName = attributes().value(QLatin1String("mRequiredDiscordUserName")).toString();
    } else {
        pHost->mRequiredDiscordUserName.clear();
    }

    if (attributes().hasAttribute(QLatin1String("mRequiredDiscordUserDiscriminator"))) {
        pHost->mRequiredDiscordUserDiscriminator = attributes().value(QLatin1String("mRequiredDiscordUserDiscriminator")).toString();
    } else {
        pHost->mRequiredDiscordUserDiscriminator.clear();
    }

    if (attributes().hasAttribute(QLatin1String("playerRoomStyle"))) {
        quint8 styleCode = 0;
        quint8 outerDiameterPercentage = 0;
        quint8 innerDiameterPercentage = 0;
        QColor outerColor;
        QColor innerColor;
        // Retrieve current (possibly default) settings:
        pHost->getPlayerRoomStyleDetails(styleCode, outerDiameterPercentage, innerDiameterPercentage, outerColor, innerColor);
        // Gather values from file:
        styleCode = static_cast<quint8>(qBound(0, attributes().value(QLatin1String("playerRoomStyle")).toInt(), 255));
        outerDiameterPercentage = static_cast<quint8>(qBound(0, attributes().value(QLatin1String("playerRoomOuterDiameter")).toInt(), 255));
        innerDiameterPercentage = static_cast<quint8>(qBound(0, attributes().value(QLatin1String("playerRoomInnerDiameter")).toInt(), 255));
#if QT_VERSION < QT_VERSION_CHECK(6, 6, 0)
        // QColor::setNamedColor(...) is depracated since Qt 6.6
        outerColor.setNamedColor(attributes().value(QLatin1String("playerRoomPrimaryColor")).toString());
        innerColor.setNamedColor(attributes().value(QLatin1String("playerRoomSecondaryColor")).toString());
#else
        outerColor = QColor::fromString(attributes().value(QLatin1String("playerRoomPrimaryColor")).toString());
        innerColor = QColor::fromString(attributes().value(QLatin1String("playerRoomSecondaryColor")).toString());
#endif
        // Store all the settings in the Host instance:
        pHost->setPlayerRoomStyleDetails(styleCode, outerDiameterPercentage, innerDiameterPercentage, outerColor, innerColor);

        if (pHost->mpMap) {
            // And the TMap instance:
            pHost->mpMap->mPlayerRoomStyle = styleCode;
            pHost->mpMap->mPlayerRoomOuterDiameterPercentage = outerDiameterPercentage;
            pHost->mpMap->mPlayerRoomInnerDiameterPercentage = innerDiameterPercentage;
            pHost->mpMap->mPlayerRoomOuterColor = outerColor;
            pHost->mpMap->mPlayerRoomInnerColor = innerColor;
        }
    }

    pHost->mRoomSize = attributes().value(qsl("mRoomSize")).toString().toDouble();

    if (qFuzzyCompare(1.0 + pHost->mRoomSize, 1.0)) {
        // The value is a float/double and the prior code using "== 0" is a BAD
        // THING to do with non-integer number types!
        pHost->mRoomSize = 0.5; // Same value as is in Host class initializer list
    }

    pHost->mLineSize = attributes().value(qsl("mLineSize")).toString().toDouble();

    if (qFuzzyCompare(1.0 + pHost->mLineSize, 1.0)) {
        pHost->mLineSize = 10.0; // Same value as is in Host class initializer list
    }

    const QStringView ignore(attributes().value(qsl("mDoubleClickIgnore")));

    for (auto character : ignore) {
        pHost->mDoubleClickIgnore.insert(character);
    }

    if (attributes().hasAttribute(QLatin1String("EditorSearchOptions"))) {
        pHost->setSearchOptions(static_cast<dlgTriggerEditor::SearchOptions>(attributes().value(qsl("EditorSearchOptions")).toInt()));
    }

    pHost->setDebugShowAllProblemCodepoints(attributes().value(qsl("DebugShowAllProblemCodepoints")) == YES);

    const bool compactInputLine = attributes().value(QLatin1String("CompactInputLine")) == YES;
    pHost->setCompactInputLine(compactInputLine);

    if (mudlet::self()->mpCurrentActiveHost == pHost) {
        mudlet::self()->dactionInputLine->setChecked(compactInputLine);
    }

    if (attributes().hasAttribute(QLatin1String("CommandLineHistorySaveSize"))) {
        pHost->setCommandLineHistorySaveSize(attributes().value(QLatin1String("CommandLineHistorySaveSize")).toInt());
    } else {
        // This is the default value, though prior to the introduction of this
        // it would have effectively been zero:
        pHost->setCommandLineHistorySaveSize(500);
    }

    if (attributes().hasAttribute(QLatin1String("NetworkPacketTimeout"))) {
        // These limits are also hard coded into the QSpinBox used to adjust
        // this setting in the preferences:
        pHost->mTelnet.setPostingTimeout(qBound(10, attributes().value(QLatin1String("NetworkPacketTimeout")).toInt(), 500));
    } else {
        // The default value, also used up to Mudlet 4.12.0:
        pHost->mTelnet.setPostingTimeout(300);
    }

    if (attributes().hasAttribute(QLatin1String("ControlCharacterHandling"))) {
        switch (attributes().value(QLatin1String("ControlCharacterHandling")).toInt()) {
        case 1:
            pHost->setControlCharacterMode(ControlCharacterMode::Picture);
            break;
        case 2:
            pHost->setControlCharacterMode(ControlCharacterMode::OEM);
            break;
        case 0:
            [[fallthrough]];
        default:
            pHost->setControlCharacterMode(ControlCharacterMode::AsIs);
        }
    } else {
        // The default value, also used up to Mudlet 4.14.1:
        pHost->setControlCharacterMode(ControlCharacterMode::AsIs);
    }

    if (attributes().hasAttribute(qsl("ShowIDsInEditor"))) {
        pHost->setShowIdsInEditor(attributes().value(qsl("ShowIDsInEditor")) == YES);
    } else {
        // The default (and for profile files from before 4.18.0):
        pHost->setShowIdsInEditor(false);
    }

    if (attributes().hasAttribute(qsl("Large2DMapAreaExitArrows"))) {
        pHost->setLargeAreaExitArrows(attributes().value(qsl("Large2DMapAreaExitArrows")) == YES);
    } else {
        // The default (and for map/profile files from before 4.15.0):
        pHost->setLargeAreaExitArrows(false);
    }

    if (attributes().value(qsl("mShowInfo")) == qsl("no")) {
        mpHost->mMapInfoContributors.clear();
    }

    QMargins borders;

    while (!atEnd()) {
        readNext();

        if (isEndElement()) {
            break;
        }
        if (isStartElement()) {
            if (name() == qsl("name")) {
                // Only read this detail into a backup location so that it can
                // be imported without changing the main setting unless it is
                // needed (intended for use when importing a profile but not
                // otherwise). In fact this detail is normally stored outside of
                // the game save in the profile base directory:
                pHost->mBackupHostName = readElementText();
            } else if (name() == qsl("mInstalledModules")) {
                QMap<QString, QStringList> entry;

                readModulesDetailsMap(entry);

                QMapIterator<QString, QStringList> it(entry);

                while (it.hasNext()) {
                    it.next();
                    QStringList moduleList;
                    const QStringList entryList = it.value();
                    moduleList << entryList.at(0);
                    moduleList << entryList.at(1);
                    pHost->mInstalledModules[it.key()] = moduleList;
                    pHost->mModulePriorities[it.key()] = entryList.at(2).toInt();
                }
            } else if (name() == qsl("mInstalledPackages")) {
                readStringList(pHost->mInstalledPackages, qsl("Host"));
            } else if (name() == qsl("url")) {
                // Only read this detail into a backup location so that it can
                // be imported without changing the main setting unless it is
                // needed (intended for use when importing a profile but not
                // otherwise). In fact this detail is normally stored outside of
                // the game save in the profile base directory:
                pHost->mBackupUrl = readElementText();
            } else if (name() == qsl("serverPackageName")) {
                pHost->mServerGUI_Package_name = readElementText();
            } else if (name() == qsl("serverPackageVersion")) {
                pHost->mServerGUI_Package_version = readElementText();
            } else if (name() == qsl("port")) {
                // Only read this detail into a backup location so that it can
                // be imported without changing the main setting unless it is
                // needed (intended for use when importing a profile but not
                // otherwise). In fact this detail is normally stored outside of
                // the game save in the profile base directory:
                pHost->mBackupPort = readElementText().toInt();
            } else if (name() == qsl("borderTopHeight")) {
                borders.setTop(readElementText().toInt());
            } else if (name() == qsl("borderBottomHeight")) {
                borders.setBottom(readElementText().toInt());
            } else if (name() == qsl("borderLeftWidth")) {
                borders.setLeft(readElementText().toInt());
            } else if (name() == qsl("borderRightWidth")) {
                borders.setRight(readElementText().toInt());
            } else if (name() == qsl("commandLineMinimumHeight")) {
                pHost->commandLineMinimumHeight = readElementText().toInt();
            } else if (name() == qsl("wrapAt")) {
                pHost->mWrapAt = readElementText().toInt();
            } else if (name() == qsl("wrapIndentCount")) {
                pHost->mWrapIndentCount = readElementText().toInt();
            } else if (name() == qsl("wrapHangingIndentCount")) {
                pHost->mWrapHangingIndentCount = readElementText().toInt();
            } else if (name() == qsl("mCommandSeparator")) {
                pHost->mCommandSeparator = readElementText();
            } else if (name() == qsl("mCommandLineFgColor")) {
#if QT_VERSION < QT_VERSION_CHECK(6, 6, 0)
                pHost->mCommandLineFgColor.setNamedColor(readElementText());
            } else if (name() == qsl("mCommandLineBgColor")) {
                pHost->mCommandLineBgColor.setNamedColor(readElementText());
            } else if (name() == qsl("mFgColor")) {
                pHost->mFgColor.setNamedColor(readElementText());
            } else if (name() == qsl("mBgColor")) {
                pHost->mBgColor.setNamedColor(readElementText());
            } else if (name() == qsl("mCommandFgColor")) {
                pHost->mCommandFgColor.setNamedColor(readElementText());
            } else if (name() == qsl("mCommandBgColor")) {
                pHost->mCommandBgColor.setNamedColor(readElementText());
            } else if (name() == qsl("mBlack")) {
                pHost->mBlack.setNamedColor(readElementText());
            } else if (name() == qsl("mLightBlack")) {
                pHost->mLightBlack.setNamedColor(readElementText());
            } else if (name() == qsl("mRed")) {
                pHost->mRed.setNamedColor(readElementText());
            } else if (name() == qsl("mLightRed")) {
                pHost->mLightRed.setNamedColor(readElementText());
            } else if (name() == qsl("mBlue")) {
                pHost->mBlue.setNamedColor(readElementText());
            } else if (name() == qsl("mLightBlue")) {
                pHost->mLightBlue.setNamedColor(readElementText());
            } else if (name() == qsl("mGreen")) {
                pHost->mGreen.setNamedColor(readElementText());
            } else if (name() == qsl("mLightGreen")) {
                pHost->mLightGreen.setNamedColor(readElementText());
            } else if (name() == qsl("mYellow")) {
                pHost->mYellow.setNamedColor(readElementText());
            } else if (name() == qsl("mLightYellow")) {
                pHost->mLightYellow.setNamedColor(readElementText());
            } else if (name() == qsl("mCyan")) {
                pHost->mCyan.setNamedColor(readElementText());
            } else if (name() == qsl("mLightCyan")) {
                pHost->mLightCyan.setNamedColor(readElementText());
            } else if (name() == qsl("mMagenta")) {
                pHost->mMagenta.setNamedColor(readElementText());
            } else if (name() == qsl("mLightMagenta")) {
                pHost->mLightMagenta.setNamedColor(readElementText());
            } else if (name() == qsl("mWhite")) {
                pHost->mWhite.setNamedColor(readElementText());
            } else if (name() == qsl("mLightWhite")) {
                pHost->mLightWhite.setNamedColor(readElementText());
#else
                pHost->mCommandLineFgColor = QColor::fromString(readElementText());
            } else if (name() == qsl("mCommandLineBgColor")) {
                pHost->mCommandLineBgColor = QColor::fromString(readElementText());
            } else if (name() == qsl("mFgColor")) {
                pHost->mFgColor = QColor::fromString(readElementText());
            } else if (name() == qsl("mBgColor")) {
                pHost->mBgColor = QColor::fromString(readElementText());
            } else if (name() == qsl("mCommandFgColor")) {
                pHost->mCommandFgColor = QColor::fromString(readElementText());
            } else if (name() == qsl("mCommandBgColor")) {
                pHost->mCommandBgColor = QColor::fromString(readElementText());
            } else if (name() == qsl("mBlack")) {
                pHost->mBlack = QColor::fromString(readElementText());
            } else if (name() == qsl("mLightBlack")) {
                pHost->mLightBlack = QColor::fromString(readElementText());
            } else if (name() == qsl("mRed")) {
                pHost->mRed = QColor::fromString(readElementText());
            } else if (name() == qsl("mLightRed")) {
                pHost->mLightRed = QColor::fromString(readElementText());
            } else if (name() == qsl("mBlue")) {
                pHost->mBlue = QColor::fromString(readElementText());
            } else if (name() == qsl("mLightBlue")) {
                pHost->mLightBlue = QColor::fromString(readElementText());
            } else if (name() == qsl("mGreen")) {
                pHost->mGreen = QColor::fromString(readElementText());
            } else if (name() == qsl("mLightGreen")) {
                pHost->mLightGreen = QColor::fromString(readElementText());
            } else if (name() == qsl("mYellow")) {
                pHost->mYellow = QColor::fromString(readElementText());
            } else if (name() == qsl("mLightYellow")) {
                pHost->mLightYellow = QColor::fromString(readElementText());
            } else if (name() == qsl("mCyan")) {
                pHost->mCyan = QColor::fromString(readElementText());
            } else if (name() == qsl("mLightCyan")) {
                pHost->mLightCyan = QColor::fromString(readElementText());
            } else if (name() == qsl("mMagenta")) {
                pHost->mMagenta = QColor::fromString(readElementText());
            } else if (name() == qsl("mLightMagenta")) {
                pHost->mLightMagenta = QColor::fromString(readElementText());
            } else if (name() == qsl("mWhite")) {
                pHost->mWhite = QColor::fromString(readElementText());
            } else if (name() == qsl("mLightWhite")) {
                pHost->mLightWhite = QColor::fromString(readElementText());
#endif
            } else if (name() == qsl("mDisplayFont")) {
                pHost->setDisplayFontFromString(readElementText());
#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
                // On GNU/Linux and FreeBSD ensure that emojis are displayed in
                // colour even if this font doesn't support it:
                QFont::insertSubstitution(pHost->getDisplayFont().family(), qsl("Noto Color Emoji"));
#endif
            } else if (name() == qsl("mCommandLineFont")) {
                // We use the same font as the main console now so discard this
                // one silently:
                Q_UNUSED(readElementText())
            } else if (name() == qsl("commandSeperator")) {
                // Ignore this misspelled duplicate, it has been removed from
                // the Xml format but will appear in older files and trip the
                // QDebug() error reporting associated with the following
                // readUnknownElement(...) for "anything not otherwise parsed"
                Q_UNUSED(readElementText())
            } else if (name() == qsl("mFgColor2")) {
#if QT_VERSION < QT_VERSION_CHECK(6, 6, 0)
                pHost->mFgColor_2.setNamedColor(readElementText());
            } else if (name() == qsl("mBgColor2")) {
                pHost->mBgColor_2.setNamedColor(readElementText());
            } else if (name() == qsl("mLowerLevelColor")) {
                pHost->mLowerLevelColor.setNamedColor(readElementText());
            } else if (name() == qsl("mUpperLevelColor")) {
                pHost->mUpperLevelColor.setNamedColor(readElementText());
            } else if (name() == qsl("mRoomBorderColor")) {
                pHost->mRoomBorderColor.setNamedColor(readElementText());
            } else if (name() == qsl("mRoomCollisionBorderColor")) {
                pHost->mRoomCollisionBorderColor.setNamedColor(readElementText());
            } else if (name() == qsl("mMapInfoBg")) {
                auto alpha = (attributes().hasAttribute(qsl("alpha"))) ? attributes().value(qsl("alpha")).toInt() : 255;
                pHost->mMapInfoBg.setNamedColor(readElementText());
                pHost->mMapInfoBg.setAlpha(alpha);
            } else if (name() == qsl("mBlack2")) {
                pHost->mBlack_2.setNamedColor(readElementText());
            } else if (name() == qsl("mLightBlack2")) {
                pHost->mLightBlack_2.setNamedColor(readElementText());
            } else if (name() == qsl("mRed2")) {
                pHost->mRed_2.setNamedColor(readElementText());
            } else if (name() == qsl("mLightRed2")) {
                pHost->mLightRed_2.setNamedColor(readElementText());
            } else if (name() == qsl("mBlue2")) {
                pHost->mBlue_2.setNamedColor(readElementText());
            } else if (name() == qsl("mLightBlue2")) {
                pHost->mLightBlue_2.setNamedColor(readElementText());
            } else if (name() == qsl("mGreen2")) {
                pHost->mGreen_2.setNamedColor(readElementText());
            } else if (name() == qsl("mLightGreen2")) {
                pHost->mLightGreen_2.setNamedColor(readElementText());
            } else if (name() == qsl("mYellow2")) {
                pHost->mYellow_2.setNamedColor(readElementText());
            } else if (name() == qsl("mLightYellow2")) {
                pHost->mLightYellow_2.setNamedColor(readElementText());
            } else if (name() == qsl("mCyan2")) {
                pHost->mCyan_2.setNamedColor(readElementText());
            } else if (name() == qsl("mLightCyan2")) {
                pHost->mLightCyan_2.setNamedColor(readElementText());
            } else if (name() == qsl("mMagenta2")) {
                pHost->mMagenta_2.setNamedColor(readElementText());
            } else if (name() == qsl("mLightMagenta2")) {
                pHost->mLightMagenta_2.setNamedColor(readElementText());
            } else if (name() == qsl("mWhite2")) {
                pHost->mWhite_2.setNamedColor(readElementText());
            } else if (name() == qsl("mLightWhite2")) {
                pHost->mLightWhite_2.setNamedColor(readElementText());
#else
                pHost->mFgColor_2 = QColor::fromString(readElementText());
            } else if (name() == qsl("mBgColor2")) {
                pHost->mBgColor_2 = QColor::fromString(readElementText());
            } else if (name() == qsl("mLowerLevelColor")) {
                pHost->mLowerLevelColor = QColor::fromString(readElementText());
            } else if (name() == qsl("mUpperLevelColor")) {
                pHost->mUpperLevelColor = QColor::fromString(readElementText());
            } else if (name() == qsl("mRoomBorderColor")) {
                pHost->mRoomBorderColor = QColor::fromString(readElementText());
            } else if (name() == qsl("mRoomCollisionBorderColor")) {
                pHost->mRoomCollisionBorderColor = QColor::fromString(readElementText());
            } else if (name() == qsl("mMapInfoBg")) {
                auto alpha = (attributes().hasAttribute(qsl("alpha"))) ? attributes().value(qsl("alpha")).toInt() : 255;
                pHost->mMapInfoBg = QColor::fromString(readElementText());
                pHost->mMapInfoBg.setAlpha(alpha);
            } else if (name() == qsl("mBlack2")) {
                pHost->mBlack_2 = QColor::fromString(readElementText());
            } else if (name() == qsl("mLightBlack2")) {
                pHost->mLightBlack_2 = QColor::fromString(readElementText());
            } else if (name() == qsl("mRed2")) {
                pHost->mRed_2 = QColor::fromString(readElementText());
            } else if (name() == qsl("mLightRed2")) {
                pHost->mLightRed_2 = QColor::fromString(readElementText());
            } else if (name() == qsl("mBlue2")) {
                pHost->mBlue_2 = QColor::fromString(readElementText());
            } else if (name() == qsl("mLightBlue2")) {
                pHost->mLightBlue_2 = QColor::fromString(readElementText());
            } else if (name() == qsl("mGreen2")) {
                pHost->mGreen_2 = QColor::fromString(readElementText());
            } else if (name() == qsl("mLightGreen2")) {
                pHost->mLightGreen_2 = QColor::fromString(readElementText());
            } else if (name() == qsl("mYellow2")) {
                pHost->mYellow_2 = QColor::fromString(readElementText());
            } else if (name() == qsl("mLightYellow2")) {
                pHost->mLightYellow_2 = QColor::fromString(readElementText());
            } else if (name() == qsl("mCyan2")) {
                pHost->mCyan_2 = QColor::fromString(readElementText());
            } else if (name() == qsl("mLightCyan2")) {
                pHost->mLightCyan_2 = QColor::fromString(readElementText());
            } else if (name() == qsl("mMagenta2")) {
                pHost->mMagenta_2 = QColor::fromString(readElementText());
            } else if (name() == qsl("mLightMagenta2")) {
                pHost->mLightMagenta_2 = QColor::fromString(readElementText());
            } else if (name() == qsl("mWhite2")) {
                pHost->mWhite_2 = QColor::fromString(readElementText());
            } else if (name() == qsl("mLightWhite2")) {
                pHost->mLightWhite_2 = QColor::fromString(readElementText());
#endif
            } else if (name() == qsl("mSpellDic")) {
                pHost->setSpellDic(readElementText());
            } else if (name() == qsl("mLineSize") || name() == qsl("mRoomSize")) {
                // These two have been dropped from the Xml format as these are
                // duplicates of attributes that were being incorrected read in
                // the parent <Host ...> element as integers {they are stored as
                // decimals but for the first one at least, it is a decimal
                // number n, where 0.1 <= n <= 1.1 so was being read as "0" for
                // all but the greatest 2 values where it was read as "1"!}
                // We still check for them so that we avoid falling into the
                // QDebug() error reporting associated with the following
                // readUnknownElement(...) for "anything not otherwise parsed"
                Q_UNUSED(readElementText())
            } else if (name() == qsl("mMapInfoContributors")) {
                readLegacyMapInfoContributors();
            } else if (name() == qsl("mapInfoContributor")) {
                readMapInfoContributor();
            } else if (name() == qsl("profileShortcut")) {
                readProfileShortcut();
            } else if (name() == qsl("stopwatches")) {
                readStopWatchMap();
            } else {
                readUnknownElement(qsl("Host"));
            }
        }
    }

    pHost->setBorders(borders);
    pHost->loadPackageInfo();
}

bool XMLimport::readDefaultTrueBool(QString name)
{
    return attributes().value(name) == YES || !attributes().hasAttribute(name);
}

// returns the ID of the root imported trigger/group
int XMLimport::readTriggerPackage()
{
    int parentItemID = -1;

    while (!atEnd()) {
        readNext();
        if (isEndElement()) {
            break;
        }

        if (isStartElement()) {
            if (name() == qsl("TriggerGroup") || name() == qsl("Trigger")) {
                gotTrigger = true;
                parentItemID = readTrigger(mPackageName.isEmpty() ? nullptr : mpTrigger);
            } else {
                readUnknownElement(qsl("TriggerPackage"));
            }
        }
    }

    return parentItemID;
}

// imports a trigger and returns its ID - in case of a group, returns the ID
// of the top-level trigger group.
int XMLimport::readTrigger(TTrigger* pParent)
{
    auto pT = new TTrigger(pParent, mpHost);

    if (module) {
        pT->mModuleMember = true;
    }

    mpHost->getTriggerUnit()->registerTrigger(pT);

    pT->setIsActive(attributes().value(qsl("isActive")) == YES);
    pT->setIsFolder(attributes().value(qsl("isFolder")) == YES);
    pT->setTemporary(attributes().value(qsl("isTempTrigger")) == YES);
    pT->mIsMultiline = attributes().value(qsl("isMultiline")) == YES;
    pT->mPerlSlashGOption = attributes().value(qsl("isPerlSlashGOption")) == YES;
    pT->mIsColorizerTrigger = attributes().value(qsl("isColorizerTrigger")) == YES;
    pT->mFilterTrigger = attributes().value(qsl("isFilterTrigger")) == YES;
    pT->mSoundTrigger = attributes().value(qsl("isSoundTrigger")) == YES;
    pT->mColorTrigger = attributes().value(qsl("isColorTrigger")) == YES;

    // Is this a "TriggerGroup" or a "Trigger"
    const QString what = name().toString();
    while (!atEnd()) {
        readNext();

        if (isEndElement()) {
            break;
        } else if (isStartElement()) {
            if (name() == qsl("name")) {
                pT->setName(readElementText());
            } else if (name() == qsl("script")) {
                const QString tempScript = readScriptElement();
                if (!pT->setScript(tempScript)) {
                    qDebug().nospace() << "XMLimport::readTrigger(...): ERROR: can not compile trigger's lua code for: " << pT->getName();
                }
            } else if (name() == qsl("packageName")) {
                pT->mPackageName = readElementText();
            } else if (name() == qsl("triggerType")) {
                pT->mTriggerType = readElementText().toInt();
            } else if (name() == qsl("conditonLineDelta")) {
                pT->mConditionLineDelta = readElementText().toInt();
            } else if (name() == qsl("mStayOpen")) {
                pT->mStayOpen = readElementText().toInt();
            } else if (name() == qsl("mCommand")) {
                pT->mCommand = readElementText();
            } else if (name() == qsl("mFgColor")) {
#if QT_VERSION < QT_VERSION_CHECK(6, 6, 0)
                pT->mFgColor.setNamedColor(readElementText());
            } else if (name() == qsl("mBgColor")) {
                pT->mBgColor.setNamedColor(readElementText());
            } else if (name() == qsl("colorTriggerFgColor")) {
                pT->mColorTriggerFgColor.setNamedColor(readElementText());
            } else if (name() == qsl("colorTriggerBgColor")) {
                pT->mColorTriggerBgColor.setNamedColor(readElementText());
#else
                pT->mFgColor = QColor::fromString(readElementText());
            } else if (name() == qsl("mBgColor")) {
                pT->mBgColor = QColor::fromString(readElementText());
            } else if (name() == qsl("colorTriggerFgColor")) {
                pT->mColorTriggerFgColor = QColor::fromString(readElementText());
            } else if (name() == qsl("colorTriggerBgColor")) {
                pT->mColorTriggerBgColor = QColor::fromString(readElementText());
#endif
            } else if (name() == qsl("mSoundFile")) {
                pT->mSoundFile = readElementText();
            } else if (name() == qsl("regexCodeList")) {
                // This and the next one ought to be combined into a single element
                // in the next revision - sample code for "RegexCode" elements
                // inside a "patterns" container (with a "size" attribute) is
                // commented out in the XMLexporter class.
                readStringList(pT->mPatterns, what);
            } else if (name() == qsl("regexCodePropertyList")) {
                readIntegerList(pT->mPatternKinds, pT->getName(), what);
                if (Q_UNLIKELY(pT->mPatterns.count() != pT->mPatternKinds.count())) {
                    qWarning().nospace() << "XMLimport::readTrigger(...) ERROR: "
                                            "mismatch in regexCode details for Trigger: "
                                         << pT->getName() << " there were " << pT->mPatterns.count() << " 'regexCodeList' sub-elements and " << pT->mPatternKinds.count()
                                         << " 'regexCodePropertyList' sub-elements so "
                                            "something is broken!";
                }
                // Fixup the first 16 incorrect ANSI colour numbers from old
                // code if there are any
                if (!pT->mPatterns.isEmpty()) {
                    remapColorsToAnsiNumber(pT->mPatterns, pT->mPatternKinds);
                }
            } else if (name() == qsl("TriggerGroup") || name() == qsl("Trigger")) {
                readTrigger(pT);
            } else {
                readUnknownElement(what);
            }
        }
    }

    if (!pT->setRegexCodeList(pT->mPatterns, pT->mPatternKinds)) {
        qDebug().nospace() << "XMLimport::readTrigger(...): ERROR: can not "
                              "initialize pattern list for trigger: "
                           << pT->getName();
    }

    return pT->getID();
}

int XMLimport::readTimerPackage()
{
    int lastImportedTimerID = -1;

    while (!atEnd()) {
        readNext();
        if (isEndElement()) {
            break;
        } else if (isStartElement()) {
            if (name() == qsl("TimerGroup") || name() == qsl("Timer")) {
                gotTimer = true;
                lastImportedTimerID = readTimer(mPackageName.isEmpty() ? nullptr : mpTimer);
            } else {
                readUnknownElement(qsl("TimerPackage"));
            }
        }
    }

    return lastImportedTimerID;
}

int XMLimport::readTimer(TTimer* pParent)
{
    auto pT = new TTimer(pParent, mpHost);

    pT->setIsFolder(attributes().value(qsl("isFolder")) == YES);
    // This should not ever be set here as, by definition, temporary timers
    // are not saved:
    pT->setTemporary(attributes().value(qsl("isTempTimer")) == YES);

    // This clears the Tree<TTimer>::mUserActiveState flag so MUST be done
    // BEFORE that flag is parsed:
    mpHost->getTimerUnit()->registerTimer(pT);

    pT->setShouldBeActive(attributes().value(qsl("isActive")) == YES);

    if (module) {
        pT->mModuleMember = true;
    }

    const QString what = name().toString();
    while (!atEnd()) {
        readNext();
        if (isEndElement()) {
            break;
        } else if (isStartElement()) {
            if (name() == qsl("name")) {
                pT->setName(readElementText());
            } else if (name() == qsl("packageName")) {
                pT->mPackageName = readElementText();
            } else if (name() == qsl("script")) {
                const QString tempScript = readScriptElement();
                if (!pT->setScript(tempScript)) {
                    qDebug().nospace() << "XMLimport::readTimer(...): ERROR: can not compile timer's lua code for: " << pT->getName();
                }
            } else if (name() == qsl("command")) {
                pT->mCommand = readElementText();
            } else if (name() == qsl("time")) {
                pT->setTime(QTime::fromString(readElementText(), "hh:mm:ss.zzz"));
            } else if (name() == qsl("TimerGroup") || name() == qsl("Timer")) {
                readTimer(pT);
            } else {
                readUnknownElement(what);
            }
        }
    }

    if (!pT->mpParent && pT->shouldBeActive()) {
        pT->setIsActive(true);
        pT->enableTimer(pT->getID());
    }

    return pT->getID();
}

int XMLimport::readAliasPackage()
{
    int lastImportedAliasID = -1;

    while (!atEnd()) {
        readNext();
        if (isEndElement()) {
            break;
        } else if (isStartElement()) {
            if (name() == qsl("AliasGroup") || name() == qsl("Alias")) {
                gotAlias = true;
                lastImportedAliasID = readAlias(mPackageName.isEmpty() ? nullptr : mpAlias);
            } else {
                readUnknownElement(qsl("AliasPackage"));
            }
        }
    }

    return lastImportedAliasID;
}

int XMLimport::readAlias(TAlias* pParent)
{
    auto pT = new TAlias(pParent, mpHost);

    mpHost->getAliasUnit()->registerAlias(pT);
    pT->setIsActive(attributes().value(qsl("isActive")) == YES);
    pT->setIsFolder(attributes().value(qsl("isFolder")) == YES);
    if (module) {
        pT->mModuleMember = true;
    }

    const QString what = name().toString();
    while (!atEnd()) {
        readNext();

        if (isEndElement()) {
            break;
        } else if (isStartElement()) {
            if (name() == qsl("name")) {
                pT->setName(readElementText());
            } else if (name() == qsl("packageName")) {
                pT->mPackageName = readElementText();
            } else if (name() == qsl("script")) {
                const QString tempScript = readScriptElement();
                if (!pT->setScript(tempScript)) {
                    qDebug().nospace() << "XMLimport::readAlias(...): ERROR: can not compile alias's lua code for: " << pT->getName();
                }
            } else if (name() == qsl("command")) {
                pT->mCommand = readElementText();
            } else if (name() == qsl("regex")) {
                pT->setRegexCode(readElementText());
            } else if (name() == qsl("AliasGroup") || name() == qsl("Alias")) {
                readAlias(pT);
            } else {
                readUnknownElement(what);
            }
        }
    }

    return pT->getID();
}

int XMLimport::readActionPackage()
{
    int lastImportedActionID = -1;

    while (!atEnd()) {
        readNext();
        if (isEndElement()) {
            break;
        } else if (isStartElement()) {
            if (name() == qsl("ActionGroup") || name() == qsl("Action")) {
                gotAction = true;
                lastImportedActionID = readAction(mPackageName.isEmpty() ? nullptr : mpAction);
            } else {
                readUnknownElement(qsl("ActionPackage"));
            }
        }
    }

    return lastImportedActionID;
}

int XMLimport::readAction(TAction* pParent)
{
    auto pT = new TAction(pParent, mpHost);

    pT->setIsFolder(attributes().value(qsl("isFolder")) == YES);
    pT->mIsPushDownButton = attributes().value(qsl("isPushButton")) == YES;
    pT->mButtonFlat = attributes().value(qsl("isFlatButton")) == YES;
    pT->mUseCustomLayout = attributes().value(qsl("useCustomLayout")) == YES;
    mpHost->getActionUnit()->registerAction(pT);
    pT->setIsActive(attributes().value(qsl("isActive")) == YES);

    if (module) {
        pT->mModuleMember = true;
    }

    const QString what = name().toString();
    while (!atEnd()) {
        readNext();
        if (isEndElement()) {
            break;
        } else if (isStartElement()) {
            if (name() == qsl("name")) {
                pT->mName = readElementText();
            } else if (name() == qsl("packageName")) {
                pT->mPackageName = readElementText();
            } else if (name() == qsl("script")) {
                const QString tempScript = readScriptElement();
                if (!pT->setScript(tempScript)) {
                    qDebug().nospace() << "XMLimport::readAction(...): ERROR: can not compile action's lua code for: " << pT->getName();
                }
            } else if (name() == qsl("css")) {
                pT->css = readElementText();
            } else if (name() == qsl("commandButtonUp")) {
                pT->mCommandButtonUp = readElementText();
            } else if (name() == qsl("commandButtonDown")) {
                pT->mCommandButtonDown = readElementText();
            } else if (name() == qsl("icon")) {
                pT->mIcon = readElementText();
            } else if (name() == qsl("orientation")) {
                pT->mOrientation = readElementText().toInt();
            } else if (name() == qsl("location")) {
                pT->mLocation = readElementText().toInt();
            } else if (name() == qsl("buttonRotation")) {
                pT->mButtonRotation = readElementText().toInt();
            } else if (name() == qsl("sizeX")) {
                pT->mSizeX = readElementText().toInt();
            } else if (name() == qsl("sizeY")) {
                pT->mSizeY = readElementText().toInt();
            } else if (name() == qsl("mButtonState")) {
                // We now use a boolean but file must use original "1" (false)
                // or "2" (true) for backward compatibility
                pT->mButtonState = (readElementText().toInt() == 2);
            } else if (name() == qsl("buttonColor")) {
                // Not longer present/used, skip over it if it is still in file:
                skipCurrentElement();
            } else if (name() == qsl("buttonColumn")) {
                pT->mButtonColumns = readElementText().toInt();
            } else if (name() == qsl("posX")) {
                pT->mPosX = readElementText().toInt();
            } else if (name() == qsl("posY")) {
                pT->mPosY = readElementText().toInt();
            } else if (name() == qsl("ActionGroup") || name() == qsl("Action")) {
                readAction(pT);
            } else {
                readUnknownElement(what);
            }
        }
    }

    return pT->getID();
}

int XMLimport::readScriptPackage()
{
    int lastImportedScriptID = -1;

    while (!atEnd()) {
        readNext();
        if (isEndElement()) {
            break;
        } else if (isStartElement()) {
            if (name() == qsl("ScriptGroup") || name() == qsl("Script")) {
                gotScript = true;
                lastImportedScriptID = readScript(mPackageName.isEmpty() ? nullptr : mpScript);
            } else {
                readUnknownElement(qsl("ScriptPackage"));
            }
        }
    }

    return lastImportedScriptID;
}

int XMLimport::readScript(TScript* pParent)
{
    auto script = new TScript(pParent, mpHost);

    script->setIsFolder(attributes().value(qsl("isFolder")) == YES);
    mpHost->getScriptUnit()->registerScript(script);
    script->setIsActive(attributes().value(qsl("isActive")) == YES);

    if (module) {
        script->mModuleMember = true;
    }

    const QString what = name().toString();
    while (!atEnd()) {
        readNext();
        if (isEndElement()) {
            break;
        } else if (isStartElement()) {
            if (name() == qsl("name")) {
                script->mName = readElementText();
            } else if (name() == qsl("packageName")) {
                script->mPackageName = readElementText();
            } else if (name() == qsl("script")) {
                const QString tempScript = readScriptElement();
                if (!script->setScript(tempScript)) {
                    qDebug().nospace().noquote() << "XMLimport::readScript(...) ERROR - can not compile script's lua code for \"" << script->getName() << "\"; reason: " << script->getError() << ".";
                }
            } else if (name() == qsl("eventHandlerList")) {
                readStringList(script->mEventHandlerList, what);
                script->setEventHandlerList(script->mEventHandlerList);
            } else if (name() == qsl("ScriptGroup") || name() == qsl("Script")) {
                readScript(script);
            } else {
                readUnknownElement(what);
            }
        }
    }

    return script->getID();
}

int XMLimport::readKeyPackage()
{
    int lastImportedKeyID = -1;

    while (!atEnd()) {
        readNext();
        if (isEndElement()) {
            break;
        } else if (isStartElement()) {
            if (name() == qsl("KeyGroup") || name() == qsl("Key")) {
                gotKey = true;
                lastImportedKeyID = readKey(mPackageName.isEmpty() ? nullptr : mpKey);
            } else {
                readUnknownElement(qsl("KeyPackage"));
            }
        }
    }

    return lastImportedKeyID;
}

int XMLimport::readKey(TKey* pParent)
{
    auto pT = new TKey(pParent, mpHost);

    mpHost->getKeyUnit()->registerKey(pT);
    pT->setIsActive(attributes().value(qsl("isActive")) == YES);
    pT->setIsFolder(attributes().value(qsl("isFolder")) == YES);
    if (module) {
        pT->mModuleMember = true;
    }

    const QString what = name().toString();
    while (!atEnd()) {
        readNext();

        if (isEndElement()) {
            break;
        } else if (isStartElement()) {
            if (name() == qsl("name")) {
                pT->setName(readElementText());
            } else if (name() == qsl("packageName")) {
                pT->mPackageName = readElementText();
            } else if (name() == qsl("script")) {
                const QString tempScript = readScriptElement();
                if (!pT->setScript(tempScript)) {
                    qDebug().nospace() << "XMLimport::readKey(...): ERROR: can not compile key's lua code for: " << pT->getName();
                }
            } else if (name() == qsl("command")) {
                pT->mCommand = readElementText();
            } else if (name() == qsl("keyCode")) {
                pT->setKeyCode(readElementText().toInt());
            } else if (name() == qsl("keyModifier")) {
                pT->setKeyModifiers(readElementText().toInt());
            } else if (name() == qsl("KeyGroup") || name() == qsl("Key")) {
                readKey(pT);
            } else {
                readUnknownElement(what);
            }
        }
    }

    return pT->getID();
}

void XMLimport::readModulesDetailsMap(QMap<QString, QStringList>& map)
{
    QString key;
    QStringList entry;

    while (!atEnd()) {
        readNext();

        if (isEndElement()) {
            break;
        } else if (isStartElement()) {
            if (name() == qsl("key")) {
                key = readElementText();
            } else if (name() == qsl("filepath")) {
                entry << readElementText();
            } else if (name() == qsl("zipSync")) {
                entry << readElementText();
            } else if (name() == qsl("globalSave")) {
                if (entry.size() < 2) {
                    entry << readElementText();
                } else {
                    skipCurrentElement();
                }
            } else if (name() == qsl("priority")) {
                // The last expected detail for the entry - so store this
                // completed entry into the QMap
                entry << readElementText();
                map[key] = entry;
                entry.clear();
            } else {
                readUnknownElement(qsl("ModulesDetailsMap"));
            }
        }
    }
}

void XMLimport::readStringList(QStringList& list, const QString& whatIsParent)
{
    while (!atEnd()) {
        readNext();

        if (isEndElement()) {
            break;
        } else if (isStartElement()) {
            if (name() == qsl("string")) {
                list << readElementText();
            } else {
                readUnknownElement(whatIsParent);
            }
        }
    }
}

void XMLimport::readIntegerList(QList<int>& list, const QString& parentName, const QString& whatIsParent)
{
    while (!atEnd()) {
        readNext();

        if (isEndElement()) {
            break;
        } else if (isStartElement()) {
            if (name() == qsl("integer")) {
                const QString numberText = readElementText();
                bool ok = false;
                const int num = numberText.toInt(&ok, 10);
                if (Q_LIKELY(!numberText.isEmpty() && ok)) {
                    switch (num) {
                    case REGEX_SUBSTRING:
                        [[fallthrough]];
                    case REGEX_PERL:
                        [[fallthrough]];
                    case REGEX_BEGIN_OF_LINE_SUBSTRING:
                        [[fallthrough]];
                    case REGEX_EXACT_MATCH:
                        [[fallthrough]];
                    case REGEX_LUA_CODE:
                        [[fallthrough]];
                    case REGEX_LINE_SPACER:
                        [[fallthrough]];
                    case REGEX_COLOR_PATTERN:
                        [[fallthrough]];
                    case REGEX_PROMPT:
                        list << num;
                        break;
                    default:
                        mpHost->postMessage(qsl("[ ERROR ] - \"%1\" as a number when reading the 'regexCodePropertyList' element of the 'Trigger' or 'TriggerGroup' element \"%2\" cannot be understood by this version of Mudlet, is it from a later version? Converting it to a SUBSTRING type so the data can be shown but it will probably not work as expected.").arg(numberText, parentName));
                        list << REGEX_SUBSTRING; //Set it to the default type
                    }

                } else {
                    qWarning(R"(XMLimport::readIntegerList(...) ERROR: unable to convert: "%s" to a number when reading the 'regexCodePropertyList' element of the 'Trigger' or 'TriggerGroup' element "%s"!)",
                           numberText.toUtf8().constData(),
                           parentName.toUtf8().constData());
                    mpHost->postMessage(qsl("[ ERROR ] - Unable to convert: \"%1\" to a number when reading the 'regexCodePropertyList' element of the 'Trigger' or 'TriggerGroup' element \"%2\"!").arg(numberText, parentName));
                    list << REGEX_SUBSTRING; //Just assume most common one
                }
            } else {
                readUnknownElement(whatIsParent);
            }
        }
    }
}

// This will be a string representation of a decimal float with three places of
// decimals
void XMLimport::getVersionString(QString& versionString)
{
    versionString = QString::number((mVersionMajor * 1000 + mVersionMinor) / 1000.0, 'f', 3);
}

QString XMLimport::readScriptElement()
{
    QString localScript = readElementText();
    if (Error() != NoError) {
        qDebug() << "XMLimport::readScriptElement() ERROR:" << errorString();
    }

    if (mVersionMajor > 1 || (mVersionMajor == 1 && mVersionMinor > 0)) {
        // This is NOT the original version, so it will have control characters
        // encoded up using Object Replacement and Control Symbol (for relevant ASCII control code) code-points
        localScript.replace(qsl("\xFFFC\x2401"), QChar('\x01')); // SOH
        localScript.replace(qsl("\xFFFC\x2402"), QChar('\x02')); // STX
        localScript.replace(qsl("\xFFFC\x2403"), QChar('\x03')); // ETX
        localScript.replace(qsl("\xFFFC\x2404"), QChar('\x04')); // EOT
        localScript.replace(qsl("\xFFFC\x2405"), QChar('\x05')); // ENQ
        localScript.replace(qsl("\xFFFC\x2406"), QChar('\x06')); // ACK
        localScript.replace(qsl("\xFFFC\x2407"), QChar('\x07')); // BEL
        localScript.replace(qsl("\xFFFC\x2408"), QChar('\x08')); // BS
        localScript.replace(qsl("\xFFFC\x240B"), QChar('\x0B')); // VT
        localScript.replace(qsl("\xFFFC\x240C"), QChar('\x0C')); // FF
        localScript.replace(qsl("\xFFFC\x240E"), QChar('\x0E')); // SS
        localScript.replace(qsl("\xFFFC\x240F"), QChar('\x0F')); // SI
        localScript.replace(qsl("\xFFFC\x2410"), QChar('\x10')); // DLE
        localScript.replace(qsl("\xFFFC\x2411"), QChar('\x11')); // DC1
        localScript.replace(qsl("\xFFFC\x2412"), QChar('\x12')); // DC2
        localScript.replace(qsl("\xFFFC\x2413"), QChar('\x13')); // DC3
        localScript.replace(qsl("\xFFFC\x2414"), QChar('\x14')); // DC4
        localScript.replace(qsl("\xFFFC\x2415"), QChar('\x15')); // NAK
        localScript.replace(qsl("\xFFFC\x2416"), QChar('\x16')); // SYN
        localScript.replace(qsl("\xFFFC\x2417"), QChar('\x17')); // ETB
        localScript.replace(qsl("\xFFFC\x2418"), QChar('\x18')); // CAN
        localScript.replace(qsl("\xFFFC\x2419"), QChar('\x19')); // EM
        localScript.replace(qsl("\xFFFC\x241A"), QChar('\x1A')); // SUB
        localScript.replace(qsl("\xFFFC\x241B"), QChar('\x1B')); // ESC
        localScript.replace(qsl("\xFFFC\x241C"), QChar('\x1C')); // FS
        localScript.replace(qsl("\xFFFC\x241D"), QChar('\x1D')); // GS
        localScript.replace(qsl("\xFFFC\x241E"), QChar('\x1E')); // RS
        localScript.replace(qsl("\xFFFC\x241F"), QChar('\x1F')); // US
        localScript.replace(qsl("\xFFFC\x2421"), QChar('\x7F')); // DEL
    }

    return localScript;
}

// Unlike the reverse operation in the XMLexport this can modify the supplied patternList:
void XMLimport::remapColorsToAnsiNumber(QStringList & patternList, const QList<int> & typeList)
{
    // The regexp is slightly modified compared to the one we once used to allow
    // it to capture a '-' sign as part of the color numbers as we use -2 for
    // ignored which was/is/will not handled by code before Mudlet 3.17.x (and
    // we might have more  negative numbers in the future!)
    const QRegularExpression regex = QRegularExpression(qsl("FG(-?\\d+)BG(-?\\d+)"));
    QMutableStringListIterator itPattern(patternList);
    QListIterator<int> itType(typeList);
    while (itPattern.hasNext() && itType.hasNext()) {
        if (itType.next() == REGEX_COLOR_PATTERN) {
            const QRegularExpressionMatch match = regex.match(itPattern.next());
            // Although we define two '('...')' capture groups the count/size is
            // 3 (0 is the whole string)!
            if (match.capturedTexts().size() == 3) {
                bool isFgOk = false;
                bool isBgOk = false;
                int ansifg = TTrigger::scmIgnored;
                int ansibg = TTrigger::scmIgnored;
                int fg = match.captured(1).toInt(&isFgOk);
                if (!isFgOk) {
                    qDebug() << "XMLimport::remapColorsToAnsiNumber(...) ERROR - failed to extract FG color code from pattern text:" << itPattern.peekPrevious() << " setting colour to default foreground";
                    fg = TTrigger::scmDefault;
                } else {
                    // clang-format off
                    switch (fg) {
                    case -2:    ansifg = TTrigger::scmIgnored;  break; // Ignored colour - not handled by old code
                    case 0:     ansifg = TTrigger::scmDefault;  break; // Default colour
                    case 1:     ansifg = 8;     break; // Light black (dark gray)
                    case 2:     ansifg = 0;     break; // Black
                    case 3:     ansifg = 9;     break; // Light red
                    case 4:     ansifg = 1;     break; // Red
                    case 5:     ansifg = 10;    break; // Light green
                    case 6:     ansifg = 2;     break; // Green
                    case 7:     ansifg = 11;    break; // Light yellow
                    case 8:     ansifg = 3;     break; // Yellow
                    case 9:     ansifg = 12;    break; // Light blue
                    case 10:    ansifg = 4;     break; // Blue
                    case 11:    ansifg = 13;    break; // Light magenta
                    case 12:    ansifg = 5;     break; // Magenta
                    case 13:    ansifg = 14;    break; // Light cyan
                    case 14:    ansifg = 6;     break; // Cyan
                    case 15:    ansifg = 15;    break; // Light white
                    case 16:    ansifg = 7;     break; // White (light gray)
                    default:
                        ansifg = fg;
                    }
                    // clang-format on
                }

                int bg = match.captured(2).toInt(&isBgOk);
                if (!isBgOk) {
                    qDebug() << "XMLimport::remapColorsToAnsiNumber(...) ERROR - failed to extract BG color code from pattern text:" << itPattern.peekPrevious() << " setting colour to default background";
                    bg = TTrigger::scmDefault;
                } else {
                    // clang-format off
                    switch (bg) {
                    case -2:    ansibg = TTrigger::scmIgnored;  break; // Ignored colour - not handled by old code
                    case 0:     ansibg = TTrigger::scmDefault;  break; // Default colour
                    case 1:     ansibg = 8;     break; // Light black (dark gray)
                    case 2:     ansibg = 0;     break; // Black
                    case 3:     ansibg = 9;     break; // Light red
                    case 4:     ansibg = 1;     break; // Red
                    case 5:     ansibg = 10;    break; // Light green
                    case 6:     ansibg = 2;     break; // Green
                    case 7:     ansibg = 11;    break; // Light yellow
                    case 8:     ansibg = 3;     break; // Yellow
                    case 9:     ansibg = 12;    break; // Light blue
                    case 10:    ansibg = 4;     break; // Blue
                    case 11:    ansibg = 13;    break; // Light magenta
                    case 12:    ansibg = 5;     break; // Magenta
                    case 13:    ansibg = 14;    break; // Light cyan
                    case 14:    ansibg = 6;     break; // Cyan
                    case 15:    ansibg = 15;    break; // Light white
                    case 16:    ansibg = 7;     break; // White (light gray)
                    default:
                        ansibg = bg;
                    }
                    // clang-format on
                }

                // Use a different string than before so that we can be certain
                // we have fixed up all cases where it is used - and it is more
                // understandable if it gets revealed in the Editor!
                itPattern.setValue(TTrigger::createColorPatternText(ansifg, ansibg));
            }

        } else {
            // Must advance the pattern iterator if it isn't a colour pattern
            itPattern.next();
        }
    }
}

void XMLimport::readStopWatchMap()
{
    while (!atEnd()) {
        readNext();

        if (isEndElement()) {
            break;
        } else if (isStartElement()) {
            if (name() == qsl("stopwatch")) {
                const int watchId = attributes().value(qsl("id")).toInt();
                auto pStopWatch = new stopWatch();
                pStopWatch->setName(attributes().value(qsl("name")).toString());
                pStopWatch->mIsPersistent = true;
                pStopWatch->mIsInitialised = true;
                if (attributes().value(qsl("running")) == YES) {
                    pStopWatch->mIsRunning = true;
                    // The stored value is the point in epoch time that the
                    // stopwatch appears to have been started so we need to
                    // make that into a QDateTime that is the equivalent:
                    pStopWatch->mEffectiveStartDateTime.setMSecsSinceEpoch(attributes().value(qsl("effectiveStartDateTimeEpochMSecs")).toLongLong());
                } else {
                    pStopWatch->mIsRunning = false;
                    pStopWatch->mElapsedTime = attributes().value(qsl("elapsedDateTimeMSecs")).toLongLong();
                }
                mpHost->mStopWatchMap.insert(watchId, pStopWatch);
                // A dummy read as there should not be any text for this element:
                readElementText();
            } else {
                readUnknownElement("stopwatches");
            }
        }
    }

}

void XMLimport::readMapInfoContributor()
{
    mpHost->mMapInfoContributors.insert(readElementText());
}

void XMLimport::readLegacyMapInfoContributors()
{
    while (!atEnd()) {
        readNext();
        if (isEndElement()) {
            break;
        }
        if (isStartElement()) {
            if (name() == qsl("mapInfoContributor")) {
                mpHost->mMapInfoContributors.insert(readElementText());
            }
        }
    }
}

void XMLimport::readProfileShortcut()
{
    auto key = attributes().value(qsl("key"));
    auto sequenceString = readElementText();
    if (mpHost->profileShortcuts.value(key.toString())) {
        QKeySequence* sequence = !sequenceString.isEmpty() ? new QKeySequence(sequenceString) : new QKeySequence();
        mpHost->profileShortcuts.value(key.toString())->swap(*sequence);
        delete sequence;
    }
}
