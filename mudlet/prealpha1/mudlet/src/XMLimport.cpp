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
        if( isEndElement() )
        {
            break;
        }
        
        if( isStartElement() ) 
        {
            if( name() == "TriggerPackage" )
            {
                readTriggerPackage();
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

void XMLimport::readUnknownTriggerElement()
{
    while( ! atEnd() ) 
    {
        
        readNext();
        qDebug()<<"[ERROR]: UNKNOWN triggerGroupElement:name="<<name().toString()<<"text:"<<text().toString();
        
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
            if( name() == "Group" )
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
            else if( name() == "Group" )
            {
                readTriggerGroup( pT );
            }
            else if( name() == "Trigger" )
            {
                readTriggerGroup( pT );
            }
            else
            {
                qDebug()<<"readTriggerGroup() UNKNOWN: name="<<name().toString()<<"text:"<<text().toString();
                readUnknownTriggerElement();
            }
        }
    }
    qDebug()<<"[REGISTER]:"<<pT->getName();
    mpHost->getTriggerUnit()->registerTrigger( pT );
    
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



