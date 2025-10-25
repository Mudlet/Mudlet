#ifndef TMEDIAPLAYLIST_H
#define TMEDIAPLAYLIST_H

/***************************************************************************
 *   Copyright (C) 2024 by Mike Conley - mike.conley@stickmud.com          *
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

#include <QList>
#include <QUrl>

class TMediaPlaylist
{
public:
    enum PlaybackMode {
        Sequential,
        Loop,
        Random
    };

    TMediaPlaylist();
    ~TMediaPlaylist();

    void addMedia(const QUrl &url);
    void removeMedia(int startIndex, int endIndex);
    void clear();
    int mediaCount() const;
    bool isEmpty() const;
    void setPlaybackMode(PlaybackMode mode);
    PlaybackMode playbackMode() const;

    QUrl currentMedia() const;
    bool setCurrentIndex(int index);
    int currentIndex() const;
    int nextIndex() const;
    QUrl next();
    QUrl previous();

private:
    QList<QUrl> mMediaList;
    int mCurrentIndex;
    PlaybackMode mPlaybackMode;
};

#endif // TMEDIAPLAYLIST_H
