#ifndef MUDLET_DLGMAPLABEL_H
#define MUDLET_DLGMAPLABEL_H

/***************************************************************************
 *   Copyright (C) 2022 by Piotr Wilczynski - delwing@gmail.com            *
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
#include <QDialog>
#include <QFontDialog>
#include "ui_map_label.h"
#include "post_guard.h"


class dlgMapLabel : public QDialog, public Ui::map_label
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(dlgMapLabel)
    dlgMapLabel(QWidget*);
    ~dlgMapLabel();

    QString getText();
    QColor& getBgColor();
    QColor& getFgColor();
    QFont& getFont();
    bool isOnTop();

signals:
    void updated();

private:
    QFontDialog* fontDialog = nullptr;
    QString text;
    QColor bgColor;
    QColor fgColor;
    QFont font;
    bool onTop;

private slots:
    void save();
    void pickFgColor();
    void pickBgColor();
    void pickFont();
    void updateControls();


};

#endif //MUDLET_DLGMAPLABEL_H
