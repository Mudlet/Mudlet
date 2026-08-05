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

#include "LsanSuppressions.h"

// These hooks are only consulted when the LeakSanitizer runtime is linked in
// (USE_SANITIZER builds, i.e. PTBs and testing builds); elsewhere they are two
// inert functions. They cannot be guarded with an "is ASAN on" macro check:
// Mudlet only applies -fsanitize=address at link time, so no compiler macro is
// set, and Qt's qcompilerdetection.h shims __has_feature to 0 on GCC anyway.

// They live in their own translation unit, built as the mudlet_lsan_hooks
// OBJECT library, because LeakSanitizer looks them up as weak symbols. A weak
// reference does not pull a member out of a static archive, so keeping them in
// mudlet_core would leave every binary that brings its own main() - the Qt Test
// executables, which get theirs from QTEST_MAIN - without any suppressions at
// all. Linking the OBJECT library directly puts the definitions on the link
// line unconditionally, where the weak reference can bind to them.

// Embeds the suppression list into the binary so leak reports shown to users
// by testing/PTB builds exclude third-party noise (GPU drivers, font stack)
// with no LSAN_OPTIONS needed at runtime:
extern "C" const char* __lsan_default_suppressions()
{
    return mudletLsanSuppressions;
}

// Without this, LeakSanitizer appends a "Suppressions used" summary to every
// clean exit, which reads like an error to users:
extern "C" const char* __lsan_default_options()
{
    return "print_suppressions=0";
}
