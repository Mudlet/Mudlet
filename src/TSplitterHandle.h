#ifndef TSPLITTERHANDLE_H
#define TSPLITTERHANDLE_H

#include <QSplitterHandle>
#include "TSplitter.h"

class TSplitter;

class TSplitterHandle : public QSplitterHandle
{
//Q_OBJECT
 public:
     TSplitterHandle(Qt::Orientation orientation, TSplitter * parent );
     void paintEvent( QPaintEvent * );
};

#endif // TSPLITTERHANDLE_H
