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


#include "dlgKeysMainArea.h"

#include <QCheckBox>

dlgKeysMainArea::dlgKeysMainArea(QWidget* pF)
: QWidget(pF)
{
    // init generated dialog
    setupUi(this);

    connect(checkBox_modifier_shift, &QCheckBox::toggled, this, &dlgKeysMainArea::slot_modifierKeyToggled_shift);
    connect(checkBox_modifier_control, &QCheckBox::toggled, this, &dlgKeysMainArea::slot_modifierKeyToggled_control);
    connect(checkBox_modifier_alt, &QCheckBox::toggled, this, &dlgKeysMainArea::slot_modifierKeyToggled_alt);
    connect(checkBox_modifier_meta, &QCheckBox::toggled, this, &dlgKeysMainArea::slot_modifierKeyToggled_meta);
    connect(checkBox_modifier_keypad, &QCheckBox::toggled, this, &dlgKeysMainArea::slot_modifierKeyToggled_keypad);
    connect(checkBox_modifier_group, &QCheckBox::toggled, this, &dlgKeysMainArea::slot_modifierKeyToggled_group);

    checkBox_modifier_keypad->setToolTip(tr("<p>On the MacOs platform this modifier may be set on the directional arrow keys whereas on other platforms it will <b>not</b> be set.</p>"));
#if defined(Q_OS_MACOS)
    checkBox_modifier_control->setText(tr("Command",
                                          // Intentional comment
                                          "This is the name of the control modifier key as it appears on the MacOs platform"));
    checkBox_modifier_meta->setText(tr("Control",
                                       // Intentional comment
                                       "This is the name of the meta modifier key as it appears on the MacOs platform"));
    checkBox_modifier_alt->setText(tr("Option",
                                      // Intentional comment
                                      "This is the name of the alt modifier key as it appears on the MacOs platform"));

    checkBox_modifier_control->setToolTip(tr("<p>On other platforms this modifier will be the <tt>Control</tt> key.</p>",
                                             // Intentional comment
                                             "This is the tooltip for the modifier key that Qt identifies as the control modifier, though it is called the command key in this usage on the MacOs platform."));
    checkBox_modifier_meta->setToolTip(tr("<p>On the Windows platform this modifier will be the <tt>Windows</tt> modifier and on other platforms the <tt>Meta</tt> modifier key.</p>",
                                          // Intentional comment
                                          "This is the tooltip for the modifier key that Qt identifies as the meta modifier, though it is called the control key in this usage on the MacOs platform."));
    checkBox_modifier_group->setToolTip(tr("<p>This modifier may only be present on other Unix-like platforms where it <i>may</i> be active when the <tt>AltGr</tt> key is used.</p>",
                                           // Intentional comment
                                           "This is the tooltip for the modifier that Qt identifies as the group switch modifier, though it may not be present on the MacOs platform."));
    checkBox_modifier_alt->setToolTip(tr("<p>On other platform this modifier will be the <tt>Alt</tt> key.</p>",
                                         // Intentional comment
                                         "This is the tooltip for the modifier that Qt identifies as the alt modifier, though it is called the option key in this usage on the MacOs platform."));
#elif defined(Q_OS_WIN32)
    checkBox_modifier_meta->setText(tr("Windows",
                                       // Intentional comment
                                       "This is the name of the modifier that Qt identifies as the meta modifier as it appears on the Windows platform"));

    checkBox_modifier_control->setToolTip(tr("<p>On the MacOs platform this will be the <tt>Command</tt> modifier.</p>",
                                             // Intentional comment
                                             "This is the tooltip for the modifier key that Qt identifies as the control modifier, though it is called the command key (only on) the MacOs platform."));
    checkBox_modifier_meta->setToolTip(tr("<p>On the MacOs platform this will be the <tt>Control</tt> modifier and on other platforms the <tt>Meta</tt> modifier.</p>",
                                          // Intentional comment
                                          "This is the tooltip for the modifier key that Qt identifies as the meta modifier, though it is called the control key on the MacOs platform."));
    checkBox_modifier_group->setToolTip(tr("<p>This modifier may only be present on Unix-like platforms where it <i>may</i> be active when the <tt>AltGr</tt> key is used.</p>",
                                           // Intentional comment
                                           "This is the tooltip for the modifier that Qt identifies as the group switch modifier, though it may not be present for usage on the Windows platform."));
    checkBox_modifier_alt->setToolTip(tr("<p>On the MacOs platform this will be the <tt>Option</tt> modifier.</p>",
                                         // Intentional comment
                                         "This is the tooltip for the modifier key that Qt identifies as the alt modifier, though it is called the option key on the MacOs platform."));
#else
    // Linux & FreeBSD:

    checkBox_modifier_control->setToolTip(tr("<p>On the MacOs platform this will be the <tt>Command</tt> modifier.</p>",
                                             // Intentional comment
                                             "This is the tooltip for the modifier key that Qt identifies as the control key, though it is called the command key on the MacOs platform."));
    checkBox_modifier_meta->setToolTip(tr("<p>On the MacOs platform this will be the <tt>Control</tt> modifier and on the Windows platform the <tt>Windows</tt> modifier.</p>",
                                          // Intentional comment
                                          "This is the tooltip for the modifier that Qt identifies as the meta key, though it is called the control key on the MacOs platform and the Windows key on the Windows platform."));
    checkBox_modifier_alt->setToolTip(tr("<p>On the MacOs platform this will be the <tt>Option</tt> modifier.</p>",
                                         // Intentional comment
                                         "This is the tooltip for the modifier that Qt identifies as the alt key, though it is called the option key on the MacOs platform."));
    checkBox_modifier_group->setToolTip(tr("<p>This modifier may only be present on Unix-like platforms where it <i>may</i> be active when the <tt>AltGr</tt> key is used.</p>",
                                           // Intentional comment
                                           "This is the tooltip for the modifier that Qt identifies as the group switch modifier, it may only be present on Unix like platforms (not Windows, not MacOs)."));
#endif
}

// Sets the modifiers without causing any signals to be emitted
void dlgKeysMainArea::setModifiers(const Qt::KeyboardModifiers modifiers)
{
    checkBox_modifier_shift->blockSignals(true);
    checkBox_modifier_shift->setChecked(modifiers & Qt::ShiftModifier);
    checkBox_modifier_shift->blockSignals(false);

    checkBox_modifier_control->blockSignals(true);
    checkBox_modifier_control->setChecked(modifiers & Qt::ControlModifier);
    checkBox_modifier_control->blockSignals(false);

    checkBox_modifier_alt->blockSignals(true);
    checkBox_modifier_alt->setChecked(modifiers & Qt::AltModifier);
    checkBox_modifier_alt->blockSignals(false);

    checkBox_modifier_meta->blockSignals(true);
    checkBox_modifier_meta->setChecked(modifiers & Qt::MetaModifier);
    checkBox_modifier_meta->blockSignals(false);

    checkBox_modifier_keypad->blockSignals(true);
    checkBox_modifier_keypad->setChecked(modifiers & Qt::KeypadModifier);
    checkBox_modifier_keypad->blockSignals(false);

    checkBox_modifier_group->blockSignals(true);
    checkBox_modifier_group->setChecked(modifiers & Qt::GroupSwitchModifier);
    checkBox_modifier_group->blockSignals(false);
}

Qt::KeyboardModifiers dlgKeysMainArea::getModifiers() const
{
    return (checkBox_modifier_shift->isChecked() ? Qt::ShiftModifier : Qt::NoModifier)
            | (checkBox_modifier_control->isChecked() ? Qt::ControlModifier : Qt::NoModifier)
            | (checkBox_modifier_alt->isChecked() ? Qt::AltModifier : Qt::NoModifier)
            | (checkBox_modifier_meta->isChecked() ? Qt::MetaModifier : Qt::NoModifier)
            | (checkBox_modifier_keypad->isChecked() ? Qt::KeypadModifier : Qt::NoModifier)
            | (checkBox_modifier_group->isChecked() ? Qt::GroupSwitchModifier : Qt::NoModifier);
}


void dlgKeysMainArea::slot_modifierKeyToggled_shift(const bool s)
{
    emit signal_modifierKeyToggled(Qt::ShiftModifier, s);
}

void dlgKeysMainArea::slot_modifierKeyToggled_control(const bool s)
{
    emit signal_modifierKeyToggled(Qt::ControlModifier, s);
}

void dlgKeysMainArea::slot_modifierKeyToggled_alt(const bool s)
{
    emit signal_modifierKeyToggled(Qt::AltModifier, s);
}

void dlgKeysMainArea::slot_modifierKeyToggled_meta(const bool s)
{
    emit signal_modifierKeyToggled(Qt::MetaModifier, s);
}

void dlgKeysMainArea::slot_modifierKeyToggled_keypad(const bool s)
{
    emit signal_modifierKeyToggled(Qt::KeypadModifier, s);
}

void dlgKeysMainArea::slot_modifierKeyToggled_group(const bool s)
{
    emit signal_modifierKeyToggled(Qt::GroupSwitchModifier, s);
}
