#ifndef TROOM_H
#define TROOM_H

#include <QVector3D>
#include <QMap>
#include <QColor>

#define DIR_NORTH 1
#define DIR_NORTHEAST 2
#define DIR_NORTHWEST 3
#define DIR_EAST 4
#define DIR_WEST 5
#define DIR_SOUTH 6
#define DIR_SOUTHEAST 7
#define DIR_SOUTHWEST 8
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
    void setExitLock( int, bool );
    void setSpecialExitLock(int to, QString cmd, bool doLock);
    bool hasExitLock(int to);
    bool hasSpecialExitLock( int, QString );
    void removeSpecialExit( int to, QString cmd );
    void addSpecialExit( int to, QString cmd );
    int hasExitStub(int direction);
    void setExitStub(int direction, int status);
    void calcRoomDimensions();
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
    int min_x;
    int min_y;
    int max_x;
    int max_y;

    qint8 c;
    QString name;
    QVector3D v;
    QMultiMap<int, QString> other; // es können mehrere exits zum gleichen raum verlaufen
                                   //verbotene exits werden mit 0 geprefixed, offene mit 1
    QList<int> exitStubs; //contains a list of: exittype (according to defined values above)
    QMap<QString, QString> userData;
    QList<int> exitLocks;
    QMap<QString, QList<QPointF> > customLines;
    QMap<QString, QList<int> > customLinesColor;
    QMap<QString, QString> customLinesStyle;
    QMap<QString, bool> customLinesArrow;
    bool highlight;
    QColor highlightColor;
    QColor highlightColor2;
    float highlightRadius;
    bool rendered;
};

#endif // TROOM_H

