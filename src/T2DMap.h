#ifndef T2DMAP_H
#define T2DMAP_H

#include <QWidget>
#include <TMap.h>

class T2DMap : public QWidget
{
    Q_OBJECT

public:

    explicit T2DMap(TMap *, QWidget *parent = 0);
    void     paintEvent( QPaintEvent * );

    TMap *   mpMap;
signals:

public slots:

};

#endif // T2DMAP_H
