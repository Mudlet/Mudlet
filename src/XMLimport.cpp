/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2016-2022 by Stephen Lyons - slysven@virginmedia.com    *
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
#include "TConsole.h"
#include "TMap.h"
#include "TRoomDB.h"
#include "TRoom.h"
#include "VarUnit.h"
#include "mudlet.h"

#include "pre_guard.h"
#include <QBuffer>
#include <QtMath>
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

bool XMLimport::importPackage(QFile* pfile, QString packName, int moduleFlag, QString* pVersionString)
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
        mpKey->setPackageName(mPackageName);
        mpKey->setIsActive(true);
        mpKey->setName(mPackageName);
        mpKey->setIsFolder(true);

        mpTrigger = new TTrigger(nullptr, mpHost);
        if (module) {
            mpTrigger->mModuleMasterFolder = true;
            mpTrigger->mModuleMember = true;
        }
        mpTrigger->setPackageName(mPackageName);
        mpTrigger->setIsActive(true);
        mpTrigger->setName(mPackageName);
        mpTrigger->setIsFolder(true);

        mpTimer = new TTimer(nullptr, mpHost);
        if (module) {
            mpTimer->mModuleMasterFolder = true;
            mpTimer->mModuleMember = true;
        }
        mpTimer->setPackageName(mPackageName);
        mpTimer->setIsActive(true);
        mpTimer->setName(mPackageName);
        mpTimer->setIsFolder(true);

        mpAlias = new TAlias(nullptr, mpHost);
        if (module) {
            mpAlias->mModuleMasterFolder = true;
            mpAlias->mModuleMember = true;
        }
        mpAlias->setPackageName(mPackageName);
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
        mpAction->setPackageName(mPackageName);
        mpAction->setIsActive(true);
        mpAction->setName(mPackageName);
        mpAction->setIsFolder(true);

        mpScript = new TScript(nullptr, mpHost);
        if (module) {
            mpScript->mModuleMasterFolder = true;
            mpScript->mModuleMember = true;
        }
        mpScript->setPackageName(mPackageName);
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
                        float versionNumber = versionString.toFloat(&isOk);
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

                    QString moanMsg = tr("[ ALERT ] - Sorry, the file being read:\n"
                                         "\"%1\"\n"
                                         "reports it has a version (%2) it must have come from a later Mudlet version,\n"
                                         "and this one cannot read it, you need a newer Mudlet!")
                                              .arg(pfile->fileName(), versionString);
                    mpHost->postMessage(moanMsg);
                    return false;
                }

                readPackage();
            } else if (name() == "map") {
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

    return !error();
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
            if (name() == "MudletPackage") {
                result = readPackage();
            } else {
                qDebug() << "ERROR:name=" << name().toString() << "text:" << text().toString();
            }
        }
    }

    return result;
}

void XMLimport::readVariableGroup(TVar* pParent)
{
    auto var = new TVar(pParent);

    LuaInterface* lI = mpHost->getLuaInterface();
    VarUnit* vu = lI->getVarUnit();
    QString keyName, value;
    int keyType = 0;
    int valueType;

    while (!atEnd()) {
        readNext();
        if (isEndElement()) {
            break;
        }

        if (isStartElement()) {
            if (name() == "name") {
                keyName = readElementText();
                continue;
            } else if (name() == "value") {
                value = readElementText();
                continue;
            } else if (name() == "keyType") {
                keyType = readElementText().toInt();
                continue;
            } else if (name() == "valueType") {
                valueType = readElementText().toInt();
                var->setName(keyName, keyType);
                var->setValue(value, valueType);
                vu->addSavedVar(var);
                lI->setValue(var);
                continue;
            } else if (name() == "VariableGroup" || name() == "Variable") {
                readVariableGroup(var);
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
            if (name() == "name") {
                QString var = readElementText();
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
            if (name() == "VariableGroup" || name() == "Variable") {
                readVariableGroup(mpVar);
            } else if (name() == "HiddenVariables") {
                readHiddenVariables();
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
            if (name() == "areas") {
                mpHost->mpMap->mpRoomDB->clearMapDB();
                mpHost->mpMap->reportStringToProgressDialog(tr("Parsing area data..."));
                mpHost->mpMap->reportProgressToProgressDialog(0, 3);
                readAreas();
            } else if (name() == "rooms") {
                mpHost->mpMap->reportStringToProgressDialog(tr("Parsing room data..."));
                mpHost->mpMap->reportProgressToProgressDialog(1, 3);
                readRooms(tempAreaRoomsHash);
            } else if (name() == "environments") {
                mpHost->mpMap->reportStringToProgressDialog(tr("Parsing environment data..."));
                mpHost->mpMap->reportProgressToProgressDialog(2, 3);
                readEnvColors();
            }
            mpHost->mpMap->reportProgressToProgressDialog(3, 3);
        }
    }

    mpHost->mpMap->reportStringToProgressDialog(tr("Assigning rooms to their areas..."));
    int roomTotal = tempAreaRoomsHash.count();
    int currentRoomCount = 0;

    QListIterator<int> itAreaWithRooms(tempAreaRoomsHash.uniqueKeys());
    while (itAreaWithRooms.hasNext()) {
        int areaId = itAreaWithRooms.next();
        auto values = tempAreaRoomsHash.values(areaId);
        QSet<int> areaRoomsSet{values.begin(), values.end()};

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

        if (name() == "environment") {
            readEnvColor();
        }
    }
}

void XMLimport::readEnvColor()
{
    int id = attributes().value(qsl("id")).toString().toInt();
    int color = attributes().value(qsl("color")).toString().toInt();

    mpHost->mpMap->mEnvColors[id] = color;
}

void XMLimport::readAreas()
{
    while (!atEnd()) {
        readNext();

        if (name() == "areas") {
            break;
        } else if (name() == "area") {
            readArea();
        }
    }
}

void XMLimport::readArea()
{
    if (attributes().hasAttribute(qsl("id"))) {
        int id = attributes().value(qsl("id")).toString().toInt();
        QString name = attributes().value(qsl("name")).toString();

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
            int e = attributes().value(qsl("target")).toString().toInt();
            // If there is a "hidden" exit mark it as a locked door, otherwise
            // if there is a "door" mark it as an open/closed/locked door
            // depending on the value (I.R.E. MUD maps always uses "1" for "door"
            // and/or "hidden" - though the latter does not always appear with
            // former):
            int door = (attributes().hasAttribute(qsl("hidden")) && attributes().value(qsl("hidden")).toString().toInt() == 1)
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

            pT->x = attributes().value(qsl("x")).toString().toInt();
            pT->y = attributes().value(qsl("y")).toString().toInt();
            pT->z = attributes().value(qsl("z")).toString().toInt();
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
            readMap();
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
            if (name() == "HostPackage") {
                readHostPackage();
            } else if (name() == "TriggerPackage") {
                objectType = dlgTriggerEditor::EditorViewType::cmTriggerView;
                rootItemID = readTriggerPackage();
            } else if (name() == "TimerPackage") {
                objectType = dlgTriggerEditor::EditorViewType::cmTimerView;
                rootItemID = readTimerPackage();
            } else if (name() == "AliasPackage") {
                objectType = dlgTriggerEditor::EditorViewType::cmAliasView;
                rootItemID = readAliasPackage();
            } else if (name() == "ActionPackage") {
                objectType = dlgTriggerEditor::EditorViewType::cmActionView;
                rootItemID = readActionPackage();
            } else if (name() == "ScriptPackage") {
                objectType = dlgTriggerEditor::EditorViewType::cmScriptView;
                rootItemID = readScriptPackage();
            } else if (name() == "KeyPackage") {
                objectType = dlgTriggerEditor::EditorViewType::cmKeysView;
                rootItemID = readKeyPackage();
            } else if (name() == "HelpPackage") {
                readHelpPackage();
            } else if (name() == "VariablePackage") {
                objectType = dlgTriggerEditor::EditorViewType::cmVarsView;
                readVariablePackage();
            } else {
                readUnknownPackage();
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
            if (name() == "helpURL") {
                QString contents = readElementText();
                mpHost->moduleHelp[mPackageName].insert("helpURL", contents);
            }
        }
    }
}

void XMLimport::readUnknownPackage()
{
    while (!atEnd()) {
        readNext();
        qDebug().nospace() << "XMLimport::readUnknownPackage(): ERROR: UNKNOWN "
                              "Package Element name: "
                           << name().toString() << " and content: " << text().toString();

        if (isEndElement()) {
            break;
        }

        if (isStartElement()) {
            readPackage();
        }
    }
}

void XMLimport::readUnknownHostElement()
{
    while (!atEnd()) {
        readNext();
        qDebug().nospace() << "XMLimport::readUnknownHostElement() ERROR: UNKNOWN "
                              "Host Package Element, name: "
                           << name().toString() << " and content: " << text().toString();

        if (isEndElement()) {
            break;
        }

        if (isStartElement()) {
            readHostPackage(mpHost);
        }
    }
}

void XMLimport::readUnknownTriggerElement()
{
    while (!atEnd()) {
        readNext();
        qDebug().nospace() << "XMLimport::readUnknownHostElement() ERROR: UNKNOWN "
                              "Trigger Package Element, name: "
                           << name().toString() << " and content: " << text().toString();

        if (isEndElement()) {
            break;
        }

        if (isStartElement()) {
            readTriggerPackage();
        }
    }
}

void XMLimport::readUnknownTimerElement()
{
    while (!atEnd()) {
        readNext();
        qDebug().nospace() << "XMLimport::readUnknownHostElement() ERROR: UNKNOWN "
                              "Timer Package Element, name: "
                           << name().toString() << " and content: " << text().toString();

        if (isEndElement()) {
            break;
        }

        if (isStartElement()) {
            readTimerPackage();
        }
    }
}

void XMLimport::readUnknownAliasElement()
{
    while (!atEnd()) {
        readNext();
        qDebug().nospace() << "XMLimport::readUnknownHostElement() ERROR: UNKNOWN "
                              "Alias Package Element, name: "
                           << name().toString() << " and content: " << text().toString();

        if (isEndElement()) {
            break;
        }

        if (isStartElement()) {
            readAliasPackage();
        }
    }
}

void XMLimport::readUnknownActionElement()
{
    while (!atEnd()) {
        readNext();
        qDebug().nospace() << "XMLimport::readUnknownHostElement() ERROR: UNKNOWN "
                              "Action Package Element, name: "
                           << name().toString() << " and content: " << text().toString();

        if (isEndElement()) {
            break;
        }

        if (isStartElement()) {
            readActionPackage();
        }
    }
}

void XMLimport::readUnknownScriptElement()
{
    while (!atEnd()) {
        readNext();
        qDebug().nospace() << "XMLimport::readUnknownHostElement() ERROR: UNKNOWN "
                              "Script Package Element, name: "
                           << name().toString() << " and content: " << text().toString();

        if (isEndElement()) {
            break;
        }

        if (isStartElement()) {
            readScriptPackage();
        }
    }
}

void XMLimport::readUnknownKeyElement()
{
    while (!atEnd()) {
        readNext();
        qDebug().nospace() << "XMLimport::readUnknownHostElement() ERROR: UNKNOWN "
                              "Key Package Element, name: "
                           << name().toString() << " and content: " << text().toString();

        if (isEndElement()) {
            break;
        }

        if (isStartElement()) {
            readKeyPackage();
        }
    }
}

void XMLimport::readHostPackage()
{
    while (!atEnd()) {
        readNext();
        if (isEndElement()) {
            break;
        } else if (isStartElement()) {
            if (name() == "Host") {
                readHostPackage(mpHost);
            } else {
                readUnknownHostElement();
            }
        }
    }
}

void XMLimport::readHostPackage(Host* pHost)
{
    pHost->mAutoClearCommandLineAfterSend = attributes().value(qsl("autoClearCommandLineAfterSend")) == YES;
    pHost->mPrintCommand = attributes().value(qsl("printCommand")) == YES;
    pHost->set_USE_IRE_DRIVER_BUGFIX(attributes().value(qsl("USE_IRE_DRIVER_BUGFIX")) == YES);
    pHost->mUSE_FORCE_LF_AFTER_PROMPT = attributes().value(qsl("mUSE_FORCE_LF_AFTER_PROMPT")) == YES;
    pHost->mUSE_UNIX_EOL = attributes().value(qsl("mUSE_UNIX_EOL")) == YES;
    pHost->getKeyUnit()->mRunAllKeyMatches = attributes().value(qsl("runAllKeyMatches")) == YES;
    pHost->mNoAntiAlias = attributes().value(qsl("mNoAntiAlias")) == YES;
    pHost->mEchoLuaErrors = attributes().value(qsl("mEchoLuaErrors")) == YES;
    pHost->mHighlightHistory = readDefaultTrueBool(qsl("HighlightHistory"));
    if (attributes().hasAttribute("AmbigousWidthGlyphsToBeWide")) {
        const QStringRef ambiguousWidthSetting(attributes().value(qsl("AmbigousWidthGlyphsToBeWide")));
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
    pHost->mIsNextLogFileInHtmlFormat = attributes().value(qsl("mRawStreamDump")) == YES;
    pHost->mIsLoggingTimestamps = attributes().value(qsl("mIsLoggingTimestamps")) == YES;
    pHost->mLogDir = attributes().value(qsl("logDirectory")).toString();
    if (attributes().hasAttribute("logFileNameFormat")) {
        // We previously mixed "yyyy-MM-dd{#|T}hh-MM-ss" with "yyyy-MM-dd{#|T}HH-MM-ss"
        // which is slightly different {always use 24-hour clock even if AM/PM is
        // present (it isn't)} and that broke some code that requires an exact
        // string to work with - now always change it to "HH":
        pHost->mLogFileNameFormat = attributes().value(qsl("logFileNameFormat")).toString().replace(QLatin1String("hh"), QLatin1String("HH"), Qt::CaseSensitive);
        pHost->mLogFileName = attributes().value(qsl("logFileName")).toString();
    }
    pHost->mAlertOnNewData = attributes().value(qsl("mAlertOnNewData")) == YES;
    pHost->mFORCE_NO_COMPRESSION = attributes().value(qsl("mFORCE_NO_COMPRESSION")) == YES;
    pHost->mFORCE_GA_OFF = attributes().value(qsl("mFORCE_GA_OFF")) == YES;
    pHost->mFORCE_SAVE_ON_EXIT = readDefaultTrueBool(qsl("mFORCE_SAVE_ON_EXIT"));
    pHost->mEnableGMCP = attributes().value(qsl("mEnableGMCP")) == YES;
    pHost->mEnableMSDP = attributes().value(qsl("mEnableMSDP")) == YES;
    pHost->mEnableMSSP = attributes().value(qsl("mEnableMSSP")) == YES;
    pHost->mEnableMSP = attributes().value(qsl("mEnableMSP")) == YES;
    pHost->mMapStrongHighlight = attributes().value(qsl("mMapStrongHighlight")) == YES;
    pHost->mEnableSpellCheck = attributes().value(qsl("mEnableSpellCheck")) == YES;
    bool enableUserDictionary = attributes().value(qsl("mEnableUserDictionary")) == YES;
    bool useSharedDictionary = attributes().value(qsl("mUseSharedDictionary")) == YES;
    pHost->setUserDictionaryOptions(enableUserDictionary, useSharedDictionary);
    pHost->mAcceptServerGUI = attributes().value(qsl("mAcceptServerGUI")) == YES;
    pHost->mAcceptServerMedia = attributes().value(qsl("mAcceptServerMedia")) == YES;
    pHost->mMapperUseAntiAlias = attributes().value(qsl("mMapperUseAntiAlias")) == YES;
    pHost->mMapperShowRoomBorders = readDefaultTrueBool(qsl("mMapperShowRoomBorders"));
    pHost->mEditorAutoComplete = (attributes().value(qsl("mEditorAutoComplete")) == YES);
    if (attributes().hasAttribute("mEditorShowBidi")) {
        pHost->setEditorShowBidi(attributes().value(qsl("mEditorShowBidi")) == YES);
    } else {
        pHost->setEditorShowBidi(true);
    }
    bool autoWrap = false;
    if (attributes().hasAttribute("mAutoWrap")) {
        autoWrap = attributes().value(qsl("mAutoWrap")) == YES;
    }
    // don't call wrap function during profile load as the console won't exist yet
    QTimer::singleShot(0, pHost, [autoWrap, pHost]() {
        pHost->setAutoWrap(autoWrap);
    });
    if (attributes().hasAttribute("announceIncomingText")) {
        pHost->mAnnounceIncomingText = attributes().value(qsl("announceIncomingText")) == YES;
    } else {
        pHost->mAnnounceIncomingText = true;
    }
    if (attributes().hasAttribute("caretShortcut")) {
        const QStringRef caretShortcut(attributes().value(qsl("caretShortcut")));
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
        const QStringRef blankLineBehaviour(attributes().value(qsl("blankLineBehaviour")));
        if (blankLineBehaviour == qsl("Hide")) {
            pHost->mBlankLineBehaviour = Host::BlankLineBehaviour::Hide;
        } else if (blankLineBehaviour == qsl("Show")) {
            pHost->mBlankLineBehaviour = Host::BlankLineBehaviour::Show;
        } else if (blankLineBehaviour == qsl("ReplaceWithSpace")) {
            pHost->mBlankLineBehaviour = Host::BlankLineBehaviour::ReplaceWithSpace;
        }
    }
    pHost->mEditorTheme = attributes().value(QLatin1String("mEditorTheme")).toString();
    pHost->mEditorThemeFile = attributes().value(QLatin1String("mEditorThemeFile")).toString();
    pHost->mThemePreviewItemID = attributes().value(QLatin1String("mThemePreviewItemID")).toInt();
    pHost->mThemePreviewType = attributes().value(QLatin1String("mThemePreviewType")).toString();
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
    pHost->setHaveColorSpaceId(attributes().value(QLatin1String("mSGRCodeHasColSpaceId")).toString() == QLatin1String("yes"));
    pHost->setMayRedefineColors(attributes().value(QLatin1String("mServerMayRedefineColors")).toString() == QLatin1String("yes"));

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
        outerColor.setNamedColor(attributes().value(QLatin1String("playerRoomPrimaryColor")).toString());
        innerColor.setNamedColor(attributes().value(QLatin1String("playerRoomSecondaryColor")).toString());
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

    pHost->mFORCE_MXP_NEGOTIATION_OFF = attributes().value(qsl("mFORCE_MXP_NEGOTIATION_OFF")) == YES;
    pHost->mFORCE_CHARSET_NEGOTIATION_OFF = attributes().value(qsl("mFORCE_CHARSET_NEGOTIATION_OFF")) == YES;
    pHost->mEnableTextAnalyzer = attributes().value(qsl("enableTextAnalyzer")) == YES;
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
    pHost->mBubbleMode = attributes().value(qsl("mBubbleMode")) == YES;
    pHost->mMapViewOnly = attributes().value(qsl("mMapViewOnly")) == YES;
    pHost->mShowRoomID = attributes().value(qsl("mShowRoomIDs")) == YES;
    pHost->mShowPanel = attributes().value(qsl("mShowPanel")) == YES;
    pHost->mHaveMapperScript = attributes().value(qsl("mHaveMapperScript")) == YES;
    QStringRef ignore = attributes().value(qsl("mDoubleClickIgnore"));
    for (auto character : ignore) {
        pHost->mDoubleClickIgnore.insert(character);
    }
    if (attributes().hasAttribute(QLatin1String("EditorSearchOptions"))) {
        pHost->setSearchOptions(static_cast<dlgTriggerEditor::SearchOptions>(attributes().value(qsl("EditorSearchOptions")).toInt()));
    }
    pHost->setDebugShowAllProblemCodepoints(attributes().value(qsl("DebugShowAllProblemCodepoints")) == YES);
    pHost->mUseProxy = attributes().value(qsl("mUseProxy")) == YES;
    pHost->mProxyAddress = attributes().value(qsl("mProxyAddress")).toString();
    if (attributes().hasAttribute(QLatin1String("mProxyPort"))) {
        pHost->mProxyPort = attributes().value(qsl("mProxyPort")).toInt();
    } else {
        pHost->mProxyPort = 0;
    }
    pHost->mProxyUsername = attributes().value(qsl("mProxyUsername")).toString();
    pHost->mProxyPassword = attributes().value(qsl("mProxyPassword")).toString();

    pHost->mSslTsl = attributes().value(qsl("mSslTsl")) == YES;
    pHost->mSslIgnoreExpired = attributes().value(qsl("mSslIgnoreExpired")) == YES;
    pHost->mSslIgnoreSelfSigned = attributes().value(qsl("mSslIgnoreSelfSigned")) == YES;
    pHost->mSslIgnoreAll = attributes().value(qsl("mSslIgnoreAll")) == YES;
    bool compactInputLine = attributes().value(QLatin1String("CompactInputLine")) == YES;
    pHost->setCompactInputLine(compactInputLine);
    if (mudlet::self()->mpCurrentActiveHost == pHost) {
        mudlet::self()->dactionInputLine->setChecked(compactInputLine);
    }
    if (attributes().hasAttribute(QLatin1String("CustomLoginId"))) {
        pHost->setCustomLoginId(attributes().value(QLatin1String("CustomLoginId")).toInt());
    } else {
        pHost->setCustomLoginId(0);
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

    if (attributes().hasAttribute(qsl("Large2DMapAreaExitArrows"))) {
        pHost->setLargeAreaExitArrows(attributes().value(qsl("Large2DMapAreaExitArrows")) == YES);
    } else {
        // The default (and for map/profile files from before 4.15.0):
        pHost->setLargeAreaExitArrows(false);
    }

    if (attributes().value(qsl("mShowInfo")) == qsl("no")) {
        mpHost->mMapInfoContributors.clear();
    }

    while (!atEnd()) {
        readNext();

        if (isEndElement()) {
            break;
        }
        if (isStartElement()) {
            if (name() == "name") {
                pHost->mHostName = readElementText();
            } else if (name() == "mInstalledModules") {
                QMap<QString, QStringList> entry;

                readModulesDetailsMap(entry);

                QMapIterator<QString, QStringList> it(entry);
                while (it.hasNext()) {
                    it.next();
                    QStringList moduleList;
                    QStringList entryList = it.value();
                    moduleList << entryList.at(0);
                    moduleList << entryList.at(1);
                    pHost->mInstalledModules[it.key()] = moduleList;
                    pHost->mModulePriorities[it.key()] = entryList.at(2).toInt();
                }
            } else if (name() == "mInstalledPackages") {
                readStringList(pHost->mInstalledPackages);
            } else if (name() == "url") {
                pHost->mUrl = readElementText();
            } else if (name() == "serverPackageName") {
                pHost->mServerGUI_Package_name = readElementText();
            } else if (name() == "serverPackageVersion") {
                pHost->mServerGUI_Package_version = readElementText();
            } else if (name() == "port") {
                pHost->mPort = readElementText().toInt();
            } else if (name() == "borderTopHeight") {
                pHost->mBorderTopHeight = readElementText().toInt();
            } else if (name() == "commandLineMinimumHeight") {
                pHost->commandLineMinimumHeight = readElementText().toInt();
            } else if (name() == "borderBottomHeight") {
                pHost->mBorderBottomHeight = readElementText().toInt();
            } else if (name() == "borderLeftWidth") {
                pHost->mBorderLeftWidth = readElementText().toInt();
            } else if (name() == "borderRightWidth") {
                pHost->mBorderRightWidth = readElementText().toInt();
            } else if (name() == "wrapAt") {
                pHost->mWrapAt = readElementText().toInt();
            } else if (name() == "wrapIndentCount") {
                pHost->mWrapIndentCount = readElementText().toInt();
            } else if (name() == "mCommandSeparator") {
                pHost->mCommandSeparator = readElementText();
            } else if (name() == "mCommandLineFgColor") {
                pHost->mCommandLineFgColor.setNamedColor(readElementText());
            } else if (name() == "mCommandLineBgColor") {
                pHost->mCommandLineBgColor.setNamedColor(readElementText());
            } else if (name() == "mFgColor") {
                pHost->mFgColor.setNamedColor(readElementText());
            } else if (name() == "mBgColor") {
                pHost->mBgColor.setNamedColor(readElementText());
            } else if (name() == "mCommandFgColor") {
                pHost->mCommandFgColor.setNamedColor(readElementText());
            } else if (name() == "mCommandBgColor") {
                pHost->mCommandBgColor.setNamedColor(readElementText());
            } else if (name() == "mBlack") {
                pHost->mBlack.setNamedColor(readElementText());
            } else if (name() == "mLightBlack") {
                pHost->mLightBlack.setNamedColor(readElementText());
            } else if (name() == "mRed") {
                pHost->mRed.setNamedColor(readElementText());
            } else if (name() == "mLightRed") {
                pHost->mLightRed.setNamedColor(readElementText());
            } else if (name() == "mBlue") {
                pHost->mBlue.setNamedColor(readElementText());
            } else if (name() == "mLightBlue") {
                pHost->mLightBlue.setNamedColor(readElementText());
            } else if (name() == "mGreen") {
                pHost->mGreen.setNamedColor(readElementText());
            } else if (name() == "mLightGreen") {
                pHost->mLightGreen.setNamedColor(readElementText());
            } else if (name() == "mYellow") {
                pHost->mYellow.setNamedColor(readElementText());
            } else if (name() == "mLightYellow") {
                pHost->mLightYellow.setNamedColor(readElementText());
            } else if (name() == "mCyan") {
                pHost->mCyan.setNamedColor(readElementText());
            } else if (name() == "mLightCyan") {
                pHost->mLightCyan.setNamedColor(readElementText());
            } else if (name() == "mMagenta") {
                pHost->mMagenta.setNamedColor(readElementText());
            } else if (name() == "mLightMagenta") {
                pHost->mLightMagenta.setNamedColor(readElementText());
            } else if (name() == "mWhite") {
                pHost->mWhite.setNamedColor(readElementText());
            } else if (name() == "mLightWhite") {
                pHost->mLightWhite.setNamedColor(readElementText());
            } else if (name() == "mDisplayFont") {
                pHost->setDisplayFontFromString(readElementText());
#if defined(Q_OS_LINUX)
                // On Linux ensure that emojis are displayed in colour even if
                // this font doesn't support it:
                QFont::insertSubstitution(pHost->mDisplayFont.family(), qsl("Noto Color Emoji"));
#endif
                pHost->setDisplayFontFixedPitch(true);
            } else if (name() == "mCommandLineFont") {
                pHost->mCommandLineFont.fromString(readElementText());
            } else if (name() == "commandSeperator") {
                // Ignore this misspelled duplicate, it has been removed from
                // the Xml format but will appear in older files and trip the
                // QDebug() error reporting associated with the following
                // readUnknownHostElement() for "anything not otherwise parsed"
                Q_UNUSED(readElementText());
            } else if (name() == "mFgColor2") {
                pHost->mFgColor_2.setNamedColor(readElementText());
            } else if (name() == "mBgColor2") {
                pHost->mBgColor_2.setNamedColor(readElementText());
            } else if (name() == "mRoomBorderColor") {
                pHost->mRoomBorderColor.setNamedColor(readElementText());
            } else if (name() == "mMapInfoBg") {
                auto alpha = (attributes().hasAttribute(qsl("alpha"))) ? attributes().value(qsl("alpha")).toInt() : 255;
                pHost->mMapInfoBg.setNamedColor(readElementText());
                pHost->mMapInfoBg.setAlpha(alpha);
            } else if (name() == "mBlack2") {
                pHost->mBlack_2.setNamedColor(readElementText());
            } else if (name() == "mLightBlack2") {
                pHost->mLightBlack_2.setNamedColor(readElementText());
            } else if (name() == "mRed2") {
                pHost->mRed_2.setNamedColor(readElementText());
            } else if (name() == "mLightRed2") {
                pHost->mLightRed_2.setNamedColor(readElementText());
            } else if (name() == "mBlue2") {
                pHost->mBlue_2.setNamedColor(readElementText());
            } else if (name() == "mLightBlue2") {
                pHost->mLightBlue_2.setNamedColor(readElementText());
            } else if (name() == "mGreen2") {
                pHost->mGreen_2.setNamedColor(readElementText());
            } else if (name() == "mLightGreen2") {
                pHost->mLightGreen_2.setNamedColor(readElementText());
            } else if (name() == "mYellow2") {
                pHost->mYellow_2.setNamedColor(readElementText());
            } else if (name() == "mLightYellow2") {
                pHost->mLightYellow_2.setNamedColor(readElementText());
            } else if (name() == "mCyan2") {
                pHost->mCyan_2.setNamedColor(readElementText());
            } else if (name() == "mLightCyan2") {
                pHost->mLightCyan_2.setNamedColor(readElementText());
            } else if (name() == "mMagenta2") {
                pHost->mMagenta_2.setNamedColor(readElementText());
            } else if (name() == "mLightMagenta2") {
                pHost->mLightMagenta_2.setNamedColor(readElementText());
            } else if (name() == "mWhite2") {
                pHost->mWhite_2.setNamedColor(readElementText());
            } else if (name() == "mLightWhite2") {
                pHost->mLightWhite_2.setNamedColor(readElementText());
            } else if (name() == "mSpellDic") {
                pHost->setSpellDic(readElementText());
            } else if (name() == "mLineSize" || name() == "mRoomSize") {
                // These two have been dropped from the Xml format as these are
                // duplicates of attributes that were being incorrected read in
                // the parent <Host ...> element as integers {they are stored as
                // decimals but for the first one at least, it is a decimal
                // number n, where 0.1 <= n <= 1.1 so was being read as "0" for
                // all but the greatest 2 values where it was read as "1"!}
                // We still check for them so that we avoid falling into the
                // QDebug() error reporting associated with the following
                // readUnknownHostElement() for "anything not otherwise parsed"
                Q_UNUSED(readElementText());
            } else if (name() == "mMapInfoContributors") {
                readLegacyMapInfoContributors();
            } else if (name() == "mapInfoContributor") {
                readMapInfoContributor();
            } else if (name() == "profileShortcut") {
                readProfileShortcut();
            } else if (name() == "stopwatches") {
                readStopWatchMap();
            } else {
                readUnknownHostElement();
            }
        }
    }
    mpHost->loadPackageInfo();
}

bool XMLimport::readDefaultTrueBool(QString name) {
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
            if (name() == "TriggerGroup" || name() == "Trigger") {
                gotTrigger = true;
                parentItemID = readTriggerGroup(mPackageName.isEmpty() ? nullptr : mpTrigger);
            } else {
                readUnknownTriggerElement();
            }
        }
    }

    return parentItemID;
}

// imports a trigger and returns its ID - in case of a group, returns the ID
// of the top-level trigger group.
int XMLimport::readTriggerGroup(TTrigger* pParent)
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


    while (!atEnd()) {
        readNext();

        if (isEndElement()) {
            break;
        } else if (isStartElement()) {
            if (name() == "name") {
                pT->setName(readElementText());
            } else if (name() == "script") {
                QString tempScript = readScriptElement();
                if (!pT->setScript(tempScript)) {
                    qDebug().nospace() << "XMLimport::readTriggerGroup(...): ERROR: can not compile trigger's lua code for: " << pT->getName();
                }
            } else if (name() == "packageName") {
                pT->mPackageName = readElementText();
            } else if (name() == "triggerType") {
                pT->mTriggerType = readElementText().toInt();
            } else if (name() == "conditonLineDelta") {
                pT->mConditionLineDelta = readElementText().toInt();
            } else if (name() == "mStayOpen") {
                pT->mStayOpen = readElementText().toInt();
            } else if (name() == "mCommand") {
                pT->mCommand = readElementText();
            } else if (name() == "mFgColor") {
                pT->mFgColor.setNamedColor(readElementText());
            } else if (name() == "mBgColor") {
                pT->mBgColor.setNamedColor(readElementText());
            } else if (name() == "colorTriggerFgColor") {
                pT->mColorTriggerFgColor.setNamedColor(readElementText());
            } else if (name() == "colorTriggerBgColor") {
                pT->mColorTriggerBgColor.setNamedColor(readElementText());
            } else if (name() == "mSoundFile") {
                pT->mSoundFile = readElementText();
            } else if (name() == "regexCodeList") {
                // This and the next one ought to be combined into a single element
                // in the next revision - sample code for "RegexCode" elements
                // inside a "patterns" container (with a "size" attribute) is
                // commented out in the XMLexporter class.
                readStringList(pT->mPatterns);
            } else if (name() == "regexCodePropertyList") {
                readIntegerList(pT->mPatternKinds, pT->getName());
                if (Q_UNLIKELY(pT->mPatterns.count() != pT->mPatternKinds.count())) {
                    qWarning().nospace() << "XMLimport::readTriggerGroup(...) ERROR: "
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
            } else if (name() == "TriggerGroup" || name() == "Trigger") {
                readTriggerGroup(pT);
            } else {
                readUnknownTriggerElement();
            }
        }
    }

    if (!pT->setRegexCodeList(pT->mPatterns, pT->mPatternKinds)) {
        qDebug().nospace() << "XMLimport::readTriggerGroup(...): ERROR: can not "
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
            if (name() == "TimerGroup" || name() == "Timer") {
                gotTimer = true;
                lastImportedTimerID = readTimerGroup(mPackageName.isEmpty() ? nullptr : mpTimer);
            } else {
                readUnknownTimerElement();
            }
        }
    }

    return lastImportedTimerID;
}

int XMLimport::readTimerGroup(TTimer* pParent)
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

    while (!atEnd()) {
        readNext();
        if (isEndElement()) {
            break;
        } else if (isStartElement()) {
            if (name() == "name") {
                pT->setName(readElementText());
            } else if (name() == "packageName") {
                pT->mPackageName = readElementText();
            } else if (name() == "script") {
                QString tempScript = readScriptElement();
                if (!pT->setScript(tempScript)) {
                    qDebug().nospace() << "XMLimport::readTimerGroup(...): ERROR: can not compile timer's lua code for: " << pT->getName();
                }
            } else if (name() == "command") {
                pT->mCommand = readElementText();
            } else if (name() == "time") {
                pT->setTime(QTime::fromString(readElementText(), "hh:mm:ss.zzz"));
            } else if (name() == "TimerGroup" || name() == "Timer") {
                readTimerGroup(pT);
            } else {
                readUnknownTimerElement();
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
            if (name() == "AliasGroup" || name() == "Alias") {
                gotAlias = true;
                lastImportedAliasID = readAliasGroup(mPackageName.isEmpty() ? nullptr : mpAlias);
            } else {
                readUnknownAliasElement();
            }
        }
    }

    return lastImportedAliasID;
}

int XMLimport::readAliasGroup(TAlias* pParent)
{
    auto pT = new TAlias(pParent, mpHost);

    mpHost->getAliasUnit()->registerAlias(pT);
    pT->setIsActive(attributes().value(qsl("isActive")) == YES);
    pT->setIsFolder(attributes().value(qsl("isFolder")) == YES);
    if (module) {
        pT->mModuleMember = true;
    }

    while (!atEnd()) {
        readNext();

        if (isEndElement()) {
            break;
        } else if (isStartElement()) {
            if (name() == "name") {
                pT->setName(readElementText());
            } else if (name() == "packageName") {
                pT->mPackageName = readElementText();
            } else if (name() == "script") {
                QString tempScript = readScriptElement();
                if (!pT->setScript(tempScript)) {
                    qDebug().nospace() << "XMLimport::readAliasGroup(...): ERROR: can not compile alias's lua code for: " << pT->getName();
                }
            } else if (name() == "command") {
                pT->mCommand = readElementText();
            } else if (name() == "regex") {
                pT->setRegexCode(readElementText());
            } else if (name() == "AliasGroup" || name() == "Alias") {
                readAliasGroup(pT);
            } else {
                readUnknownAliasElement();
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
            if (name() == "ActionGroup" || name() == "Action") {
                gotAction = true;
                lastImportedActionID = readActionGroup(mPackageName.isEmpty() ? nullptr : mpAction);
            } else {
                readUnknownActionElement();
            }
        }
    }

    return lastImportedActionID;
}

int XMLimport::readActionGroup(TAction* pParent)
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

    while (!atEnd()) {
        readNext();
        if (isEndElement()) {
            break;
        } else if (isStartElement()) {
            if (name() == "name") {
                pT->mName = readElementText();
            } else if (name() == "packageName") {
                pT->mPackageName = readElementText();
            } else if (name() == "script") {
                QString tempScript = readScriptElement();
                if (!pT->setScript(tempScript)) {
                    qDebug().nospace() << "XMLimport::readActionGroup(...): ERROR: can not compile action's lua code for: " << pT->getName();
                }
            } else if (name() == "css") {
                pT->css = readElementText();
            } else if (name() == "commandButtonUp") {
                pT->mCommandButtonUp = readElementText();
            } else if (name() == "commandButtonDown") {
                pT->mCommandButtonDown = readElementText();
            } else if (name() == "icon") {
                pT->mIcon = readElementText();
            } else if (name() == "orientation") {
                pT->mOrientation = readElementText().toInt();
            } else if (name() == "location") {
                pT->mLocation = readElementText().toInt();
            } else if (name() == "buttonRotation") {
                pT->mButtonRotation = readElementText().toInt();
            } else if (name() == "sizeX") {
                pT->mSizeX = readElementText().toInt();
            } else if (name() == "sizeY") {
                pT->mSizeY = readElementText().toInt();
            } else if (name() == "mButtonState") {
                // We now use a boolean but file must use original "1" (false)
                // or "2" (true) for backward compatibility
                pT->mButtonState = (readElementText().toInt() == 2);
            } else if (name() == "buttonColor") {
                // Not longer present/used, skip over it if it is still in file:
                skipCurrentElement();
            } else if (name() == "buttonColumn") {
                pT->mButtonColumns = readElementText().toInt();
            } else if (name() == "posX") {
                pT->mPosX = readElementText().toInt();
            } else if (name() == "posY") {
                pT->mPosY = readElementText().toInt();
            } else if (name() == "ActionGroup" || name() == "Action") {
                readActionGroup(pT);
            } else {
                readUnknownActionElement();
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
            if (name() == "ScriptGroup" || name() == "Script") {
                gotScript = true;
                lastImportedScriptID = readScriptGroup(mPackageName.isEmpty() ? nullptr : mpScript);
            } else {
                readUnknownScriptElement();
            }
        }
    }

    return lastImportedScriptID;
}

int XMLimport::readScriptGroup(TScript* pParent)
{
    auto script = new TScript(pParent, mpHost);

    script->setIsFolder(attributes().value(qsl("isFolder")) == YES);
    mpHost->getScriptUnit()->registerScript(script);
    script->setIsActive(attributes().value(qsl("isActive")) == YES);

    if (module) {
        script->mModuleMember = true;
    }

    while (!atEnd()) {
        readNext();
        if (isEndElement()) {
            break;
        } else if (isStartElement()) {
            if (name() == "name") {
                script->mName = readElementText();
            } else if (name() == "packageName") {
                script->mPackageName = readElementText();
            } else if (name() == "script") {
                QString tempScript = readScriptElement();
                if (!script->setScript(tempScript)) {
                    qDebug().nospace().noquote() << "XMLimport::readScriptGroup(...) ERROR - can not compile script's lua code for \"" << script->getName() << "\"; reason: " << script->getError() << ".";
                }
            } else if (name() == "eventHandlerList") {
                readStringList(script->mEventHandlerList);
                script->setEventHandlerList(script->mEventHandlerList);
            } else if (name() == "ScriptGroup" || name() == "Script") {
                readScriptGroup(script);
            } else {
                readUnknownScriptElement();
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
            if (name() == "KeyGroup" || name() == "Key") {
                gotKey = true;
                lastImportedKeyID = readKeyGroup(mPackageName.isEmpty() ? nullptr : mpKey);
            } else {
                readUnknownKeyElement();
            }
        }
    }

    return lastImportedKeyID;
}

int XMLimport::readKeyGroup(TKey* pParent)
{
    auto pT = new TKey(pParent, mpHost);

    mpHost->getKeyUnit()->registerKey(pT);
    pT->setIsActive(attributes().value(qsl("isActive")) == YES);
    pT->setIsFolder(attributes().value(qsl("isFolder")) == YES);
    if (module) {
        pT->mModuleMember = true;
    }

    while (!atEnd()) {
        readNext();

        if (isEndElement()) {
            break;
        } else if (isStartElement()) {
            if (name() == "name") {
                pT->setName(readElementText());
            } else if (name() == "packageName") {
                pT->mPackageName = readElementText();
            } else if (name() == "script") {
                QString tempScript = readScriptElement();
                if (!pT->setScript(tempScript)) {
                    qDebug().nospace() << "XMLimport::readKeyGroup(...): ERROR: can not compile key's lua code for: " << pT->getName();
                }
            } else if (name() == "command") {
                pT->mCommand = readElementText();
            } else if (name() == "keyCode") {
                pT->setKeyCode(readElementText().toInt());
            } else if (name() == "keyModifier") {
                pT->setKeyModifiers(readElementText().toInt());
            } else if (name() == "KeyGroup" || name() == "Key") {
                readKeyGroup(pT);
            } else {
                readUnknownKeyElement();
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
            if (name() == "key") {
                key = readElementText();
            } else if (name() == "filepath") {
                entry << readElementText();
            } else if (name() == "zipSync") {
                entry << readElementText();
            } else if (name() == "globalSave") {
                if (entry.size() < 2) {
                    entry << readElementText();
                } else {
                    skipCurrentElement();
                }
            } else if (name() == "priority") {
                // The last expected detail for the entry - so store this
                // completed entry into the QMap
                entry << readElementText();
                map[key] = entry;
                entry.clear();
            } else {
                readUnknownHostElement();
            }
        }
    }
}

void XMLimport::readStringList(QStringList& list)
{
    while (!atEnd()) {
        readNext();

        if (isEndElement()) {
            break;
        } else if (isStartElement()) {
            if (name() == "string") {
                list << readElementText();
            } else {
                readUnknownTriggerElement();
            }
        }
    }
}

void XMLimport::readIntegerList(QList<int>& list, const QString& parentName)
{
    while (!atEnd()) {
        readNext();

        if (isEndElement()) {
            break;
        } else if (isStartElement()) {
            if (name() == "integer") {
                QString numberText = readElementText();
                bool ok = false;
                int num = numberText.toInt(&ok, 10);
                if (Q_LIKELY(!numberText.isEmpty() && ok)) {
                    list << num;
                } else {
                    // Using qFatal() seems a little, erm, fatalistic but it
                    // seems no lesser one will always be detectable on the
                    // RELEASE version on Windows? - Slysven
                    qFatal(R"(XMLimport::readIntegerList(...) ERROR: unable to convert: "%s" to a number when reading the 'regexCodePropertyList' element of the 'Trigger' or 'TriggerGroup' element "%s"!)",
                           numberText.toUtf8().constData(),
                           parentName.toUtf8().constData());
                }
            } else {
                readUnknownTriggerElement();
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
    QRegularExpression regex = QRegularExpression(qsl("FG(-?\\d+)BG(-?\\d+)"));
    QMutableStringListIterator itPattern(patternList);
    QListIterator<int> itType(typeList);
    while (itPattern.hasNext() && itType.hasNext()) {
        if (itType.next() == REGEX_COLOR_PATTERN) {
            QRegularExpressionMatch match = regex.match(itPattern.next());
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
            if (name() == "stopwatch") {
                int watchId = attributes().value(qsl("id")).toInt();
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
                readUnknownHostElement();
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
            if (name() == "mapInfoContributor") {
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
