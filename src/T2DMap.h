
/***************************************************************************
 *   Copyright (C) 2008-2011 by Heiko Koehn - KoehnHeiko@googlemail.com    *
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

#ifndef T2DMAP_H
#define T2DMAP_H

#include <QWidget>
#include <TMap.h>
#include <QPixmap>

class T2DMap : public QWidget
{
    Q_OBJECT

public:

    explicit T2DMap( QWidget *parent = 0 );
    void     paintMap();
    void     setMapZoom( int zoom );
    QColor   getColor( int id );
    QColor   _getColor( int id );
    void     init();
    void     exportAreaImage( int );
    void     paintEvent( QPaintEvent * );
    void     mousePressEvent(QMouseEvent * );
    void     mouseDoubleClickEvent ( QMouseEvent * event );
    bool     event(QEvent * event );
    void     wheelEvent ( QWheelEvent * );
    void     mouseMoveEvent( QMouseEvent * event );
    void     mouseReleaseEvent(QMouseEvent * e );
    int      getTopLeftSelection();
    void     setRoomSize( double );
    void     setExitSize( double );
    void     createLabel( QRectF labelRect );
    TMap *   mpMap;
    Host *   mpHost;
    int      xzoom;
    int      yzoom;
    int      _rx;
    int      _ry;
    QPoint   mPHighlight;
    bool     mPick;
    int      mTarget;
    //int      mRoomSelection;
    bool     mStartSpeedWalk;
    QMap<int, QPoint> mAreaExitList;
    QMap<QString, QStringList> mUserActions; //string list: 0 is event name, 1 is menu it is under if it is
    QMap<QString, QStringList> mUserMenus; //unique name, List:parent name ("" if null), display name
    QPoint   mMoveTarget;
    bool     mRoomBeingMoved;
    QPoint   mPHighlightMove;
    float    mTX;
    float    mTY;
    int      mChosenRoomColor;
    float    xspan;
    float    yspan;
    bool     mMultiSelection;
    QRectF   mMultiRect;
    bool     mPopupMenu;
    QList<int> mMultiSelectionList;
    QPoint   mOldMousePos;
    bool     mNewMoveAction;
    QRectF   mMapInfoRect;
    int      mFontHeight;
    bool     mShowRoomID;
    QMap<int,QPixmap> mPixMap;
//    QMap<int, QPixmap > mGridPix;
    int      gzoom;
    double   rSize;
    double   eSize;
    int      mRID;
    int      mAID;
    int      mOx;
    int      mOy;
    int      mOz;
    bool     mShiftMode;
    bool     mShowInfo;
    QComboBox * arealist_combobox;
    QDialog * mpCustomLinesDialog;
    int  mCustomLinesRoomFrom;
    int  mCustomLinesRoomTo;
    QString mCustomLinesRoomExit;
    QComboBox * mpCurrentLineStyle;
    QString mCurrentLineStyle;
    QPushButton * mpCurrentLineColor;
    QColor mCurrentLineColor;
    QCheckBox * mpCurrentLineArrow;
    bool mCurrentLineArrow;
    bool mShowGrid;
    QPointF mLastMouseClick;
    bool mBubbleMode;
    bool mMapperUseAntiAlias;
    bool mLabelHilite;
    bool mMoveLabel;
    int mCustomLineSelectedRoom;
    QString mCustomLineSelectedExit;
    int mCustomLineSelectedPoint;
    QTreeWidget mMultiSelectionListWidget;
    bool mSizeLabel;
    bool gridMapSizeChange;
    bool isCenterViewCall;
    QString mHelpMsg;

signals:

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
    void switchArea(QString);
    void toggleShiftMode();
    void shiftUp();
    void shiftDown();
    void shiftLeft();
    void shiftRight();
    void slot_setCharacter();
    void slot_setImage();
    void slot_movePosition();
    void slot_defineNewColor();
    void slot_selectRoomColor(QListWidgetItem * pI );
    void slot_moveRoom();
    void slot_deleteRoom();
    void slot_changeColor();
    void slot_spread();
    void slot_shrink();
    void slot_setExits();
    void slot_setUserData();
    void slot_lockRoom();
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
};

#endif // T2DMAP_H
