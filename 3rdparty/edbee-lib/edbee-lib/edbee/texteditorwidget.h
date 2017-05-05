/**
 * Copyright 2011-2013 - Reliable Bits Software by Blommers IT. All Rights Reserved.
 * Author Rick Blommers
 */

#pragma once

//#include <QAbstractScrollArea>
#include <QStringList>
#include <QWidget>

class QResizeEvent;
class QScrollBar;

namespace edbee {

class TextDocument;
class TextEditorCommandMap;
class TextEditorComponent;
class TextEditorConfig;
class TextEditorController;
class TextEditorKeyMap;
class TextEditorScrollArea;
class TextMarginComponent;
class TextRenderer;
class TextSelection;


/// This is the general edbee widget
/// This core functionality of this widget is divided in several seperate
/// compnents. (TextEditorComponent: the main editor, TextMarginComponent: the sidebar with line numbers)
class TextEditorWidget : public QWidget
{
    Q_OBJECT
public:

    explicit TextEditorWidget(QWidget *parent = 0);
    virtual ~TextEditorWidget();

    void scrollPositionVisible( int xPosIn, int yPosIn );

    // a whole bunch of getters
    TextEditorController* controller() const;
    TextEditorConfig* config() const;
    TextEditorCommandMap* commandMap() const;
    TextEditorKeyMap* keyMap() const;
    TextDocument* textDocument() const;
    TextRenderer* textRenderer() const;
    TextSelection* textSelection() const;
    TextEditorComponent* textEditorComponent() const;
    TextMarginComponent* textMarginComponent() const;
    TextEditorScrollArea* textScrollArea() const;

    void resetCaretTime();
    void fullUpdate();

    QScrollBar* horizontalScrollBar() const;
    QScrollBar* verticalScrollBar() const;
    void setVerticalScrollBar( QScrollBar* scrollBar );
    void setHorizontalScrollBar( QScrollBar* scrollBar );

protected:

    virtual void resizeEvent(QResizeEvent* event);
    bool eventFilter(QObject *obj, QEvent *event );

signals:
    void focusIn(QWidget* event);
    void focusOut(QWidget* event);

    void verticalScrollBarChanged( QScrollBar* newScrollBar );
    void horizontalScrollBarChanged(  QScrollBar* newScrollBar );

protected slots:

    void connectVerticalScrollBar();
    void connectHorizontalScrollBar();

public slots:

    void scrollTopToLine( int line );
    virtual void updateLineAtOffset(int offset);
    virtual void updateAreaAroundOffset(int offset, int width=8);
    virtual void updateLine( int line, int length=1 );
    virtual void updateComponents();

    virtual void updateGeometryComponents();

    virtual void updateRendererViewport();

private:

    TextEditorController* controller_;      ///< This method returns the controller

    TextEditorScrollArea* scrollAreaRef_;   ///< The scrollarea of the widget
    TextEditorComponent* editCompRef_;      ///< The editor ref
    TextMarginComponent* marginCompRef_;    ///< The margin components

};

} // edbee
