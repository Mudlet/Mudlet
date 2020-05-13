/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2016-2020 by Stephen Lyons - slysven@virginmedia.com    *
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


#include "LuaInterface.h"
#include "TConsole.h"
#include "TMap.h"
#include "TRoomDB.h"
#include "VarUnit.h"
#include "mudlet.h"

#include "pre_guard.h"
#include <QBuffer>
#include <QtMath>
#include "post_guard.h"


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
            if (name() == QStringLiteral("MudletPackage")) {
                QString versionString;
                if (attributes().hasAttribute(QStringLiteral("version"))) {
                    versionString = attributes().value(QStringLiteral("version")).toString();
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
        QSet<int> areaRoomsSet = tempAreaRoomsHash.values(areaId).toSet();

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
        } else if (name() == "area") {
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
            } else {
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
    auto pT = new TRoom(mpHost->mpMap->mpRoomDB);

    pT->id = attributes().value(QStringLiteral("id")).toString().toInt();
    pT->area = attributes().value(QStringLiteral("area")).toString().toInt();
    pT->name = attributes().value(QStringLiteral("title")).toString();
    pT->environment = attributes().value(QStringLiteral("environment")).toString().toInt();

    while (!atEnd()) {
        readNext();

        if (Q_UNLIKELY(pT->id < 1)) {
            continue; // Skip further tests on exits as we'd have to throw away
                      // this invalid room and it would mess up the
                      // entranceMultiHash
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
    return std::make_pair(objectType, rootItemID);
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
            auto result = readPackage();
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
    pHost->mAutoClearCommandLineAfterSend = (attributes().value("autoClearCommandLineAfterSend") == "yes");
    pHost->mPrintCommand = (attributes().value("printCommand") == "yes");
    pHost->set_USE_IRE_DRIVER_BUGFIX(attributes().value("USE_IRE_DRIVER_BUGFIX") == "yes");
    pHost->mUSE_FORCE_LF_AFTER_PROMPT = (attributes().value("mUSE_FORCE_LF_AFTER_PROMPT") == "yes");
    pHost->mUSE_UNIX_EOL = (attributes().value("mUSE_UNIX_EOL") == "yes");
    pHost->getKeyUnit()->mRunAllKeyMatches = (attributes().value("runAllKeyMatches") == "yes");
    pHost->mNoAntiAlias = (attributes().value("mNoAntiAlias") == "yes");
    pHost->mEchoLuaErrors = (attributes().value("mEchoLuaErrors") == "yes");
    if (attributes().hasAttribute("AmbigousWidthGlyphsToBeWide")) {
        const QStringRef ambiguousWidthSetting(attributes().value("AmbigousWidthGlyphsToBeWide"));
        if (ambiguousWidthSetting == QStringLiteral("yes")) {
            pHost->setWideAmbiguousEAsianGlyphs(Qt::Checked);
        } else if (ambiguousWidthSetting == QStringLiteral("auto")) {
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
    pHost->mIsNextLogFileInHtmlFormat = (attributes().value("mRawStreamDump") == "yes");
    pHost->mIsLoggingTimestamps = (attributes().value("mIsLoggingTimestamps") == "yes");
    pHost->mLogDir = attributes().value("logDirectory").toString();
    if (attributes().hasAttribute("logFileNameFormat")) {
        // We previously mixed "yyyy-MM-dd{#|T}hh-MM-ss" with "yyyy-MM-dd{#|T}HH-MM-ss"
        // which is slightly different {always use 24-hour clock even if AM/PM is
        // present (it isn't)} and that broke some code that requires an exact
        // string to work with - now always change it to "HH":
        pHost->mLogFileNameFormat = attributes().value("logFileNameFormat").toString().replace(QLatin1String("hh"), QLatin1String("HH"), Qt::CaseSensitive);
        pHost->mLogFileName = attributes().value("logFileName").toString();
    }
    pHost->mAlertOnNewData = (attributes().value("mAlertOnNewData") == "yes");
    pHost->mFORCE_NO_COMPRESSION = (attributes().value("mFORCE_NO_COMPRESSION") == "yes");
    pHost->mFORCE_GA_OFF = (attributes().value("mFORCE_GA_OFF") == "yes");
    pHost->mFORCE_SAVE_ON_EXIT = (attributes().value("mFORCE_SAVE_ON_EXIT") == "yes");
    pHost->mEnableGMCP = (attributes().value("mEnableGMCP") == "yes");
    pHost->mEnableMSDP = (attributes().value("mEnableMSDP") == "yes");
    if (attributes().hasAttribute(QLatin1String("mEnableMSSP"))) {
        pHost->mEnableMSSP = (attributes().value(QStringLiteral("mEnableMSSP")) == "yes");
    }
    if (attributes().hasAttribute(QLatin1String("mEnableMSP"))) {
        pHost->mEnableMSP = (attributes().value(QStringLiteral("mEnableMSP")) == "yes");
    }
    pHost->mMapStrongHighlight = (attributes().value("mMapStrongHighlight") == "yes");
    pHost->mLogStatus = (attributes().value("mLogStatus") == "yes");
    pHost->mEnableSpellCheck = (attributes().value("mEnableSpellCheck") == "yes");
    bool enableUserDictionary = (attributes().value("mEnableUserDictionary") == "yes");
    bool useSharedDictionary = (attributes().value("mUseSharedDictionary") == "yes");
    pHost->setUserDictionaryOptions(enableUserDictionary, useSharedDictionary);
    pHost->mShowInfo = (attributes().value("mShowInfo") == "yes");
    pHost->mAcceptServerGUI = (attributes().value("mAcceptServerGUI") == "yes");
    if (attributes().hasAttribute(QLatin1String("mAcceptServerMedia"))) {
        pHost->mAcceptServerMedia = (attributes().value("mAcceptServerMedia") == "yes");
    }
    pHost->mMapperUseAntiAlias = (attributes().value("mMapperUseAntiAlias") == "yes");
    if (attributes().hasAttribute(QStringLiteral("mEditorAutoComplete"))) {
        pHost->mEditorAutoComplete = (attributes().value(QStringLiteral("mEditorAutoComplete")) == "yes");
    }
    if (attributes().hasAttribute(QLatin1String("mEditorTheme"))) {
        pHost->mEditorTheme = attributes().value(QLatin1String("mEditorTheme")).toString();
    }
    if (attributes().hasAttribute(QLatin1String("mEditorThemeFile"))) {
        pHost->mEditorThemeFile = attributes().value(QLatin1String("mEditorThemeFile")).toString();
    }
    if (attributes().hasAttribute(QLatin1String("mThemePreviewItemID"))) {
        pHost->mThemePreviewItemID = attributes().value(QLatin1String("mThemePreviewItemID")).toInt();
    }
    if (attributes().hasAttribute(QLatin1String("mThemePreviewType"))) {
        pHost->mThemePreviewType = attributes().value(QLatin1String("mThemePreviewType")).toString();
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
        pHost->mDiscordAccessFlags = static_cast<Host::DiscordOptionFlags>(attributes().value("mDiscordAccessFlags").toString().toInt());
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

    if (attributes().hasAttribute(QLatin1String("mSGRCodeHasColSpaceId"))) {
        pHost->setHaveColorSpaceId(attributes().value(QLatin1String("mSGRCodeHasColSpaceId")).toString() == QLatin1String("yes"));
    } else {
        pHost->setHaveColorSpaceId(false);
    }

    if (attributes().hasAttribute(QLatin1String("mServerMayRedefineColors"))) {
        pHost->setMayRedefineColors(attributes().value(QLatin1String("mServerMayRedefineColors")).toString() == QLatin1String("yes"));
    } else {
        pHost->setMayRedefineColors(false);
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

    pHost->mFORCE_MXP_NEGOTIATION_OFF = (attributes().value("mFORCE_MXP_NEGOTIATION_OFF") == "yes");
    pHost->mEnableTextAnalyzer = (attributes().value("enableTextAnalyzer") == "yes");
    pHost->mRoomSize = attributes().value("mRoomSize").toString().toDouble();
    if (qFuzzyCompare(1.0 + pHost->mRoomSize, 1.0)) {
        // The value is a float/double and the prior code using "== 0" is a BAD
        // THING to do with non-integer number types!
        pHost->mRoomSize = 0.5; // Same value as is in Host class initalizer list
    }
    pHost->mLineSize = attributes().value("mLineSize").toString().toDouble();
    if (qFuzzyCompare(1.0 + pHost->mLineSize, 1.0)) {
        pHost->mLineSize = 10.0; // Same value as is in Host class initalizer list
    }
    pHost->mBubbleMode = (attributes().value("mBubbleMode") == "yes");
    pHost->mShowRoomID = (attributes().value("mShowRoomIDs") == "yes");
    pHost->mShowPanel = (attributes().value("mShowPanel") == "yes");
    pHost->mHaveMapperScript = (attributes().value("mHaveMapperScript") == "yes");
    QStringRef ignore = attributes().value("mDoubleClickIgnore");
    for (auto character : ignore) {
        pHost->mDoubleClickIgnore.insert(character);
    }
    if (attributes().hasAttribute(QLatin1String("EditorSearchOptions"))) {
        pHost->setSearchOptions(static_cast<dlgTriggerEditor::SearchOptions>(attributes().value("EditorSearchOptions").toInt()));
    }
    pHost->mUseProxy = (attributes().value("mUseProxy") == "yes");
    pHost->mProxyAddress = attributes().value("mProxyAddress").toString();
    if (attributes().hasAttribute(QLatin1String("mProxyPort"))) {
        pHost->mProxyPort = attributes().value("mProxyPort").toInt();
    } else {
        pHost->mProxyPort = 0;
    }
    pHost->mProxyUsername = attributes().value("mProxyUsername").toString();
    pHost->mProxyPassword = attributes().value("mProxyPassword").toString();

    pHost->mSslTsl = (attributes().value("mSslTsl") == "yes");
    pHost->mAutoReconnect = (attributes().value("mAutoReconnect") == "yes");
    pHost->mSslIgnoreExpired = (attributes().value("mSslIgnoreExpired") == "yes");
    pHost->mSslIgnoreSelfSigned = (attributes().value("mSslIgnoreSelfSigned") == "yes");
    pHost->mSslIgnoreAll = (attributes().value("mSslIgnoreAll") == "yes");

    while (!atEnd()) {
        readNext();

        if (isEndElement()) {
            break;
        } else if (isStartElement()) {
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
                QFont::insertSubstitution(pHost->mDisplayFont.family(), QStringLiteral("Noto Color Emoji"));
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
            } else if (name() == "stopwatches") {
                readStopWatchMap();
            } else {
                readUnknownHostElement();
            }
        }
    }


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

    pT->setIsActive(attributes().value("isActive") == "yes");
    pT->setIsFolder(attributes().value("isFolder") == "yes");
    pT->setTemporary((attributes().value("isTempTrigger") == "yes"));
    pT->mIsMultiline = (attributes().value("isMultiline") == "yes");
    pT->mPerlSlashGOption = (attributes().value("isPerlSlashGOption") == "yes");
    pT->mIsColorizerTrigger = (attributes().value("isColorizerTrigger") == "yes");
    pT->mFilterTrigger = (attributes().value("isFilterTrigger") == "yes");
    pT->mSoundTrigger = (attributes().value("isSoundTrigger") == "yes");
    pT->mColorTrigger = (attributes().value("isColorTrigger") == "yes");


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
                // inside a "RegexList" container (with a "size" attribute) is
                // commented out in the XMLexporter class.
                readStringList(pT->mRegexCodeList);
            } else if (name() == "regexCodePropertyList") {
                readIntegerList(pT->mRegexCodePropertyList, pT->getName());
                if (Q_UNLIKELY(pT->mRegexCodeList.count() != pT->mRegexCodePropertyList.count())) {
                    qWarning().nospace() << "XMLimport::readTriggerGroup(...) ERROR: "
                                            "mis-match in regexCode details for Trigger: "
                                         << pT->getName() << " there were " << pT->mRegexCodeList.count() << " 'regexCodeList' sub-elements and " << pT->mRegexCodePropertyList.count()
                                         << " 'regexCodePropertyList' sub-elements so "
                                            "something is broken!";
                }
                // Fixup the first 16 incorrect ANSI colour numbers from old
                // code if there are any
                if (!pT->mRegexCodeList.isEmpty()) {
                    remapColorsToAnsiNumber(pT->mRegexCodeList, pT->mRegexCodePropertyList);
                }
            } else if (name() == "TriggerGroup" || name() == "Trigger") {
                readTriggerGroup(pT);
            } else {
                readUnknownTriggerElement();
            }
        }
    }

    if (!pT->setRegexCodeList(pT->mRegexCodeList, pT->mRegexCodePropertyList)) {
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

    pT->setIsFolder(attributes().value("isFolder") == "yes");
    // This should not ever be set here as, by definition, temporary timers
    // are not saved:
    pT->setTemporary(attributes().value("isTempTimer") == "yes");

    // This clears the Tree<TTimer>::mUserActiveState flag so MUST be done
    // BEFORE that flag is parsed:
    mpHost->getTimerUnit()->registerTimer(pT);

    pT->setShouldBeActive(attributes().value("isActive") == "yes");

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
    pT->setIsActive(attributes().value("isActive") == "yes");
    pT->setIsFolder((attributes().value("isFolder") == "yes"));
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

    pT->setIsFolder((attributes().value("isFolder") == "yes"));
    pT->mIsPushDownButton = (attributes().value("isPushButton") == "yes");
    pT->mButtonFlat = (attributes().value("isFlatButton") == "yes");
    pT->mUseCustomLayout = (attributes().value("useCustomLayout") == "yes");
    mpHost->getActionUnit()->registerAction(pT);
    pT->setIsActive(attributes().value("isActive") == "yes");

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
                pT->mButtonColor.setNamedColor(readElementText());
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
    auto pT = new TScript(pParent, mpHost);

    pT->setIsFolder((attributes().value("isFolder") == "yes"));
    mpHost->getScriptUnit()->registerScript(pT);
    pT->setIsActive(attributes().value("isActive") == "yes");

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
                    qDebug().nospace() << "XMLimport::readScriptGroup(...): ERROR: can not compile script's lua code for: " << pT->getName();
                }
            } else if (name() == "eventHandlerList") {
                readStringList(pT->mEventHandlerList);
                pT->setEventHandlerList(pT->mEventHandlerList);
            } else if (name() == "ScriptGroup" || name() == "Script") {
                readScriptGroup(pT);
            } else {
                readUnknownScriptElement();
            }
        }
    }

    return pT->getID();
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
    pT->setIsActive(attributes().value("isActive") == "yes");
    pT->setIsFolder((attributes().value("isFolder") == "yes"));
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
                pT->mKeyCode = readElementText().toInt();
            } else if (name() == "keyModifier") {
                pT->mKeyModifier = readElementText().toInt();
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
            } else if (name() == "globalSave") {
                entry << readElementText();
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
        localScript.replace(QStringLiteral("\xFFFC\x2401"), QChar('\x01')); // SOH
        localScript.replace(QStringLiteral("\xFFFC\x2402"), QChar('\x02')); // STX
        localScript.replace(QStringLiteral("\xFFFC\x2403"), QChar('\x03')); // ETX
        localScript.replace(QStringLiteral("\xFFFC\x2404"), QChar('\x04')); // EOT
        localScript.replace(QStringLiteral("\xFFFC\x2405"), QChar('\x05')); // ENQ
        localScript.replace(QStringLiteral("\xFFFC\x2406"), QChar('\x06')); // ACK
        localScript.replace(QStringLiteral("\xFFFC\x2407"), QChar('\x07')); // BEL
        localScript.replace(QStringLiteral("\xFFFC\x2408"), QChar('\x08')); // BS
        localScript.replace(QStringLiteral("\xFFFC\x240B"), QChar('\x0B')); // VT
        localScript.replace(QStringLiteral("\xFFFC\x240C"), QChar('\x0C')); // FF
        localScript.replace(QStringLiteral("\xFFFC\x240E"), QChar('\x0E')); // SS
        localScript.replace(QStringLiteral("\xFFFC\x240F"), QChar('\x0F')); // SI
        localScript.replace(QStringLiteral("\xFFFC\x2410"), QChar('\x10')); // DLE
        localScript.replace(QStringLiteral("\xFFFC\x2411"), QChar('\x11')); // DC1
        localScript.replace(QStringLiteral("\xFFFC\x2412"), QChar('\x12')); // DC2
        localScript.replace(QStringLiteral("\xFFFC\x2413"), QChar('\x13')); // DC3
        localScript.replace(QStringLiteral("\xFFFC\x2414"), QChar('\x14')); // DC4
        localScript.replace(QStringLiteral("\xFFFC\x2415"), QChar('\x15')); // NAK
        localScript.replace(QStringLiteral("\xFFFC\x2416"), QChar('\x16')); // SYN
        localScript.replace(QStringLiteral("\xFFFC\x2417"), QChar('\x17')); // ETB
        localScript.replace(QStringLiteral("\xFFFC\x2418"), QChar('\x18')); // CAN
        localScript.replace(QStringLiteral("\xFFFC\x2419"), QChar('\x19')); // EM
        localScript.replace(QStringLiteral("\xFFFC\x241A"), QChar('\x1A')); // SUB
        localScript.replace(QStringLiteral("\xFFFC\x241B"), QChar('\x1B')); // ESC
        localScript.replace(QStringLiteral("\xFFFC\x241C"), QChar('\x1C')); // FS
        localScript.replace(QStringLiteral("\xFFFC\x241D"), QChar('\x1D')); // GS
        localScript.replace(QStringLiteral("\xFFFC\x241E"), QChar('\x1E')); // RS
        localScript.replace(QStringLiteral("\xFFFC\x241F"), QChar('\x1F')); // US
        localScript.replace(QStringLiteral("\xFFFC\x2421"), QChar('\x7F')); // DEL
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
    QRegularExpression regex = QRegularExpression(QStringLiteral("FG(-?\\d+)BG(-?\\d+)"));
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
            // Must advance the pattern interator if it isn't a colour pattern
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
                int watchId = attributes().value("id").toInt();
                auto pStopWatch = new stopWatch();
                pStopWatch->setName(attributes().value("name").toString());
                pStopWatch->mIsPersistent = true;
                pStopWatch->mIsInitialised = true;
                if (attributes().value("running") == "yes") {
                    pStopWatch->mIsRunning = true;
                    // The stored value is the point in epoch time that the
                    // stopwatch appears to have been started so we need to
                    // make that into a QDateTime that is the equivalent:
                    pStopWatch->mEffectiveStartDateTime.setMSecsSinceEpoch(attributes().value("effectiveStartDateTimeEpochMSecs").toLongLong());
                } else {
                    pStopWatch->mIsRunning = false;
                    pStopWatch->mElapsedTime = attributes().value("elapsedDateTimeMSecs").toLongLong();
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
