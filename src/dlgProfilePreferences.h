#ifndef MUDLET_DLGPROFILEPREFERENCES_H
#define MUDLET_DLGPROFILEPREFERENCES_H

/***************************************************************************
 *   Copyright (C) 2008-2012 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
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
#include "ui_profile_preferences.h"
#include <QDialog>
#include <QtCore>
#include <QDir>
#include "post_guard.h"

class Host;


class dlgProfilePreferences : public QDialog, public Ui::profile_preferences
{
    Q_OBJECT

public:
    dlgProfilePreferences( QWidget *, Host * );

    int mFontSize;

signals:
    // Used by "Copy Map" to inform a list of profiles - asynchronously, via
    // mudlet class - to load in an updated map
    void    signal_otherProfilesToReloadMaps( QList<QString> );


public slots:
    void setFontSize();
    void setColorBlack();
    void setColorLightBlack();
    void setColorRed();
    void setColorLightRed();
    void setColorBlue();
    void setColorLightBlue();
    void setColorGreen();
    void setColorLightGreen();
    void setColorYellow();
    void setColorLightYellow();
    void setColorCyan();
    void setColorLightCyan();
    void setColorMagenta();
    void setColorLightMagenta();
    void setColorWhite();
    void setColorLightWhite();
    void setDisplayFont();
    void setCommandLineFont();
    void setFgColor();
    void setBgColor();
    void setCommandLineBgColor();
    void setCommandLineFgColor();

    void setCommandFgColor();
    void setCommandBgColor();
    void setColorBlack2();
    void setColorLightBlack2();
    void setColorRed2();
    void setColorLightRed2();
    void setColorBlue2();
    void setColorLightBlue2();
    void setColorGreen2();
    void setColorLightGreen2();
    void setColorYellow2();
    void setColorLightYellow2();
    void setColorCyan2();
    void setColorLightCyan2();
    void setColorMagenta2();
    void setColorLightMagenta2();
    void setColorWhite2();
    void setColorLightWhite2();
    void setFgColor2();
    void setBgColor2();
    void resetColors();
    void downloadMap();
    void loadMap();
    void saveMap();
    void copyMap();
    void hideActionLabel();
    void slot_save_and_exit();
    void slot_chooseProfilesChanged(QAction *);

private slots:
    void slot_timeValueChanged(QTime);

private:
    void setColors();
    Host * mpHost;
};

#endif // MUDLET_DLGPROFILEPREFERENCES_H
