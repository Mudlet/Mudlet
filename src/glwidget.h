#ifndef MUDLET_GLWIDGET_H
#define MUDLET_GLWIDGET_H

/***************************************************************************
 *   Copyright (C) 2010-2011 by Heiko Koehn - KoehnHeiko@googlemail.com    *
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
#include <QtOpenGL/qgl.h> //problem with git
#include <QPointer>
#include "post_guard.h"

class Host;
class TMap;


class GLWidget : public QGLWidget
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(GLWidget)
    GLWidget(QWidget* parent = nullptr);
    GLWidget(TMap* pM, QWidget* parent = nullptr);
    ~GLWidget();
    void wheelEvent(QWheelEvent* e) override;
    void setViewCenter(int, int, int, int);

    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

public slots:
    void showInfo();
    void shiftUp();
    void shiftDown();
    void shiftLeft();
    void shiftRight();
    void shiftZup();
    void shiftZdown();
    void setXRotation(int angle);
    void setYRotation(int angle);
    void setZRotation(int angle);
    void setXDist(int angle);
    void setYDist(int angle);
    void setZDist(int angle);
    void setScale(int);
    void goRoom(const QString&);
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
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int width, int height) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

public:
    TMap* mpMap;

private:
    bool is2DView;

    int mRID;
    int mAID;
    int mOx;
    int mOy;
    int mOz;
    bool mShiftMode;
    bool mShowInfo;

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
    int mTarget;
    QPointer<Host> mpHost;
    QMap<int, int> mQuads;
};

#endif // MUDLET_GLWIDGET_H
