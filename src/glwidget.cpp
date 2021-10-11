/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2014, 2016, 2019-2021 by Stephen Lyons                  *
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


#include "glwidget.h"


#include "mudlet.h"
#include "TArea.h"
#include "TRoomDB.h"
#include "dlgMapper.h"

#include "pre_guard.h"
#include <QtEvents>
#include "post_guard.h"

#include <QPainter>
#ifdef Q_OS_MACOS
#include <OpenGL/glu.h>
#else
#include <GL/glu.h>
#endif

#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE 0x809D
#endif


GLWidget::GLWidget(TMap* pMap, Host* pHost, QWidget *parent)
: QOpenGLWidget(parent)
, mpMap(pMap)
, mpHost(pHost)
{
    setAttribute(Qt::WA_OpaquePaintEvent);
}

QSize GLWidget::minimumSizeHint() const
{
    return QSize(50, 50);
}

QSize GLWidget::sizeHint() const
{
    return QSize(400, 400);
}

static void qNormalizeAngle(int& angle)
{
    angle /= 10;
}

void GLWidget::slot_showAllLevels()
{
    mShowTopLevels = 999999;
    mShowBottomLevels = 999999;
    update();
}


void GLWidget::slot_shiftDown()
{
    mShiftMode = true;
    mOy--;
    update();
}

void GLWidget::slot_shiftUp()
{
    mShiftMode = true;
    mOy++;
    update();
}

void GLWidget::slot_shiftLeft()
{
    mShiftMode = true;
    mOx--;
    update();
}

void GLWidget::slot_shiftRight()
{
    mShiftMode = true;
    mOx++;
    update();
}

void GLWidget::slot_shiftZup()
{
    mShiftMode = true;
    mOz++;
    update();
}

void GLWidget::slot_shiftZdown()
{
    mShiftMode = true;
    mOz--;
    update();
}

void GLWidget::slot_singleLevelView()
{
    mShowTopLevels = 0;
    mShowBottomLevels = 0;
    update();
}

void GLWidget::slot_showMoreUpperLevels()
{
    mShowTopLevels += 1;
    update();
}

void GLWidget::slot_showLessUpperLevels()
{
    mShowTopLevels--;
    if (mShowTopLevels < 0) {
        mShowTopLevels = 0;
    }
    update();
}

void GLWidget::slot_showMoreLowerLevels()
{
    mShowBottomLevels++;
    update();
}

void GLWidget::slot_showLessLowerLevels()
{
    mShowBottomLevels--;
    if (mShowBottomLevels < 0) {
        mShowBottomLevels = 0;
    }
    update();
}

void GLWidget::slot_defaultView()
{
    // Do not attempt to change between 2D and 3D map modes as the button to
    // activate this slot is only visible in the 3D mode anyhow!
    xRot = 1.0;
    yRot = 5.0;
    zRot = 10.0;
    mScale = 1.0;
    is2DView = false;
    update();
}

void GLWidget::slot_sideView()
{
    xRot = 7.0;
    yRot = -10.0;
    zRot = 0.0;
    mScale = 1.0;
    is2DView = false;
    update();
}

void GLWidget::slot_topView()
{
    xRot = 0.0;
    yRot = 0.0;
    zRot = 15.0;
    mScale = 1.0;
    // This is the ONLY place this value is set:
    is2DView = true;
    update();
}

void GLWidget::slot_setScale(int angle)
{
    mScale = 150 / (static_cast<float>(angle) + 300.0f);
    makeCurrent();
    resizeGL(width(), height());
    doneCurrent();
    update();
}

void GLWidget::slot_setCameraPositionX(int angle)
{
    qNormalizeAngle(angle);
    xRot = angle;
    is2DView = false;
    update();
}

void GLWidget::slot_setCameraPositionY(int angle)
{
    qNormalizeAngle(angle);
    yRot = angle;
    is2DView = false;
    update();
}

void GLWidget::slot_setCameraPositionZ(int angle)
{
    qNormalizeAngle(angle);
    zRot = angle;
    is2DView = false;
    update();
}

void GLWidget::initializeGL()
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
    QColor color(QColorConstants::Black);
#else
    QColor color(Qt::black);
#endif
    glClearColor(color.redF(), color.greenF(), color.blueF(), color.alphaF());
    xRot = 1;
    yRot = 5;
    zRot = 10;
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearDepth(1.0);
    is2DView = false;
}

// Replaces setArea() - now fed the coordinates of the room chosen as the
// view center in the area given from the set operation in the 2D Map
void GLWidget::setViewCenter(int areaId, int xPos, int yPos, int zPos)
{
    mAID = areaId;
    mShiftMode = true;
    mOx = xPos;
    mOy = yPos;
    mOz = zPos;
    update();
}

void GLWidget::paintGL()
{
    if (!mpMap) {
        return;
    }
    float px, py, pz;
    if (mRID != mpMap->mRoomIdHash.value(mpMap->mProfileName) && mShiftMode) {
        mShiftMode = false;
    }

    int ox, oy, oz;
    if (!mShiftMode) {
        mRID = mpMap->mRoomIdHash.value(mpMap->mProfileName);
        TRoom* pRID = mpMap->mpRoomDB->getRoom(mRID);
        if (!pRID) {
            glClearDepth(1.0);
            glDepthFunc(GL_LESS);
            glClearColor(0.0, 0.0, 0.0, 1.0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            QPainter painter(this);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
            painter.setPen(QColorConstants::White);
#else
            painter.setPen(Qt::white);
#endif
            painter.setFont(QFont("Bitstream Vera Sans Mono", 30));
            painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);

            QString message;
            if (mpMap->mpRoomDB) {
                if (mpMap->mpRoomDB->isEmpty()) {
                    message = tr("No rooms in the map - load another one, or start mapping from scratch to begin.");
                } else {
                    message = tr("You have a map loaded (%n room(s)), but Mudlet does not know where you are at the moment.", "", mpMap->mpRoomDB->size());
                }
            } else {
                message = tr("You do not have a map yet - load one, or start mapping from scratch to begin.");
            }
            painter.drawText(0, 0, (width() -1), (height() -1), Qt::AlignCenter | Qt::TextWordWrap, message);
            painter.end();

            glLoadIdentity();
            glFlush();
            return;
        }
        mAID = pRID->getArea();
        ox = pRID->x;
        oy = pRID->y;
        oz = pRID->z;
        mOx = ox;
        mOy = oy;
        mOz = oz;

    } else {
        ox = mOx;
        oy = mOy;
        oz = mOz;
    }
    px = static_cast<float>(ox); //mpMap->rooms[mpMap->mRoomId]->x);
    py = static_cast<float>(oy); //mpMap->rooms[mpMap->mRoomId]->y);
    pz = static_cast<float>(oz); //mpMap->rooms[mpMap->mRoomId]->z);
    TArea* pArea = mpMap->mpRoomDB->getArea(mAID);
    if (!pArea) {
        return;
    }
    if (pArea->gridMode) {
        xRot = 0.0;
        yRot = 0.0;
        zRot = 15.0;
    }
    zmax = static_cast<float>(pArea->max_z);
    zmin = static_cast<float>(pArea->min_z);
    float zPlane;
    glEnable(GL_CULL_FACE);
    glClearDepth(1.0);
    glDepthFunc(GL_LESS);

    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    GLfloat diffuseLight[] = {0.507, 0.507, 0.507, 1.0};
    GLfloat diffuseLight2[] = {0.501, 0.901, 0.501, 1.0};
    GLfloat ambientLight[] = {0.403, 0.403, 0.403, 1.0};
    GLfloat ambientLight2[] = {0.4501, 0.4501, 0.4501, 1.0};

    //GLfloat specularLight[] = {.01, .01, .01, 1.};//TODO: for me-sphere
    GLfloat light0Pos[] = {5000.0, 4000.0, 1000.0, 0};
    GLfloat light1Pos[] = {5000.0, 1000.0, 1000.0, 0};
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseLight);
    glLightfv(GL_LIGHT0, GL_POSITION, light0Pos);
    glLightfv(GL_LIGHT1, GL_AMBIENT, ambientLight2);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, diffuseLight2);
    glLightfv(GL_LIGHT1, GL_POSITION, light1Pos);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
    glLightfv(GL_LIGHT0, GL_POSITION, light0Pos);
    glLightfv(GL_LIGHT1, GL_POSITION, light1Pos);
    glBlendFunc(GL_SRC_ALPHA, GL_SRC_COLOR); //GL_ONE_MINUS_SRC_ALPHA);
    glLoadIdentity();

    glDisable(GL_FOG);
    glEnable(GL_BLEND);
    glEnable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LIGHT0);
    //glEnable(GL_LIGHT1);

    if (zRot <= 0) {
        zPlane = zmax;
    } else {
        zPlane = zmin;
    }

    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_LINE_STIPPLE);
    glLineWidth(1.0);
    int quads = 0;
    int verts = 0;
    float planeColor2[][4] = {{0.9, 0.5, 0.0, 1.0},
                               {165.0 / 255.0, 102.0 / 255.0, 167.0 / 255.0, 1.0},
                               {170.0 / 255.0, 10.0 / 255.0, 127.0 / 255.0, 1.0},
                               {203.0 / 255.0, 135.0 / 255.0, 101.0 / 255.0, 1.0},
                               {154.0 / 255.0, 154.0 / 255.0, 115.0 / 255.0, 1.0},
                               {107.0 / 255.0, 154.0 / 255.0, 100.0 / 255.0, 1.0},
                               {154.0 / 255.0, 184.0 / 255.0, 111.0 / 255.0, 1.0},
                               {67.0 / 255.0, 154.0 / 255.0, 148.0 / 255.0, 1.0},
                               {154.0 / 255.0, 118.0 / 255.0, 151.0 / 255.0, 1.0},
                               {208.0 / 255.0, 213.0 / 255.0, 164.0 / 255.0, 1.0},
                               {213.0 / 255.0, 169.0 / 255.0, 158.0 / 255.0, 1.0},
                               {139.0 / 255.0, 209.0 / 255.0, 0, 1.0},
                               {163.0 / 255.0, 209.0 / 255.0, 202.0 / 255.0, 1.0},
                               {158.0 / 255.0, 156.0 / 255.0, 209.0 / 255.0, 1.0},
                               {209.0 / 255.0, 144.0 / 255.0, 162.0 / 255.0, 1.0},
                               {209.0 / 255.0, 183.0 / 255.0, 78.0 / 255.0, 1.0},
                               {111.0 / 255.0, 209.0 / 255.0, 88.0 / 255.0, 1.0},
                               {95.0 / 255.0, 120.0 / 255.0, 209.0 / 255.0, 1.0},
                               {31.0 / 255.0, 209.0 / 255.0, 126.0 / 255.0, 1.0},
                               {1.0, 170.0 / 255.0, 1.0, 1.0},
                               {158.0 / 255.0, 105.0 / 255.0, 158.0 / 255.0, 1.0},
                               {68.0 / 255.0, 189.0 / 255.0, 189.0 / 255.0, 1.0},
                               {0.1, 0.69, 0.49, 1.0},
                               {0.0, 0.15, 1.0, 1.0},
                               {0.12, 0.02, 0.20, 1.0},
                               {0.0, 0.3, 0.1, 1.0}};


    float planeColor[][4] = {{0.5, 0.6, 0.5, 0.2},
                              {0.233, 0.498, 0.113, 0.2},
                              {0.666, 0.333, 0.498, 0.2},
                              {0.5, 0.333, 0.666, 0.2},
                              {0.69, 0.458, 0.0, 0.2},
                              {0.333, 0.0, 0.49, 0.2},
                              {133.0 / 255.0, 65.0 / 255.0, 98.0 / 255.0, 0.2},
                              {0.3, 0.3, 0.0, 0.2},
                              {0.6, 0.2, 0.6, 0.2},
                              {0.6, 0.6, 0.2, 0.2},
                              {0.4, 0.1, 0.4, 0.2},
                              {0.4, 0.4, 0.1, 0.2},
                              {0.3, 0.1, 0.3, 0.2},
                              {0.3, 0.3, 0.1, 0.2},
                              {0.2, 0.1, 0.2, 0.2},
                              {0.2, 0.2, 0.1, 0.2},
                              {0.24, 0.1, 0.5, 0.2},
                              {0.1, 0.1, 0.0, 0.2},
                              {0.54, 0.6, 0.2, 0.2},
                              {0.2, 0.2, 0.5, 0.2},
                              {0.6, 0.6, 0.2, 0.2},
                              {0.6, 0.4, 0.6, 0.2},
                              {0.4, 0.4, 0.1, 0.2},
                              {0.4, 0.2, 0.4, 0.2},
                              {0.2, 0.2, 0.0, 0.2},
                              {0.2, 0.1, 0.3, 0.2}};

    while (true) {
        if (zRot <= 0) {
            if (zPlane < zmin) {
                break;
            }
        } else {
            if (zPlane > zmax) {
                break;
            }
        }
        QSetIterator<int> itRoom(pArea->getAreaRooms());
        while (itRoom.hasNext()) {
            TRoom* pR = mpMap->mpRoomDB->getRoom(itRoom.next());
            if (!pR) {
                continue;
            }
            auto rx = static_cast<float>(pR->x);
            auto ry = static_cast<float>(pR->y);
            auto rz = static_cast<float>(pR->z);
            if (rz != zPlane) {
                continue;
            }
            if (rz > pz) {
                if (abs(rz - pz) > mShowTopLevels) {
                    continue;
                }
            }
            if (rz < pz) {
                if (abs(rz - pz) > mShowBottomLevels) {
                    continue;
                }
            }
            QList<int> exitList;
            exitList.push_back(pR->getNorth());
            exitList.push_back(pR->getNorthwest());
            exitList.push_back(pR->getEast());
            exitList.push_back(pR->getSoutheast());
            exitList.push_back(pR->getSouth());
            exitList.push_back(pR->getSouthwest());
            exitList.push_back(pR->getWest());
            exitList.push_back(pR->getNorthwest());
            exitList.push_back(pR->getUp());
            exitList.push_back(pR->getDown());
            int e = pR->z;
            const int ef = abs(e % 26);
            glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, planeColor[ef]);
            glMateriali(GL_FRONT, GL_SHININESS, 1);
            glDisable(GL_DEPTH_TEST);
            if (rz <= pz) {
                if ((rz == pz) && (rx == px) && (ry == py)) {
                    glDisable(GL_BLEND);
                    glEnable(GL_LIGHTING);
                    float mc3[] = {1.0f, 0.0f, 0.0f, 1.0f};
                    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mc3);
                    glMateriali(GL_FRONT, GL_SHININESS, 1);
                    glColor4f(1.0, 0.0, 0.0, 1.0);
                } else {
                    glDisable(GL_BLEND);
                    glEnable(GL_LIGHTING);
                    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, planeColor[ef]);
                    glMateriali(GL_FRONT, GL_SHININESS, 1);
                    glColor4f(0.3, 0.3, 0.3, 1.0); /*planeColor[ef][0],
                              planeColor[ef][1],
                              planeColor[ef][2],
                              planeColor[ef][3]);*/
                }
                for (int k : exitList) {
                    bool areaExit = false;
                    if (k == -1) {
                        continue;
                    }
                    TRoom* pExit = mpMap->mpRoomDB->getRoom(k);
                    if (!pExit) {
                        continue;
                    }
                    if (pExit->getArea() != mAID) {
                        areaExit = true;
                    } else {
                        areaExit = false;
                    }
                    auto ex = static_cast<float>(pExit->x);
                    auto ey = static_cast<float>(pExit->y);
                    auto ez = static_cast<float>(pExit->z);
                    QVector3D p1(ex, ey, ez);
                    QVector3D p2(rx, ry, rz);
                    glLoadIdentity();
                    gluLookAt(px * 0.1 + xRot, py * 0.1 + yRot, pz * 0.1 + zRot, px * 0.1, py * 0.1, pz * 0.1, 0.0, 1.0, 0.0);
                    glScalef(0.1, 0.1, 0.1);
                    // if (areaExit) {
                    //    glLineWidth(1); //1/mScale+2);
                    // } else {
                        glLineWidth(1); //1/mScale);
                    // }
                    if (k == mRID || ((rz == pz) && (rx == px) && (ry == py))) {
                        glDisable(GL_BLEND);
                        glEnable(GL_LIGHTING);
                        float mc3[] = {1.0f, 0.0f, 0.0f, 1.0f};
                        glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mc3);
                        glMateriali(GL_FRONT, GL_SHININESS, 1);
                        glColor4f(1.0, 0.0, 0.0, 1.0);
                    } else {
                        glDisable(GL_BLEND);
                        glEnable(GL_LIGHTING);
                        glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, planeColor[ef]);
                        glMateriali(GL_FRONT, GL_SHININESS, 1);
                        glColor4f(0.3, 0.3, 0.3, 1.0); //planeColor[ef][0],
                                                       //                                  planeColor[ef][1],
                                                       //                                  planeColor[ef][2],
                                                       //                                  planeColor[ef][3]);
                    }
                    glBegin(GL_LINES);
                    if (!areaExit) {
                        glVertex3f(p1.x(), p1.y(), p1.z());
                    } else {
                        if (pR->getNorth() == k) {
                            glVertex3f(p2.x(), p2.y() + 1, p2.z());
                        } else if (pR->getSouth() == k) {
                            glVertex3f(p2.x(), p2.y() - 1, p2.z());
                        } else if (pR->getWest() == k) {
                            glVertex3f(p2.x() - 1, p2.y(), p2.z());
                        } else if (pR->getEast() == k) {
                            glVertex3f(p2.x() + 1, p2.y(), p2.z());
                        } else if (pR->getSouthwest() == k) {
                            glVertex3f(p2.x() - 1, p2.y() - 1, p2.z());
                        } else if (pR->getSoutheast() == k) {
                            glVertex3f(p2.x() + 1, p2.y() - 1, p2.z());
                        } else if (pR->getNortheast() == k) {
                            glVertex3f(p2.x() + 1, p2.y() + 1, p2.z());
                        } else if (pR->getNorthwest() == k) {
                            glVertex3f(p2.x() - 1, p2.y() + 1, p2.z());
                        } else if (pR->getUp() == k) {
                            glVertex3f(p2.x(), p2.y(), p2.z() + 1);
                        } else if (pR->getDown() == k) {
                            glVertex3f(p2.x(), p2.y(), p2.z() - 1);
                        }
                    }
                    glVertex3f(p2.x(), p2.y(), p2.z());
                    glEnd();
                    verts++;
                    if (areaExit) {
                        glDisable(GL_BLEND);
                        glEnable(GL_LIGHTING);
                        glDisable(GL_LIGHT1);
                        float mc4[] = {85.0 / 255.0, 170.0 / 255.0, 0.0 / 255.0, 1.0};
                        glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mc4);
                        glMateriali(GL_FRONT, GL_SHININESS, 1);
                        glColor4f(85.0 / 255.0, 170.0 / 255.0, 0.0 / 255.0, 1.0);
                        glLoadIdentity();
                        gluLookAt(px * 0.1 + xRot, py * 0.1 + yRot, pz * 0.1 + zRot, px * 0.1, py * 0.1, pz * 0.1, 0.0, 1.0, 0.0);
                        glScalef(0.1, 0.1, 0.1);
                        if (pR->getNorth() == k) {
                            glTranslatef(p2.x(), p2.y() + 1, p2.z());
                        } else if (pR->getSouth() == k) {
                            glTranslatef(p2.x(), p2.y() - 1, p2.z());
                        } else if (pR->getWest() == k) {
                            glTranslatef(p2.x() - 1, p2.y(), p2.z());
                        } else if (pR->getEast() == k) {
                            glTranslatef(p2.x() + 1, p2.y(), p2.z());
                        } else if (pR->getSouthwest() == k) {
                            glTranslatef(p2.x() - 1, p2.y() - 1, p2.z());
                        } else if (pR->getSoutheast() == k) {
                            glTranslatef(p2.x() + 1, p2.y() - 1, p2.z());
                        } else if (pR->getNortheast() == k) {
                            glTranslatef(p2.x() + 1, p2.y() + 1, p2.z());
                        } else if (pR->getNorthwest() == k) {
                            glTranslatef(p2.x() - 1, p2.y() + 1, p2.z());
                        } else if (pR->getUp() == k) {
                            glTranslatef(p2.x(), p2.y(), p2.z() + 1);
                        } else if (pR->getDown() == k) {
                            glTranslatef(p2.x(), p2.y(), p2.z() - 1);
                        }

                        float mc6[] = {85.0 / 255.0, 170.0 / 255.0, 0.0 / 255.0, 0.0};
                        glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mc6);
                        glMateriali(GL_FRONT, GL_SHININESS, 96);

                        glLoadName(k);
                        quads++;
                        glBegin(GL_QUADS);
                        glNormal3f(0.57735, -0.57735, 0.57735);
                        glVertex3f(1.0 / scale, -1.0 / scale, 1.0 / scale);
                        glNormal3f(-0.57735, -0.57735, 0.57735);
                        glVertex3f(-1.0 / scale, -1.0 / scale, 1.0 / scale);
                        glNormal3f(-0.57735, -0.57735, -0.57735);
                        glVertex3f(-1.0 / scale, -1.0 / scale, -1.0 / scale);
                        glNormal3f(0.57735, -0.57735, -0.57735);
                        glVertex3f(1.0 / scale, -1.0 / scale, -1.0 / scale);

                        glNormal3f(0.57735, 0.57735, 0.57735);
                        glVertex3f(1.0 / scale, 1.0 / scale, 1.0 / scale);
                        glNormal3f(-0.57735, 0.57735, 0.57735);
                        glVertex3f(-1.0 / scale, 1.0 / scale, 1.0 / scale);
                        glNormal3f(-0.57735, -0.57735, 0.57735);
                        glVertex3f(-1.0 / scale, -1.0 / scale, 1.0 / scale);
                        glNormal3f(0.57735, -0.57735, 0.57735);
                        glVertex3f(1.0 / scale, -1.0 / scale, 1.0 / scale);

                        glNormal3f(-0.57735, 0.57735, -0.57735);
                        glVertex3f(-1.0 / scale, 1.0 / scale, -1.0 / scale);
                        glNormal3f(0.57735, 0.57735, -0.57735);
                        glVertex3f(1.0 / scale, 1.0 / scale, -1.0 / scale);
                        glNormal3f(0.57735, -0.57735, -0.57735);
                        glVertex3f(1.0 / scale, -1.0 / scale, -1.0 / scale);
                        glNormal3f(-0.57735, -0.57735, -0.57735);
                        glVertex3f(-1.0 / scale, -1.0 / scale, -1.0 / scale);

                        glNormal3f(0.57735, 0.57735, -0.57735);
                        glVertex3f(1.0 / scale, 1.0 / scale, -1.0 / scale);
                        glNormal3f(0.57735, 0.57735, 0.57735);
                        glVertex3f(1.0 / scale, 1.0 / scale, 1.0 / scale);
                        glNormal3f(0.57735, -0.57735, 0.57735);
                        glVertex3f(1.0 / scale, -1.0 / scale, 1.0 / scale);
                        glNormal3f(0.57735, -0.57735, -0.57735);
                        glVertex3f(1.0 / scale, -1.0 / scale, -1.0 / scale);

                        glNormal3f(-0.57735, 0.57735, 0.57735);
                        glVertex3f(-1.0 / scale, 1.0 / scale, 1.0 / scale);
                        glNormal3f(-0.57735, 0.57735, -0.57735);
                        glVertex3f(-1.0 / scale, 1.0 / scale, -1.0 / scale);
                        glNormal3f(-0.57735, -0.57735, -0.57735);
                        glVertex3f(-1.0 / scale, -1.0 / scale, -1.0 / scale);
                        glNormal3f(-0.57735, -0.57735, 0.57735);
                        glVertex3f(-1.0 / scale, -1.0 / scale, 1.0 / scale);

                        glNormal3f(0.57735, 0.57735, -0.57735);
                        glVertex3f(1.0 / scale, 1.0 / scale, -1.0 / scale);
                        glNormal3f(-0.57735, 0.57735, -0.57735);
                        glVertex3f(-1.0 / scale, 1.0 / scale, -1.0 / scale);
                        glNormal3f(-0.57735, 0.57735, 0.57735);
                        glVertex3f(-1.0 / scale, 1.0 / scale, 1.0 / scale);
                        glNormal3f(0.57735, 0.57735, 0.57735);
                        glVertex3f(1.0 / scale, 1.0 / scale, 1.0 / scale);
                        glEnd();

                        //on top
                        float mc3[] = {0.2, 0.2, 0.6, 1.0};
                        int env = pExit->environment;
                        if (mpMap->mEnvColors.contains(env)) {
                            env = mpMap->mEnvColors[env];
                        } else {
                            if (!mpMap->mCustomEnvColors.contains(env)) {
                                env = 1;
                            }
                        }
                        switch (env) {
                        case 1:
                            glColor4ub(128, 50, 50, 200);
                            mc3[0] = 128.0 / 255.0;
                            mc3[1] = 0.0 / 255.0;
                            mc3[2] = 0.0 / 255.0;
                            mc3[3] = 0.2;
                            break;

                        case 2:
                            glColor4ub(128, 128, 50, 200);
                            mc3[0] = 0.0 / 255.0;
                            mc3[1] = 128.0 / 255.0;
                            mc3[2] = 0.0 / 255.0;
                            mc3[3] = 0.2;
                            break;
                        case 3:
                            glColor4ub(50, 128, 50, 200);
                            mc3[0] = 128.0 / 255.0;
                            mc3[1] = 128.0 / 255.0;
                            mc3[2] = 0.0 / 255.0;
                            mc3[3] = 0.2;
                            break;

                        case 4:
                            glColor4ub(50, 50, 128, 200);
                            mc3[0] = 0.0 / 255.0;
                            mc3[1] = 0.0 / 255.0;
                            mc3[2] = 128.0 / 255.0;
                            mc3[3] = 0.2;
                            break;

                        case 5:
                            glColor4ub(128, 50, 128, 200);
                            mc3[0] = 128.0 / 255.0;
                            mc3[1] = 128.0 / 255.0;
                            mc3[2] = 0.0 / 255.0;
                            mc3[3] = 0.2;
                            break;
                        case 6:
                            glColor4ub(50, 128, 128, 200);
                            mc3[0] = 0.0 / 255.0;
                            mc3[1] = 128.0 / 255.0;
                            mc3[2] = 128.0 / 255.0;
                            mc3[3] = 0.2;
                            break;
                        case 7:
                            glColor4ub(52, 38, 78, 200);
                            mc3[0] = 128.0 / 255.0;
                            mc3[1] = 128.0 / 255.0;
                            mc3[2] = 128.0 / 255.0;
                            mc3[3] = 0.2;
                            break;
                        case 8:
                            glColor4ub(65, 55, 35, 200);
                            mc3[0] = 55.0 / 255.0;
                            mc3[1] = 55.0 / 255.0;
                            mc3[2] = 55.0 / 255.0;
                            mc3[3] = 0.2;
                            break;

                        case 9:
                            glColor4ub(175, 50, 50, 200);
                            mc3[0] = 255.0 / 255.0;
                            mc3[1] = 50.0 / 255.0;
                            mc3[2] = 50.0 / 255.0;
                            mc3[3] = 0.2;
                            break;

                        case 10:
                            glColor4ub(255, 255, 50, 200);
                            mc3[0] = 50.0 / 255.0;
                            mc3[1] = 255.0 / 255.0;
                            mc3[2] = 50.0 / 255.0;
                            mc3[3] = 0.2;
                            break;
                        case 11:
                            glColor4ub(50, 175, 175, 200);
                            mc3[0] = 255.0 / 255.0;
                            mc3[1] = 255.0 / 255.0;
                            mc3[2] = 50.0 / 255.0;
                            mc3[3] = 0.2;
                            break;

                        case 12:
                            glColor4ub(175, 175, 50, 200);
                            mc3[0] = 50.0 / 255.0;
                            mc3[1] = 50.0 / 255.0;
                            mc3[2] = 255.0 / 255.0;
                            mc3[3] = 0.2;
                            break;

                        case 13:
                            glColor4ub(175, 50, 175, 200);
                            mc3[0] = 255.0 / 255.0;
                            mc3[1] = 50.0 / 255.0;
                            mc3[2] = 255.0 / 255.0;
                            mc3[3] = 0.2;
                            break;
                        case 14:
                            glColor4ub(50, 175, 50, 200);
                            mc3[0] = 50.0 / 255.0;
                            mc3[1] = 255.0 / 255.0;
                            mc3[2] = 255.0 / 255.0;
                            mc3[3] = 0.2;
                            break;
                        case 15:
                            glColor4ub(50, 50, 175, 200);
                            mc3[0] = 255.0 / 255.0;
                            mc3[1] = 255.0 / 255.0;
                            mc3[2] = 255.0 / 255.0;
                            mc3[3] = 0.2;
                            break;
                        default: //user defined room color
                            if (!mpMap->mCustomEnvColors.contains(env)) {
                                if (16 < env && env < 232)
                                {
                                    quint8 base = env - 16;
                                    quint8 r = base / 36;
                                    quint8 g = (base - (r * 36)) / 6;
                                    quint8 b = (base - (r * 36)) - (g * 6);

                                    r = r == 0 ? 0 : (r - 1) * 40 + 95;
                                    g = g == 0 ? 0 : (g - 1) * 40 + 95;
                                    b = b == 0 ? 0 : (b - 1) * 40 + 95;
                                    glColor4ub(r, g, b, 200);
                                    mc3[0] = r / 255.0;
                                    mc3[1] = g / 255.0;
                                    mc3[2] = b / 255.0;
                                    mc3[3] = 0.2;
                                } else if (231 < env && env < 256) {
                                    quint8 k = ((env - 232) * 10) + 8;
                                    glColor4ub(k, k, k, 200);
                                    mc3[0] = k / 255.0;
                                    mc3[1] = k / 255.0;
                                    mc3[2] = k / 255.0;
                                    mc3[3] = 0.2;
                                }
                                break;
                            }
                            QColor& _c = mpMap->mCustomEnvColors[env];
                            glColor4ub(_c.red(), _c.green(), _c.blue(), 25);
                            mc3[0] = _c.redF();
                            mc3[1] = _c.greenF();
                            mc3[2] = _c.blueF();
                            mc3[3] = 0.2;
                        }
                        glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mc3);
                        glMateriali(GL_FRONT, GL_SHININESS, 1);
                        glDisable(GL_DEPTH_TEST);
                        glLoadIdentity();
                        gluLookAt(px * 0.1 + xRot, py * 0.1 + yRot, pz * 0.1 + zRot, px * 0.1, py * 0.1, pz * 0.1, 0.0, 1.0, 0.0);
                        glScalef(0.05, 0.05, 0.020);
                        if (pR->getNorth() == k) {
                            glTranslatef(2 * p2.x(), 2 * (p2.y() + 1), 5.0 * (p2.z() + 0.25));
                        } else if (pR->getSouth() == k) {
                            glTranslatef(2 * p2.x(), 2 * (p2.y() - 1), 5.0 * (p2.z() + 0.25));
                        } else if (pR->getWest() == k) {
                            glTranslatef(2 * (p2.x() - 1), 2 * p2.y(), 5.0 * (p2.z() + 0.25));
                        } else if (pR->getEast() == k) {
                            glTranslatef(2 * (p2.x() + 1), 2 * p2.y(), 5.0 * (p2.z() + 0.25));
                        } else if (pR->getSouthwest() == k) {
                            glTranslatef(2 * (p2.x() - 1), 2 * (p2.y() - 1), 5.0 * (p2.z() + 0.25));
                        } else if (pR->getSoutheast() == k) {
                            glTranslatef(2 * (p2.x() + 1), 2 * (p2.y() - 1), 5.0 * (p2.z() + 0.25));
                        } else if (pR->getNortheast() == k) {
                            glTranslatef(2 * (p2.x() + 1), 2 * (p2.y() + 1), 5.0 * (p2.z() + 0.25));
                        } else if (pR->getNorthwest() == k) {
                            glTranslatef(2 * (p2.x() - 1), 2 * (p2.y() + 1), 5.0 * (p2.z() + 0.25));
                        } else if (pR->getUp() == k) {
                            glTranslatef(2 * p2.x(), 2 * p2.y(), 5.0 * (p2.z() + 1 + 0.25));
                        } else if (pR->getDown() == k) {
                            glTranslatef(2 * p2.x(), 2 * p2.y(), 5.0 * (p2.z() - 1 + 0.25));
                        }

                        glBegin(GL_QUADS);
                        glNormal3f(0.57735, -0.57735, 0.57735);
                        glVertex3f(1.0 / scale, -1.0 / scale, 1.0 / scale);
                        glNormal3f(-0.57735, -0.57735, 0.57735);
                        glVertex3f(-1.0 / scale, -1.0 / scale, 1.0 / scale);
                        glNormal3f(-0.57735, -0.57735, -0.57735);
                        glVertex3f(-1.0 / scale, -1.0 / scale, -1.0 / scale);
                        glNormal3f(0.57735, -0.57735, -0.57735);
                        glVertex3f(1.0 / scale, -1.0 / scale, -1.0 / scale);

                        glNormal3f(0.57735, 0.57735, 0.57735);
                        glVertex3f(1.0 / scale, 1.0 / scale, 1.0 / scale);
                        glNormal3f(-0.57735, 0.57735, 0.57735);
                        glVertex3f(-1.0 / scale, 1.0 / scale, 1.0 / scale);
                        glNormal3f(-0.57735, -0.57735, 0.57735);
                        glVertex3f(-1.0 / scale, -1.0 / scale, 1.0 / scale);
                        glNormal3f(0.57735, -0.57735, 0.57735);
                        glVertex3f(1.0 / scale, -1.0 / scale, 1.0 / scale);

                        glNormal3f(-0.57735, 0.57735, -0.57735);
                        glVertex3f(-1.0 / scale, 1.0 / scale, -1.0 / scale);
                        glNormal3f(0.57735, 0.57735, -0.57735);
                        glVertex3f(1.0 / scale, 1.0 / scale, -1.0 / scale);
                        glNormal3f(0.57735, -0.57735, -0.57735);
                        glVertex3f(1.0 / scale, -1.0 / scale, -1.0 / scale);
                        glNormal3f(-0.57735, -0.57735, -0.57735);
                        glVertex3f(-1.0 / scale, -1.0 / scale, -1.0 / scale);

                        glNormal3f(0.57735, 0.57735, -0.57735);
                        glVertex3f(1.0 / scale, 1.0 / scale, -1.0 / scale);
                        glNormal3f(0.57735, 0.57735, 0.57735);
                        glVertex3f(1.0 / scale, 1.0 / scale, 1.0 / scale);
                        glNormal3f(0.57735, -0.57735, 0.57735);
                        glVertex3f(1.0 / scale, -1.0 / scale, 1.0 / scale);
                        glNormal3f(0.57735, -0.57735, -0.57735);
                        glVertex3f(1.0 / scale, -1.0 / scale, -1.0 / scale);

                        glNormal3f(-0.57735, 0.57735, 0.57735);
                        glVertex3f(-1.0 / scale, 1.0 / scale, 1.0 / scale);
                        glNormal3f(-0.57735, 0.57735, -0.57735);
                        glVertex3f(-1.0 / scale, 1.0 / scale, -1.0 / scale);
                        glNormal3f(-0.57735, -0.57735, -0.57735);
                        glVertex3f(-1.0 / scale, -1.0 / scale, -1.0 / scale);
                        glNormal3f(-0.57735, -0.57735, 0.57735);
                        glVertex3f(-1.0 / scale, -1.0 / scale, 1.0 / scale);

                        glNormal3f(0.57735, 0.57735, -0.57735);
                        glVertex3f(1.0 / scale, 1.0 / scale, -1.0 / scale);
                        glNormal3f(-0.57735, 0.57735, -0.57735);
                        glVertex3f(-1.0 / scale, 1.0 / scale, -1.0 / scale);
                        glNormal3f(-0.57735, 0.57735, 0.57735);
                        glVertex3f(-1.0 / scale, 1.0 / scale, 1.0 / scale);
                        glNormal3f(0.57735, 0.57735, 0.57735);
                        glVertex3f(1.0 / scale, 1.0 / scale, 1.0 / scale);
                        glEnd();
                    }
                }
            } else {
                for (int k : exitList) {
                    bool areaExit = false;
                    if (k == -1) {
                        continue;
                    }
                    TRoom* pExit = mpMap->mpRoomDB->getRoom(k);
                    if (!pExit) {
                        continue;
                    }
                    if (pExit->getArea() != mAID) {
                        areaExit = true;
                    } else {
                        areaExit = false;
                    }

                    auto ex = static_cast<float>(pExit->x);
                    auto ey = static_cast<float>(pExit->y);
                    auto ez = static_cast<float>(pExit->z);
                    QVector3D p1(ex, ey, ez);
                    QVector3D p2(rx, ry, rz);
                    glLoadIdentity();
                    gluLookAt(px * 0.1 + xRot, py * 0.1 + yRot, pz * 0.1 + zRot, px * 0.1, py * 0.1, pz * 0.1, 0.0, 1.0, 0.0);
                    glScalef(0.1, 0.1, 0.1);
                    // if (areaExit) {
                    //    glLineWidth(1); //1/mScale+2);
                    // } else {
                        glLineWidth(1); //1/mScale);
                    // }
                    if (k == mRID || ((rz == pz) && (rx == px) && (ry == py))) {
                        glDisable(GL_BLEND);
                        glEnable(GL_LIGHTING);
                        float mc3[] = {1.0f, 0.0f, 0.0f, 1.0f};
                        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, mc3);
                        glMateriali(GL_FRONT_AND_BACK, GL_SHININESS, 1);
                        glColor4f(1.0, 0.0, 0.0, 1.0);
                    } else {
                        glEnable(GL_BLEND);
                        glEnable(GL_LIGHTING);
                        glEnable(GL_LIGHT1);
                        glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, planeColor2[ef]);
                        glMateriali(GL_FRONT, GL_SHININESS, 1); //gut:36
                        glColor4f(0.3, 0.3, 0.3, 1.0); /*planeColor2[ef][0],
                                  planeColor2[ef][1],
                                  planeColor2[ef][2],
                                  planeColor2[ef][3])*/
                        ;
                    }
                    glBegin(GL_LINES);
                    if (!areaExit) {
                        glVertex3f(p1.x(), p1.y(), p1.z());
                    } else {
                        if (pR->getNorth() == k) {
                            glVertex3f(p2.x(), p2.y() + 1, p2.z());
                        } else if (pR->getSouth() == k) {
                            glVertex3f(p2.x(), p2.y() - 1, p2.z());
                        } else if (pR->getWest() == k) {
                            glVertex3f(p2.x() - 1, p2.y(), p2.z());
                        } else if (pR->getEast() == k) {
                            glVertex3f(p2.x() + 1, p2.y(), p2.z());
                        } else if (pR->getSouthwest() == k) {
                            glVertex3f(p2.x() - 1, p2.y() - 1, p2.z());
                        } else if (pR->getSoutheast() == k) {
                            glVertex3f(p2.x() + 1, p2.y() - 1, p2.z());
                        } else if (pR->getNortheast() == k) {
                            glVertex3f(p2.x() + 1, p2.y() - 1, p2.z());
                        } else if (pR->getNorthwest() == k) {
                            glVertex3f(p2.x() - 1, p2.y() + 1, p2.z());
                        } else if (pR->getUp() == k) {
                            glVertex3f(p2.x(), p2.y(), p2.z() + 1);
                        } else if (pR->getDown() == k) {
                            glVertex3f(p2.x(), p2.y(), p2.z() - 1);
                        }
                    }
                    glVertex3f(p2.x(), p2.y(), p2.z());
                    glEnd();
                    verts++;
                    if (areaExit) {
                        glDisable(GL_BLEND);
                        glEnable(GL_LIGHTING);
                        glDisable(GL_LIGHT1);
                        float mc4[] = {85.0 / 255.0, 170.0 / 255.0, 0.0 / 255.0, 1.0};
                        glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mc4);
                        glMateriali(GL_FRONT, GL_SHININESS, 1);
                        glColor4f(85.0 / 255.0, 170.0 / 255.0, 0.0 / 255.0, 1.0);
                        glLoadIdentity();
                        gluLookAt(px * 0.1 + xRot, py * 0.1 + yRot, pz * 0.1 + zRot, px * 0.1, py * 0.1, pz * 0.1, 0.0, 1.0, 0.0);
                        glScalef(0.1, 0.1, 0.1);
                        if (pR->getNorth() == k) {
                            glTranslatef(p2.x(), p2.y() + 1, p2.z());
                        } else if (pR->getSouth() == k) {
                            glTranslatef(p2.x(), p2.y() - 1, p2.z());
                        } else if (pR->getWest() == k) {
                            glTranslatef(p2.x() - 1, p2.y(), p2.z());
                        } else if (pR->getEast() == k) {
                            glTranslatef(p2.x() + 1, p2.y(), p2.z());
                        } else if (pR->getSouthwest() == k) {
                            glTranslatef(p2.x() - 1, p2.y() - 1, p2.z());
                        } else if (pR->getSoutheast() == k) {
                            glTranslatef(p2.x() + 1, p2.y() - 1, p2.z());
                        } else if (pR->getNortheast() == k) {
                            glTranslatef(p2.x() + 1, p2.y() + 1, p2.z());
                        } else if (pR->getNorthwest() == k) {
                            glTranslatef(p2.x() - 1, p2.y() + 1, p2.z());
                        } else if (pR->getUp() == k) {
                            glTranslatef(p2.x(), p2.y(), p2.z() + 1);
                        } else if (pR->getDown() == k) {
                            glTranslatef(p2.x(), p2.y(), p2.z() - 1);
                        }

                        glLoadName(k);
                        quads++;
                        glBegin(GL_QUADS);
                        glNormal3f(0.57735, -0.57735, 0.57735);
                        glVertex3f(1.0 / scale, -1.0 / scale, 1.0 / scale);
                        glNormal3f(-0.57735, -0.57735, 0.57735);
                        glVertex3f(-1.0 / scale, -1.0 / scale, 1.0 / scale);
                        glNormal3f(-0.57735, -0.57735, -0.57735);
                        glVertex3f(-1.0 / scale, -1.0 / scale, -1.0 / scale);
                        glNormal3f(0.57735, -0.57735, -0.57735);
                        glVertex3f(1.0 / scale, -1.0 / scale, -1.0 / scale);

                        glNormal3f(0.57735, 0.57735, 0.57735);
                        glVertex3f(1.0 / scale, 1.0 / scale, 1.0 / scale);
                        glNormal3f(-0.57735, 0.57735, 0.57735);
                        glVertex3f(-1.0 / scale, 1.0 / scale, 1.0 / scale);
                        glNormal3f(-0.57735, -0.57735, 0.57735);
                        glVertex3f(-1.0 / scale, -1.0 / scale, 1.0 / scale);
                        glNormal3f(0.57735, -0.57735, 0.57735);
                        glVertex3f(1.0 / scale, -1.0 / scale, 1.0 / scale);

                        glNormal3f(-0.57735, 0.57735, -0.57735);
                        glVertex3f(-1.0 / scale, 1.0 / scale, -1.0 / scale);
                        glNormal3f(0.57735, 0.57735, -0.57735);
                        glVertex3f(1.0 / scale, 1.0 / scale, -1.0 / scale);
                        glNormal3f(0.57735, -0.57735, -0.57735);
                        glVertex3f(1.0 / scale, -1.0 / scale, -1.0 / scale);
                        glNormal3f(-0.57735, -0.57735, -0.57735);
                        glVertex3f(-1.0 / scale, -1.0 / scale, -1.0 / scale);

                        glNormal3f(0.57735, 0.57735, -0.57735);
                        glVertex3f(1.0 / scale, 1.0 / scale, -1.0 / scale);
                        glNormal3f(0.57735, 0.57735, 0.57735);
                        glVertex3f(1.0 / scale, 1.0 / scale, 1.0 / scale);
                        glNormal3f(0.57735, -0.57735, 0.57735);
                        glVertex3f(1.0 / scale, -1.0 / scale, 1.0 / scale);
                        glNormal3f(0.57735, -0.57735, -0.57735);
                        glVertex3f(1.0 / scale, -1.0 / scale, -1.0 / scale);

                        glNormal3f(-0.57735, 0.57735, 0.57735);
                        glVertex3f(-1.0 / scale, 1.0 / scale, 1.0 / scale);
                        glNormal3f(-0.57735, 0.57735, -0.57735);
                        glVertex3f(-1.0 / scale, 1.0 / scale, -1.0 / scale);
                        glNormal3f(-0.57735, -0.57735, -0.57735);
                        glVertex3f(-1.0 / scale, -1.0 / scale, -1.0 / scale);
                        glNormal3f(-0.57735, -0.57735, 0.57735);
                        glVertex3f(-1.0 / scale, -1.0 / scale, 1.0 / scale);

                        glNormal3f(0.57735, 0.57735, -0.57735);
                        glVertex3f(1.0 / scale, 1.0 / scale, -1.0 / scale);
                        glNormal3f(-0.57735, 0.57735, -0.57735);
                        glVertex3f(-1.0 / scale, 1.0 / scale, -1.0 / scale);
                        glNormal3f(-0.57735, 0.57735, 0.57735);
                        glVertex3f(-1.0 / scale, 1.0 / scale, 1.0 / scale);
                        glNormal3f(0.57735, 0.57735, 0.57735);
                        glVertex3f(1.0 / scale, 1.0 / scale, 1.0 / scale);
                        glEnd();

                        //on top
                        float mc3[] = {0.2, 0.2, 0.6, 0.2};
                        int env = pExit->environment;
                        if (mpMap->mEnvColors.contains(env)) {
                            env = mpMap->mEnvColors[env];
                        } else {
                            if (!mpMap->mCustomEnvColors.contains(env)) {
                                env = 1;
                            }
                        }
                        switch (env) {
                        case 1:
                            glColor4ub(128, 50, 50, 2);
                            mc3[0] = 128.0 / 255.0;
                            mc3[1] = 0.0 / 255.0;
                            mc3[2] = 0.0 / 255.0;
                            mc3[3] = 0.2;
                            break;

                        case 2:
                            glColor4ub(128, 128, 50, 2);
                            mc3[0] = 0.0 / 255.0;
                            mc3[1] = 128.0 / 255.0;
                            mc3[2] = 0.0 / 255.0;
                            mc3[3] = 0.2;
                            break;
                        case 3:
                            glColor4ub(50, 128, 50, 2);
                            mc3[0] = 128.0 / 255.0;
                            mc3[1] = 128.0 / 255.0;
                            mc3[2] = 0.0 / 255.0;
                            mc3[3] = 0.2;
                            break;

                        case 4:
                            glColor4ub(50, 50, 128, 2);
                            mc3[0] = 0.0 / 255.0;
                            mc3[1] = 0.0 / 255.0;
                            mc3[2] = 128.0 / 255.0;
                            mc3[3] = 0.2;
                            break;

                        case 5:
                            glColor4ub(128, 50, 128, 2);
                            mc3[0] = 128.0 / 255.0;
                            mc3[1] = 128.0 / 255.0;
                            mc3[2] = 0.0 / 255.0;
                            mc3[3] = 0.2;
                            break;
                        case 6:
                            glColor4ub(50, 128, 128, 2);
                            mc3[0] = 0.0 / 255.0;
                            mc3[1] = 128.0 / 255.0;
                            mc3[2] = 128.0 / 255.0;
                            mc3[3] = 0.2;
                            break;
                        case 7:
                            glColor4ub(52, 38, 78, 2);
                            mc3[0] = 128.0 / 255.0;
                            mc3[1] = 128.0 / 255.0;
                            mc3[2] = 128.0 / 255.0;
                            mc3[3] = 0.2;
                            break;
                        case 8:
                            glColor4ub(65, 55, 35, 2);
                            mc3[0] = 55.0 / 255.0;
                            mc3[1] = 55.0 / 255.0;
                            mc3[2] = 55.0 / 255.0;
                            mc3[3] = 0.2;
                            break;

                        case 9:
                            glColor4ub(175, 50, 50, 2);
                            mc3[0] = 255.0 / 255.0;
                            mc3[1] = 50.0 / 255.0;
                            mc3[2] = 50.0 / 255.0;
                            mc3[3] = 0.2;
                            break;

                        case 10:
                            glColor4ub(255, 255, 50, 2);
                            mc3[0] = 50.0 / 255.0;
                            mc3[1] = 255.0 / 255.0;
                            mc3[2] = 50.0 / 255.0;
                            mc3[3] = 0.2;
                            break;
                        case 11:
                            glColor4ub(50, 175, 175, 2);
                            mc3[0] = 255.0 / 255.0;
                            mc3[1] = 255.0 / 255.0;
                            mc3[2] = 50.0 / 255.0;
                            mc3[3] = 0.2;
                            break;

                        case 12:
                            glColor4ub(175, 175, 50, 2);
                            mc3[0] = 50.0 / 255.0;
                            mc3[1] = 50.0 / 255.0;
                            mc3[2] = 255.0 / 255.0;
                            mc3[3] = 0.2;
                            break;

                        case 13:
                            glColor4ub(175, 50, 175, 2);
                            mc3[0] = 255.0 / 255.0;
                            mc3[1] = 50.0 / 255.0;
                            mc3[2] = 255.0 / 255.0;
                            mc3[3] = 0.2;
                            break;
                        case 14:
                            glColor4ub(50, 175, 50, 2);
                            mc3[0] = 50.0 / 255.0;
                            mc3[1] = 255.0 / 255.0;
                            mc3[2] = 255.0 / 255.0;
                            mc3[3] = 0.2;
                            break;
                        case 15:
                            glColor4ub(50, 50, 175, 2);
                            mc3[0] = 255.0 / 255.0;
                            mc3[1] = 255.0 / 255.0;
                            mc3[2] = 255.0 / 255.0;
                            mc3[3] = 0.2;
                            break;
                        default: //user defined room color
                            if (!mpMap->mCustomEnvColors.contains(env)) {
                                if (16 < env && env < 232)
                                {
                                    quint8 base = env - 16;
                                    quint8 r = base / 36;
                                    quint8 g = (base - (r * 36)) / 6;
                                    quint8 b = (base - (r * 36)) - (g * 6);

                                    r = r == 0 ? 0 : (r - 1) * 40 + 95;
                                    g = g == 0 ? 0 : (g - 1) * 40 + 95;
                                    b = b == 0 ? 0 : (b - 1) * 40 + 95;
                                    glColor4ub(r, g, b, 200);
                                    mc3[0] = r / 255.0;
                                    mc3[1] = g / 255.0;
                                    mc3[2] = b / 255.0;
                                    mc3[3] = 0.2;
                                } else if (231 < env && env < 256) {
                                    quint8 k = ((env - 232) * 10) + 8;
                                    glColor4ub(k, k, k, 200);
                                    mc3[0] = k / 255.0;
                                    mc3[1] = k / 255.0;
                                    mc3[2] = k / 255.0;
                                    mc3[3] = 0.2;
                                }
                                break;
                            }
                            QColor& _c = mpMap->mCustomEnvColors[env];
                            glColor4ub(_c.red(), _c.green(), _c.blue(), 255);
                            mc3[0] = _c.redF();
                            mc3[1] = _c.greenF();
                            mc3[2] = _c.blueF();
                            mc3[3] = 0.2;
                        }
                        glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mc3);
                        glMateriali(GL_FRONT, GL_SHININESS, 36);
                        glDisable(GL_DEPTH_TEST);
                        glLoadIdentity();
                        gluLookAt(px * 0.1 + xRot, py * 0.1 + yRot, pz * 0.1 + zRot, px * 0.1, py * 0.1, pz * 0.1, 0.0, 1.0, 0.0);
                        glScalef(0.05, 0.05, 0.020);
                        if (pR->getNorth() == k) {
                            glTranslatef(2 * p2.x(), 2 * (p2.y() + 1), 5.0 * (p2.z() + 0.25));
                        } else if (pR->getSouth() == k) {
                            glTranslatef(2 * p2.x(), 2 * (p2.y() - 1), 5.0 * (p2.z() + 0.25));
                        } else if (pR->getWest() == k) {
                            glTranslatef(2 * (p2.x() - 1), 2 * p2.y(), 5.0 * (p2.z() + 0.25));
                        } else if (pR->getEast() == k) {
                            glTranslatef(2 * (p2.x() + 1), 2 * p2.y(), 5.0 * (p2.z() + 0.25));
                        } else if (pR->getSouthwest() == k) {
                            glTranslatef(2 * (p2.x() - 1), 2 * (p2.y() - 1), 5.0 * (p2.z() + 0.25));
                        } else if (pR->getSoutheast() == k) {
                            glTranslatef(2 * (p2.x() + 1), 2 * (p2.y() - 1), 5.0 * (p2.z() + 0.25));
                        } else if (pR->getNortheast() == k) {
                            glTranslatef(2 * (p2.x() + 1), 2 * (p2.y() + 1), 5.0 * (p2.z() + 0.25));
                        } else if (pR->getNorthwest() == k) {
                            glTranslatef(2 * (p2.x() - 1), 2 * (p2.y() + 1), 5.0 * (p2.z() + 0.25));
                        } else if (pR->getUp() == k) {
                            glTranslatef(2 * p2.x(), 2 * p2.y(), 5.0 * (p2.z() + 1 + 0.25));
                        } else if (pR->getDown() == k) {
                            glTranslatef(2 * p2.x(), 2 * p2.y(), 5.0 * (p2.z() - 1 + 0.25));
                        }

                        glBegin(GL_QUADS);
                        glNormal3f(0.57735, -0.57735, 0.57735);
                        glVertex3f(1.0 / scale, -1.0 / scale, 1.0 / scale);
                        glNormal3f(-0.57735, -0.57735, 0.57735);
                        glVertex3f(-1.0 / scale, -1.0 / scale, 1.0 / scale);
                        glNormal3f(-0.57735, -0.57735, -0.57735);
                        glVertex3f(-1.0 / scale, -1.0 / scale, -1.0 / scale);
                        glNormal3f(0.57735, -0.57735, -0.57735);
                        glVertex3f(1.0 / scale, -1.0 / scale, -1.0 / scale);

                        glNormal3f(0.57735, 0.57735, 0.57735);
                        glVertex3f(1.0 / scale, 1.0 / scale, 1.0 / scale);
                        glNormal3f(-0.57735, 0.57735, 0.57735);
                        glVertex3f(-1.0 / scale, 1.0 / scale, 1.0 / scale);
                        glNormal3f(-0.57735, -0.57735, 0.57735);
                        glVertex3f(-1.0 / scale, -1.0 / scale, 1.0 / scale);
                        glNormal3f(0.57735, -0.57735, 0.57735);
                        glVertex3f(1.0 / scale, -1.0 / scale, 1.0 / scale);

                        glNormal3f(-0.57735, 0.57735, -0.57735);
                        glVertex3f(-1.0 / scale, 1.0 / scale, -1.0 / scale);
                        glNormal3f(0.57735, 0.57735, -0.57735);
                        glVertex3f(1.0 / scale, 1.0 / scale, -1.0 / scale);
                        glNormal3f(0.57735, -0.57735, -0.57735);
                        glVertex3f(1.0 / scale, -1.0 / scale, -1.0 / scale);
                        glNormal3f(-0.57735, -0.57735, -0.57735);
                        glVertex3f(-1.0 / scale, -1.0 / scale, -1.0 / scale);

                        glNormal3f(0.57735, 0.57735, -0.57735);
                        glVertex3f(1.0 / scale, 1.0 / scale, -1.0 / scale);
                        glNormal3f(0.57735, 0.57735, 0.57735);
                        glVertex3f(1.0 / scale, 1.0 / scale, 1.0 / scale);
                        glNormal3f(0.57735, -0.57735, 0.57735);
                        glVertex3f(1.0 / scale, -1.0 / scale, 1.0 / scale);
                        glNormal3f(0.57735, -0.57735, -0.57735);
                        glVertex3f(1.0 / scale, -1.0 / scale, -1.0 / scale);

                        glNormal3f(-0.57735, 0.57735, 0.57735);
                        glVertex3f(-1.0 / scale, 1.0 / scale, 1.0 / scale);
                        glNormal3f(-0.57735, 0.57735, -0.57735);
                        glVertex3f(-1.0 / scale, 1.0 / scale, -1.0 / scale);
                        glNormal3f(-0.57735, -0.57735, -0.57735);
                        glVertex3f(-1.0 / scale, -1.0 / scale, -1.0 / scale);
                        glNormal3f(-0.57735, -0.57735, 0.57735);
                        glVertex3f(-1.0 / scale, -1.0 / scale, 1.0 / scale);

                        glNormal3f(0.57735, 0.57735, -0.57735);
                        glVertex3f(1.0 / scale, 1.0 / scale, -1.0 / scale);
                        glNormal3f(-0.57735, 0.57735, -0.57735);
                        glVertex3f(-1.0 / scale, 1.0 / scale, -1.0 / scale);
                        glNormal3f(-0.57735, 0.57735, 0.57735);
                        glVertex3f(-1.0 / scale, 1.0 / scale, 1.0 / scale);
                        glNormal3f(0.57735, 0.57735, 0.57735);
                        glVertex3f(1.0 / scale, 1.0 / scale, 1.0 / scale);
                        glEnd();
                    }
                }
                glFlush();
            }
        }

        if (zRot <= 0) {
            zPlane -= 1.0;
        } else {
            zPlane += 1.0;
        }
    }

    quads = 0;
    zPlane = zmin;
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glDisable(GL_LIGHT1);

    while (true) {
        if (zPlane > zmax) {
            break;
        }
        QSetIterator<int> itRoom(pArea->getAreaRooms());
        while (itRoom.hasNext()) {
            glDisable(GL_LIGHT1);
            int currentRoomId = itRoom.next();
            TRoom* pR = mpMap->mpRoomDB->getRoom(currentRoomId);
            if (!pR) {
                continue;
            }
            auto rx = static_cast<float>(pR->x);
            auto ry = static_cast<float>(pR->y);
            auto rz = static_cast<float>(pR->z);
            if (rz != zPlane) {
                continue;
            }

            if (rz > pz) {
                if (abs(rz - pz) > mShowTopLevels) {
                    continue;
                }
            }
            if (rz < pz) {
                if (abs(rz - pz) > mShowBottomLevels) {
                    continue;
                }
            }

            int e = pR->z;
            const int ef = abs(e % 26);
            glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, planeColor[ef]);
            glMateriali(GL_FRONT, GL_SHININESS, 36); //gut:96

            if ((rz == pz) && (rx == px) && (ry == py)) {
                glDisable(GL_BLEND);
                glEnable(GL_LIGHTING);
                glDisable(GL_LIGHT1);
                float mc3[] = {1.0f, 0.0f, 0.0f, 1.0f};
                glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mc3);
                glMateriali(GL_FRONT, GL_SHININESS, 36);
                glColor4f(1.0, 0.0, 0.0, 1.0);
            } else if (currentRoomId == mTarget) {
                glDisable(GL_BLEND);
                glEnable(GL_LIGHTING);
                glDisable(GL_LIGHT1);
                float mc4[] = {0.0, 1.0, 0.0, 1.0};
                glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mc4);
                glMateriali(GL_FRONT, GL_SHININESS, 36); //36
                glColor4f(0.0, 1.0, 0.0, 1.0);
            } else if (rz <= pz) {
                glDisable(GL_BLEND);
                glEnable(GL_LIGHTING);
                glDisable(GL_LIGHT1);
                glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, planeColor2[ef]);
                glMateriali(GL_FRONT, GL_SHININESS, 36);
                glColor4f(planeColor[ef][0], planeColor[ef][1], planeColor[ef][2], planeColor[ef][3]);
            } else {
                glEnable(GL_BLEND);
                glEnable(GL_LIGHTING);
                glEnable(GL_LIGHT1);
                glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, planeColor[ef]);
                glMateriali(GL_FRONT, GL_SHININESS, 36); //56);//gut:36
                glColor4f(planeColor2[ef][0], planeColor2[ef][1], planeColor2[ef][2], planeColor2[ef][3]);

                glLoadIdentity();
                gluLookAt(px * 0.1 + xRot, py * 0.1 + yRot, pz * 0.1 + zRot, px * 0.1, py * 0.1, pz * 0.1, 0.0, 1.0, 0.0);
                if (pArea->gridMode) {
                    glScalef(0.2, 0.2, 0.1);
                    glTranslatef(0.5 * rx, 0.5 * ry, rz);
                } else {
                    glScalef(0.1, 0.1, 0.1);
                    glTranslatef(rx, ry, rz);
                }

                glLoadName(currentRoomId);
                quads++;
                glBegin(GL_QUADS);
                glNormal3f(0.57735, -0.57735, 0.57735);
                glVertex3f(1.0 / scale, -1.0 / scale, 1.0 / scale);
                glNormal3f(-0.57735, -0.57735, 0.57735);
                glVertex3f(-1.0 / scale, -1.0 / scale, 1.0 / scale);
                glNormal3f(-0.57735, -0.57735, -0.57735);
                glVertex3f(-1.0 / scale, -1.0 / scale, -1.0 / scale);
                glNormal3f(0.57735, -0.57735, -0.57735);
                glVertex3f(1.0 / scale, -1.0 / scale, -1.0 / scale);


                glNormal3f(0.57735, 0.57735, 0.57735);
                glVertex3f(1.0 / scale, 1.0 / scale, 1.0 / scale);
                glNormal3f(-0.57735, 0.57735, 0.57735);
                glVertex3f(-1.0 / scale, 1.0 / scale, 1.0 / scale);
                glNormal3f(-0.57735, -0.57735, 0.57735);
                glVertex3f(-1.0 / scale, -1.0 / scale, 1.0 / scale);
                glNormal3f(0.57735, -0.57735, 0.57735);
                glVertex3f(1.0 / scale, -1.0 / scale, 1.0 / scale);

                glNormal3f(-0.57735, 0.57735, -0.57735);
                glVertex3f(-1.0 / scale, 1.0 / scale, -1.0 / scale);
                glNormal3f(0.57735, 0.57735, -0.57735);
                glVertex3f(1.0 / scale, 1.0 / scale, -1.0 / scale);
                glNormal3f(0.57735, -0.57735, -0.57735);
                glVertex3f(1.0 / scale, -1.0 / scale, -1.0 / scale);
                glNormal3f(-0.57735, -0.57735, -0.57735);
                glVertex3f(-1.0 / scale, -1.0 / scale, -1.0 / scale);

                glNormal3f(0.57735, 0.57735, -0.57735);
                glVertex3f(1.0 / scale, 1.0 / scale, -1.0 / scale);
                glNormal3f(0.57735, 0.57735, 0.57735);
                glVertex3f(1.0 / scale, 1.0 / scale, 1.0 / scale);
                glNormal3f(0.57735, -0.57735, 0.57735);
                glVertex3f(1.0 / scale, -1.0 / scale, 1.0 / scale);
                glNormal3f(0.57735, -0.57735, -0.57735);
                glVertex3f(1.0 / scale, -1.0 / scale, -1.0 / scale);

                glNormal3f(-0.57735, 0.57735, 0.57735);
                glVertex3f(-1.0 / scale, 1.0 / scale, 1.0 / scale);
                glNormal3f(-0.57735, 0.57735, -0.57735);
                glVertex3f(-1.0 / scale, 1.0 / scale, -1.0 / scale);
                glNormal3f(-0.57735, -0.57735, -0.57735);
                glVertex3f(-1.0 / scale, -1.0 / scale, -1.0 / scale);
                glNormal3f(-0.57735, -0.57735, 0.57735);
                glVertex3f(-1.0 / scale, -1.0 / scale, 1.0 / scale);

                glNormal3f(0.57735, 0.57735, -0.57735);
                glVertex3f(1.0 / scale, 1.0 / scale, -1.0 / scale);
                glNormal3f(-0.57735, 0.57735, -0.57735);
                glVertex3f(-1.0 / scale, 1.0 / scale, -1.0 / scale);
                glNormal3f(-0.57735, 0.57735, 0.57735);
                glVertex3f(-1.0 / scale, 1.0 / scale, 1.0 / scale);
                glNormal3f(0.57735, 0.57735, 0.57735);
                glVertex3f(1.0 / scale, 1.0 / scale, 1.0 / scale);
                glEnd();

                float mc3[] = {0.2, 0.2, 0.6, 0.2};
                int env = pR->environment;
                if (mpMap->mEnvColors.contains(env)) {
                    env = mpMap->mEnvColors[env];
                } else {
                    if (!mpMap->mCustomEnvColors.contains(env)) {
                        env = 1;
                    }
                }

                switch (env) {
                case 1:
                    glColor4ub(128, 50, 50, 2);
                    mc3[0] = 128.0 / 255.0;
                    mc3[1] = 0.0 / 255.0;
                    mc3[2] = 0.0 / 255.0;
                    mc3[3] = 0.2;
                    break;

                case 2:
                    glColor4ub(128, 128, 50, 2);
                    mc3[0] = 0.0 / 255.0;
                    mc3[1] = 128.0 / 255.0;
                    mc3[2] = 0.0 / 255.0;
                    mc3[3] = 0.2;
                    break;
                case 3:
                    glColor4ub(50, 128, 50, 2);
                    mc3[0] = 128.0 / 255.0;
                    mc3[1] = 128.0 / 255.0;
                    mc3[2] = 0.0 / 255.0;
                    mc3[3] = 0.2;
                    break;

                case 4:
                    glColor4ub(50, 50, 128, 2);
                    mc3[0] = 0.0 / 255.0;
                    mc3[1] = 0.0 / 255.0;
                    mc3[2] = 128.0 / 255.0;
                    mc3[3] = 0.2;
                    break;

                case 5:
                    glColor4ub(128, 50, 128, 2);
                    mc3[0] = 128.0 / 255.0;
                    mc3[1] = 128.0 / 255.0;
                    mc3[2] = 0.0 / 255.0;
                    mc3[3] = 0.2;
                    break;
                case 6:
                    glColor4ub(50, 128, 128, 2);
                    mc3[0] = 0.0 / 255.0;
                    mc3[1] = 128.0 / 255.0;
                    mc3[2] = 128.0 / 255.0;
                    mc3[3] = 0.2;
                    break;
                case 7:
                    glColor4ub(52, 38, 78, 2);
                    mc3[0] = 128.0 / 255.0;
                    mc3[1] = 128.0 / 255.0;
                    mc3[2] = 128.0 / 255.0;
                    mc3[3] = 0.2;
                    break;
                case 8:
                    glColor4ub(65, 55, 35, 2);
                    mc3[0] = 55.0 / 255.0;
                    mc3[1] = 55.0 / 255.0;
                    mc3[2] = 55.0 / 255.0;
                    mc3[3] = 0.2;
                    break;

                case 9:
                    glColor4ub(175, 50, 50, 2);
                    mc3[0] = 255.0 / 255.0;
                    mc3[1] = 50.0 / 255.0;
                    mc3[2] = 50.0 / 255.0;
                    mc3[3] = 0.2;
                    break;

                case 10:
                    glColor4ub(255, 255, 50, 2);
                    mc3[0] = 50.0 / 255.0;
                    mc3[1] = 255.0 / 255.0;
                    mc3[2] = 50.0 / 255.0;
                    mc3[3] = 0.2;
                    break;
                case 11:
                    glColor4ub(50, 175, 175, 2);
                    mc3[0] = 255.0 / 255.0;
                    mc3[1] = 255.0 / 255.0;
                    mc3[2] = 50.0 / 255.0;
                    mc3[3] = 0.2;
                    break;

                case 12:
                    glColor4ub(175, 175, 50, 2);
                    mc3[0] = 50.0 / 255.0;
                    mc3[1] = 50.0 / 255.0;
                    mc3[2] = 255.0 / 255.0;
                    mc3[3] = 0.2;
                    break;

                case 13:
                    glColor4ub(175, 50, 175, 2);
                    mc3[0] = 255.0 / 255.0;
                    mc3[1] = 50.0 / 255.0;
                    mc3[2] = 255.0 / 255.0;
                    mc3[3] = 0.2;
                    break;
                case 14:
                    glColor4ub(50, 175, 50, 2);
                    mc3[0] = 50.0 / 255.0;
                    mc3[1] = 255.0 / 255.0;
                    mc3[2] = 255.0 / 255.0;
                    mc3[3] = 0.2;
                    break;
                case 15:
                    glColor4ub(50, 50, 175, 2);
                    mc3[0] = 255.0 / 255.0;
                    mc3[1] = 255.0 / 255.0;
                    mc3[2] = 255.0 / 255.0;
                    mc3[3] = 0.2;
                    break;
                default: //user defined room color
                    if (!mpMap->mCustomEnvColors.contains(env)) {
                        if (16 < env && env < 232)
                        {
                            quint8 base = env - 16;
                            quint8 r = base / 36;
                            quint8 g = (base - (r * 36)) / 6;
                            quint8 b = (base - (r * 36)) - (g * 6);

                            r = r == 0 ? 0 : (r - 1) * 40 + 95;
                            g = g == 0 ? 0 : (g - 1) * 40 + 95;
                            b = b == 0 ? 0 : (b - 1) * 40 + 95;
                            glColor4ub(r, g, b, 200);
                            mc3[0] = r / 255.0;
                            mc3[1] = g / 255.0;
                            mc3[2] = b / 255.0;
                            mc3[3] = 0.2;
                        } else if (231 < env && env < 256) {
                            quint8 k = ((env - 232) * 10) + 8;
                            glColor4ub(k, k, k, 200);
                            mc3[0] = k / 255.0;
                            mc3[1] = k / 255.0;
                            mc3[2] = k / 255.0;
                            mc3[3] = 0.2;
                        }
                        break;
                    }
                    QColor& _c = mpMap->mCustomEnvColors[env];
                    glColor4ub(_c.red(), _c.green(), _c.blue(), 255);
                    mc3[0] = _c.redF();
                    mc3[1] = _c.greenF();
                    mc3[2] = _c.blueF();
                    mc3[3] = 0.2;
                }
                glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mc3);
                glMateriali(GL_FRONT, GL_SHININESS, 96);
                glDisable(GL_DEPTH_TEST);
                glLoadIdentity();
                gluLookAt(px * 0.1 + xRot, py * 0.1 + yRot, pz * 0.1 + zRot, px * 0.1, py * 0.1, pz * 0.1, 0.0, 1.0, 0.0);

                if (pArea->gridMode) {
                    if ((rz == pz) && (rx == px) && (ry == py)) {
                        glScalef(0.1, 0.1, 0.020);
                        glTranslatef(0.1 * rx, 0.1 * ry, 5.0 * (rz + 0.25));
                    } else {
                        glScalef(0.2, 0.2, 0.020);
                        glTranslatef(0.5 * rx, 0.5 * ry, 5.0 * (rz + 0.25));
                    }
                } else {
                    glScalef(0.075, 0.075, 0.020);
                    glTranslatef(1.333333333 * rx, 1.333333333 * ry, 5.0 * (rz + 0.25)); //+0.4
                }

                glBegin(GL_QUADS);
                glNormal3f(0.57735, -0.57735, 0.57735);
                glVertex3f(1.0 / scale, -1.0 / scale, 1.0 / scale);
                glNormal3f(-0.57735, -0.57735, 0.57735);
                glVertex3f(-1.0 / scale, -1.0 / scale, 1.0 / scale);
                glNormal3f(-0.57735, -0.57735, -0.57735);
                glVertex3f(-1.0 / scale, -1.0 / scale, -1.0 / scale);
                glNormal3f(0.57735, -0.57735, -0.57735);
                glVertex3f(1.0 / scale, -1.0 / scale, -1.0 / scale);

                glNormal3f(0.57735, 0.57735, 0.57735);
                glVertex3f(1.0 / scale, 1.0 / scale, 1.0 / scale);
                glNormal3f(-0.57735, 0.57735, 0.57735);
                glVertex3f(-1.0 / scale, 1.0 / scale, 1.0 / scale);
                glNormal3f(-0.57735, -0.57735, 0.57735);
                glVertex3f(-1.0 / scale, -1.0 / scale, 1.0 / scale);
                glNormal3f(0.57735, -0.57735, 0.57735);
                glVertex3f(1.0 / scale, -1.0 / scale, 1.0 / scale);

                glNormal3f(-0.57735, 0.57735, -0.57735);
                glVertex3f(-1.0 / scale, 1.0 / scale, -1.0 / scale);
                glNormal3f(0.57735, 0.57735, -0.57735);
                glVertex3f(1.0 / scale, 1.0 / scale, -1.0 / scale);
                glNormal3f(0.57735, -0.57735, -0.57735);
                glVertex3f(1.0 / scale, -1.0 / scale, -1.0 / scale);
                glNormal3f(-0.57735, -0.57735, -0.57735);
                glVertex3f(-1.0 / scale, -1.0 / scale, -1.0 / scale);

                glNormal3f(0.57735, 0.57735, -0.57735);
                glVertex3f(1.0 / scale, 1.0 / scale, -1.0 / scale);
                glNormal3f(0.57735, 0.57735, 0.57735);
                glVertex3f(1.0 / scale, 1.0 / scale, 1.0 / scale);
                glNormal3f(0.57735, -0.57735, 0.57735);
                glVertex3f(1.0 / scale, -1.0 / scale, 1.0 / scale);
                glNormal3f(0.57735, -0.57735, -0.57735);
                glVertex3f(1.0 / scale, -1.0 / scale, -1.0 / scale);

                glNormal3f(-0.57735, 0.57735, 0.57735);
                glVertex3f(-1.0 / scale, 1.0 / scale, 1.0 / scale);
                glNormal3f(-0.57735, 0.57735, -0.57735);
                glVertex3f(-1.0 / scale, 1.0 / scale, -1.0 / scale);
                glNormal3f(-0.57735, -0.57735, -0.57735);
                glVertex3f(-1.0 / scale, -1.0 / scale, -1.0 / scale);
                glNormal3f(-0.57735, -0.57735, 0.57735);
                glVertex3f(-1.0 / scale, -1.0 / scale, 1.0 / scale);

                glNormal3f(0.57735, 0.57735, -0.57735);
                glVertex3f(1.0 / scale, 1.0 / scale, -1.0 / scale);
                glNormal3f(-0.57735, 0.57735, -0.57735);
                glVertex3f(-1.0 / scale, 1.0 / scale, -1.0 / scale);
                glNormal3f(-0.57735, 0.57735, 0.57735);
                glVertex3f(-1.0 / scale, 1.0 / scale, 1.0 / scale);
                glNormal3f(0.57735, 0.57735, 0.57735);
                glVertex3f(1.0 / scale, 1.0 / scale, 1.0 / scale);
                glEnd();

                continue;
            }

            float mc3[] = {0.2f, 0.2f, 0.7f, 1.0f};
            glLoadIdentity();
            gluLookAt(px * 0.1 + xRot, py * 0.1 + yRot, pz * 0.1 + zRot, px * 0.1, py * 0.1, pz * 0.1, 0.0, 1.0, 0.0);
            if (pArea->gridMode) {
                glScalef(0.2, 0.2, 0.1);
                glTranslatef(0.5 * rx, 0.5 * ry, rz);
            } else {
                glScalef(0.1, 0.1, 0.1);
                glTranslatef(rx, ry, rz);
            }

            glLoadName(currentRoomId);
            quads++;
            glBegin(GL_QUADS);
            glNormal3f(0.57735, -0.57735, 0.57735);
            glVertex3f(1.0 / scale, -1.0 / scale, 1.0 / scale);
            glNormal3f(-0.57735, -0.57735, 0.57735);
            glVertex3f(-1.0 / scale, -1.0 / scale, 1.0 / scale);
            glNormal3f(-0.57735, -0.57735, -0.57735);
            glVertex3f(-1.0 / scale, -1.0 / scale, -1.0 / scale);
            glNormal3f(0.57735, -0.57735, -0.57735);
            glVertex3f(1.0 / scale, -1.0 / scale, -1.0 / scale);

            glNormal3f(0.57735, 0.57735, 0.57735);
            glVertex3f(1.0 / scale, 1.0 / scale, 1.0 / scale);
            glNormal3f(-0.57735, 0.57735, 0.57735);
            glVertex3f(-1.0 / scale, 1.0 / scale, 1.0 / scale);
            glNormal3f(-0.57735, -0.57735, 0.57735);
            glVertex3f(-1.0 / scale, -1.0 / scale, 1.0 / scale);
            glNormal3f(0.57735, -0.57735, 0.57735);
            glVertex3f(1.0 / scale, -1.0 / scale, 1.0 / scale);

            glNormal3f(-0.57735, 0.57735, -0.57735);
            glVertex3f(-1.0 / scale, 1.0 / scale, -1.0 / scale);
            glNormal3f(0.57735, 0.57735, -0.57735);
            glVertex3f(1.0 / scale, 1.0 / scale, -1.0 / scale);
            glNormal3f(0.57735, -0.57735, -0.57735);
            glVertex3f(1.0 / scale, -1.0 / scale, -1.0 / scale);
            glNormal3f(-0.57735, -0.57735, -0.57735);
            glVertex3f(-1.0 / scale, -1.0 / scale, -1.0 / scale);

            glNormal3f(0.57735, 0.57735, -0.57735);
            glVertex3f(1.0 / scale, 1.0 / scale, -1.0 / scale);
            glNormal3f(0.57735, 0.57735, 0.57735);
            glVertex3f(1.0 / scale, 1.0 / scale, 1.0 / scale);
            glNormal3f(0.57735, -0.57735, 0.57735);
            glVertex3f(1.0 / scale, -1.0 / scale, 1.0 / scale);
            glNormal3f(0.57735, -0.57735, -0.57735);
            glVertex3f(1.0 / scale, -1.0 / scale, -1.0 / scale);

            glNormal3f(-0.57735, 0.57735, 0.57735);
            glVertex3f(-1.0 / scale, 1.0 / scale, 1.0 / scale);
            glNormal3f(-0.57735, 0.57735, -0.57735);
            glVertex3f(-1.0 / scale, 1.0 / scale, -1.0 / scale);
            glNormal3f(-0.57735, -0.57735, -0.57735);
            glVertex3f(-1.0 / scale, -1.0 / scale, -1.0 / scale);
            glNormal3f(-0.57735, -0.57735, 0.57735);
            glVertex3f(-1.0 / scale, -1.0 / scale, 1.0 / scale);

            glNormal3f(0.57735, 0.57735, -0.57735);
            glVertex3f(1.0 / scale, 1.0 / scale, -1.0 / scale);
            glNormal3f(-0.57735, 0.57735, -0.57735);
            glVertex3f(-1.0 / scale, 1.0 / scale, -1.0 / scale);
            glNormal3f(-0.57735, 0.57735, 0.57735);
            glVertex3f(-1.0 / scale, 1.0 / scale, 1.0 / scale);
            glNormal3f(0.57735, 0.57735, 0.57735);
            glVertex3f(1.0 / scale, 1.0 / scale, 1.0 / scale);

            glEnd();

            int env = pR->environment;
            if (mpMap->mEnvColors.contains(env)) {
                env = mpMap->mEnvColors[env];
            } else {
                if (!mpMap->mCustomEnvColors.contains(env)) {
                    env = 1;
                }
            }
            switch (env) {
            case 1:
                glColor4ub(128, 50, 50, 255);
                mc3[0] = 128.0 / 255.0;
                mc3[1] = 0.0 / 255.0;
                mc3[2] = 0.0 / 255.0;
                mc3[3] = 255.0 / 255.0;
                break;

            case 2:
                glColor4ub(128, 128, 50, 255);
                mc3[0] = 0.0 / 255.0;
                mc3[1] = 128.0 / 255.0;
                mc3[2] = 0.0 / 255.0;
                mc3[3] = 255.0 / 255.0;
                break;
            case 3:
                glColor4ub(50, 128, 50, 255);
                mc3[0] = 128.0 / 255.0;
                mc3[1] = 128.0 / 255.0;
                mc3[2] = 0.0 / 255.0;
                mc3[3] = 255.0 / 255.0;
                break;

            case 4:
                glColor4ub(50, 50, 128, 255);
                mc3[0] = 0.0 / 255.0;
                mc3[1] = 0.0 / 255.0;
                mc3[2] = 128.0 / 255.0;
                mc3[3] = 255.0 / 255.0;
                break;

            case 5:
                glColor4ub(128, 50, 128, 255);
                mc3[0] = 128.0 / 255.0;
                mc3[1] = 128.0 / 255.0;
                mc3[2] = 0.0 / 255.0;
                mc3[3] = 255.0 / 255.0;
                break;
            case 6:
                glColor4ub(50, 128, 128, 255);
                mc3[0] = 0.0 / 255.0;
                mc3[1] = 128.0 / 255.0;
                mc3[2] = 128.0 / 255.0;
                mc3[3] = 255.0 / 255.0;
                break;
            case 7:
                glColor4ub(52, 38, 78, 255);
                mc3[0] = 128.0 / 255.0;
                mc3[1] = 128.0 / 255.0;
                mc3[2] = 128.0 / 255.0;
                mc3[3] = 255.0 / 255.0;
                break;
            case 8:
                glColor4ub(65, 55, 35, 255);
                mc3[0] = 55.0 / 255.0;
                mc3[1] = 55.0 / 255.0;
                mc3[2] = 55.0 / 255.0;
                mc3[3] = 255.0 / 255.0;
                break;

            case 9:
                glColor4ub(175, 50, 50, 255);
                mc3[0] = 255.0 / 255.0;
                mc3[1] = 50.0 / 255.0;
                mc3[2] = 50.0 / 255.0;
                mc3[3] = 255.0 / 255.0;
                break;

            case 10:
                glColor4ub(255, 255, 50, 255);
                mc3[0] = 50.0 / 255.0;
                mc3[1] = 255.0 / 255.0;
                mc3[2] = 50.0 / 255.0;
                mc3[3] = 255.0 / 255.0;
                break;
            case 11:
                glColor4ub(50, 175, 175, 255);
                mc3[0] = 255.0 / 255.0;
                mc3[1] = 255.0 / 255.0;
                mc3[2] = 50.0 / 255.0;
                mc3[3] = 255.0 / 255.0;
                break;

            case 12:
                glColor4ub(175, 175, 50, 255);
                mc3[0] = 50.0 / 255.0;
                mc3[1] = 50.0 / 255.0;
                mc3[2] = 255.0 / 255.0;
                mc3[3] = 255.0 / 255.0;
                break;

            case 13:
                glColor4ub(175, 50, 175, 255);
                mc3[0] = 255.0 / 255.0;
                mc3[1] = 50.0 / 255.0;
                mc3[2] = 255.0 / 255.0;
                mc3[3] = 255.0 / 255.0;
                break;
            case 14:
                glColor4ub(50, 175, 50, 255);
                mc3[0] = 50.0 / 255.0;
                mc3[1] = 255.0 / 255.0;
                mc3[2] = 255.0 / 255.0;
                mc3[3] = 255.0 / 255.0;
                break;
            case 15:
                glColor4ub(50, 50, 175, 255);
                mc3[0] = 255.0 / 255.0;
                mc3[1] = 255.0 / 255.0;
                mc3[2] = 255.0 / 255.0;
                mc3[3] = 255.0 / 255.0;
                break;
            default: //user defined room color
                if (!mpMap->mCustomEnvColors.contains(env)) {
                    if (16 < env && env < 232)
                    {
                        quint8 base = env - 16;
                        quint8 r = base / 36;
                        quint8 g = (base - (r * 36)) / 6;
                        quint8 b = (base - (r * 36)) - (g * 6);

                        r = r == 0 ? 0 : (r - 1) * 40 + 95;
                        g = g == 0 ? 0 : (g - 1) * 40 + 95;
                        b = b == 0 ? 0 : (b - 1) * 40 + 95;
                        glColor4ub(r, g, b, 200);
                        mc3[0] = r / 255.0;
                        mc3[1] = g / 255.0;
                        mc3[2] = b / 255.0;
                        mc3[3] = 0.2;
                    } else if (231 < env && env < 256) {
                        quint8 k = ((env - 232) * 10) + 8;
                        glColor4ub(k, k, k, 200);
                        mc3[0] = k / 255.0;
                        mc3[1] = k / 255.0;
                        mc3[2] = k / 255.0;
                        mc3[3] = 0.2;
                    }
                    break;
                }
                QColor& _c = mpMap->mCustomEnvColors[env];
                glColor4ub(_c.red(), _c.green(), _c.blue(), 255);
                mc3[0] = _c.redF();
                mc3[1] = _c.greenF();
                mc3[2] = _c.blueF();
                mc3[3] = 1.0;
            }
            glDisable(GL_DEPTH_TEST);
            glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mc3);
            glMateriali(GL_FRONT, GL_SHININESS, 6);
            glLoadIdentity();
            gluLookAt(px * 0.1 + xRot, py * 0.1 + yRot, pz * 0.1 + zRot, px * 0.1, py * 0.1, pz * 0.1, 0.0, 1.0, 0.0);
            if (pArea->gridMode) {
                if ((rz == pz) && (rx == px) && (ry == py)) {
                    glScalef(0.1, 0.1, 0.020);
                    glTranslatef(rx, ry, 5.0 * (rz + 0.25));
                } else {
                    glScalef(0.2, 0.2, 0.020);
                    glTranslatef(0.5 * rx, 0.5 * ry, 5.0 * (rz + 0.25));
                }
            } else {
                // This is the only place this flag is used:
                if (is2DView) {
                    glScalef(0.090, 0.090, 0.020);
                    glTranslatef(1.1111111 * rx, 1.1111111 * ry, 5.0 * (rz + 0.25)); //+0.4
                } else {
                    glScalef(0.075, 0.075, 0.020);
                    glTranslatef(1.333333333 * rx, 1.333333333 * ry, 5.0 * (rz + 0.25)); //+0.4
                }
            }
            glBegin(GL_QUADS);
            glNormal3f(0.57735, -0.57735, 0.57735);
            glVertex3f(1.0 / scale, -1.0 / scale, 1.0 / scale);
            glNormal3f(-0.57735, -0.57735, 0.57735);
            glVertex3f(-1.0 / scale, -1.0 / scale, 1.0 / scale);
            glNormal3f(-0.57735, -0.57735, -0.57735);
            glVertex3f(-1.0 / scale, -1.0 / scale, -1.0 / scale);
            glNormal3f(0.57735, -0.57735, -0.57735);
            glVertex3f(1.0 / scale, -1.0 / scale, -1.0 / scale);

            glNormal3f(0.57735, 0.57735, 0.57735);
            glVertex3f(1.0 / scale, 1.0 / scale, 1.0 / scale);
            glNormal3f(-0.57735, 0.57735, 0.57735);
            glVertex3f(-1.0 / scale, 1.0 / scale, 1.0 / scale);
            glNormal3f(-0.57735, -0.57735, 0.57735);
            glVertex3f(-1.0 / scale, -1.0 / scale, 1.0 / scale);
            glNormal3f(0.57735, -0.57735, 0.57735);
            glVertex3f(1.0 / scale, -1.0 / scale, 1.0 / scale);

            glNormal3f(-0.57735, 0.57735, -0.57735);
            glVertex3f(-1.0 / scale, 1.0 / scale, -1.0 / scale);
            glNormal3f(0.57735, 0.57735, -0.57735);
            glVertex3f(1.0 / scale, 1.0 / scale, -1.0 / scale);
            glNormal3f(0.57735, -0.57735, -0.57735);
            glVertex3f(1.0 / scale, -1.0 / scale, -1.0 / scale);
            glNormal3f(-0.57735, -0.57735, -0.57735);
            glVertex3f(-1.0 / scale, -1.0 / scale, -1.0 / scale);

            glNormal3f(0.57735, 0.57735, -0.57735);
            glVertex3f(1.0 / scale, 1.0 / scale, -1.0 / scale);
            glNormal3f(0.57735, 0.57735, 0.57735);
            glVertex3f(1.0 / scale, 1.0 / scale, 1.0 / scale);
            glNormal3f(0.57735, -0.57735, 0.57735);
            glVertex3f(1.0 / scale, -1.0 / scale, 1.0 / scale);
            glNormal3f(0.57735, -0.57735, -0.57735);
            glVertex3f(1.0 / scale, -1.0 / scale, -1.0 / scale);

            glNormal3f(-0.57735, 0.57735, 0.57735);
            glVertex3f(-1.0 / scale, 1.0 / scale, 1.0 / scale);
            glNormal3f(-0.57735, 0.57735, -0.57735);
            glVertex3f(-1.0 / scale, 1.0 / scale, -1.0 / scale);
            glNormal3f(-0.57735, -0.57735, -0.57735);
            glVertex3f(-1.0 / scale, -1.0 / scale, -1.0 / scale);
            glNormal3f(-0.57735, -0.57735, 0.57735);
            glVertex3f(-1.0 / scale, -1.0 / scale, 1.0 / scale);

            glNormal3f(0.57735, 0.57735, -0.57735);
            glVertex3f(1.0 / scale, 1.0 / scale, -1.0 / scale);
            glNormal3f(-0.57735, 0.57735, -0.57735);
            glVertex3f(-1.0 / scale, 1.0 / scale, -1.0 / scale);
            glNormal3f(-0.57735, 0.57735, 0.57735);
            glVertex3f(-1.0 / scale, 1.0 / scale, 1.0 / scale);
            glNormal3f(0.57735, 0.57735, 0.57735);
            glVertex3f(1.0 / scale, 1.0 / scale, 1.0 / scale);
            glEnd();

            glColor4ub(128, 128, 128, 255);


            glBegin(GL_TRIANGLES);

            if (pR->getDown() > -1) {
                glVertex3f(0.0, -0.95 / scale, 0.0);
                glVertex3f(0.95 / scale, -0.25 / scale, 0.0);
                glVertex3f(-0.95 / scale, -0.25 / scale, 0.0);
            }
            if (pR->getUp() > -1) {
                glVertex3f(0.0, 0.95 / scale, 0.0);
                glVertex3f(-0.95 / scale, 0.25 / scale, 0.0);
                glVertex3f(0.95 / scale, 0.25 / scale, 0.0);
            }
            glEnd();

//            if (mpMap->rooms[pArea->rooms[i]]->out > -1) {
//                glBegin( GL_LINE_LOOP );
//                for (int angle=0; angle<360; angle += 1 ) {
//                    glVertex3f((0.5 + sin((float)angle) * 0.25)/scale, ( cos((float)angle) * 0.25)/scale, 0.0);
//                }
//                glEnd();
//            }

//            glTranslatef(-0.1, 0.0, 0.0);
//            if (mpMap->rooms[pArea->rooms[i]]->in > -1) {
//                glBegin(GL_TRIANGLE_FAN);
//                glVertex3f(0.0, 0.0, 0.0);
//                for (int angle=0; angle<=360; angle += 5) {
//                    glVertex3f((sin((float)angle)*0.25)/scale, (cos((float)angle)*0.25)/scale, 0.0);
//                }
//                glEnd();
//            }
        }


        zPlane += 1.0;
    }
    glFlush();
}

void GLWidget::resizeGL(int w, int h)
{
    glViewport(0, 0, (GLint)w, (GLint)h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60 * mScale, (GLfloat)w / (GLfloat)h, 0.0001, 10000.0);
    glMatrixMode(GL_MODELVIEW);
}

void GLWidget::mousePressEvent(QMouseEvent* event)
{
    mudlet::self()->activateProfile(mpHost);
    if (!mpMap||!mpMap->mpRoomDB) {
        return;
    }
    if (event->buttons() & Qt::LeftButton) {
        int x = event->x();
        int y = height() - event->y(); // the opengl origin is at bottom left
        GLuint buff[16] = {0};
        GLint hits;
        GLint view[4];
        makeCurrent();
        glSelectBuffer(16, buff);
        glGetIntegerv(GL_VIEWPORT, view);
        glRenderMode(GL_SELECT);
        glInitNames();
        glPushName(0);
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        gluPickMatrix(x, y, 0.1, 0.1, view);
        gluPerspective(60 * mScale, (GLfloat)width() / (GLfloat)height(), 0.0001, 10000.0);
        glMatrixMode(GL_MODELVIEW);
        doneCurrent();
        mTarget = -22;
        makeCurrent();
        paintGL();
        doneCurrent();
        makeCurrent();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        hits = glRenderMode(GL_RENDER);

        for (int i = 0; i < hits; i++) {
            mTarget = buff[i * 4 + 3];
            //TODO: multiple assignments
            //            unsigned int minZ = buff[i * 4 + 1];
            //            unsigned int maxZ = buff[i * 4 + 2];
        }
        glViewport(0, 0, (GLint)width(), (GLint)height());
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluPerspective(60 * mScale, (GLfloat)width() / (GLfloat)height(), 0.0001, 10000.0);
        glMatrixMode(GL_MODELVIEW);
        doneCurrent();
        update();
        if (mpMap->mpRoomDB->getRoom(mTarget)) {
            mpMap->mTargetID = mTarget;
            if (mpMap->findPath(mpMap->mRoomIdHash.value(mpMap->mProfileName), mpMap->mTargetID)) {
                mpMap->mpHost->startSpeedWalk();
            }
            //            else
            //            {
            //                QMessageBox msgBox;
            //                msgBox.setText("Cannot find a path to this room using regular exits.#glWidget\n");
            //                msgBox.exec();
            //            }
        } else {
            mPanMode = true;
            mPanXStart = x;
            mPanYStart = y;
        }
        //        else
        //        {
        //            QMessageBox msgBox;
        //            msgBox.setText("ERROR: Target room cannot be found in map db.\n");
        //            msgBox.exec();
        //        }
    }
}

void GLWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (!mpMap||!mpMap->mpRoomDB) {
        return;
    }
    if (mPanMode) {
        int x = event->x();
        int y = height() - event->y(); // the opengl origin is at bottom left
        if ((mPanXStart - x) > 1) {
            slot_shiftRight();
            mPanXStart = x;
        } else if ((mPanXStart - x) < -1) {
            slot_shiftLeft();
            mPanXStart = x;
        }
        if ((mPanYStart - y) > 1) {
            slot_shiftUp();
            mPanYStart = y;
        } else if ((mPanYStart - y) < -1) {
            slot_shiftDown();
            mPanYStart = y;
        }
    }
}

void GLWidget::mouseReleaseEvent(QMouseEvent* event)
{
    Q_UNUSED(event)
    mPanMode = false;
}

void GLWidget::wheelEvent(QWheelEvent* e)
{
    int xDelta = qRound(e->angleDelta().x() / (8.0 * 15.0));
    int yDelta = qRound(e->angleDelta().y() / (8.0 * 15.0));
    bool used = false;
    if (yDelta) {
        if (abs(mScale) < 0.3) {
            mScale += 0.01 * yDelta;
        } else {
            mScale += 0.03 * yDelta;
        }
        makeCurrent();
        resizeGL(width(), height());
        doneCurrent();
        update();
        used = true;
    }

    // Space for future use of xDelta - depending on what that is the update
    // may need to be moved out of the yDelta part
    Q_UNUSED(xDelta)

    e->setAccepted(used);
}
