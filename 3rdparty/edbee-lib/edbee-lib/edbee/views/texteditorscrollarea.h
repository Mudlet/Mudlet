/**
 * Copyright 2011-2013 - Reliable Bits Software by Blommers IT. All Rights Reserved.
 * Author Rick Blommers
 */

#pragma once

#include <QScrollArea>

class QLinearGradient;
class QPainter;

namespace edbee {

class PrivateShadowWidget;

/// A special scrollarea that makes it possible to add components to the margins of it
class TextEditorScrollArea : public QScrollArea
{
    Q_OBJECT
public:
    explicit TextEditorScrollArea(QWidget* parent = 0);
    virtual ~TextEditorScrollArea();
    
    void setLeftWidget(QWidget* widget);
    void setTopWidget(QWidget* widget);
    void setRightWidget(QWidget* widget);
    void setBottomWidget(QWidget* widget);


public:

    void layoutMarginWidgets();


protected:
    virtual void resizeEvent(QResizeEvent* event);

public:
    void renderShade(QPaintEvent *event, QPainter* painter );


signals:    
public slots:

private:

    QWidget* leftWidgetRef_;               ///< The left widget
    QWidget* topWidgetRef_;                ///< The top widget
    QWidget* rightWidgetRef_;              ///< The right widget
    QWidget* bottomWidgetRef_;             ///< The bottom widget
    PrivateShadowWidget* shadowWidgetRef_; ///< The private shadow widget


};

} // edbee
