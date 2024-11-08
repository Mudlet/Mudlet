/***************************************************************************
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "SingleLineTextEdit.h"
#include <QKeyEvent>

SingleLineTextEdit::SingleLineTextEdit(QWidget *parent)
    : QTextEdit(parent)
{
    highlighter = new TriggerHighlighter(this->document());
    highlighter->setHighlightingEnabled(true);
    // Restrict the height to a single line by setting fixed height based on font size
    //setFixedHeight(fontMetrics().height() + 10); // Adjust as needed for padding
    setWordWrapMode(QTextOption::NoWrap); // Prevent text wrapping
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

// Override keyPressEvent to prevent new lines and handle Enter key
void SingleLineTextEdit::keyPressEvent(QKeyEvent *event)
{
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

void SingleLineTextEdit::setHighlightingEnabled(bool enabled)
{
    highlighter->setHighlightingEnabled(enabled);
}
