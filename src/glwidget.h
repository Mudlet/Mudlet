/***************************************************************************
 *   Copyright (C) 2010 by Heiko Koehn ( KoehnHeiko@googlemail.com )       *
 *                                                                         *
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

#ifndef GLWIDGET_H
#define GLWIDGET_H
class TMap;
#include <QtOpenGL/qgl.h>//<QGLWidget>
#include "TMap.h"
#include "Host.h"

class Host;


class GLWidget : public QGLWidget
{
    Q_OBJECT

public:
    GLWidget(QWidget *parent = 0);
    GLWidget(TMap * pM, QWidget *parent = 0);
    ~GLWidget();
    void wheelEvent ( QWheelEvent * e );

    bool is2DView;

    QSize minimumSizeHint() const;
    QSize sizeHint() const;

    int mRID;
    int mAID;
    int mOx;
    int mOy;
    int mOz;
    bool mShiftMode;
    bool mShowInfo;


public slots:

    void showInfo();
    void shiftUp();
    void shiftDown();
    void shiftLeft();
    void shiftRight();
    void shiftZup();
    void shiftZdown();
    void showArea(QString);
    void setXRotation(int angle);
    void setYRotation(int angle);
    void setZRotation(int angle);
    void setXDist(int angle);
    void setYDist(int angle);
    void setZDist(int angle);
    void setScale(int);
    void goRoom(const QString &);
    void fullView();
    void singleView();
    void increaseTop();
    void reduceTop();
    void increaseBottom();
    void reduceBottom();
    void defaultView();
    void sideView();
    void topView();

signals:
    void xRotationChanged(int angle);
    void yRotationChanged(int angle);
    void zRotationChanged(int angle);
    void xDistChanged(int angle);
    void yDistChanged(int angle);
    void zDistChanged(int angle);

protected:
    void initializeGL();
    void paintGL();
    void resizeGL(int width, int height);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);

public:
    float xRot;
    float yRot;
    float zRot;
    float xDist;
    float yDist;
    float zDist;
    float dehnung;
    int mShowTopLevels;
    int mShowBottomLevels;
    QPoint lastPos;
    QColor qtGreen;
    QColor qtPurple;

    GLfloat rotTri, rotQuad;
    float mScale;
    TMap * mpMap;
    int mTarget;
    Host * mpHost;
    QMap<int,int> mQuads;
};
#endif
