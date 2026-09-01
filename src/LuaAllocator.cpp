/***************************************************************************
 *   Copyright (C) 2026 by Vadim Peretokin - vperetokin@hey.com            *
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

#include "LuaAllocator.h"

#include <QCoreApplication>
#include <QThread>

#include <array>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>

extern "C" {
#if defined(INCLUDE_VERSIONED_LUA_HEADERS)
#include <lua5.1/lua.h>
#else
#include <lua.h>
#endif
}

// Lua reaches its allocator millions of times over a busy session - a trigger
// firing hands captures over as fresh tables and strings - and every one of
// those was a separate glibc malloc()/free() pair. Lua hands the true old size
// back on every free and every resize, so blocks need no header of their own
// and a size class can be recovered from that size alone; that is what lets a
// bounded free list per size serve most of that traffic without reaching
// malloc() at all.
//
// Define MUDLET_LUA_ALLOC_PASSTHROUGH to route every request straight at
// realloc()/free(), which is how a suspected allocator fault gets bisected.
//
// A sanitised build defines it for itself. A block held on a free list is
// still allocated as far as the system is concerned, so a write to a freed Lua
// object, or a leaked one, would look like ordinary use of a live allocation -
// a preset built to find those bugs would stop finding them in Lua.
#if defined(__SANITIZE_ADDRESS__) || defined(__SANITIZE_THREAD__) || defined(__SANITIZE_MEMORY__)
#define MUDLET_LUA_ALLOC_PASSTHROUGH
#elif defined(__has_feature)
#if __has_feature(address_sanitizer) || __has_feature(thread_sanitizer) || __has_feature(memory_sanitizer)
#define MUDLET_LUA_ALLOC_PASSTHROUGH
#endif
#endif

// LuaJIT hands its allocator a size class tag in osize rather than the true old
// size, and its own allocator cannot be replaced on 64-bit at all, so the whole
// design below rests on this being PUC-Rio Lua 5.1.
#if !defined(MUDLET_LUA_ALLOC_PASSTHROUGH) && defined(LUAJIT_VERSION)
#error "LuaAllocator assumes PUC-Rio Lua 5.1: LuaJIT reports a type tag in osize, so blocks would be freed to the wrong size class"
#endif

#if !defined(MUDLET_LUA_ALLOC_PASSTHROUGH)
namespace {
// Blocks are handed out in multiples of this, which also keeps every one of
// them as aligned as malloc() would have made it.
constexpr size_t cGranularity = 16;
// Requests above this go to malloc() unchanged. Lua's own traffic is
// overwhelmingly small - table nodes, TValue arrays, interned strings - and a
// heaptrack census of the flood benchmark puts over 99% of it under this cap.
constexpr size_t cLargestClass = 512;
constexpr size_t cClassCount = cLargestClass / cGranularity;
// A free list exists to hand a block straight back to the next request of its
// size, so only as many are worth holding as a busy moment actually reuses.
// Past this the block is returned to malloc() instead, which is what bounds
// the whole thing: a session whose allocation sizes shift - a phase of long
// strings after a phase of small tables - would otherwise leave every phase's
// blocks parked on their own class for good. The cap holds the total under
// about two megabytes however the sizes move.
constexpr size_t cMaxPooledPerClass = 256;

// Lua is single threaded here: every lua_State Mudlet creates belongs to a
// Host, and Host code, timers, triggers and the Lua API all run on the main
// thread - the only work Mudlet moves off it (zipping packages, writing module
// XML, saving map images, the updater) touches no lua_State. That is why these
// are unguarded globals. Should a lua_State ever be driven from another thread
// this allocator has to gain a lock first.
//
// Index 0 is never handed out: sizeClass() only reaches it for a zero size,
// which no allocation can ask for. Keeping the slot instead of biasing the
// index by one means a free arriving with a bogus zero size drops the block on
// an unused list rather than running off the front of the array.
//
// Zero initialised and with no destructor on purpose: states are closed at
// shutdown in whatever order Qt tears Mudlet down, and this has to keep working
// through all of it.
std::array<void*, cClassCount + 1> sFreeLists;
std::array<size_t, cClassCount + 1> sPooledCounts;

inline size_t sizeClass(const size_t size)
{
    return (size + cGranularity - 1) / cGranularity;
}

// Sizes are compared before sizeClass() rather than after, so the rounding it
// does can never wrap a huge request round to a small class.
inline void* allocate(const size_t nsize)
{
    if (nsize > cLargestClass) {
        return std::malloc(nsize);
    }
    const size_t index = sizeClass(nsize);
    if (void* block = sFreeLists[index]) {
        sFreeLists[index] = *reinterpret_cast<void**>(block);
        --sPooledCounts[index];
        return block;
    }
    // Rounded up to the class, so the block that comes back can serve every
    // later request of that class and be recognised by its size alone.
    return std::malloc(index * cGranularity);
}

inline void release(void* ptr, const size_t osize)
{
    if (osize > cLargestClass) {
        std::free(ptr);
        return;
    }
    const size_t index = sizeClass(osize);
    if (sPooledCounts[index] >= cMaxPooledPerClass) {
        std::free(ptr);
        return;
    }
    *reinterpret_cast<void**>(ptr) = sFreeLists[index];
    sFreeLists[index] = ptr;
    ++sPooledCounts[index];
}

} // namespace
#endif // !MUDLET_LUA_ALLOC_PASSTHROUGH

// C language linkage: these are handed to Lua as C function pointers.
extern "C" {
// What luaL_newstate() installs, kept so that an unprotected error still says
// so on stderr rather than exiting silently.
static int luaPanic(lua_State* L)
{
    std::fprintf(stderr, "PANIC: unprotected error in call to Lua API (%s)\n", lua_tostring(L, -1));
    return 0;
}

static void* luaAlloc(void*, void* ptr, [[maybe_unused]] const size_t osize, const size_t nsize)
{
#if defined(MUDLET_LUA_ALLOC_PASSTHROUGH)
    if (nsize == 0) {
        std::free(ptr);
        return nullptr;
    }
    return std::realloc(ptr, nsize);
#else
    if (nsize == 0) {
        if (ptr) {
            release(ptr, osize);
        }
        // Lua reads a null return here as confirmation, not as failure.
        return nullptr;
    }
    if (!ptr) {
        // osize is not a size on this path - Lua passes 0 in 5.1 and a type tag
        // in later versions - so nothing may be derived from it.
        return allocate(nsize);
    }
    const bool wasSmall = osize <= cLargestClass;
    const bool isSmall = nsize <= cLargestClass;
    if (!wasSmall && !isSmall) {
        return std::realloc(ptr, nsize);
    }
    if (wasSmall && isSmall && sizeClass(osize) == sizeClass(nsize)) {
        // A resize inside one class already has the room it is asking for.
        return ptr;
    }
    void* moved = allocate(nsize);
    if (!moved) {
        return nullptr;
    }
    std::memcpy(moved, ptr, osize < nsize ? osize : nsize);
    release(ptr, osize);
    return moved;
#endif
}
} // extern "C"

lua_State* mudletNewLuaState()
{
    // Unguarded globals above: this has to stay the main thread's alone.
    Q_ASSERT(!qApp || QThread::currentThread() == qApp->thread());
    lua_State* L = lua_newstate(&luaAlloc, nullptr);
    if (L) {
        lua_atpanic(L, &luaPanic);
    }
    return L;
}
