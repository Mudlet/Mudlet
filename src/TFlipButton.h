#ifndef TFLIPBUTTON_H
#define TFLIPBUTTON_H

#include "TToolBar.h"
#include "TEasyButtonBar.h"
#include <QPushButton>
#include <QStyleOptionButton>

class TAction;
class Host;
class TToolBar;
class TEasyButtonBar;

class TFlipButton : public QPushButton
{
public:
    TFlipButton( TToolBar *, TAction *, int, Host * );
    TFlipButton( TEasyButtonBar *, TAction *, int, Host * );
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
    
    Qt::Orientation mOrientation;
    bool mMirrored;
    TAction * mpTAction;
    int mID;
    Host * mpHost;
};

#endif
