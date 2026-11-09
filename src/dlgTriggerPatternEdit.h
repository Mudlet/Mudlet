#ifndef MUDLET_DLGTRIGGERPATTERNEDIT_H
#define MUDLET_DLGTRIGGERPATTERNEDIT_H

/***************************************************************************
 *   Copyright (C) 2008-2009 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2019, 2022 by Stephen Lyons - slysven@virginmedia.com   *
 *                                                                         *
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


#include "ui_trigger_pattern_edit.h"
#include "utils.h"

#include <QColor>
#include <QIcon>
#include <QPalette>


class QAction;

class dlgTriggerPatternEdit : public QWidget, public Ui::trigger_pattern_edit
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(dlgTriggerPatternEdit)
    explicit dlgTriggerPatternEdit(QWidget*);

    void applyThemePalette(const QPalette& editorPalette);

    // Drawn on hover, and on focus since a control that can be tabbed to must then be visible.
    // An empty icon rather than a hidden button, so crossing the rows does not shuffle them.
    void setDeleteGlyph(const QIcon& deleteGlyph);

    void setHoverTint(const QColor& hoverTint);

    int mRow = 0;


protected:
    // A QWidget subclass draws nothing a stylesheet gives it without this
    void paintEvent(QPaintEvent*) override;
    // Qt sends these along the whole chain under the mouse, so a child control keeps the row hovered
    void enterEvent(TEnterEvent*) override;
    void leaveEvent(QEvent*) override;
    bool eventFilter(QObject* watched, QEvent* event) override;


private:
    void resetThemePalette();
    void setRowHovered(const bool hovered);
    void updateDeleteGlyph();

    QIcon mDeleteGlyph;
    QColor mHoverTint;
    bool mRowHovered = false;
    bool mDeleteButtonFocused = false;

    QPalette mDefaultPalette;
    QPalette mDefaultPatternNumberPalette;
    QPalette mDefaultPromptPalette;
    QPalette mDefaultComboPalette;
    QPalette mDefaultSpinPalette;
    QPalette mDefaultForegroundButtonPalette;
    QPalette mDefaultBackgroundButtonPalette;
    QPalette mDefaultPatternEditPalette;
    QPalette mDefaultPatternEditViewportPalette;
    bool mDefaultPatternEditViewportAutoFillBackground = false;
};

#endif // MUDLET_DLGTRIGGERPATTERNEDIT_H
