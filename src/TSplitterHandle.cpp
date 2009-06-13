#include "TSplitterHandle.h"
#include "TSplitter.h"
#include <QPainter>
#include <QtGui>
#include <QGradient>

TSplitterHandle::TSplitterHandle( Qt::Orientation orientation, TSplitter * parent )
: QSplitterHandle( orientation, (QSplitter*) parent )
{
}

void TSplitterHandle::paintEvent( QPaintEvent * event )
 {
     QPainter painter( this );
     QLinearGradient gradient(QPointF(100, 100), QPointF(200, 200));
     gradient.setColorAt(0, Qt::black);
     gradient.setColorAt(1, Qt::white);
     if(orientation() == Qt::Horizontal)
     {
         gradient.setStart(rect().left(), rect().height()/2);
         gradient.setFinalStop(rect().right(), rect().height()/2);
     }
     else
     {
         gradient.setStart(rect().width()/2, rect().top());
         gradient.setFinalStop(rect().width()/2, rect().bottom());
     }
     painter.fillRect(event->rect(), QBrush(gradient));
 }
