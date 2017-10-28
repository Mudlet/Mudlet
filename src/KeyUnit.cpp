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


#include "KeyUnit.h"


#include "mudlet.h"
#include "Host.h"
#include "TKey.h"


using namespace std;

KeyUnit::KeyUnit(Host* pHost) : mpHost(pHost), mMaxID(0), mModuleMember()
{

    slot_guiLanguageChange();
    if (mudlet::self()) {
        // CHECK: mudlet::self() is null during start-up at this point for the
        // "default_host" case - it is not clear that this will be an issue...
        connect(mudlet::self(), SIGNAL(signal_translatorChangeCompleted(const QString&, const QString&)), this, SLOT(slot_guiLanguageChange()));
    }
}

void KeyUnit::_uninstall(TKey* pChild, const QString& packageName)
{
    list<TKey*>* childrenList = pChild->mpMyChildrenList;
    for (auto key : *childrenList) {
        _uninstall(key, packageName);
        uninstallList.append(key);
    }
}


void KeyUnit::uninstall(const QString& packageName)
{
    for (auto rootKey : mKeyRootNodeList) {
        if (rootKey->mPackageName == packageName) {
            _uninstall(rootKey, packageName);
            uninstallList.append(rootKey);
        }
    }
    for (auto& key : uninstallList) {
        unregisterKey(key);
    }
    uninstallList.clear();
}

bool KeyUnit::processDataStream(int key, int modifier)
{
    for (auto keyObject : mKeyRootNodeList) {
        if (keyObject->match(key, modifier)) {
            return true;
        }
    }

    return false;
}

void KeyUnit::compileAll()
{
    for (auto key : mKeyRootNodeList) {
        if (key->isActive()) {
            key->compileAll();
        }
    }
}

TKey* KeyUnit::findKey(QString& name)
{
    QMap<QString, TKey*>::const_iterator it = mLookupTable.constFind(name);
    while (it != mLookupTable.cend() && it.key() == name) {
        TKey* pT = it.value();
        return pT;
    }
    return nullptr;
}

bool KeyUnit::enableKey(const QString& name)
{
    bool found = false;
    QMutexLocker locker(&mKeyUnitLock);
    for (auto key : mKeyRootNodeList) {
        key->enableKey(name);
        found = true;
    }
    return found;
}

bool KeyUnit::disableKey(const QString& name)
{
    bool found = false;
    QMutexLocker locker(&mKeyUnitLock);
    for (auto key : mKeyRootNodeList) {
        key->disableKey(name);
        found = true;
    }
    return found;
}

bool KeyUnit::killKey(QString& name)
{
    for (auto it = mKeyRootNodeList.begin(); it != mKeyRootNodeList.end(); it++) {
        TKey* pChild = *it;
        if (pChild->getName() == name) {
            // only temporary Keys can be killed
            if (!pChild->isTemporary()) {
                return false;
            } else {
                pChild->setIsActive(false);
                markCleanup(pChild);
                return true;
            }
        }
    }
    return false;
}

void KeyUnit::addKeyRootNode(TKey* pT, int parentPosition, int childPosition)
{
    if (!pT) {
        return;
    }
    if (!pT->getID()) {
        pT->setID(getNewID());
    }

    if ((parentPosition == -1) || (childPosition >= static_cast<int>(mKeyRootNodeList.size()))) {
        mKeyRootNodeList.push_back(pT);
    } else {
        // insert item at proper position
        int cnt = 0;
        for (auto it = mKeyRootNodeList.begin(); it != mKeyRootNodeList.end(); it++) {
            if (cnt >= childPosition) {
                mKeyRootNodeList.insert(it, pT);
                break;
            }
            cnt++;
        }
    }

    if (mKeyMap.find(pT->getID()) == mKeyMap.end()) {
        mKeyMap.insert(pT->getID(), pT);
    }
}

void KeyUnit::reParentKey(int childID, int oldParentID, int newParentID, int parentPosition, int childPosition)
{
    QMutexLocker locker(&mKeyUnitLock);

    TKey* pOldParent = getKeyPrivate(oldParentID);
    TKey* pNewParent = getKeyPrivate(newParentID);
    TKey* pChild = getKeyPrivate(childID);
    if (!pChild) {
        return;
    }
    if (pOldParent) {
        pOldParent->popChild(pChild);
    }
    if (!pOldParent) {
        removeKeyRootNode(pChild);
    }
    if (pNewParent) {
        pNewParent->addChild(pChild, parentPosition, childPosition);
        if (pChild) {
            pChild->setParent(pNewParent);
        }
        //cout << "dumping family of newParent:"<<endl;
        //pNewParent->Dump();
    } else {
        pChild->Tree<TKey>::setParent(nullptr);
        addKeyRootNode(pChild, parentPosition, childPosition);
    }
}

void KeyUnit::removeKeyRootNode(TKey* pT)
{
    if (!pT) {
        return;
    }
    if (!pT->isTemporary()) {
        mLookupTable.remove(pT->getName(), pT);
    } else {
        mLookupTable.remove(pT->getName());
    }
    mKeyMap.remove(pT->getID());
    mKeyRootNodeList.remove(pT);
}

TKey* KeyUnit::getKey(int id)
{
    QMutexLocker locker(&mKeyUnitLock);
    if (mKeyMap.find(id) != mKeyMap.end()) {
        return mKeyMap.value(id);
    } else {
        return nullptr;
    }
}


TKey* KeyUnit::getKeyPrivate(int id)
{
    if (mKeyMap.find(id) != mKeyMap.end()) {
        return mKeyMap.value(id);
    } else {
        return nullptr;
    }
}

bool KeyUnit::registerKey(TKey* pT)
{
    if (!pT) {
        return false;
    }

    if (pT->getParent()) {
        addKey(pT);
        return true;
    } else {
        addKeyRootNode(pT);
        return true;
    }
}

void KeyUnit::unregisterKey(TKey* pT)
{
    if (!pT) {
        return;
    }
    if (pT->getParent()) {
        removeKey(pT);
        return;
    } else {
        removeKeyRootNode(pT);
        return;
    }
}


void KeyUnit::addKey(TKey* pT)
{
    if (!pT) {
        return;
    }

    pT->setID(getNewID());

    mKeyMap.insert(pT->getID(), pT);
}

void KeyUnit::removeKey(TKey* pT)
{
    if (!pT) {
        return;
    }
    if (!pT->isTemporary()) {
        mLookupTable.remove(pT->getName(), pT);
    } else {
        mLookupTable.remove(pT->getName());
    }
    mKeyMap.remove(pT->getID());
}


int KeyUnit::getNewID()
{
    return ++mMaxID;
}

QString KeyUnit::getKeyName(int keyCode, int modifierCode)
{
    /*
     Qt::NoModifier      0x00000000 No modifier key is pressed.
     Qt::ShiftModifier   0x02000000 A Shift key on the keyboard is pressed.
     Qt::ControlModifier 0x04000000 A Ctrl key on the keyboard is pressed.
     Qt::AltModifier     0x08000000 An Alt key on the keyboard is pressed.
     Qt::MetaModifier    0x10000000 A Meta key on the keyboard is pressed.
     Qt::KeypadModifier  0x20000000 A keypad button is pressed.
     Qt::GroupSwitchModifier 0x40000000 X11 only. A Mode_switch key on the keyboard is pressed.
    */
    QStringList elements;
    /*: The name for a key is made by assembling a list of modifiers and then
     * either the base key name from the look-up table with a couple of hundreds
     * of key names or the fragment here which gives the system code for the key
     * if the code is not present in that table; all these elements are joined
     * together with a " + " between them which will read from left to right in
     * LTR locales - should this algorithm not be suitable for your language
     * please raise it with the Mudlet developers!
     */
    QString hexCode = tr("unknown key (0x%1)").arg(QString::number(keyCode, 16).toUpper());

    if (modifierCode & Qt::ShiftModifier) {
        elements << tr("shift");
    }

    if (modifierCode & Qt::ControlModifier) {
#if defined(Q_OS_MACOS)
        //: This is the translation for the ControlModifier on macOS Platform where this is the "Command" key
        elements << tr("command");
#else
        //: This is the translation for the ControlModifier on non-macOS Platforms where this is the "Control" key
        elements << tr("control");
#endif
    }

    if (modifierCode & Qt::AltModifier) {
        elements << tr("alt");
    }

    if (modifierCode & Qt::MetaModifier) {
#if defined(Q_OS_MACOS)
        //: This is the translation for the MetaModifier key on macOS Platform where this is the "Control" key
        elements << tr("control");
#else
        //: This is the translation for the MetaModifier key on non-macOS Platform where this is the "Meta" key
        elements << tr("meta");
#endif
    }

    if (modifierCode & Qt::KeypadModifier) {
        elements << tr("keypad");
    }

    if (modifierCode & Qt::GroupSwitchModifier) {
        elements << tr("groupswitch");
    }

    if (elements.isEmpty()) {
        elements << tr("no modifiers");
    }

    elements << mKeys.value(keyCode, hexCode);
#if QT_VERSION >= 0x050800
    // This form was only introduced in Qt 5.8:
    return elements.join(QLatin1String(" + "));
#else
    return elements.join(QStringLiteral(" + "));
#endif
}

void KeyUnit::initStats()
{
    statsKeyTotal = 0;
    statsTempKeys = 0;
    statsActiveKeys = 0;
    statsActiveKeysMax = 0;
    statsActiveKeysMin = 0;
    statsActiveKeysAverage = 0;
    statsTempKeysCreated = 0;
    statsTempKeysKilled = 0;
}

void KeyUnit::_assembleReport(TKey* pChild)
{
    list<TKey*>* childrenList = pChild->mpMyChildrenList;
    for (auto it2 = childrenList->begin(); it2 != childrenList->end(); it2++) {
        TKey* pT = *it2;
        _assembleReport(pT);
        if (pT->isActive()) {
            statsActiveKeys++;
        }
        if (pT->isTemporary()) {
            statsTempKeys++;
        }
        statsKeyTotal++;
    }
}

QString KeyUnit::assembleReport()
{
    statsActiveKeys = 0;
    statsKeyTotal = 0;
    statsTempKeys = 0;
    for (auto it = mKeyRootNodeList.begin(); it != mKeyRootNodeList.end(); it++) {
        TKey* pChild = *it;
        if (pChild->isActive()) {
            statsActiveKeys++;
        }
        if (pChild->isTemporary()) {
            statsTempKeys++;
        }
        statsKeyTotal++;
        list<TKey*>* childrenList = pChild->mpMyChildrenList;
        for (auto it2 = childrenList->begin(); it2 != childrenList->end(); it2++) {
            TKey* pT = *it2;
            _assembleReport(pT);
            if (pT->isActive()) {
                statsActiveKeys++;
            }
            if (pT->isTemporary()) {
                statsTempKeys++;
            }
            statsKeyTotal++;
        }
    }

    return tr("Keys current total: %1\n"
              "tempKeys current total: %2\n"
              "active Keys: %3\n")
            .arg(QString::number(statsKeyTotal),
                 QString::number(statsTempKeys),
                 QString::number(statsActiveKeys));
}

void KeyUnit::markCleanup(TKey* pT)
{
    for (auto it = mCleanupList.begin(); it != mCleanupList.end(); it++) {
        if (*it == pT) {
            return;
        }
    }
    mCleanupList.push_back(pT);
}

void KeyUnit::doCleanup()
{
    for (auto it = mCleanupList.begin(); it != mCleanupList.end(); it++) {
        delete *it;
    }
    mCleanupList.clear();
}

void KeyUnit::slot_guiLanguageChange()
{
    // PLACEMARKER: Redefine GUI Texts
    mKeys[0x20] = tr("Space");
    mKeys[0x21] = tr("Exclam");
    mKeys[0x22] = tr("QuoteDbl");
    mKeys[0x23] = tr("NumberSign");
    mKeys[0x24] = tr("Dollar");
    mKeys[0x25] = tr("Percent");
    mKeys[0x26] = tr("Ampersand");
    mKeys[0x27] = tr("Apostrophe");
    mKeys[0x28] = tr("ParenLeft");
    mKeys[0x29] = tr("ParenRight");
    mKeys[0x2a] = tr("Asterisk");
    mKeys[0x2b] = tr("Plus");
    mKeys[0x2c] = tr("Comma");
    mKeys[0x2d] = tr("Minus");
    mKeys[0x2e] = tr("Period");
    mKeys[0x2f] = tr("Slash");
    mKeys[0x30] = tr("0");
    mKeys[0x31] = tr("1");
    mKeys[0x32] = tr("2");
    mKeys[0x33] = tr("3");
    mKeys[0x34] = tr("4");
    mKeys[0x35] = tr("5");
    mKeys[0x36] = tr("6");
    mKeys[0x37] = tr("7");
    mKeys[0x38] = tr("8");
    mKeys[0x39] = tr("9");
    mKeys[0x3a] = tr("Colon");
    mKeys[0x3b] = tr("Semicolon");
    mKeys[0x3c] = tr("Less");
    mKeys[0x3d] = tr("Equal");
    mKeys[0x3e] = tr("Greater");
    mKeys[0x3f] = tr("Question");
    mKeys[0x40] = tr("At");
    mKeys[0x41] = tr("A");
    mKeys[0x42] = tr("B");
    mKeys[0x43] = tr("C");
    mKeys[0x44] = tr("D");
    mKeys[0x45] = tr("E");
    mKeys[0x46] = tr("F");
    mKeys[0x47] = tr("G");
    mKeys[0x48] = tr("H");
    mKeys[0x49] = tr("I");
    mKeys[0x4a] = tr("J");
    mKeys[0x4b] = tr("K");
    mKeys[0x4c] = tr("L");
    mKeys[0x4d] = tr("M");
    mKeys[0x4e] = tr("N");
    mKeys[0x4f] = tr("O");
    mKeys[0x50] = tr("P");
    mKeys[0x51] = tr("Q");
    mKeys[0x52] = tr("R");
    mKeys[0x53] = tr("S");
    mKeys[0x54] = tr("T");
    mKeys[0x55] = tr("U");
    mKeys[0x56] = tr("V");
    mKeys[0x57] = tr("W");
    mKeys[0x58] = tr("X");
    mKeys[0x59] = tr("Y");
    mKeys[0x5a] = tr("Z");
    mKeys[0x5b] = tr("BracketLeft");
    mKeys[0x5c] = tr("Backslash");
    mKeys[0x5d] = tr("BracketRight");
    mKeys[0x5e] = tr("AsciiCircum");
    mKeys[0x5f] = tr("Underscore");
    mKeys[0x60] = tr("QuoteLeft");

    mKeys[0x7b] = tr("BraceLeft");
    mKeys[0x7c] = tr("Bar");
    mKeys[0x7d] = tr("BraceRight");
    mKeys[0x7e] = tr("AsciiTilde");

    mKeys[0x0a0] = tr("nobreakspace");
    mKeys[0x0a1] = tr("exclamdown");
    mKeys[0x0a2] = tr("cent");
    mKeys[0x0a3] = tr("sterling");
    mKeys[0x0a4] = tr("currency");
    mKeys[0x0a5] = tr("yen");
    mKeys[0x0a6] = tr("brokenbar");
    mKeys[0x0a7] = tr("section");
    mKeys[0x0a8] = tr("diaeresis");
    mKeys[0x0a9] = tr("copyright");
    mKeys[0x0aa] = tr("ordfeminine");
    mKeys[0x0ab] = tr("guillemotleft");
    mKeys[0x0ac] = tr("notsign");
    mKeys[0x0ad] = tr("hyphen");
    mKeys[0x0ae] = tr("registered");
    mKeys[0x0af] = tr("macron");
    mKeys[0x0b0] = tr("degree");
    mKeys[0x0b1] = tr("plusminus");
    mKeys[0x0b2] = tr("twosuperior");
    mKeys[0x0b3] = tr("threesuperior");
    mKeys[0x0b4] = tr("acute");
    mKeys[0x0b5] = tr("mu");
    mKeys[0x0b6] = tr("paragraph");
    mKeys[0x0b7] = tr("periodcentered");
    mKeys[0x0b8] = tr("cedilla");
    mKeys[0x0b9] = tr("onesuperior");
    mKeys[0x0ba] = tr("masculine");
    mKeys[0x0bb] = tr("guillemotright");
    mKeys[0x0bc] = tr("onequarter");
    mKeys[0x0bd] = tr("onehalf");
    mKeys[0x0be] = tr("threequarters");
    mKeys[0x0bf] = tr("questiondown");
    mKeys[0x0c0] = tr("Agrave");
    mKeys[0x0c1] = tr("Aacute");
    mKeys[0x0c2] = tr("Acircumflex");
    mKeys[0x0c3] = tr("Atilde");
    mKeys[0x0c4] = tr("Adiaeresis");
    mKeys[0x0c5] = tr("Aring");
    mKeys[0x0c6] = tr("AE");
    mKeys[0x0c7] = tr("Ccedilla");
    mKeys[0x0c8] = tr("Egrave");
    mKeys[0x0c9] = tr("Eacute");
    mKeys[0x0ca] = tr("Ecircumflex");
    mKeys[0x0cb] = tr("Ediaeresis");
    mKeys[0x0cc] = tr("Igrave");
    mKeys[0x0cd] = tr("Iacute");
    mKeys[0x0ce] = tr("Icircumflex");
    mKeys[0x0cf] = tr("Idiaeresis");
    mKeys[0x0d0] = tr("ETH");
    mKeys[0x0d1] = tr("Ntilde");
    mKeys[0x0d2] = tr("Ograve");
    mKeys[0x0d3] = tr("Oacute");
    mKeys[0x0d4] = tr("Ocircumflex");
    mKeys[0x0d5] = tr("Otilde");
    mKeys[0x0d6] = tr("Odiaeresis");
    mKeys[0x0d7] = tr("multiply");
    mKeys[0x0d8] = tr("Ooblique");
    mKeys[0x0d9] = tr("Ugrave");
    mKeys[0x0da] = tr("Uacute");
    mKeys[0x0db] = tr("Ucircumflex");
    mKeys[0x0dc] = tr("Udiaeresis");
    mKeys[0x0dd] = tr("Yacute");
    mKeys[0x0de] = tr("THORN");
    mKeys[0x0df] = tr("ssharp");

    mKeys[0x0f7] = tr("division");
    mKeys[0x0ff] = tr("ydiaeresis");

    mKeys[0x01000000] = tr("Escape");
    mKeys[0x01000001] = tr("Tab");
    mKeys[0x01000002] = tr("Backtab");
    mKeys[0x01000003] = tr("Backspace");
    mKeys[0x01000004] = tr("Return");
    mKeys[0x01000005] = tr("Enter");
    mKeys[0x01000006] = tr("Insert");
    mKeys[0x01000007] = tr("Delete");
    mKeys[0x01000008] = tr("Pause");
    mKeys[0x01000009] = tr("Print");
    mKeys[0x0100000a] = tr("SysReq");
    mKeys[0x0100000b] = tr("Clear");

    mKeys[0x01000010] = tr("Home");
    mKeys[0x01000011] = tr("End");
    mKeys[0x01000012] = tr("Left");
    mKeys[0x01000013] = tr("Up");
    mKeys[0x01000014] = tr("Right");
    mKeys[0x01000015] = tr("Down");
    mKeys[0x01000016] = tr("PageUp");
    mKeys[0x01000017] = tr("PageDown");

    mKeys[0x01000020] = tr("Shift");
    mKeys[0x01000021] = tr("Control");
    mKeys[0x01000022] = tr("Meta");
    mKeys[0x01000023] = tr("Alt");
    mKeys[0x01000024] = tr("CapsLock");
    mKeys[0x01000025] = tr("NumLock");
    mKeys[0x01000026] = tr("ScrollLock");

    mKeys[0x01000030] = tr("F1");
    mKeys[0x01000031] = tr("F2");
    mKeys[0x01000032] = tr("F3");
    mKeys[0x01000033] = tr("F4");
    mKeys[0x01000034] = tr("F5");
    mKeys[0x01000035] = tr("F6");
    mKeys[0x01000036] = tr("F7");
    mKeys[0x01000037] = tr("F8");
    mKeys[0x01000038] = tr("F9");
    mKeys[0x01000039] = tr("F10");
    mKeys[0x0100003a] = tr("F11");
    mKeys[0x0100003b] = tr("F12");
    mKeys[0x0100003c] = tr("F13");
    mKeys[0x0100003d] = tr("F14");
    mKeys[0x0100003e] = tr("F15");
    mKeys[0x0100003f] = tr("F16");
    mKeys[0x01000040] = tr("F17");
    mKeys[0x01000041] = tr("F18");
    mKeys[0x01000042] = tr("F19");
    mKeys[0x01000043] = tr("F20");
    mKeys[0x01000044] = tr("F21");
    mKeys[0x01000045] = tr("F22");
    mKeys[0x01000046] = tr("F23");
    mKeys[0x01000047] = tr("F24");
    mKeys[0x01000048] = tr("F25");
    mKeys[0x01000049] = tr("F26");
    mKeys[0x0100004a] = tr("F27");
    mKeys[0x0100004b] = tr("F28");
    mKeys[0x0100004c] = tr("F29");
    mKeys[0x0100004d] = tr("F30");
    mKeys[0x0100004e] = tr("F31");
    mKeys[0x0100004f] = tr("F32");
    mKeys[0x01000050] = tr("F33");
    mKeys[0x01000051] = tr("F34");
    mKeys[0x01000052] = tr("F35");
    mKeys[0x01000053] = tr("Super_L");
    mKeys[0x01000054] = tr("Super_R");
    mKeys[0x01000055] = tr("Menu");
    mKeys[0x01000056] = tr("Hyper_L");
    mKeys[0x01000057] = tr("Hyper_R");
    mKeys[0x01000058] = tr("Help");
    mKeys[0x01000059] = tr("Direction_L");

    mKeys[0x01000060] = tr("Direction_R");
    mKeys[0x01000061] = tr("Back");
    mKeys[0x01000062] = tr("Forward");
    mKeys[0x01000063] = tr("Stop");
    mKeys[0x01000064] = tr("Refresh");

    mKeys[0x01000070] = tr("VolumeDown");
    mKeys[0x01000071] = tr("VolumeMute");
    mKeys[0x01000072] = tr("VolumeUp");
    mKeys[0x01000073] = tr("BassBoost");
    mKeys[0x01000074] = tr("BassUp");
    mKeys[0x01000075] = tr("BassDown");
    mKeys[0x01000076] = tr("TrebleUp");
    mKeys[0x01000077] = tr("TrebleDown");

    mKeys[0x01000080] = tr("MediaPlay");
    mKeys[0x01000081] = tr("MediaStop");
    mKeys[0x01000082] = tr("MediaPrevious");
    mKeys[0x01000083] = tr("MediaNext");
    mKeys[0x01000084] = tr("MediaRecord");

    mKeys[0x01000090] = tr("HomePage");
    mKeys[0x01000091] = tr("Favorites");
    mKeys[0x01000092] = tr("Search");
    mKeys[0x01000093] = tr("Standby");
    mKeys[0x01000094] = tr("OpenUrl");

    mKeys[0x010000a0] = tr("LaunchMail");
    mKeys[0x010000a1] = tr("LaunchMedia");
    mKeys[0x010000a2] = tr("Launch0");
    mKeys[0x010000a3] = tr("Launch1");
    mKeys[0x010000a4] = tr("Launch2");
    mKeys[0x010000a5] = tr("Launch3");
    mKeys[0x010000a6] = tr("Launch4");
    mKeys[0x010000a7] = tr("Launch5");
    mKeys[0x010000a8] = tr("Launch6");
    mKeys[0x010000a9] = tr("Launch7");
    mKeys[0x010000aa] = tr("Launch8");
    mKeys[0x010000ab] = tr("Launch9");
    mKeys[0x010000ac] = tr("LaunchA");
    mKeys[0x010000ad] = tr("LaunchB");
    mKeys[0x010000ae] = tr("LaunchC");
    mKeys[0x010000af] = tr("LaunchD");
    mKeys[0x010000b0] = tr("LaunchE");
    mKeys[0x010000b1] = tr("LaunchF");

    mKeys[0x01001103] = tr("AltGr");

    mKeys[0x01001120] = tr("Multi_key");
    mKeys[0x01001121] = tr("Kanji");
    mKeys[0x01001122] = tr("Muhenkan");
    mKeys[0x01001123] = tr("Henkan");
    mKeys[0x01001124] = tr("Romaji");
    mKeys[0x01001125] = tr("Hiragana");
    mKeys[0x01001126] = tr("Katakana");
    mKeys[0x01001127] = tr("Hiragana_Katakana");
    mKeys[0x01001128] = tr("Zenkaku");
    mKeys[0x01001129] = tr("Hankaku");
    mKeys[0x0100112a] = tr("Zenkaku_Hankaku");
    mKeys[0x0100112b] = tr("Touroku");
    mKeys[0x0100112c] = tr("Massyo");
    mKeys[0x0100112d] = tr("Kana_Lock");
    mKeys[0x0100112e] = tr("Kana_Shift");
    mKeys[0x0100112f] = tr("Eisu_Shift");
    mKeys[0x01001130] = tr("Eisu_toggle");
    mKeys[0x01001131] = tr("Hangul");
    mKeys[0x01001132] = tr("Hangul_Start");
    mKeys[0x01001133] = tr("Hangul_End");
    mKeys[0x01001134] = tr("Hangul_Hanja");
    mKeys[0x01001135] = tr("Hangul_Jamo");
    mKeys[0x01001136] = tr("Hangul_Romaja");
    mKeys[0x01001137] = tr("Codeinput");
    mKeys[0x01001138] = tr("Hangul_Jeonja");
    mKeys[0x01001139] = tr("Hangul_Banja");
    mKeys[0x0100113a] = tr("Hangul_PreHanja");
    mKeys[0x0100113b] = tr("Hangul_PostHanja");
    mKeys[0x0100113c] = tr("SingleCandidate");
    mKeys[0x0100113d] = tr("MultipleCandidate");
    mKeys[0x0100113e] = tr("PreviousCandidate");
    mKeys[0x0100113f] = tr("Hangul_Special");

    mKeys[0x0100117e] = tr("Mode_switch");

    mKeys[0x01001250] = tr("Dead_Grave");
    mKeys[0x01001251] = tr("Dead_Acute");
    mKeys[0x01001252] = tr("Dead_Circumflex");
    mKeys[0x01001253] = tr("Dead_Tilde");
    mKeys[0x01001254] = tr("Dead_Macron");
    mKeys[0x01001255] = tr("Dead_Breve");
    mKeys[0x01001256] = tr("Dead_Abovedot");
    mKeys[0x01001257] = tr("Dead_Diaeresis");
    mKeys[0x01001258] = tr("Dead_Abovering");
    mKeys[0x01001259] = tr("Dead_Doubleacute");
    mKeys[0x0100125a] = tr("Dead_Caron");
    mKeys[0x0100125b] = tr("Dead_Cedilla");
    mKeys[0x0100125c] = tr("Dead_Ogonek");
    mKeys[0x0100125d] = tr("Dead_Iota");
    mKeys[0x0100125e] = tr("Dead_Voiced_Sound");
    mKeys[0x0100125f] = tr("Dead_Semivoiced_Sound");
    mKeys[0x01001260] = tr("Dead_Belowdot");
    mKeys[0x01001261] = tr("Dead_Hook");
    mKeys[0x01001262] = tr("Dead_Horn");

    mKeys[0x0100ffff] = tr("MediaLast");

    mKeys[0x01010000] = tr("Select");
    mKeys[0x01010001] = tr("Yes");
    mKeys[0x01010002] = tr("No");

    mKeys[0x01020001] = tr("Cancel");
    mKeys[0x01020002] = tr("Printer");
    mKeys[0x01020003] = tr("Execute");
    mKeys[0x01020004] = tr("Sleep");
    mKeys[0x01020005] = tr("Play");
    mKeys[0x01020006] = tr("Zoom");

    mKeys[0x01100000] = tr("Context1");
    mKeys[0x01100001] = tr("Context2");
    mKeys[0x01100002] = tr("Context3");
    mKeys[0x01100003] = tr("Context4");
    mKeys[0x01100004] = tr("Call");
    mKeys[0x01100005] = tr("Hangup");
    mKeys[0x01100006] = tr("Flip");

    mKeys[0x01ffffff] = tr("Unknown");
}
