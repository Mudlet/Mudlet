
/* Wrapper interface for zziplib's file IO functions. 
 *
 * -- In memory-access IO function --
 *  Provides default fileio functions open/read/lseek/close and functions 
 *  that enable blocked, memory buffered IO. 
 *
 *  (c) A. Schiffler, aschiffler@home.com                         
 *  Released under same license as zziplib (LGPL), Oct 2001 
 *
 *  modified 2002 to use new plugin_io interface by Guido Draheim
 */

#include <zzip/conf.h>
#include <zzip/plugin.h>
/* #incl <zzip/wrap.h> */
#include      "wrap.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef ZZIP_HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/* Some globals that we use for operation */

static int zzip_memory_blocksize=0;
static zzipwrap_pfn_t zzip_memory_callback=NULL;
static void *zzip_memory_callbackdata=NULL;
static char *zzip_memory_buffer=NULL;
static int zzip_memory_bufferblock=0;
static int zzip_memory_fd=-1;
static int zzip_memory_pos=0;
static int zzip_memory_size=0;

/* at the moment, we do not export the five handlers of zzip_memry plugin_io */
#define ZZIP_memory static

ZZIP_memory void zzip_memory_reset(void)
{
 zzip_memory_buffer=NULL;
 zzip_memory_bufferblock=0;
 zzip_memory_pos=0;
 zzip_memory_size=0;
 zzip_memory_fd=-1;
}

ZZIP_memory int zzip_memory_read(int fd, char *buf, size_t count)
{
 int requested_block;
 int transfer_start;
 int transfer_bytes;
 int bytes_read;
 int bytes_to_write;
 int bytes_written;
 
 /* Check file descriptor */
 if (fd!=zzip_memory_fd) {
  return(-1);
 }

 if ((int)count<0) 
     return -1;

 /* Work out how many bytes we have to read based */
 /*  on filepos and requested buffersize. */
 bytes_to_write=zzip_memory_size-zzip_memory_pos;
 if (bytes_to_write>=(int)count) {
  bytes_to_write=(int)count;
 }

 /* Read/transfer loop */
 bytes_written=0;
 while (bytes_to_write>0) {
  /* Check which block we are reading from */
  requested_block=zzip_memory_pos/zzip_memory_blocksize;
  /* Check if we have this block available */
  if (zzip_memory_bufferblock!=requested_block) {
   /* Read this block */
   lseek(fd,requested_block*zzip_memory_blocksize,SEEK_SET);
   bytes_read=read(fd,zzip_memory_buffer,zzip_memory_blocksize);
   /* Clear unused bytes */
   if ((bytes_read>=0) && (bytes_read<zzip_memory_blocksize)) {
    memset(&zzip_memory_buffer[bytes_read],0,zzip_memory_blocksize-bytes_read);
   }
   /* Set current block pos */
   zzip_memory_bufferblock=requested_block;
   /* Process buffer if callback is set */
   if (zzip_memory_callback) {
    zzip_memory_callback(zzip_memory_buffer,zzip_memory_blocksize,zzip_memory_callbackdata);
   }
  } else {
   bytes_read=zzip_memory_blocksize;
  }
  /* Check if we have read any data */ 
  if (bytes_read > 0) {
   /* Copy data to output buffer */
   transfer_start=zzip_memory_pos-zzip_memory_bufferblock*zzip_memory_blocksize;
   transfer_bytes=zzip_memory_blocksize-transfer_start;
   if (transfer_bytes>bytes_to_write) {
    transfer_bytes=bytes_to_write;
   }
   memcpy (&buf[bytes_written], &zzip_memory_buffer[transfer_start],transfer_bytes);
   zzip_memory_pos += transfer_bytes;
   bytes_written += transfer_bytes;
   bytes_to_write -= transfer_bytes;
  } else {
   /* Exit on error during read */
   bytes_to_write=0;
  }
 }
 return(bytes_written);
}

ZZIP_memory int zzip_memory_open(const char *pathname, int flags)
{
 int fd;
 struct stat stat_buffer;
 
 /* Deallocate any memory buffer laying around */
 if (zzip_memory_buffer) {
  free(zzip_memory_buffer);
 }
 /* Reset variables */
 zzip_memory_reset();
 /* Open file */
 fd=open(pathname, flags);
 if (fd<0) { 
  return(-1);
 } else { 
  /* Get filesize */
  if (fstat(fd, &stat_buffer)<0) {
   return(-1);
  } 
  if (stat_buffer.st_size<1) {
   return(-1);
  }
  /* Store filesize */
  zzip_memory_size = stat_buffer.st_size;
  /* Allocate readbuffer */
  zzip_memory_buffer = malloc(zzip_memory_blocksize);
  /* Invalidate buffer block */
  zzip_memory_bufferblock = -1;
  /* Store file descriptor */
  zzip_memory_fd=fd;
  /* Return file descriptor */
  return(fd);
 } 
}

ZZIP_memory int zzip_memory_close(int fd)
{
 /* Check file descriptor */
 if (fd!=zzip_memory_fd) {
  return(-1);
 }
 /* Clear any memory buffer that might be laying around */
 if (zzip_memory_buffer) {
  free(zzip_memory_buffer);
 }
 /* Reset variables */
 zzip_memory_reset();
 /* Close file */
 return(close(fd));
}

ZZIP_memory off_t zzip_memory_lseek(int fildes, off_t offset, int whence)
{
 /* Check file descriptor */
 if (fildes!=zzip_memory_fd) {
  return(-1);
 }
 /* Change position pointer */
 switch (whence) {
  case SEEK_SET:
   zzip_memory_pos=offset;
   break;
  case SEEK_CUR:
   zzip_memory_pos += offset;
   break;
  case SEEK_END:
   zzip_memory_pos = zzip_memory_size-1;
   break;
 }
 /* Limit position to the last byte of the file */
 if (zzip_memory_pos>(zzip_memory_size-1)) {
  zzip_memory_pos=zzip_memory_size-1;
 }
 /* Return current position */ 
 return(zzip_memory_pos);
}

/* -------- Control wrapper usage */


zzip_plugin_io_t
zzipwrap_use_memory_io(int blocksize, 
		       zzipwrap_pfn_t callback, void *callbackdata)
{
 static const struct zzip_plugin_io zzip_memory_io =
 {
  (void *) zzip_memory_open,
  (void *) zzip_memory_close,
  (void *) zzip_memory_read,
  (void *) zzip_memory_lseek,
  (void *) zzip_filesize,
  0
 };

 /* Store blocksize and block-processing callback data */

 zzip_memory_blocksize = blocksize;
 zzip_memory_callback = callback;
 zzip_memory_callbackdata = callbackdata;

 return (zzip_plugin_io_t) &zzip_memory_io;
}

