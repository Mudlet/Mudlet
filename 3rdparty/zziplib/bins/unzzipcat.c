/*
 *	Copyright (c) 2003 Guido Draheim <guidod@gmx.de>
 *      Use freely under the restrictions of the ZLIB license.
 *
 *      This file is used as an example to clarify zzipmmap api usage.
 */

#include <zzip/mmapped.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef ZZIP_HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef ZZIP_HAVE_IO_H
#include <io.h>
#endif

#ifdef ZZIP_HAVE_FNMATCH_H
#include <fnmatch.h>
#else
#define fnmatch(x,y,z) strcmp(x,y)
#endif

#ifndef O_BINARY
#define O_BINARY 0
#endif

static const char usage[] = 
{
    "unzzipdir <zip> [names].. \n"
    "  - unzzip data content of files contained in a zip archive.\n"
};

static void zzip_disk_entry_fprint(ZZIP_DISK* disk, 
				   ZZIP_DISK_ENTRY* entry, FILE* out)
{
    ZZIP_DISK_FILE* file = zzip_disk_entry_fopen (disk, entry);
    if (file) 
    {
	char buffer[1024]; int len;
	while ((len = zzip_disk_fread (buffer, 1024, 1, file)))
	    fwrite (buffer, len, 1, out);
	
	zzip_disk_fclose (file);
    }
}

static void zzip_disk_cat_file(ZZIP_DISK* disk, char* name, FILE* out)
{
    ZZIP_DISK_FILE* file = zzip_disk_fopen (disk, name);
    if (file) 
    {
	char buffer[1024]; int len;
	while ((len = zzip_disk_fread (buffer, 1, 1024, file))) 
	{
	    fwrite (buffer, 1, len, out);
	}
	
	zzip_disk_fclose (file);
    }
}

int 
main (int argc, char ** argv)
{
    int argn;
    ZZIP_DISK* disk;

    if (argc <= 1 || ! strcmp (argv[1], "--help"))
    {
        printf (usage);
        return 0;
    }
    if (! strcmp (argv[1], "--version"))
    {
	printf (__FILE__" version "ZZIP_PACKAGE" "ZZIP_VERSION"\n");
	return 0;
    }

    disk = zzip_disk_open (argv[1]);
    if (! disk) {
	perror(argv[1]);
	return -1;
    }

    if (argc == 2)
    {  /* print directory list */
	ZZIP_DISK_ENTRY* entry = zzip_disk_findfirst(disk);
	for (; entry ; entry = zzip_disk_findnext(disk, entry))
	{
	    char* name = zzip_disk_entry_strdup_name (disk, entry);
	    printf ("%s\n", name);
	    free (name);
	}
	return 0;
    }

    if (argc == 3)
    {  /* list from one spec */
	ZZIP_DISK_ENTRY* entry = 0;
	while ((entry = zzip_disk_findmatch(disk, argv[2], entry, 0, 0)))
	     zzip_disk_entry_fprint (disk, entry, stdout);

	return 0;
    }

    for (argn=1; argn < argc; argn++)
    {   /* list only the matching entries - each in order of commandline */
	ZZIP_DISK_ENTRY* entry = zzip_disk_findfirst(disk);
	for (; entry ; entry = zzip_disk_findnext(disk, entry))
	{
	    char* name = zzip_disk_entry_strdup_name (disk, entry);
	    if (! fnmatch (argv[argn], name, 
			   FNM_NOESCAPE|FNM_PATHNAME|FNM_PERIOD))
		zzip_disk_cat_file (disk, name, stdout);
	    free (name);
	}
    }
    return 0;
} 

/* 
 * Local variables:
 * c-file-style: "stroustrup"
 * End:
 */
