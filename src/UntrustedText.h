#ifndef MUDLET_UNTRUSTEDTEXT_H
#define MUDLET_UNTRUSTEDTEXT_H

/***************************************************************************
 *   Copyright (C) 2026 by Mike Conley - mike.conley@stickmud.com          *
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

#include <QString>

class UntrustedText
{
public:
    // True for code points that render as invisible, zero-width, direction
    // reordering, or line breaking text. Such a character lets remote metadata
    // display one target while the link carries another.
    static bool unsafeCharacter(char32_t codePoint);

    // Replaces every unsafe code point with a visible \u{XXXX} escape, leaving
    // all other text - including non-Latin scripts and astral plane code
    // points - untouched.
    static QString forDisplay(const QString& text);
};

#endif // MUDLET_UNTRUSTEDTEXT_H
