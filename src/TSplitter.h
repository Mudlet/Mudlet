#ifndef TSPLITTER_H
#define TSPLITTER_H

#include <QSplitter>
#include "TSplitterHandle.h"

class TSplitterHandle;

class TSplitter : public QSplitter
{
Q_OBJECT
 public:
     TSplitter( Qt::Orientation orientation, QWidget *parent = 0 );

 protected:
     QSplitterHandle * createHandle();

     TSplitterHandle * mpSplitterHandle;
};


#endif // TSPLITTER_H
