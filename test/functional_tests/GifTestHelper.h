#ifndef MUDLET_GIFTESTHELPER_H
#define MUDLET_GIFTESTHELPER_H

/***************************************************************************
 *   Copyright (C) 2026 by Vadim Peretokin - vadim.peretokin@mudlet.org    *
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

#include <QByteArray>

// Qt ships no GIF writer, so a test that needs a movie on disk has to spell one
// out. Three frames rather than one, so the frame count is a distinctive thing
// to compare and QMovie::state() is meaningfully Running, and a 60 second frame
// delay so the animation never advances between two reads.
inline QByteArray threeFrameGif()
{
    QByteArray gif("GIF89a");
    gif.append(QByteArray::fromHex("01000100910000"));
    gif.append(QByteArray::fromHex("ff000000ff000000ff000000"));
    const QByteArray frame = QByteArray::fromHex("21f90400701700002c00000000010001000002024c0100");
    gif.append(frame).append(frame).append(frame);
    gif.append(QByteArray::fromHex("3b"));
    return gif;
}

#endif // MUDLET_GIFTESTHELPER_H
