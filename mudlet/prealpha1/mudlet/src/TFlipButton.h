#ifndef TFLIPBUTTON_H
#define TFLIPBUTTON_H

#include <QToolButton>
#include <QPushButton>
#include <QStyleOptionButton>

class TFlipButton : public QToolButton//QPushButton
{
public:
    TFlipButton( QWidget* parent = 0 );
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
    
private:
    QStyleOptionButton getStyleOption() const;
    void init();
    
    Qt::Orientation orientation_;
    bool mirrored_;
};

#endif
