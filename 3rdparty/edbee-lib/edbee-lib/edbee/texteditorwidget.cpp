/**
 * Copyright 2011-2013 - Reliable Bits Software by Blommers IT. All Rights Reserved.
 * Author Rick Blommers
 */

#include "texteditorwidget.h"

#include <QApplication>
#include <QGraphicsBlurEffect>
#include <QGraphicsDropShadowEffect>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QPropertyAnimation>
#include <QScrollArea>
#include <QScrollBar>
#include <QStyle>
#include <QStyleOptionFocusRect>
#include <QTextEdit>
#include <QTimer>
#include <QVBoxLayout>

#include "edbee/commands/selectioncommand.h"
#include "edbee/edbee.h"
#include "edbee/texteditorcontroller.h"
#include "edbee/models/change.h"
#include "edbee/models/textdocument.h"
#include "edbee/models/texteditorconfig.h"
#include "edbee/models/texteditorcommandmap.h"
#include "edbee/models/texteditorkeymap.h"
#include "edbee/models/textundostack.h"
#include "edbee/views/components/texteditorcomponent.h"
#include "edbee/views/components/textmargincomponent.h"
#include "edbee/views/texteditorscrollarea.h"
#include "edbee/views/textrenderer.h"
#include "edbee/views/textselection.h"
#include "edbee/views/texttheme.h"

#include "edbee/debug.h"


//#define DEBUG_DRAW_RENDER_CLIPPING_RECTANGLE 1

namespace edbee {


/// The default TextEditor widget constructor
TextEditorWidget::TextEditorWidget( QWidget* parent)
    : QWidget( parent )
    , controller_(0)
    , scrollAreaRef_(0)
    , editCompRef_(0)
{
    // auto initialize edbee if this hasn't been done alread
    Edbee::instance()->autoInit();

    // create the controller
    controller_ = new TextEditorController(this );

    // setup the ui
    scrollAreaRef_ = new class TextEditorScrollArea(this);
    scrollAreaRef_->setWidgetResizable(true);

    editCompRef_   = new TextEditorComponent( controller_, scrollAreaRef_);
    marginCompRef_ = new TextMarginComponent( this, scrollAreaRef_ );


    scrollAreaRef_->setWidget( editCompRef_ );
    scrollAreaRef_->setLeftWidget( marginCompRef_ );
//    scrollAreaRef_->setLeftWidget( new QLabel("Left",this) );//marginCompRef_ );
//    scrollAreaRef_->setTopWidget( new QLabel("Top",this));
//    scrollAreaRef_->setRightWidget( new QLabel("Right",this));
//    scrollAreaRef_->setBottomWidget( new QLabel("Bottom",this));

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget( scrollAreaRef_ );
    layout->setMargin(0);

    setLayout(layout);
    setFocusProxy( editCompRef_ );

    marginCompRef_->init();
    connectHorizontalScrollBar();
    connectVerticalScrollBar();
    connect( this, SIGNAL(horizontalScrollBarChanged(QScrollBar*)), SLOT(connectHorizontalScrollBar()) );
    connect( this, SIGNAL(verticalScrollBarChanged(QScrollBar*)), SLOT(connectVerticalScrollBar()) );


    setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );

    editCompRef_->installEventFilter(this);     // recieve events for the ability to emit focus events
}


/// The editor constructor
TextEditorWidget::~TextEditorWidget()
{
    // we need to perform manual deletion to force the deletion order
    delete marginCompRef_;
    delete editCompRef_;
    delete controller_;
}


/// This method makes sure the given position is visible
/// @param xposIn the position in text-editor 'coordinates'
/// @param yPosIn the position in text-editor 'coordinates'
void TextEditorWidget::scrollPositionVisible(int xPosIn, int yPosIn)
{
    scrollAreaRef_->ensureVisible( xPosIn, yPosIn );
}


/// returns the controller for this editor
/// @return the active TextEditorController
TextEditorController* TextEditorWidget::controller() const
{
    return controller_;
}


/// this method scrolls the top position to the given line
/// @param line the line to scroll to
void TextEditorWidget::scrollTopToLine(int line)
{
    int yPos = line * textRenderer()->lineHeight();
    scrollAreaRef_->verticalScrollBar()->setValue( qMax(0,yPos) );
//    scrollAreaRef_->ensureVisible( 0,  qMax(0,yPos) );
}


/// Returns the confirguation object for this widget
TextEditorConfig* TextEditorWidget::config() const
{
    return textDocument()->config();
}


/// Return the associated commandmap
TextEditorCommandMap* TextEditorWidget::commandMap() const
{
    return controller_->commandMap();
}


/// return the associated keymap
TextEditorKeyMap* TextEditorWidget::keyMap() const
{
    return controller_->keyMap();
}


/// Returns the textdocument for this widget
TextDocument* TextEditorWidget::textDocument() const
{
    return controller_->textDocument();
}


/// Returns the text-renderer
TextRenderer* TextEditorWidget::textRenderer() const
{
    return controller_->textRenderer();
}


/// Return the textselection
TextSelection* TextEditorWidget::textSelection() const
{
    return controller_->textSelection();
}


/// Returns the editor component-part of the editor
TextEditorComponent* TextEditorWidget::textEditorComponent() const
{
    return editCompRef_;
}


/// Returns the active margin component
TextMarginComponent* TextEditorWidget::textMarginComponent() const
{
    return marginCompRef_;
}


/// Returns the active scroll area
TextEditorScrollArea* TextEditorWidget::textScrollArea() const
{
    return scrollAreaRef_;
}


/// This method resets the caret time
void TextEditorWidget::resetCaretTime()
{
    editCompRef_->resetCaretTime();
}


/// This method performs a full update. Which means it callibirates the scrollbars
/// invalidates all caches, redraws the screen and updates the scrollbars
void TextEditorWidget::fullUpdate()
{
    editCompRef_->fullUpdate();
    marginCompRef_->fullUpdate();
}


/// Return the horizontal scrollbar
QScrollBar* TextEditorWidget::horizontalScrollBar() const
{
    return scrollAreaRef_->horizontalScrollBar();
}


/// returns the vertical scrollbar
QScrollBar* TextEditorWidget::verticalScrollBar() const
{
    return scrollAreaRef_->verticalScrollBar();
}


/// Use this method to change a scrollbar. By using this method listeners of the scrollbar
/// well be informed the scrollbar has been changed
/// @param scrollBar the scrollbar to use
void TextEditorWidget::setVerticalScrollBar(QScrollBar* scrollBar)
{
    scrollAreaRef_->setVerticalScrollBar(scrollBar);
    emit verticalScrollBarChanged( scrollBar );
}


/// Use this method to change the horizontal scrollbar.  By using this method listeners of the scrollbar
/// well be informed the scrollbar has been changed
/// @param scrollBar the scrollbar to use
void TextEditorWidget::setHorizontalScrollBar(QScrollBar* scrollBar)
{
    scrollAreaRef_->setHorizontalScrollBar(scrollBar);
    emit verticalScrollBarChanged( scrollBar );
}


/// This mehtod is called when a resize happens
/// @param event the event of the editor widget
void TextEditorWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    updateRendererViewport();
}


/// a basic event-filter for recieving focus-events of the editor
/// @param obj the object to filter the events for
/// @param event the event to filter
bool TextEditorWidget::eventFilter(QObject* obj, QEvent* event)
{
    if( obj == editCompRef_) {
        if ( event->type() == QEvent::FocusIn ) {
            emit focusIn( this );
        }
        if ( event->type() == QEvent::FocusOut ) {
            emit focusOut(this);
        }
    }
    return false;
}


//=================================================================================
// protected slots
//=================================================================================

/// Connects the vertical scrollbar so it updates the rendering viewport
void TextEditorWidget::connectVerticalScrollBar()
{
    connect( verticalScrollBar(), SIGNAL(valueChanged(int)), SLOT(updateRendererViewport()) );
}


/// Connects the horizontal scrollbar so it updates the rendering viewport
void TextEditorWidget::connectHorizontalScrollBar()
{
    connect( horizontalScrollBar(), SIGNAL(valueChanged(int)), SLOT(updateRendererViewport()) );
}


//=================================================================================
// public slots
//=================================================================================


/// updates the given line so it will be repainted
/// @Param offset the offset of the line that needs repainting
void TextEditorWidget::updateLineAtOffset(int offset)
{
    editCompRef_->updateLineAtOffset(offset);
    marginCompRef_->updateLineAtOffset(offset);
}


/// updates the character before and at the given offest
/// @param offset the offset of the area to repaint.
/// @param width the width of the area to update (default is 8 pixels)
void TextEditorWidget::updateAreaAroundOffset(int offset, int width )
{
    editCompRef_->updateAreaAroundOffset(offset,width);
    marginCompRef_->updateLineAtOffset(offset);
}


/// This method repaints the given lines
/// @param line the line that updates the given line
/// @param length the number of lines to update. (default is 1 line)
void TextEditorWidget::updateLine(int line, int length)
{
    editCompRef_->updateLine(line,length);
    marginCompRef_->updateLine(line,length);

}


/// Calls update on all components
void TextEditorWidget::updateComponents()
{
    editCompRef_->update();
    marginCompRef_->update();
}


/// Updates the geometry for all components
void TextEditorWidget::updateGeometryComponents()
{
    // force size change (ugly)
    textEditorComponent()->updateGeometry();

    // when a geometry change is required recalculate all widgets
    if( textMarginComponent()->isGeometryChangeRequired() ) {
        scrollAreaRef_->layoutMarginWidgets();
    }
}


/// Updates the renderer viewport.
void TextEditorWidget::updateRendererViewport()
{
    QRect rect( horizontalScrollBar()->value(), this->verticalScrollBar()->value(), scrollAreaRef_->viewport()->width(), scrollAreaRef_->height() );
    controller()->textRenderer()->setViewport( rect );
}



} // edbee
