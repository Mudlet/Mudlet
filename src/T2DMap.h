#ifndef MUDLET_T2DMAP_H
#define MUDLET_T2DMAP_H

/***************************************************************************
 *   Copyright (C) 2008-2012 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2016, 2018-2019, 2022 by Stephen Lyons                        *
 *                                               - slysven@virginmedia.com *
 *   Copyright (C) 2021-2022 by Piotr Wilczynski - delwing@gmail.com       *
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

#include "dlgMapLabel.h"
#include "dlgRoomSymbol.h"
#include "dlgRoomProperties.h"

#include "pre_guard.h"
#include <QCache>
#include <QColor>
#include <QFont>
#include <QPixmap>
#include <QPointer>
#include <QString>
#include <QTreeWidget>
#include <QWidget>
#include "post_guard.h"

class Host;
class TArea;
class TMap;
class TRoom;
class MapInfoProperties;

class QCheckBox;
class QComboBox;
class QElapsedTimer;
class QListWidgetItem;
class QPushButton;
class QTreeWidgetItem;


class T2DMap : public QWidget
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(T2DMap)
    explicit T2DMap(QWidget* parent = nullptr);
    void setMapZoom(qreal zoom);
    void init();
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    bool event(QEvent* event) override;
    void wheelEvent(QWheelEvent*) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* e) override;

    // Was getTopLeft() which returned an index into mMultiSelectionList but that
    // has been been changed to mMultiSelectionSet which cannot be accessed via
    // an index in the same way - this function now sets
    // mMultiSelectionHighlightRoomId and returns a (bool) on success or failure
    // to do so.
    bool getCenterSelection();
    int getCenterSelectedRoomId() const { return mMultiSelectionHighlightRoomId; }

    void setRoomSize(double);
    void setExitSize(double);
    void createLabel(QRectF labelRectangle);
    // Clears cache so new symbols are built at next paintEvent():
    void flushSymbolPixmapCache() {mSymbolPixmapCache.clear();}
    void addSymbolToPixmapCache(const QString, const QString, const QColor, const bool);
    void setPlayerRoomStyle(const int style);
#if (QT_VERSION) >= (QT_VERSION_CHECK(5, 15, 0))
    // This is NOT used as a slot in newer versions
    void switchArea(const QString& newAreaName);
#endif


    TMap* mpMap = nullptr;
    QPointer<Host> mpHost;
    qreal xyzoom = 20.0;
    int mRX = 0;
    int mRY = 0;
    QPoint mPHighlight;
    bool mPick = false;
    int mTarget = 0;
    bool mStartSpeedWalk = false;


    // string list: 0 is event name, 1 is menu it is under if it is
    QMap<QString, QStringList> mUserActions;

    // unique name, List:parent name ("" if null), display name
    QMap<QString, QStringList> mUserMenus;

    bool mRoomBeingMoved = false;
    // These are the on-screen width and height pixel numbers of the area for a
    // room symbol, (for the non-grid map mode case what gets filled in is
    // multiplied by rsize which is 1.0 to exactly fill space between adjacent
    // coordinates):
    float mRoomWidth = 0.0f;
    float mRoomHeight = 0.0f;
    int mChosenRoomColor = 5;
    float xspan = 0.0f;
    float yspan = 0.0f;

    // Flag that the "drag to select rectangle"
    // (mMultiRect) is active and is being *resized*
    // by dragging
    bool mMultiSelection = false;

    QRectF mMultiRect;
    bool mPopupMenu = false;
    QSet<int> mMultiSelectionSet;
    bool mNewMoveAction = false;
    QRect mMapInfoRect;
    int mFontHeight = 20;
    bool mShowRoomID = false;
    QMap<int, QPixmap> mPixMap;
    double rSize = 0.5;
    double eSize = 3.0;
    int mRoomID = 0;
    int mAreaID = 0;
    // These next three represent the room coordinates at the middle of the map
    // the first pair needs to not be integer types as a more flexible zoom
    // in/out mechanism was adopted that meant non-integral coordinates were
    // needed, OTOH a "snap to a fractional (power of 2?) value" might help to
    // keep the decimal numbers reasonable:
    qreal mOx = 0.0;
    qreal mOy = 0.0;
    int mOz = 0;
    bool mShiftMode = false;
    QComboBox* arealist_combobox = nullptr;
    QPointer<QDialog> mpCustomLinesDialog;
    int mCustomLinesRoomFrom = 0;
    int mCustomLinesRoomTo = 0;
    QString mCustomLinesRoomExit;

    // Pointers to controls that hold the settings
    QPointer<QComboBox> mpCurrentLineStyle;
    QPointer<QPushButton> mpCurrentLineColor;
    QPointer<QCheckBox> mpCurrentLineArrow;

    // Variables that hold the current or last used setting:
    Qt::PenStyle mCurrentLineStyle = Qt::SolidLine;
    QColor mCurrentLineColor = QColorConstants::Red;
    bool mCurrentLineArrow = true;

    bool mBubbleMode = false;
    bool mMapperUseAntiAlias = true;

    // Controls if the mapper is in view-only mode
    bool mMapViewOnly = true;

    bool mLabelHighlighted = false;
    bool mMoveLabel = false;
    int mCustomLineSelectedRoom = 0;
    QString mCustomLineSelectedExit;
    int mCustomLineSelectedPoint = -1;
    QTreeWidget mMultiSelectionListWidget;
    bool mSizeLabel = false;
    bool isCenterViewCall = false;
    QString mHelpMsg;
    QColor mOpenDoorColor = QColor(10, 155, 10);
    QColor mClosedDoorColor = QColor(155, 155, 10);
    QColor mLockedDoorColor = QColor(155, 10, 10);
    // Introduced as a side effect of #4608 the larger area exit arrows don't
    // always work well on existing maps - so allow for them to be reverted
    // almost back to how they were before that PR:
    bool mLargeAreaExitArrows = false;

public slots:
    void slot_roomSelectionChanged();
    void slot_deleteCustomExitLine();
    void slot_moveLabel();
    void slot_deleteLabel();
    void slot_editLabel();
    void slot_setPlayerLocation();
    void slot_toggleMapViewOnly();
    void slot_createLabel();
    void slot_customLineColor();
    void slot_shiftZup();
    void slot_shiftZdown();
#if (QT_VERSION) < (QT_VERSION_CHECK(5, 15, 0))
    // This is ONLY used as a slot in older versions
    void slot_switchArea(const QString& newAreaName);
#endif
// Not used: void slot_toggleShiftMode();
    void slot_shiftUp();
    void slot_shiftDown();
    void slot_shiftLeft();
    void slot_shiftRight();
    void slot_showSymbolSelection();
    void slot_showPropertiesSelection();
    void slot_setRoomSymbol(QString newSymbol, QColor symbolColor, QSet<TRoom*> rooms);
    void slot_setRoomProperties(QString newSymbol, QColor symbolColor, QSet<TRoom*> rooms);
    void slot_setImage();
    void slot_movePosition();
    void slot_defineNewColor();
    void slot_selectRoomColor(QListWidgetItem* pI);
    void slot_moveRoom();
    void slot_deleteRoom();
    void slot_changeColor();
    void slot_spread();
    void slot_shrink();
    void slot_setExits();
    void slot_setUserData();
    void slot_lockRoom();
    void slot_unlockRoom();
    void slot_setRoomWeight();
    void slot_setArea();
    void slot_setCustomLine();
    void slot_setCustomLine2();
    void slot_userAction(QString);
    void slot_setCustomLine2B(QTreeWidgetItem*, int);
    void slot_undoCustomLineLastPoint();
    void slot_doneCustomLine();
    void slot_customLineProperties();
    void slot_customLineAddPoint();
    void slot_customLineRemovePoint();
    void slot_cancelCustomLineDialog();
    void slot_loadMap();
    void slot_newMap();

private:
    void updateSelectionWidget();
    void resizeMultiSelectionWidget();
    std::pair<int, int> getMousePosition();
    bool checkButtonIsForGivenDirection(const QPushButton*, const QString&, const int&);
    bool sizeFontToFitTextInRect(QFont&, const QRectF&, const QString&, const quint8 percentageMargin = 10, const qreal minFontSize = 7.0);
    void drawRoom(QPainter&, QFont&, QFont&, QPen&, TRoom*, const bool isGridMode, const bool areRoomIdsLegible, const bool showRoomNames, const int, const float, const float, const QMap<int, QPointF>&);
    void paintMapInfo(const QElapsedTimer& renderTimer, QPainter& painter, const int displayAreaId, QColor& infoColor);
    int paintMapInfoContributor(QPainter&, int xOffset, int yOffset, const MapInfoProperties& properties);
    void paintRoomExits(QPainter&, QPen&, QList<int>& exitList, QList<int>& oneWayExits, const TArea*, int, float, QMap<int, QPointF>&);
    void initiateSpeedWalk(const int speedWalkStartRoomId, const int speedWalkTargetRoomId);
    inline void drawDoor(QPainter&, const TRoom&, const QString&, const QLineF&);
    void updateMapLabel(QRectF labelRectangle, int labelId, TArea* pArea);


    bool mDialogLock = false;
    struct ClickPosition {
        int x;
        int y;
    } mContextMenuClickPosition;

    // When more than zero rooms are selected this
    // is either the first (only) room in the set
    // or if getCenterSelectionId() is used the
    // room that is selected - this is so that it
    // can be painted in yellow rather than orange
    // when more than one room is selected to
    // indicate the particular room that will be
    // modified or be the center of those
    // modifications. {for slot_spread(),
    // slot_shrink(), slot_setUserData() - if ever
    // implemented, slot_setExits(),
    // slot_movePosition(), etc.} - previously have
    // used -1 but is now reset to 0 if it is not valid.
    int mMultiSelectionHighlightRoomId = 0;

    bool mIsSelectionSorting = true;
    bool mIsSelectionSortByNames = false;

    // Used to keep track of if sorting the multiple
    // room listing/selection widget, and by what,
    // as we now show room names (if present) as well.
    bool mIsSelectionUsingNames = false;
    QCache<QString, QPixmap> mSymbolPixmapCache;
    ushort mSymbolFontSize = 1;
    QFont mMapSymbolFont;
    QPointer<QAction> mpCreateRoomAction;
    // in the players current area, how many digits does the biggest room number have?
    quint8 mMaxRoomIdDigits = 0;

    // Holds the QRadialGradient details to use for the player room:
    QGradientStops mPlayerRoomColorGradentStops;
    dlgRoomSymbol* mpDlgRoomSymbol = nullptr;
    dlgRoomProperties* mpDlgRoomProperties = nullptr;
    dlgMapLabel* mpDlgMapLabel = nullptr;

private slots:
    void slot_createRoom();
};

#endif // MUDLET_T2DMAP_H
