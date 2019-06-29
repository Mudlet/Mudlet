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

    checkBox_modifier_shift->setCheckState(Qt::PartiallyChecked);
    checkBox_modifier_control->setCheckState(Qt::PartiallyChecked);
    checkBox_modifier_alt->setCheckState(Qt::PartiallyChecked);
    checkBox_modifier_meta->setCheckState(Qt::PartiallyChecked);
    checkBox_modifier_keypad->setCheckState(Qt::PartiallyChecked);
    checkBox_modifier_group->setCheckState(Qt::PartiallyChecked);

    checkBox_modifier_keypad->setToolTip(tr("<p>On the MacOs platform this modifier may be set on the directional arrow keys whereas on other platforms it will <b>not</b> be set.</p>"
                                            "<p>All six of these controls have three states with the partially checked state being the default meaning <i>this modifier is ignored</i>; otherwise it must be present (checked) or absent (unchecked) for the key-binding to fire.  This is so that separate <i>combinations</i> of modifiers can be used for the <i>same</i> base key.</p>"));
    checkBox_modifier_shift->setToolTip(tr("<p>All six of these controls have three states with the partially checked state being the default meaning <i>this modifier is ignored</i>; otherwise it must be present (checked) or absent (unchecked) for the key-binding to fire.  This is so that separate <i>combinations</i> of modifiers can be used for the <i>same</i> base key.</p>"));

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

    checkBox_modifier_control->setToolTip(tr("<p>On other platforms this modifier will be the <tt>Control</tt> key.</p>"
                                          "<p>All six of these controls have three states with the partially checked state being the default meaning <i>this modifier is ignored</i>; otherwise it must be present (checked) or absent (unchecked) for the key-binding to fire.  This is so that separate <i>combinations</i> of modifiers can be used for the <i>same</i> base key.</p>",
                                          // Intentional comment
                                          "This is the tooltip for the modifier key that Qt identifies as the control modifier, though it is called the command key in this usage on the MacOs platform."));
    checkBox_modifier_meta->setToolTip(tr("<p>On the Windows platform this modifier will be the <tt>Windows</tt> modifier and on other platforms the <tt>Meta</tt> modifier key.</p>"
                                          "<p>All six of these controls have three states with the partially checked state being the default meaning <i>this modifier is ignored</i>; otherwise it must be present (checked) or absent (unchecked) for the key-binding to fire.  This is so that separate <i>combinations</i> of modifiers can be used for the <i>same</i> base key.</p>",
                                          // Intentional comment
                                          "This is the tooltip for the modifier key that Qt identifies as the meta modifier, though it is called the control key in this usage on the MacOs platform."));
    checkBox_modifier_group->setToolTip(tr("<p>This modifier may only be present on other Unix-like platforms where it <i>may</i> be active when the <tt>AltGr</tt> key is used.</p>"
                                           "<p>All six of these controls have three states with the partially checked state being the default meaning <i>this modifier is ignored</i>; otherwise it must be present (checked) or absent (unchecked) for the key-binding to fire.  This is so that separate <i>combinations</i> of modifiers can be used for the <i>same</i> base key.</p>",
                                           // Intentional comment
                                           "This is the tooltip for the modifier that Qt identifies as the group switch modifier, though it may not be present on the MacOs platform."));
    checkBox_modifier_alt->setToolTip(tr("<p>On other platform this modifier will be the <tt>Alt</tt> key.</p>"
                                         "<p>All six of these controls have three states with the partially checked state being the default meaning <i>this modifier is ignored</i>; otherwise it must be present (checked) or absent (unchecked) for the key-binding to fire.  This is so that separate <i>combinations</i> of modifiers can be used for the <i>same</i> base key.</p>",
                                         // Intentional comment
                                         "This is the tooltip for the modifier that Qt identifies as the alt modifier, though it is called the option key in this usage on the MacOs platform."));
#elif defined(Q_OS_WIN32)
    checkBox_modifier_meta->setText(tr("Windows",
                                       // Intentional comment
                                       "This is the name of the modifier that Qt identifies as the meta modifier as it appears on the Windows platform"));

    checkBox_modifier_control->setToolTip(tr("<p>On the MacOs platform this will be the <tt>Command</tt> modifier.</p>"
                                          "<p>All six of these controls have three states with the partially checked state being the default meaning <i>this modifier is ignored</i>; otherwise it must be present (checked) or absent (unchecked) for the key-binding to fire.  This is so that separate <i>combinations</i> of modifiers can be used for the <i>same</i> base key.</p>",
                                          // Intentional comment
                                          "This is the tooltip for the modifier key that Qt identifies as the control modifier, though it is called the command key (only on) the MacOs platform."));
    checkBox_modifier_meta->setToolTip(tr("<p>On the MacOs platform this will be the <tt>Control</tt> modifier and on other platforms the <tt>Meta</tt> modifier.</p>"
                                          "<p>All six of these controls have three states with the partially checked state being the default meaning <i>this modifier is ignored</i>; otherwise it must be present (checked) or absent (unchecked) for the key-binding to fire.  This is so that separate <i>combinations</i> of modifiers can be used for the <i>same</i> base key.</p>",
                                          // Intentional comment
                                          "This is the tooltip for the modifier key that Qt identifies as the meta modifier, though it is called the control key on the MacOs platform."));
    checkBox_modifier_group->setToolTip(tr("<p>This modifier may only be present on Unix-like platforms where it <i>may</i> be active when the <tt>AltGr</tt> key is used.</p>"
                                           "<p>All six of these controls have three states with the partially checked state being the default meaning <i>this modifier is ignored</i>; otherwise it must be present (checked) or absent (unchecked) for the key-binding to fire.  This is so that separate <i>combinations</i> of modifiers can be used for the <i>same</i> base key.</p>",
                                           // Intentional comment
                                           "This is the tooltip for the modifier that Qt identifies as the group switch modifier, though it may not be present for usage on the Windows platform."));
    checkBox_modifier_alt->setToolTip(tr("<p>On the MacOs platform this will be the <tt>Option</tt> modifier.</p>"
                                         "<p>All six of these controls have three states with the partially checked state being the default meaning <i>this modifier is ignored</i>; otherwise it must be present (checked) or absent (unchecked) for the key-binding to fire.  This is so that separate <i>combinations</i> of modifiers can be used for the <i>same</i> base key.</p>",
                                         // Intentional comment
                                         "This is the tooltip for the modifier key that Qt identifies as the alt modifier, though it is called the option key on the MacOs platform."));
#else
    // Linux & FreeBSD:

    checkBox_modifier_control->setToolTip(tr("<p>On the MacOs platform this will be the <tt>Command</tt> modifier.</p>"
                                          "<p>All six of these controls have three states with the partially checked state being the default meaning <i>this modifier is ignored</i>; otherwise it must be present (checked) or absent (unchecked) for the key-binding to fire.  This is so that separate <i>combinations</i> of modifiers can be used for the <i>same</i> base key.</p>",
                                          // Intentional comment
                                          "This is the tooltip for the modifier key that Qt identifies as the control key, though it is called the command key on the MacOs platform."));
    checkBox_modifier_meta->setToolTip(tr("<p>On the MacOs platform this will be the <tt>Control</tt> modifier and on the Windows platform the <tt>Windows</tt> modifier.</p>"
                                          "<p>All six of these controls have three states with the partially checked state being the default meaning <i>this modifier is ignored</i>; otherwise it must be present (checked) or absent (unchecked) for the key-binding to fire.  This is so that separate <i>combinations</i> of modifiers can be used for the <i>same</i> base key.</p>",
                                          // Intentional comment
                                          "This is the tooltip for the modifier that Qt identifies as the meta key, though it is called the control key on the MacOs platform and the Windows key on the Windows platform."));
    checkBox_modifier_alt->setToolTip(tr("<p>On the MacOs platform this will be the <tt>Option</tt> modifier.</p>"
                                         "<p>All six of these controls have three states with the partially checked state being the default meaning <i>this modifier is ignored</i>; otherwise it must be present (checked) or absent (unchecked) for the key-binding to fire.  This is so that separate <i>combinations</i> of modifiers can be used for the <i>same</i> base key.</p>",
                                         // Intentional comment
                                         "This is the tooltip for the modifier that Qt identifies as the alt key, though it is called the option key on the MacOs platform."));
    checkBox_modifier_group->setToolTip(tr("<p>This modifier may only be present on Unix-like platforms where it <i>may</i> be active when the <tt>AltGr</tt> key is used.</p>"
                                           "<p>All six of these controls have three states with the partially checked state being the default meaning <i>this modifier is ignored</i>; otherwise it must be present (checked) or absent (unchecked) for the key-binding to fire.  This is so that separate <i>combinations</i> of modifiers can be used for the <i>same</i> base key.</p>",
                                           // Intentional comment
                                           "This is the tooltip for the modifier that Qt identifies as the group switch modifier, it may only be present on Unix like platforms (not Windows, not MacOs)."));
#endif
}

// Sets the modifiers without causing any signals to be emitted
void dlgKeysMainArea::setModifiers(const QPair<Qt::KeyboardModifiers, Qt::KeyboardModifiers> modifiers)
{
    checkBox_modifier_shift->blockSignals(true);
    if (modifiers.first & Qt::ShiftModifier) {
        checkBox_modifier_shift->setCheckState(Qt::Checked);
    } else if (modifiers.second & Qt::ShiftModifier) {
        checkBox_modifier_shift->setCheckState(Qt::Unchecked);
    } else {
        checkBox_modifier_shift->setCheckState(Qt::PartiallyChecked);
    }
    checkBox_modifier_shift->blockSignals(false);

    checkBox_modifier_control->blockSignals(true);
    if (modifiers.first & Qt::ControlModifier) {
        checkBox_modifier_control->setCheckState(Qt::Checked);
    } else if (modifiers.second & Qt::ControlModifier) {
        checkBox_modifier_control->setCheckState(Qt::Unchecked);
    } else {
        checkBox_modifier_control->setCheckState(Qt::PartiallyChecked);
    }
    checkBox_modifier_control->blockSignals(false);

    checkBox_modifier_alt->blockSignals(true);
    if (modifiers.first & Qt::AltModifier) {
        checkBox_modifier_alt->setCheckState(Qt::Checked);
    } else if (modifiers.second & Qt::AltModifier) {
        checkBox_modifier_alt->setCheckState(Qt::Unchecked);
    } else {
        checkBox_modifier_alt->setCheckState(Qt::PartiallyChecked);
    }
    checkBox_modifier_alt->blockSignals(false);

    checkBox_modifier_meta->blockSignals(true);
    if (modifiers.first & Qt::MetaModifier) {
        checkBox_modifier_meta->setCheckState(Qt::Checked);
    } else if (modifiers.second & Qt::MetaModifier) {
        checkBox_modifier_meta->setCheckState(Qt::Unchecked);
    } else {
        checkBox_modifier_meta->setCheckState(Qt::PartiallyChecked);
    }
    checkBox_modifier_meta->blockSignals(false);

    checkBox_modifier_keypad->blockSignals(true);
    if (modifiers.first & Qt::KeypadModifier) {
        checkBox_modifier_keypad->setCheckState(Qt::Checked);
    } else if (modifiers.second & Qt::KeypadModifier) {
        checkBox_modifier_keypad->setCheckState(Qt::Unchecked);
    } else {
        checkBox_modifier_keypad->setCheckState(Qt::PartiallyChecked);
    }
    checkBox_modifier_keypad->blockSignals(false);

    checkBox_modifier_group->blockSignals(true);
    if (modifiers.first & Qt::GroupSwitchModifier) {
        checkBox_modifier_group->setCheckState(Qt::Checked);
    } else if (modifiers.second & Qt::GroupSwitchModifier) {
        checkBox_modifier_group->setCheckState(Qt::Unchecked);
    } else {
        checkBox_modifier_group->setCheckState(Qt::PartiallyChecked);
    }
    checkBox_modifier_group->blockSignals(false);
}

QPair<Qt::KeyboardModifiers, Qt::KeyboardModifiers>  dlgKeysMainArea::getModifiers() const
{
    // clang-format off
    return qMakePair(( (checkBox_modifier_shift->checkState() == Qt::Checked ? Qt::ShiftModifier : Qt::NoModifier)
                       |(checkBox_modifier_control->checkState() == Qt::Checked ? Qt::ControlModifier : Qt::NoModifier)
                       |(checkBox_modifier_alt->checkState() == Qt::Checked ? Qt::AltModifier : Qt::NoModifier)
                       |(checkBox_modifier_meta->checkState() == Qt::Checked ? Qt::MetaModifier : Qt::NoModifier)
                       |(checkBox_modifier_keypad->checkState() == Qt::Checked  ? Qt::KeypadModifier : Qt::NoModifier)
                       |(checkBox_modifier_group->checkState() == Qt::Checked ? Qt::GroupSwitchModifier : Qt::NoModifier)),
                     ( (checkBox_modifier_shift->checkState() == Qt::Unchecked ? Qt::ShiftModifier : Qt::NoModifier)
                       |(checkBox_modifier_control->checkState() == Qt::Unchecked ? Qt::ControlModifier : Qt::NoModifier)
                       |(checkBox_modifier_alt->checkState() == Qt::Unchecked ? Qt::AltModifier : Qt::NoModifier)
                       |(checkBox_modifier_meta->checkState() == Qt::Unchecked ? Qt::MetaModifier : Qt::NoModifier)
                       |(checkBox_modifier_keypad->checkState() == Qt::Unchecked  ? Qt::KeypadModifier : Qt::NoModifier)
                       |(checkBox_modifier_group->checkState() == Qt::Unchecked ? Qt::GroupSwitchModifier : Qt::NoModifier)));
    // clang-format on
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
