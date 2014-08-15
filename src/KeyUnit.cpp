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


#include "KeyUnit.h"


#include "Host.h"
#include "TKey.h"


using namespace std;

KeyUnit::KeyUnit( Host * pHost )
: mpHost(pHost)
, mMaxID(0)
{
    setupKeyNames();
}


void KeyUnit::_uninstall( TKey * pChild, QString packageName )
{
    typedef list<TKey *>::const_iterator I;
    list<TKey*> * childrenList = pChild->mpMyChildrenList;
    for( I it2 = childrenList->begin(); it2 != childrenList->end(); it2++)
    {
        TKey * pT = *it2;
        _uninstall( pT, packageName );
        uninstallList.append( pT );
    }
}


void KeyUnit::uninstall( QString packageName )
{
    typedef std::list<TKey *>::iterator IT;
    for( IT it = mKeyRootNodeList.begin(); it != mKeyRootNodeList.end(); it ++ )
    {
        TKey * pT = *it;

        if( pT->mPackageName == packageName )
        {
            _uninstall( pT, packageName );
            uninstallList.append( pT );
        }
    }
    for( int i=0; i<uninstallList.size(); i++ )
    {
        unregisterKey(uninstallList[i]);
    }
     uninstallList.clear();
}

bool KeyUnit::processDataStream( int key, int modifier )
{
    typedef list<TKey *>::const_iterator I;
    for( I it = mKeyRootNodeList.begin(); it != mKeyRootNodeList.end(); it++)
    {
        TKey * pChild = *it;
        if( pChild->match( key, modifier ) ) return true;
    }

    return false;
}

void KeyUnit::compileAll()
{
    typedef list<TKey *>::const_iterator I;
    for( I it = mKeyRootNodeList.begin(); it != mKeyRootNodeList.end(); it++)
    {
        TKey * pChild = *it;
        if( pChild->isActive() )
        {
            pChild->compileAll();
        }
    }
}

bool KeyUnit::enableKey( QString & name )
{
    bool found = false;
    QMutexLocker locker(& mKeyUnitLock);
    typedef list<TKey *>::const_iterator I;
    for( I it = mKeyRootNodeList.begin(); it != mKeyRootNodeList.end(); it++)
    {
        TKey * pChild = *it;
        pChild->enableKey( name );
        found = true;
    }
    return found;
}

bool KeyUnit::disableKey( QString & name )
{
    bool found = false;
    QMutexLocker locker(& mKeyUnitLock);
    typedef list<TKey *>::const_iterator I;
    for( I it = mKeyRootNodeList.begin(); it != mKeyRootNodeList.end(); it++)
    {
        TKey * pChild = *it;
        pChild->disableKey( name );
        found = true;
    }
    return found;
}

void KeyUnit::addKeyRootNode( TKey * pT, int parentPosition, int childPosition )
{
    if( ! pT ) return;
    if( ! pT->getID() )
    {
        pT->setID( getNewID() );
    }

    if( ( parentPosition == -1 ) || ( childPosition >= static_cast<int>(mKeyRootNodeList.size()) ) )
    {
        mKeyRootNodeList.push_back( pT );
    }
    else
    {
        // insert item at proper position
        int cnt = 0;
        typedef std::list<TKey *>::iterator IT;
        for( IT it = mKeyRootNodeList.begin(); it != mKeyRootNodeList.end(); it ++ )
        {
            if( cnt >= childPosition )
            {
                mKeyRootNodeList.insert( it, pT );
                break;
            }
            cnt++;
        }
    }

    if( mKeyMap.find( pT->getID() ) == mKeyMap.end() )
    {
        mKeyMap.insert( pT->getID(), pT );
    }
}

void KeyUnit::reParentKey( int childID, int oldParentID, int newParentID, int parentPosition, int childPosition )
{
    QMutexLocker locker(& mKeyUnitLock);

    TKey * pOldParent = getKeyPrivate( oldParentID );
    TKey * pNewParent = getKeyPrivate( newParentID );
    TKey * pChild = getKeyPrivate( childID );
    if( ! pChild )
    {
        return;
    }
    if( pOldParent )
    {
        pOldParent->popChild( pChild );
    }
    if( ! pOldParent )
    {
        removeKeyRootNode( pChild );
    }
    if( pNewParent )
    {
        pNewParent->addChild( pChild, parentPosition, childPosition );
        if( pChild )
        {
            pChild->setParent( pNewParent );
        }
        //cout << "dumping family of newParent:"<<endl;
        //pNewParent->Dump();
    }
    else
    {
        pChild->Tree<TKey>::setParent( 0 );
        addKeyRootNode( pChild, parentPosition, childPosition );
    }
}

void KeyUnit::removeKeyRootNode( TKey * pT )
{
    if( ! pT ) return;
    mKeyRootNodeList.remove( pT );
}

TKey * KeyUnit::getKey( int id )
{
    QMutexLocker locker(& mKeyUnitLock);
    if( mKeyMap.find( id ) != mKeyMap.end() )
    {
        return mKeyMap.value( id );
    }
    else
    {
        return 0;
    }
}



TKey * KeyUnit::getKeyPrivate( int id )
{
    if( mKeyMap.find( id ) != mKeyMap.end() )
    {
        return mKeyMap.value( id );
    }
    else
    {
        return 0;
    }
}

bool KeyUnit::registerKey( TKey * pT )
{
    if( ! pT ) return false;

    if( pT->getParent() )
    {
        addKey( pT );
        return true;
    }
    else
    {
        addKeyRootNode( pT );
        return true;
    }
}

void KeyUnit::unregisterKey( TKey * pT )
{
    if( ! pT ) return;
    if( pT->getParent() )
    {
        removeKey( pT );
        return;
    }
    else
    {
        removeKeyRootNode( pT );
        return;
    }
}


void KeyUnit::addKey( TKey * pT )
{
    if( ! pT ) return;

    pT->setID( getNewID() );

    mKeyMap.insert( pT->getID(), pT );
}

void KeyUnit::removeKey( TKey * pT )
{
    if( ! pT ) return;
    mKeyMap.remove( pT->getID() );
}


qint64 KeyUnit::getNewID()
{
    return ++mMaxID;
}

QString KeyUnit::getKeyName( int keyCode, int modifierCode )
{
    QString name;
    /*
     Qt::NoModifier      0x00000000 No modifier key is pressed.
     Qt::ShiftModifier   0x02000000 A Shift key on the keyboard is pressed.
     Qt::ControlModifier 0x04000000 A Ctrl key on the keyboard is pressed.
     Qt::AltModifier     0x08000000 An Alt key on the keyboard is pressed.
     Qt::MetaModifier    0x10000000 A Meta key on the keyboard is pressed.
     Qt::KeypadModifier  0x20000000 A keypad button is pressed.
     Qt::GroupSwitchModifier 0x40000000 X11 only. A Mode_switch key on the keyboard is pressed.
    */
    if( modifierCode == 0x00000000 ) name += "no modifiers + ";
    if( modifierCode & 0x02000000 ) name += "shift + ";
    if( modifierCode & 0x04000000 ) name += "control + ";
    if( modifierCode & 0x08000000 ) name += "alt + ";
    if( modifierCode & 0x10000000 ) name += "meta + ";
    if( modifierCode & 0x20000000 ) name += "keypad + ";
    if( modifierCode & 0x40000000 ) name += "groupswitch + ";
    if( mKeys.contains( keyCode ) )
    {
        name += mKeys[keyCode];
        return name;
    }
    else return QString("undefined key");
}



void KeyUnit::setupKeyNames()
{
    mKeys[0x01000000]=QString("Escape");
    mKeys[0x01000001]=QString("Tab");
    mKeys[0x01000002]=QString("Backtab");
    mKeys[0x01000003]=QString("Backspace");
    mKeys[0x01000004]=QString("Return");
    mKeys[0x01000005]=QString("Enter");
    mKeys[0x01000006]=QString("Insert");
    mKeys[0x01000007]=QString("Delete");
    mKeys[0x01000008]=QString("Pause");
    mKeys[0x01000009]=QString("Print");
    mKeys[0x0100000a]=QString("SysReq");
    mKeys[0x0100000b]=QString("Clear");
    mKeys[0x01000010]=QString("Home");
    mKeys[0x01000011]=QString("End");
    mKeys[0x01000012]=QString("Left");
    mKeys[0x01000013]=QString("Up");
    mKeys[0x01000014]=QString("Right");
    mKeys[0x01000015]=QString("Down");
    mKeys[0x01000016]=QString("PageUp");
    mKeys[0x01000017]=QString("PageDown");
    mKeys[0x01000020]=QString("Shift");
    mKeys[0x01000021]=QString("Control");
    mKeys[0x01000022]=QString("Meta");
    mKeys[0x01000023]=QString("Alt");
    mKeys[0x01001103]=QString("AltGr");
    mKeys[0x01000024]=QString("CapsLock");
    mKeys[0x01000025]=QString("NumLock");
    mKeys[0x01000026]=QString("ScrollLock");
    mKeys[0x01000030]=QString("F1");
    mKeys[0x01000031]=QString("F2");
    mKeys[0x01000032]=QString("F3");
    mKeys[0x01000033]=QString("F4");
    mKeys[0x01000034]=QString("F5");
    mKeys[0x01000035]=QString("F6");
    mKeys[0x01000036]=QString("F7");
    mKeys[0x01000037]=QString("F8");
    mKeys[0x01000038]=QString("F9");
    mKeys[0x01000039]=QString("F10");
    mKeys[0x0100003a]=QString("F11");
    mKeys[0x0100003b]=QString("F12");
    mKeys[0x0100003c]=QString("F13");
    mKeys[0x0100003d]=QString("F14");
    mKeys[0x0100003e]=QString("F15");
    mKeys[0x0100003f]=QString("F16");
    mKeys[0x01000040]=QString("F17");
    mKeys[0x01000041]=QString("F18");
    mKeys[0x01000042]=QString("F19");
    mKeys[0x01000043]=QString("F20");
    mKeys[0x01000044]=QString("F21");
    mKeys[0x01000045]=QString("F22");
    mKeys[0x01000046]=QString("F23");
    mKeys[0x01000047]=QString("F24");
    mKeys[0x01000048]=QString("F25");
    mKeys[0x01000049]=QString("F26");
    mKeys[0x0100004a]=QString("F27");
    mKeys[0x0100004b]=QString("F28");
    mKeys[0x0100004c]=QString("F29");
    mKeys[0x0100004d]=QString("F30");
    mKeys[0x0100004e]=QString("F31");
    mKeys[0x0100004f]=QString("F32");
    mKeys[0x01000050]=QString("F33");
    mKeys[0x01000051]=QString("F34");
    mKeys[0x01000052]=QString("F35");
    mKeys[0x01000053]=QString("Super_L");
    mKeys[0x01000054]=QString("Super_R");
    mKeys[0x01000055]=QString("Menu");
    mKeys[0x01000056]=QString("Hyper_L");
    mKeys[0x01000057]=QString("Hyper_R");
    mKeys[0x01000058]=QString("Help");
    mKeys[0x01000059]=QString("Direction_L");
    mKeys[0x01000060]=QString("Direction_R");
    mKeys[0x20]=QString("Space");
    mKeys[0x21]=QString("Exclam");
    mKeys[0x22]=QString("QuoteDbl");
    mKeys[0x23]=QString("NumberSign");
    mKeys[0x24]=QString("Dollar");
    mKeys[0x25]=QString("Percent");
    mKeys[0x26]=QString("Ampersand");
    mKeys[0x27]=QString("Apostrophe");
    mKeys[0x28]=QString("ParenLeft");
    mKeys[0x29]=QString("ParenRight");
    mKeys[0x2a]=QString("Asterisk");
    mKeys[0x2b]=QString("Plus");
    mKeys[0x2c]=QString("Comma");
    mKeys[0x2d]=QString("Minus");
    mKeys[0x2e]=QString("Period");
    mKeys[0x2f]=QString("Slash");
    mKeys[0x30]=QString("0");
    mKeys[0x31]=QString("1");
    mKeys[0x32]=QString("2");
    mKeys[0x33]=QString("3");
    mKeys[0x34]=QString("4");
    mKeys[0x35]=QString("5");
    mKeys[0x36]=QString("6");
    mKeys[0x37]=QString("7");
    mKeys[0x38]=QString("8");
    mKeys[0x39]=QString("9");
    mKeys[0x3a]=QString("Colon");
    mKeys[0x3b]=QString("Semicolon");
    mKeys[0x3c]=QString("Less");
    mKeys[0x3d]=QString("Equal");
    mKeys[0x3e]=QString("Greater");
    mKeys[0x3f]=QString("Question");
    mKeys[0x40]=QString("At");
    mKeys[0x41]=QString("A");
    mKeys[0x42]=QString("B");
    mKeys[0x43]=QString("C");
    mKeys[0x44]=QString("D");
    mKeys[0x45]=QString("E");
    mKeys[0x46]=QString("F");
    mKeys[0x47]=QString("G");
    mKeys[0x48]=QString("H");
    mKeys[0x49]=QString("I");
    mKeys[0x4a]=QString("J");
    mKeys[0x4b]=QString("K");
    mKeys[0x4c]=QString("L");
    mKeys[0x4d]=QString("M");
    mKeys[0x4e]=QString("N");
    mKeys[0x4f]=QString("O");
    mKeys[0x50]=QString("P");
    mKeys[0x51]=QString("Q");
    mKeys[0x52]=QString("R");
    mKeys[0x53]=QString("S");
    mKeys[0x54]=QString("T");
    mKeys[0x55]=QString("U");
    mKeys[0x56]=QString("V");
    mKeys[0x57]=QString("W");
    mKeys[0x58]=QString("X");
    mKeys[0x59]=QString("Y");
    mKeys[0x5a]=QString("Z");
    mKeys[0x5b]=QString("BracketLeft");
    mKeys[0x5c]=QString("Backslash");
    mKeys[0x5d]=QString("BracketRight");
    mKeys[0x5e]=QString("AsciiCircum");
    mKeys[0x5f]=QString("Underscore");
    mKeys[0x60]=QString("QuoteLeft");
    mKeys[0x7b]=QString("BraceLeft");
    mKeys[0x7c]=QString("Bar");
    mKeys[0x7d]=QString("BraceRight");
    mKeys[0x7e]=QString("AsciiTilde");
    mKeys[0x0a0]=QString("nobreakspace");
    mKeys[0x0a1]=QString("exclamdown");
    mKeys[0x0a2]=QString("cent");
    mKeys[0x0a3]=QString("sterling");
    mKeys[0x0a4]=QString("currency");
    mKeys[0x0a5]=QString("yen");
    mKeys[0x0a6]=QString("brokenbar");
    mKeys[0x0a7]=QString("section");
    mKeys[0x0a8]=QString("diaeresis");
    mKeys[0x0a9]=QString("copyright");
    mKeys[0x0aa]=QString("ordfeminine");
    mKeys[0x0ab]=QString("guillemotleft");
    mKeys[0x0ac]=QString("notsign");
    mKeys[0x0ad]=QString("hyphen");
    mKeys[0x0ae]=QString("registered");
    mKeys[0x0af]=QString("macron");
    mKeys[0x0b0]=QString("degree");
    mKeys[0x0b1]=QString("plusminus");
    mKeys[0x0b2]=QString("twosuperior");
    mKeys[0x0b3]=QString("threesuperior");
    mKeys[0x0b4]=QString("acute");
    mKeys[0x0b5]=QString("mu");
    mKeys[0x0b6]=QString("paragraph");
    mKeys[0x0b7]=QString("periodcentered");
    mKeys[0x0b8]=QString("cedilla");
    mKeys[0x0b9]=QString("onesuperior");
    mKeys[0x0ba]=QString("masculine");
    mKeys[0x0bb]=QString("guillemotright");
    mKeys[0x0bc]=QString("onequarter");
    mKeys[0x0bd]=QString("onehalf");
    mKeys[0x0be]=QString("threequarters");
    mKeys[0x0bf]=QString("questiondown");
    mKeys[0x0c0]=QString("Agrave");
    mKeys[0x0c1]=QString("Aacute");
    mKeys[0x0c2]=QString("Acircumflex");
    mKeys[0x0c3]=QString("Atilde");
    mKeys[0x0c4]=QString("Adiaeresis");
    mKeys[0x0c5]=QString("Aring");
    mKeys[0x0c6]=QString("AE");
    mKeys[0x0c7]=QString("Ccedilla");
    mKeys[0x0c8]=QString("Egrave");
    mKeys[0x0c9]=QString("Eacute");
    mKeys[0x0ca]=QString("Ecircumflex");
    mKeys[0x0cb]=QString("Ediaeresis");
    mKeys[0x0cc]=QString("Igrave");
    mKeys[0x0cd]=QString("Iacute");
    mKeys[0x0ce]=QString("Icircumflex");
    mKeys[0x0cf]=QString("Idiaeresis");
    mKeys[0x0d0]=QString("ETH");
    mKeys[0x0d1]=QString("Ntilde");
    mKeys[0x0d2]=QString("Ograve");
    mKeys[0x0d3]=QString("Oacute");
    mKeys[0x0d4]=QString("Ocircumflex");
    mKeys[0x0d5]=QString("Otilde");
    mKeys[0x0d6]=QString("Odiaeresis");
    mKeys[0x0d7]=QString("multiply");
    mKeys[0x0d8]=QString("Ooblique");
    mKeys[0x0d9]=QString("Ugrave");
    mKeys[0x0da]=QString("Uacute");
    mKeys[0x0db]=QString("Ucircumflex");
    mKeys[0x0dc]=QString("Udiaeresis");
    mKeys[0x0dd]=QString("Yacute");
    mKeys[0x0de]=QString("THORN");
    mKeys[0x0df]=QString("ssharp");
    mKeys[0x0f7]=QString("division");
    mKeys[0x0ff]=QString("ydiaeresis");
    mKeys[0x01001120]=QString("Multi_key");
    mKeys[0x01001137]=QString("Codeinput");
    mKeys[0x0100113c]=QString("SingleCandidate");
    mKeys[0x0100113d]=QString("MultipleCandidate");
    mKeys[0x0100113e]=QString("PreviousCandidate");
    mKeys[0x0100117e]=QString("Mode_switch");
    mKeys[0x01001121]=QString("Kanji");
    mKeys[0x01001122]=QString("Muhenkan");
    mKeys[0x01001123]=QString("Henkan");
    mKeys[0x01001124]=QString("Romaji");
    mKeys[0x01001125]=QString("Hiragana");
    mKeys[0x01001126]=QString("Katakana");
    mKeys[0x01001127]=QString("Hiragana_Katakana");
    mKeys[0x01001128]=QString("Zenkaku");
    mKeys[0x01001129]=QString("Hankaku");
    mKeys[0x0100112a]=QString("Zenkaku_Hankaku");
    mKeys[0x0100112b]=QString("Touroku");
    mKeys[0x0100112c]=QString("Massyo");
    mKeys[0x0100112d]=QString("Kana_Lock");
    mKeys[0x0100112e]=QString("Kana_Shift");
    mKeys[0x0100112f]=QString("Eisu_Shift");
    mKeys[0x01001130]=QString("Eisu_toggle");
    mKeys[0x01001131]=QString("Hangul");
    mKeys[0x01001132]=QString("Hangul_Start");
    mKeys[0x01001133]=QString("Hangul_End");
    mKeys[0x01001134]=QString("Hangul_Hanja");
    mKeys[0x01001135]=QString("Hangul_Jamo");
    mKeys[0x01001136]=QString("Hangul_Romaja");
    mKeys[0x01001138]=QString("Hangul_Jeonja");
    mKeys[0x01001139]=QString("Hangul_Banja");
    mKeys[0x0100113a]=QString("Hangul_PreHanja");
    mKeys[0x0100113b]=QString("Hangul_PostHanja");
    mKeys[0x0100113f]=QString("Hangul_Special");
    mKeys[0x01001250]=QString("Dead_Grave");
    mKeys[0x01001251]=QString("Dead_Acute");
    mKeys[0x01001252]=QString("Dead_Circumflex");
    mKeys[0x01001253]=QString("Dead_Tilde");
    mKeys[0x01001254]=QString("Dead_Macron");
    mKeys[0x01001255]=QString("Dead_Breve");
    mKeys[0x01001256]=QString("Dead_Abovedot");
    mKeys[0x01001257]=QString("Dead_Diaeresis");
    mKeys[0x01001258]=QString("Dead_Abovering");
    mKeys[0x01001259]=QString("Dead_Doubleacute");
    mKeys[0x0100125a]=QString("Dead_Caron");
    mKeys[0x0100125b]=QString("Dead_Cedilla");
    mKeys[0x0100125c]=QString("Dead_Ogonek");
    mKeys[0x0100125d]=QString("Dead_Iota");
    mKeys[0x0100125e]=QString("Dead_Voiced_Sound");
    mKeys[0x0100125f]=QString("Dead_Semivoiced_Sound");
    mKeys[0x01001260]=QString("Dead_Belowdot");
    mKeys[0x01001261]=QString("Dead_Hook");
    mKeys[0x01001262]=QString("Dead_Horn");
    mKeys[0x01000061]=QString("Back");
    mKeys[0x01000062]=QString("Forward");
    mKeys[0x01000063]=QString("Stop");
    mKeys[0x01000064]=QString("Refresh");
    mKeys[0x01000070]=QString("VolumeDown");
    mKeys[0x01000071]=QString("VolumeMute");
    mKeys[0x01000072]=QString("VolumeUp");
    mKeys[0x01000073]=QString("BassBoost");
    mKeys[0x01000074]=QString("BassUp");
    mKeys[0x01000075]=QString("BassDown");
    mKeys[0x01000076]=QString("TrebleUp");
    mKeys[0x01000077]=QString("TrebleDown");
    mKeys[0x01000080]=QString("MediaPlay");
    mKeys[0x01000081]=QString("MediaStop");
    mKeys[0x01000082]=QString("MediaPrevious");
    mKeys[0x01000083]=QString("MediaNext");
    mKeys[0x01000084]=QString("MediaRecord");
    mKeys[0x01000090]=QString("HomePage");
    mKeys[0x01000091]=QString("Favorites");
    mKeys[0x01000092]=QString("Search");
    mKeys[0x01000093]=QString("Standby");
    mKeys[0x01000094]=QString("OpenUrl");
    mKeys[0x010000a0]=QString("LaunchMail");
    mKeys[0x010000a1]=QString("LaunchMedia");
    mKeys[0x010000a2]=QString("Launch0");
    mKeys[0x010000a3]=QString("Launch1");
    mKeys[0x010000a4]=QString("Launch2");
    mKeys[0x010000a5]=QString("Launch3");
    mKeys[0x010000a6]=QString("Launch4");
    mKeys[0x010000a7]=QString("Launch5");
    mKeys[0x010000a8]=QString("Launch6");
    mKeys[0x010000a9]=QString("Launch7");
    mKeys[0x010000aa]=QString("Launch8");
    mKeys[0x010000ab]=QString("Launch9");
    mKeys[0x010000ac]=QString("LaunchA");
    mKeys[0x010000ad]=QString("LaunchB");
    mKeys[0x010000ae]=QString("LaunchC");
    mKeys[0x010000af]=QString("LaunchD");
    mKeys[0x010000b0]=QString("LaunchE");
    mKeys[0x010000b1]=QString("LaunchF");
    mKeys[0x0100ffff]=QString("MediaLast");
    mKeys[0x01ffffff]=QString("unknown");
    mKeys[0x01100004]=QString("Call");
    mKeys[0x01100000]=QString("Context1");
    mKeys[0x01100001]=QString("Context2");
    mKeys[0x01100002]=QString("Context3");
    mKeys[0x01100003]=QString("Context4");
    mKeys[0x01100006]=QString("Flip");
    mKeys[0x01100005]=QString("Hangup");
    mKeys[0x01010002]=QString("No");
    mKeys[0x01010000]=QString("Select");
    mKeys[0x01010001]=QString("Yes");
    mKeys[0x01020003]=QString("Execute");
    mKeys[0x01020002]=QString("Printer");
    mKeys[0x01020005]=QString("Play");
    mKeys[0x01020004]=QString("Sleep");
    mKeys[0x01020006]=QString("Zoom");
    mKeys[0x01020001]=QString("Cancel");
}
