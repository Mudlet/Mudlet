/*
 *	Copyright (c) 2000,2001,2002 Guido Draheim <guidod@gmx.de>
 *      Use freely under the restrictions of the ZLIB License
 *
 *      show zip-reading with xor-obfuscation. 
 *      Note that the difference to the standard zzcat.c is quite small.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <zzip/zzip.h>
#include <zzip/plugin.h>

#if defined ZZIP_HAVE_UNISTD_H
#include <unistd.h>
#elif defined ZZIP_HAVE_IO_H
#include <io.h>
#else
#error need posix io for this example
#endif

#ifndef O_BINARY
#define O_BINARY 0
#endif

static const char usage[] = 
{
    " zzxorcat [-#] <file>... \n"
    "  - prints the file to stdout, so you may want to redirect the output; \n"
    " the file is part of an inflated zip-archive obfuscated with xor value,\n"
    " given by the numeric option (default is 0x55). \n"
    " to get 'README' from dist.dat you may write \n"
    "    zzcat dist/README \n"
};

static int xor_value;

static zzip_ssize_t xor_read (int f, void* p, zzip_size_t l)
{
    zzip_ssize_t r = read(f, p, l);
    zzip_ssize_t x; char* q; for (x=0, q=p; x < r; x++) q[x] ^= xor_value;
    return r;
}

static zzip_plugin_io_handlers xor_handlers;
static zzip_strings_t xor_fileext[] = { ".dat", "", 0 };

int 
main (int argc, char ** argv)
{
    int argn;
    xor_value = 0x55;

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

    zzip_init_io (&xor_handlers, 0); xor_handlers.fd.read = &xor_read;
    
    for (argn=1; argn < argc; argn++)
    {
	ZZIP_FILE* fp;

	if (argv[argn][0] == '-')
	{
	    if (isdigit(argv[argn][1]))	xor_value = atoi (argv[argn]+1);
	    continue;
	}

        fp = zzip_open_ext_io (argv[argn], O_RDONLY|O_BINARY,
			       ZZIP_CASELESS|ZZIP_ONLYZIP,
			       xor_fileext, &xor_handlers);
        if (! fp)
        {
            perror (argv[argn]);
            continue;
        }else{
            char buf[17];
            zzip_ssize_t n;

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
                perror (argv[n]);
        }
    }
    
    return 0;
} 

/* 
 * Local variables:
 * c-file-style: "stroustrup"
 * End:
 */
