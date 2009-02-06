
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

#include "XMLexport.h"

XMLexport::XMLexport( Host * pH )
: mpHost( pH )
, mType( "Host" )
{
    setAutoFormatting(true);
}

XMLexport::XMLexport( TTrigger * pT )
: mpTrigger( pT )
, mType( "Trigger" )
{
    setAutoFormatting(true);
}

bool XMLexport::exportTrigger( QIODevice * device )
{
    setDevice(device);
    
    writeStartDocument();
    writeDTD("<!DOCTYPE MudletPackage>");
    
    writeStartElement( "MudletPackage" );
    writeAttribute("version", "1.0");
    
    writeStartElement( "TriggerPackage" );
    writeTrigger( mpTrigger );
    writeEndElement();//TriggerPackage
    
    writeEndElement();//MudletPackage
    writeEndDocument();
    return true;
}

bool XMLexport::writeTrigger( TTrigger * pT )
{
    QString tag;
    if( pT->mIsFolder )
    {
        tag = "Group";
    }
    else
    {
        tag = "Trigger";
    }
    
    writeStartElement( tag );
    
    
    writeAttribute( "isActive", pT->mIsActive ? "yes" : "no" );
    writeAttribute( "isFolder", pT->mIsFolder ? "yes" : "no" );
    writeAttribute( "isTempTrigger", pT->mIsTempTrigger ? "yes" : "no" ); 
    writeAttribute( "isMultiline", pT->mIsMultiline ? "yes" : "no" );
    
    writeTextElement( "name", pT->mName );
    writeTextElement( "script", pT->mScript );
    writeTextElement( "triggerType", QString::number( pT->mTriggerType ) );
    writeTextElement( "conditonLineDelta", QString::number( pT->mConditionLineDelta ) );
    writeStartElement( "regexCodeList" );
    for( int i=0; i<pT->mRegexCodeList.size(); i++ )
    {
        writeTextElement( "string", pT->mRegexCodeList[i] );
    }
    writeEndElement();
    
    writeStartElement( "regexCodePropertyList" );
    for( int i=0; i<pT->mRegexCodePropertyList.size(); i++ )
    {
        writeTextElement( "integer", QString::number( pT->mRegexCodePropertyList[i] ) );
    }
    writeEndElement();
        
    typedef list<TTrigger *>::const_iterator I;
    for( I it = pT->mpMyChildrenList->begin(); it != pT->mpMyChildrenList->end(); it++)
    {
        TTrigger * pChild = *it;
        writeTrigger( pChild );
    }
    writeEndElement();
}


