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

#include "TMediaPlaylist.h"

#include <QRandomGenerator>

TMediaPlaylist::TMediaPlaylist()
: mCurrentIndex(0)
, mPlaybackMode(Sequential)
{}

TMediaPlaylist::~TMediaPlaylist()
{}

void TMediaPlaylist::addMedia(const QUrl &url)
{
    mMediaList.append(url);
}

void TMediaPlaylist::removeMedia(int startIndex, int endIndex)
{
    if (endIndex >= startIndex && startIndex >= 0 && endIndex < mMediaList.size()) {
        for (int i = endIndex; i >= startIndex; --i) {
            mMediaList.removeAt(i);
        }
    }
}

void TMediaPlaylist::clear()
{
    mMediaList.clear();
}

int TMediaPlaylist::mediaCount() const
{
    return mMediaList.count();
}

bool TMediaPlaylist::isEmpty() const {
    return mMediaList.isEmpty();
}

void TMediaPlaylist::setPlaybackMode(PlaybackMode mode)
{
    mPlaybackMode = mode;
}

TMediaPlaylist::PlaybackMode TMediaPlaylist::playbackMode() const
{
    return mPlaybackMode;
}

QUrl TMediaPlaylist::currentMedia() const
{
    if (!mMediaList.isEmpty() && mCurrentIndex >= 0 && mCurrentIndex < mMediaList.size()) {
        return mMediaList.at(mCurrentIndex);
    }
    return QUrl();
}

bool TMediaPlaylist::setCurrentIndex(int index)
{
    if (index >= 0 && index < mMediaList.size()) {
        mCurrentIndex = index;
        return true;
    }
    return false;
}

int TMediaPlaylist::currentIndex() const
{
    return mCurrentIndex;
}

int TMediaPlaylist::nextIndex() const
{
    if (mPlaybackMode == Loop && (mCurrentIndex + 1 == mMediaList.size())) {
        return 0;
    } else if (mPlaybackMode == Random) {
        return QRandomGenerator::global()->bounded(mMediaList.size());
    } else if (mCurrentIndex + 1 < mMediaList.size()) {
        return mCurrentIndex + 1;
    }
    return -1; // No valid next index if out of range
}

QUrl TMediaPlaylist::next()
{
    if (mMediaList.isEmpty()) {
        return QUrl();
    }

    if (mPlaybackMode == Loop && (mCurrentIndex + 1 == mMediaList.size())) {
        mCurrentIndex = 0;
    } else if (mPlaybackMode == Random) {
        mCurrentIndex = QRandomGenerator::global()->bounded(mMediaList.size());
    } else {
        mCurrentIndex++;
    }

    if (mCurrentIndex < mMediaList.size()) {
        return mMediaList.at(mCurrentIndex);
    }

    return QUrl();
}

QUrl TMediaPlaylist::previous()
{
    if (mPlaybackMode == Loop && mCurrentIndex == 0) {
        mCurrentIndex = mMediaList.size() - 1;
    } else {
        mCurrentIndex--;
    }

    if (mCurrentIndex >= 0) {
        return mMediaList.at(mCurrentIndex);
    }

    return QUrl();
}
