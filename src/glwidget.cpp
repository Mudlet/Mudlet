/***************************************************************************
 *   Copyright (C) 2010-2011 by Heiko Koehn ( KoehnHeiko@googlemail.com )  *
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

#include <QtGui>

//#include <QtOpenGL/qgl.h>

#include <QtOpenGL/qgl.h> //problem with git
#include <math.h>
#include <QDebug>
#include "glwidget.h"
#include "Host.h"
#include "dlgMapper.h"

#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE  0x809D
#endif

#if QT_VERSION >= 0x040800
    #include <GL/glu.h>
#endif

bool ortho;
bool selectionMode = false;
bool mPanMode = false;
float xpos = 0, ypos = 0, zpos = 0, xrot = 0, yrot = 0, angle=0.0, mPanXStart=0, mPanYStart=0;
float zmax, zmin;

GLWidget::GLWidget(QWidget *parent)
    : QGLWidget(QGLFormat(QGL::SampleBuffers), parent)
{
    mpMap = 0;
    xDist=0.0;
    yDist=0.0;
    zDist=0.0;
    xRot=1.0;
    yRot=5.0;
    zRot=10.0;
    ortho=false;//true;
    xDist = 0;
    yDist = 0;
    zDist = -1 ;
    mScale = 1.0;
    zmax = 9999999.0;
    zmin = 9999999.0;
    mShowTopLevels = 9999999;
    mShowBottomLevels = 999999;
    setAttribute( Qt::WA_OpaquePaintEvent );
    is2DView = false;
    mShiftMode = false;
    mAID = 0;
    mRID = 0;
    mOx = 0;
    mOy = 0;
    mOz = 0;
}


GLWidget::GLWidget(TMap * pM, QWidget *parent)
    : QGLWidget(QGLFormat(QGL::SampleBuffers), parent)
{
    mpHost = 0;
    mpMap = pM;
    is2DView = false;
    mShiftMode = false;
    mAID = 0;
    mRID = 0;
    mOx = 0;
    mOy = 0;
    mOz = 0;
}


GLWidget::~GLWidget()
{
}

QSize GLWidget::minimumSizeHint() const
{
    return QSize(50, 50);
}

QSize GLWidget::sizeHint() const
{
    return QSize(400, 400);
}

static void qNormalizeAngle(int &angle)
{
    angle /= 10;
}

void GLWidget::fullView()
{
    mShowTopLevels = 9999999;
    mShowBottomLevels = 999999;
    updateGL();
}


void GLWidget::shiftDown()
{
    mShiftMode = true;
    mOy--;
    updateGL();
}

void GLWidget::shiftUp()
{
    mShiftMode = true;
    mOy++;
    updateGL();
}

void GLWidget::shiftLeft()
{
    mShiftMode = true;
    mOx--;
    updateGL();
}

void GLWidget::shiftRight()
{
    mShiftMode = true;
    mOx++;
    updateGL();
}
void GLWidget::shiftZup()
{
    mShiftMode = true;
    mOz++;
    updateGL();
}

void GLWidget::shiftZdown()
{
    mShiftMode = true;
    mOz--;
    updateGL();
}

void GLWidget::showInfo()
{
    mShowInfo = !mShowInfo;
    updateGL();
}


void GLWidget::singleView()
{
    mShowTopLevels = 0;
    mShowBottomLevels = 0;
    updateGL();
}

void GLWidget::increaseTop()
{
    mShowTopLevels += 1;
    updateGL();
}

void GLWidget::reduceTop()
{
    if( mShowTopLevels <= 0 ) mShowTopLevels = abs(zmax);
    if( abs(mShowTopLevels) > abs(zmax) )
        mShowTopLevels = abs(zmax);
    mShowTopLevels--;
    updateGL();
}

void GLWidget::increaseBottom()
{
    mShowBottomLevels++;
    updateGL();
}

void GLWidget::reduceBottom()
{
    if( mShowBottomLevels <= 0 ) mShowBottomLevels = abs(zmin);
    if( abs(mShowBottomLevels) > abs(zmin) )
        mShowBottomLevels = abs(zmin);
    mShowBottomLevels--;
    updateGL();
}

void GLWidget::defaultView()
{
    xRot=1.0;
    yRot=5.0;
    zRot=10.0;
    mScale = 1.0;
    is2DView = false;
    setVisible(!isVisible());
    mpMap->mpMapper->mp2dMap->setVisible(!mpMap->mpMapper->mp2dMap->isVisible());
    updateGL();
}

void GLWidget::sideView()
{
    xRot=7.0;
    yRot=-10.0;
    zRot=0.0;
    mScale = 1.0;
    is2DView = false;
    updateGL();
}

void GLWidget::topView()
{
    xRot=0.0;
    yRot=0.0;
    zRot=15.0;
    mScale = 1.0;
    is2DView = true;
    updateGL();
}


void GLWidget::goRoom( const QString & s )
{

}

void GLWidget::setScale(int angle)
{
    mScale = 150/((float)angle+300);
    resizeGL(width(),height());
    updateGL();
    return;
}

void GLWidget::setXRotation(int angle)
{
    qNormalizeAngle(angle);
    xRot=angle;
    is2DView = false;
    updateGL();
    return;
}

void GLWidget::setYRotation(int angle)
{
    qNormalizeAngle(angle);
    yRot=angle;
    is2DView = false;
    updateGL();
    return;
}

void GLWidget::setZRotation(int angle)
{
    qNormalizeAngle(angle);
    zRot=angle;
    is2DView = false;
    updateGL();
    return;
}

void GLWidget::setXDist(int angle)
{
    xDist=angle;
    is2DView = false;
    updateGL();
    return;
}

void GLWidget::setYDist(int angle)
{
    yDist=angle;
    is2DView = false;
    updateGL();
    return;
}

void GLWidget::setZDist(int angle)
{
    zDist=angle;
    is2DView = false;
    updateGL();
    return;
}


void GLWidget::initializeGL()
{
    qglClearColor( Qt::black );
    xRot = 1;
    yRot = 5;
    zRot = 10;
    glEnable( GL_LIGHTING );
    glEnable( GL_LIGHT0 );
    glShadeModel( GL_SMOOTH );
    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearDepth(1.0);
    is2DView = false;
}

void GLWidget::showArea(QString name)
{
    if( !mpMap ) return;
    QMapIterator<int, QString> it( mpMap->mpRoomDB->getAreaNamesMap() );
    while( it.hasNext() )
    {
        it.next();
        int areaID = it.key();
        QString _n = it.value();
        if( name == _n )
        {
            mAID = areaID;
            mRID = mpMap->mRoomId;//FIXME:
            mShiftMode = true;
            mOx = 0;
            mOy = 0;
            mOz = 0;
            updateGL();
            break;
        }
    }

}

void GLWidget::paintGL()
{
    if( ! mpMap ) return;
    float px,py,pz;
    if( mRID != mpMap->mRoomId && mShiftMode )  mShiftMode = false;

    int ox, oy, oz;
    if( ! mShiftMode )
    {


        mRID = mpMap->mRoomId;
        TRoom * pRID = mpMap->mpRoomDB->getRoom( mRID );
        if( !pRID  )
        {
            glClearDepth(1.0);
            glDepthFunc(GL_LESS);
            glClearColor (0.0,0.0,0.0,1.0);
            glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            renderText(width()/3,height()/2,"no map or no valid position on map", QFont("Bitstream Vera Sans Mono", 30, QFont::Courier ) );

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

    }
    else
    {
        ox = mOx;
        oy = mOy;
        oz = mOz;
    }
    px = static_cast<float>(ox);//mpMap->rooms[mpMap->mRoomId]->x);
    py = static_cast<float>(oy);//mpMap->rooms[mpMap->mRoomId]->y);
    pz = static_cast<float>(oz);//mpMap->rooms[mpMap->mRoomId]->z);
    TArea * pArea = mpMap->mpRoomDB->getArea(mAID);
    if( ! pArea ) return;
    if( pArea->gridMode )
    {
        xRot=0.0;
        yRot=0.0;
        zRot=15.0;
    }
    zmax = static_cast<float>(pArea->max_z);
    zmin = static_cast<float>(pArea->min_z);
    float zEbene;
    glEnable(GL_CULL_FACE);
    glClearDepth(1.0);
    glDepthFunc(GL_LESS);

    glClearColor (0.0,0.0,0.0,1.0);
    glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    GLfloat diffuseLight[] = {0.507, 0.507, 0.507, 1.0};
    GLfloat diffuseLight2[] = {0.501, 0.901, 0.501, 1.0};
    GLfloat ambientLight[] = {0.403, 0.403, 0.403, 1.0};
    GLfloat ambientLight2[] = {0.4501, 0.4501, 0.4501, 1.0};

    //GLfloat specularLight[] = {.01, .01, .01, 1.};//TODO: fuer ich-sphere
    GLfloat light0Pos[] = {5000.0, 4000.0, 1000.0, 0};
    GLfloat light1Pos[] = {5000.0, 1000.0, 1000.0, 0};
    glLightfv( GL_LIGHT0, GL_AMBIENT, ambientLight );
    glLightfv( GL_LIGHT0, GL_DIFFUSE, diffuseLight );
    glLightfv (GL_LIGHT0, GL_POSITION, light0Pos );
    glLightfv( GL_LIGHT1, GL_AMBIENT, ambientLight2 );
    glLightfv( GL_LIGHT1, GL_DIFFUSE, diffuseLight2 );
    glLightfv (GL_LIGHT1, GL_POSITION, light1Pos );
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE );
    glLightfv (GL_LIGHT0, GL_POSITION, light0Pos );
    glLightfv (GL_LIGHT1, GL_POSITION, light1Pos );
    glBlendFunc(GL_SRC_ALPHA, GL_SRC_COLOR);//GL_ONE_MINUS_SRC_ALPHA);
    glLoadIdentity();

    dehnung = 4.0;

    glDisable( GL_FOG );
    glEnable(GL_BLEND);
    glEnable( GL_LIGHTING );
    glDisable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LIGHT0);
    //glEnable(GL_LIGHT1);

    if( zRot <= 0 )
        zEbene = zmax;
    else
        zEbene = zmin;

    glEnable (GL_LINE_SMOOTH);
    glEnable (GL_LINE_STIPPLE);
    glLineWidth(1.0);
    int quads=0;
    int verts=0;
    float ebenenColor2[][4] =
    {
        {0.9, 0.5, 0.0, 1.0},
        {165.0/255.0, 102.0/255.0, 167.0/255.0, 1.0},
        {170.0/255.0, 10.0/255.0, 127.0/255.0, 1.0},
        {203.0/255.0, 135.0/255.0, 101.0/255.0, 1.0},
        {154.0/255.0, 154.0/255.0, 115.0/255.0, 1.0},
        {107.0/255.0, 154.0/255.0, 100.0/255.0, 1.0},
        {154.0/255.0, 184.0/255.0, 111.0/255.0, 1.0},
        {67.0/255.0, 154.0/255.0, 148.0/255.0, 1.0},
        {154.0/255.0, 118.0/255.0, 151.0/255.0, 1.0},
        {208.0/255.0, 213.0/255.0, 164.0/255.0, 1.0},
        {213.0/255.0, 169.0/255.0, 158.0/255.0, 1.0},
        {139.0/255.0, 209.0/255.0, 0, 1.0},
        {163.0/255.0, 209.0/255.0, 202.0/255.0, 1.0},
        {158.0/255.0, 156.0/255.0, 209.0/255.0, 1.0},
        {209.0/255.0, 144.0/255.0, 162.0/255.0, 1.0},
        {209.0/255.0, 183.0/255.0, 78.0/255.0, 1.0},
        {111.0/255.0, 209.0/255.0, 88.0/255.0, 1.0},
        {95.0/255.0, 120.0/255.0, 209.0/255.0, 1.0},
        {31.0/255.0, 209.0/255.0, 126.0/255.0, 1.0},
        {1.0, 170.0/255.0, 1.0, 1.0},
        {158.0/255.0, 105.0/255.0, 158.0/255.0, 1.0},
        {68.0/255.0, 189.0/255.0, 189.0/255.0, 1.0},
        {0.1, 0.69, 0.49, 1.0},
        {0.0, 0.15, 1.0, 1.0},
        {0.12, 0.02, 0.20, 1.0},
        {0.0, 0.3, 0.1, 1.0}
    };



    float ebenenColor[][4] =
    {
        { 0.5, 0.6, 0.5, 0.2 },
        {0.233, 0.498, 0.113, 0.2},
        {0.666, 0.333, 0.498, 0.2},
        {0.5, 0.333, 0.666, 0.2},
        {0.69, 0.458, 0.0, 0.2},
        {0.333, 0.0, 0.49, 0.2},
        {133.0/255.0, 65.0/255.0, 98.0/255.0, 0.2},
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
        {0.2, 0.1, 0.3, 0.2}
    };

    while( true )
    {
        if( zRot <= 0 )
        {
            if( zEbene < zmin )
            {
                break;
            }
        }
        else
        {
            if( zEbene > zmax )
            {
                break;
            }
        }
        for( int i=0; i<pArea->rooms.size(); i++ )
        {
            TRoom * pR = mpMap->mpRoomDB->getRoom(pArea->rooms[i]);
            if( !pR ) continue;
            float rx = static_cast<float>(pR->x);
            float ry = static_cast<float>(pR->y);
            float rz = static_cast<float>(pR->z);
            if( rz != zEbene ) continue;
            if( rz > pz )
                if( abs(rz-pz) > mShowTopLevels ) continue;
            if( rz < pz )
                if( abs(rz-pz) > mShowBottomLevels ) continue;
            QList<int> exitList;
            exitList.push_back( pR->getNorth() );
            exitList.push_back( pR->getNorthwest() );
            exitList.push_back( pR->getEast() );
            exitList.push_back( pR->getSoutheast() );
            exitList.push_back( pR->getSouth() );
            exitList.push_back( pR->getSouthwest() );
            exitList.push_back( pR->getWest() );
            exitList.push_back( pR->getNorthwest() );
            exitList.push_back( pR->getUp() );
            exitList.push_back( pR->getDown() );
            int e = pR->z;
            int ef;
            ef = abs(e%26);
            glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, ebenenColor[ef]);
            glMateriali(GL_FRONT, GL_SHININESS, 1);
            glDisable(GL_DEPTH_TEST);
            if( rz <= pz )
            {
                if( ( rz == pz ) && ( rx == px ) && ( ry == py ) )
                {
                    glDisable(GL_BLEND);
                    glEnable( GL_LIGHTING );
                    float mc3[] = { 1.0f, 0.0f, 0.0f, 1.0f };
                    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mc3);
                    glMateriali(GL_FRONT, GL_SHININESS, 1);
                    glColor4f(1.0, 0.0, 0.0, 1.0);
                }
                else
                {
                    glDisable(GL_BLEND);
                    glEnable( GL_LIGHTING );
                    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, ebenenColor[ef]);
                    glMateriali(GL_FRONT, GL_SHININESS, 1);
                    glColor4f(0.3,0.3,0.3,1.0);/*ebenenColor[ef][0],
                              ebenenColor[ef][1],
                              ebenenColor[ef][2],
                              ebenenColor[ef][3]);*/
                }
                for( int k=0; k<exitList.size(); k++ )
                {
                    bool areaExit = false;
                    if( exitList[k] == -1 ) continue;
                    TRoom * pExit = mpMap->mpRoomDB->getRoom( exitList[k] );
                    if( !pExit )
                    {
                        continue;
                    }
                    if( pExit->getArea() != mAID )
                    {
                        areaExit = true;
                    }
                    else
                    {
                        areaExit = false;
                    }
                    float ex = static_cast<float>(pExit->x);
                    float ey = static_cast<float>(pExit->y);
                    float ez = static_cast<float>(pExit->z);
                    QVector3D p1( ex, ey, ez );
                    QVector3D p2( rx, ry, rz );
                    glLoadIdentity();
                    gluLookAt(px*0.1+xRot, py*0.1+yRot, pz*0.1+zRot, px*0.1, py*0.1, pz*0.1,0.0,1.0,0.0);
                    glScalef( 0.1, 0.1, 0.1);
                    if( areaExit )
                        glLineWidth(1);//1/mScale+2);
                    else
                        glLineWidth(1);//1/mScale);
                    if( exitList[k] == mRID || ( ( rz == pz ) && ( rx == px ) && ( ry == py ) ) )
                    {
                        glDisable(GL_BLEND);
                        glEnable( GL_LIGHTING );
                        float mc3[] = { 1.0f, 0.0f, 0.0f, 1.0f };
                        glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mc3);
                        glMateriali(GL_FRONT, GL_SHININESS, 1);
                        glColor4f(1.0, 0.0, 0.0, 1.0);
                    }
                    else
                    {
                        glDisable(GL_BLEND);
                        glEnable( GL_LIGHTING );
                        glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, ebenenColor[ef]);
                        glMateriali(GL_FRONT, GL_SHININESS, 1);
                        glColor4f(0.3,0.3,0.3,1.0);//ebenenColor[ef][0],
//                                  ebenenColor[ef][1],
//                                  ebenenColor[ef][2],
//                                  ebenenColor[ef][3]);
                    }
                    glBegin(GL_LINES);
                    if( ! areaExit )
                    {
                        glVertex3f( p1.x(), p1.y(), p1.z() );
                    }
                    else
                    {
                        if( pR->getNorth() == exitList[k] )
                            glVertex3f( p2.x(), p2.y()+1, p2.z() );
                        else if( pR->getSouth() == exitList[k] )
                            glVertex3f( p2.x(), p2.y()-1, p2.z() );
                        else if( pR->getWest() == exitList[k] )
                            glVertex3f( p2.x()-1, p2.y(), p2.z() );
                        else if( pR->getEast() == exitList[k] )
                            glVertex3f( p2.x()+1, p2.y(), p2.z() );
                        else if( pR->getSouthwest() == exitList[k] )
                            glVertex3f( p2.x()-1, p2.y()-1, p2.z() );
                        else if( pR->getSoutheast() == exitList[k] )
                            glVertex3f( p2.x()+1, p2.y()-1, p2.z() );
                        else if( pR->getNortheast() == exitList[k] )
                            glVertex3f( p2.x()+1, p2.y()+1, p2.z() );
                        else if( pR->getNorthwest() == exitList[k] )
                            glVertex3f( p2.x()-1, p2.y()+1, p2.z() );
                        else if( pR->getUp() == exitList[k] )
                            glVertex3f( p2.x(), p2.y(), p2.z()+1 );
                        else if( pR->getDown() == exitList[k] )
                            glVertex3f( p2.x(), p2.y(), p2.z()-1 );
                    }
                    glVertex3f( p2.x(), p2.y(), p2.z() );
                    glEnd();
                    verts++;
                    if( areaExit )
                    {
                        glDisable(GL_BLEND);
                        glEnable( GL_LIGHTING );
                        glDisable(GL_LIGHT1);
                        float mc4[] = { 85.0/255.0, 170.0/255.0, 0.0/255.0, 1.0 };
                        glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mc4);
                        glMateriali(GL_FRONT, GL_SHININESS, 1);
                        glColor4f(85.0/255.0, 170.0/255.0, 0.0/255.0, 1.0);
                        glLoadIdentity();
                        gluLookAt(px*0.1+xRot, py*0.1+yRot, pz*0.1+zRot, px*0.1, py*0.1, pz*0.1,0.0,1.0,0.0);
                        glScalef( 0.1, 0.1, 0.1);
                        if( pR->getNorth() == exitList[k] )
                            glTranslatef( p2.x(), p2.y()+1, p2.z() );
                        else if( pR->getSouth() == exitList[k] )
                            glTranslatef( p2.x(), p2.y()-1, p2.z() );
                        else if( pR->getWest() == exitList[k] )
                            glTranslatef( p2.x()-1, p2.y(), p2.z() );
                        else if( pR->getEast() == exitList[k] )
                            glTranslatef( p2.x()+1, p2.y(), p2.z() );
                        else if( pR->getSouthwest() == exitList[k] )
                            glTranslatef( p2.x()-1, p2.y()-1, p2.z() );
                        else if( pR->getSoutheast() == exitList[k] )
                            glTranslatef( p2.x()+1, p2.y()-1, p2.z() );
                        else if( pR->getNortheast() == exitList[k] )
                            glTranslatef( p2.x()+1, p2.y()+1, p2.z() );
                        else if( pR->getNorthwest() == exitList[k] )
                            glTranslatef( p2.x()-1, p2.y()+1, p2.z() );
                        else if( pR->getUp() == exitList[k] )
                            glTranslatef( p2.x(), p2.y(), p2.z()+1 );
                        else if( pR->getDown() == exitList[k] )
                            glTranslatef( p2.x(), p2.y(), p2.z()-1 );

                        float mc6[] = { 85.0/255.0, 170.0/255.0, 0.0/255.0, 0.0 };
                        glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mc6);
                        glMateriali(GL_FRONT, GL_SHININESS, 96);

                        glLoadName( exitList[k] );
                        quads++;
                        glBegin( GL_QUADS );
                        glNormal3f(0.57735, -0.57735, 0.57735);
                        glVertex3f(1.0/dehnung, -1.0/dehnung, 1.0/dehnung);
                        glNormal3f(-0.57735, -0.57735, 0.57735);
                        glVertex3f(-1.0/dehnung, -1.0/dehnung, 1.0/dehnung);
                        glNormal3f(-0.57735, -0.57735, -0.57735);
                        glVertex3f(-1.0/dehnung, -1.0/dehnung, -1.0/dehnung);
                        glNormal3f(0.57735, -0.57735, -0.57735);
                        glVertex3f(1.0/dehnung, -1.0/dehnung, -1.0/dehnung);

                        glNormal3f(0.57735, 0.57735, 0.57735);
                        glVertex3f(1.0/dehnung, 1.0/dehnung, 1.0/dehnung);
                        glNormal3f(-0.57735, 0.57735, 0.57735);
                        glVertex3f(-1.0/dehnung, 1.0/dehnung, 1.0/dehnung);
                        glNormal3f(-0.57735, -0.57735, 0.57735);
                        glVertex3f(-1.0/dehnung, -1.0/dehnung, 1.0/dehnung);
                        glNormal3f(0.57735, -0.57735, 0.57735);
                        glVertex3f(1.0/dehnung, -1.0/dehnung, 1.0/dehnung);

                        glNormal3f(-0.57735, 0.57735, -0.57735);
                        glVertex3f(-1.0/dehnung, 1.0/dehnung, -1.0/dehnung);
                        glNormal3f(0.57735, 0.57735, -0.57735);
                        glVertex3f(1.0/dehnung, 1.0/dehnung, -1.0/dehnung);
                        glNormal3f(0.57735, -0.57735, -0.57735);
                        glVertex3f(1.0/dehnung, -1.0/dehnung, -1.0/dehnung);
                        glNormal3f(-0.57735, -0.57735, -0.57735);
                        glVertex3f(-1.0/dehnung, -1.0/dehnung, -1.0/dehnung);

                        glNormal3f(0.57735, 0.57735, -0.57735);
                        glVertex3f(1.0/dehnung, 1.0/dehnung, -1.0/dehnung);
                        glNormal3f(0.57735, 0.57735, 0.57735);
                        glVertex3f(1.0/dehnung, 1.0/dehnung, 1.0/dehnung);
                        glNormal3f(0.57735, -0.57735, 0.57735);
                        glVertex3f(1.0/dehnung, -1.0/dehnung, 1.0/dehnung);
                        glNormal3f(0.57735, -0.57735, -0.57735);
                        glVertex3f(1.0/dehnung, -1.0/dehnung, -1.0/dehnung);

                        glNormal3f(-0.57735, 0.57735, 0.57735);
                        glVertex3f(-1.0/dehnung, 1.0/dehnung, 1.0/dehnung);
                        glNormal3f(-0.57735, 0.57735, -0.57735);
                        glVertex3f(-1.0/dehnung, 1.0/dehnung, -1.0/dehnung);
                        glNormal3f(-0.57735, -0.57735, -0.57735);
                        glVertex3f(-1.0/dehnung, -1.0/dehnung, -1.0/dehnung);
                        glNormal3f(-0.57735, -0.57735, 0.57735);
                        glVertex3f(-1.0/dehnung, -1.0/dehnung, 1.0/dehnung);

                        glNormal3f(0.57735, 0.57735, -0.57735);
                        glVertex3f(1.0/dehnung, 1.0/dehnung, -1.0/dehnung);
                        glNormal3f(-0.57735, 0.57735, -0.57735);
                        glVertex3f(-1.0/dehnung, 1.0/dehnung, -1.0/dehnung);
                        glNormal3f(-0.57735, 0.57735, 0.57735);
                        glVertex3f(-1.0/dehnung, 1.0/dehnung, 1.0/dehnung);
                        glNormal3f(0.57735, 0.57735, 0.57735);
                        glVertex3f(1.0/dehnung, 1.0/dehnung, 1.0/dehnung);
                        glEnd();
                        //drauf
                        float mc3[] = { 0.2, 0.2, 0.6, 1.0 };
                        int env = pExit->environment;
                        if( mpMap->envColors.contains(env) )
                            env = mpMap->envColors[env];
                        else
                        {
                            if( ! mpMap->customEnvColors.contains(env))
                            {
                                env = 1;
                            }
                        }
                        switch( env )
                        {
                        case 1:
                            glColor4b(128,50,50,200);
                            mc3[0]=128.0/255.0; mc3[1]=0.0/255.0; mc3[2]=0.0/255.0; mc3[3]=0.2;
                            break;

                        case 2:
                            glColor4b(128,128,50, 200);
                            mc3[0]=0.0/255.0; mc3[1]=128.0/255.0; mc3[2]=0.0/255.0; mc3[3]=0.2;
                            break;
                        case 3:
                            glColor4b(50,128,50,200);
                            mc3[0]=128.0/255.0; mc3[1]=128.0/255.0; mc3[2]=0.0/255.0; mc3[3]=0.2;
                            break;

                        case 4:
                            glColor4b(50,50,128,200);
                            mc3[0]=0.0/255.0; mc3[1]=0.0/255.0; mc3[2]=128.0/255.0; mc3[3]=0.2;
                            break;

                        case 5:
                            glColor4b(128,50,128,200);
                            mc3[0]=128.0/255.0; mc3[1]=128.0/255.0; mc3[2]=0.0/255.0; mc3[3]=0.2;
                            break;
                        case 6:
                            glColor4b(50,128,128,200);
                            mc3[0]=0.0/255.0; mc3[1]=128.0/255.0; mc3[2]=128.0/255.0; mc3[3]=0.2;
                            break;
                        case 7:
                            glColor4b(52,38,78,200);
                            mc3[0]=128.0/255.0; mc3[1]=128.0/255.0; mc3[2]=128.0/255.0; mc3[3]=0.2;
                            break;
                        case 8:
                            glColor4b(65, 55, 35, 200);
                            mc3[0]=55.0/255.0; mc3[1]=55.0/255.0; mc3[2]=55.0/255.0; mc3[3]=0.2;
                            break;

                        case 9:
                            glColor4b(175,50,50,200);
                            mc3[0]=255.0/255.0; mc3[1]=50.0/255.0; mc3[2]=50.0/255.0; mc3[3]=0.2;
                            break;

                        case 10:
                            glColor4b(255,255,50,200);
                            mc3[0]=50.0/255.0; mc3[1]=255.0/255.0; mc3[2]=50.0/255.0; mc3[3]=0.2;
                            break;
                        case 11:
                            glColor4b(50,175,175,200);
                            mc3[0]=255.0/255.0; mc3[1]=255.0/255.0; mc3[2]=50.0/255.0; mc3[3]=0.2;
                            break;

                        case 12:
                            glColor4b(175,175,50,200);
                            mc3[0]=50.0/255.0; mc3[1]=50.0/255.0; mc3[2]=255.0/255.0; mc3[3]=0.2;
                            break;

                        case 13:
                            glColor4b(175,50,175,200);
                            mc3[0]=255.0/255.0; mc3[1]=50.0/255.0; mc3[2]=255.0/255.0; mc3[3]=0.2;
                            break;
                        case 14:
                            glColor4b(50,175,50,200);
                            mc3[0]=50.0/255.0; mc3[1]=255.0/255.0; mc3[2]=255.0/255.0; mc3[3]=0.2;
                            break;
                        case 15:
                            glColor4b(50,50,175,200);
                            mc3[0]=255.0/255.0; mc3[1]=255.0/255.0; mc3[2]=255.0/255.0; mc3[3]=0.2;
                            break;
                        default: //user defined room color
                            if( ! mpMap->customEnvColors.contains(env) ) break;
                            QColor &_c = mpMap->customEnvColors[env];
                            glColor4b(_c.red(),_c.green(),_c.blue(),25);
                            mc3[0]=_c.redF();
                            mc3[1]=_c.greenF();
                            mc3[2]=_c.blueF();
                            mc3[3]=0.2;
                        }
                        glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mc3);
                        glMateriali(GL_FRONT, GL_SHININESS, 1);
                        glDisable(GL_DEPTH_TEST);
                        glLoadIdentity();
                        gluLookAt(px*0.1+xRot, py*0.1+yRot, pz*0.1+zRot, px*0.1, py*0.1, pz*0.1,0.0,1.0,0.0);
                        glScalef( 0.05, 0.05, 0.020);
                        if( pR->getNorth() == exitList[k] )
                            glTranslatef( 2*p2.x(), 2*(p2.y()+1), 5.0*(p2.z()+0.25) );
                        else if( pR->getSouth() == exitList[k] )
                            glTranslatef( 2*p2.x(), 2*(p2.y()-1), 5.0*(p2.z()+0.25) );
                        else if( pR->getWest() == exitList[k] )
                            glTranslatef( 2*(p2.x()-1), 2*p2.y(), 5.0*(p2.z()+0.25) );
                        else if( pR->getEast() == exitList[k] )
                            glTranslatef( 2*(p2.x()+1), 2*p2.y(), 5.0*(p2.z()+0.25) );
                        else if( pR->getSouthwest() == exitList[k] )
                            glTranslatef( 2*(p2.x()-1), 2*(p2.y()-1), 5.0*(p2.z()+0.25) );
                        else if( pR->getSoutheast() == exitList[k] )
                            glTranslatef( 2*(p2.x()+1), 2*(p2.y()-1), 5.0*(p2.z()+0.25) );
                        else if( pR->getNortheast() == exitList[k] )
                            glTranslatef( 2*(p2.x()+1), 2*(p2.y()+1), 5.0*(p2.z()+0.25) );
                        else if( pR->getNorthwest() == exitList[k] )
                            glTranslatef( 2*(p2.x()-1), 2*(p2.y()+1), 5.0*(p2.z()+0.25) );
                        else if( pR->getUp() == exitList[k] )
                            glTranslatef( 2*p2.x(), 2*p2.y(), 5.0*(p2.z()+1+0.25) );
                        else if( pR->getDown() == exitList[k] )
                            glTranslatef( 2*p2.x(), 2*p2.y(), 5.0*(p2.z()-1+0.25) );

                        glBegin( GL_QUADS );
                        glNormal3f(0.57735, -0.57735, 0.57735);
                        glVertex3f(1.0/dehnung, -1.0/dehnung, 1.0/dehnung);
                        glNormal3f(-0.57735, -0.57735, 0.57735);
                        glVertex3f(-1.0/dehnung, -1.0/dehnung, 1.0/dehnung);
                        glNormal3f(-0.57735, -0.57735, -0.57735);
                        glVertex3f(-1.0/dehnung, -1.0/dehnung, -1.0/dehnung);
                        glNormal3f(0.57735, -0.57735, -0.57735);
                        glVertex3f(1.0/dehnung, -1.0/dehnung, -1.0/dehnung);

                        glNormal3f(0.57735, 0.57735, 0.57735);
                        glVertex3f(1.0/dehnung, 1.0/dehnung, 1.0/dehnung);
                        glNormal3f(-0.57735, 0.57735, 0.57735);
                        glVertex3f(-1.0/dehnung, 1.0/dehnung, 1.0/dehnung);
                        glNormal3f(-0.57735, -0.57735, 0.57735);
                        glVertex3f(-1.0/dehnung, -1.0/dehnung, 1.0/dehnung);
                        glNormal3f(0.57735, -0.57735, 0.57735);
                        glVertex3f(1.0/dehnung, -1.0/dehnung, 1.0/dehnung);

                        glNormal3f(-0.57735, 0.57735, -0.57735);
                        glVertex3f(-1.0/dehnung, 1.0/dehnung, -1.0/dehnung);
                        glNormal3f(0.57735, 0.57735, -0.57735);
                        glVertex3f(1.0/dehnung, 1.0/dehnung, -1.0/dehnung);
                        glNormal3f(0.57735, -0.57735, -0.57735);
                        glVertex3f(1.0/dehnung, -1.0/dehnung, -1.0/dehnung);
                        glNormal3f(-0.57735, -0.57735, -0.57735);
                        glVertex3f(-1.0/dehnung, -1.0/dehnung, -1.0/dehnung);

                        glNormal3f(0.57735, 0.57735, -0.57735);
                        glVertex3f(1.0/dehnung, 1.0/dehnung, -1.0/dehnung);
                        glNormal3f(0.57735, 0.57735, 0.57735);
                        glVertex3f(1.0/dehnung, 1.0/dehnung, 1.0/dehnung);
                        glNormal3f(0.57735, -0.57735, 0.57735);
                        glVertex3f(1.0/dehnung, -1.0/dehnung, 1.0/dehnung);
                        glNormal3f(0.57735, -0.57735, -0.57735);
                        glVertex3f(1.0/dehnung, -1.0/dehnung, -1.0/dehnung);

                        glNormal3f(-0.57735, 0.57735, 0.57735);
                        glVertex3f(-1.0/dehnung, 1.0/dehnung, 1.0/dehnung);
                        glNormal3f(-0.57735, 0.57735, -0.57735);
                        glVertex3f(-1.0/dehnung, 1.0/dehnung, -1.0/dehnung);
                        glNormal3f(-0.57735, -0.57735, -0.57735);
                        glVertex3f(-1.0/dehnung, -1.0/dehnung, -1.0/dehnung);
                        glNormal3f(-0.57735, -0.57735, 0.57735);
                        glVertex3f(-1.0/dehnung, -1.0/dehnung, 1.0/dehnung);

                        glNormal3f(0.57735, 0.57735, -0.57735);
                        glVertex3f(1.0/dehnung, 1.0/dehnung, -1.0/dehnung);
                        glNormal3f(-0.57735, 0.57735, -0.57735);
                        glVertex3f(-1.0/dehnung, 1.0/dehnung, -1.0/dehnung);
                        glNormal3f(-0.57735, 0.57735, 0.57735);
                        glVertex3f(-1.0/dehnung, 1.0/dehnung, 1.0/dehnung);
                        glNormal3f(0.57735, 0.57735, 0.57735);
                        glVertex3f(1.0/dehnung, 1.0/dehnung, 1.0/dehnung);
                        glEnd();
                    }

                }
            }
            else
            {
                for( int k=0; k<exitList.size(); k++ )
                {
                    bool areaExit = false;
                    if( exitList[k] == -1 ) continue;
                    TRoom * pExit = mpMap->mpRoomDB->getRoom( exitList[k] );
                    if( !pExit )
                    {
                        continue;
                    }
                    if( pExit->getArea() != mAID )
                    {
                        areaExit = true;
                    }
                    else
                    {
                        areaExit = true;
                    }

                    float ex = static_cast<float>(pExit->x);
                    float ey = static_cast<float>(pExit->y);
                    float ez = static_cast<float>(pExit->z);
                    QVector3D p1( ex, ey, ez );
                    QVector3D p2( rx, ry, rz );
                    glLoadIdentity();
                    gluLookAt(px*0.1+xRot, py*0.1+yRot, pz*0.1+zRot, px*0.1, py*0.1, pz*0.1,0.0,1.0,0.0);
                    glScalef( 0.1, 0.1, 0.1);
                    if( areaExit )
                        glLineWidth(1);//1/mScale+2);
                    else
                        glLineWidth(1);//1/mScale);
                    if( exitList[k] == mRID || ( ( rz == pz ) && ( rx == px ) && ( ry == py ) ) )
                    {
                        glDisable(GL_BLEND);
                        glEnable( GL_LIGHTING );
                        float mc3[] = { 1.0f, 0.0f, 0.0f, 1.0f };
                        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, mc3);
                        glMateriali(GL_FRONT_AND_BACK, GL_SHININESS, 1);
                        glColor4f(1.0, 0.0, 0.0, 1.0);
                    }
                    else
                    {
                        glEnable(GL_BLEND);
                        glEnable( GL_LIGHTING );
                        glEnable(GL_LIGHT1);
                        glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, ebenenColor2[ef]);
                        glMateriali(GL_FRONT, GL_SHININESS, 1);//gut:36
                        glColor4f(0.3,0.3,0.3,1.0);/*ebenenColor2[ef][0],
                                  ebenenColor2[ef][1],
                                  ebenenColor2[ef][2],
                                  ebenenColor2[ef][3])*/;
                    }
                    glBegin(GL_LINES);
                    if( ! areaExit )
                    {
                        glVertex3f( p1.x(), p1.y(), p1.z() );
                    }
                    else
                    {
                        if( pR->getNorth() == exitList[k] )
                            glVertex3f( p2.x(), p2.y()+1, p2.z() );
                        else if( pR->getSouth() == exitList[k] )
                            glVertex3f( p2.x(), p2.y()-1, p2.z() );
                        else if( pR->getWest() == exitList[k] )
                            glVertex3f( p2.x()-1, p2.y(), p2.z() );
                        else if( pR->getEast() == exitList[k] )
                            glVertex3f( p2.x()+1, p2.y(), p2.z() );
                        else if( pR->getSouthwest() == exitList[k] )
                            glVertex3f( p2.x()-1, p2.y()-1, p2.z() );
                        else if( pR->getSoutheast() == exitList[k] )
                            glVertex3f( p2.x()+1, p2.y()-1, p2.z() );
                        else if( pR->getNortheast() == exitList[k] )
                            glVertex3f( p2.x()+1, p2.y()-1, p2.z() );
                        else if( pR->getNorthwest() == exitList[k] )
                            glVertex3f( p2.x()-1, p2.y()+1, p2.z() );
                        else if( pR->getUp() == exitList[k] )
                            glVertex3f( p2.x(), p2.y(), p2.z()+1 );
                        else if( pR->getDown() == exitList[k] )
                            glVertex3f( p2.x(), p2.y(), p2.z()-1 );
                    }
                    glVertex3f( p2.x(), p2.y(), p2.z() );
                    glEnd();
                    verts++;
                    if( areaExit )
                    {
                        glDisable(GL_BLEND);
                        glEnable( GL_LIGHTING );
                        glDisable(GL_LIGHT1);
                        float mc4[] = { 85.0/255.0, 170.0/255.0, 0.0/255.0, 1.0 };
                        glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mc4);
                        glMateriali(GL_FRONT, GL_SHININESS, 1);
                        glColor4f(85.0/255.0, 170.0/255.0, 0.0/255.0, 1.0);
                        glLoadIdentity();
                        gluLookAt(px*0.1+xRot, py*0.1+yRot, pz*0.1+zRot, px*0.1, py*0.1, pz*0.1,0.0,1.0,0.0);
                        glScalef( 0.1, 0.1, 0.1);
                        if( pR->getNorth() == exitList[k] )
                            glTranslatef( p2.x(), p2.y()+1, p2.z() );
                        else if( pR->getSouth() == exitList[k] )
                            glTranslatef( p2.x(), p2.y()-1, p2.z() );
                        else if( pR->getWest() == exitList[k] )
                            glTranslatef( p2.x()-1, p2.y(), p2.z() );
                        else if( pR->getEast() == exitList[k] )
                            glTranslatef( p2.x()+1, p2.y(), p2.z() );
                        else if( pR->getSouthwest() == exitList[k] )
                            glTranslatef( p2.x()-1, p2.y()-1, p2.z() );
                        else if( pR->getSoutheast() == exitList[k] )
                            glTranslatef( p2.x()+1, p2.y()-1, p2.z() );
                        else if( pR->getNortheast() == exitList[k] )
                            glTranslatef( p2.x()+1, p2.y()+1, p2.z() );
                        else if( pR->getNorthwest() == exitList[k] )
                            glTranslatef( p2.x()-1, p2.y()+1, p2.z() );
                        else if( pR->getUp() == exitList[k] )
                            glTranslatef( p2.x(), p2.y(), p2.z()+1 );
                        else if( pR->getDown() == exitList[k] )
                            glTranslatef( p2.x(), p2.y(), p2.z()-1 );

                        glLoadName( exitList[k] );
                        quads++;
                        glBegin( GL_QUADS );
                        glNormal3f(0.57735, -0.57735, 0.57735);
                        glVertex3f(1.0/dehnung, -1.0/dehnung, 1.0/dehnung);
                        glNormal3f(-0.57735, -0.57735, 0.57735);
                        glVertex3f(-1.0/dehnung, -1.0/dehnung, 1.0/dehnung);
                        glNormal3f(-0.57735, -0.57735, -0.57735);
                        glVertex3f(-1.0/dehnung, -1.0/dehnung, -1.0/dehnung);
                        glNormal3f(0.57735, -0.57735, -0.57735);
                        glVertex3f(1.0/dehnung, -1.0/dehnung, -1.0/dehnung);

                        glNormal3f(0.57735, 0.57735, 0.57735);
                        glVertex3f(1.0/dehnung, 1.0/dehnung, 1.0/dehnung);
                        glNormal3f(-0.57735, 0.57735, 0.57735);
                        glVertex3f(-1.0/dehnung, 1.0/dehnung, 1.0/dehnung);
                        glNormal3f(-0.57735, -0.57735, 0.57735);
                        glVertex3f(-1.0/dehnung, -1.0/dehnung, 1.0/dehnung);
                        glNormal3f(0.57735, -0.57735, 0.57735);
                        glVertex3f(1.0/dehnung, -1.0/dehnung, 1.0/dehnung);

                        glNormal3f(-0.57735, 0.57735, -0.57735);
                        glVertex3f(-1.0/dehnung, 1.0/dehnung, -1.0/dehnung);
                        glNormal3f(0.57735, 0.57735, -0.57735);
                        glVertex3f(1.0/dehnung, 1.0/dehnung, -1.0/dehnung);
                        glNormal3f(0.57735, -0.57735, -0.57735);
                        glVertex3f(1.0/dehnung, -1.0/dehnung, -1.0/dehnung);
                        glNormal3f(-0.57735, -0.57735, -0.57735);
                        glVertex3f(-1.0/dehnung, -1.0/dehnung, -1.0/dehnung);

                        glNormal3f(0.57735, 0.57735, -0.57735);
                        glVertex3f(1.0/dehnung, 1.0/dehnung, -1.0/dehnung);
                        glNormal3f(0.57735, 0.57735, 0.57735);
                        glVertex3f(1.0/dehnung, 1.0/dehnung, 1.0/dehnung);
                        glNormal3f(0.57735, -0.57735, 0.57735);
                        glVertex3f(1.0/dehnung, -1.0/dehnung, 1.0/dehnung);
                        glNormal3f(0.57735, -0.57735, -0.57735);
                        glVertex3f(1.0/dehnung, -1.0/dehnung, -1.0/dehnung);

                        glNormal3f(-0.57735, 0.57735, 0.57735);
                        glVertex3f(-1.0/dehnung, 1.0/dehnung, 1.0/dehnung);
                        glNormal3f(-0.57735, 0.57735, -0.57735);
                        glVertex3f(-1.0/dehnung, 1.0/dehnung, -1.0/dehnung);
                        glNormal3f(-0.57735, -0.57735, -0.57735);
                        glVertex3f(-1.0/dehnung, -1.0/dehnung, -1.0/dehnung);
                        glNormal3f(-0.57735, -0.57735, 0.57735);
                        glVertex3f(-1.0/dehnung, -1.0/dehnung, 1.0/dehnung);

                        glNormal3f(0.57735, 0.57735, -0.57735);
                        glVertex3f(1.0/dehnung, 1.0/dehnung, -1.0/dehnung);
                        glNormal3f(-0.57735, 0.57735, -0.57735);
                        glVertex3f(-1.0/dehnung, 1.0/dehnung, -1.0/dehnung);
                        glNormal3f(-0.57735, 0.57735, 0.57735);
                        glVertex3f(-1.0/dehnung, 1.0/dehnung, 1.0/dehnung);
                        glNormal3f(0.57735, 0.57735, 0.57735);
                        glVertex3f(1.0/dehnung, 1.0/dehnung, 1.0/dehnung);
                        glEnd();

                        //drauf
                        float mc3[] = { 0.2, 0.2, 0.6, 0.2 };
                        int env = pExit->environment;
                        if( mpMap->envColors.contains(env) )
                            env = mpMap->envColors[env];
                        else
                        {
                            if( ! mpMap->customEnvColors.contains(env))
                            {
                                env = 1;
                            }
                        }
                        switch( env )
                        {
                        case 1:
                            glColor4b(128,50,50,2);
                            mc3[0]=128.0/255.0; mc3[1]=0.0/255.0; mc3[2]=0.0/255.0; mc3[3]=0.2;
                            break;

                        case 2:
                            glColor4b(128,128,50, 2);
                            mc3[0]=0.0/255.0; mc3[1]=128.0/255.0; mc3[2]=0.0/255.0; mc3[3]=0.2;
                            break;
                        case 3:
                            glColor4b(50,128,50,2);
                            mc3[0]=128.0/255.0; mc3[1]=128.0/255.0; mc3[2]=0.0/255.0; mc3[3]=0.2;
                            break;

                        case 4:
                            glColor4b(50,50,128,2);
                            mc3[0]=0.0/255.0; mc3[1]=0.0/255.0; mc3[2]=128.0/255.0; mc3[3]=0.2;
                            break;

                        case 5:
                            glColor4b(128,50,128,2);
                            mc3[0]=128.0/255.0; mc3[1]=128.0/255.0; mc3[2]=0.0/255.0; mc3[3]=0.2;
                            break;
                        case 6:
                            glColor4b(50,128,128,2);
                            mc3[0]=0.0/255.0; mc3[1]=128.0/255.0; mc3[2]=128.0/255.0; mc3[3]=0.2;
                            break;
                        case 7:
                            glColor4b(52,38,78,2);
                            mc3[0]=128.0/255.0; mc3[1]=128.0/255.0; mc3[2]=128.0/255.0; mc3[3]=0.2;
                            break;
                        case 8:
                            glColor4b(65, 55, 35, 2);
                            mc3[0]=55.0/255.0; mc3[1]=55.0/255.0; mc3[2]=55.0/255.0; mc3[3]=0.2;
                            break;

                        case 9:
                            glColor4b(175,50,50,2);
                            mc3[0]=255.0/255.0; mc3[1]=50.0/255.0; mc3[2]=50.0/255.0; mc3[3]=0.2;
                            break;

                        case 10:
                            glColor4b(255,255,50,2);
                            mc3[0]=50.0/255.0; mc3[1]=255.0/255.0; mc3[2]=50.0/255.0; mc3[3]=0.2;
                            break;
                        case 11:
                            glColor4b(50,175,175,2);
                            mc3[0]=255.0/255.0; mc3[1]=255.0/255.0; mc3[2]=50.0/255.0; mc3[3]=0.2;
                            break;

                        case 12:
                            glColor4b(175,175,50,2);
                            mc3[0]=50.0/255.0; mc3[1]=50.0/255.0; mc3[2]=255.0/255.0; mc3[3]=0.2;
                            break;

                        case 13:
                            glColor4b(175,50,175,2);
                            mc3[0]=255.0/255.0; mc3[1]=50.0/255.0; mc3[2]=255.0/255.0; mc3[3]=0.2;
                            break;
                        case 14:
                            glColor4b(50,175,50,2);
                            mc3[0]=50.0/255.0; mc3[1]=255.0/255.0; mc3[2]=255.0/255.0; mc3[3]=0.2;
                            break;
                        case 15:
                            glColor4b(50,50,175,2);
                            mc3[0]=255.0/255.0; mc3[1]=255.0/255.0; mc3[2]=255.0/255.0; mc3[3]=0.2;
                            break;
                        default: //user defined room color
                            if( ! mpMap->customEnvColors.contains(env) ) break;
                            QColor &_c = mpMap->customEnvColors[env];
                            glColor4b(_c.red(),_c.green(),_c.blue(),255);
                            mc3[0]=_c.redF();
                            mc3[1]=_c.greenF();
                            mc3[2]=_c.blueF();
                            mc3[3]=0.2;
                        }
                        glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mc3);
                        glMateriali(GL_FRONT, GL_SHININESS, 36);
                        glDisable(GL_DEPTH_TEST);
                        glLoadIdentity();
                        gluLookAt(px*0.1+xRot, py*0.1+yRot, pz*0.1+zRot, px*0.1, py*0.1, pz*0.1,0.0,1.0,0.0);
                        glScalef( 0.05, 0.05, 0.020);
                        if( pR->getNorth() == exitList[k] )
                            glTranslatef( 2*p2.x(), 2*(p2.y()+1), 5.0*(p2.z()+0.25) );
                        else if( pR->getSouth() == exitList[k] )
                            glTranslatef( 2*p2.x(), 2*(p2.y()-1), 5.0*(p2.z()+0.25) );
                        else if( pR->getWest() == exitList[k] )
                            glTranslatef( 2*(p2.x()-1), 2*p2.y(), 5.0*(p2.z()+0.25) );
                        else if( pR->getEast() == exitList[k] )
                            glTranslatef( 2*(p2.x()+1), 2*p2.y(), 5.0*(p2.z()+0.25) );
                        else if( pR->getSouthwest() == exitList[k] )
                            glTranslatef( 2*(p2.x()-1), 2*(p2.y()-1), 5.0*(p2.z()+0.25) );
                        else if( pR->getSoutheast() == exitList[k] )
                            glTranslatef( 2*(p2.x()+1), 2*(p2.y()-1), 5.0*(p2.z()+0.25) );
                        else if( pR->getNortheast() == exitList[k] )
                            glTranslatef( 2*(p2.x()+1), 2*(p2.y()+1), 5.0*(p2.z()+0.25) );
                        else if( pR->getNorthwest() == exitList[k] )
                            glTranslatef( 2*(p2.x()-1), 2*(p2.y()+1), 5.0*(p2.z()+0.25) );
                        else if( pR->getUp() == exitList[k] )
                            glTranslatef( 2*p2.x(), 2*p2.y(), 5.0*(p2.z()+1+0.25) );
                        else if( pR->getDown() == exitList[k] )
                            glTranslatef( 2*p2.x(), 2*p2.y(), 5.0*(p2.z()-1+0.25) );

                        glBegin( GL_QUADS );
                        glNormal3f(0.57735, -0.57735, 0.57735);
                        glVertex3f(1.0/dehnung, -1.0/dehnung, 1.0/dehnung);
                        glNormal3f(-0.57735, -0.57735, 0.57735);
                        glVertex3f(-1.0/dehnung, -1.0/dehnung, 1.0/dehnung);
                        glNormal3f(-0.57735, -0.57735, -0.57735);
                        glVertex3f(-1.0/dehnung, -1.0/dehnung, -1.0/dehnung);
                        glNormal3f(0.57735, -0.57735, -0.57735);
                        glVertex3f(1.0/dehnung, -1.0/dehnung, -1.0/dehnung);

                        glNormal3f(0.57735, 0.57735, 0.57735);
                        glVertex3f(1.0/dehnung, 1.0/dehnung, 1.0/dehnung);
                        glNormal3f(-0.57735, 0.57735, 0.57735);
                        glVertex3f(-1.0/dehnung, 1.0/dehnung, 1.0/dehnung);
                        glNormal3f(-0.57735, -0.57735, 0.57735);
                        glVertex3f(-1.0/dehnung, -1.0/dehnung, 1.0/dehnung);
                        glNormal3f(0.57735, -0.57735, 0.57735);
                        glVertex3f(1.0/dehnung, -1.0/dehnung, 1.0/dehnung);

                        glNormal3f(-0.57735, 0.57735, -0.57735);
                        glVertex3f(-1.0/dehnung, 1.0/dehnung, -1.0/dehnung);
                        glNormal3f(0.57735, 0.57735, -0.57735);
                        glVertex3f(1.0/dehnung, 1.0/dehnung, -1.0/dehnung);
                        glNormal3f(0.57735, -0.57735, -0.57735);
                        glVertex3f(1.0/dehnung, -1.0/dehnung, -1.0/dehnung);
                        glNormal3f(-0.57735, -0.57735, -0.57735);
                        glVertex3f(-1.0/dehnung, -1.0/dehnung, -1.0/dehnung);

                        glNormal3f(0.57735, 0.57735, -0.57735);
                        glVertex3f(1.0/dehnung, 1.0/dehnung, -1.0/dehnung);
                        glNormal3f(0.57735, 0.57735, 0.57735);
                        glVertex3f(1.0/dehnung, 1.0/dehnung, 1.0/dehnung);
                        glNormal3f(0.57735, -0.57735, 0.57735);
                        glVertex3f(1.0/dehnung, -1.0/dehnung, 1.0/dehnung);
                        glNormal3f(0.57735, -0.57735, -0.57735);
                        glVertex3f(1.0/dehnung, -1.0/dehnung, -1.0/dehnung);

                        glNormal3f(-0.57735, 0.57735, 0.57735);
                        glVertex3f(-1.0/dehnung, 1.0/dehnung, 1.0/dehnung);
                        glNormal3f(-0.57735, 0.57735, -0.57735);
                        glVertex3f(-1.0/dehnung, 1.0/dehnung, -1.0/dehnung);
                        glNormal3f(-0.57735, -0.57735, -0.57735);
                        glVertex3f(-1.0/dehnung, -1.0/dehnung, -1.0/dehnung);
                        glNormal3f(-0.57735, -0.57735, 0.57735);
                        glVertex3f(-1.0/dehnung, -1.0/dehnung, 1.0/dehnung);

                        glNormal3f(0.57735, 0.57735, -0.57735);
                        glVertex3f(1.0/dehnung, 1.0/dehnung, -1.0/dehnung);
                        glNormal3f(-0.57735, 0.57735, -0.57735);
                        glVertex3f(-1.0/dehnung, 1.0/dehnung, -1.0/dehnung);
                        glNormal3f(-0.57735, 0.57735, 0.57735);
                        glVertex3f(-1.0/dehnung, 1.0/dehnung, 1.0/dehnung);
                        glNormal3f(0.57735, 0.57735, 0.57735);
                        glVertex3f(1.0/dehnung, 1.0/dehnung, 1.0/dehnung);
                        glEnd();
                    }

                }
                glFlush();

            }
        }

        if( zRot <= 0)
            zEbene -= 1.0;
        else
            zEbene += 1.0;
    }

    if( zRot <= 0 )
        zEbene = zmax;
    else
        zEbene = zmin;

    quads = 0;
    zEbene = zmin;
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glDisable(GL_LIGHT1);

    while( true )
    {
        if( zEbene > zmax )
        {
            break;
        }
        for( int i=0; i<pArea->rooms.size(); i++ )
        {
            glDisable(GL_LIGHT1);
            TRoom * pR = mpMap->mpRoomDB->getRoom( pArea->rooms[i] );
            if( !pR ) continue;
            float rx = static_cast<float>(pR->x);
            float ry = static_cast<float>(pR->y);
            float rz = static_cast<float>(pR->z);
            if( rz != zEbene ) continue;

            if( rz > pz )
                if( abs(rz-pz) > mShowTopLevels ) continue;
            if( rz < pz )
                if( abs(rz-pz) > mShowBottomLevels ) continue;

            int e = pR->z;
            int ef = abs(e%26);
            glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, ebenenColor[ef]);
            glMateriali(GL_FRONT, GL_SHININESS, 36);//gut:96

            if( ( rz == pz ) && ( rx == px ) && ( ry == py ) )
            {
                glDisable(GL_BLEND);
                glEnable( GL_LIGHTING );
                glDisable(GL_LIGHT1);
                float mc3[] = { 1.0f, 0.0f, 0.0f, 1.0f };
                glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mc3);
                glMateriali(GL_FRONT, GL_SHININESS, 36);
                glColor4f(1.0, 0.0, 0.0, 1.0);
            }
            else if( pArea->rooms[i] == mTarget )
            {
                glDisable(GL_BLEND);
                glEnable( GL_LIGHTING );
                glDisable(GL_LIGHT1);
                float mc4[] = { 0.0, 1.0, 0.0, 1.0 };
                glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mc4);
                glMateriali(GL_FRONT, GL_SHININESS, 36);//36
                glColor4f(0.0, 1.0, 0.0, 1.0);
            }
            else if( rz <= pz )
            {
                glDisable(GL_BLEND);
                glEnable( GL_LIGHTING );
                glDisable(GL_LIGHT1);
                glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, ebenenColor2[ef]);
                glMateriali(GL_FRONT, GL_SHININESS, 36);
                glColor4f(ebenenColor[ef][0],
                          ebenenColor[ef][1],
                          ebenenColor[ef][2],
                          ebenenColor[ef][3]);
            }
            else
            {
                glEnable(GL_BLEND);
                glEnable( GL_LIGHTING );
                glEnable(GL_LIGHT1);
                glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, ebenenColor[ef]);
                glMateriali(GL_FRONT, GL_SHININESS, 36);//56);//gut:36
                glColor4f(ebenenColor2[ef][0],
                          ebenenColor2[ef][1],
                          ebenenColor2[ef][2],
                          ebenenColor2[ef][3]);

                glLoadIdentity();
                gluLookAt(px*0.1+xRot, py*0.1+yRot, pz*0.1+zRot, px*0.1, py*0.1, pz*0.1,0.0,1.0,0.0);
                if( pArea->gridMode )
                {
                    glScalef( 0.2, 0.2, 0.1);
                    glTranslatef( 0.5*rx, 0.5*ry, rz );
                }
                else
                {
                    glScalef( 0.1, 0.1, 0.1);
                    glTranslatef( rx, ry, rz );
                }

                glLoadName( pArea->rooms[i] );
                quads++;
                glBegin( GL_QUADS );
                glNormal3f(0.57735, -0.57735, 0.57735);
                glVertex3f(1.0/dehnung, -1.0/dehnung, 1.0/dehnung);
                glNormal3f(-0.57735, -0.57735, 0.57735);
                glVertex3f(-1.0/dehnung, -1.0/dehnung, 1.0/dehnung);
                glNormal3f(-0.57735, -0.57735, -0.57735);
                glVertex3f(-1.0/dehnung, -1.0/dehnung, -1.0/dehnung);
                glNormal3f(0.57735, -0.57735, -0.57735);
                glVertex3f(1.0/dehnung, -1.0/dehnung, -1.0/dehnung);


                glNormal3f(0.57735, 0.57735, 0.57735);
                glVertex3f(1.0/dehnung, 1.0/dehnung, 1.0/dehnung);
                glNormal3f(-0.57735, 0.57735, 0.57735);
                glVertex3f(-1.0/dehnung, 1.0/dehnung, 1.0/dehnung);
                glNormal3f(-0.57735, -0.57735, 0.57735);
                glVertex3f(-1.0/dehnung, -1.0/dehnung, 1.0/dehnung);
                glNormal3f(0.57735, -0.57735, 0.57735);
                glVertex3f(1.0/dehnung, -1.0/dehnung, 1.0/dehnung);

                glNormal3f(-0.57735, 0.57735, -0.57735);
                glVertex3f(-1.0/dehnung, 1.0/dehnung, -1.0/dehnung);
                glNormal3f(0.57735, 0.57735, -0.57735);
                glVertex3f(1.0/dehnung, 1.0/dehnung, -1.0/dehnung);
                glNormal3f(0.57735, -0.57735, -0.57735);
                glVertex3f(1.0/dehnung, -1.0/dehnung, -1.0/dehnung);
                glNormal3f(-0.57735, -0.57735, -0.57735);
                glVertex3f(-1.0/dehnung, -1.0/dehnung, -1.0/dehnung);

                glNormal3f(0.57735, 0.57735, -0.57735);
                glVertex3f(1.0/dehnung, 1.0/dehnung, -1.0/dehnung);
                glNormal3f(0.57735, 0.57735, 0.57735);
                glVertex3f(1.0/dehnung, 1.0/dehnung, 1.0/dehnung);
                glNormal3f(0.57735, -0.57735, 0.57735);
                glVertex3f(1.0/dehnung, -1.0/dehnung, 1.0/dehnung);
                glNormal3f(0.57735, -0.57735, -0.57735);
                glVertex3f(1.0/dehnung, -1.0/dehnung, -1.0/dehnung);

                glNormal3f(-0.57735, 0.57735, 0.57735);
                glVertex3f(-1.0/dehnung, 1.0/dehnung, 1.0/dehnung);
                glNormal3f(-0.57735, 0.57735, -0.57735);
                glVertex3f(-1.0/dehnung, 1.0/dehnung, -1.0/dehnung);
                glNormal3f(-0.57735, -0.57735, -0.57735);
                glVertex3f(-1.0/dehnung, -1.0/dehnung, -1.0/dehnung);
                glNormal3f(-0.57735, -0.57735, 0.57735);
                glVertex3f(-1.0/dehnung, -1.0/dehnung, 1.0/dehnung);

                glNormal3f(0.57735, 0.57735, -0.57735);
                glVertex3f(1.0/dehnung, 1.0/dehnung, -1.0/dehnung);
                glNormal3f(-0.57735, 0.57735, -0.57735);
                glVertex3f(-1.0/dehnung, 1.0/dehnung, -1.0/dehnung);
                glNormal3f(-0.57735, 0.57735, 0.57735);
                glVertex3f(-1.0/dehnung, 1.0/dehnung, 1.0/dehnung);
                glNormal3f(0.57735, 0.57735, 0.57735);
                glVertex3f(1.0/dehnung, 1.0/dehnung, 1.0/dehnung);
                glEnd();

                float mc3[] = { 0.2, 0.2, 0.6, 0.2 };
                int env = pR->environment;
                if( mpMap->envColors.contains(env) )
                    env = mpMap->envColors[env];
                else
                {
                    if( ! mpMap->customEnvColors.contains(env))
                    {
                        env = 1;
                    }
                }

                switch( env )
                {
                case 1:
                    glColor4b(128,50,50,2);
                    mc3[0]=128.0/255.0; mc3[1]=0.0/255.0; mc3[2]=0.0/255.0; mc3[3]=0.2;
                    break;

                case 2:
                    glColor4b(128,128,50, 2);
                    mc3[0]=0.0/255.0; mc3[1]=128.0/255.0; mc3[2]=0.0/255.0; mc3[3]=0.2;
                    break;
                case 3:
                    glColor4b(50,128,50,2);
                    mc3[0]=128.0/255.0; mc3[1]=128.0/255.0; mc3[2]=0.0/255.0; mc3[3]=0.2;
                    break;

                case 4:
                    glColor4b(50,50,128,2);
                    mc3[0]=0.0/255.0; mc3[1]=0.0/255.0; mc3[2]=128.0/255.0; mc3[3]=0.2;
                    break;

                case 5:
                    glColor4b(128,50,128,2);
                    mc3[0]=128.0/255.0; mc3[1]=128.0/255.0; mc3[2]=0.0/255.0; mc3[3]=0.2;
                    break;
                case 6:
                    glColor4b(50,128,128,2);
                    mc3[0]=0.0/255.0; mc3[1]=128.0/255.0; mc3[2]=128.0/255.0; mc3[3]=0.2;
                    break;
                case 7:
                    glColor4b(52,38,78,2);
                    mc3[0]=128.0/255.0; mc3[1]=128.0/255.0; mc3[2]=128.0/255.0; mc3[3]=0.2;
                    break;
                case 8:
                    glColor4b(65, 55, 35, 2);
                    mc3[0]=55.0/255.0; mc3[1]=55.0/255.0; mc3[2]=55.0/255.0; mc3[3]=0.2;
                    break;

                case 9:
                    glColor4b(175,50,50,2);
                    mc3[0]=255.0/255.0; mc3[1]=50.0/255.0; mc3[2]=50.0/255.0; mc3[3]=0.2;
                    break;

                case 10:
                    glColor4b(255,255,50,2);
                    mc3[0]=50.0/255.0; mc3[1]=255.0/255.0; mc3[2]=50.0/255.0; mc3[3]=0.2;
                    break;
                case 11:
                    glColor4b(50,175,175,2);
                    mc3[0]=255.0/255.0; mc3[1]=255.0/255.0; mc3[2]=50.0/255.0; mc3[3]=0.2;
                    break;

                case 12:
                    glColor4b(175,175,50,2);
                    mc3[0]=50.0/255.0; mc3[1]=50.0/255.0; mc3[2]=255.0/255.0; mc3[3]=0.2;
                    break;

                case 13:
                    glColor4b(175,50,175,2);
                    mc3[0]=255.0/255.0; mc3[1]=50.0/255.0; mc3[2]=255.0/255.0; mc3[3]=0.2;
                    break;
                case 14:
                    glColor4b(50,175,50,2);
                    mc3[0]=50.0/255.0; mc3[1]=255.0/255.0; mc3[2]=255.0/255.0; mc3[3]=0.2;
                    break;
                case 15:
                    glColor4b(50,50,175,2);
                    mc3[0]=255.0/255.0; mc3[1]=255.0/255.0; mc3[2]=255.0/255.0; mc3[3]=0.2;
                    break;
                default: //user defined room color
                    if( ! mpMap->customEnvColors.contains(env) ) break;
                    QColor &_c = mpMap->customEnvColors[env];
                    glColor4b(_c.red(),_c.green(),_c.blue(),255);
                    mc3[0]=_c.redF();
                    mc3[1]=_c.greenF();
                    mc3[2]=_c.blueF();
                    mc3[3]=0.2;
                }
                glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mc3);
                glMateriali(GL_FRONT, GL_SHININESS, 96);
                glDisable(GL_DEPTH_TEST);
                glLoadIdentity();
                gluLookAt(px*0.1+xRot, py*0.1+yRot, pz*0.1+zRot, px*0.1, py*0.1, pz*0.1,0.0,1.0,0.0);

                if( pArea->gridMode )
                {
                    if( ( rz == pz ) && ( rx == px ) && ( ry == py ) )
                    {
                        glScalef( 0.1, 0.1, 0.020 );
                        glTranslatef( 0.1*rx, 0.1*ry, 5.0*(rz+0.25) );
                    }
                    else
                    {
                        glScalef( 0.2, 0.2, 0.020);
                        glTranslatef( 0.5*rx, 0.5*ry, 5.0*(rz+0.25) );
                    }
                }
                else
                {
                    glScalef( 0.075, 0.075, 0.020);
                    glTranslatef( 1.333333333*rx,1.333333333*ry,5.0*(rz+0.25) );//+0.4
                }

                glBegin( GL_QUADS );
                glNormal3f(0.57735, -0.57735, 0.57735);
                glVertex3f(1.0/dehnung, -1.0/dehnung, 1.0/dehnung);
                glNormal3f(-0.57735, -0.57735, 0.57735);
                glVertex3f(-1.0/dehnung, -1.0/dehnung, 1.0/dehnung);
                glNormal3f(-0.57735, -0.57735, -0.57735);
                glVertex3f(-1.0/dehnung, -1.0/dehnung, -1.0/dehnung);
                glNormal3f(0.57735, -0.57735, -0.57735);
                glVertex3f(1.0/dehnung, -1.0/dehnung, -1.0/dehnung);

                glNormal3f(0.57735, 0.57735, 0.57735);
                glVertex3f(1.0/dehnung, 1.0/dehnung, 1.0/dehnung);
                glNormal3f(-0.57735, 0.57735, 0.57735);
                glVertex3f(-1.0/dehnung, 1.0/dehnung, 1.0/dehnung);
                glNormal3f(-0.57735, -0.57735, 0.57735);
                glVertex3f(-1.0/dehnung, -1.0/dehnung, 1.0/dehnung);
                glNormal3f(0.57735, -0.57735, 0.57735);
                glVertex3f(1.0/dehnung, -1.0/dehnung, 1.0/dehnung);

                glNormal3f(-0.57735, 0.57735, -0.57735);
                glVertex3f(-1.0/dehnung, 1.0/dehnung, -1.0/dehnung);
                glNormal3f(0.57735, 0.57735, -0.57735);
                glVertex3f(1.0/dehnung, 1.0/dehnung, -1.0/dehnung);
                glNormal3f(0.57735, -0.57735, -0.57735);
                glVertex3f(1.0/dehnung, -1.0/dehnung, -1.0/dehnung);
                glNormal3f(-0.57735, -0.57735, -0.57735);
                glVertex3f(-1.0/dehnung, -1.0/dehnung, -1.0/dehnung);

                glNormal3f(0.57735, 0.57735, -0.57735);
                glVertex3f(1.0/dehnung, 1.0/dehnung, -1.0/dehnung);
                glNormal3f(0.57735, 0.57735, 0.57735);
                glVertex3f(1.0/dehnung, 1.0/dehnung, 1.0/dehnung);
                glNormal3f(0.57735, -0.57735, 0.57735);
                glVertex3f(1.0/dehnung, -1.0/dehnung, 1.0/dehnung);
                glNormal3f(0.57735, -0.57735, -0.57735);
                glVertex3f(1.0/dehnung, -1.0/dehnung, -1.0/dehnung);

                glNormal3f(-0.57735, 0.57735, 0.57735);
                glVertex3f(-1.0/dehnung, 1.0/dehnung, 1.0/dehnung);
                glNormal3f(-0.57735, 0.57735, -0.57735);
                glVertex3f(-1.0/dehnung, 1.0/dehnung, -1.0/dehnung);
                glNormal3f(-0.57735, -0.57735, -0.57735);
                glVertex3f(-1.0/dehnung, -1.0/dehnung, -1.0/dehnung);
                glNormal3f(-0.57735, -0.57735, 0.57735);
                glVertex3f(-1.0/dehnung, -1.0/dehnung, 1.0/dehnung);

                glNormal3f(0.57735, 0.57735, -0.57735);
                glVertex3f(1.0/dehnung, 1.0/dehnung, -1.0/dehnung);
                glNormal3f(-0.57735, 0.57735, -0.57735);
                glVertex3f(-1.0/dehnung, 1.0/dehnung, -1.0/dehnung);
                glNormal3f(-0.57735, 0.57735, 0.57735);
                glVertex3f(-1.0/dehnung, 1.0/dehnung, 1.0/dehnung);
                glNormal3f(0.57735, 0.57735, 0.57735);
                glVertex3f(1.0/dehnung, 1.0/dehnung, 1.0/dehnung);
                glEnd();

                continue;
            }

            float mc3[] = { 0.2f, 0.2f, 0.7f, 1.0f };
            glLoadIdentity();
            gluLookAt(px*0.1+xRot, py*0.1+yRot, pz*0.1+zRot, px*0.1, py*0.1, pz*0.1,0.0,1.0,0.0);
            if( pArea->gridMode )
            {
                glScalef( 0.2, 0.2, 0.1 );
                glTranslatef( 0.5*rx, 0.5*ry, rz );
            }
            else
            {
                glScalef( 0.1, 0.1, 0.1);
                glTranslatef( rx, ry, rz );
            }

            glLoadName( pArea->rooms[i] );
            quads++;
            glBegin( GL_QUADS );
            glNormal3f(0.57735, -0.57735, 0.57735);
            glVertex3f(1.0/dehnung, -1.0/dehnung, 1.0/dehnung);
            glNormal3f(-0.57735, -0.57735, 0.57735);
            glVertex3f(-1.0/dehnung, -1.0/dehnung, 1.0/dehnung);
            glNormal3f(-0.57735, -0.57735, -0.57735);
            glVertex3f(-1.0/dehnung, -1.0/dehnung, -1.0/dehnung);
            glNormal3f(0.57735, -0.57735, -0.57735);
            glVertex3f(1.0/dehnung, -1.0/dehnung, -1.0/dehnung);

            glNormal3f(0.57735, 0.57735, 0.57735);
            glVertex3f(1.0/dehnung, 1.0/dehnung, 1.0/dehnung);
            glNormal3f(-0.57735, 0.57735, 0.57735);
            glVertex3f(-1.0/dehnung, 1.0/dehnung, 1.0/dehnung);
            glNormal3f(-0.57735, -0.57735, 0.57735);
            glVertex3f(-1.0/dehnung, -1.0/dehnung, 1.0/dehnung);
            glNormal3f(0.57735, -0.57735, 0.57735);
            glVertex3f(1.0/dehnung, -1.0/dehnung, 1.0/dehnung);

            glNormal3f(-0.57735, 0.57735, -0.57735);
            glVertex3f(-1.0/dehnung, 1.0/dehnung, -1.0/dehnung);
            glNormal3f(0.57735, 0.57735, -0.57735);
            glVertex3f(1.0/dehnung, 1.0/dehnung, -1.0/dehnung);
            glNormal3f(0.57735, -0.57735, -0.57735);
            glVertex3f(1.0/dehnung, -1.0/dehnung, -1.0/dehnung);
            glNormal3f(-0.57735, -0.57735, -0.57735);
            glVertex3f(-1.0/dehnung, -1.0/dehnung, -1.0/dehnung);

            glNormal3f(0.57735, 0.57735, -0.57735);
            glVertex3f(1.0/dehnung, 1.0/dehnung, -1.0/dehnung);
            glNormal3f(0.57735, 0.57735, 0.57735);
            glVertex3f(1.0/dehnung, 1.0/dehnung, 1.0/dehnung);
            glNormal3f(0.57735, -0.57735, 0.57735);
            glVertex3f(1.0/dehnung, -1.0/dehnung, 1.0/dehnung);
            glNormal3f(0.57735, -0.57735, -0.57735);
            glVertex3f(1.0/dehnung, -1.0/dehnung, -1.0/dehnung);

            glNormal3f(-0.57735, 0.57735, 0.57735);
            glVertex3f(-1.0/dehnung, 1.0/dehnung, 1.0/dehnung);
            glNormal3f(-0.57735, 0.57735, -0.57735);
            glVertex3f(-1.0/dehnung, 1.0/dehnung, -1.0/dehnung);
            glNormal3f(-0.57735, -0.57735, -0.57735);
            glVertex3f(-1.0/dehnung, -1.0/dehnung, -1.0/dehnung);
            glNormal3f(-0.57735, -0.57735, 0.57735);
            glVertex3f(-1.0/dehnung, -1.0/dehnung, 1.0/dehnung);

            glNormal3f(0.57735, 0.57735, -0.57735);
            glVertex3f(1.0/dehnung, 1.0/dehnung, -1.0/dehnung);
            glNormal3f(-0.57735, 0.57735, -0.57735);
            glVertex3f(-1.0/dehnung, 1.0/dehnung, -1.0/dehnung);
            glNormal3f(-0.57735, 0.57735, 0.57735);
            glVertex3f(-1.0/dehnung, 1.0/dehnung, 1.0/dehnung);
            glNormal3f(0.57735, 0.57735, 0.57735);
            glVertex3f(1.0/dehnung, 1.0/dehnung, 1.0/dehnung);

            glEnd();

            int env = pR->environment;
            if( mpMap->envColors.contains(env) )
                env = mpMap->envColors[env];
            else
            {
                if( ! mpMap->customEnvColors.contains(env))
                {
                    env = 1;
                }
            }
            switch( env )
            {
            case 1:
                glColor4b(128,50,50,255);
                mc3[0]=128.0/255.0; mc3[1]=0.0/255.0; mc3[2]=0.0/255.0; mc3[3]=255.0/255.0;
                break;

            case 2:
                glColor4b(128,128,50, 255);
                mc3[0]=0.0/255.0; mc3[1]=128.0/255.0; mc3[2]=0.0/255.0; mc3[3]=255.0/255.0;
                break;
            case 3:
                glColor4b(50,128,50,255);
                mc3[0]=128.0/255.0; mc3[1]=128.0/255.0; mc3[2]=0.0/255.0; mc3[3]=255.0/255.0;
                break;

            case 4:
                glColor4b(50,50,128,255);
                mc3[0]=0.0/255.0; mc3[1]=0.0/255.0; mc3[2]=128.0/255.0; mc3[3]=255.0/255.0;
                break;

            case 5:
                glColor4b(128,50,128,255);
                mc3[0]=128.0/255.0; mc3[1]=128.0/255.0; mc3[2]=0.0/255.0; mc3[3]=255.0/255.0;
                break;
            case 6:
                glColor4b(50,128,128,255);
                mc3[0]=0.0/255.0; mc3[1]=128.0/255.0; mc3[2]=128.0/255.0; mc3[3]=255.0/255.0;
                break;
            case 7:
                glColor4b(52,38,78,255);
                mc3[0]=128.0/255.0; mc3[1]=128.0/255.0; mc3[2]=128.0/255.0; mc3[3]=255.0/255.0;
                break;
            case 8:
                glColor4b(65, 55, 35, 255);
                mc3[0]=55.0/255.0; mc3[1]=55.0/255.0; mc3[2]=55.0/255.0; mc3[3]=255.0/255.0;
                break;

            case 9:
                glColor4b(175,50,50,255);
                mc3[0]=255.0/255.0; mc3[1]=50.0/255.0; mc3[2]=50.0/255.0; mc3[3]=255.0/255.0;
                break;

            case 10:
                glColor4b(255,255,50,255);
                mc3[0]=50.0/255.0; mc3[1]=255.0/255.0; mc3[2]=50.0/255.0; mc3[3]=255.0/255.0;
                break;
            case 11:
                glColor4b(50,175,175,255);
                mc3[0]=255.0/255.0; mc3[1]=255.0/255.0; mc3[2]=50.0/255.0; mc3[3]=255.0/255.0;
                break;

            case 12:
                glColor4b(175,175,50,255);
                mc3[0]=50.0/255.0; mc3[1]=50.0/255.0; mc3[2]=255.0/255.0; mc3[3]=255.0/255.0;
                break;

            case 13:
                glColor4b(175,50,175,255);
                mc3[0]=255.0/255.0; mc3[1]=50.0/255.0; mc3[2]=255.0/255.0; mc3[3]=255.0/255.0;
                break;
            case 14:
                glColor4b(50,175,50,255);
                mc3[0]=50.0/255.0; mc3[1]=255.0/255.0; mc3[2]=255.0/255.0; mc3[3]=255.0/255.0;
                break;
            case 15:
                glColor4b(50,50,175,255);
                mc3[0]=255.0/255.0; mc3[1]=255.0/255.0; mc3[2]=255.0/255.0; mc3[3]=255.0/255.0;
                break;
            default: //user defined room color
                if( ! mpMap->customEnvColors.contains(env) ) break;
                QColor &_c = mpMap->customEnvColors[env];
                glColor4b(_c.red(),_c.green(),_c.blue(),255);
                mc3[0]=_c.redF();
                mc3[1]=_c.greenF();
                mc3[2]=_c.blueF();
                mc3[3]=1.0;
            }
            glDisable(GL_DEPTH_TEST);
            glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mc3);
            glMateriali(GL_FRONT, GL_SHININESS, 6);
            glLoadIdentity();
            gluLookAt(px*0.1+xRot, py*0.1+yRot, pz*0.1+zRot, px*0.1, py*0.1, pz*0.1,0.0,1.0,0.0);
            if( pArea->gridMode )
            {
                if( ( rz == pz ) && ( rx == px ) && ( ry == py ) )
                {
                    glScalef( 0.1, 0.1, 0.020 );
                    glTranslatef( rx, ry, 5.0*(rz+0.25) );
                }
                else
                {
                    glScalef( 0.2, 0.2, 0.020);
                    glTranslatef( 0.5*rx, 0.5*ry, 5.0*(rz+0.25) );
                }
            }
            else
            {
                if( is2DView )
                {
                    glScalef( 0.090, 0.090, 0.020);
                    glTranslatef( 1.1111111*rx,1.1111111*ry,5.0*(rz+0.25) );//+0.4
                }
                else
                {
                    glScalef( 0.075, 0.075, 0.020);
                    glTranslatef( 1.333333333*rx,1.333333333*ry,5.0*(rz+0.25) );//+0.4
                }
            }
            glBegin( GL_QUADS );
            glNormal3f(0.57735, -0.57735, 0.57735);
            glVertex3f(1.0/dehnung, -1.0/dehnung, 1.0/dehnung);
            glNormal3f(-0.57735, -0.57735, 0.57735);
            glVertex3f(-1.0/dehnung, -1.0/dehnung, 1.0/dehnung);
            glNormal3f(-0.57735, -0.57735, -0.57735);
            glVertex3f(-1.0/dehnung, -1.0/dehnung, -1.0/dehnung);
            glNormal3f(0.57735, -0.57735, -0.57735);
            glVertex3f(1.0/dehnung, -1.0/dehnung, -1.0/dehnung);

            glNormal3f(0.57735, 0.57735, 0.57735);
            glVertex3f(1.0/dehnung, 1.0/dehnung, 1.0/dehnung);
            glNormal3f(-0.57735, 0.57735, 0.57735);
            glVertex3f(-1.0/dehnung, 1.0/dehnung, 1.0/dehnung);
            glNormal3f(-0.57735, -0.57735, 0.57735);
            glVertex3f(-1.0/dehnung, -1.0/dehnung, 1.0/dehnung);
            glNormal3f(0.57735, -0.57735, 0.57735);
            glVertex3f(1.0/dehnung, -1.0/dehnung, 1.0/dehnung);

            glNormal3f(-0.57735, 0.57735, -0.57735);
            glVertex3f(-1.0/dehnung, 1.0/dehnung, -1.0/dehnung);
            glNormal3f(0.57735, 0.57735, -0.57735);
            glVertex3f(1.0/dehnung, 1.0/dehnung, -1.0/dehnung);
            glNormal3f(0.57735, -0.57735, -0.57735);
            glVertex3f(1.0/dehnung, -1.0/dehnung, -1.0/dehnung);
            glNormal3f(-0.57735, -0.57735, -0.57735);
            glVertex3f(-1.0/dehnung, -1.0/dehnung, -1.0/dehnung);

            glNormal3f(0.57735, 0.57735, -0.57735);
            glVertex3f(1.0/dehnung, 1.0/dehnung, -1.0/dehnung);
            glNormal3f(0.57735, 0.57735, 0.57735);
            glVertex3f(1.0/dehnung, 1.0/dehnung, 1.0/dehnung);
            glNormal3f(0.57735, -0.57735, 0.57735);
            glVertex3f(1.0/dehnung, -1.0/dehnung, 1.0/dehnung);
            glNormal3f(0.57735, -0.57735, -0.57735);
            glVertex3f(1.0/dehnung, -1.0/dehnung, -1.0/dehnung);

            glNormal3f(-0.57735, 0.57735, 0.57735);
            glVertex3f(-1.0/dehnung, 1.0/dehnung, 1.0/dehnung);
            glNormal3f(-0.57735, 0.57735, -0.57735);
            glVertex3f(-1.0/dehnung, 1.0/dehnung, -1.0/dehnung);
            glNormal3f(-0.57735, -0.57735, -0.57735);
            glVertex3f(-1.0/dehnung, -1.0/dehnung, -1.0/dehnung);
            glNormal3f(-0.57735, -0.57735, 0.57735);
            glVertex3f(-1.0/dehnung, -1.0/dehnung, 1.0/dehnung);

            glNormal3f(0.57735, 0.57735, -0.57735);
            glVertex3f(1.0/dehnung, 1.0/dehnung, -1.0/dehnung);
            glNormal3f(-0.57735, 0.57735, -0.57735);
            glVertex3f(-1.0/dehnung, 1.0/dehnung, -1.0/dehnung);
            glNormal3f(-0.57735, 0.57735, 0.57735);
            glVertex3f(-1.0/dehnung, 1.0/dehnung, 1.0/dehnung);
            glNormal3f(0.57735, 0.57735, 0.57735);
            glVertex3f(1.0/dehnung, 1.0/dehnung, 1.0/dehnung);
            glEnd();

            glColor4b(128,128,128,255);


            glBegin( GL_TRIANGLES );

            if( pR->getDown() > -1 )
            {
                glVertex3f( 0.0, -0.95/dehnung, 0.0);
                glVertex3f(0.95/dehnung, -0.25/dehnung, 0.0);
                glVertex3f( -0.95/dehnung, -0.25/dehnung, 0.0);
            }
            if( pR->getUp() > -1 )
            {
                glVertex3f( 0.0, 0.95/dehnung, 0.0);
                glVertex3f(-0.95/dehnung, 0.25/dehnung, 0.0);
                glVertex3f( 0.95/dehnung, 0.25/dehnung, 0.0);
            }
            glEnd();

//            if( mpMap->rooms[pArea->rooms[i]]->out > -1 )
//            {
//                glBegin( GL_LINE_LOOP );
//                for( int angle=0; angle<360; angle += 1 )
//                {
//                    glVertex3f( (0.5 + sin((float)angle) * 0.25)/dehnung, ( cos((float)angle) * 0.25)/dehnung, 0.0);
//                }
//                glEnd();
//            }


//            glTranslatef( -0.1, 0.0, 0.0 );
//            if( mpMap->rooms[pArea->rooms[i]]->in > -1 )
//            {
//                glBegin( GL_TRIANGLE_FAN );
//                glVertex3f( 0.0, 0.0, 0.0);
//                for( int angle=0; angle<=360; angle += 5 )
//                {
//                    glVertex3f( (sin((float)angle)*0.25)/dehnung, (cos((float)angle)*0.25)/dehnung, 0.0);
//                }
//                glEnd();
//            }

        }


        zEbene += 1.0;
    }
 //   qDebug()<<"FINAL: mQuads.size()="<<mQuads.size()<<"area.size()="<<pArea->rooms.size()<<" quads="<<quads<<" verts="<<verts;
//    qDebug()<<"mScale="<<mScale<<" 1/mScale="<<1/mScale<<" env="<<env;
//    cout<<"dif r="<<diffuseLight[0]<<" g="<<diffuseLight[1]<<" b="<<diffuseLight[2]<<endl;
//    cout << "xRot:"<<xRot<<" yRot:"<<yRot<<" zRot:"<<zRot<<endl;
    glFlush();
}

void GLWidget::resizeGL(int w, int h)
{
    glViewport( 0, 0, (GLint)w, (GLint)h );
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
    if( !ortho )
        gluPerspective( 60*mScale, (GLfloat)w / (GLfloat)h, 0.0001, 10000.0);
    else
        gluOrtho2D( 0.0, (GLdouble) w, 0.0, (GLdouble) h );
    glMatrixMode( GL_MODELVIEW );
}

void GLWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton)
    {
        int x = event->x();
        int y = height()-event->y();//opengl ursprungspunkt liegt unten links
        GLuint buff[16] = {0};
        GLint hits;
        GLint view[4];
        glSelectBuffer( 16, buff );
        glGetIntegerv( GL_VIEWPORT, view );
        glRenderMode( GL_SELECT );
        glInitNames();
        glPushName( 0 );
        glMatrixMode( GL_PROJECTION );
        glPushMatrix();
        glLoadIdentity();
        gluPickMatrix(x, y, 0.1, 0.1, view);
        gluPerspective( 60*mScale, (GLfloat)width() / (GLfloat)height(), 0.0001, 10000.0);
        glMatrixMode(GL_MODELVIEW);

        mQuads.clear();
        mTarget=-22;
        selectionMode = true;
        paintGL();
        selectionMode = false;

        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        hits = glRenderMode(GL_RENDER);
        int i;

        for (i = 0; i < hits; i++)
        {
            mTarget = buff[i * 4 + 3];
            //TODO: Mehrfachbelegungen
//            unsigned int minZ = buff[i * 4 + 1];
//            unsigned int maxZ = buff[i * 4 + 2];
        }
        glViewport( 0, 0, (GLint)width(), (GLint)height() );
        glMatrixMode( GL_PROJECTION );
        glLoadIdentity();
        gluPerspective( 60*mScale, (GLfloat)width() / (GLfloat)height(), 0.0001, 10000.0);
        glMatrixMode(GL_MODELVIEW);
        updateGL();
        if( mpMap->mpRoomDB->getRoom(mTarget) )
        {
            mpMap->mTargetID = mTarget;
            if( mpMap->findPath( mpMap->mRoomId, mpMap->mTargetID) )
            {
               mpMap->mpHost->startSpeedWalk();
            }
//            else
//            {
//                QMessageBox msgBox;
//                msgBox.setText("Cannot find a path to this room using regular exits.#glWidget\n");
//                msgBox.exec();
//            }
        }
        else{
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

void GLWidget::mouseMoveEvent( QMouseEvent * event )
{
    if (mPanMode)
    {
        int x = event->x();
        int y = height()-event->y();//opengl ursprungspunkt liegt unten links
        if ((mPanXStart-x) > 1){
            shiftRight();
            mPanXStart = x;
        }
        else if ((mPanXStart-x) < -1){
            shiftLeft();
            mPanXStart = x;
        }
        if ((mPanYStart-y) > 1){
            shiftUp();
            mPanYStart = y;
        }
        else if ((mPanYStart-y) < -1){
            shiftDown();
            mPanYStart = y;
        }
    }
}

void GLWidget::mouseReleaseEvent( QMouseEvent * event )
{
    mPanMode = false;
}

void GLWidget::wheelEvent ( QWheelEvent * e )
{
    //int delta = e->delta() / 8 / 15;
    if( e->delta() < 0 )
    {
        if( abs(mScale) < 0.3 )
            mScale -= 0.01;
        else
            mScale -= 0.03;
        resizeGL(width(),height());
        updateGL();
        e->accept();
        return;
    }
    if( e->delta() > 0 )
    {
        if( abs(mScale) < 0.3 )
            mScale += 0.01;
        else
            mScale += 0.03;
        resizeGL(width(),height());
        updateGL();
        e->accept();
        return;
    }
    e->ignore();
    return;
}










