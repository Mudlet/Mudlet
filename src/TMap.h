#ifndef MUDLET_TMAP_H
#define MUDLET_TMAP_H

/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2014-2016, 2018-2019 by Stephen Lyons                   *
 *                                               - slysven@virginmedia.com *
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
#if defined(INCLUDE_3DMAPPER)
#include "glwidget.h"
#endif

#include "pre_guard.h"
#include <QApplication>
#include <QColor>
#include <QFont>
#include <QMap>
#include <QMutex>
#include <QNetworkReply>
#include <QPixmap>
#include <QPointer>
#include <QSizeF>
#include <QVector3D>
#include "post_guard.h"

#include <stdlib.h>

class dlgMapper;
class Host;
#if defined(INCLUDE_3DMAPPER)
class GLWidget;
#endif
class TArea;
class TRoom;
class TRoomDB;
class QFile;
class QNetworkAccessManager;
class QProgressDialog;


class TMapLabel
{
public:
    TMapLabel()
    {
        highlight = false;
        showOnTop = false;
        noScaling = false;
    }

    QVector3D pos;
    QPointF pointer;
    QSizeF size;
    QSizeF clickSize;
    QString text;
    QColor fgColor;
    QColor bgColor;
    QPixmap pix;
    bool highlight;
    bool showOnTop;
    bool noScaling;
};


class TMap : public QObject
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(TMap)
    TMap(Host*, const QString&);
    ~TMap();
    void mapClear();
    int createMapLabelID(int area);
    int createMapImageLabel(int area, QString filePath, float x, float y, float z, float width, float height, float zoom, bool showOnTop, bool noScaling);
    int createMapLabel(int area, QString text, float x, float y, float z, QColor fg, QColor bg, bool showOnTop = true, bool noScaling = true, qreal zoom = 15.0, int fontSize = 15);
    void deleteMapLabel(int area, int labelID);
    bool addRoom(int id = 0);
    bool setRoomArea(int id, int area, bool isToDeferAreaRelatedRecalculations = false);
    void deleteArea(int id);
    int createNewRoomID(int minimumId = 1);
    void logError(QString& msg);
    void tidyMap(int area);
    bool setExit(int from, int to, int dir);
    bool setRoomCoordinates(int id, int x, int y, int z);

    // Was init( Host * ) but host pointer was not used and it does not initialise a map!
    void audit();

    QList<int> detectRoomCollisions(int id);
    void solveRoomCollision(int id, int creationDirection, bool PCheck = true);
    void setRoom(int);
    bool findPath(int from, int to);
    bool gotoRoom(int);
    bool gotoRoom(int, int);
    bool serialize(QDataStream&, int saveVersion = 0);
    bool restore(QString location, bool downloadIfNotFound = true);
    bool retrieveMapFileStats(QString, QString*, int*, int*, int*, int*);
    void initGraph();
    void connectExitStub(int roomId, int dirType);
    void postMessage(QString text);

    // Used by the 2D mapper to send view center coordinates to 3D one
    void set3DViewCenter(int, int, int, int);

    void appendRoomErrorMsg(int, QString, bool isToSetFileViewingRecommended = false);
    void appendAreaErrorMsg(int, QString, bool isToSetFileViewingRecommended = false);
    void appendErrorMsg(QString, bool isToSetFileViewingRecommended = false);
    void appendErrorMsgWithNoLf(QString, bool isToSetFileViewingRecommended = false);

    // If the argument is true does not write out any thing if there is no data
    // to dump, intended to be used before an operation like a map load so that
    // any messages previously recorded are not associated with a "fresh" batch
    // from the operation.
    void pushErrorMessagesToFile(QString, bool isACleanup = false);

    // Moved and revised from dlgMapper:
    void downloadMap(const QString& remoteUrl = QString(), const QString& localFileName = QString());

    // Also uses readXmlMapFile(...) but for local files:
    bool importMap(QFile&, QString* errMsg = Q_NULLPTR);

    // Used at end of downloadMap(...) OR as part of importMap(...) but not by
    // both at the same time thanks to mXmlImportMutex
    bool readXmlMapFile(QFile&, QString* errMsg = Q_NULLPTR);

    // Use progress dialog for post-download operations.
    void reportStringToProgressDialog(QString);

    // Use progress dialog for post-download operations.
    void reportProgressToProgressDialog(int, int);

    // Show which rooms have which symbols:
    QHash<QString, QSet<int>> roomSymbolsHash();

    void setMmpMapLocation(const QString &location);
    QString getMmpMapLocation() const;


    TRoomDB* mpRoomDB;
    QMap<int, int> envColors;
    QPointer<Host> mpHost;
    QString mProfileName;

    // Was a single int mRoomId but that breaks things when maps are
    // copied/shared between profiles - so now we track the profile name
    QHash<QString, int> mRoomIdHash;
    bool m2DPanMode;
    bool mLeftDown;
    bool mRightDown;
    float m2DPanXStart;
    float m2DPanYStart;
    int mTargetID;
    QList<int> mPathList;
    QList<QString> mDirList;
    QList<int> mWeightList;
    QMap<int, QColor> customEnvColors;
    QMap<int, QVector3D> unitVectors;

    // contains complementary directions of dirs on TRoom.h
    QMap<int, int> reverseDirections;

#if defined(INCLUDE_3DMAPPER)
    QPointer<GLWidget> mpM;
#endif
    QPointer<dlgMapper> mpMapper;
    QMap<int, int> roomidToIndex;

    typedef boost::adjacency_list<boost::listS, boost::vecS, boost::directedS, boost::no_property, boost::property<boost::edge_weight_t, cost>> mygraph_t;
    typedef boost::property_map<mygraph_t, boost::edge_weight_t>::type WeightMap;
    typedef mygraph_t::vertex_descriptor vertex;
    typedef mygraph_t::edge_descriptor edge_descriptor;
    mygraph_t g;
    QHash<QPair<unsigned int, unsigned int>, route> edgeHash; // For Mudlet to decode BGL edges
    std::vector<location> locations;
    bool mMapGraphNeedsUpdate;
    bool mNewMove;
    QMap<qint32, QMap<qint32, TMapLabel>> mapLabels;

    // loaded map file format version
    int mVersion;

    // replaces CURRENT_MAP_VERSION
    const int mDefaultVersion;

    // normally the same as mDefaultVersion but can be
    // higher for development builds and is the maximum
    // version the development build can parse.
    const int mMaxVersion;

    // normally the same as mDefaultVersion but can be
    // lower for release builds and is the minimum
    // version recommended for saving , which might
    // perhaps be one less than mDefault to permit sharing
    // of a map with users of an older version "in the field"!
    const int mMinVersion;

    // what to use when saving the map, defaults to mDefaultVersion
    // but can be override by control in special options (last)
    // tab on profile preference dialog using the limits set
    // by mMinVersion and mMaxVersion.
    int mSaveVersion;

    QMap<QString, QString> mUserData;

    // This is the font that the map file or user *wants* to use - what actually
    // gets used may be different, and will be stored in the T2DMap class.
    QFont mMapSymbolFont;
    // For 2D mapper: the symbol text is scaled to fill a rectangle based upone
    // the room symbol with this scaling factor. This may be needed because
    // different users could have differing requirement for symbol sizing
    // depending on font usage, language, symbol choice, etc. but this has not
    // (yet) made (user) adjustable:
    qreal mMapSymbolFontFudgeFactor;
    // Disables font substitution if set:
    bool mIsOnlyMapSymbolFontToBeUsed;

    // location of an MMP map provided by the game
    QString mMmpMapLocation;

    // Base color(s) for the player room in the mappers:
    QColor mPlayerRoomOuterColor;
    QColor mPlayerRoomInnerColor;
    // Mode selected - 0 is closest to original style:
    quint8 mPlayerRoomStyle;
    // Percentage of the room size (actually width) for the outer diameter of
    // the circular marking, integer percentage clamped in the preferences
    // between 200 and 50 - default 120:
    quint8 mPlayerRoomOuterDiameterPercentage;
    // Percentage of the outer size for the inner diameter of the circular
    // marking, integer percentage clamped in the preferences between 83 and 0,
    // with a default of 70. NOT USED FOR "Original" style marking (the 0'th
    // one):
    quint8 mPlayerRoomInnerDiameterPercentage;

public slots:
    // Moved and revised from dlgMapper:
    void slot_setDownloadProgress(qint64, qint64);
    void slot_downloadCancel();
    void slot_downloadError(QNetworkReply::NetworkError);
    void slot_replyFinished(QNetworkReply*);


private:
    const QString createFileHeaderLine(QString, QChar);

    QStringList mStoredMessages;

    // Key is room number (where renumbered is the original one), Value is the errors, appended as they are found
    QMap<int, QList<QString>> mMapAuditRoomErrors;

    // As for the Room ones but with key as the area number
    QMap<int, QList<QString>> mMapAuditAreaErrors;

    // For the whole map
    QList<QString> mMapAuditErrors;

    // Are things so bad the user needs to check the log (ignored if messages ARE already sent to screen)
    bool mIsFileViewingRecommended;

    // Moved and revised from dlgMapper:
    QNetworkAccessManager* mpNetworkAccessManager;

    QProgressDialog* mpProgressDialog;
    QNetworkReply* mpNetworkReply;
    QString mLocalMapFileName;
    int mExpectedFileSize;
    QMutex mXmlImportMutex;
};

#endif // MUDLET_TMAP_H
