#ifndef TROOM_H
#define TROOM_H

#include <QVector3D>

class TRoom
{
public:
    TRoom();
    bool hasExit( int );
    void setWeight( int );
    int id;
    int area;
    int x;
    int y;
    int z;
    int north;
    int northeast;
    int east;
    int southeast;
    int south;
    int southwest;
    int west;
    int northwest;
    int up;
    int down;
    int in;
    int out;
    int environment;
    int weight;
    bool isLocked;
    float xRot;
    float yRot;
    float zRot;
    float zoom;
    QString name;
    QVector3D v;
};

#endif // TROOM_H

