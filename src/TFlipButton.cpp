

#include <QPushButton>
#include "TFlipButton.h"
#include <QStylePainter>
#include <QMenu>

TFlipButton::TFlipButton( TToolBar * parent, TAction * pTAction, int id, Host * pHost )
: QPushButton( 0 )
, mpTAction( pTAction )
, mID( id )
, mpHost( pHost )
{
    init();
}

TFlipButton::TFlipButton( TEasyButtonBar * parent, TAction * pTAction, int id, Host * pHost )
: QPushButton( 0 )
, mpTAction( pTAction )
, mID( id )
, mpHost( pHost )
{
    init();
}

void TFlipButton::init()
{
    mOrientation = Qt::Horizontal;
    mMirrored = false;
}

Qt::Orientation TFlipButton::orientation() const
{
    return mOrientation;
}

void TFlipButton::setOrientation( Qt::Orientation orientation )
{
    mOrientation = orientation;
    switch( orientation )
    {
    case Qt::Horizontal:
        setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Fixed );
        break;
        
    case Qt::Vertical:
        setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Minimum );
        break;
    }
}

bool TFlipButton::mirrored() const
{
    return mMirrored;
}

void TFlipButton::setMirrored( bool mirrored )
{
    mMirrored = mirrored;
}

QSize TFlipButton::sizeHint() const
{
    QSize size = QPushButton::sizeHint();
    if( mOrientation == Qt::Vertical )
    {
        size.transpose();
    }
    return size;
}

QSize TFlipButton::minimumSizeHint() const
{
    QSize size = QPushButton::minimumSizeHint();
    if( mOrientation == Qt::Vertical )
    {
        size.transpose();
    }
    return size;
}

void TFlipButton::paintEvent( QPaintEvent * event )
{
    Q_UNUSED( event );
    QStylePainter p( this );
    
    switch( mOrientation )
    {
    case Qt::Horizontal:
        if( mMirrored )
        {
            p.rotate( 180 );
            p.translate( -width(), -height() );
        }
        break;
        
    case Qt::Vertical:
        if( mMirrored )
        {
            p.rotate( -90 );
            p.translate( -height(), 0 );
        }
        else
        {
            p.rotate( 90 );
            p.translate( 0, -width() );
        }
        break;
    }
    
    p.drawControl( QStyle::CE_PushButton, getStyleOption() );
}

QStyleOptionButton TFlipButton::getStyleOption() const
{
    QStyleOptionButton opt;
    opt.initFrom( this );
    if( mOrientation == Qt::Vertical )
    {
        QSize size = opt.rect.size();
        size.transpose();
        opt.rect.setSize(size);
    }
    opt.features = QStyleOptionButton::None;
    if( menu() ) opt.features |= QStyleOptionButton::HasMenu;
    if( isDown() || ( menu() && menu()->isVisible() ) ) opt.state |= QStyle::State_Sunken;
    if( isChecked() ) opt.state |= QStyle::State_On;
    opt.text = text();
    opt.icon = icon();
    opt.iconSize = iconSize();
    return opt;
}

