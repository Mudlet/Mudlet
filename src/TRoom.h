#ifndef TROOM_H
#define TROOM_H

#include <QVector3D>

#define DIR_NORTH 1
#define DIR_NORTHEAST 2
#define DIR_NORTHWEST 3
#define DIR_EAST 4
#define DIR_WEST 5
#define DIR_SOUTH 6
#define DIR_SOUTHEAST 7
#define DIR_SOUTWEST 8
#define DIR_UP 9
#define DIR_DOWN 10
#define DIR_IN 11
#define DIR_OUT 12

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

