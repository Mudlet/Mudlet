/*
 *	Copyright (c) 2003 Guido Draheim <guidod@gmx.de>
 *      Use freely under the restrictions of the ZLIB license.
 *
 *      This file is used as an example to clarify zzip api usage.
 */

#include <zzip/zzip.h>
#include <stdio.h>
#include <string.h>

#ifdef ZZIP_HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef ZZIP_HAVE_IO_H
#include <io.h>
#endif

#ifndef O_BINARY
#define O_BINARY 0
#endif

static const char usage[] = 
{
    "unzzip <dir>.. \n"
    "  - unzzip the files contained in a zip archive.\n"
};

int 
main (int argc, char ** argv)
{
    int argn;
    int exitcode = 0;
    zzip_error_t error;

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
  
    for (argn=1; argn < argc; argn++)
    {
        ZZIP_DIR * dir;
        ZZIP_DIRENT d;

        dir = zzip_dir_open(argv[argn], &error);
        if (! dir)
        {
            fprintf (stderr, "did not open %s: \n", argv[argn]);
	    fprintf (stderr, "%s: %s\n", argv[argn], zzip_strerror(error));
	    exitcode++;
            continue;
        }
  
        if (argc > 2) printf ("%s: \n", argv[argn]);

	/* read each dir entry and show one line of info per file */
        while (zzip_dir_read (dir, &d))
        {
	    int output;
	    ZZIP_FILE* input = zzip_file_open (dir, d.d_name, O_RDONLY);
	    if (! input)
	    {
		fprintf (stderr, "|did not open %s: \n", d.d_name);
		fprintf (stderr, "|%s: %s\n", d.d_name, zzip_strerror_of(dir));
		continue;
	    }

	    output = creat (d.d_name, 0664);
	    if (output == -1)
	    {
		fprintf (stderr, "|output file %s: \n", d.d_name);
		perror(d.d_name);
		zzip_file_close (input);
		continue;
	    }

	    printf("%s\n", d.d_name);
	    
	    { 
		char buf[17]; zzip_ssize_t n;
		/* read chunks of 16 bytes into buf */
		while (0 < (n = zzip_read (input, buf, 16)))
		{
		    write (output, buf, n);
		}

		if (n == -1)
		    perror (d.d_name);
	    }
	    close (output);
	    zzip_file_close (input);
        }

        zzip_dir_close(dir);
    }
    
    return exitcode;
} 

/* 
 * Local variables:
 * c-file-style: "stroustrup"
 * End:
 */
