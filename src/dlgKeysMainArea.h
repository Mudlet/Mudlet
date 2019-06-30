#ifndef MUDLET_DLGKEYSMAINAREA_H
#define MUDLET_DLGKEYSMAINAREA_H

/***************************************************************************
 *   Copyright (C) 2008-2009 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2019 by Stephen Lyons - slysven@virginmedia.com         *
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


#include "pre_guard.h"
#include "ui_keybindings_main_area.h"
#include "post_guard.h"


class dlgKeysMainArea : public QWidget, public Ui::keybindings_main_area
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(dlgKeysMainArea)
    dlgKeysMainArea(QWidget*);
    void setModifiers(const QPair<Qt::KeyboardModifiers, Qt::KeyboardModifiers> modifiers);
    QPair<Qt::KeyboardModifiers, Qt::KeyboardModifiers> getModifiers() const;


signals:
    void signal_modifierKeyToggled(const Qt::KeyboardModifier, const bool);


private slots:
    void slot_modifierKeyToggled_shift(const bool);
    void slot_modifierKeyToggled_control(const bool);
    void slot_modifierKeyToggled_alt(const bool);
    void slot_modifierKeyToggled_meta(const bool);
    void slot_modifierKeyToggled_keypad(const bool);
    void slot_modifierKeyToggled_group(const bool);
    void slot_showModifierControls(const bool);

private:
    void setModifier(QCheckBox*, const bool isPresentModifierSet, const bool isAbsentModifierSet);
};

#endif // MUDLET_DLGKEYSMAINAREA_H
