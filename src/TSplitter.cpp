#include "TSplitter.h"
#include <QSplitterHandle>

TSplitter::TSplitter(Qt::Orientation o, QWidget * p )
: QSplitter( o, p )
{
}

QSplitterHandle * TSplitter::createHandle()
 {
     return new TSplitterHandle( orientation(), this );
 }

