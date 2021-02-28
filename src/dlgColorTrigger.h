#ifndef MUDLET_DLGCOLORTRIGGER_H
#define MUDLET_DLGCOLORTRIGGER_H

/***************************************************************************
 *   Copyright (C) 2008-2009 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2018-2019 by Stephen Lyons - slysven@virginmedia.com    *
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
#include "ui_color_trigger.h"
#include <QSignalMapper>
#include <QPointer>
#include "post_guard.h"

class Host;
class TTrigger;


class dlgColorTrigger : public QDialog, public Ui::color_trigger
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(dlgColorTrigger)
    dlgColorTrigger(QWidget*, TTrigger*, const bool isBackground, const QString &title = QString());

public slots:
    void slot_basicColorClicked(int);
    void slot_resetColorClicked();
    void slot_defaultColorClicked();
    void slot_moreColorsClicked();
    void slot_rgbColorChanged();
    void slot_setRBGButtonFocus();
    void slot_grayColorChanged(int);
    void slot_setGreyButtonFocus();
    void slot_rgbColorClicked();
    void slot_grayColorClicked();


private:
    void setupBasicButton(QPushButton*, const int, const QColor&, const QString&);

    QSignalMapper* mSignalMapper;
    TTrigger* mpTrigger;
    bool mIsBackground;
    QColor mRgbAnsiColor;
    QColor mGrayAnsiColor;
    int mRgbAnsiColorNumber;
    int mGrayAnsiColorNumber;
};

#endif // MUDLET_DLGCOLORTRIGGER_H
