/***************************************************************************
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

// MSVC's leak detection allowed for the definition of macros for the
// functions/keywords below. This is fine for Mudlet's code, since it does
// not use placement new, etc. The macros add file/line information to the
// debug information shown by MSVC.
// These pragmas save the value of the macros and then undefine the macros
// so that when external headers, like Qt's or Boost's are included, they
// are not broken by the unexpected macro definitions.

#if defined(_DEBUG) && defined(_MSC_VER)
#pragma push_macro("new")
#undef new
#pragma push_macro("malloc")
#undef malloc
#pragma push_macro("realloc")
#undef realloc
#pragma push_macro("free")
#undef free
#endif // _DEBUG && _MSC_VER
