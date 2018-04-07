/*
 *      Copyright (c) 2001 Guido Draheim <guidod@gmx.de>
 *      Use freely under the restrictions of the ZLIB License
 *
 *      You should be able to drop it in the place of a SDL_RWFromFile. Then
 *      go to X/share/myapp and do `cd graphics && zip -9r ../graphics.zip .`
 *      and rename the graphics/ subfolder - and still all your files
 *      are found: a filepath like X/shared/graphics/game/greetings.bmp 
 *      will open X/shared/graphics.zip and return the zipped file 
 *      game/greetings.bmp in the zip-archive (for reading that is).
 *
 */

#ifndef _SDL_RWops_ZZIP_h
#define _SDL_RWops_ZZIP_h

#include <SDL_rwops.h>

#ifndef ZZIP_NO_DECLSPEC
#define ZZIP_DECLSPEC
#else /* use DECLSPEC from SDL/begin_code.h */
#define ZZIP_DECLSPEC DECLSPEC
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern ZZIP_DECLSPEC
SDL_RWops *SDL_RWFromZZIP(const char* file, const char* mode);

#ifdef __cplusplus
} /* extern C */
#endif

#endif
