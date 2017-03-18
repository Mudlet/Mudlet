/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2016-2017 by Stephen Lyons - slysven@virginmedia.com    *
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


#include "LuaInterface.h"
#include "TAction.h"
#include "TAlias.h"
#include "TKey.h"
#include "TMap.h"
#include "TRoom.h"
#include "TRoomDB.h"
#include "TScript.h"
#include "TTimer.h"
#include "TTrigger.h"
#include "TVar.h"
#include "VarUnit.h"
#include "mudlet.h"

// clang-format: off
#include "pre_guard.h"
// clang-format: on
#include <QDebug>
#include <QStringList>
// clang-format: off
#include "post_guard.h"
// clang-format: on

XMLimport::XMLimport(Host* pH)
    : mpHost(pH)
    , mPackageName(QString())
    , mpTrigger(Q_NULLPTR)
    , mpTimer(Q_NULLPTR)
    , mpAlias(Q_NULLPTR)
    , mpKey(Q_NULLPTR)
    , mpAction(Q_NULLPTR)
    , mpScript(Q_NULLPTR)
    , mpVar(Q_NULLPTR)
    , gotTrigger(false)
    , gotTimer(false)
    , gotAlias(false)
    , gotKey(false)
    , gotAction(false)
    , gotScript(false)
    , module(0)
    , mMaxRoomId(0)
    , mMaxAreaId(-1)
{
}

bool XMLimport::importPackage(QIODevice* device, QString packName, int moduleFlag)
{
    mPackageName = packName;
    setDevice(device);

    module = moduleFlag;

    if (!packName.isEmpty()) {
        mpKey = new TKey(0, mpHost);
        if (module) {
            mpKey->mModuleMasterFolder = true;
            mpKey->mModuleMember = true;
        }
        mpKey->setPackageName(mPackageName);
        mpKey->setIsActive(true);
        mpKey->setName(mPackageName);
        mpKey->setIsFolder(true);

        mpTrigger = new TTrigger(0, mpHost);
        if (module) {
            mpTrigger->mModuleMasterFolder = true;
            mpTrigger->mModuleMember = true;
        }
        mpTrigger->setPackageName(mPackageName);
        mpTrigger->setIsActive(true);
        mpTrigger->setName(mPackageName);
        mpTrigger->setIsFolder(true);

        mpTimer = new TTimer(0, mpHost);
        mpTimer->setIsFolder(true);

        if (module) {
            mpTimer->mModuleMasterFolder = true;
            mpTimer->mModuleMember = true;
        }

        mpTimer->setPackageName(mPackageName);
        mpTimer->setName(mPackageName);
        mpTimer->setIsActive(true);

        mpAlias = new TAlias(0, mpHost);
        if (module) {
            mpAlias->mModuleMasterFolder = true;
            mpAlias->mModuleMember = true;
        }
        mpAlias->setPackageName(mPackageName);
        mpAlias->setName(mPackageName);
        mpAlias->setIsFolder(true);
        mpAlias->setScript(QString());
        mpAlias->setRegexCode(QString());
        mpAlias->setIsActive(true);

        mpAction = new TAction(0, mpHost);
        if (module) {
            mpAction->mModuleMasterFolder = true;
            mpAction->mModuleMember = true;
        }
        mpAction->setPackageName(mPackageName);
        mpAction->setIsActive(true);
        mpAction->setName(mPackageName);
        mpAction->setIsFolder(true);
        mpScript = new TScript(0, mpHost);
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
            if (name() == "MudletPackage") {
                readPackage();
            } else if (name() == "map") {
                readMap();
                mpHost->mpMap->audit();
            } else {
                qDebug().nospace() << "XMLimport::importPackage(...) ERROR: unrecognised element with name: " << name().toString() << " and content: " << text().toString();
            }
        }
    }

    if (gotTimer && !packName.isEmpty()) {
        mpTimer->setIsActive(true);
        mpTimer->enableTimer(mpTimer->getID());
    }

    if (gotAlias && !packName.isEmpty()) {
        mpAlias->setIsActive(true);
    }

    if (gotAction && !packName.isEmpty()) {
        mpHost->getActionUnit()->updateToolbar();
    }

    if (!packName.isEmpty()) {
        if (!gotTrigger) {
            mpHost->getTriggerUnit()->unregisterTrigger(mpTrigger);
        }
        if (!gotTimer) {
            mpHost->getTimerUnit()->unregisterTimer(mpTimer);
        }
        if (!gotAlias) {
            mpHost->getAliasUnit()->unregisterAlias(mpAlias);
        }
        if (!gotAction) {
            mpHost->getActionUnit()->unregisterAction(mpAction);
        }
        if (!gotKey) {
            mpHost->getKeyUnit()->unregisterKey(mpKey);
        }
        if (!gotScript) {
            mpHost->getScriptUnit()->unregisterScript(mpScript);
        }
    }
    return !error();
}

void XMLimport::readVariableGroup(TVar* pParent)
{
    TVar* var = new TVar(pParent);

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
        QSet<int> areaRoomsSet = tempAreaRoomsHash.values(areaId).toSet();

        if (!mpHost->mpMap->mpRoomDB->areas.contains(areaId)) {
            // It is known for map files to have rooms with area Ids that are not in the
            // listed areas - this cures that:
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
        /*else
        {
            readUnknownMapElement();
        }*/
    }
}

void XMLimport::readEnvColor()
{

    int id = attributes().value("id").toString().toInt();
    int color = attributes().value("color").toString().toInt();
    mpHost->mpMap->envColors[id] = color;
}

void XMLimport::readAreas()
{
    while (!atEnd()) {
        readNext();
        if (name() == "areas") {
            break;
        }
        if (name() == "area") {
            readArea();
        }
    }
}

void XMLimport::readArea()
{
    int id = attributes().value("id").toString().toInt();
    QString name = attributes().value("name").toString();
    mpHost->mpMap->mpRoomDB->addArea(id, name);
}

void XMLimport::readRooms(QMultiHash<int, int>& areaRoomsHash)
{

    unsigned int roomCount = 0;
    while (!atEnd()) {
        readNext();

        if (Q_LIKELY(isStartElement())) {
            if (Q_LIKELY(name() == QStringLiteral("room"))) {
                readRoom(areaRoomsHash, &roomCount);
            }

            else {
                readUnknownMapElement();
            }
        } else if (isEndElement()) {
            break;
        }
    }
}

// This is a CPU/Time hog without the non-default (true) third argument to
// TRoomDB::addRoom(...)
void XMLimport::readRoom(QMultiHash<int, int>& areamRoomMultiHash, unsigned int* roomCount)
{
    TRoom* pT = new TRoom(mpHost->mpMap->mpRoomDB);
    pT->id = attributes().value(QStringLiteral("id")).toString().toInt();
    pT->area = attributes().value(QStringLiteral("area")).toString().toInt();
    pT->name = attributes().value(QStringLiteral("title")).toString();
    pT->environment = attributes().value(QStringLiteral("environment")).toString().toInt();

    while (!atEnd()) {
        readNext();

        if (Q_UNLIKELY(pT->id < 1)) {
            continue; // Skip further tests on exits as we'd have to throw away
                      // this invalid room and it would mess up the entranceMultiHash
        } else if (Q_LIKELY(name() == QStringLiteral("exit"))) {
            QString dir = attributes().value(QStringLiteral("direction")).toString();
            int e = attributes().value(QStringLiteral("target")).toString().toInt();
            if (dir.isEmpty()) {
                continue;
            } else if (dir == QStringLiteral("north")) {
                pT->north = e;
            } else if (dir == QStringLiteral("east")) {
                pT->east = e;
            } else if (dir == QStringLiteral("south")) {
                pT->south = e;
            } else if (dir == QStringLiteral("west")) {
                pT->west = e;
            } else if (dir == QStringLiteral("up")) {
                pT->up = e;
            } else if (dir == QStringLiteral("down")) {
                pT->down = e;
            } else if (dir == QStringLiteral("northeast")) {
                pT->northeast = e;
            } else if (dir == QStringLiteral("southwest")) {
                pT->southwest = e;
            } else if (dir == QStringLiteral("southeast")) {
                pT->southeast = e;
            } else if (dir == QStringLiteral("northwest")) {
                pT->northwest = e;
            } else if (dir == QStringLiteral("in")) {
                pT->in = e;
            } else if (dir == QStringLiteral("out")) {
                pT->out = e;
            } else {
                // TODO: Handle Special Exits
            }
            continue;
        } else if (name() == QStringLiteral("coord")) {
            if (attributes().value("x").toString().isEmpty()) {
                continue;
            }

            pT->x = attributes().value(QStringLiteral("x")).toString().toInt();
            pT->y = attributes().value(QStringLiteral("y")).toString().toInt();
            pT->z = attributes().value(QStringLiteral("z")).toString().toInt();
            continue;
        } else if (Q_UNLIKELY(name().isEmpty())) {
            continue;
        }

        if (isEndElement()) {
            break;
        }
    }

    if (pT->id > 0) {
        if ((++(*roomCount) % 100 == 0)) {
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
        }

        if (isStartElement()) {
            readMap();
        }
    }
}

void XMLimport::readPackage()
{
    while (!atEnd()) {
        readNext();
        if (isEndElement()) {
            break;
        }

        if (isStartElement()) {
            if (name() == "HostPackage") {
                readHostPackage();
                continue;
            }
            if (name() == "TriggerPackage") {
                readTriggerPackage();
                continue;
            } else if (name() == "TimerPackage") {
                readTimerPackage();
                continue;
            } else if (name() == "AliasPackage") {
                readAliasPackage();
                continue;
            } else if (name() == "ActionPackage") {
                readActionPackage();
                continue;
            } else if (name() == "ScriptPackage") {
                readScriptPackage();
                continue;
            } else if (name() == "KeyPackage") {
                readKeyPackage();
                continue;
            } else if (name() == "HelpPackage") {
                readHelpPackage();
                continue;
            } else if (name() == "VariablePackage") {
                readVariablePackage();
            } else {
                readUnknownPackage();
            }
        }
    }
}

void XMLimport::readHelpPackage()
{
    while (!atEnd()) {
        readNext();
        if (isEndElement()) {
            break;
        }
        if (isStartElement()) {
            if (name() == "helpURL") {
                QString contents = readElementText();
                mpHost->moduleHelp[mPackageName].insert("helpURL", contents);
                continue;
            }
        }
    }
}


void XMLimport::readUnknownPackage()
{
    while (!atEnd()) {

        readNext();
        qDebug().nospace() << "XMLimport::readUnknownPackage(): ERROR: UNKNOWN Package Element name: " << name().toString() << " and content: " << text().toString();

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
        qDebug().nospace() << "XMLimport::readUnknownHostElement() ERROR: UNKNOWN Host Package Element, name: " << name().toString() << " and content: " << text().toString();

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
        qDebug().nospace() << "XMLimport::readUnknownTriggerElement() ERROR: UNKNOWN Trigger Package Element, name: " << name().toString() << " and content: " << text().toString();

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
        qDebug().nospace() << "XMLimport::readUnknownTimerElement() ERROR: UNKNOWN Timer Package Element, name: " << name().toString() << " and content: " << text().toString();

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
        qDebug().nospace() << "XMLimport::readUnknownAliasElement() ERROR: UNKNOWN Alias Package Element, name: " << name().toString() << " and content: " << text().toString();

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
        qDebug().nospace() << "XMLimport::readUnknownActionElement() ERROR: UNKNOWN Action Package Element, name: " << name().toString() << " and content: " << text().toString();

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
        qDebug().nospace() << "XMLimport::readUnknownScriptElement() ERROR: UNKNOWN Script Package Element, name: " << name().toString() << " and content: " << text().toString();

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
        qDebug().nospace() << "XMLimport::readUnknownKeyElement() ERROR: UNKNOWN Key Package Element, name: " << name().toString() << " and content: " << text().toString();

        if (isEndElement()) {
            break;
        }

        if (isStartElement()) {
            readKeyPackage();
        }
    }
}


void XMLimport::readTriggerPackage()
{
    while (!atEnd()) {
        readNext();
        if (isEndElement()) {
            break;
        }

        if (isStartElement()) {
            if (name() == "TriggerGroup" || name() == "Trigger") {
                gotTrigger = true;
                if (mPackageName.isEmpty()) {
                    readTriggerGroup(0);
                } else {
                    readTriggerGroup(mpTrigger);
                }
            } else {
                readUnknownTriggerElement();
            }
        }
    }
}

void XMLimport::readHostPackage()
{
    while (!atEnd()) {
        readNext();
        if (isEndElement()) {
            break;
        }

        if (isStartElement()) {
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
    pHost->mAutoClearCommandLineAfterSend = (attributes().value("autoClearCommandLineAfterSend") == "yes");
    pHost->mDisableAutoCompletion = (attributes().value("disableAutoCompletion") == "yes");
    pHost->mPrintCommand = (attributes().value("printCommand") == "yes");
    pHost->set_USE_IRE_DRIVER_BUGFIX(attributes().value("USE_IRE_DRIVER_BUGFIX") == "yes");
    pHost->mUSE_FORCE_LF_AFTER_PROMPT = (attributes().value("mUSE_FORCE_LF_AFTER_PROMPT") == "yes");
    pHost->mUSE_UNIX_EOL = (attributes().value("mUSE_UNIX_EOL") == "yes");
    pHost->mNoAntiAlias = (attributes().value("mNoAntiAlias") == "yes");
    pHost->mIsNextLogFileInHtmlFormat = (attributes().value("mRawStreamDump") == "yes");
    pHost->mAlertOnNewData = (attributes().value("mAlertOnNewData") == "yes");
    pHost->mFORCE_NO_COMPRESSION = (attributes().value("mFORCE_NO_COMPRESSION") == "yes");
    pHost->mFORCE_GA_OFF = (attributes().value("mFORCE_GA_OFF") == "yes");
    pHost->mFORCE_SAVE_ON_EXIT = (attributes().value("mFORCE_SAVE_ON_EXIT") == "yes");
    pHost->mEnableGMCP = (attributes().value("mEnableGMCP") == "yes");
    pHost->mEnableMSDP = (attributes().value("mEnableMSDP") == "yes");
    pHost->mMapStrongHighlight = (attributes().value("mMapStrongHighlight") == "yes");
    pHost->mLogStatus = (attributes().value("mLogStatus") == "yes");
    pHost->mEnableSpellCheck = (attributes().value("mEnableSpellCheck") == "yes");
    pHost->mShowInfo = (attributes().value("mShowInfo") == "yes");
    pHost->mAcceptServerGUI = (attributes().value("mAcceptServerGUI") == "yes");
    pHost->mMapperUseAntiAlias = (attributes().value("mMapperUseAntiAlias") == "yes");
    pHost->mFORCE_MXP_NEGOTIATION_OFF = (attributes().value("mFORCE_MXP_NEGOTIATION_OFF") == "yes");
    pHost->mRoomSize = attributes().value("mRoomSize").toString().toFloat();
    if (qFuzzyCompare(1.0 + pHost->mRoomSize, 1.0)) {
        // The value is a float/double and the prior code using "== 0" is a BAD
        // THING to do with non-integer number types!
        pHost->mRoomSize = 0.5f; // Same value as is in Host class initalizer list
    }
    pHost->mLineSize = attributes().value("mLineSize").toString().toFloat();
    if (qFuzzyCompare(1.0 + pHost->mLineSize, 1.0)) {
        pHost->mLineSize = 1.0f; // Same value as is in Host class initalizer list
    }
    pHost->mBubbleMode = (attributes().value("mBubbleMode") == "yes");
    pHost->mShowRoomID = (attributes().value("mShowRoomIDs") == "yes");
    pHost->mShowPanel = (attributes().value("mShowPanel") == "yes");
    pHost->mHaveMapperScript = (attributes().value("mHaveMapperScript") == "yes");
    QStringRef ignore = attributes().value("mDoubleClickIgnore");
    for (int i = 0; i < ignore.size(); i++) {
        pHost->mDoubleClickIgnore.insert(ignore.at(i));
    }

    while (!atEnd()) {
        readNext();
        if (isEndElement()) {
            break;
        }

        if (isStartElement()) {
            if (name() == "name") {
                pHost->mHostName = readElementText();
                continue;
            } else if (name() == "mInstalledModules") {
                QMap<QString, QStringList> entry;
                readModulesDetailsMap(entry);
                QMapIterator<QString, QStringList> it(entry);
                while (it.hasNext()) {
                    it.next();
                    QStringList moduleList;
                    QStringList entryList = it.value();
                    moduleList << entryList[0];
                    moduleList << entryList[1];
                    pHost->mInstalledModules[it.key()] = moduleList;
                    pHost->mModulePriorities[it.key()] = entryList[2].toInt();
                }
                continue;
            } else if (name() == "mInstalledPackages") {
                readStringList(pHost->mInstalledPackages);
                continue;
            } else if (name() == "url") {
                pHost->mUrl = readElementText();
                continue;
            } else if (name() == "serverPackageName") {
                pHost->mServerGUI_Package_name = readElementText();
                continue;
            } else if (name() == "serverPackageVersion") {
                pHost->mServerGUI_Package_version = readElementText().toInt();
                continue;
            } else if (name() == "port") {
                pHost->mPort = readElementText().toInt();
                continue;
            } else if (name() == "borderTopHeight") {
                pHost->mBorderTopHeight = readElementText().toInt();
                continue;
            } else if (name() == "commandLineMinimumHeight") {
                pHost->commandLineMinimumHeight = readElementText().toInt();
                continue;
            } else if (name() == "borderBottomHeight") {
                pHost->mBorderBottomHeight = readElementText().toInt();
                continue;
            } else if (name() == "borderLeftWidth") {
                pHost->mBorderLeftWidth = readElementText().toInt();
                continue;
            } else if (name() == "borderRightWidth") {
                pHost->mBorderRightWidth = readElementText().toInt();
                continue;
            } else if (name() == "wrapAt") {
                pHost->mWrapAt = readElementText().toInt();
                continue;
            } else if (name() == "wrapIndentCount") {
                pHost->mWrapIndentCount = readElementText().toInt();
                continue;
            } else if (name() == "mCommandSeparator") {
                pHost->mCommandSeparator = readElementText();
                continue;
            } else if (name() == "mCommandLineFgColor") {
                pHost->mCommandLineFgColor.setNamedColor(readElementText());
                continue;
            } else if (name() == "mCommandLineBgColor") {
                pHost->mCommandLineBgColor.setNamedColor(readElementText());
                continue;
            } else if (name() == "mFgColor") {
                pHost->mFgColor.setNamedColor(readElementText());
                continue;
            } else if (name() == "mBgColor") {
                pHost->mBgColor.setNamedColor(readElementText());
                continue;
            } else if (name() == "mCommandFgColor") {
                pHost->mCommandFgColor.setNamedColor(readElementText());
                continue;
            } else if (name() == "mCommandBgColor") {
                pHost->mCommandBgColor.setNamedColor(readElementText());
                continue;
            } else if (name() == "mBlack") {
                pHost->mBlack.setNamedColor(readElementText());
                continue;
            } else if (name() == "mLightBlack") {
                pHost->mLightBlack.setNamedColor(readElementText());
                continue;
            } else if (name() == "mRed") {
                pHost->mRed.setNamedColor(readElementText());
                continue;
            } else if (name() == "mLightRed") {
                pHost->mLightRed.setNamedColor(readElementText());
                continue;
            } else if (name() == "mBlue") {
                pHost->mBlue.setNamedColor(readElementText());
                continue;
            } else if (name() == "mLightBlue") {
                pHost->mLightBlue.setNamedColor(readElementText());
                continue;
            } else if (name() == "mGreen") {
                pHost->mGreen.setNamedColor(readElementText());
                continue;
            } else if (name() == "mLightGreen") {
                pHost->mLightGreen.setNamedColor(readElementText());
                continue;
            } else if (name() == "mYellow") {
                pHost->mYellow.setNamedColor(readElementText());
                continue;
            } else if (name() == "mLightYellow") {
                pHost->mLightYellow.setNamedColor(readElementText());
                continue;
            } else if (name() == "mCyan") {
                pHost->mCyan.setNamedColor(readElementText());
                continue;
            } else if (name() == "mLightCyan") {
                pHost->mLightCyan.setNamedColor(readElementText());
                continue;
            } else if (name() == "mMagenta") {
                pHost->mMagenta.setNamedColor(readElementText());
                continue;
            } else if (name() == "mLightMagenta") {
                pHost->mLightMagenta.setNamedColor(readElementText());
                continue;
            } else if (name() == "mWhite") {
                pHost->mWhite.setNamedColor(readElementText());
                continue;
            } else if (name() == "mLightWhite") {
                pHost->mLightWhite.setNamedColor(readElementText());
                continue;
            } else if (name() == "mDisplayFont") {
                pHost->mDisplayFont.fromString(readElementText());
                pHost->mDisplayFont.setFixedPitch(true);
                continue;
            } else if (name() == "mCommandLineFont") {
                pHost->mCommandLineFont.fromString(readElementText());
                continue;
            } else if (name() == "commandSeperator") {
                // Ignore this misspelled duplicate, it has been removed from
                // the Xml format but will appear in older files and trip the
                // QDebug() error reporting associated with the following
                // readUnknownHostElement() for "anything not otherwise parsed"
                Q_UNUSED(readElementText());
            } else if (name() == "mFgColor2") {
                pHost->mFgColor_2.setNamedColor(readElementText());
                continue;
            } else if (name() == "mBgColor2") {
                pHost->mBgColor_2.setNamedColor(readElementText());
                continue;
            } else if (name() == "mBlack2") {
                pHost->mBlack_2.setNamedColor(readElementText());
                continue;
            } else if (name() == "mLightBlack2") {
                pHost->mLightBlack_2.setNamedColor(readElementText());
                continue;
            } else if (name() == "mRed2") {
                pHost->mRed_2.setNamedColor(readElementText());
                continue;
            } else if (name() == "mLightRed2") {
                pHost->mLightRed_2.setNamedColor(readElementText());
                continue;
            } else if (name() == "mBlue2") {
                pHost->mBlue_2.setNamedColor(readElementText());
                continue;
            } else if (name() == "mLightBlue2") {
                pHost->mLightBlue_2.setNamedColor(readElementText());
                continue;
            } else if (name() == "mGreen2") {
                pHost->mGreen_2.setNamedColor(readElementText());
                continue;
            } else if (name() == "mLightGreen2") {
                pHost->mLightGreen_2.setNamedColor(readElementText());
                continue;
            } else if (name() == "mYellow2") {
                pHost->mYellow_2.setNamedColor(readElementText());
                continue;
            } else if (name() == "mLightYellow2") {
                pHost->mLightYellow_2.setNamedColor(readElementText());
                continue;
            } else if (name() == "mCyan2") {
                pHost->mCyan_2.setNamedColor(readElementText());
                continue;
            } else if (name() == "mLightCyan2") {
                pHost->mLightCyan_2.setNamedColor(readElementText());
                continue;
            } else if (name() == "mMagenta2") {
                pHost->mMagenta_2.setNamedColor(readElementText());
                continue;
            } else if (name() == "mLightMagenta2") {
                pHost->mLightMagenta_2.setNamedColor(readElementText());
                continue;
            } else if (name() == "mWhite2") {
                pHost->mWhite_2.setNamedColor(readElementText());
                continue;
            } else if (name() == "mLightWhite2") {
                pHost->mLightWhite_2.setNamedColor(readElementText());
                continue;
            } else if (name() == "mSpellDic") {
                pHost->mSpellDic = readElementText();
                continue;
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
            } else {
                readUnknownHostElement();
            }
        }
    }
}


void XMLimport::readTriggerGroup(TTrigger* pParent)
{
    TTrigger* pT = new TTrigger(pParent, mpHost);

    if (module) {
        pT->mModuleMember = true;
    }

    mpHost->getTriggerUnit()->registerTrigger(pT);

    pT->setIsActive((attributes().value("isActive") == "yes"));
    pT->mIsFolder = (attributes().value("isFolder") == "yes");
    pT->mIsTempTrigger = (attributes().value("isTempTrigger") == "yes");
    pT->mIsMultiline = (attributes().value("isMultiline") == "yes");
    pT->mPerlSlashGOption = (attributes().value("isPerlSlashGOption") == "yes");
    pT->mIsColorizerTrigger = (attributes().value("isColorizerTrigger") == "yes");
    pT->mFilterTrigger = (attributes().value("isFilterTrigger") == "yes");
    pT->mSoundTrigger = (attributes().value("isSoundTrigger") == "yes");
    pT->mColorTrigger = (attributes().value("isColorTrigger") == "yes");
    pT->mColorTriggerBg = (attributes().value("isColorTriggerBg") == "yes");
    pT->mColorTriggerFg = (attributes().value("isColorTriggerFg") == "yes");

    while (!atEnd()) {
        readNext();
        // qDebug()<<"[INFO] element:"<<name().toString()<<" text:"<<text().toString();
        if (isEndElement()) {
            break;
        }

        if (isStartElement()) {
            if (name() == "name") {
                pT->setName(readElementText());
                continue;
            } else if (name() == "script") {
                QString tempScript = readElementText();
                if (!pT->setScript(tempScript)) {
                    qDebug().nospace() << "XMLimport::readTriggerGroup(...): ERROR: can not compile trigger's lua code for: " << pT->getName();
                }
                continue;
            } else if (name() == "packageName") {
                pT->mPackageName = readElementText();
                continue;
            } else if (name() == "triggerType") {
                pT->mTriggerType = readElementText().toInt();
                continue;
            } else if (name() == "conditonLineDelta") {
                pT->mConditionLineDelta = readElementText().toInt();
                continue;
            } else if (name() == "mStayOpen") {
                pT->mStayOpen = readElementText().toInt();
                continue;
            } else if (name() == "mCommand") {
                pT->mCommand = readElementText();
                continue;
            } else if (name() == "mFgColor") {
                pT->mFgColor.setNamedColor(readElementText());
                continue;
            } else if (name() == "mBgColor") {
                pT->mBgColor.setNamedColor(readElementText());
                continue;
            } else if (name() == "colorTriggerFgColor") {
                pT->mColorTriggerFgColor.setNamedColor(readElementText());
                continue;
            } else if (name() == "colorTriggerBgColor") {
                pT->mColorTriggerBgColor.setNamedColor(readElementText());
                continue;
            } else if (name() == "mSoundFile") {
                pT->mSoundFile = readElementText();
                continue;
            } else if (name() == "regexCodeList") {
                // This and the next one ought to be combined into a single element
                // in the next revision - sample code for "RegexCode" elements
                // inside a "RegexList" container (with a "size" attribute) is
                // commented out in the XMLexporter class.
                readStringList(pT->mRegexCodeList);
                continue;
            } else if (name() == "regexCodePropertyList") {
                readIntegerList(pT->mRegexCodePropertyList, pT->getName());
                if (Q_UNLIKELY(pT->mRegexCodeList.count() != pT->mRegexCodePropertyList.count())) {
                    qWarning().nospace() << "XMLimport::readTriggerGroup(...) ERROR: mis-match in regexCode details for Trigger: " << pT->getName() << " there were " << pT->mRegexCodeList.count()
                                         << " 'regexCodeList' sub-elements and " << pT->mRegexCodePropertyList.count() << " 'regexCodePropertyList' sub-elements so something is broken!";
                }
                continue;
            } else if (name() == "TriggerGroup" || name() == "Trigger") {
                readTriggerGroup(pT);
            } else {
                readUnknownTriggerElement();
            }
        }
    }

    if (!pT->setRegexCodeList(pT->mRegexCodeList, pT->mRegexCodePropertyList)) {
        qDebug().nospace() << "XMLimport::readTriggerGroup(...): ERROR: can not initialize pattern list for trigger: " << pT->getName();
    }
}

void XMLimport::readTimerPackage()
{
    while (!atEnd()) {
        readNext();
        if (isEndElement()) {
            break;
        }

        if (isStartElement()) {
            if (name() == "TimerGroup" || name() == "Timer") {
                gotTimer = true;
                if (mPackageName.isEmpty()) {
                    readTimerGroup(0);
                } else {
                    readTimerGroup(mpTimer);
                }
            } else {
                readUnknownTimerElement();
            }
        }
    }
}

void XMLimport::readTimerGroup(TTimer* pParent)
{
    TTimer* pT = new TTimer(pParent, mpHost);

    pT->mIsFolder = (attributes().value("isFolder") == "yes");
    pT->mIsTempTimer = (attributes().value("isTempTimer") == "yes");

    mpHost->getTimerUnit()->registerTimer(pT);
    pT->setShouldBeActive((attributes().value("isActive") == "yes"));

    if (module) {
        pT->mModuleMember = true;
    }

    while (!atEnd()) {
        readNext();
        if (isEndElement()) {
            break;
        }

        if (isStartElement()) {
            if (name() == "name") {

                pT->setName(readElementText());
                continue;
            } else if (name() == "packageName") {
                pT->mPackageName = readElementText();
                continue;
            } else if (name() == "script") {
                QString tempScript = readElementText();
                if (!pT->setScript(tempScript)) {
                    qDebug().nospace() << "XMLimport::readTimerGroup(...): ERROR: can not compile timer's lua code for: " << pT->getName();
                }
                continue;
            } else if (name() == "command") {
                pT->mCommand = readElementText();
                continue;
            } else if (name() == "time") {
                QString timeString = readElementText();
                QTime time = QTime::fromString(timeString, "hh:mm:ss.zzz");
                pT->setTime(time);
                continue;
            } else if (name() == "TimerGroup" || name() == "Timer") {
                readTimerGroup(pT);
            } else {
                readUnknownTimerElement();
            }
        }
    }

    mudlet::self()->registerTimer(pT, pT->mpTimer);

    if (!pT->mpParent && pT->shouldBeActive()) {
        pT->setIsActive(true);
        pT->enableTimer(pT->getID());
    }
}

void XMLimport::readAliasPackage()
{
    while (!atEnd()) {
        readNext();
        if (isEndElement()) {
            break;
        }

        if (isStartElement()) {
            if (name() == "AliasGroup" || name() == "Alias") {
                gotAlias = true;
                if (mPackageName.isEmpty()) {
                    readAliasGroup(0);
                } else {
                    readAliasGroup(mpAlias);
                }
            } else {
                readUnknownAliasElement();
            }
        }
    }
}

void XMLimport::readAliasGroup(TAlias* pParent)
{
    TAlias* pT = new TAlias(pParent, mpHost);

    mpHost->getAliasUnit()->registerAlias(pT);
    pT->setIsActive((attributes().value("isActive") == "yes"));
    pT->mIsFolder = (attributes().value("isFolder") == "yes");
    if (module) {
        pT->mModuleMember = true;
    }

    while (!atEnd()) {
        readNext();
        if (isEndElement()) {
            break;
        }

        if (isStartElement()) {
            if (name() == "name") {
                pT->setName(readElementText());
                continue;
            } else if (name() == "packageName") {
                pT->mPackageName = readElementText();
                continue;
            } else if (name() == "script") {
                QString tempScript = readElementText();
                if (!pT->setScript(tempScript)) {
                    qDebug().nospace() << "XMLimport::readAliasGroup(...): ERROR: can not compile alias's lua code for: " << pT->getName();
                }
                continue;
            } else if (name() == "command") {
                pT->mCommand = readElementText();
                continue;
            } else if (name() == "regex") {
                pT->setRegexCode(readElementText());
                continue;
            } else if (name() == "AliasGroup" || name() == "Alias") {
                readAliasGroup(pT);
            } else {
                readUnknownAliasElement();
            }
        }
    }
}

void XMLimport::readActionPackage()
{
    while (!atEnd()) {
        readNext();
        if (isEndElement()) {
            break;
        }
        if (isStartElement()) {
            if (name() == "ActionGroup" || name() == "Action") {
                gotAction = true;
                if (mPackageName.isEmpty()) {
                    readActionGroup(0);
                } else {
                    readActionGroup(mpAction);
                }
            } else {
                readUnknownActionElement();
            }
        }
    }
}

void XMLimport::readActionGroup(TAction* pParent)
{
    TAction* pT = new TAction(pParent, mpHost);

    mpHost->getActionUnit()->registerAction(pT);
    pT->setIsActive((attributes().value("isActive") == "yes"));
    pT->mIsFolder = (attributes().value("isFolder") == "yes");
    pT->mIsPushDownButton = (attributes().value("isPushButton") == "yes");
    pT->mButtonFlat = (attributes().value("isFlatButton") == "yes");
    pT->mUseCustomLayout = (attributes().value("useCustomLayout") == "yes");
    if (module) {
        pT->mModuleMember = true;
    }

    while (!atEnd()) {
        readNext();
        if (isEndElement()) {
            break;
        }

        if (isStartElement()) {
            if (name() == "name") {
                pT->mName = readElementText();
                continue;
            } else if (name() == "packageName") {
                pT->mPackageName = readElementText();
                continue;
            } else if (name() == "script") {
                QString tempScript = readElementText();
                if (!pT->setScript(tempScript)) {
                    qDebug().nospace() << "XMLimport::readActionGroup(...): ERROR: can not compile action's lua code for: " << pT->getName();
                }
                continue;
            } else if (name() == "css") {
                pT->css = readElementText();
                continue;
            } else if (name() == "commandButtonUp") {
                pT->mCommandButtonUp = readElementText();
                continue;
            } else if (name() == "commandButtonDown") {
                pT->mCommandButtonDown = readElementText();
                continue;
            } else if (name() == "icon") {
                // Must check for attributes BEFORE we read the element text!
                QXmlStreamAttributes tempAttributes( attributes() );
                QString tempPathFile = readElementText();
                if( tempAttributes.hasAttribute( "isRelative" ) ) {
                    if( tempAttributes.value( "isRelative" ) == "true" ) {
                        // Only load the relative pathFileName if we have both
                        pT->setIconPathFileName( tempPathFile );
                    }
                } else {
                    pT->setIconPathFileName( tempPathFile );
                }
                continue;
            } else if (name() == "orientation") {
                pT->mOrientation = readElementText().toInt();
                continue;
            } else if (name() == "location") {
                pT->mLocation = readElementText().toInt();
                continue;
            } else if (name() == "buttonRotation") {
                pT->mButtonRotation = readElementText().toInt();
                continue;
            } else if (name() == "sizeX") {
                pT->mSizeX = readElementText().toInt();
                continue;
            } else if (name() == "sizeY") {
                pT->mSizeY = readElementText().toInt();
                continue;
            } else if (name() == "mButtonState") {
                pT->mButtonState = readElementText().toInt();
                continue;
            } else if (name() == "buttonColor") {
                pT->mButtonColor.setNamedColor(readElementText());
                continue;
            } else if (name() == "buttonColumn") {
                pT->mButtonColumns = readElementText().toInt();
                continue;
            } else if (name() == "posX") {
                pT->mPosX = readElementText().toInt();
                continue;
            } else if (name() == "posY") {
                pT->mPosY = readElementText().toInt();
                continue;
            } else if (name() == "ActionGroup" || name() == "Action") {
                readActionGroup(pT);
            } else {
                readUnknownActionElement();
            }
        }
    }
}

void XMLimport::readScriptPackage()
{
    while (!atEnd()) {
        readNext();
        if (isEndElement()) {
            break;
        }

        if (isStartElement()) {
            if (name() == "ScriptGroup" || name() == "Script") {
                gotScript = true;
                if (mPackageName.isEmpty()) {
                    readScriptGroup(0);
                } else {
                    readScriptGroup(mpScript);
                }
            } else {
                readUnknownScriptElement();
            }
        }
    }
}

void XMLimport::readScriptGroup(TScript* pParent)
{
    TScript* pT = new TScript(pParent, mpHost);

    mpHost->getScriptUnit()->registerScript(pT);
    pT->setIsActive((attributes().value("isActive") == "yes"));
    pT->mIsFolder = (attributes().value("isFolder") == "yes");
    if (module) {
        pT->mModuleMember = true;
    }

    while (!atEnd()) {
        readNext();
        if (isEndElement()) {
            break;
        }

        if (isStartElement()) {
            if (name() == "name") {
                pT->mName = readElementText();
                continue;
            } else if (name() == "packageName") {
                pT->mPackageName = readElementText();
                continue;
            } else if (name() == "script") {
                QString tempScript = readElementText();
                if (!pT->setScript(tempScript)) {
                    qDebug().nospace() << "XMLimport::readScriptGroup(...): ERROR: can not compile script's lua code for: " << pT->getName();
                }
                continue;
            } else if (name() == "eventHandlerList") {
                readStringList(pT->mEventHandlerList);
                pT->setEventHandlerList(pT->mEventHandlerList);
                continue;
            } else if (name() == "ScriptGroup" || name() == "Script") {
                readScriptGroup(pT);
            } else {
                readUnknownScriptElement();
            }
        }
    }
}

void XMLimport::readKeyPackage()
{
    while (!atEnd()) {
        readNext();
        if (isEndElement()) {
            break;
        }

        if (isStartElement()) {
            if (name() == "KeyGroup" || name() == "Key") {
                gotKey = true;
                if (mPackageName.isEmpty()) {
                    readKeyGroup(0);
                } else {
                    readKeyGroup(mpKey);
                }
            } else {
                readUnknownKeyElement();
            }
        }
    }
}

void XMLimport::readKeyGroup(TKey* pParent)
{
    TKey* pT = new TKey(pParent, mpHost);

    mpHost->getKeyUnit()->registerKey(pT);
    pT->setIsActive((attributes().value("isActive") == "yes"));
    pT->mIsFolder = (attributes().value("isFolder") == "yes");
    if (module) {
        pT->mModuleMember = true;
    }

    while (!atEnd()) {
        readNext();
        if (isEndElement()) {
            break;
        }

        if (isStartElement()) {
            if (name() == "name") {
                pT->mName = readElementText();
                continue;
            } else if (name() == "packageName") {
                pT->mPackageName = readElementText();
                continue;
            } else if (name() == "script") {
                QString tempScript = readElementText();
                if (!pT->setScript(tempScript)) {
                    qDebug().nospace() << "XMLimport::readKeyGroup(...): ERROR: can not compile key's lua code for: " << pT->getName();
                }
                continue;
            } else if (name() == "command") {
                pT->mCommand = readElementText();
                continue;
            } else if (name() == "keyCode") {
                pT->mKeyCode = readElementText().toInt();
                continue;
            } else if (name() == "keyModifier") {
                pT->mKeyModifier = readElementText().toInt();
                continue;
            } else if (name() == "KeyGroup" || name() == "Key") {
                readKeyGroup(pT);
            } else {
                readUnknownKeyElement();
            }
        }
    }
}

void XMLimport::readModulesDetailsMap(QMap<QString, QStringList>& map)
{
    QString key;
    QStringList entry;
    while (!atEnd()) {
        readNext();

        if (isEndElement()) {
            break;
        }

        if (isStartElement()) {
            if (name() == "key") {
                key = readElementText();
            } else if (name() == "filepath") {
                entry << readElementText();
                // map[key] = readElementText();
            } else if (name() == "globalSave") {
                entry << readElementText();
            } else if (name() == "priority") {
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
        }

        if (isStartElement()) {
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
        }

        if (isStartElement()) {
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
                    qFatal("XMLimport::readIntegerList(...) ERROR: unable to convert: \"%s\" to a number when reading the 'regexCodePropertyList' element of the 'Trigger' or 'TriggerGroup' element "
                           "\"%s\"!",
                           numberText.toUtf8().constData(), parentName.toUtf8().constData());
                }
            } else {
                readUnknownTriggerElement();
            }
        }
    }
}
