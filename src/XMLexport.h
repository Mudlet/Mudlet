/***************************************************************************
 *   Copyright (C) 2008 by Heiko Koehn  ( KoehnHeiko@googlemail.com )      *
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


#ifndef XML_EXPORT_H
#define XML_EXPORT_H


#include <QXmlStreamWriter>

#include "Host.h"
#include "TVar.h"
#include "VarUnit.h"
#include "LuaInterface.h"


class XMLexport : public QXmlStreamWriter
{
public:
                    XMLexport( Host * );
                    XMLexport( Host *, bool );
                    XMLexport( TTrigger * );
                    XMLexport( TTimer * );
                    XMLexport( TAlias * );
                    XMLexport( TAction * );
                    XMLexport( TScript * );
                    XMLexport( TKey * );

    bool            writeHost( Host * );

    bool            writeTrigger( TTrigger * );
    bool            writeTimer( TTimer * );
    bool            writeAlias( TAlias * );
    bool            writeAction( TAction * );
    bool            writeScript( TScript * );
    bool            writeKey( TKey * );
    bool            writeVariable( TVar *, LuaInterface *, VarUnit * );
    bool            writeModuleXML( QIODevice * device, QString moduleName);
    bool            exportHost( Host * );

    bool            exportHost( QIODevice * );
    bool            exportGenericPackage( QIODevice * device );
    bool            writeGenericPackage( Host * pT );
    bool            exportTrigger( QIODevice * );
    bool            exportTimer( QIODevice * );
    bool            exportAlias( QIODevice * );
    bool            exportAction( QIODevice * );
    bool            exportScript( QIODevice * );
    bool            exportKey( QIODevice * );

    bool            exportTrigger( TTrigger * );
    bool            exportTimer( TTimer * );
    bool            exportAlias( TAlias * );
    bool            exportAction( TAction * );
    bool            exportScript( TScript * );
    bool            exportKey( TKey * );

private:
    Host *          mpHost;
    TTrigger *      mpTrigger;
    TTimer *        mpTimer;
    TAlias *        mpAlias;
    TAction *       mpAction;
    TScript *       mpScript;
    TKey *          mpKey;
    QString         mType;
};

#endif // XML_EXPORT_H
