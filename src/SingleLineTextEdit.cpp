// SingleLineTextEdit.cpp
#include "SingleLineTextEdit.h"
#include <QKeyEvent>

SingleLineTextEdit::SingleLineTextEdit(QWidget *parent)
    : QTextEdit(parent)
{
    // Restrict the height to a single line by setting fixed height based on font size
    //setFixedHeight(fontMetrics().height() + 10); // Adjust as needed for padding
    setWordWrapMode(QTextOption::NoWrap); // Prevent text wrapping
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

// Override keyPressEvent to prevent new lines and handle Enter key
void SingleLineTextEdit::keyPressEvent(QKeyEvent *event)
{
    // TODO: and TAB?
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        // Emit editingFinished signal similar to QLineEdit's behavior
        emit editingFinished();
    }
    QTextEdit::keyPressEvent(event); // Process other keys normally
}

// Override resizeEvent to ensure the height remains single-line fixed
void SingleLineTextEdit::resizeEvent(QResizeEvent *event)
{
    //setFixedHeight(fontMetrics().height() + 10); // Adjust to desired padding
    QTextEdit::resizeEvent(event);
}

