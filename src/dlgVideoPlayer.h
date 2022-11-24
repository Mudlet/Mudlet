#ifndef MUDLET_DLGVIDEOPLAYER_H
#define MUDLET_DLGVIDEOPLAYER_H

/***************************************************************************
 *   Copyright (C) 2022 by Mike Conley - mike.conley@stickmud.com          *
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
#include <QPointer>
#include <QWidget>
#include "ui_video_player.h"
#include "post_guard.h"

class Host;
class TMedia;

class dlgVideoPlayer : public QWidget, public Ui::video_player
{
    Q_OBJECT

public:
    Q_DISABLE_COPY(dlgVideoPlayer)
    dlgVideoPlayer(QWidget*, Host*, TMedia*);

    bool isFloatAndDockable() const;

public slots:
    void slot_togglePanel();
    void slot_setVideoPlayerPanelVisible(bool);

private:
    TMedia* mpMedia = nullptr;
    QPointer<Host> mpHost;
};

#endif // MUDLET_DLGVIDEOPLAYER_H
