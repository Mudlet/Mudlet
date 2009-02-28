#ifndef TFLIPBUTTON_H
#define TFLIPBUTTON_H

#include "TToolBar.h"
#include <QPushButton>
#include <QStyleOptionButton>

class TAction;
class Host;
class TToolBar;

class TFlipButton : public QPushButton
{
public:
    TFlipButton( TToolBar *, TAction *, int, Host * );
    TFlipButton( const QString & text, QWidget* parent = 0);
    TFlipButton( const QIcon & icon, const QString & text, QWidget * parent = 0 );
    
    Qt::Orientation orientation() const;
    void setOrientation( Qt::Orientation orientation );
    
    bool mirrored() const;
    void setMirrored( bool mirrored );
    
    QSize sizeHint() const;
    QSize minimumSizeHint() const;
    
protected:
    
    void paintEvent( QPaintEvent * event );
    
public:
    
    QStyleOptionButton getStyleOption() const;
    void init();
    
    Qt::Orientation orientation_;
    bool mirrored_;
    TAction * mpTAction;
    int mID;
    Host * mpHost;
};

#endif
