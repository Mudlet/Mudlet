#include "T2DMap.h"
#include "TMap.h"
#include "TArea.h"
#include "TRoom.h"
#include "Host.h"

T2DMap::T2DMap(TMap * pM, QWidget * parent)
: QWidget(parent)
, mpMap( pM )
{
}

void T2DMap::paintEvent( QPaintEvent * e )
{
}
    /*
    int px,py,pz;
    if( ! mpMap->rooms.contains( mpMap->mRoomId ) )
    {
        qDebug()<<"ERROR: roomID not in rooms map";
        return;
    }
    px = mpMap->rooms[mpMap->mRoomId]->x;
    py = mpMap->rooms[mpMap->mRoomId]->y;
    pz = mpMap->rooms[mpMap->mRoomId]->z;

    TArea * pArea = mpMap->areas[mpMap->rooms[mpMap->mRoomId]->area];
    if( ! pArea ) return;

    if( pArea->gridMode )
    {
        ;//TODO
    }
    int zEbene;
    if( ! mpMap->rooms.contains(mpMap->mRoomId) ) return;
    if( ! mpMap ) return;

    for( int i=0; i<pArea->rooms.size(); i++ )
    {
        int rx = static_cast<float>(mpMap->rooms[pArea->rooms[i]]->x);
        int ry = static_cast<float>(mpMap->rooms[pArea->rooms[i]]->y);
        int rz = static_cast<float>(mpMap->rooms[pArea->rooms[i]]->z);

        if( rz != zEbene ) continue;

        QList<int> exitList;
        exitList.push_back( mpMap->rooms[pArea->rooms[i]]->north );
        exitList.push_back( mpMap->rooms[pArea->rooms[i]]->northwest );
        exitList.push_back( mpMap->rooms[pArea->rooms[i]]->east );
        exitList.push_back( mpMap->rooms[pArea->rooms[i]]->southeast );
        exitList.push_back( mpMap->rooms[pArea->rooms[i]]->south );
        exitList.push_back( mpMap->rooms[pArea->rooms[i]]->southwest );
        exitList.push_back( mpMap->rooms[pArea->rooms[i]]->west );
        exitList.push_back( mpMap->rooms[pArea->rooms[i]]->northwest );
        exitList.push_back( mpMap->rooms[pArea->rooms[i]]->up );
        exitList.push_back( mpMap->rooms[pArea->rooms[i]]->down );
        int e = mpMap->rooms[pArea->rooms[i]]->z;


        if( ( rz == pz ) && ( rx == px ) && ( ry == py ) )
        {
            ;//TODO: highlight current room
        }
        else
        {
            ;//TODO: set room color
        }
        for( int k=0; k<exitList.size(); k++ )
        {
            bool areaExit = false;
            if( exitList[k] == -1 ) continue;
            if( ! mpMap->rooms.contains( exitList[k] ) )
            {
                continue;
            }
            if( mpMap->rooms[exitList[k]]->area != mpMap->rooms[mpMap->mRoomId]->area )
            {
                areaExit = true;
            }
            int ex = static_cast<float>(mpMap->rooms[exitList[k]]->x);
            int ey = static_cast<float>(mpMap->rooms[exitList[k]]->y);
            int ez = static_cast<float>(mpMap->rooms[exitList[k]]->z);
            QVector3D p1( ex, ey, ez );
            QVector3D p2( rx, ry, rz );

            if( areaExit )
                glLineWidth(1);//1/mScale+2);
            else
                glLineWidth(1);//1/mScale);

            if( exitList[k] == mpMap->mRoomId || ( ( rz == pz ) && ( rx == px ) && ( ry == py ) ) )
            {
                ;
            }
            else
            {
                ;//TODO: set exit color
            }

            if( ! areaExit )
            {
                glVertex3f( p1.x(), p1.y(), p1.z() );
            }
            else
            {
                if( mpMap->rooms[pArea->rooms[i]]->north == exitList[k] )
                    glVertex3f( p2.x(), p2.y()+1, p2.z() );
                else if( mpMap->rooms[pArea->rooms[i]]->south == exitList[k] )
                    glVertex3f( p2.x(), p2.y()-1, p2.z() );
                else if( mpMap->rooms[pArea->rooms[i]]->west == exitList[k] )
                    glVertex3f( p2.x()-1, p2.y(), p2.z() );
                else if( mpMap->rooms[pArea->rooms[i]]->east == exitList[k] )
                    glVertex3f( p2.x()+1, p2.y(), p2.z() );
                else if( mpMap->rooms[pArea->rooms[i]]->southwest == exitList[k] )
                    glVertex3f( p2.x()-1, p2.y()-1, p2.z() );
                else if( mpMap->rooms[pArea->rooms[i]]->southeast == exitList[k] )
                    glVertex3f( p2.x()+1, p2.y()-1, p2.z() );
                else if( mpMap->rooms[pArea->rooms[i]]->northeast == exitList[k] )
                    glVertex3f( p2.x()+1, p2.y()+1, p2.z() );
                else if( mpMap->rooms[pArea->rooms[i]]->northwest == exitList[k] )
                    glVertex3f( p2.x()-1, p2.y()+1, p2.z() );
                else if( mpMap->rooms[pArea->rooms[i]]->up == exitList[k] )
                    glVertex3f( p2.x(), p2.y(), p2.z()+1 );
                else if( mpMap->rooms[pArea->rooms[i]]->down == exitList[k] )
                    glVertex3f( p2.x(), p2.y(), p2.z()-1 );
            }

            glVertex3f( p2.x(), p2.y(), p2.z() );

            // TODO: draw area exits
            if( mpMap->rooms[pArea->rooms[i]]->north == exitList[k] )
                glTranslatef( p2.x(), p2.y()+1, p2.z() );
            else if( mpMap->rooms[pArea->rooms[i]]->south == exitList[k] )
                glTranslatef( p2.x(), p2.y()-1, p2.z() );
            else if( mpMap->rooms[pArea->rooms[i]]->west == exitList[k] )
                glTranslatef( p2.x()-1, p2.y(), p2.z() );
            else if( mpMap->rooms[pArea->rooms[i]]->east == exitList[k] )
                glTranslatef( p2.x()+1, p2.y(), p2.z() );
            else if( mpMap->rooms[pArea->rooms[i]]->southwest == exitList[k] )
                glTranslatef( p2.x()-1, p2.y()-1, p2.z() );
            else if( mpMap->rooms[pArea->rooms[i]]->southeast == exitList[k] )
                glTranslatef( p2.x()+1, p2.y()-1, p2.z() );
            else if( mpMap->rooms[pArea->rooms[i]]->northeast == exitList[k] )
                glTranslatef( p2.x()+1, p2.y()+1, p2.z() );
            else if( mpMap->rooms[pArea->rooms[i]]->northwest == exitList[k] )
                glTranslatef( p2.x()-1, p2.y()+1, p2.z() );
            else if( mpMap->rooms[pArea->rooms[i]]->up == exitList[k] )
                glTranslatef( p2.x(), p2.y(), p2.z()+1 );
            else if( mpMap->rooms[pArea->rooms[i]]->down == exitList[k] )
                glTranslatef( p2.x(), p2.y(), p2.z()-1 );

            //drauf
            int env = mpMap->rooms[exitList[k]]->environment;
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
                glColor4b(_c.red(),_c.green(),_c.blue(),25);
                mc3[0]=_c.redF();
                mc3[1]=_c.greenF();
                mc3[2]=_c.blueF();
                mc3[3]=0.2;
            }

            if( mpMap->rooms[pArea->rooms[i]]->north == exitList[k] )
                glTranslatef( 2*p2.x(), 2*(p2.y()+1), 5.0*(p2.z()+0.25) );
            else if( mpMap->rooms[pArea->rooms[i]]->south == exitList[k] )
                glTranslatef( 2*p2.x(), 2*(p2.y()-1), 5.0*(p2.z()+0.25) );
            else if( mpMap->rooms[pArea->rooms[i]]->west == exitList[k] )
                glTranslatef( 2*(p2.x()-1), 2*p2.y(), 5.0*(p2.z()+0.25) );
            else if( mpMap->rooms[pArea->rooms[i]]->east == exitList[k] )
                glTranslatef( 2*(p2.x()+1), 2*p2.y(), 5.0*(p2.z()+0.25) );
            else if( mpMap->rooms[pArea->rooms[i]]->southwest == exitList[k] )
                glTranslatef( 2*(p2.x()-1), 2*(p2.y()-1), 5.0*(p2.z()+0.25) );
            else if( mpMap->rooms[pArea->rooms[i]]->southeast == exitList[k] )
                glTranslatef( 2*(p2.x()+1), 2*(p2.y()-1), 5.0*(p2.z()+0.25) );
            else if( mpMap->rooms[pArea->rooms[i]]->northeast == exitList[k] )
                glTranslatef( 2*(p2.x()+1), 2*(p2.y()+1), 5.0*(p2.z()+0.25) );
            else if( mpMap->rooms[pArea->rooms[i]]->northwest == exitList[k] )
                glTranslatef( 2*(p2.x()-1), 2*(p2.y()+1), 5.0*(p2.z()+0.25) );
            else if( mpMap->rooms[pArea->rooms[i]]->up == exitList[k] )
                glTranslatef( 2*p2.x(), 2*p2.y(), 5.0*(p2.z()+1+0.25) );
            else if( mpMap->rooms[pArea->rooms[i]]->down == exitList[k] )
                glTranslatef( 2*p2.x(), 2*p2.y(), 5.0*(p2.z()-1+0.25) );
        }

    }
}


void T2DMap::mousePressEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton)
    {
        int x = event->x();
        int y = height()-event->y();//opengl ursprungspunkt liegt unten links
        if( mpMap->rooms.contains(mTarget) )
        {
            mpMap->mTargetID = mTarget;
            if( mpMap->findPath( mpMap->mRoomId, mpMap->mTargetID) )
            {
               qDebug()<<"glwidget: starting speedwalk path length="<<mpMap->mPathList.size();
               mpMap->mpHost->startSpeedWalk();
            }
            else
            {
                QMessageBox msgBox;
                msgBox.setText("Cannot find a path to this room using regular exits.#glWidget\n");
                msgBox.exec();
            }
        }
        else
        {
            QMessageBox msgBox;
            msgBox.setText("ERROR: Target room cannot be found in map db.\n");
            msgBox.exec();
        }
    }
}

void T2DMap::mouseMoveEvent( QMouseEvent * event )
{
}

void T2DMap::wheelEvent ( QWheelEvent * e )
{
    //int delta = e->delta() / 8 / 15;
    if( e->delta() < 0 )
    {
        if( abs(mScale) < 0.3 )
            mScale -= 0.01;
        else
            mScale -= 0.03;
        update();
        e->accept();
        return;
    }
    if( e->delta() > 0 )
    {
        if( abs(mScale) < 0.3 )
            mScale += 0.01;
        else
            mScale += 0.03;
        //resizeGL(width(),height());
        update();
        e->accept();
        return;
    }
    e->ignore();
    return;
}

*/
