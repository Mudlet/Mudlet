#include "TLabel.h"
#include <QDebug>

TLabel::TLabel( QWidget * pW )
: QLabel( pW )
, mpHost( 0 )
{
    setMouseTracking( true );
}

QString nothing = "";

void TLabel::mousePressEvent( QMouseEvent * event )
{
    if( event->button() == Qt::LeftButton )
    {
        if( mpHost )
        {
            mpHost->getLuaInterpreter()->call( mScript, nothing );
        }
        event->accept();
        return;
    }

    QWidget::mousePressEvent( event );
}
