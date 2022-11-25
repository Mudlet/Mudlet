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


#include "dlgVideoPlayer.h"

#include "Host.h"
#include "TMedia.h"

#include "pre_guard.h"
#include "ui_video_player.h"
#include "post_guard.h"

dlgVideoPlayer::dlgVideoPlayer(QWidget* parent, Host* pH, TMedia* pM)
: QWidget(parent)
, mpHost(pH)
, mpMedia(pM)
{
    setupUi(this);

    mpMedia->mpVideoPlayer = this;
    widget_panel->setVisible(mpHost->mShowVideoPlayerPanel);
}

void dlgVideoPlayer::slot_togglePanel()
{
    dlgVideoPlayer::slot_setVideoPlayerPanelVisible(!widget_panel->isVisible());
}

void dlgVideoPlayer::slot_setVideoPlayerPanelVisible(bool panelVisible)
{
    widget_panel->setVisible(panelVisible);
    mpHost->mShowVideoPlayerPanel = panelVisible;
}

// Is the video player contained inside a floating/dockable QDockWidget?
bool dlgVideoPlayer::isFloatAndDockable() const
{
    // The class name should be a const char* - no QString wrapper is needed:
    if (parentWidget() && parentWidget()->inherits("QDockWidget")) {
        return true;
    }

    return false;
}
