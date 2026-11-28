/***************************************************************************
 *   Copyright (C) 2026 by Michael Conley - sousesider@gmail.com           *
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

#include "TAccessibleLabel.h"

#include "TLabel.h"

TAccessibleLabel::TAccessibleLabel(QWidget* widget)
: QAccessibleWidget(widget, QAccessible::StaticText)
{
}

TLabel* TAccessibleLabel::label() const
{
    return static_cast<TLabel*>(widget());
}

bool TAccessibleLabel::actsAsButton() const
{
    return label()->model().actsAsButton();
}

const TLabelAccessibleValue* TAccessibleLabel::accessibleValue() const
{
    const std::optional<TLabelAccessibleValue>& value = label()->model().mAccessibleValue;
    return value && !actsAsButton() ? &*value : nullptr;
}

void* TAccessibleLabel::interface_cast(QAccessible::InterfaceType t)
{
    if (t == QAccessible::ValueInterface && accessibleValue()) {
        return static_cast<QAccessibleValueInterface*>(this);
    }
    return QAccessibleWidget::interface_cast(t);
}

QAccessible::Role TAccessibleLabel::role() const
{
    if (actsAsButton()) {
        return QAccessible::Button;
    }
    if (accessibleValue()) {
        return QAccessible::ProgressBar;
    }
    // As Qt's own QLabel interface reports a pixmap or movie; Mudlet's SVG layer counts as a pixmap
    if (label()->movie()) {
        return QAccessible::Animation;
    }
    return label()->showsImage() ? QAccessible::Graphic : QAccessible::StaticText;
}

QString TAccessibleLabel::text(QAccessible::Text t) const
{
    if (t == QAccessible::Name) {
        // QLabel::setText() sends NameChanged before the model is given the new text, so read what the label shows
        return label()->model().effectiveAccessibleName(label()->text());
    }
    // A tooltip serving as the name would otherwise be read a second time as the description
    if (t == QAccessible::Description && label()->model().namedByToolTip()) {
        return {};
    }
    return QAccessibleWidget::text(t);
}

QStringList TAccessibleLabel::actionNames() const
{
    return actsAsButton() ? QStringList{pressAction()} : QAccessibleWidget::actionNames();
}

void TAccessibleLabel::doAction(const QString& actionName)
{
    if (actionName == pressAction() && actsAsButton()) {
        label()->press();
        return;
    }
    QAccessibleWidget::doAction(actionName);
}

QVariant TAccessibleLabel::currentValue() const
{
    const TLabelAccessibleValue* value = accessibleValue();
    return value ? QVariant(value->value) : QVariant();
}

QVariant TAccessibleLabel::maximumValue() const
{
    const TLabelAccessibleValue* value = accessibleValue();
    return value ? QVariant(value->max) : QVariant();
}

QVariant TAccessibleLabel::minimumValue() const
{
    const TLabelAccessibleValue* value = accessibleValue();
    return value ? QVariant(value->min) : QVariant();
}

QVariant TAccessibleLabel::minimumStepSize() const
{
    // Any value between the two ends is one, as for Qt's own progress bar
    return accessibleValue() ? QVariant(0.0) : QVariant();
}

/*static*/ QAccessibleInterface* TAccessibleLabel::labelFactory(const QString& classname, QObject* object)
{
    // pInterface, not interface: the latter is a macro on MinGW
    QAccessibleInterface* pInterface = nullptr;
    if (classname == QLatin1String("TLabel") && object && object->isWidgetType()) {
        pInterface = new TAccessibleLabel(static_cast<QWidget*>(object));
    }
    return pInterface;
}
