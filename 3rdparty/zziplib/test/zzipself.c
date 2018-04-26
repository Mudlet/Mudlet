/*
 *	Copyright (c) 2000,2001,2002 Guido Draheim <guidod@gmx.de>
 *      Use freely under the restrictions of the ZLIB License
 */

#include <stdio.h>
#include <stdlib.h>
#include <zzip/zzip.h>

#ifndef O_BINARY
#define O_BINARY 0
#endif

static const char usage[] = 
{
    " zzipself <file>... \n"
    "  - prints the file to stdout, so you may want to redirect the output; \n"
    " the file can be a normal file or an inflated part of a zip-archive, \n"
    " however in this case the zip-archive happens to be the executed\n"
    " binary itself, as if this program is a self-extract header of a zip. \n"
    "    zzipself README # same as # zzcat zzipself/README \n"
};

int 
main (int argc, char ** argv)
{
	int status = 0;
    int argn;
    if (argc <= 1)
    {
        printf (usage);
        exit (0);
    }
    
    for (argn=1; argn < argc; argn++)
    {
	/* ZZIP_FILE* fp = zzip_fopen (argv[0]+"/"+argv[argn], "rbi"); */
	/* .... = zzip_open (argv[0]+"/"+argv[argn], O_RDONLY|O_BINARY,
	 *                   ZZIP_CASELESS|ZZIP_ONLYZIP, ext, 0); */

	static const char* ext[] = { "", ".exe", ".EXE", 0 };
	ZZIP_FILE* fp;
	ZZIP_DIR* zip = zzip_opendir_ext_io (argv[0], 
				     ZZIP_CASELESS|ZZIP_ONLYZIP, ext, 0);

	if (! zip) { perror(argv[0]); break; }

        fp = zzip_file_open (zip, argv[argn], ZZIP_CASELESS);

        if (! fp)
        {
            perror (argv[argn]);
            continue;
        }else{
            char buf[17];
            int n;

	    /* read chunks of 16 bytes into buf and print them to stdout */
            while (0 < (n = zzip_read(fp, buf, 16)))
            {
                buf[n] = '\0';
#	      ifdef STDOUT_FILENO
                write (STDOUT_FILENO, buf, n);
#	      else
                fwrite (buf, 1, n, stdout);
#             endif
            }

            if (n == -1) 
			{
                perror (argv[argn]);
				status ++;
			}
        }

	zzip_file_close (fp);
	zzip_closedir (zip);
    }
    
    return status;
} 

/* 
 * Local variables:
 * c-file-style: "stroustrup"
 * End:
 */
