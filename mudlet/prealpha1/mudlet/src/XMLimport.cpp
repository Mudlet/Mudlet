/***************************************************************************
 *   Copyright (C) 2008 by Heiko Koehn  ( KoehnHeiko@googlemail.com )      *
 *                                                                         *                                                                         *
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
#include <QStringList>
#include <QDebug>

XMLimport::XMLimport( Host * pH )
: mpHost( pH )
{
}

bool XMLimport::importPackage( QIODevice * device )
{
    setDevice( device );
    
    while( ! atEnd() ) 
    {
        readNext();
        
        if( isStartElement() ) 
        {
            qDebug()<<"FOUND package HEADER";
            if( name() == "MudletPackage" )// && attributes().value("version") == "1.0")
            {
                readPackage();
            }
            else
            {
                qDebug()<<"ERROR:name="<<name().toString()<<"text:"<<text().toString();
                raiseError( QObject::tr("The file is not a Mudlet package version 1.0 file.") );
            }
        }
    }
    return ! error();
}

void XMLimport::readPackage()
{
    while( ! atEnd() ) 
    {
        readNext();
        qDebug()<<"-----> readPackage() Package name="<<name().toString()<<"text:"<<text().toString();
        if( isEndElement() )
        {
            break;
        }
        
        if( isStartElement() ) 
        {
            if( name() == "HostPackage" )
            {
                readHostPackage();
                continue;
            }
            if( name() == "TriggerPackage" )
            {
                readTriggerPackage();
                continue;
            }
            else if( name() == "TimerPackage" )
            {
                readTimerPackage();
                continue;
            }
            else if( name() == "AliasPackage" )
            {
                readAliasPackage();
                continue;
            }
            else if( name() == "ActionPackage" )
            {
                readActionPackage();
                continue;
            }
            else if( name() == "ScriptPackage" )
            {
                readScriptPackage();
                continue;
            }
            else if( name() == "KeyPackage" )
            {
                readKeyPackage();
                continue;
            }
            else
            {
                readUnknownPackage();
            }
        }
    }
}

void XMLimport::readUnknownPackage()
{
    while( ! atEnd() ) 
    {
        
        readNext();
        qDebug()<<"[ERROR]: UNKNOWN readUnknownPackage() Package Element:"<<name().toString()<<"text:"<<text().toString();        
        if( isEndElement() )
        {
            break;
        }
        
        if( isStartElement() )
        {
            readPackage();
        }
    }
}

void XMLimport::readUnknownHostElement()
{
    while( ! atEnd() ) 
    {
        
        readNext();
        qDebug()<<"[ERROR]: UNKNOWN Host Package Element:name="<<name().toString()<<"text:"<<text().toString();
        
        if( isEndElement() )
        {
            break;
        }
        
        if( isStartElement() )
        {
            readHostPackage( mpHost );
        }
    }
}


void XMLimport::readUnknownTriggerElement()
{
    while( ! atEnd() ) 
    {
        
        readNext();
        qDebug()<<"[ERROR]: UNKNOWN Trigger Package Element:name="<<name().toString()<<"text:"<<text().toString();
        
        if( isEndElement() )
        {
            break;
        }
        
        if( isStartElement() )
        {
            readTriggerPackage();
        }
    }
}

void XMLimport::readUnknownTimerElement()
{
    while( ! atEnd() ) 
    {
        
        readNext();
        qDebug()<<"[ERROR]: UNKNOWN Timer Package Element:name="<<name().toString()<<"text:"<<text().toString();
        
        if( isEndElement() )
        {
            break;
        }
        
        if( isStartElement() )
        {
            readTimerPackage();
        }
    }
}

void XMLimport::readUnknownAliasElement()
{
    while( ! atEnd() ) 
    {
        
        readNext();
        qDebug()<<"[ERROR]: UNKNOWN Alias Package Element:name="<<name().toString()<<"text:"<<text().toString();
        
        if( isEndElement() )
        {
            break;
        }
        
        if( isStartElement() )
        {
            readAliasPackage();
        }
    }
}

void XMLimport::readUnknownActionElement()
{
    while( ! atEnd() ) 
    {
        
        readNext();
        qDebug()<<"[ERROR]: UNKNOWN Action Package Element:name="<<name().toString()<<"text:"<<text().toString();
        
        if( isEndElement() )
        {
            break;
        }
        
        if( isStartElement() )
        {
            readActionPackage();
        }
    }
}

void XMLimport::readUnknownScriptElement()
{
    while( ! atEnd() ) 
    {
        
        readNext();
        qDebug()<<"[ERROR]: UNKNOWN Script Package Element:name="<<name().toString()<<"text:"<<text().toString();
        
        if( isEndElement() )
        {
            break;
        }
        
        if( isStartElement() )
        {
            readScriptPackage();
        }
    }
}

void XMLimport::readUnknownKeyElement()
{
    while( ! atEnd() ) 
    {
        
        readNext();
        qDebug()<<"[ERROR]: UNKNOWN Key Package Element:name="<<name().toString()<<"text:"<<text().toString();
        
        if( isEndElement() )
        {
            break;
        }
        
        if( isStartElement() )
        {
            readKeyPackage();
        }
    }
}



void XMLimport::readTriggerPackage()
{
    while( ! atEnd() ) 
    {
        readNext();
        if( isEndElement() )
        {
            break;
        }
        
        if( isStartElement() ) 
        {
            if( name() == "TriggerGroup" )
            {
                readTriggerGroup( 0 );
            }
            else if( name() == "Trigger" )
            {
                readTriggerGroup( 0 );
            }
            else
            {
                readUnknownTriggerElement();
            }
        }
    }
}

void XMLimport::readHostPackage()
{
    while( ! atEnd() ) 
    {
        readNext();
        if( isEndElement() )
        {
            break;
        }
        
        if( isStartElement() ) 
        {
            if( name() == "Host" )
            {
                readHostPackage( mpHost );
            }
           
            else
            {
                qDebug()<<"[INFO] unknown Host element (readHostPackage())";
                readUnknownHostElement();
            }
        }
    }
}

void XMLimport::readHostPackage( Host * pT )
{
    pT->mAutoClearCommandLineAfterSend = ( attributes().value("autoClearCommandLineAfterSend") == "yes" );
    pT->mDisableAutoCompletion = ( attributes().value("disableAutoCompletion") == "yes" );
    pT->mPrintCommand = ( attributes().value("printCommand") == "yes" );
    
    while( ! atEnd() ) 
    {
        readNext();
        if( isEndElement() ) break;
        
        if( isStartElement() ) 
        {
            if( name() == "name" )
            {
                pT->mHostName = readElementText();
                qDebug()<<"hostName="<<pT->mHostName;
                continue;
            }
            else if( name() == "login")
            {
                pT->mLogin = readElementText();
                qDebug()<<"login="<<pT->mLogin;
                continue;
            }
            else if( name() == "pass")
            {
                pT->mPass = readElementText();
                qDebug()<<"pass="<<pT->mPass;
                continue;
            }       
            else if( name() =="url" )
            {
                pT->mUrl = readElementText();
                qDebug()<<"url="<<pT->mUrl;
                continue;
            }       
                
            else if( name() == "port")
            {
                pT->mPort = readElementText().toInt();
                qDebug()<<"port="<<pT->mPort;
                continue;
            }            
            else if( name() == "wrapAt")
            {
                pT->mWrapAt = readElementText().toInt();
                qDebug()<<"wrapAt="<<pT->mWrapAt;
                continue;
            }
            else if( name() == "wrapIndentCount" )
            {
                pT->mWrapIndentCount = readElementText().toInt();
                qDebug()<<"wrapIndentCount"<<pT->mWrapIndentCount;
                continue;
            }
            else if( name() == "commandSeperator" )
            {
                pT->mCommandSeperator = readElementText();
                continue;
            }
            else if( name() == "mFgColor")
            {
                pT->mFgColor.setNamedColor( readElementText() );
                continue;
            }
            else if( name() == "mBgColor")
            {
                pT->mBgColor.setNamedColor( readElementText() );
                continue;
            } 
            else if( name() == "mBlack")
            {
                pT->mBlack.setNamedColor( readElementText() );
                continue;
            } 
            else if( name() == "mLightBlack")
            {
                pT->mLightBlack.setNamedColor( readElementText() );
                continue;
            } 
            else if( name() == "mRed")
            {
                pT->mRed.setNamedColor( readElementText() );
                continue;
            } 
            else if( name() == "mLightRed")
            {
                pT->mLightRed.setNamedColor( readElementText() );
                continue;
            } 
            else if( name() == "mBlue")
            {
                pT->mBlue.setNamedColor( readElementText() );
                continue;
            } 
            else if( name() == "mLightBlue")
            {
                pT->mLightBlue.setNamedColor( readElementText() );
                continue;
            } 
            else if( name() == "mGreen")
            {
                pT->mGreen.setNamedColor( readElementText() );
                continue;
            } 
            else if( name() == "mLightGreen")
            {
                pT->mLightGreen.setNamedColor( readElementText() );
                continue;
            } 
            else if( name() == "mYellow")
            {
                pT->mYellow.setNamedColor( readElementText() );
                continue;
            } 
            else if( name() == "mLightYellow")
            {
                pT->mLightYellow.setNamedColor( readElementText() );
                continue;
            } 
            else if( name() == "mCyan")
            {
                pT->mCyan.setNamedColor( readElementText() );
                continue;
            } 
            else if( name() == "mLightCyan")
            {
                pT->mLightCyan.setNamedColor( readElementText() );
                continue;
            } 
            else if( name() == "mMagenta")
            {
                pT->mMagenta.setNamedColor( readElementText() );
                continue;
            } 
            else if( name() == "mLightMagenta")
            {
                pT->mLightMagenta.setNamedColor( readElementText() );
                continue;
            } 
            else if( name() == "mWhite")
            {
                pT->mWhite.setNamedColor( readElementText() );
                continue;
            } 
            else if( name() == "mLightWhite")
            {
                pT->mLightWhite.setNamedColor( readElementText() );
                continue;
            } 
            else if( name() == "mDisplayFont")
            {
                pT->mDisplayFont.fromString( readElementText() );
                continue;
            } 
            else if( name() == "mCommandLineFont")
            {
                pT->mCommandLineFont.fromString( readElementText() );
                continue;
            } 
            else
            {
                qDebug()<<"[INFO] UNKNOWN Trigger element: name="<<name().toString()<<"text:"<<text().toString();
                readUnknownHostElement();
            } 
        }
    }
}


void XMLimport::readTriggerGroup( TTrigger * pParent )
{
    TTrigger * pT;
    if( pParent ) 
    {
        pT = new TTrigger( pParent, mpHost );
    } 
    else 
    {
        pT = new TTrigger( 0, mpHost );
    }
    
    pT->mIsActive = ( attributes().value("isActive") == "yes" );
    pT->mIsFolder = ( attributes().value("isFolder") == "yes" );
    pT->mIsTempTrigger = ( attributes().value("isTempTrigger") == "yes" );
    pT->mIsMultiline = ( attributes().value("isMultiline") == "yes" );
    
    while( ! atEnd() ) 
    {
        readNext();
        qDebug()<<"[INFO] element:"<<name().toString()<<" text:"<<text().toString();        
        if( isEndElement() ) break;
        
        if( isStartElement() ) 
        {
            if( name() == "name" )
            {
                pT->mName = readElementText();
                qDebug()<<"name="<<pT->mName;
                continue;
            }
            else if( name() == "script")
            {
                pT->mScript = readElementText();
                qDebug()<<"script="<<pT->mScript;
                continue;
            }
            else if( name() == "triggerType" )
            {
                pT->mTriggerType = readElementText().toInt();
                qDebug()<<"triggerType="<<pT->mTriggerType;
                continue;
            }
            else if( name() == "conditonLineDelta" )
            {
                pT->mConditionLineDelta = readElementText().toInt();
                qDebug()<<"conditonLineDelta="<<pT->mConditionLineDelta;
                continue;
            }
            else if( name() == "mCommand" )
            {
                pT->mCommand = readElementText();
                continue;
            }
            else if( name() == "regexCodeList")
            {
                readStringList( pT->mRegexCodeList );
                qDebug()<<"regexCodeList="<<pT->mRegexCodeList;
                continue;
            }
            else if( name() == "regexCodePropertyList" )
            {
                readIntegerList( pT->mRegexCodePropertyList );
                qDebug()<<"regexCodePropertyList="<<pT->mRegexCodePropertyList;
                qDebug()<<"-----------------------------------------------------\n";
                continue;
            }
            
            else if( name() == "TriggerGroup" )
            {
                readTriggerGroup( pT );
            }
            else if( name() == "Trigger" )
            {
                readTriggerGroup( pT );
            }
            else
            {
                qDebug()<<"[INFO] UNKNOWN Trigger element: name="<<name().toString()<<"text:"<<text().toString();
                readUnknownTriggerElement();
            }
        }
    }
    pT->setRegexCodeList( pT->mRegexCodeList, pT->mRegexCodePropertyList );
    qDebug()<<"[REGISTER] Trigger:"<<pT->getName();
    mpHost->getTriggerUnit()->registerTrigger( pT );
    
}

void XMLimport::readTimerPackage()
{
    while( ! atEnd() ) 
    {
        readNext();
        if( isEndElement() )
        {
            break;
        }
        
        if( isStartElement() ) 
        {
            if( name() == "TimerGroup" )
            {
                readTimerGroup( 0 );
            }
            else if( name() == "Timer" )
            {
                readTimerGroup( 0 );
            }
            else
            {
                readUnknownTimerElement();
            }
        }
    }
}

void XMLimport::readTimerGroup( TTimer * pParent )
{
    TTimer * pT;
    if( pParent ) 
    {
        pT = new TTimer( pParent, mpHost );
    } 
    else 
    {
        pT = new TTimer( 0, mpHost );
    }
    
    pT->mUserActiveState = ( attributes().value("isActive") == "yes" );
    pT->mIsActive = pT->mUserActiveState;
    pT->mIsFolder = ( attributes().value("isFolder") == "yes" );
    pT->mIsTempTimer = ( attributes().value("isTempTimer") == "yes" );
    
    while( ! atEnd() ) 
    {
        readNext();
        if( isEndElement() ) break;
        
        if( isStartElement() ) 
        {
            if( name() == "name" )
            {
                pT->mName = readElementText();
                qDebug()<<"name="<<pT->mName;
                continue;
            }
            else if( name() == "script")
            {
                pT->mScript = readElementText();
                qDebug()<<"script="<<pT->mScript;
                continue;
            }
            else if( name() == "command")
            {
                pT->mCommand = readElementText();
                qDebug()<<"command="<<pT->mCommand;
                continue;
            }
            else if( name() == "time")
            {
                QString timeString = readElementText();
                QTime time = QTime::fromString( timeString, "hh:mm:ss.zzz" );
                pT->setTime( time );
                qDebug()<<"time="<<pT->mTime.toString("hh:mm:ss.zzz" );
                qDebug()<<"-----------------------------------------------------\n";
                continue;
            }
            else if( name() == "TimerGroup" )
            {
                readTimerGroup( pT );
            }
            else if( name() == "Timer" )
            {
                readTimerGroup( pT );
            }
            else
            {
                qDebug()<<"[INFO] UNKNOWN Timer element: name="<<name().toString()<<"text:"<<text().toString();
                readUnknownTimerElement();
            }
        }
    }
    qDebug()<<"[REGISTER] Timer:"<<pT->getName();
    pT->registerTimer();
    
}

void XMLimport::readAliasPackage()
{
    while( ! atEnd() ) 
    {
        readNext();
        if( isEndElement() )
        {
            break;
        }
        
        if( isStartElement() ) 
        {
            if( name() == "AliasGroup" )
            {
                readAliasGroup( 0 );
            }
            else if( name() == "Alias" )
            {
                readAliasGroup( 0 );
            }
            else
            {
                readUnknownAliasElement();
            }
        }
    }
}

void XMLimport::readAliasGroup( TAlias * pParent )
{
    TAlias * pT;
    if( pParent ) 
    {
        pT = new TAlias( pParent, mpHost );
    } 
    else 
    {
        pT = new TAlias( 0, mpHost );
    }
    
    pT->mIsActive = ( attributes().value("isActive") == "yes" );
    pT->mIsFolder = ( attributes().value("isFolder") == "yes" );
    
    while( ! atEnd() ) 
    {
        readNext();
        if( isEndElement() ) break;
        
        if( isStartElement() ) 
        {
            if( name() == "name" )
            {
                pT->mName = readElementText();
                qDebug()<<"name="<<pT->mName;
                continue;
            }
            else if( name() == "script")
            {
                pT->mScript = readElementText();
                qDebug()<<"script="<<pT->mScript;
                continue;
            }
            else if( name() == "command")
            {
                pT->mCommand = readElementText();
                qDebug()<<"command="<<pT->mCommand;
                continue;
            }
            else if( name() == "regex")
            {
                pT->mRegexCode = readElementText();
                qDebug()<<"regex="<<pT->mRegexCode;
                qDebug()<<"-----------------------------------------------------\n";
                continue;
            }
            else if( name() == "AliasGroup" )
            {
                readAliasGroup( pT );
            }
            else if( name() == "Alias" )
            {
                readAliasGroup( pT );
            }
            else
            {
                qDebug()<<"[INFO] UNKNOWN Alias element: name="<<name().toString()<<"text:"<<text().toString();
                readUnknownAliasElement();
            }
        }
    }
    qDebug()<<"[REGISTER] Alias:"<<pT->getName();
    mpHost->getAliasUnit()->registerAlias( pT );
    
}

void XMLimport::readActionPackage()
{
    while( ! atEnd() ) 
    {
        readNext();
        if( isEndElement() )
        {
            break;
        }
        
        if( isStartElement() ) 
        {
            if( name() == "ActionGroup" )
            {
                readActionGroup( 0 );
            }
            else if( name() == "Action" )
            {
                readActionGroup( 0 );
            }
            else
            {
                readUnknownActionElement();
            }
        }
    }
}

void XMLimport::readActionGroup( TAction * pParent )
{
    TAction * pT;
    if( pParent ) 
    {
        pT = new TAction( pParent, mpHost );
    } 
    else 
    {
        pT = new TAction( 0, mpHost );
    }
    
    pT->mIsActive = ( attributes().value("isActive") == "yes" );
    pT->mIsFolder = ( attributes().value("isFolder") == "yes" );
    pT->mIsPushDownButton = ( attributes().value("isPushButton") == "yes" );
    
    while( ! atEnd() ) 
    {
        readNext();
        if( isEndElement() ) break;
        
        if( isStartElement() ) 
        {
            if( name() == "name" )
            {
                pT->mName = readElementText();
                qDebug()<<"name="<<pT->mName;
                continue;
            }
            else if( name() == "script")
            {
                pT->mScript = readElementText();
                qDebug()<<"script="<<pT->mScript;
                continue;
            }
            else if( name() == "commandButtonUp")
            {
                pT->mCommandButtonUp = readElementText();
                qDebug()<<"mCommandButtonUp="<<pT->mCommandButtonUp;
                continue;
            }
            else if( name() == "commandButtonDown")
            {
                pT->mCommandButtonDown = readElementText();
                qDebug()<<"commandButtonDown="<<pT->mCommandButtonDown;
                continue;
            }
            else if( name() == "icon")
            {
                pT->mIcon = readElementText();
                qDebug()<<"icon="<<pT->mIcon;
                qDebug()<<"-----------------------------------------------------\n";
                continue;
            }
            else if( name() == "ActionGroup" )
            {
                readActionGroup( pT );
            }
            else if( name() == "Action" )
            {
                readActionGroup( pT );
            }
            else
            {
                qDebug()<<"[INFO]: UNKNOWN Action element: name="<<name().toString()<<"text:"<<text().toString();
                readUnknownActionElement();
            }
        }
    }
    qDebug()<<"[REGISTER] Action:"<<pT->getName();
    mpHost->getActionUnit()->registerAction( pT );
    
}

void XMLimport::readScriptPackage()
{
    while( ! atEnd() ) 
    {
        readNext();
        if( isEndElement() )
        {
            break;
        }
        
        if( isStartElement() ) 
        {
            if( name() == "ScriptGroup" )
            {
                readScriptGroup( 0 );
            }
            else if( name() == "Script" )
            {
                readScriptGroup( 0 );
            }
            else
            {
                readUnknownScriptElement();
            }
        }
    }
}

void XMLimport::readScriptGroup( TScript * pParent )
{
    TScript * pT;
    if( pParent ) 
    {
        pT = new TScript( pParent, mpHost );
    } 
    else 
    {
        pT = new TScript( 0, mpHost );
    }
    
    pT->mIsActive = ( attributes().value("isActive") == "yes" );
    pT->mIsFolder = ( attributes().value("isFolder") == "yes" );
    
    while( ! atEnd() ) 
    {
        readNext();
        if( isEndElement() ) break;
        
        if( isStartElement() ) 
        {
            if( name() == "name" )
            {
                pT->mName = readElementText();
                qDebug()<<"name="<<pT->mName;
                continue;
            }
            else if( name() == "script")
            {
                pT->mScript = readElementText();
                qDebug()<<"script="<<pT->mScript;
                continue;
            }
            else if( name() == "eventHandlerList")
            {
                readStringList( pT->mEventHandlerList );
                qDebug()<<"eventHandlerList="<<pT->mEventHandlerList;
                continue;
                qDebug()<<"-----------------------------------------------------\n";
            }
            else if( name() == "ScriptGroup" )
            {
                readScriptGroup( pT );
            }
            else if( name() == "Script" )
            {
                readScriptGroup( pT );
            }
            else
            {
                qDebug()<<"[INFO] UNKNOWN Script element: name="<<name().toString()<<"text:"<<text().toString();
                readUnknownScriptElement();
            }
        }
    }
    qDebug()<<"[REGISTER] Script:"<<pT->getName();
    mpHost->getScriptUnit()->registerScript( pT );
    
}

void XMLimport::readKeyPackage()
{
    while( ! atEnd() ) 
    {
        readNext();
        if( isEndElement() )
        {
            break;
        }
        
        if( isStartElement() ) 
        {
            if( name() == "KeyGroup" )
            {
                readKeyGroup( 0 );
            }
            else if( name() == "Key" )
            {
                readKeyGroup( 0 );
            }
            else
            {
                readUnknownKeyElement();
            }
        }
    }
}

void XMLimport::readKeyGroup( TKey * pParent )
{
    TKey * pT;
    if( pParent ) 
    {
        pT = new TKey( pParent, mpHost );
    } 
    else 
    {
        pT = new TKey( 0, mpHost );
    }
    
    pT->mIsActive = ( attributes().value("isActive") == "yes" );
    pT->mIsFolder = ( attributes().value("isFolder") == "yes" );
    
    while( ! atEnd() ) 
    {
        readNext();
        if( isEndElement() ) break;
        
        if( isStartElement() ) 
        {
            if( name() == "name" )
            {
                pT->mName = readElementText();
                qDebug()<<"name="<<pT->mName;
                continue;
            }
            else if( name() == "script")
            {
                pT->mScript = readElementText();
                qDebug()<<"script="<<pT->mScript;
                continue;
            }
            else if( name() == "command")
            {
                pT->mCommand = readElementText();
                qDebug()<<"command="<<pT->mCommand;
                continue;
            }
            else if( name() == "keyCode" )
            {
                pT->mKeyCode = readElementText().toInt();
                qDebug()<<"keyCode="<<pT->mKeyCode;
                continue;
            }
            else if( name() == "keyModifier" )
            {
                pT->mKeyModifier = readElementText().toInt();
                qDebug()<<"keyModifier="<<pT->mKeyModifier;
                qDebug()<<"-----------------------------------------------------\n";
                continue;
            }

            else if( name() == "KeyGroup" )
            {
                readKeyGroup( pT );
            }
            else if( name() == "Key" )
            {
                readKeyGroup( pT );
            }
            else
            {
                qDebug()<<"[INFO] UNKNOWN Key element: name="<<name().toString()<<"text:"<<text().toString();
                readUnknownKeyElement();
            }
        }
    }
    qDebug()<<"[REGISTER] Key:"<<pT->getName();
    mpHost->getKeyUnit()->registerKey( pT );
    
}


void XMLimport::readStringList( QStringList & list )
{
    while( ! atEnd() ) 
    {
        readNext();
        
        if( isEndElement() ) break;
        
        if( isStartElement() ) 
        {
            if( name() == "string")
            {
                list << readElementText();
            }
            else
            {
                readUnknownTriggerElement();
            }
        }
    }
}

void XMLimport::readIntegerList( QList<int> & list )
{
    while( ! atEnd() ) 
    {
        readNext();
        
        if( isEndElement() ) break;
        
        if( isStartElement() ) 
        {
            if( name() == "integer")
            {
                list << readElementText().toInt();
            }
            else
            {
                readUnknownTriggerElement();
            }
        }
    }
}


