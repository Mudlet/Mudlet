#ifndef MUDLET_TMAP_H
#define MUDLET_TMAP_H

/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2014-2016, 2018-2022 by Stephen Lyons                   *
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
#include "utils.h"

#include "pre_guard.h"
#include <QApplication>
#include <QColor>
#include <QFont>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QNetworkReply>
#include <QPixmap>
#include <QPointer>
#include <QSizeF>
#include <QVector3D>
#include <stdlib.h>
#include <optional>
#include "post_guard.h"

#define DIR_NORTH 1
#define DIR_NORTHEAST 2
#define DIR_NORTHWEST 3
#define DIR_EAST 4
#define DIR_WEST 5
#define DIR_SOUTH 6
#define DIR_SOUTHEAST 7
#define DIR_SOUTHWEST 8
#define DIR_UP 9
#define DIR_DOWN 10
#define DIR_IN 11
#define DIR_OUT 12
#define DIR_OTHER 13

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
class MapInfoContributorManager;

class TMap : public QObject
{
    Q_OBJECT

    QString mDefaultAreaName;
    QString mUnnamedAreaName;

public:
    Q_DISABLE_COPY(TMap)
    TMap(Host*, const QString&);
    ~TMap();
    void mapClear();
    int createMapImageLabel(int area,
                            QString filePath,
                            float x,
                            float y,
                            float z,
                            float width,
                            float height,
                            float zoom,
                            bool showOnTop,
                            bool temporary);
    int createMapLabel(int area,
                       const QString& text,
                       float x,
                       float y,
                       float z,
                       QColor fg,
                       QColor bg,
                       bool showOnTop = true,
                       bool noScaling = true,
                       bool temporary = false,
                       qreal zoom = 30.0,
                       int fontSize = 50,
                       std::optional<QString> fontName = std::nullopt);
    void deleteMapLabel(int area, int labelID);
    bool addRoom(int id = 0);
    bool setRoomArea(int id, int area, bool isToDeferAreaRelatedRecalculations = false);
    void deleteArea(int id);
    int createNewRoomID(int minimumId = 1);
    void logError(QString& msg);
    bool setExit(int from, int to, int dir);
    bool setRoomCoordinates(int id, int x, int y, int z);
    void update();

    void audit();

    QList<int> detectRoomCollisions(int id);
    void setRoom(int);
    bool findPath(int from, int to);
    bool gotoRoom(int);
    bool gotoRoom(int, int);
    bool serialize(QDataStream&, int saveVersion = 0);
    bool restore(QString location, bool downloadIfNotFound = true);
    bool retrieveMapFileStats(QString, QString*, int*, int*, int*, int*);
    void initGraph();
    QString connectExitStubByDirection(const int fromRoomId, const int dirType);
    QString connectExitStubByToId(const int fromRoomId, const int toRoomId);
    QString connectExitStubByDirectionAndToId(const int fromRoomId, const int dirType, const int toRoomId);
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

    // Downloads a map, then installs it via readXmlMapFile().
    // Protected from running twice by 'mImportRunning' flag
    void downloadMap(const QString& remoteUrl = QString(), const QString& localFileName = QString());

    // like 'downloadMap' but for local files:
    bool importMap(QFile&, QString* errMsg = nullptr);

    // Used at end of downloadMap(...) OR as part of importMap(...)
    bool readXmlMapFile(QFile&, QString* errMsg = nullptr);

    // Use progress dialog for post-download operations.
    void reportStringToProgressDialog(QString);

    // Use progress dialog for post-download operations.
    void reportProgressToProgressDialog(int, int);

    // Show which rooms have which symbols:
    QHash<QString, QSet<int>> roomSymbolsHash();

    void setMmpMapLocation(const QString &location);
    QString getMmpMapLocation() const;

    // has setRoomNamesShown ever been called on this map?
    bool getRoomNamesPresent();
    // show room labels on the map?
    bool getRoomNamesShown();
    void setRoomNamesShown(bool shown);

    std::pair<bool, QString> writeJsonMapFile(const QString&);
    std::pair<bool, QString> readJsonMapFile(const QString&, const bool translatableTexts = false, const bool allowUserCancellation = true);
    int getCurrentProgressRoomCount() const { return mProgressDialogRoomsCount; }
    bool incrementJsonProgressDialog(const bool isExportNotImport, const bool isRoomNotLabel, const int increment = 1);
    QString getDefaultAreaName() const { return mDefaultAreaName; }
    QString getUnnamedAreaName() const { return mUnnamedAreaName; }

    QColor getColor(int id);

    static void writeJsonColor(QJsonObject&, const QColor&);
    static QColor readJsonColor(const QJsonObject&);
    void restore16ColorSet();

    // These trivial methods are to prevent casual modification to the
    // underlying flag (and by setting a breakpoint on setUnsave() we can
    // determine which bit of code is responsible for changing the flag!)
    void setUnsaved(const char*);
    void resetUnsaved() { mUnsavedMap = false; }
    bool isUnsaved() const { return mUnsavedMap; }

    TRoomDB* mpRoomDB = nullptr;
    QMap<int, int> mEnvColors;
    QPointer<Host> mpHost;
    QString mProfileName;

    // Was a single int mRoomId but that breaks things when maps are
    // copied/shared between profiles - so now we track the profile name
    QHash<QString, int> mRoomIdHash;
    bool m2DPanMode = false;
    bool mLeftDown = false;
    bool mRightDown = false;
    float m2DPanXStart = 0.0f;
    float m2DPanYStart = 0.0f;
    int mTargetID = 0;
    QList<int> mPathList;
    QList<QString> mDirList;
    QList<int> mWeightList;
    QMap<int, QColor> mCustomEnvColors;
    inline static const QMap<int, QVector3D> scmUnitVectors = {
        {DIR_NORTH, {0, -1, 0}},
        {DIR_NORTHEAST, {1, -1, 0}},
        {DIR_NORTHWEST, {-1, -1, 0}},
        {DIR_EAST, {1, 0, 0}},
        {DIR_WEST, {-1, 0, 0}},
        {DIR_SOUTH, {0, 1, 0}},
        {DIR_SOUTHEAST, {1, 1, 0}},
        {DIR_SOUTHWEST, {-1, 1, 0}},
        {DIR_UP, {0, 0, 1}},
        {DIR_DOWN, {0, 0, -1}}};

    // contains complementary directions of DIR_XXXX
    inline static const QMap<int, int> scmReverseDirections = {
        {DIR_NORTH, DIR_SOUTH},
        {DIR_NORTHEAST, DIR_SOUTHWEST},
        {DIR_NORTHWEST, DIR_SOUTHEAST},
        {DIR_EAST, DIR_WEST},
        {DIR_WEST, DIR_EAST},
        {DIR_SOUTH, DIR_NORTH},
        {DIR_SOUTHEAST, DIR_NORTHWEST},
        {DIR_SOUTHWEST, DIR_NORTHEAST},
        {DIR_UP, DIR_DOWN},
        {DIR_DOWN, DIR_UP},
        {DIR_IN, DIR_OUT},
        {DIR_OUT, DIR_IN}};

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
    bool mMapGraphNeedsUpdate = true;
    bool mNewMove = true;

    // Replaced CURRENT_MAP_VERSION, default map version that new maps will get:
    const int mDefaultVersion = 20;

    // Normally the same as mDefaultVersion but can be higher for development
    // builds and is the maximum version the development build can parse, it is
    // thus the maximum version of the map format that this Mudlet can
    // understand and will allow the user to load:
    /*
     * WARNING: There is new code that will be activated when this is incremented
     * above 20:
     * * The room special exits (QMap<QString, int>) and special exit locks data
     *   QSet<QString> will be stored directly in those new container elements
     *   replacing the backwards compatible combination (a QMultiMap<int, QString>)
     *   that prefixed a '1' for a locked exit or '0' for an unlocked one onto the
     *   special exit name (stored in the VALUE) in the old format.
     * It has been tested and *seems* to work. SlySven - 2020/12
     */
    const int mMaxVersion = 20;

    // Ideally would be the same as mDefaultVersion but we have it lower,
    // particularly for release builds and is the minimum version allowed for
    // saving, which would perhaps be one less than mDefaultVersion to permit
    // sharing of a map with users of an older version "in the field"!
    const int mMinVersion = 17;

    // what to use when saving the map, defaults to mDefaultVersion but can be
    // overridden by control in mapper tab on profile preference dialog using
    // the limits set by mMinVersion and mMaxVersion:
    int mSaveVersion = mDefaultVersion;

    // Loaded map file format version - overwritten during a map load to the
    // value for that file:
    int mVersion = mDefaultVersion;

    QMap<QString, QString> mUserData;

    // This is the font that the map file or user *wants* to use - what actually
    // gets used may be different, and will be stored in the T2DMap class.
    QFont mMapSymbolFont = QFont(qsl("Bitstream Vera Sans Mono"), 12, QFont::Normal);
    // For 2D mapper: the symbol text is scaled to fill a rectangle based upone
    // the room symbol with this scaling factor. This may be needed because
    // different users could have differing requirement for symbol sizing
    // depending on font usage, language, symbol choice, etc. but this has not
    // (yet) made (user) adjustable:
    qreal mMapSymbolFontFudgeFactor = 1.0;
    // Disables font substitution if set:
    bool mIsOnlyMapSymbolFontToBeUsed = false;

    // location of an MMP map provided by the game
    QString mMmpMapLocation;

    // Base color(s) for the player room in the mappers:
    QColor mPlayerRoomOuterColor;
    QColor mPlayerRoomInnerColor;
    // These three are actually set to values from the Host class but initialising
    // them to the same defaults here keeps Coverity happy:
    // Mode selected - 0 is closest to original style:
    quint8 mPlayerRoomStyle = 0;
    // Percentage of the room size (actually width) for the outer diameter of
    // the circular marking, integer percentage clamped in the preferences
    // between 200 and 50 - default 120:
    quint8 mPlayerRoomOuterDiameterPercentage = 120;
    // Percentage of the outer size for the inner diameter of the circular
    // marking, integer percentage clamped in the preferences between 83 and 0,
    // with a default of 70. NOT USED FOR "Original" style marking (the 0'th
    // one):
    quint8 mPlayerRoomInnerDiameterPercentage = 70;

    MapInfoContributorManager* mMapInfoContributorManager;

public slots:
    void slot_setDownloadProgress(qint64, qint64);
    void slot_downloadCancel();
    void slot_downloadError(QNetworkReply::NetworkError);
    void slot_replyFinished(QNetworkReply*);


private:
    const QString createFileHeaderLine(QString, QChar);
    void writeJsonUserData(QJsonObject&) const;
    void readJsonUserData(const QJsonObject&);
    bool validatePotentialMapFile(QFile&, QDataStream&);

    QStringList mStoredMessages;

    // Key is room number (where renumbered is the original one), Value is the errors, appended as they are found
    QMap<int, QList<QString>> mMapAuditRoomErrors;

    // As for the Room ones but with key as the area number
    QMap<int, QList<QString>> mMapAuditAreaErrors;

    // For the whole map
    QList<QString> mMapAuditErrors;

    // Are things so bad the user needs to check the log (ignored if messages ARE already sent to screen)
    bool mIsFileViewingRecommended = false;

    // Moved and revised from dlgMapper:
    QNetworkAccessManager* mpNetworkAccessManager = nullptr;

    QNetworkReply* mpNetworkReply = nullptr;
    QString mLocalMapFileName;
    int mExpectedFileSize = 0;
    bool mImportRunning = false;

    QProgressDialog* mpProgressDialog = nullptr;
    // Using during updates of text in progress dialog partially from other
    // classes:
    int mProgressDialogAreasTotal = 0;
    int mProgressDialogAreasCount = 0;
    int mProgressDialogRoomsTotal = 0;
    int mProgressDialogRoomsCount = 0;
    int mProgressDialogLabelsTotal = 0;
    int mProgressDialogLabelsCount = 0;

    // Used to flag whether the map auto-save needs to be done after the next interval:
    bool mUnsavedMap = false;
};

#endif // MUDLET_TMAP_H
