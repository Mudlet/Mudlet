/***************************************************************************
 *   Copyright (C) 2008-2009 by Heiko Koehn  ( KoehnHeiko@googlemail.com )      *
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
    pT->set_USE_IRE_DRIVER_BUGFIX( attributes().value("USE_IRE_DRIVER_BUGFIX") == "yes" );
    pT->mUSE_FORCE_LF_AFTER_PROMPT = ( attributes().value("mUSE_FORCE_LF_AFTER_PROMPT") == "yes" );
    pT->mUSE_UNIX_EOL = ( attributes().value("mUSE_UNIX_EOL") == "yes" );
    pT->mNoAntiAlias = ( attributes().value("mNoAntiAlias") == "yes" );
    pT->mRawStreamDump = ( attributes().value("mRawStreamDump") == "yes" );

    while( ! atEnd() ) 
    {
        readNext();
        if( isEndElement() ) break;

        if( isStartElement() ) 
        {
            if( name() == "name" )
            {
                pT->mHostName = readElementText();
                continue;
            }
            else if( name() =="url" )
            {
                pT->mUrl = readElementText();
                continue;
            }       

            else if( name() == "port")
            {
                pT->mPort = readElementText().toInt();
                continue;
            }
            else if( name() == "borderTopHeight")
            {
                pT->mBorderTopHeight = readElementText().toInt();
                continue;
            }
            else if( name() == "borderBottomHeight")
            {
                pT->mBorderBottomHeight = readElementText().toInt();
                continue;
            }
            else if( name() == "borderLeftWidth")
            {
                pT->mBorderLeftWidth = readElementText().toInt();
                continue;
            }
            else if( name() == "borderRightWidth")
            {
                pT->mBorderRightWidth = readElementText().toInt();
                continue;
            }
            else if( name() == "wrapAt")
            {
                pT->mWrapAt = readElementText().toInt();
                continue;
            }
            else if( name() == "wrapIndentCount" )
            {
                pT->mWrapIndentCount = readElementText().toInt();
                continue;
            }
            else if( name() == "mCommandSeperator" )
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

    mpHost->getTriggerUnit()->registerTrigger( pT );

    pT->setIsActive( (attributes().value("isActive") == "yes" ) );
    pT->mIsFolder = ( attributes().value("isFolder") == "yes" );
    pT->mIsTempTrigger = ( attributes().value("isTempTrigger") == "yes" );
    pT->mIsMultiline = ( attributes().value("isMultiline") == "yes" );
    pT->mPerlSlashGOption = ( attributes().value("isPerlSlashGOption") == "yes" );
    pT->mIsColorizerTrigger = ( attributes().value("isColorizerTrigger") == "yes" );
    pT->mFilterTrigger = ( attributes().value("isFilterTrigger") == "yes" );
    pT->mSoundTrigger = ( attributes().value("isSoundTrigger") == "yes" );
    pT->mColorTrigger = ( attributes().value("isColorTrigger") == "yes" );
    pT->mColorTriggerBg = ( attributes().value("isColorTriggerBg") == "yes" );
    pT->mColorTriggerFg = ( attributes().value("isColorTriggerFg") == "yes" );


    while( ! atEnd() ) 
    {
        readNext();
        //qDebug()<<"[INFO] element:"<<name().toString()<<" text:"<<text().toString();        
        if( isEndElement() ) break;

        if( isStartElement() ) 
        {
            if( name() == "name" )
            {
                pT->setName( readElementText() );
                continue;
            }
            else if( name() == "script")
            {
                pT->mScript = readElementText();
                continue;
            }
            else if( name() == "triggerType" )
            {
                pT->mTriggerType = readElementText().toInt();
                continue;
            }
            else if( name() == "conditonLineDelta" )
            {
                pT->mConditionLineDelta = readElementText().toInt();
                continue;
            }
            else if( name() == "mStayOpen" )
            {
                pT->mStayOpen = readElementText().toInt();
                continue;
            }
            else if( name() == "mCommand" )
            {
                pT->mCommand = readElementText();
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
            else if( name() == "colorTriggerFgColor")
            {
                pT->mColorTriggerFgColor.setNamedColor( readElementText() );
                continue;
            }
            else if( name() == "colorTriggerBgColor")
            {
                pT->mColorTriggerBgColor.setNamedColor( readElementText() );
                continue;
            }
            else if( name() == "mSoundFile" )
            {
                pT->mSoundFile = readElementText();
                continue;
            }
            else if( name() == "regexCodeList")
            {
                readStringList( pT->mRegexCodeList );
                continue;
            }
            else if( name() == "regexCodePropertyList" )
            {
                readIntegerList( pT->mRegexCodePropertyList );
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
                readUnknownTriggerElement();
            }
        }
    }

    if( ! pT->setRegexCodeList( pT->mRegexCodeList, pT->mRegexCodePropertyList ) )
    {
        qDebug()<<"IMPORT: ERROR: cant initialize pattern list for trigger "<<pT->getName();
    }
    QString script = pT->mScript;
    if( ! pT->setScript( script ) )
    {
        qDebug()<<"IMPORT: ERROR: trigger script "<< pT->getName()<<" does not compile";
    }
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
    pT->registerTimer();
    pT->setShouldBeActive( ( attributes().value("isActive") == "yes" ) );
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
                pT->setName( readElementText() );
                continue;
            }
            else if( name() == "script")
            {
                QString script = readElementText();
                pT->setScript( script );
                continue;
            }
            else if( name() == "command")
            {
                pT->mCommand = readElementText();
                continue;
            }
            else if( name() == "time")
            {
                QString timeString = readElementText();
                QTime time = QTime::fromString( timeString, "hh:mm:ss.zzz" );
                pT->setTime( time );
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
                readUnknownTimerElement();
            }
        }
    }

    if( ( ! pT->isOffsetTimer() ) && ( pT->shouldBeActive() ) )
        pT->enableTimer( pT->getID() );
    else
        pT->disableTimer( pT->getID() );

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

    mpHost->getAliasUnit()->registerAlias( pT );
    pT->setIsActive( ( attributes().value("isActive") == "yes" ) );
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
                continue;
            }
            else if( name() == "script")
            {
                QString script = readElementText();
                pT->setScript( script );
                continue;
            }
            else if( name() == "command")
            {
                pT->mCommand = readElementText();
                continue;
            }
            else if( name() == "regex")
            {
                pT->setRegexCode( readElementText() );
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
                readUnknownAliasElement();
            }
        }
    }


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
    mpHost->getActionUnit()->registerAction( pT );
    pT->setIsActive( ( attributes().value("isActive") == "yes" ) );
    pT->mIsFolder = ( attributes().value("isFolder") == "yes" );
    pT->mIsPushDownButton = ( attributes().value("isPushButton") == "yes" );
    pT->mButtonFlat = ( attributes().value("isFlatButton") == "yes" );
    pT->mUseCustomLayout = ( attributes().value("useCustomLayout") == "yes" );

    while( ! atEnd() ) 
    {
        readNext();
        if( isEndElement() ) break;

        if( isStartElement() ) 
        {
            if( name() == "name" )
            {
                pT->mName = readElementText();
                continue;
            }
            else if( name() == "script")
            {
                QString script = readElementText();
                pT->setScript( script );
                continue;
            }
            else if( name() == "css")
            {
                pT->css = readElementText();
                continue;
            }
            else if( name() == "commandButtonUp")
            {
                pT->mCommandButtonUp = readElementText();
                continue;
            }
            else if( name() == "commandButtonDown")
            {
                pT->mCommandButtonDown = readElementText();
                continue;
            }
            else if( name() == "icon")
            {
                pT->mIcon = readElementText();
                continue;
            }
            else if( name() == "orientation")
            {
                pT->mOrientation = readElementText().toInt();
                continue;
            }
            else if( name() == "location")
            {
                pT->mLocation = readElementText().toInt();
                continue;
            }
            
            else if( name() == "buttonRotation")
            {
                pT->mButtonRotation = readElementText().toInt();
                continue;
            }
            else if( name() == "sizeX")
            {
                pT->mSizeX = readElementText().toInt();
                continue;
            }
            else if( name() == "sizeY")
            {
                pT->mSizeY = readElementText().toInt();
                continue;
            }
            else if( name() == "buttonColor")
            {
                pT->mButtonColor.setNamedColor( readElementText() );
                continue;
            } 
            else if( name() == "buttonColumn")
            {
                pT->mButtonColumns = readElementText().toInt();
                continue;
            }
            
            else if( name() == "posX")
            {
                pT->mPosX = readElementText().toInt();
                continue;
            }
            else if( name() == "posY")
            {
                pT->mPosY = readElementText().toInt();
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
                readUnknownActionElement();
            }
        }
    }


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
    mpHost->getScriptUnit()->registerScript( pT );
    pT->setIsActive( ( attributes().value("isActive") == "yes" ) );
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
                continue;
            }
            else if( name() == "script")
            {
                QString script = readElementText();
                pT->setScript( script );
                continue;
            }
            else if( name() == "eventHandlerList")
            {
                readStringList( pT->mEventHandlerList );
                pT->setEventHandlerList( pT->mEventHandlerList );
                continue;
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
                readUnknownScriptElement();
            }
        }
    }
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
    mpHost->getKeyUnit()->registerKey( pT );
    pT->setIsActive( ( attributes().value("isActive") == "yes" ) );
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
                continue;
            }
            else if( name() == "script")
            {
                QString script = readElementText();
                pT->setScript( script );
                continue;
            }
            else if( name() == "command")
            {
                pT->mCommand = readElementText();
                continue;
            }
            else if( name() == "keyCode" )
            {
                pT->mKeyCode = readElementText().toInt();
                continue;
            }
            else if( name() == "keyModifier" )
            {
                pT->mKeyModifier = readElementText().toInt();
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
                readUnknownKeyElement();
            }
        }
    }
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
                bool ok = false;
                int num;
                num = readElementText().toInt( &ok, 10 );
                if( ok )
                {
                    list << num;
                }
                else
                {
                    qFatal("FATAL ERROR: reading package property list contained invalid elements");
                }
            }
            else
            {
                readUnknownTriggerElement();
            }
        }
    }
}

