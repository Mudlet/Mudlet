#ifndef MUDLET_TACCESSIBLELABEL_H
#define MUDLET_TACCESSIBLELABEL_H

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

#include <QAccessibleWidget>

class TLabel;

// A script's label as assistive technology sees it: a button as TLabelModel decides, else a graphic, an
// animation or static text by what it shows, and named as TLabelModel decides.
class TAccessibleLabel : public QAccessibleWidget
{
public:
    explicit TAccessibleLabel(QWidget* widget);

    QAccessible::Role role() const override;
    QString text(QAccessible::Text t) const override;
    QStringList actionNames() const override;
    void doAction(const QString& actionName) override;

    static QAccessibleInterface* labelFactory(const QString& classname, QObject* object);

private:
    TLabel* label() const;
    bool actsAsButton() const;
};

#endif // MUDLET_TACCESSIBLELABEL_H
