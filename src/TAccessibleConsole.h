#ifndef MUDLET_TACCESSIBLECONSOLE_H
#define MUDLET_TACCESSIBLECONSOLE_H

/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014-2017 by Ahmed Charles - acharles@outlook.com       *
 *   Copyright (C) 2014-2020 by Stephen Lyons - slysven@virginmedia.com    *
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

#include "TConsole.h"
#include "TTextEdit.h"

#include "pre_guard.h"
#include <QAccessible>
#include <QAccessibleActionInterface>
#include <QAccessibleInterface>
#include <QAccessibleObject>
#include <QAccessibleTextInterface>
#include <QAccessibleWidget>
#include "post_guard.h"

class TAccessibleConsole : public QAccessibleWidget, public QAccessibleTextInterface
{
public:
    explicit TAccessibleConsole(QWidget* w) : QAccessibleWidget(w, QAccessible::EditableText)
    {
        Q_ASSERT(isValid());

        qDebug() << QStringLiteral("TAccessibleConsole instantiated");
    }

    static QAccessibleInterface* consoleFactory(const QString &classname, QObject *object)
    {
        QAccessibleInterface *interface = 0;

        if (classname == QLatin1String("TConsole") && object && object->isWidgetType()) {
            qDebug() << "TConsole::consoleFactory function called with classname: TConsole";

            interface = new TAccessibleConsole(static_cast<QWidget *>(object));
        }

        return interface;
    }

    void* interface_cast(QAccessible::InterfaceType t) override
    {
        qDebug() << QStringLiteral("TAccessibleConsole::interface_cast");

        if (t == QAccessible::ActionInterface) {
            return static_cast<QAccessibleActionInterface*>(this);
        }

        if (t == QAccessible::TextInterface) {
            return static_cast<QAccessibleTextInterface*>(this);
        }

        return QAccessibleWidget::interface_cast(t);
    }

    TConsole* display() const;
    QAccessibleInterface* childAt(int x, int y) const;
    int childCount() const;
    int indexOfChild(const QAccessibleInterface *child) const;
    QAccessible::Role role() const;
    QAccessible::State state() const;
    int lineForOffset(int offset) const;
    int columnForOffset(int offset) const;
    void selection(int selectionIndex, int *startOffset, int *endOffset) const override;
    int selectionCount() const override;
    void addSelection(int startOffset, int endOffset) override;
    void removeSelection(int selectionIndex) override;
    void setSelection(int selectionIndex, int startOffset, int endOffset) override;
    int cursorPosition() const override;
    void setCursorPosition(int position) override;
    QString text(int startOffset, int endOffset) const override;
    int characterCount() const override;
    QRect characterRect(int offset) const override;
    int offsetAtPoint(const QPoint &point) const override;
    void scrollToSubstring(int startIndex, int endIndex) override;
    QString attributes(int offset, int *startOffset, int *endOffset) const override;
    QString textAfterOffset(int offset, QAccessible::TextBoundaryType boundaryType, int *startOffset, int *endOffset) const override;
    QString textAtOffset(int offset, QAccessible::TextBoundaryType boundaryType, int *startOffset, int *endOffset) const override;
    QString textBeforeOffset(int offset, QAccessible::TextBoundaryType boundaryType, int *startOffset, int *endOffset) const override;
};

#endif // MUDLET_TACCESSIBLECONSOLE_H