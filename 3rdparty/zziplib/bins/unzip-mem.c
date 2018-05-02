/*
 *	Copyright (c) 2003 Guido Draheim <guidod@gmx.de>
 *      Use freely under the restrictions of the ZLIB license.
 *
 *      This file is used as an example to clarify zzipmmap api usage.
 */

#include <zlib.h> /* crc32 */

#include <zzip/memdisk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef ZZIP_HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef ZZIP_HAVE_IO_H
#include <io.h>
#endif

#include <time.h>

#ifdef ZZIP_HAVE_FNMATCH_H
#include <fnmatch.h>
#else
#define fnmatch(x,y,z) strcmp(x,y)
#endif

#ifndef O_BINARY
#define O_BINARY 0
#endif

#define ___ {
#define ____ }

#define EXIT_SUCCESS 0
#define EXIT_WARNINGS 1
#define EXIT_ERRORS 2
#define EXIT_EFORMAT 3
#define EXIT_ENOMEM 4
#define EXIT_ENOTTY 5
#define EXIT_EINFLATE 6
#define EXIT_ENOARCH 9
#define EXIT_EOPTION 10
#define EXIT_ENOFILE 11
#define EXIT_EDISKFULL 50
#define EXIT_EPREMATURE 51
#define EXIT_USERABORT 80
#define EXIT_BADCOMPRESSION 81
#define EXIT_BADPASSWORD 82

static int status = EXIT_SUCCESS;
static int option_list = 0;        /* "-l" */
static int option_pipe = 0;        /* "-p" */
static int option_verbose = 0;     /* "-v" */
static int option_testcrc = 0;     /* "-t" */
static int option_binary = 0;      /* "-b" */
static int option_nocase = 0;      /* "-C" */
static int option_junkpaths = 0;   /* "-j" */
static int option_dosfiles = 0;    /* "-L" */
static int option_keepold = 0;     /* "-n" */
static int option_overwrite = 0;   /* "-o" */
static int option_quiet = 0;       /* "-q" */
static int option_permbits = 0;    /* "-X" */

static const char usage[] = 
{
    "unzzip-mem <zip> [names].. \n"
    "  - unzzip a zip archive.\n"
    "options:\n"
    "  -l list archive files (name, usize, mtime, comments)\n" /* +totals */
    "  -p extract archive files to pipe, i.e. stdout (binary mode)\n"
    "  -t test archive files (check the crc values)\n"
    "  -v verbose list of archive files\n"
    "  -b accept and ignore (force binary extract)\n"
    "  -C match filenames case-insensitively\n"
    "  -j junk paths (do not recreate directory structure)\n"
    "  -L convert dos filenames to lowercase upon extract\n"
    "  -n never overwrite existing files\n"
    "  -o always overwrite existing files\n"
    "  -q quite operation\n"
    "  -X restore user/owner attributes of files\n"
};

static void zzip_mem_entry_pipe(ZZIP_MEM_DISK* disk, 
				ZZIP_MEM_ENTRY* entry, FILE* out)
{
    ZZIP_DISK_FILE* file = zzip_mem_entry_fopen (disk, entry);
    if (file) 
    {
	char buffer[1024]; int len;
	while ((len = zzip_mem_disk_fread (buffer, 1024, 1, file)))
	    fwrite (buffer, len, 1, out);
	
	zzip_mem_disk_fclose (file);
    }
}

static void zzip_mem_entry_make(ZZIP_MEM_DISK* disk, 
				ZZIP_MEM_ENTRY* entry)
{
    FILE* file = fopen (entry->zz_name, "w");
    if (file) { zzip_mem_entry_pipe (disk, entry, file); fclose (file); }
    perror (entry->zz_name);
    if (status < EXIT_WARNINGS) status = EXIT_WARNINGS;
}

/* ------------------------- test ------------------------------------ */

static char* archive = 0;
static int test_errors = 0;

static void zzip_mem_entry_test(ZZIP_MEM_DISK* disk, 
				ZZIP_MEM_ENTRY* entry)
{
    ZZIP_DISK_FILE* file = zzip_mem_entry_fopen (disk, entry);
    printf ("    testing: %s ", entry->zz_name);
    if (strlen (entry->zz_name) < 24) {
	printf ("%.*s", 24 - (int) strlen (entry->zz_name),
		"                        ");
    }
    if (file) 
    {
	unsigned long crc = crc32 (0L, NULL, 0);
	unsigned char buffer[1024]; int len; 
	while ((len = zzip_mem_disk_fread (buffer, 1024, 1, file))) {
	    crc = crc32 (crc, buffer, len);
	}
	
	zzip_mem_disk_fclose (file);
	if (crc == (unsigned long) entry->zz_crc32) {
	    printf ("OK\n");
	} else {
	    printf ("BAD %lx (should be %lx)\n", crc, entry->zz_crc32);
	    test_errors ++;
	} 
    } else {
	printf ("ERROR (no such file)\n");
	test_errors ++;
    }
}

static void zzip_mem_entry_test_start(void)
{
    test_errors = 0;
    printf ("Archive: %s\n", archive);
}

static void zzip_mem_entry_test_done(void)
{
    if (test_errors == 0) {
	printf ("No errors detected in compressed data of %s\n", archive);
    } else {
	printf ("%i errors detected in compressed data of %s\n", 
		test_errors, archive);
    }
}

/* ------------------------- list ------------------------------------ */

static char _zzip_time_[30];
static char* _zzip_ctime (const time_t* timeval) 
{
    struct tm* date = localtime (timeval);
    sprintf (_zzip_time_, "%02i-%02i-%02i %02i:%02i",
	     date->tm_mon, date->tm_mday, date->tm_year%100,
	     date->tm_hour, date->tm_min);
    return _zzip_time_;
}

static const char* comprlevel[] = {
    "stored",   "shrunk",   "redu:1",   "redu:2",   "redu:3",   "redu:4",
    "impl:N",   "toknze",   "defl:N",   "defl:B",   "impl:B" };

zzip_off_t sum_usize = 0;
zzip_off_t sum_csize = 0;
zzip_off_t sum_files = 0;
#define L (long)

static void zzip_mem_entry_direntry_start (void)
{
    sum_usize = 0;
    sum_csize = 0;
    sum_files = 0;
    if (option_verbose) goto verbose;
    printf("  Length    Date & Time     Name\n");
    printf(" --------    ----   ----    ----\n");
    return;
 verbose:
    printf(" Length   Method    Size  Ratio   Date   Time   CRC-32    Name\n");
    printf("--------  ------  ------- -----   ----   ----   ------    ----\n");
}

static void zzip_mem_entry_direntry_done (void)
{
    char exp = ' ';
    if (sum_usize / 1024 > 1024*1024*1024) { exp = 'G';
	sum_usize /= 1024*1024*1024; sum_usize /= 1024*1024*1024; }
    if (sum_usize > 1024*1024*1024) { exp = 'M';
	sum_usize /= 1024*1024; sum_csize /= 1024*1024; }
    if (sum_usize > 1024*1024) { exp = 'K';
	sum_usize /= 1024; sum_csize /= 1024; }
    if (option_verbose) goto verbose;
    printf(" --------                   ----\n");
    printf(" %8li%c           %8li %s\n", L sum_usize, exp, L sum_files,
	   sum_files == 1 ? "file" : "files");
    return;
 verbose:
    printf("--------  ------  ------- -----                           ----\n");
    printf("%8li%c       %8li%c %3li%%                     %8li %s\n",
	   L sum_usize, exp, L sum_csize, exp, 
	   L (100 - (sum_csize*100/sum_usize)), L sum_files, 
	   sum_files == 1 ? "file" : "files");
}

static void zzip_mem_entry_direntry(ZZIP_MEM_ENTRY* entry)
{
    char* name = zzip_mem_entry_to_name (entry);
    zzip_off_t usize = zzip_mem_entry_usize (entry);
    zzip_off_t csize = zzip_mem_entry_csize (entry);
    int compr = zzip_mem_entry_data_comprlevel (entry);
    time_t mtime = entry->zz_mktime;
    long crc32 = entry->zz_crc32;
    const char* comment = zzip_mem_entry_to_comment (entry);
    char exp = ' ';

    sum_usize += usize;
    sum_csize += csize;
    sum_files += 1;

    if (usize / 1024 > 1024*1024*1024) { exp = 'G';
	usize /= 1024*1024*1024; usize /= 1024*1024*1024; }
    if (usize > 1024*1024*1024) { exp = 'M';
	usize /= 1024*1024; csize /= 1024*1024; }
    if (usize > 1024*1024) { exp = 'K';
	usize /= 1024; csize /= 1024; }
   
    if (! comment) comment = "";
    if (*name == '\n') name++;

    if (option_verbose) {
	printf("%8li%c %s %8li%c%3li%%  %s  %8lx  %s %s\n", 
	       L usize, exp, comprlevel[compr], L csize, exp, 
	       L (100 - (csize*100/usize)),
	       _zzip_ctime(&mtime), crc32, name, comment);
    } else {
	printf(" %8li%c %s   %s %s\n", 
	       L usize, exp, _zzip_ctime(&mtime), name, comment);
    }    
}

static void zzip_mem_entry_listfiles(ZZIP_MEM_DISK* disk, char* filespec)
{
    zzip_mem_entry_direntry_start ();
    ___ ZZIP_MEM_ENTRY* entry = 0;
    while ((entry = zzip_mem_disk_findmatch(disk, filespec, entry, 0, 0)))
	zzip_mem_entry_direntry (entry); ____;
    zzip_mem_entry_direntry_done ();
}

static void zzip_mem_entry_listall(ZZIP_MEM_DISK* disk)
{
    zzip_mem_entry_direntry_start ();
    ___ ZZIP_MEM_ENTRY* entry = zzip_mem_disk_findfirst(disk);
    for (; entry ; entry = zzip_mem_disk_findnext(disk, entry))
	zzip_mem_entry_direntry (entry); ____;
    zzip_mem_entry_direntry_done ();
}

static void zzip_mem_entry_testfiles(ZZIP_MEM_DISK* disk, char* filespec)
{
    zzip_mem_entry_test_start ();
    ___ ZZIP_MEM_ENTRY* entry = 0;
    while ((entry = zzip_mem_disk_findmatch(disk, filespec, entry, 0, 0)))
	zzip_mem_entry_test (disk, entry); ____;
    zzip_mem_entry_test_done ();
}

static void zzip_mem_entry_testall(ZZIP_MEM_DISK* disk)
{
    zzip_mem_entry_test_start ();
    ___ ZZIP_MEM_ENTRY* entry = zzip_mem_disk_findfirst(disk);
    for (; entry ; entry = zzip_mem_disk_findnext(disk, entry))
	zzip_mem_entry_test (disk, entry); ____;
    zzip_mem_entry_test_done ();
}

static void zzip_mem_entry_pipefiles(ZZIP_MEM_DISK* disk, char* filespec)
{
    ZZIP_MEM_ENTRY* entry = 0;
    while ((entry = zzip_mem_disk_findmatch(disk, filespec, entry, 0, 0)))
	zzip_mem_entry_pipe (disk, entry, stdout);
}

static void zzip_mem_entry_pipeall(ZZIP_MEM_DISK* disk)
{
    ZZIP_MEM_ENTRY* entry = zzip_mem_disk_findfirst(disk);
    for (; entry ; entry = zzip_mem_disk_findnext(disk, entry))
	zzip_mem_entry_pipe (disk, entry, stdout);
}

static void zzip_mem_entry_makefiles(ZZIP_MEM_DISK* disk, char* filespec)
{
    ZZIP_MEM_ENTRY* entry = 0;
    while ((entry = zzip_mem_disk_findmatch(disk, filespec, entry, 0, 0)))
	zzip_mem_entry_make (disk, entry);
}

static void zzip_mem_entry_makeall(ZZIP_MEM_DISK* disk)
{
    ZZIP_MEM_ENTRY* entry = zzip_mem_disk_findfirst(disk);
    for (; entry ; entry = zzip_mem_disk_findnext(disk, entry))
	zzip_mem_entry_make (disk, entry);
}

int 
main (int argc, char ** argv)
{
    int argn; int archname = 0; int filespec = 0;
    ZZIP_MEM_DISK* disk;
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

    for (argn=1; argn < argc; argn++) {
	if ((argv[argn])[0] == '-') {
	    int x = 1;
	    while ((argv[argn])[x]) {
		switch (argv[argn][x]) {
		case 'l': option_list++; break;
		case 'p': option_pipe++; break;
		case 't': option_testcrc++; break;
		case 'v': option_verbose++; break;
		case 'b': option_binary++; break;
		case 'C': option_nocase++; break;
		case 'j': option_junkpaths++; break;
		case 'L': option_dosfiles++; break;
		case 'n': option_keepold++; break;
		case 'o': option_overwrite++; break;
		case 'q': option_quiet++; break;
		case 'X': option_permbits++; break;
		}
		x++;
	    }
	    (argv[argn])[0] = 0;
	} else {
	    if (! archname) { archname = argn; continue; }
	    if (! filespec) { filespec = argn; continue; }
	}
    }

    if (! archname) {
	printf (usage);
	return 0;
    } else archive = argv[archname];

    disk = zzip_mem_disk_open (argv[archname]);
    if (! disk) {
	perror(argv[archname]);
	return -1;
    }

    if (option_list || option_verbose) {
	if (! filespec) {
	    zzip_mem_entry_listall (disk);
	} else {
	    for (argn=filespec; argn < argc; argn++) {
		if (argv[argn][0]) {
		    zzip_mem_entry_listfiles (disk, argv[argn]);
		}
	    }
	}
    } else if (option_pipe) {
	if (! filespec) {
	    zzip_mem_entry_pipeall (disk);
	} else {
	    for (argn=filespec; argn < argc; argn++) {
		if (argv[argn][0]) {
		    zzip_mem_entry_pipefiles (disk, argv[argn]);
		}
	    }
	}
    } else if (option_testcrc) {
	if (! filespec) {
	    zzip_mem_entry_testall (disk);
	} else {
	    for (argn=filespec; argn < argc; argn++) {
		if (argv[argn][0]) {
		    zzip_mem_entry_testfiles (disk, argv[argn]);
		}
	    }
	}
    } else {
	if (! filespec) {
	    zzip_mem_entry_makeall (disk);
	} else {
	    for (argn=filespec; argn < argc; argn++) {
		if (argv[argn][0]) {
		    zzip_mem_entry_makefiles (disk, argv[argn]);
		}
	    }
	}
    }

    return status;
} 

/* 
 * Local variables:
 * c-file-style: "stroustrup"
 * End:
 */
