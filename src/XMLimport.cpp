/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2015 by Stephen Lyons - slysven@virginmedia.com         *
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
#include "mudlet.h"
#include "TArea.h"
#include "TAction.h"
#include "TAlias.h"
#include "TKey.h"
#include "TMap.h"
#include "TRoom.h"
#include "TRoomDB.h"
#include "TScript.h"
#include "TTrigger.h"
#include "TTimer.h"
#include "TVar.h"
#include "VarUnit.h"

#include "pre_guard.h"
#include <QStringList>
#include <QDebug>
#include "post_guard.h"


int maxRooms;
int maxAreas;

XMLimport::XMLimport( Host * pH )
: mpHost( pH )
{
}

void XMLimport::dumpElementAttributes( QString & elementName, QMap<QString, QString> & elementAttributes, const QString & title )
{
    elementName = name().toString();
    elementAttributes.clear();
    foreach( QXmlStreamAttribute elementAttribute, attributes() ) {
        elementAttributes.insert( elementAttribute.name().toString(), elementAttribute.value().toString() );
    }
    QString output;
    if( ! elementAttributes.isEmpty() ) {
        output = QStringLiteral( "XMLimport::%1 INFO: parsing <%2 " ).arg( title ).arg( elementName );
        QMapIterator<QString, QString> itAttribute( elementAttributes );
        while( itAttribute.hasNext() ) {
            itAttribute.next();
            output.append( QStringLiteral( "%1=\"%2\" " ).arg( itAttribute.key() ).arg( itAttribute.value() ) );
        }
        output.chop(1); // lose unwanted last space
        output.append( QStringLiteral( ">" ) );
    }
    else {
        output = QStringLiteral( "XMLimport::%1 INFO: parsing <%2>" ).arg( title ).arg( elementName );
    }
    qDebug() << output.toUtf8().constData();
}

bool XMLimport::importPackage( QIODevice * device, QString packName, int moduleFlag)
{
    mPackageName = packName;
    setDevice( device );

    gotTrigger = false;
    gotTimer = false;
    gotAlias = false;
    gotKey = false;
    gotAction = false;
    gotScript = false;
    module = moduleFlag;

    if( ! packName.isEmpty() ) {
        mpKey = new TKey( 0, mpHost );
        if( module ) {
            mpKey->mModuleMasterFolder=true;
            mpKey->mModuleMember=true;
        }
        mpKey->setPackageName( mPackageName );
        mpKey->setIsActive( true );
        mpKey->setName( mPackageName );
        mpKey->setIsFolder( true );

        mpTrigger = new TTrigger( 0, mpHost );
        if( module ) {
            mpTrigger->mModuleMasterFolder=true;
            mpTrigger->mModuleMember=true;
        }
        mpTrigger->setPackageName( mPackageName );
        mpTrigger->setIsActive( true );
        mpTrigger->setName( mPackageName );
        mpTrigger->setIsFolder( true );

        mpTimer = new TTimer( 0, mpHost );
        if( module ) {
            mpTimer->mModuleMasterFolder=true;
            mpTimer->mModuleMember=true;
        }
        mpTimer->setPackageName( mPackageName );
        mpTimer->setIsActive( true );
        mpTimer->setName( mPackageName );
        mpTimer->setIsFolder( true );

        mpAlias = new TAlias( 0, mpHost );
        if( module ) {
            mpAlias->mModuleMasterFolder=true;
            mpAlias->mModuleMember=true;
        }
        mpAlias->setPackageName( mPackageName );
        mpAlias->setName( mPackageName );
        mpAlias->setIsFolder( true );
        QString _nothing = QString();
        mpAlias->setScript( _nothing );
        mpAlias->setRegexCode( QString() );
        mpAlias->setIsActive( true );

        mpAction = new TAction( 0, mpHost );
        if( module ) {
            mpAction->mModuleMasterFolder=true;
            mpAction->mModuleMember=true;
        }
        mpAction->setPackageName( mPackageName );
        mpAction->setIsActive( true );
        mpAction->setName( mPackageName );
        mpAction->setIsFolder( true );

        mpScript = new TScript( 0, mpHost );
        if( module ) {
            mpScript->mModuleMasterFolder=true;
            mpScript->mModuleMember=true;
        }
        mpScript->setPackageName( mPackageName );
        mpScript->setIsActive( true );
        mpScript->setName( mPackageName );
        mpScript->setIsFolder( true );

        mpHost->getTriggerUnit()->registerTrigger( mpTrigger );
        mpHost->getTimerUnit()->registerTimer( mpTimer );
        mpHost->getAliasUnit()->registerAlias( mpAlias );
        mpHost->getActionUnit()->registerAction( mpAction );
        mpHost->getKeyUnit()->registerKey( mpKey );
        mpHost->getScriptUnit()->registerScript( mpScript );
    }

    QString elementName;
    QMap<QString, QString> elementAttributes;
    if( ! atEnd() ) { // Originally a while() but we only do this once!
        readNext();

        if( isStartDocument() ) {
            qDebug() << "XMLimport::importPackage(...) INFO: Xml encoding:" << documentEncoding() << "Xml version:" << documentVersion();
            readNext(); // Move on to next element
        }

        if( isDTD() ) {
            QString message = QStringLiteral( "XMLimport::importPackage(...) INFO: DTD name: \"%1\" DTD Public Id: \"%2\" DTD System Id: \"%3\"" ).arg( dtdName().toString() ).arg( dtdPublicId().toString() ).arg( dtdSystemId().toString() );
            QXmlStreamEntityDeclarations documentEntityDeclarations = entityDeclarations();
            if( ! documentEntityDeclarations.isEmpty() ) {
                message.append( QStringLiteral( "\n    INFO: external entity declarations:" ) );
                foreach(QXmlStreamEntityDeclaration documentEntityDeclaration, documentEntityDeclarations ) {
                    message.append( QStringLiteral( "\n    \"%1\" ==> \"%2\"" ).arg( documentEntityDeclaration.name().toString() ).arg(  documentEntityDeclaration.value().toString() ) ); // There is more that we might show but pretty academic for us
                }
            }
            qDebug() << message.toUtf8().constData();
            readNext(); // Move on to next element
        }

        if( isStartElement() ) {
            dumpElementAttributes( elementName, elementAttributes, QStringLiteral( "importPackage(...)" ) );
            if( name() == QStringLiteral( "MudletPackage" ) /* && attributes().value( QStringLiteral("version") == QStringLiteral("1.0" */ ) {
                readPackage(); // only returns at end of file (though should also on error!)
            }
            else if( name() == QStringLiteral( "map" ) ) {
                maxAreas = 0;
                maxRooms = 0;
                mpHost->mpMap->mpRoomDB->clearMapDB();
                readMap(); // only returns at end of file (though should also on error!)
                QMapIterator<int, QString> itEnv( mpHost->mpMap->mEnvColorNamesMap );
                while( itEnv.hasNext() ) {
                    itEnv.next();
                    if( itEnv.value().isEmpty() ) {
                        continue;
                    }
                    else {
                        QColor envColor;
                        envColor.setNamedColor( itEnv.value() );
                        if( envColor.isValid() ) {
                            mpHost->mpMap->customEnvColors.insert( itEnv.key(), envColor );
                            mpHost->mpMap->mEnvColorIdMap.insert( itEnv.key(), itEnv.key() ); // Forceable overwrite the 1-15 mapping already read
                        }
                    }
                }
                mpHost->mpMap->init(mpHost);
            }
            else {
                qDebug() << "XMLimport::importPackage(...) ERROR: unrecognised top level element, name:" << name().toString() << "text:" << text().toString();
            }
        }
    }

    if( gotTimer && ! packName.isEmpty() ) {
        mpTimer->setIsActive( true );
        mpTimer->enableTimer( mpTimer->getID() );
    }

    if( gotAlias && ! packName.isEmpty() ) {
        mpAlias->setIsActive( true );
    }

    if( gotAction && ! packName.isEmpty() ) {
        mpHost->getActionUnit()->updateToolbar();
    }

    if( ! packName.isEmpty()) {
       if( ! gotTrigger ) {
            mpHost->getTriggerUnit()->unregisterTrigger( mpTrigger );
       }
       if( ! gotTimer ) {
            mpHost->getTimerUnit()->unregisterTimer( mpTimer );
       }
       if( ! gotAlias ) {
            mpHost->getAliasUnit()->unregisterAlias( mpAlias );
       }
       if( ! gotAction ) {
            mpHost->getActionUnit()->unregisterAction( mpAction );
       }
       if( ! gotKey ) {
            mpHost->getKeyUnit()->unregisterKey( mpKey );
       }
       if( ! gotScript ) {
            mpHost->getScriptUnit()->unregisterScript( mpScript );
       }
    }
    return ! error();
}

void XMLimport::readVariableGroup( TVar * pParent )
{
    TVar * var;
    if( pParent ) {
        var = new TVar( pParent );
    }
    else {
        var = new TVar( );
    }

    LuaInterface * lI = mpHost->getLuaInterface();
    VarUnit * vu = lI->getVarUnit();
    QString keyName, value;
    int keyType = 0;
    int valueType;

    QString elementName;
    QMap<QString, QString> elementAttributes;
    while( ! atEnd() ) {
        readNext();

        if( isCharacters() && isWhitespace() ) {
            readNext(); // Skip the linefeed and whitespace that separates tags
        }

        if( isEndElement() ) {
            break;
        }

        if( isStartElement() ) {
            dumpElementAttributes( elementName, elementAttributes, QStringLiteral( "readVariableGroup(...)" ) );
            if( name() == QStringLiteral( "name" ) ) {
                keyName = readElementText();
                continue;
            }
            else if( name() == QStringLiteral( "value" ) ) {
                value = readElementText();
                continue;
            }
            else if( name() == QStringLiteral( "keyType" ) ) {
                keyType = readElementText().toInt() ;
                continue;
            }
            else if ( name() == QStringLiteral( "valueType" ) ) {
                valueType = readElementText().toInt();
                var->setName( keyName, keyType );
                var->setValue( value, valueType );
                vu->addSavedVar( var );
                lI->setValue( var );
                continue;
            }
            else if( name() == QStringLiteral( "VariableGroup" ) ) {
                readVariableGroup( var );
            }
            else if( name() == QStringLiteral( "Variable" ) ) {
                readVariableGroup( var );
            }
        }
    }
}

void XMLimport::readHiddenVariables()
{
    LuaInterface * lI = mpHost->getLuaInterface();
    VarUnit * vu = lI->getVarUnit();
    QString elementName;
    QMap<QString, QString> elementAttributes;
    while( ! atEnd() ) {
        readNext();

        if( isCharacters() && isWhitespace() ) {
            readNext(); // Skip the linefeed and whitespace that separates tags
        }

        if( isEndElement() ) {
            break;
        }

        if( isStartElement() ) {
            dumpElementAttributes( elementName, elementAttributes, QStringLiteral( "readHiddenVariables(...)" ) );
            if( name() == QStringLiteral( "name" ) ) {
                QString var = readElementText();
                vu->addHidden( var );
                continue;
            }
        }
    }
}

void XMLimport::readVariablePackage()
{
    LuaInterface * lI = mpHost->getLuaInterface();
    VarUnit * vu = lI->getVarUnit();
    mpVar = vu->getBase();
    QString elementName;
    QMap<QString, QString> elementAttributes;
    while( ! atEnd() ) {
        readNext();

        if( isCharacters() && isWhitespace() ) {
            readNext(); // Skip the linefeed and whitespace that separates tags
        }

        if( isEndElement() ) {
            break;
        }

        if( isStartElement() ) {
            dumpElementAttributes( elementName, elementAttributes, QStringLiteral( "readVariablePackage()" ) );
            if(  name() == QStringLiteral( "VariableGroup" )
              || name() == QStringLiteral( "Variable" ) ) {
                readVariableGroup( mpVar );
            }
            else if ( name() == QStringLiteral( "HiddenVariables") ) {
                readHiddenVariables( );
            }
        }
    }
}

void XMLimport::readMap()
{
    QString elementName;
    QMap<QString, QString> elementAttributes;
    while( ! atEnd() ) {
        readNext();

        if( isCharacters() && isWhitespace() ) {
            readNext(); // Skip the linefeed and whitespace that separates tags
        }

        if( isEndElement() ) {
            break;
        }

        if( isStartElement() ) {
            dumpElementAttributes( elementName, elementAttributes, QStringLiteral( "readMap()" ) );
            if( name() == QStringLiteral( "areas" ) ) {
                readAreas(); // Read ALL the areas, should return on the endElement </areas> (or <areas/> ???)
            }
            else if( name() == QStringLiteral( "rooms" ) ) {
                readRooms();
            }
            else if( name() == QStringLiteral( "environments" ) ) {
                readEnvColors();
            }
            else {
                // TODO: Store any other (1st level) XML map data
            }
        }
    }
}

void XMLimport::readEnvColors()
{
    QString elementName;
    QMap<QString, QString> elementAttributes;
    while( ! atEnd() ) {
        readNext();

        if( isCharacters() && isWhitespace() ) {
            readNext(); // Skip the linefeed and whitespace that separates tags
        }

        if( isEndElement() ) {
            break;
        }

        if( isStartElement() ) {
            dumpElementAttributes( elementName, elementAttributes, QStringLiteral( "readEnvColors()" ) );

            if( name() == QStringLiteral( "environment" ) ) {
                readEnvColor();
            }
            else {
                readUnknownElement();
            }
            readNext(); // Move to endElement of this <environment> tag
        }
    }

}

void XMLimport::readEnvColor()
{
    int id;
    int colorId;
    bool isIdFound = false;
    QString name;
    QString colorName;
    QMap<QString, QString> userData;
    QVectorIterator<QXmlStreamAttribute> itEnvAttribute( attributes() );
    while( itEnvAttribute.hasNext() ) {
        QXmlStreamAttribute attribute = itEnvAttribute.next();
        if(      attribute.name() == QStringLiteral("id") ) {
            id = attribute.value().toString().toInt();
            isIdFound = true;
        }
        else if( attribute.name() == QStringLiteral("color") ) {
            colorId = attribute.value().toString().toInt();
        }
        else if( attribute.name() == QStringLiteral("name") ) {
            name = attribute.value().toString();
        }
        else if( attribute.name() == QStringLiteral("htmlcolor") ) {
            colorName = attribute.value().toString();
        }
        else {
            // Let's capture anything else and make it available to user
            userData.insert( attribute.name().toString(),
                             attribute.value().toString() );
        }
    }

    mpHost->mpMap->mEnvColorIdMap[id] = colorId;
    if( ! name.isNull() ) {
        mpHost->mpMap->mEnvNamesMap[id] = name;
    }
    if( ! colorName.isNull() ) {
        mpHost->mpMap->mEnvColorNamesMap[id] = colorName;
    }
    if( ! userData.isEmpty() ) {
        QMapIterator<QString, QString> itDataItem( userData );
        while( itDataItem.hasNext() ) {
            itDataItem.next();
            mpHost->mpMap->mUserData.insert( QStringLiteral( "xmlParse.environment.%1.%2" )
                                             .arg( id )
                                             .arg( itDataItem.key() ),
                                             itDataItem.value() );
        }
    }
}

// Should be on startElement <areas> tag
void XMLimport::readAreas()
{
    QString elementName;
    QMap<QString, QString> elementAttributes;
    while( ! atEnd() ) {
        readNext();

        if( isCharacters() && isWhitespace() ) {
            readNext(); // Skip the linefeed and whitespace that separates tags
        }

        if( isEndElement() ) {
            break; // Should be at endElement, </areas>
        }

        if( isStartElement() ) {
            dumpElementAttributes( elementName, elementAttributes, QStringLiteral( "readAreas(...)" ) );
            if( name() == QStringLiteral( "area" ) ) {
                readArea();
            }
            else {
                readUnknownElement();
            }
            readNext(); // Move to endElement of THIS startElement
        }
    }
}

void XMLimport::readArea()
{
    QMap<QString, QString> userData;
    int id = -1;
    QString name;
    QVectorIterator<QXmlStreamAttribute> itAreaAttribute( attributes() );
    while( itAreaAttribute.hasNext() ) {
        QXmlStreamAttribute attribute = itAreaAttribute.next();
        if(      attribute.name() == QStringLiteral("id") ) {
            id = attribute.value().toString().toInt();
        }
        else if( attribute.name() == QStringLiteral("name") ) {
            name = attribute.value().toString().toInt();
        }
        else {
            // Let's capture anything else and make it available to user
            userData.insert( QStringLiteral( "xmlParse.%1" ).arg( attribute.name().toString() ),
                             attribute.value().toString() );
        }
    }

    if( id >= 0 ) {
        // Allow 0 but it might be problematic...!
        if( mpHost->mpMap->mpRoomDB->addArea( id, name ) ) {
            // If name is empty the area will be given a default one which will be uniquified if necessary with a number suffix
            if( ! userData.isEmpty() ) {
                TArea * pA = mpHost->mpMap->mpRoomDB->getArea( id );
                pA->mUserData = userData;
            }
        }
        else {
            QString errMsg = tr( "[ ERROR ] - While parsing the XML data was unable to add an area with id:%1 called:\n"
                                             "\"%2\" to the map - data has been lost!" )
                             .arg( id )
                             .arg( name );
            mpHost->postMessage( errMsg );
        }
    }
    else {
        QString errMsg = tr( "[ ERROR ] - While parsing the XML data was unable to add an area with a negative id:%1 called:\n"
                                         "\"%2\" to the map - data has been lost!" )
                         .arg( id )
                         .arg( name );
        mpHost->postMessage( errMsg );
    }
}

void XMLimport::readRooms()
{
//    QString elementName;
//    QMap<QString, QString> elementAttributes;
    while( ! atEnd() ) {
        readNext();

        if( isCharacters() && isWhitespace() ) {
            readNext(); // Skip the linefeed and whitespace that separates tags
        }

        if( isEndElement() ) {
            break;
        }

        if( isStartElement() ) {
            // dumpElementAttributes( elementName, elementAttributes, QStringLiteral( "readRooms()" ) );

            if( name() == QStringLiteral( "room" ) ) {
                readRoom();
            }
            else {
                readUnknownMapElement();
            }
        }
    }
    // Need to update the TArea instances, to do the deferred actions from the
    // per room calls to TRoomDB::addRoom( pT->id, pT, true ) in readRoom()
    QMapIterator<int, TArea *> itArea( mpHost->mpMap->mpRoomDB->getAreaMap() );
    while( itArea.hasNext() ) {
        itArea.next();
        if( itArea.value() ) {
            itArea.value()->calcSpan();
            itArea.value()->determineAreaExits();
            itArea.value()->mIsDirty = false;
        }
    }
}


void XMLimport::readRoom()
{
    TRoom * pT = new TRoom( mpHost->mpMap->mpRoomDB );

    // Refactored code to use iterators so we can process ANY attribute found
    QVectorIterator<QXmlStreamAttribute> itRoomAttribute( attributes() );
    while( itRoomAttribute.hasNext() ) {
        QXmlStreamAttribute attribute = itRoomAttribute.next();
        if(      attribute.name() == QStringLiteral("id") ) {
            pT->id = attribute.value().toString().toInt();
        }
        else if( attribute.name() == QStringLiteral("area") ) {
            pT->area = attribute.value().toString().toInt();
        }
        else if( attribute.name() == QStringLiteral("title") ) {
            pT->name = attribute.value().toString();
        }
        else if( attribute.name() == QStringLiteral("environment") ) {
            pT->environment = attribute.value().toString().toInt();
        }
        else if( attribute.name() == QStringLiteral("locked") ) {
            if( attribute.value().toString() == QStringLiteral( "true" )
             || attribute.value().toString() == QStringLiteral( "1" )
             || attribute.value().toString() == QStringLiteral( "yes" ) ) {
                pT->isLocked = true;
            }
        }
        else if( attribute.name() == QStringLiteral("cost") ) {
            bool isWeightOk = false;
            int weight = attribute.value().toString().toInt( &isWeightOk );
            if(  isWeightOk && weight > 0 ) {
                pT->weight = weight;
            }
        }
        else {
            // Though we don't {currently} have an explict room description field
            // other clients can - and for some MUDs having this data is vital for
            // location detection (when the MUD doesn't tell us a room Vnumb by an
            // out-of-band channel).  It is thus something we should store if found...

            // In fact - let's capture anything else and make it available to user
            pT->userData.insert( QStringLiteral( "xmlParse.%1" ).arg( attribute.name().toString() ),
                                                 attribute.value().toString() );
        }
    }

    while( ! atEnd() ) {
        readNext();

        if( isCharacters() && isWhitespace() ) {
            readNext(); // Skip the linefeed and whitespace that separates tags
        }

        if( isEndElement() ) {
            break;
        }

        if( isStartElement() ) {
            if( name() == QStringLiteral("coord") ) {
                QVectorIterator<QXmlStreamAttribute> itCoordAttribute( attributes() );
                while( itCoordAttribute.hasNext() ) {
                    QXmlStreamAttribute attribute = itCoordAttribute.next();
                    if(      attribute.name() == QStringLiteral("x") ) {
                        pT->x = attribute.value().toString().toInt();
                    }
                    else if( attribute.name() == QStringLiteral("y") ) {
                        pT->y = attribute.value().toString().toInt();
                    }
                    else if( attribute.name() == QStringLiteral("z") ) {
                        pT->z = attribute.value().toString().toInt();
                    }
                    else {
                        // Let's capture anything else and make it available to user
                        pT->userData.insert( QStringLiteral( "xmlParse.coord.%1" ).arg( attribute.name().toString() ),
                                                             attribute.value().toString() );
                    }
                }
            }
            else if( name() == QStringLiteral("exit") ) {
                QVectorIterator<QXmlStreamAttribute> itExitAttribute( attributes() );
                QString dir;
                int target;
                int _door = 0; // raw door value
                bool isDoorOk = false;
                int _hidden = 0; // raw hidden value - we don't support hidden exits
                bool isHiddenOk = false;
                int _weight = 0;
                bool isWeightOk = false;
                int door = 0; // cooked door value - comes from _door but can be overridden by _hidden
                bool isLocked = false;
                QMap<QString, QString> exitOtherDataItems;
                while( itExitAttribute.hasNext() ) {
                    QXmlStreamAttribute attribute = itExitAttribute.next();
                    if(      attribute.name() == QStringLiteral("direction") ) {
                        dir = attribute.value().toString();
                    }
                    else if( attribute.name() == QStringLiteral("target") ) {
                        target = attribute.value().toString().toInt();
                    }
                    else if( attribute.name() == QStringLiteral("door") ) {
                        exitOtherDataItems.insert( QStringLiteral("door"), attribute.value().toString() );
                        _door = attribute.value().toString().toInt( &isDoorOk );
                    }
                    else if( attribute.name() == QStringLiteral("hidden") ) {
                        exitOtherDataItems.insert( QStringLiteral("hidden"), attribute.value().toString() );
                        _hidden = attribute.value().toString().toInt(&isHiddenOk);
                    }
                    else if( attribute.name() == QStringLiteral("locked") ) {
                        exitOtherDataItems.insert( QStringLiteral("locked"), attribute.value().toString() );
                        if( attribute.value().toString() == QStringLiteral( "true" )
                         || attribute.value().toString() == QStringLiteral( "1" )
                         || attribute.value().toString() == QStringLiteral( "yes" ) ) {
                            isLocked = true;
                        }
                    }
                    else if( attribute.name() == QStringLiteral("cost") ) {
                        _weight = attribute.value().toString().toInt( &isWeightOk );
                    }
                    else {
                        // Let's capture anything else and make it available to user
                        exitOtherDataItems.insert( attribute.name().toString(), attribute.value().toString() );
                    }
                } // end of while() - finished parsing all the attributes of this exit

                // The following was suggested by browsing an Achea map, which had
                // door = "1" or hidden = "1" on some exits, allow for 2 or 3 also
                // for doors, and treat "hidden" (though hidden can appear without door)
                // as door = 3... - Slysven
                if( isDoorOk && _door > 0 && _door <= 3 ) {
                    door = _door;
                }
                if( isHiddenOk ) {
                    door = ( _hidden != 0 ) ? 3 : 0;  // So make it a "locked" (red) door (and override the door value!)
                }
                if( ( !isWeightOk ) || _weight < 1 ) {
                    _weight = 0;
                }

                QString dirDoorAndWeightString;
                if( dir.isEmpty() ) {
                    qWarning() << "XMLimport::readRoom() WARN: <exit> element with an empty exit id string found, it has been discarded!";
                }
                else if( dir == QStringLiteral("north") ) {
                    dirDoorAndWeightString = QStringLiteral("n");
                    pT->north = target;
                    pT->setExitLock( DIR_NORTH, isLocked );
                }
                else if( dir == QStringLiteral("south") ) {
                    dirDoorAndWeightString = QStringLiteral("s");
                    pT->south = target;
                    pT->setExitLock( DIR_SOUTH, isLocked );
                }
                else if( dir == QStringLiteral("northwest") ) {
                    dirDoorAndWeightString = QStringLiteral("nw");
                    pT->northwest = target;
                    pT->setExitLock( DIR_NORTHWEST, isLocked );
                }
                else if( dir == QStringLiteral("southwest") ) {
                    dirDoorAndWeightString = QStringLiteral("sw");
                    pT->southwest = target;
                    pT->setExitLock( DIR_SOUTHWEST, isLocked );
                }
                else if( dir == QStringLiteral("northeast") ) {
                    dirDoorAndWeightString = QStringLiteral("nw");
                    pT->northeast = target;
                    pT->setExitLock( DIR_NORTHEAST, isLocked );
                }
                else if( dir == QStringLiteral("southeast") ) {
                    dirDoorAndWeightString = QStringLiteral("se");
                    pT->southeast = target;
                    pT->setExitLock( DIR_SOUTHEAST, isLocked );
                }
                else if( dir == QStringLiteral("west") ) {
                    dirDoorAndWeightString = QStringLiteral("w");
                    pT->west = target;
                    pT->setExitLock( DIR_WEST, isLocked );
                }
                else if( dir == QStringLiteral("east") ) {
                    dirDoorAndWeightString = QStringLiteral("e");
                    pT->east = target;
                    pT->setExitLock( DIR_EAST, isLocked );
                }
                else if( dir == QStringLiteral("up") ) {
                    dirDoorAndWeightString = QStringLiteral("up");
                    pT->up = target;
                    pT->setExitLock( DIR_UP, isLocked );
                }
                else if( dir == QStringLiteral("down") ) {
                    dirDoorAndWeightString = QStringLiteral("down");
                    pT->down = target;
                    pT->setExitLock( DIR_DOWN, isLocked );
                }
                else if( dir == QStringLiteral("in") ) {
                    dirDoorAndWeightString = QStringLiteral("in");
                    pT->in = target;
                    pT->setExitLock( DIR_IN, isLocked );
                }
                else if( dir == QStringLiteral("out") ) {
                    dirDoorAndWeightString = QStringLiteral("out");
                    pT->out = target;
                    pT->setExitLock( DIR_OUT, isLocked );
                }
                else {
                    dirDoorAndWeightString = dir;
                    pT->other.insert( target, QStringLiteral( "%1%2" )
                                         .arg( isLocked ? "1" : "0" )
                                         .arg( dir ) );
                }
                pT->setDoor( dirDoorAndWeightString, door );
                pT->setExitWeight( dirDoorAndWeightString, _weight );

                if( ! exitOtherDataItems.isEmpty() ) {
                    QMapIterator<QString, QString> itExitOtherDataItem( exitOtherDataItems );
                    while( itExitOtherDataItem.hasNext() ) {
                        itExitOtherDataItem.next();
                        pT->userData.insert( QStringLiteral( "xmlParse.exit.%1.%2" )
                                                 .arg( dir )
                                                 .arg( itExitOtherDataItem.key() ),
                                             itExitOtherDataItem.value() );
                    }
                }
            }
            else {
                readUnknownElement();
            }
        readNext(); // Pull to the endElement of this
        }
    }

    if( pT->id > 0 ) { // CHECKME: Changed from just a test for non-zero
        mpHost->mpMap->mpRoomDB->addRoom( pT->id, pT, true ); // Last flag is because we are loading a map and can skip some things
        mpHost->mpMap->setRoomArea( pT->id, pT->area, true );
        maxRooms++;
    }
    else {
        qDebug() << "XMLimport::readRoom() INFO: Discarding room with room Id less than 1!";
        delete pT;
    }
}

void XMLimport::readUnknownMapElement()
{
    QString elementName;
    QMap<QString, QString> elementAttributes;
    while( ! atEnd() ) {
        readNext();

        if( isCharacters() && isWhitespace() ) {
            readNext(); // Skip the linefeed and whitespace that separates tags
        }

        if( isEndElement() ) {
            break;
        }

        if( isStartElement() ) {
            dumpElementAttributes( elementName, elementAttributes, QStringLiteral("readUnknownMapElement()") );
            readMap();
        }
    }
}

// Will be at startElement of "<MudletPackage>" on entry
void XMLimport::readPackage()
{
    QString elementName;
    QMap<QString, QString> elementAttributes;
    while( ! atEnd() && error() == QXmlStreamReader::NoError ) { // Should really do something about QXmlStreamReader::PrematureEndOfDocumentError
        readNext();

        if( isCharacters() && isWhitespace() ) {
            readNext(); // Skip the linefeed and whitespace that separates tags
        }

        if( isEndElement() ) {
            break;
        }

        if( isStartElement() ) {
            dumpElementAttributes( elementName, elementAttributes, QStringLiteral( "readPackage()" ) );
            if( name() == QStringLiteral( "HostPackage" ) ) {
                readHostPackage();
            }
            else if( name() == QStringLiteral( "TriggerPackage" ) ) {
                readTriggerPackage();
            }
            else if( name() == QStringLiteral( "TimerPackage" ) ) {
                readTimerPackage();
            }
            else if( name() == QStringLiteral( "AliasPackage" ) ) {
                readAliasPackage();
            }
            else if( name() == QStringLiteral( "ActionPackage" ) ) {
                readActionPackage();
            }
            else if( name() == QStringLiteral( "ScriptPackage" ) ) {
                readScriptPackage();
            }
            else if( name() == QStringLiteral( "KeyPackage" ) ) {
                readKeyPackage();
            }
            else if( name() == QStringLiteral( "HelpPackage") ) {
                readHelpPackage();
            }
            else if( name() == QStringLiteral( "VariablePackage" ) ) {
                readVariablePackage();
            }
            else {
                readUnknownElement();
            }
        }
    }
}

void XMLimport::readHelpPackage()
{
    QString elementName;
    QMap<QString, QString> elementAttributes;
    while( ! atEnd() ) {
        readNext();

        if( isCharacters() && isWhitespace() ) {
            readNext(); // Skip the linefeed and whitespace that separates tags
        }

        if( isEndElement() ) {
            break;
        }

        if( isStartElement() ) {
            dumpElementAttributes( elementName, elementAttributes, QStringLiteral( "readHelpPackage()" ) );
            if( name() == QStringLiteral( "helpURL" ) ) {
                QString contents = readElementText();
                mpHost->moduleHelp[mPackageName].insert( QStringLiteral( "helpURL" ), contents );
                continue;
            }
        }
    }
}


// Changed to use readElement() which parses ALL the contents under the current
// element, and as it is generic can be used for all unknown elements:
void XMLimport::readUnknownElement()
{
    if( ! isStartElement() ) {
        qWarning() << "XMLimport::readUnknownHostElement() ERROR: Tried to use when NOT at a startElement.";
    }
    else {
        QString contents = readElementText( QXmlStreamReader::IncludeChildElements );
        // This will get everything until the corresponding closing tag...
        qDebug() << "    WARNING: This element was unexpected, its {text} contents are: " << contents;
    }
}

void XMLimport::readTriggerPackage()
{
    QString elementName;
    QMap<QString, QString> elementAttributes;
    while( ! atEnd() ) {
        readNext();

        if( isCharacters() && isWhitespace() ) {
            readNext(); // Skip the linefeed and whitespace that separates tags
        }

        if( isEndElement() ) {
            break;
        }

        if( isStartElement() ) {
            dumpElementAttributes( elementName, elementAttributes, QStringLiteral( "readTriggerPackage()" ) );

            if( name() == QStringLiteral( "TriggerGroup" ) ) {
                gotTrigger = true;
                readTriggerGroup( mPackageName.isEmpty() ? 0 : mpTrigger );
            }
            else {
                readUnknownElement();
            }
        }
    }
}

void XMLimport::readHostPackage()
{
    QString elementName;
    QMap<QString, QString> elementAttributes;
    while( ! atEnd() ) {
        readNext();

        if( isCharacters() && isWhitespace() ) {
            readNext(); // Skip the linefeed and whitespace that separates tags
        }

        if( isEndElement() ) {
            break;
        }

        if( isStartElement() ) {
            dumpElementAttributes( elementName, elementAttributes, QStringLiteral( "readHostPackage()" ) );
            if( name() == QStringLiteral( "Host" ) ) {
                readHostPackage( mpHost );
            }
            else {
                readUnknownElement();
            }
        }
    }
}

// This actually parses the "<Host ....>" element under the <HostPackage> one,
// Will be at the <Host ...> startElement on entry
void XMLimport::readHostPackage( Host * pT )
{
    pT->mAutoClearCommandLineAfterSend = ( attributes().value("autoClearCommandLineAfterSend") == "yes" );
    pT->mDisableAutoCompletion = ( attributes().value("disableAutoCompletion") == "yes" );
    pT->mPrintCommand = ( attributes().value("printCommand") == "yes" );
    pT->set_USE_IRE_DRIVER_BUGFIX( attributes().value("USE_IRE_DRIVER_BUGFIX") == "yes" );
    pT->mUSE_FORCE_LF_AFTER_PROMPT = ( attributes().value("mUSE_FORCE_LF_AFTER_PROMPT") == "yes" );
    pT->mUSE_UNIX_EOL = ( attributes().value("mUSE_UNIX_EOL") == "yes" );
    pT->mNoAntiAlias = ( attributes().value("mNoAntiAlias") == "yes" );
    pT->mIsNextLogFileInHtmlFormat = ( attributes().value("mRawStreamDump") == "yes" );
    pT->mAlertOnNewData = ( attributes().value("mAlertOnNewData") == "yes" );
    pT->mFORCE_NO_COMPRESSION = ( attributes().value("mFORCE_NO_COMPRESSION") == "yes" );
    pT->mFORCE_GA_OFF = ( attributes().value("mFORCE_GA_OFF") == "yes" );
    pT->mFORCE_SAVE_ON_EXIT = ( attributes().value("mFORCE_SAVE_ON_EXIT") == "yes" );
    pT->mEnableGMCP = ( attributes().value("mEnableGMCP") == "yes" );
    pT->mEnableMSDP = ( attributes().value("mEnableMSDP") == "yes" );
    pT->mMapStrongHighlight = ( attributes().value("mMapStrongHighlight") == "yes" );
    pT->mLogStatus = ( attributes().value("mLogStatus") == "yes" );
    pT->mEnableSpellCheck = ( attributes().value("mEnableSpellCheck") == "yes" );
    pT->mShowInfo = ( attributes().value("mShowInfo") == "yes" );
    pT->mAcceptServerGUI = ( attributes().value("mAcceptServerGUI") == "yes" );
    pT->mMapperUseAntiAlias = ( attributes().value("mMapperUseAntiAlias") == "yes" );
    pT->mFORCE_MXP_NEGOTIATION_OFF = ( attributes().value("mFORCE_MXP_NEGOTIATION_OFF") == "yes" );
    // Round these to one decimal place (were being treated as integers!)
    pT->mRoomSize = qRound( attributes().value("mRoomSize").toString().toFloat() * 10.0 ) / 10.0;
    pT->mLineSize = qRound( attributes().value("mLineSize").toString().toFloat() * 10.0 ) / 10.0;
    pT->mBubbleMode = ( attributes().value("mBubbleMode") == "yes" );
    pT->mShowRoomID = ( attributes().value("mShowRoomIDs") == "yes" );
    pT->mShowPanel = ( attributes().value("mShowPanel") == "yes" );
    pT->mHaveMapperScript = ( attributes().value("mHaveMapperScript") == "yes");
    QString ignore = attributes().value("mDoubleClickIgnore").toString();

    if( !pT->mRoomSize ) {
        pT->mRoomSize=3;
    }
    if( !pT->mLineSize ) {
        pT->mLineSize=1;
    }
    for( int i=0; i < ignore.size(); i++ ) {
        pT->mDoubleClickIgnore.insert( ignore.at( i ) );
    }

    QString elementName;
    QMap<QString, QString> elementAttributes;
    while( ! atEnd() ) {
        readNext();

        if( isCharacters() && isWhitespace() ) {
            readNext(); // Skip the linefeed and whitespace that separates tags
        }

        if( isEndElement() ) {
            break;
        }

        if( isStartElement() ) {
            dumpElementAttributes( elementName, elementAttributes, QStringLiteral( "readHostPackage(...)" ) );
            if( name() == QStringLiteral( "name" ) ) {
                pT->mHostName = readElementText(); // Actually a bit pointless as it is already set...!
            }
            else if( name() == QStringLiteral( "mInstalledModules") ) {
                QMap<QString, QStringList> entry;
                readMapList(entry);

                QMapIterator<QString, QStringList> it(entry);
                while( it.hasNext() ) {
                    it.next();
                    QStringList moduleList;
                    QStringList entryList = it.value();
                    moduleList << entryList.at(0);
                    moduleList << entryList.at(1);
                    pT->mInstalledModules[it.key()] = moduleList;
                    pT->mModulePriorities[it.key()] = entryList.at(2).toInt();
                }
            }
            else if( name() == QStringLiteral( "mInstalledPackages") ) {
                readStringList( pT->mInstalledPackages );
            }
            else if( name() == QStringLiteral( "url" ) ) {
                pT->mUrl = readElementText();
            }
            else if( name() == QStringLiteral( "serverPackageName" ) ) {
                pT->mServerGUI_Package_name = readElementText();
            }
            else if( name() == QStringLiteral( "serverPackageVersion" ) ) {
                pT->mServerGUI_Package_version = readElementText().toInt();
            }
            else if( name() == QStringLiteral( "port" ) ) {
                pT->mPort = readElementText().toInt();
            }
            else if( name() == QStringLiteral( "borderTopHeight" ) ) {
                pT->mBorderTopHeight = readElementText().toInt();
            }
            else if( name() == QStringLiteral( "commandLineMinimumHeight" ) ) {
                pT->commandLineMinimumHeight = readElementText().toInt();
            }
            else if( name() == QStringLiteral( "borderBottomHeight" ) ) {
                pT->mBorderBottomHeight = readElementText().toInt();
            }
            else if( name() == QStringLiteral( "borderLeftWidth" ) ) {
                pT->mBorderLeftWidth = readElementText().toInt();
            }
            else if( name() == QStringLiteral( "borderRightWidth" ) ) {
                pT->mBorderRightWidth = readElementText().toInt();
            }
            else if( name() == QStringLiteral( "wrapAt" ) ) {
                pT->mWrapAt = readElementText().toInt();
            }
            else if( name() == QStringLiteral( "wrapIndentCount" ) ) {
                pT->mWrapIndentCount = readElementText().toInt();
            }
            else if( name() == QStringLiteral( "mCommandSeparator" ) ) {
                pT->mCommandSeparator = readElementText();
            }
            else if( name() == QStringLiteral( "mCommandLineFgColor" ) ) {
                pT->mCommandLineFgColor.setNamedColor( readElementText() );
            }
            else if( name() == QStringLiteral( "mCommandLineBgColor" ) ) {
                pT->mCommandLineBgColor.setNamedColor( readElementText() );
            }
            else if( name() == QStringLiteral( "mFgColor" ) ) {
                pT->mFgColor.setNamedColor( readElementText() );
            }
            else if( name() == QStringLiteral( "mBgColor" ) ) {
                pT->mBgColor.setNamedColor( readElementText() );
            }
            else if( name() == QStringLiteral( "mCommandFgColor" ) ) {
                pT->mCommandFgColor.setNamedColor( readElementText() );
            }
            else if( name() == QStringLiteral( "mCommandBgColor" ) ) {
                pT->mCommandBgColor.setNamedColor( readElementText() );
            }
            else if( name() == QStringLiteral( "mBlack" ) ) {
                pT->mBlack.setNamedColor( readElementText() );
            }
            else if( name() == QStringLiteral( "mLightBlack" ) ) {
                pT->mLightBlack.setNamedColor( readElementText() );
            }
            else if( name() == QStringLiteral( "mRed" ) ) {
                pT->mRed.setNamedColor( readElementText() );
            }
            else if( name() == QStringLiteral( "mLightRed" ) ) {
                pT->mLightRed.setNamedColor( readElementText() );
            }
            else if( name() == QStringLiteral( "mBlue" ) ) {
                pT->mBlue.setNamedColor( readElementText() );
            }
            else if( name() == QStringLiteral( "mLightBlue" ) ) {
                pT->mLightBlue.setNamedColor( readElementText() );
            }
            else if( name() == QStringLiteral( "mGreen" ) ) {
                pT->mGreen.setNamedColor( readElementText() );
            }
            else if( name() == QStringLiteral( "mLightGreen" ) ) {
                pT->mLightGreen.setNamedColor( readElementText() );
            }
            else if( name() == QStringLiteral( "mYellow" ) ) {
                pT->mYellow.setNamedColor( readElementText() );
            }
            else if( name() == QStringLiteral( "mLightYellow" ) ) {
                pT->mLightYellow.setNamedColor( readElementText() );
            }
            else if( name() == QStringLiteral( "mCyan" ) ) {
                pT->mCyan.setNamedColor( readElementText() );
            }
            else if( name() == QStringLiteral( "mLightCyan" ) ) {
                pT->mLightCyan.setNamedColor( readElementText() );
            }
            else if( name() == QStringLiteral( "mMagenta" ) ) {
                pT->mMagenta.setNamedColor( readElementText() );
            }
            else if( name() == QStringLiteral( "mLightMagenta" ) ) {
                pT->mLightMagenta.setNamedColor( readElementText() );
            }
            else if( name() == QStringLiteral( "mWhite" ) ) {
                pT->mWhite.setNamedColor( readElementText() );
            }
            else if( name() == QStringLiteral( "mLightWhite" ) ) {
                pT->mLightWhite.setNamedColor( readElementText() );
            }
            else if( name() == QStringLiteral( "mDisplayFont" ) ) {
                pT->mDisplayFont.fromString( readElementText() );
                pT->mDisplayFont.setFixedPitch( true );
            }
            else if( name() == QStringLiteral( "mCommandLineFont" ) ) {
                pT->mCommandLineFont.fromString( readElementText() );
            }
            else if( name() == QStringLiteral( "mFgColor2" ) ) {
                pT->mFgColor_2.setNamedColor( readElementText() );
            }
            else if( name() == QStringLiteral( "mBgColor2" ) ) {
                pT->mBgColor_2.setNamedColor( readElementText() );
            }
            else if( name() == QStringLiteral( "mBlack2" ) ) {
                pT->mBlack_2.setNamedColor( readElementText() );
            }
            else if( name() == QStringLiteral( "mLightBlack2" ) ) {
                pT->mLightBlack_2.setNamedColor( readElementText() );
            }
            else if( name() == QStringLiteral( "mRed2" ) ) {
                pT->mRed_2.setNamedColor( readElementText() );
            }
            else if( name() == QStringLiteral( "mLightRed2" ) ) {
                pT->mLightRed_2.setNamedColor( readElementText() );
            }
            else if( name() == QStringLiteral( "mBlue2" ) ) {
                pT->mBlue_2.setNamedColor( readElementText() );
            }
            else if( name() == QStringLiteral( "mLightBlue2" ) ) {
                pT->mLightBlue_2.setNamedColor( readElementText() );
            }
            else if( name() == QStringLiteral( "mGreen2" ) ) {
                pT->mGreen_2.setNamedColor( readElementText() );
            }
            else if( name() == QStringLiteral( "mLightGreen2" ) ) {
                pT->mLightGreen_2.setNamedColor( readElementText() );
            }
            else if( name() == QStringLiteral( "mYellow2" ) ) {
                pT->mYellow_2.setNamedColor( readElementText() );
            }
            else if( name() == QStringLiteral( "mLightYellow2" ) ) {
                pT->mLightYellow_2.setNamedColor( readElementText() );
            }
            else if( name() == QStringLiteral( "mCyan2" ) ) {
                pT->mCyan_2.setNamedColor( readElementText() );
            }
            else if( name() == QStringLiteral( "mLightCyan2" ) ) {
                pT->mLightCyan_2.setNamedColor( readElementText() );
            }
            else if( name() == QStringLiteral( "mMagenta2" ) ) {
                pT->mMagenta_2.setNamedColor( readElementText() );
            }
            else if( name() == QStringLiteral( "mLightMagenta2" ) ) {
                pT->mLightMagenta_2.setNamedColor( readElementText() );
            }
            else if( name() == QStringLiteral( "mWhite2" ) ) {
                pT->mWhite_2.setNamedColor( readElementText() );
            }
            else if( name() == QStringLiteral( "mLightWhite2" ) ) {
                pT->mLightWhite_2.setNamedColor( readElementText() );
            }
            else if( name() == QStringLiteral( "mSpellDic" ) ) {
                pT->mSpellDic = readElementText();
            }
// Removed previous long term error, this data already included as attributes to parent "<Host>"...
//            else if( name() == QStringLiteral( "mRoomSize" ) ) {
//                pT->mRoomSize = readElementText().toDouble();
//            }
//            else if( name() == QStringLiteral( "mLineSize" ) ) {
//                pT->mLineSize = readElementText().toDouble();
//            }
            else {
                readUnknownElement();
            }
        }
    }
}


void XMLimport::readTriggerGroup( TTrigger * pParent )
{
    TTrigger * pT = new TTrigger( pParent, mpHost );
    pT->mModuleMember = (module != 0);

    mpHost->getTriggerUnit()->registerTrigger( pT );

    pT->setIsActive( (attributes().value( QStringLiteral("isActive") ) == QStringLiteral("yes") ) );
    pT->mIsFolder = ( attributes().value( QStringLiteral("isFolder") ) == QStringLiteral("yes") );
    pT->mIsTempTrigger = ( attributes().value( QStringLiteral("isTempTrigger") ) == QStringLiteral("yes") );
    pT->mIsMultiline = ( attributes().value( QStringLiteral("isMultiline") ) == QStringLiteral("yes") );
    pT->mPerlSlashGOption = ( attributes().value( QStringLiteral("isPerlSlashGOption") ) == QStringLiteral("yes") );
    pT->mIsColorizerTrigger = ( attributes().value( QStringLiteral("isColorizerTrigger") ) == QStringLiteral("yes") );
    pT->mFilterTrigger = ( attributes().value( QStringLiteral("isFilterTrigger") ) == QStringLiteral("yes") );
    pT->mSoundTrigger = ( attributes().value( QStringLiteral("isSoundTrigger") ) == QStringLiteral("yes") );
    pT->mColorTrigger = ( attributes().value( QStringLiteral("isColorTrigger") ) == QStringLiteral("yes") );
    pT->mColorTriggerBg = ( attributes().value( QStringLiteral("isColorTriggerBg") ) == QStringLiteral("yes") );
    pT->mColorTriggerFg = ( attributes().value( QStringLiteral("isColorTriggerFg") ) == QStringLiteral("yes") );

    QString elementName;
    QMap<QString, QString> elementAttributes;
    while( ! atEnd() ) {
        readNext();

        if( isCharacters() && isWhitespace() ) {
            readNext(); // Skip the linefeed and whitespace that separates tags
        }

        if( isEndElement() ) {
            break;
        }

        if( isStartElement() ) {
            dumpElementAttributes( elementName, elementAttributes, QStringLiteral( "readTriggerGroup(...)" ) );
            if( name() == QStringLiteral( "name" ) ) {
                pT->setName( readElementText() ); // This will read everything in this element, and leave at the endElement...!
            }
            else if( name() == QStringLiteral( "script" ) ) {
                pT->mScript = readElementText();
            }
            else if( name() == QStringLiteral( "packageName" ) ) {
                pT->mPackageName = readElementText();
            }
            else if( name() == QStringLiteral( "triggerType" ) ) {
                pT->mTriggerType = readElementText().toInt();
            }
            else if( name() == QStringLiteral( "conditonLineDelta" ) ) {
                pT->mConditionLineDelta = readElementText().toInt();
            }
            else if( name() == QStringLiteral( "mStayOpen" ) ) {
                pT->mStayOpen = readElementText().toInt();
            }
            else if( name() == QStringLiteral( "mCommand" ) ) {
                pT->mCommand = readElementText();
            }
            else if( name() == QStringLiteral( "mFgColor" ) ) {
                pT->mFgColor.setNamedColor( readElementText() );
            }
            else if( name() == QStringLiteral( "mBgColor" ) ) {
                pT->mBgColor.setNamedColor( readElementText() );
            }
            else if( name() == QStringLiteral( "colorTriggerFgColor" ) ) {
                pT->mColorTriggerFgColor.setNamedColor( readElementText() );
            }
            else if( name() == QStringLiteral( "colorTriggerBgColor" ) ) {
                pT->mColorTriggerBgColor.setNamedColor( readElementText() );
            }
            else if( name() == QStringLiteral( "mSoundFile" ) ) {
                pT->mSoundFile = readElementText();
            }
            else if( name() == QStringLiteral( "regexCodeList" ) ) {
                readStringList( pT->mRegexCodeList );
            }
            else if( name() == QStringLiteral( "regexCodePropertyList" ) ) {
                readIntegerList( pT->mRegexCodePropertyList );
                // Do this now, to avoid confusing things with children's data,
                // It does assume that this element comes AFTER <regexCodeList>
                // CHECKME: Would make the setRegexCodeList(...) conditional on non-empty arguments' list's sizes but not sure if that is OK
                if( ! pT->setRegexCodeList( pT->mRegexCodeList, pT->mRegexCodePropertyList ) ) {
                    qWarning() << "XMLimport::readTriggerGroup(...) ERROR: whilst importing, can not initialize pattern list for a trigger, name:" << pT->getName();
                }
            }
            else if(  name() == QStringLiteral( "TriggerGroup" )
                   || name() == QStringLiteral( "Trigger" ) ) {
                readTriggerGroup( pT );
            }
            else {
                readUnknownElement();
            }
        }
    }

    QString script = pT->mScript;
    if( ! pT->setScript( script ) ) {
        qWarning() << "XMLimport::readTriggerGroup(...) ERROR: whilst importing, a trigger script does not compile, name:" << pT->getName();
    }
}

void XMLimport::readTimerPackage()
{
    QString elementName;
    QMap<QString, QString> elementAttributes;
    while( ! atEnd() ) {
        readNext();

        if( isCharacters() && isWhitespace() ) {
            readNext(); // Skip the linefeed and whitespace that separates tags
        }

        if( isEndElement() ) {
            break;
        }

        if( isStartElement() ) {
            dumpElementAttributes( elementName, elementAttributes, QStringLiteral( "readTimerPackage(...)" ) );
            if(  name() == QStringLiteral( "TimerGroup" )
              || name() == QStringLiteral( "Timer" ) ) {
                gotTimer = true;
                readTimerGroup( mPackageName.isEmpty() ? 0 : mpTimer);
            }
            else {
                readUnknownElement();
            }
        }
    }
}

void XMLimport::readTimerGroup( TTimer * pParent )
{
    TTimer * pT = new TTimer( pParent, mpHost );

    pT->mIsFolder = ( attributes().value( QStringLiteral("isFolder") ) == QStringLiteral("yes") );
    pT->mIsTempTimer = ( attributes().value( QStringLiteral("isTempTimer") ) == QStringLiteral("yes") );

    mpHost->getTimerUnit()->registerTimer( pT );
    pT->setShouldBeActive( attributes().value( QStringLiteral("isActive") ) == QStringLiteral("yes") );

// N/U:     bool isOffsetTimer = ( attributes().value( QStringLiteral("isOffsetTimer") ) == QStringLiteral("yes") );

    pT->mModuleMember = (module != 0);

    QString elementName;
    QMap<QString, QString> elementAttributes;
    while( ! atEnd() ) {
        readNext();

        if( isCharacters() && isWhitespace() ) {
            readNext(); // Skip the linefeed and whitespace that separates tags
        }

        if( isEndElement() ) {
            break;
        }

        if( isStartElement() ) {
            dumpElementAttributes( elementName, elementAttributes, QStringLiteral( "readTimerGroup(...)" ) );
            if( name() == QStringLiteral( "name" ) ) {
                pT->setName( readElementText() );
            }
            else if( name() == QStringLiteral( "packageName" ) ) {
                pT->mPackageName = readElementText();
            }
            else if( name() == QStringLiteral( "script" ) ) {
                QString script = readElementText();
                pT->setScript( script );
            }
            else if( name() == QStringLiteral( "command" ) ) {
                pT->mCommand = readElementText();
            }
            else if( name() == QStringLiteral( "time" ) ) {
                QString timeString = readElementText();
                QTime time = QTime::fromString( timeString, QStringLiteral("hh:mm:ss.zzz") );
                pT->setTime( time );
            }
            else if(  name() == QStringLiteral( "TimerGroup" )
                   || name() == QStringLiteral( "Timer" ) ) {
                readTimerGroup( pT );
            }
            else {
                readUnknownElement();
            }
        }
    }

    mudlet::self()->registerTimer( pT, pT->mpTimer );

    if( ! pT->mpParent && pT->shouldBeActive() ) {
        pT->setIsActive( true );
        pT->enableTimer( pT->getID() );
    }
}

void XMLimport::readAliasPackage()
{
    QString elementName;
    QMap<QString, QString> elementAttributes;
    while( ! atEnd() ) {
        readNext();

        if( isCharacters() && isWhitespace() ) {
            readNext(); // Skip the linefeed and whitespace that separates tags
        }

        if( isEndElement() ) {
            break;
        }

        if( isStartElement() ) {
            dumpElementAttributes( elementName, elementAttributes, QStringLiteral( "readAliasPackage()" ) );
            if(  name() == QStringLiteral( "AliasGroup" )
              || name() == QStringLiteral( "Alias" ) ) {
                gotAlias = true;
                readAliasGroup( mPackageName.isEmpty() ? 0 : mpAlias );
            }
            else {
                readUnknownElement();
            }
        }
    }
}

void XMLimport::readAliasGroup( TAlias * pParent )
{
    TAlias * pT = new TAlias( pParent, mpHost );

    mpHost->getAliasUnit()->registerAlias( pT );
    pT->setIsActive( attributes().value( QStringLiteral("isActive") ) == QStringLiteral("yes" ) );
    pT->mIsFolder = ( attributes().value( QStringLiteral("isFolder") ) == QStringLiteral("yes" ) );
    pT->mModuleMember = (module != 0);

    QString elementName;
    QMap<QString, QString> elementAttributes;
    while( ! atEnd() ) {
        readNext();

        if( isCharacters() && isWhitespace() ) {
            readNext(); // Skip the linefeed and whitespace that separates tags
        }

        if( isEndElement() ) {
            break;
        }

        if( isStartElement() ) {
            dumpElementAttributes( elementName, elementAttributes, QStringLiteral( "readAliasGroup(...)" ) );
            if( name() == QStringLiteral( "name" ) ) {
                pT->setName( readElementText() );
            }
            else if( name() == QStringLiteral( "packageName" ) ) {
                pT->mPackageName = readElementText();
            }
            else if( name() == QStringLiteral( "script" ) ) {
                QString script = readElementText();
                pT->setScript( script );
            }
            else if( name() == QStringLiteral( "command" ) ) {
                pT->mCommand = readElementText();
            }
            else if( name() == QStringLiteral( "regex") ) {
                pT->setRegexCode( readElementText() );
            }
            else if(  name() == QStringLiteral( "Alias" )
                   || name() == QStringLiteral( "AliasGroup" ) ) {
                readAliasGroup( pT );
            }
            else {
                readUnknownElement();
            }
        }
    }
}

void XMLimport::readActionPackage()
{
    QString elementName;
    QMap<QString, QString> elementAttributes;
    while( ! atEnd() ) {
        readNext();

        if( isCharacters() && isWhitespace() ) {
            readNext(); // Skip the linefeed and whitespace that separates tags
        }

        if( isEndElement() ) {
            break;
        }

        if( isStartElement() ) {
            dumpElementAttributes( elementName, elementAttributes, QStringLiteral( "readActionPackage()" ) );
            if(  name() == QStringLiteral( "Action" )
              || name() == QStringLiteral( "ActionGroup" ) ) {
                gotAction = true;
                readActionGroup( mPackageName.isEmpty() ? 0 : mpAction );
            }
            else {
                readUnknownElement();
            }
        }
    }
}

void XMLimport::readActionGroup( TAction * pParent )
{
    TAction * pT = new TAction( pParent, mpHost );

    mpHost->getActionUnit()->registerAction( pT );
    pT->setIsActive( attributes().value( QStringLiteral("isActive") ) == QStringLiteral("yes") );
    pT->mIsFolder = ( attributes().value( QStringLiteral("isFolder") ) == QStringLiteral("yes") );
    pT->mIsPushDownButton = ( attributes().value( QStringLiteral("isPushButton") ) == QStringLiteral("yes") );
    pT->mButtonFlat = ( attributes().value( QStringLiteral("isFlatButton") ) == QStringLiteral("yes") );
    pT->mUseCustomLayout = ( attributes().value( QStringLiteral("useCustomLayout") ) == QStringLiteral("yes") );
    pT->mModuleMember = (module != 0);

    QString elementName;
    QMap<QString, QString> elementAttributes;
    while( ! atEnd() ) {
        readNext();

        if( isCharacters() && isWhitespace() ) {
            readNext(); // Skip the linefeed and whitespace that separates tags
        }

        if( isEndElement() ) {
            break;
        }

        if( isStartElement() ) {
            dumpElementAttributes( elementName, elementAttributes, QStringLiteral( "readActionGroup(...)" ) );
            if( name() == QStringLiteral( "name" ) ) {
                pT->mName = readElementText();
            }
            else if( name() == QStringLiteral( "packageName" ) ) {
                pT->mPackageName = readElementText();
            }
            else if( name() == QStringLiteral( "script" ) ) {
                QString script = readElementText();
                pT->setScript( script );
            }
            else if( name() == QStringLiteral( "css" ) ) {
                pT->css = readElementText();
            }
            else if( name() == QStringLiteral( "commandButtonUp" ) ) {
                pT->mCommandButtonUp = readElementText();
            }
            else if( name() == QStringLiteral( "commandButtonDown" ) ) {
                pT->mCommandButtonDown = readElementText();
            }
            else if( name() == QStringLiteral( "icon" ) ) {
                pT->mIcon = readElementText();
            }
            else if( name() == QStringLiteral( "orientation" ) ) {
                pT->mOrientation = readElementText().toInt();
            }
            else if( name() == QStringLiteral( "location" ) ) {
                pT->mLocation = readElementText().toInt();
            }
            else if( name() == QStringLiteral( "buttonRotation" ) ) {
                pT->mButtonRotation = readElementText().toInt();
            }
            else if( name() == QStringLiteral( "sizeX" ) ) {
                pT->mSizeX = readElementText().toInt();
            }
            else if( name() == QStringLiteral( "sizeY" ) ) {
                pT->mSizeY = readElementText().toInt();
            }
            else if( name() == QStringLiteral( "mButtonState" ) ) {
                pT->mButtonState = readElementText().toInt();
            }
            else if( name() == QStringLiteral( "buttonColor" ) ) {
                pT->mButtonColor.setNamedColor( readElementText() );
            }
            else if( name() == QStringLiteral( "buttonColumn" ) ) {
                pT->mButtonColumns = readElementText().toInt();
            }
            else if( name() == QStringLiteral( "posX" ) ) {
                pT->mPosX = readElementText().toInt();
            }
            else if( name() == QStringLiteral( "posY" ) ) {
                pT->mPosY = readElementText().toInt();
            }
            else if(  name() == QStringLiteral( "Action" )
                   || name() == QStringLiteral( "ActionGroup" ) ) {
                readActionGroup( pT );
            }
            else {
                readUnknownElement();
            }
        }
    }
}

void XMLimport::readScriptPackage()
{
    QString elementName;
    QMap<QString, QString> elementAttributes;
    while( ! atEnd() ) {
        readNext();

        if( isCharacters() && isWhitespace() ) {
            readNext(); // Skip the linefeed and whitespace that separates tags
        }

        if( isEndElement() ) {
            break;
        }

        if( isStartElement() ) {
            dumpElementAttributes( elementName, elementAttributes, QStringLiteral( "readScriptPackage()" ) );
            if(  name() == QStringLiteral( "ScriptGroup" )
              || name() == QStringLiteral( "Script" ) ) {
                gotScript = true;
                readScriptGroup( mPackageName.isEmpty() ? 0 : mpScript );
            }
            else {
                readUnknownElement();
            }
        }
    }
}

void XMLimport::readScriptGroup( TScript * pParent )
{
    TScript * pT = new TScript( pParent, mpHost );
    mpHost->getScriptUnit()->registerScript( pT );
    pT->setIsActive( attributes().value( QStringLiteral("isActive") ) == QStringLiteral("yes") );
    pT->mIsFolder = ( attributes().value( QStringLiteral("isFolder") ) == QStringLiteral("yes") );
    pT->mModuleMember = (module != 0);

    QString elementName;
    QMap<QString, QString> elementAttributes;
    while( ! atEnd() ) {
        readNext();

        if( isCharacters() && isWhitespace() ) {
            readNext(); // Skip the linefeed and whitespace that separates tags
        }

        if( isEndElement() ) {
            break;
        }

        if( isStartElement() ) {
            dumpElementAttributes( elementName, elementAttributes, QStringLiteral( "readScriptGroup(...)" ) );
            if( name() == QStringLiteral( "name" ) ) {
                pT->mName = readElementText();
            }
            else if( name() == QStringLiteral( "packageName" ) ) {
                pT->mPackageName = readElementText();
            }
            else if( name() == QStringLiteral( "script" ) ) {
                QString script = readElementText();
                pT->setScript( script );
            }
            else if( name() == QStringLiteral( "eventHandlerList" ) ) {
                readStringList( pT->mEventHandlerList );
                pT->setEventHandlerList( pT->mEventHandlerList );
            }
            else if(  name() == QStringLiteral( "Script" )
                   || name() == QStringLiteral( "ScriptGroup" ) ) {
                readScriptGroup( pT );
            }
            else {
                readUnknownElement();
            }
        }
    }
}

void XMLimport::readKeyPackage()
{
    QString elementName;
    QMap<QString, QString> elementAttributes;
    while( ! atEnd() ) {
        readNext();

        if( isCharacters() && isWhitespace() ) {
            readNext(); // Skip the linefeed and whitespace that separates tags
        }

        if( isEndElement() ) {
            break;
        }

        if( isStartElement() ) {
            dumpElementAttributes( elementName, elementAttributes, QStringLiteral( "readKeyPackage()" ) );
            if(  name() == QStringLiteral( "Key" )
              || name() == QStringLiteral( "KeyGroup" ) ) {
                gotKey = true;
                readKeyGroup( mPackageName.isEmpty() ? 0 : mpKey );
            }
            else {
                readUnknownElement();
            }
        }
    }
}

void XMLimport::readKeyGroup( TKey * pParent )
{
    TKey * pT = new TKey( pParent, mpHost );
    mpHost->getKeyUnit()->registerKey( pT );
    pT->setIsActive( attributes().value( QStringLiteral("isActive") ) == QStringLiteral("yes") );
    pT->mIsFolder = ( attributes().value( QStringLiteral("isFolder") ) == QStringLiteral("yes") );
    pT->mModuleMember = (module !=0);

    QString elementName;
    QMap<QString, QString> elementAttributes;
    while( ! atEnd() ) {
        readNext();

        if( isCharacters() && isWhitespace() ) {
            readNext(); // Skip the linefeed and whitespace that separates tags
        }

        if( isEndElement() ) {
            break;
        }

        if( isStartElement() ) {
            dumpElementAttributes( elementName, elementAttributes, QStringLiteral( "readKeyGroup(...)" ) );
            if( name() == QStringLiteral( "name" ) ) {
                pT->mName = readElementText();
            }
            else if( name() == QStringLiteral( "packageName" ) ) {
                pT->mPackageName = readElementText();
            }
            else if( name() == QStringLiteral( "script" ) ) {
                QString script = readElementText();
                pT->setScript( script );
            }
            else if( name() == QStringLiteral( "command") ) {
                pT->mCommand = readElementText();
            }
            else if( name() == QStringLiteral( "keyCode" ) ) {
                pT->mKeyCode = readElementText().toInt();
            }
            else if( name() == QStringLiteral( "keyModifier" ) ) {
                pT->mKeyModifier = readElementText().toInt();
            }
            else if(  name() == QStringLiteral( "Key" )
                   || name() == QStringLiteral( "KeyGroup" ) ) {
                readKeyGroup( pT );
            }
            else {
                readUnknownElement();
            }
        }
    }
}

void XMLimport::readMapList( QMap<QString, QStringList> & map )
{
    QString key;
    QStringList entry;
    while( ! atEnd() ) {
        readNext();

        if( isCharacters() && isWhitespace() ) {
            readNext(); // Skip the linefeed and whitespace that separates tags
        }

        if( isEndElement() ) {
            break;
        }

        if( isStartElement() ) {
            if( name() == QStringLiteral( "key" ) ) {
                key = readElementText();
            }
            else if (name() == QStringLiteral( "filepath" ) ) {
                entry << readElementText();
            }
            else if (name() == QStringLiteral( "globalSave" ) ) {
                entry << readElementText();
            }
            else if (name() == QStringLiteral( "priority" ) ) {
                entry << readElementText();
                map[key] = entry;
                entry.clear();
            }
            else {
                readUnknownElement();
            }
        }
    }
}

void XMLimport::readStringList( QStringList & list )
{
    while( ! atEnd() ) {
        readNext();

        if( isCharacters() && isWhitespace() ) {
            readNext(); // Skip the linefeed and whitespace that separates tags
        }

        if( isEndElement() ) {
            break;
        }

        if( isStartElement() ) {
            if( name() == QStringLiteral( "string" ) ) {
                list << readElementText(); // Have found that there can be duplicates, but it is not helpful to add them...!
            }
            else {
                readUnknownElement();
            }
        }
    }
}

void XMLimport::readIntegerList( QList<int> & list )
{
    while( ! atEnd() ) {
        readNext();

        if( isCharacters() && isWhitespace() ) {
            readNext(); // Skip the linefeed and whitespace that separates tags
        }

        if( isEndElement() ) {
            break;
        }

        if( isStartElement() ) {
            if( name() == QStringLiteral( "integer" ) ) {
                bool ok = false;
                QString _text = readElementText();
                int num = _text.toInt( &ok, 10 );
                if( ok ) {
                    list << num;
                }
                else {
                    qWarning() << "XMLimport::readIntegerList(...) ERROR: whilst reading package property list, contained invalid non-integer number element:" << _text;
                }
            }
            else {
                readUnknownElement();
            }
        }
    }
}
