/**
 * Copyright 2011-2013 - Reliable Bits Software by Blommers IT. All Rights Reserved.
 * Author Rick Blommers
 */

#pragma once

#include <QSize>
#include <QWidget>

namespace edbee {

class TextDocument;
class TextEditorCommandMap;
class TextEditorConfig;
class TextEditorController;
class TextEditorRenderer;
class TextEditorKeyMap;
class TextRenderer;
class TextSelection;


/// This is the main texteditor-component (which is the true editor)
/// This is the QWidget that recieves the keypresses, mouse presses etc.
class TextEditorComponent : public QWidget
{
    Q_OBJECT
public:
    explicit TextEditorComponent( TextEditorController* controller, QWidget *parent = 0);
    virtual ~TextEditorComponent();

    TextEditorCommandMap* commandMap();
    TextEditorConfig* config() const;
    TextDocument* textDocument() const;
    TextEditorKeyMap* keyMap();
    TextEditorController* controller();
    TextSelection* textSelection();

    TextEditorRenderer* textEditorRenderer() { return textEditorRenderer_; }
    void giveTextEditorRenderer( TextEditorRenderer* renderer );

    void resetCaretTime();
    void fullUpdate();

    virtual QSize sizeHint() const;

protected:
    virtual void paintEvent( QPaintEvent* paintEvent );
    virtual bool event( QEvent* event );
    virtual void keyPressEvent( QKeyEvent* event );
    virtual void keyReleaseEvent ( QKeyEvent* event );
    void inputMethodEvent( QInputMethodEvent* m );
    QVariant inputMethodQuery( Qt::InputMethodQuery p ) const;
    virtual void mousePressEvent( QMouseEvent* event );
    virtual void mouseDoubleClickEvent( QMouseEvent* event );
    virtual void mouseMoveEvent( QMouseEvent* event );
    virtual void focusInEvent( QFocusEvent* event );
    virtual void focusOutEvent( QFocusEvent* event );
    virtual void contextMenuEvent(QContextMenuEvent* event);

public slots:
    
    void repaintCarets();
    virtual void updateLineAtOffset(int offset);
    virtual void updateAreaAroundOffset(int offset, int width=8);
    virtual void updateLine( int line, int length );

private:

    TextRenderer* textRenderer() const;

    QTimer* caretTimer_;                ///< A timer for updating the carets

    QKeySequence lastKeySequence_;              ///< the last key sequence
    QString lastCharacter_;                     ///< The last added character. (for undo-group per space support)

    TextEditorController* controllerRef_;       ///< A reference to the controller
    TextEditorRenderer* textEditorRenderer_;    /// A text-editor renderer
};

} // edbee
