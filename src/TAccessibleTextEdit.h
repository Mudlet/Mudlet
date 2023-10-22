#ifndef MUDLET_TACCESSIBLETEXTEDIT_H
#define MUDLET_TACCESSIBLETEXTEDIT_H

/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014-2017 by Ahmed Charles - acharles@outlook.com       *
 *   Copyright (C) 2014-2020, 2022 by Stephen Lyons                        *
 *                                               - slysven@virginmedia.com *
 *   Copyright (C) 2022 by Thiago Jung Bauermann - bauermann@kolabnow.com  *
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

#include "TTextEdit.h"

#include "pre_guard.h"
#include <QAccessibleInterface>
#include <QAccessibleTextInterface>
#include <QAccessibleWidget>
#include "post_guard.h"

class TAccessibleTextEdit : public QAccessibleWidget, public QAccessibleTextInterface
{
public:
    explicit TAccessibleTextEdit(QWidget* widget)
    : QAccessibleWidget(widget, QAccessible::EditableText)
    {
        Q_ASSERT(isValid());
    }

    static QAccessibleInterface* textEditFactory(const QString &classname, QObject *object)
    {
        // The original identifier "interface" was no good as it is an existing
        // macro (on MSYS2/Mingw-w64) and breaks compilation on that platform in
        // an obsecure way:
        QAccessibleInterface* pInterface = nullptr;

        if (classname == QLatin1String("TTextEdit") && object && object->isWidgetType()) {
            pInterface = new TAccessibleTextEdit(static_cast<QWidget *>(object));
        }

        return pInterface;
    }

    void* interface_cast(QAccessible::InterfaceType type) override
    {
        if (type == QAccessible::TextInterface) {
            return static_cast<QAccessibleTextInterface*>(this);
        }

        return QAccessibleWidget::interface_cast(type);
    }

    QAccessible::State state() const override;
    void selection(int selectionIndex, int *startOffset, int *endOffset) const override;
    int selectionCount() const override;
    void addSelection(int startOffset, int endOffset) override;
    void removeSelection(int selectionIndex) override;
    void setSelection(int selectionIndex, int startOffset, int endOffset) override;
    int cursorPosition() const override;
    void setCursorPosition(int position) override;
    QString text(QAccessible::Text text) const override;
    QString text(int startOffset, int endOffset) const override;
    int characterCount() const override;
    QRect characterRect(int offset) const override;
    int offsetAtPoint(const QPoint &point) const override;
    void scrollToSubstring(int startIndex, int endIndex) override;
    QString attributes(int offset, int *startOffset, int *endOffset) const override;
    QString textAfterOffset(int offset, QAccessible::TextBoundaryType boundaryType, int *startOffset, int *endOffset) const override;
    QString textAtOffset(int offset, QAccessible::TextBoundaryType boundaryType, int *startOffset, int *endOffset) const override;
    QString textBeforeOffset(int offset, QAccessible::TextBoundaryType boundaryType, int *startOffset, int *endOffset) const override;

private:
    enum class TextOp {
        BeforeOffset,
        AtOffset,
        AfterOffset,
    };

    TTextEdit* textEdit() const;
    bool offsetIsInvalid(int offset) const;
    int lineForOffset(int offset, int *lengthSoFar) const;
    int columnForOffset(int offset) const;
    int offsetForPosition(int line, int column) const;
    bool lineIsVisible(int line) const;
    QString textAroundOffset(TextOp operation, int offset, QAccessible::TextBoundaryType boundaryType,
                             int* startOffset, int* endOffset) const;
};

#endif // MUDLET_TACCESSIBLETEXTEDIT_H
