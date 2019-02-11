#ifndef MUDLET_T2DMAP_H
#define MUDLET_T2DMAP_H

/***************************************************************************
 *   Copyright (C) 2008-2012 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2016, 2018 by Stephen Lyons - slysven@virginmedia.com   *
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
class TMap;

class QCheckBox;
class QComboBox;
class QListWidgetItem;
class QPushButton;
class QTreeWidgetItem;


class T2DMap : public QWidget
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(T2DMap)
    explicit T2DMap(QWidget* parent = nullptr);
    void paintMap();
    void setMapZoom(int zoom);
    QColor getColor(int id);
    void init();
    void exportAreaImage(int);
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
    void addSymbolToPixmapCache(const QString, const bool);


    TMap* mpMap;
    QPointer<Host> mpHost;
    int xyzoom;
    int mRX;
    int mRY;
    QPoint mPHighlight;
    bool mPick;
    int mTarget;
    bool mStartSpeedWalk;
    QMap<int, QPoint> mAreaExitsList;

    // string list: 0 is event name, 1 is menu it is under if it is
    QMap<QString, QStringList> mUserActions;

    // unique name, List:parent name ("" if null), display name
    QMap<QString, QStringList> mUserMenus;

    bool mRoomBeingMoved;
    // These are the on-screen width and height pixel numbers of the area for a
    // room symbol, (for the non-grid map mode case what gets filled in is
    // multipled by rsize which is 1.0 to exactly fill space between adjacent
    // coordinates):
    float mRoomWidth;
    float mRoomHeight;
    int mChosenRoomColor;
    float xspan;
    float yspan;

    // Flag that the "drag to select rectangle"
    // (mMultiRect) is active and is being *resized*
    // by dragging
    bool mMultiSelection;

    QRectF mMultiRect;
    bool mPopupMenu;
    QSet<int> mMultiSelectionSet;
    bool mNewMoveAction;
    QRectF mMapInfoRect;
    int mFontHeight;
    bool mShowRoomID;
    QMap<int, QPixmap> mPixMap;
    int gzoom;
    double rSize;
    double eSize;
    int mRoomID;
    int mAreaID;
    int mOx;
    int mOy;
    int mOz;
    bool mShiftMode;
    bool mShowInfo;
    QComboBox* arealist_combobox;
    QPointer<QDialog> mpCustomLinesDialog;
    int mCustomLinesRoomFrom;
    int mCustomLinesRoomTo;
    QString mCustomLinesRoomExit;

    // Pointers to controls that hold the settings
    QPointer<QComboBox> mpCurrentLineStyle;
    QPointer<QPushButton> mpCurrentLineColor;
    QPointer<QCheckBox> mpCurrentLineArrow;

    // Variables that hold the current or last used setting:
    Qt::PenStyle mCurrentLineStyle;
    QColor mCurrentLineColor;
    bool mCurrentLineArrow;

    bool mBubbleMode;
    bool mMapperUseAntiAlias;
    bool mLabelHighlighted;
    bool mMoveLabel;
    int mCustomLineSelectedRoom;
    QString mCustomLineSelectedExit;
    int mCustomLineSelectedPoint;
    QTreeWidget mMultiSelectionListWidget;
    bool mSizeLabel;
    bool isCenterViewCall;
    QString mHelpMsg;

public slots:
    void slot_roomSelectionChanged();
    void slot_deleteCustomExitLine();
    void slot_moveLabel();
    void slot_deleteLabel();
    void slot_editLabel();
    void slot_setPlayerLocation();
    void slot_createLabel();
    void slot_customLineColor();
    void shiftZup();
    void shiftZdown();
    void slot_switchArea(const QString&);
    void toggleShiftMode();
    void shiftUp();
    void shiftDown();
    void shiftLeft();
    void shiftRight();
    void slot_setSymbol();
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
    void resizeMultiSelectionWidget();
    std::pair<int, int> getMousePosition();
    bool checkButtonIsForGivenDirection(const QPushButton*, const QString&, const int&);

    bool mDialogLock;

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
    int mMultiSelectionHighlightRoomId;

    bool mIsSelectionSorting;
    bool mIsSelectionSortByNames;

    // Used to keep track of if sorting the multiple
    // room listing/selection widget, and by what,
    // as we now show room names (if present) as well.
    bool mIsSelectionUsingNames;

    QCache<QString, QPixmap> mSymbolPixmapCache;
    ushort mSymbolFontSize;
    QFont mMapSymbolFont;

    QPointer<QAction> mpCreateRoomAction;


private slots:
    void slot_createRoom();
};

#endif // MUDLET_T2DMAP_H
