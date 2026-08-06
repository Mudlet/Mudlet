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

// Inert unless the LeakSanitizer runtime is linked in. They cannot be guarded
// with an "is ASAN on" macro check: Mudlet only applies -fsanitize=address at
// link time, so no compiler macro is set, and Qt's qcompilerdetection.h shims
// __has_feature to 0 on GCC anyway. Built as their own OBJECT library, see
// src/CMakeLists.txt.

// Keeps third-party noise (GPU drivers, font stack) out of the leak reports
// that testing/PTB builds show users, with no LSAN_OPTIONS set at runtime
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
