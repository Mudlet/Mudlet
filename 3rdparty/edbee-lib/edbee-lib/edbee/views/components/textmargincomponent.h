/**
 * Copyright 2011-2013 - Reliable Bits Software by Blommers IT. All Rights Reserved.
 * Author Rick Blommers
 */

#pragma once

#include <QWidget>

class QEvent;
class QFont;
class QLinearGradient;
class QMouseEvent;
class QPainter;

namespace edbee {

class TextEditorWidget;
class TextMarginComponent;
class TextRenderer;

/// The textmargin component delegate
/// You can override the methods in the class for adding functionality to the text-margin component
class TextMarginComponentDelegate
{
public:    
    TextMarginComponentDelegate();
    virtual ~TextMarginComponentDelegate() {}

    virtual QString lineText( int line );

    virtual int widthBeforeLineNumber();
    virtual void renderBefore( QPainter* painter, int startLine, int endLine, int width );
    virtual void renderAfter( QPainter* painter, int startLine, int endLine, int width );

    virtual bool requiresMouseTracking();
    virtual void mouseMoveEvent( int line, QMouseEvent* event );
    virtual void mousePressEvent( int line, QMouseEvent* event );
    virtual void leaveEvent( QEvent* event );

    TextMarginComponent* marginComponent() { return marginComponentRef_; }
    void setMarginCompenent( TextMarginComponent* comp ) { marginComponentRef_ = comp; }

private:
    TextMarginComponent* marginComponentRef_;           /// A reference to the margincomponent
};


/// The margin/line-number component
/// This class is used for rendering line-numbers etc
class TextMarginComponent : public QWidget
{
    Q_OBJECT

public:
    TextMarginComponent( TextEditorWidget* editorWidget, QWidget* parent);
    virtual ~TextMarginComponent();

    void init();

    int widthHint() const;
    virtual QSize sizeHint() const;
    bool isGeometryChangeRequired();

    void fullUpdate();
    TextEditorWidget* editorWidget() const { return editorRef_;}
    TextRenderer* renderer() const;

    TextMarginComponentDelegate* delegate() const { return delegateRef_; }
    void setDelegate( TextMarginComponentDelegate* delegate );
    void giveDelegate( TextMarginComponentDelegate* delegate );

protected:

    virtual void paintEvent( QPaintEvent* event );
    virtual void renderCaretMarkers( QPainter* painter, int startLine, int endLine, int width );
    virtual void renderLineNumber( QPainter* painter, int startLine, int endLine, int width );

    virtual void mouseMoveEvent(QMouseEvent* event);
    virtual void mousePressEvent(QMouseEvent* event);
    virtual void mouseDoubleClickEvent(QMouseEvent *);
    virtual void leaveEvent(QEvent* event);
    virtual void wheelEvent(QWheelEvent* event);

public:
    virtual void updateLineAtOffset(int offset);
    virtual void updateLine(int line, int length);

protected slots:

    virtual void topChanged(int value);
    virtual void connectScrollBar();

private:


    /// the width of the sidebar
    int top_;                                   ///< The first pixel that needs to been shown
    mutable int width_;
    mutable int lastLineCount_;
    QFont* marginFont_;                         ///< the font used for the margin
    TextEditorWidget* editorRef_;               ///< The text-editor widget
    TextMarginComponentDelegate* delegate_;     ///< The 'owned' text delegate
    TextMarginComponentDelegate* delegateRef_;  ///< The delegate reference
};

} // edbee
