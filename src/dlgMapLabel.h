#ifndef MUDLET_DLGMAPLABEL_H
#define MUDLET_DLGMAPLABEL_H

/***************************************************************************
 *   Copyright (C) 2022 by Piotr Wilczynski - delwing@gmail.com            *
 *   Copyright (C) 2022 by Stephen Lyons - slysven@virginmedia.com         *
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

#include "ui_map_label.h"
#include <QDialog>

class QColorDialog;
class QFontDialog;

class dlgMapLabel : public QDialog, public Ui::map_label
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(dlgMapLabel)
    explicit dlgMapLabel(QWidget*);

    bool isTextLabel();
    QString getImagePath();
    bool stretchImage();
    QString getText();
    QColor& getBgColor();
    QColor& getFgColor();
    QColor& getOutlineColor();
    QFont& getFont();
    bool isOnTop();
    bool noScale();

signals:
    void updated();

private:
    QFontDialog* fontDialog = nullptr;
    QColorDialog* bgColorDialog = nullptr;
    QColorDialog* fgColorDialog = nullptr;
    QColorDialog* outlineColorDialog = nullptr;
    QString imagePath;
    QString text;
    QColor fgColor = QColor(255, 255, 50, 255);
    QColor bgColor = QColor(50, 50, 150, 100);
    QColor outlineColor = QColor(255, 255, 50, 255);
    QFont font;

private slots:
    void slot_save();
    void slot_pickFgColor();
    void slot_pickBgColor();
    void slot_pickOutlineColor();
    void slot_pickFont();
    void slot_pickFile();
    void slot_updateControls();
    void slot_updateControlsVisibility();
};

#endif //MUDLET_DLGMAPLABEL_H
