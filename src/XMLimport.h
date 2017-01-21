#ifndef MUDLET_XMLIMPORT_H
#define MUDLET_XMLIMPORT_H

/***************************************************************************
 *   Copyright (C) 2008-2012 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2016 by Stephen Lyons - slysven@virginmedia.com         *
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


#include "pre_guard.h"
#include <QApplication>
#include <QMap>
#include <QMultiHash>
#include <QXmlStreamReader>
#include "post_guard.h"

class Host;
class TAction;
class TAlias;
class TKey;
class TScript;
class TTimer;
class TTrigger;
class TVar;


class XMLimport : public QXmlStreamReader
{
    Q_DECLARE_TR_FUNCTIONS(XMLimport) // Needed so we can use tr() even though XMLimport is NOT derived from QObject

public:
              XMLimport( Host * );

    bool      importPackage( QIODevice *device, QString packageName = QString(), int moduleFlag = 0 );

private:

    void      readPackage();
    void      readUnknownPackage();

    void      readHostPackage();
    void      readTriggerPackage();
    void      readTimerPackage();
    void      readAliasPackage();
    void      readActionPackage();
    void      readScriptPackage();
    void      readKeyPackage();
    void      readVariablePackage();
    void      readUnknownMapElement();
    void      readMap();
    void      readRoom( QMultiHash<int, int> &, unsigned int * );
    void      readRooms( QMultiHash<int, int> & );
    void      readEnvColor();
    void      readEnvColors();
    void      readArea();
    void      readAreas();
    void      readHelpPackage();

    void      readUnknownHostElement();


    void      readUnknownTriggerElement();
    void      readUnknownTimerElement();
    void      readUnknownAliasElement();
    void      readUnknownActionElement();
    void      readUnknownScriptElement();
    void      readUnknownKeyElement();

    void      readHostPackage( Host * );
    void      readTriggerGroup( TTrigger * pParent );
    void      readTimerGroup( TTimer * pParent );
    void      readAliasGroup( TAlias * pParent );
    void      readActionGroup( TAction * pParent );
    void      readScriptGroup( TScript * pParent );
    void      readKeyGroup( TKey * pParent );
    void      readVariableGroup( TVar * pParent );
    void      readHiddenVariables();


    void      readStringList( QStringList & );
    void      readIntegerList( QList<int> &, const QString &  );
    void      readModulesDetailsMap( QMap<QString, QStringList> & );

    Host *    mpHost;
    QString   mPackageName;

    TTrigger * mpTrigger;
    TTimer * mpTimer;
    TAlias * mpAlias;
    TKey *   mpKey;
    TAction * mpAction;
    TScript * mpScript;
    TVar *    mpVar;

    bool gotTrigger;
    bool gotTimer;
    bool gotAlias;
    bool gotKey;
    bool gotAction;
    bool gotScript;
    int  module;
    int         mMaxRoomId;
    int         mMaxAreaId; // Could be useful when iterating through map data
};

#endif // MUDLET_XMLEXPORT_H
