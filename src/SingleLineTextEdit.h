#ifndef SINGLELINETEXTEDIT_H
#define SINGLELINETEXTEDIT_H

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

#include "TriggerHighlighter.h"

#include "pre_guard.h"
#include <QMimeData>
#include <QTextEdit>
#include "post_guard.h"

class SingleLineTextEdit : public QTextEdit
{
    Q_OBJECT

public:
    explicit SingleLineTextEdit(QWidget *parent = nullptr);
    void setHighlightingEnabled(bool enabled);
    void setTheme(const QString&);
    void rehighlight();

protected:
    void insertFromMimeData(const QMimeData *source) override;
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void focusOutEvent(QFocusEvent* event) override;

private:
    TriggerHighlighter *highlighter;
};

#endif // SINGLELINETEXTEDIT_H
