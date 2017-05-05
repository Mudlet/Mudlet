/**
 * Copyright 2011-2013 - Reliable Bits Software by Blommers IT. All Rights Reserved.
 * Author Rick Blommers
 */

#include "texteditorcomponent.h"

#include <QMenu>
#include <QPainter>
#include <QPaintEvent>
#include <QTimer>

#include "edbee/commands/selectioncommand.h"
#include "edbee/models/textdocument.h"
#include "edbee/models/texteditorcommandmap.h"
#include "edbee/models/texteditorconfig.h"
#include "edbee/models/texteditorkeymap.h"
#include "edbee/models/textundostack.h"
#include "edbee/texteditorcommand.h"
#include "edbee/texteditorcontroller.h"
#include "edbee/texteditorwidget.h"
#include "edbee/views/components/texteditorrenderer.h"
#include "edbee/views/texteditorscrollarea.h"
#include "edbee/views/textrenderer.h"
#include "edbee/views/textselection.h"

#include "edbee/debug.h"

namespace edbee {

/// The default texteditor compenent constructor
/// @param controller the active controller for this editor
/// @param parent the parent QObject
TextEditorComponent::TextEditorComponent(TextEditorController* controller, QWidget* parent)
    : QWidget(parent)
    , caretTimer_(0)
    , controllerRef_(controller)
    , textEditorRenderer_(0)
{
    textEditorRenderer_ = new TextEditorRenderer( controller->textRenderer());

//    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding );

    // disabled autofilling of backgrounds
    setAutoFillBackground(false);

    //  editCompRef_->setAutoFillBackground(false);
    //  setAutoFillBackground(true);
    //  setAttribute(Qt::WA_StaticContents);
    setAttribute(Qt::WA_OpaquePaintEvent);          // make sure not everything is redrawn
    setAttribute(Qt::WA_NoSystemBackground);

//    setAttribute( Qt::WA_MacShowFocusRect); // show a mac focus rect

    setFocusPolicy( Qt::WheelFocus   );
    //  setFocusPolicy(Qt::ClickFocus);       (since 2013-02-16, can be removed tab still works :) )
    setAttribute(Qt::WA_KeyCompression);
    setAttribute(Qt::WA_InputMethodEnabled);

    // set the cursor type
    setCursor(Qt::IBeamCursor);

    // set the timer for the carets
    caretTimer_ = new QTimer();
    connect( caretTimer_, SIGNAL(timeout()), SLOT(repaintCarets()) );
    if( config()->caretBlinkingRate() > 0 ) {
        caretTimer_->start( config()->caretBlinkingRate() >> 1 );
    }
}


/// The destructor of the the component
TextEditorComponent::~TextEditorComponent()
{
    delete caretTimer_;
    delete textEditorRenderer_;
}


/// Returns the active commandmap
TextEditorCommandMap* TextEditorComponent::commandMap()
{
    return controllerRef_->commandMap();
}


/// Returns the active configuration
TextEditorConfig* TextEditorComponent::config() const
{
    return textDocument()->config();
}


/// Returns the active textdocument
TextDocument* TextEditorComponent::textDocument() const
{
    return controllerRef_->textDocument();
}


/// Returns the current keymap
TextEditorKeyMap* TextEditorComponent::keyMap()
{
    return controllerRef_->keyMap();
}


/// Returns the active texteditor controller
TextEditorController* TextEditorComponent::controller()
{
    return controllerRef_;
}


/// Returns the texselection range
TextSelection *TextEditorComponent::textSelection()
{
    return controllerRef_->textSelection();
}

void TextEditorComponent::giveTextEditorRenderer(TextEditorRenderer *renderer)
{
    delete textEditorRenderer_;
    textEditorRenderer_ = renderer;
}


/// returns the size hint. This is the total size of the editor-area
QSize TextEditorComponent::sizeHint() const
{
    TextRenderer* ren = textRenderer();
    int height = ren->totalHeight();

    // when scroll past end is enabled we need to be able to scroll past the last line
    if( config()->scrollPastEnd() ) {
        int viewPortHeight = controllerRef_->widget()->textScrollArea()->size().height();
        height += viewPortHeight;
        height -= textRenderer()->lineHeight()*2;   // *2 because we have an extra blank line at the end
    }

    // added 1 extra emWidth so there's at least 1 character spacing at the end
    return QSize( ren->totalWidth() + ren->emWidth() , height );
}


/// This method resets the caret timer
/// The caret time is used for blinking carets
/// When resetting the timer the caret is displayed directly
void TextEditorComponent::resetCaretTime()
{
    // restart the timer
    if( caretTimer_->isActive() ) {
        caretTimer_->start();
        textRenderer()->resetCaretTime();
    }

}


/// A slow and full update of the control
void TextEditorComponent::fullUpdate()
{
//    qlog_info() << "**** fullUpdate!!! **** ";
    controller()->textRenderer()->invalidateCaches();
    updateGeometry();
    update();
}


/// paint the editor
/// @param paintEvent the paint event that occured
void TextEditorComponent::paintEvent(QPaintEvent* paintEvent)
{
    QWidget::paintEvent(paintEvent);
    QPainter p(this);

    // retrieve the scrollbar-offsets
    const QRect& clipRect = paintEvent->rect();

    int offsetX = 0; //horizontalScrollBar()->value();
    int offsetY = 0; //verticalScrollBar()->value();
    QRect translatedRect( clipRect.x()+offsetX, clipRect.y()+offsetY, clipRect.width(), clipRect.height() );

//qlog_info() << "CLIPRECT..... " << translatedRect;

    // render the editor
//    textRenderer()->render( p, translatedRect );
    textRenderer()->renderBegin(translatedRect);
    textEditorRenderer_->render(&p);
    textRenderer()->renderEnd(translatedRect);

#if DEBUG_DRAW_RENDER_CLIPPING_RECTANGLE
    // draw the untralated clipping rectangle

    // draw a clipping rectangle
    p.setClipping(false);
    p.setPen( QColor( 0,200,0 ));
  QRect clipDrawRect( clipRect );
    clipDrawRect.adjust(0,0,-1,-1);
    p.drawRect( clipDrawRect );
    p.setClipping(true);
#endif

//    QStyleOptionFocusRect option;
//    option.init(this);
//    option.backgroundColor = palette().color(QPalette::Window);
//    style()->drawPrimitive(QStyle::PE_FrameFocusRect, &option, &p,this);
}


/// This method is used to FORCE the interception of tab events
/// @param event the event filter
bool TextEditorComponent::event(QEvent* event)
{
    // keypress event
    if( event->type() == QEvent::KeyPress ) {
        QKeyEvent* keyEvent = dynamic_cast<QKeyEvent*>(event);
        if( keyEvent ) {
            int key = keyEvent->key();
            if( key == Qt::Key_Tab || key == Qt::Key_Backtab ) {
                keyPressEvent( keyEvent );
                keyEvent->accept();
                return true;
            }
        }
    }
    return QWidget::event( event );
}



/// handle keypresses
void TextEditorComponent::keyPressEvent(QKeyEvent* event)
{
    int key = event->key();

    // unkown key return or control key
    if( key == Qt::Key_unknown ) { return; }
    if(key == Qt::Key_Control || key == Qt::Key_Shift || key == Qt::Key_Alt || key == Qt::Key_Meta) { return; }

    // convert the key to a string
    Qt::KeyboardModifiers modifiers = event->modifiers();
    if(modifiers & Qt::ShiftModifier)    key += Qt::SHIFT;
    if(modifiers & Qt::ControlModifier)  key += Qt::CTRL;
    if(modifiers & Qt::AltModifier)      key += Qt::ALT;
    if(modifiers & Qt::MetaModifier)     key += Qt::META;
//    if(modifiers & Qt::KeypadModifier)   key += Qt::KeypadModifier;

    // grow the current sequence
    QKeySequence::SequenceMatch match;
    QKeySequence keySequence = TextEditorKeyMap::joinKeySequences( lastKeySequence_, QKeySequence( key ) );
    QString command = keyMap()->findBySequence( keySequence, match );

    // when a sequence was busy and we didn't find a match, eat the key :P
    // this is different from other editors like sublime, but I think it's more natural to
    // ignore the key if a sequence of keys is detected
    if( match == QKeySequence::NoMatch && !lastKeySequence_.isEmpty() ) {
        lastKeySequence_ = QKeySequence();
//        qlog_info() << "[[[ eat the last key ]]";
        return;
    }

    // a partial match
    if( match == QKeySequence::PartialMatch ) {
        lastKeySequence_ = keySequence;
//        qlog_info() << "[[[ match in progress for " << command << "]]";
        return;
    }

    // we've found the command
    if( match == QKeySequence::ExactMatch ) {
//        qlog_info() << "[[[ found command:" << command << "]]";
        controller()->executeCommand( command );
        lastKeySequence_ = QKeySequence();
        return;
    }

    // not partial, not found, clear the mark
    lastKeySequence_ = QKeySequence();

    // else replace the selection if there's a text
    QString text = event->text();
    bool specialKey =(modifiers&(Qt::MetaModifier|Qt::ControlModifier));
    if( !text.isEmpty() && !specialKey ) {
        // last character is used for "undo-group after" space support
        if( this->config()->undoGroupPerSpace() ) {
            if( text.compare(" ") == 0 && lastCharacter_.compare(" ") != 0  ) {
                textDocument()->textUndoStack()->resetAllLastCoalesceIds();
            }
        }
        lastCharacter_ = text;
        controller()->replaceSelection( text, CoalesceId_AppendChar );
        controller()->updateStatusText();
    } else {
        QWidget::keyPressEvent(event);
    }

}


/// the key release event
void TextEditorComponent::keyReleaseEvent(QKeyEvent *event)
{
    Q_UNUSED(event);
}


/// input method event
// When receiving an input method event, the text widget has to performs the following steps:
// 1. If the widget has selected text, the selected text should get removed.
///
// 2. Remove the text starting at replacementStart() with length replacementLength() and replace it by the commitString().
//     If replacementLength() is 0, replacementStart() gives the insertion position for the commitString().
//  .  When doing replacement the area of the preedit string is ignored, thus a replacement starting at -1 with a length of 2 will remove the
//     last character before the preedit string and the first character afterwards, and insert the commit string directly before the preedit string.
// .   If the widget implements undo/redo, this operation gets added to the undo stack.
//
// 3 If there is no current preedit string, insert the preeditString() at the current cursor position; otherwise
//     replace the previous preeditString with the one received from this event.
//     If the widget implements undo/redo, the preeditString() should not influence the undo/redo stack in any way.
//     The widget should examine the list of attributes to apply to the preedit string. It has to understand at least the
//     TextFormat and Cursor attributes and render them as specified.

void TextEditorComponent::inputMethodEvent( QInputMethodEvent* m )
{
   // replace the selection with an empty text
    if( textSelection()->hasSelection() ) {
        controller()->replaceSelection("",false);
    }

/*
    /// TODO: Honer the arguments
    QString str;
    foreach( QInputMethodEvent::Attribute attr, m->attributes() ) {
        str.append( QString("[%1,%2-%3,%4]").arg(attr.type).arg(attr.start).arg(attr.length).arg(attr.value.toString()) );
    }
//    qlog_info() << "inputMethodEvent: commitStr=" << m->commitString() << ", preEditStr=" << m->preeditString() << ", start=" << m->replacementStart() << ", length" << m->replacementStart()  << " | " << str;
*/

    if( !m->preeditString().isEmpty() ) {
//        replaceSelection(m->preeditString());
    } else {
        controller()->replaceSelection(m->commitString(),false);
    }

}


/// currently unused ?!
QVariant TextEditorComponent::inputMethodQuery( Qt::InputMethodQuery p ) const
{
    Q_UNUSED(p);
    return QVariant();
}


/// A mouse press happens
///
/// The normal operation is to move the caret to the character under the given mouse position.
/// When shift is pressed, the selection is extended to the given position
///
/// If the controller modifier is used a new caret is added to character at the given position
///
/// @param event the mouse event
void TextEditorComponent::mousePressEvent(QMouseEvent* event)
{
    if( event->button() == Qt::LeftButton ) {
        TextRenderer* renderer = textRenderer();

//        int x = renderer->widgetXToXpos( event->x() + horizontalScrollBar()->value() );
//        int y = renderer->widgetYToYpos( event->y() + verticalScrollBar()->value() );
        int x = event->x();
        int y = event->y();

        int line = renderer->rawLineIndexForYpos( y );
        int col = renderer->columnIndexForXpos( line, x );

        if( event->modifiers()&Qt::ControlModifier ) {
            controller()->addCaretAt( line, col );
        } else {
            controller()->moveCaretTo( line, col, event->modifiers()&Qt::ShiftModifier );
        }
    }
    QWidget::mousePressEvent(event);
}


/// A mouse double click happens
/// @param event the mouse double click that occured
void TextEditorComponent::mouseDoubleClickEvent( QMouseEvent* event )
{
    static SelectionCommand selectWord( SelectionCommand::SelectWord );
    if( event->button() == Qt::LeftButton ) {

        if( event->modifiers()&Qt::ControlModifier ) {

            // get the location of the double
            int x = event->x();
            int y = event->y();
            int line = textRenderer()->rawLineIndexForYpos( y );
            int col = textRenderer()->columnIndexForXpos( line, x );

            // add the word there
            SelectionCommand toggleWordSelectionAtCommand( SelectionCommand::ToggleWordSelectionAt, textDocument()->offsetFromLineAndColumn(line,col) );
            controller()->executeCommand( &toggleWordSelectionAtCommand );
        } else {
            controller()->executeCommand( &selectWord  );
        }
        return;
    }
    QWidget::mouseDoubleClickEvent(event);
}



/// A mouse move event
/// @param event the mouse event
void TextEditorComponent::mouseMoveEvent(QMouseEvent* event )
{
    if( event->buttons() & Qt::LeftButton ) {
        TextRenderer* renderer = textRenderer();

//        int x = renderer->widgetXToXpos( event->x() + horizontalScrollBar()->value() );
//        int y = renderer->widgetYToYpos( event->y() + verticalScrollBar()->value() );
        int x = event->x();
        int y = event->y();

        int line = renderer->rawLineIndexForYpos( y );
        int col = 0;
        if( line >= 0 ) { col = renderer->columnIndexForXpos( line, x ); }
        if( line < 0 ) { line = 0; }

        controller()->moveCaretTo( line, col, true );
    }
    QWidget::mouseMoveEvent(event);
}


/// A focus in event occured
void TextEditorComponent::focusInEvent(QFocusEvent *event)
{
    Q_UNUSED(event);
}


/// The focus is lost, we must STOP coalescing of undo-events!
void TextEditorComponent::focusOutEvent(QFocusEvent *event)
{
    Q_UNUSED(event);
    // no merging for new changes!!
    textDocument()->textUndoStack()->resetAllLastCoalesceIds();
}


/// The default context menu is requested
/// Add the default menu actions
void TextEditorComponent::contextMenuEvent(QContextMenuEvent* event)
{
    Q_UNUSED(event);

    QMenu* menu = new QMenu();
    menu->addAction( controller()->createAction("cut", tr("Cut") ) );
    menu->addAction( controller()->createAction("copy", tr("Copy") ) );
    menu->addAction( controller()->createAction("paste", tr("Paste") ) );
    menu->addSeparator();
    menu->addAction( controller()->createAction("sel_all", tr("Select All") ) );

    // contextmenu can always be placed under the current cursosr
    menu->exec( QCursor::pos() );
    qDeleteAll(menu->actions());
    delete menu;
}


/// This method repaints 'all' carets
void TextEditorComponent::repaintCarets()
{
    bool visible = textRenderer()->isCaretVisible();
    bool focus = hasFocus();
    if( focus != visible ) {
        textRenderer()->setCaretVisible(focus);
    } else {
        if( !visible ) { return; }
    }
//    /// BIG TODO: Optimize so only blinks visible carets
//        textRenderer()->toggleCaretVisible( );
//    } else {
//        textRenderer()->setCaretVisible(false);
//    }

    // invalidate all 'caret' ranges
    TextSelection* ranges = controllerRef_->textSelection();
    int rangeCount = ranges->rangeCount();
    if( rangeCount < 10 ) {  // just a number :P
        for( int i=0; i<rangeCount; ++i ) {
            updateAreaAroundOffset( ranges->range(i).caret() );
        }
    } else {
        update();
    }
}



/// updates the given line so it will be repainted
void TextEditorComponent::updateLineAtOffset(int offset)
{
    TextRenderer* renderer = textRenderer();
    int yPos = renderer->yPosForOffset( offset ) - textEditorRenderer_->extraPixelsToUpdateAroundLines();

// the text-only line:
//    viewport()->update( renderer->viewportX(), yPos - renderer->viewportY(), renderer->viewportWidth(), renderer->lineHeight()  );

    // the full line inclusive sidebars
//    viewport()->update( renderer->totalViewportX(), yPos - renderer->totalViewportY(), renderer->totalViewportWidth(), renderer->lineHeight()  );
    update( 0,  yPos - renderer->viewportY(), renderer->viewportWidth(), renderer->lineHeight() + textEditorRenderer_->extraPixelsToUpdateAroundLines()*2  );
}


/// updates the character before and at the given offest
void TextEditorComponent::updateAreaAroundOffset(int offset, int width )
{
    TextRenderer* ren = textRenderer();
    update( ren->xPosForOffset(offset)-width/2, ren->yPosForOffset(offset)-textEditorRenderer_->extraPixelsToUpdateAroundLines(), width, ren->lineHeight()+textEditorRenderer_->extraPixelsToUpdateAroundLines()*2 );
/*
    int yPos = renderer->yPosToWidgetY( renderer->yPosForOffset( offset ) );
    int xPos = renderer->xPosToWidgetX( renderer->xPosForOffset( offset ) - width/2 );
    viewport()->update( xPos - renderer->totalViewportX(), yPos - renderer->totalViewportY(), width, renderer->lineHeight()  );
*/
}


/// This method invalidates the given lines
/// @param line the first line to update
/// @param length the number of lines to update
void TextEditorComponent::updateLine(int line, int length)
{
    TextRenderer* ren = textRenderer();
    int startY = ren->yPosForLine( line ) - textEditorRenderer_->extraPixelsToUpdateAroundLines();
    int endY   = ren->yPosForLine( line + length ) + textEditorRenderer_->extraPixelsToUpdateAroundLines();
    update( 0, startY, width(), endY );
}


/// Returns the current textrenderer
TextRenderer *TextEditorComponent::textRenderer() const
{
    return controllerRef_->textRenderer();
}


} // edbee
