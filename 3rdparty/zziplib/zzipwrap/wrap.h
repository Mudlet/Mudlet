/*
 * Author: 
 *	Andreas Schiffler <aschiffler@appwares.com>
 *
 *	Copyright (c) 2001 Andreas Schiffler
 * 	    All rights reserved, 
 *          usage allowed under the restrictions of the
 *	    Lesser GNU General Public License 
 */

#ifndef _ZZIPWRAP_H
#define _ZZIPWRAP_H

#include <zzip/zzip.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * toggle alternate read routine and set callback and callback data
 * return: a plugin_io pointer to be used 
 *  with => zzip_opendir_ext_io or => zzip_open_ext_io and cousins
 */
typedef void (*zzipwrap_pfn_t)(void* mem, int blocksize, void* privatedata);
_zzip_export
zzip_plugin_io_t
zzipwrap_use_memory_io(int blocksize, zzipwrap_pfn_t callback, void *callbackdata);


#ifdef __cplusplus
};
#endif

#endif /* _ZZIPWRAP_H */

