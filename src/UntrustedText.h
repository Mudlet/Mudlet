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

// Escapes characters a remote game server could use to make displayed text
// misrepresent itself. Two policies, because the two kinds of text have
// different needs:
//
//   forTarget()       - a command or URL the user reads to decide whether to
//                       trust a link. Nothing invisible may survive here.
//   forAuthoredText() - a tooltip, menu label or menu title the server author
//                       wrote to be read. Same policy, except it keeps the
//                       joiners and tag characters that multi-part emoji and
//                       Persian, Arabic and Indic text are built from.
class UntrustedText
{
public:
    // True for the enumerated set of code points that render as invisible,
    // zero-width, direction reordering, or line breaking text - not a general
    // test for those properties. The set is deliberately narrow; widen it in
    // the implementation rather than assuming coverage.
    static bool unsafeCharacter(char32_t codePoint);

    // As unsafeCharacter(), minus the zero-width joiner and non-joiner and the
    // assigned tag characters U+E0020 to U+E007F. Those are load-bearing in
    // text meant to be read: ZWJ builds 👨‍🍳 and 🏳️‍🌈, the tag characters build
    // subdivision flags like 🏴󠁧󠁢󠁳󠁣󠁴󠁿, and ZWNJ is required for correct Persian
    // and Indic shaping.
    static bool unsafeAuthoredCharacter(char32_t codePoint);

    // Replace every unsafe code point with a visible \u{...} escape carrying
    // its hex value, leaving all other text - including non-Latin scripts and
    // astral plane code points - untouched.
    static QString forTarget(const QString& text);
    static QString forAuthoredText(const QString& text);

private:
    static QString escapeWith(const QString& text, bool (*unsafe)(char32_t));
};

#endif // MUDLET_UNTRUSTEDTEXT_H
