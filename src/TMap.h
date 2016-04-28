#ifndef MUDLET_TMAP_H
#define MUDLET_TMAP_H

/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2014-2015 by Stephen Lyons - slysven@virginmedia.com    *
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


#include "TAstar.h"

#include "pre_guard.h"
#include <QApplication>
#include <QColor>
#include <QMap>
#include <QPixmap>
#include <QPointer>
#include <QSizeF>
#include <QVector3D>
#include "post_guard.h"

#include <stdlib.h>

class dlgMapper;
class Host;
class GLWidget;
class TArea;
class TRoom;
class TRoomDB;
class T2DMap;


class TMapLabel
{
public:
    TMapLabel(){ hilite=false; showOnTop=false; noScaling=false; }

    QVector3D pos;
    QPointF pointer;
    QSizeF size;
    QSizeF clickSize;
    QString text;
    QColor fgColor;
    QColor bgColor;
    QPixmap pix;
    bool hilite;
    bool showOnTop;
    bool noScaling;
};


class TMap
{
    Q_DECLARE_TR_FUNCTIONS(TMap) // Needed so we can use tr() even though TMap is NOT derived from QObject

public:
    TMap( Host *);
    ~TMap();
    void mapClear();
    int createMapLabelID( int area );
    int createMapImageLabel(int area, QString filePath, float x, float y, float z, float width, float height, float zoom, bool showOnTop, bool noScaling );
    int createMapLabel(int area, QString text, float x, float y, float z, QColor fg, QColor bg, bool showOnTop=true, bool noScaling=true, qreal zoom=15.0, int fontSize=15 );
    void deleteMapLabel( int area, int labelID );
    bool addRoom( int id=0 );
    bool setRoomArea( int id, int area, bool isToDeferAreaRelatedRecalculations = false );
    // void deleteRoom( int id );
    void deleteArea( int id );
    int createNewRoomID();
    void logError(QString &msg);
    void tidyMap( int area );
    void getConnectedNodesGreaterThanX( int id, int x );
    void getConnectedNodesSmallerThanX( int id, int x );
    void getConnectedNodesGreaterThanY( int id, int x );
    void getConnectedNodesSmallerThanY( int id, int x );
    void astBreitenAnpassung( int id, int );
    void astHoehenAnpassung( int id, int );
    bool setExit( int from, int to, int dir );
    bool setRoomCoordinates( int id, int x, int y, int z );
    void audit(); // Was init( Host * ) but host pointer was not used and it does not initialise a map!
    QList<int> detectRoomCollisions( int id );
    void solveRoomCollision( int id, int creationDirection, bool PCheck=true );
    void setRoom( int );
    bool findPath( int from, int to );
    bool gotoRoom( int );
    bool gotoRoom( int, int );
    void setView( float, float, float, float );
    bool serialize( QDataStream & );
    bool restore( QString );
    const bool retrieveMapFileStats( QString, QString *, int *, int *, int *, int * );
    void initGraph();
    void exportMapToDatabase();
    void importMapFromDatabase();
    void connectExitStub(int roomId, int dirType);
    void postMessage( const QString text );
    void set3DViewCenter( const int, const int, const int, const int ); // Used by the 2D mapper to send view center coordinates to 3D one


    TRoomDB * mpRoomDB;
    QMap<int, int> envColors;
    QVector3D span;
    QPointer<Host> mpHost;
    // Was a single int mRoomId but that breaks things when maps are
    // copied/shared between profiles - so now we track the profile name
    QHash<QString, int> mRoomIdHash;
    bool m2DPanMode;
    bool mLeftDown;
    bool mRightDown;
    float m2DPanXStart;
    float m2DPanYStart;
    int mViewArea;
    int mTargetID;
    QList<int> mPathList;
    QList<QString> mDirList;
    QList<int> mWeightList;
    QMap<int, QColor> customEnvColors;
    QMap<int, QVector3D> unitVectors;
    QMap<int, int> reverseDirections; // contains complementary directions of dirs on TRoom.h
    GLWidget * mpM;
    dlgMapper * mpMapper;
    QMap<int, int> roomidToIndex;
    // QMap<int, int> indexToRoomid;

    QMap<QString, int> pixNameTable;
    QMap<int, QPixmap> pixTable;
    typedef adjacency_list<listS, vecS, directedS, no_property, property<edge_weight_t, cost> > mygraph_t;
    typedef property_map<mygraph_t, edge_weight_t>::type WeightMap;
    typedef mygraph_t::vertex_descriptor vertex;
    typedef mygraph_t::edge_descriptor edge_descriptor;
    mygraph_t g;
    QHash<QPair<unsigned int, unsigned int>, route> edgeHash; // For Mudlet to decode BGL edges
    std::vector<location> locations;
    bool mMapGraphNeedsUpdate;
    bool mNewMove;
    QMap<qint32, QMap<qint32, TMapLabel> > mapLabels;

    int mVersion; // loaded map file format version
    const int mDefaultVersion; // replaces CURRENT_MAP_VERSION
    const int mMaxVersion; // normally the same as mDefaultVersion but can be
                           // higher for development builds and is the maximum
                           // version the development build can parse.
    const int mMinVersion; // normally the same as mDefaultVersion but can be
                           // lower for release builds and is the minimum
                           // version recommended for saving , which might
                           // perhaps be one less than mDefault to permit sharing
                           // of a map with users of an older version "in the field"!
    int mSaveVersion; // what to use when saving the map, defaults to mDefaultVersion
                      // but can be override by control in special options (last)
                      // tab on profile preference dialog using the limits set
                      // by mMinVersion and mMaxVersion.

    QMap<QString, QString> mUserData;

private:
    QStringList mStoredMessages;
};

#endif // MUDLET_TMAP_H
