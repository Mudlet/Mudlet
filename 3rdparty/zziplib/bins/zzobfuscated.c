/*
 *	Copyright (c) 2002 Mike Nordell
 *  portions  Copyright (c) 2000,2001,2002 Guido Draheim <guidod@gmx.de>
 *      Use freely under the restrictions of the ZLIB License
 *
 * A small example that displays how the plugin I/O functions can be used
 * to read "encrypted" zip archives.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <zzip/zzip.h>
#include <zzip/plugin.h>

#ifndef O_BINARY
#define O_BINARY 0
#endif

#if defined ZZIP_HAVE_UNISTD_H
#include <unistd.h> /* read */
#elif defined ZZIP_HAVE_IO_H
#include <io.h>     /* win32 */
#else
#endif

#ifdef _MSC_VER
#define _MSC_VER_NULL NULL
#else
#define _MSC_VER_NULL
#endif

/*
 * Only override our the read handler. Let the system take care
 * the rest.
 */

static zzip_ssize_t our_read(int fd, void* buf, zzip_size_t len)
{
    const zzip_ssize_t bytes = read(fd, buf, len);
    zzip_ssize_t i;
    char* pch = (char*)buf;
    for (i=0; i<bytes; ++i) {
        pch[i] ^= 0x55;
    }
    return bytes;
}

static zzip_plugin_io_handlers our_handlers = { _MSC_VER_NULL };
static const char* const our_fileext [] = { ".dat", ".sav", 0 };


static const char usage[] = 
{
    " zzobfuscated <file> [in-zip filename]\n"
    "  - Demonstrates the use of installable file I/O handlers.\n"
    " Copies <file> to \"obfuscated[.dat]\" while \"encrypting\" it by xor'ing\n"
    " every byte with 0x55, installs file I/O handlers, and then uses the\n"
    " zzip_open_ext_io function to read and print the file to stdout.\n"
    " The file can be a normal file or an inflated part of a zip-archive,\n"
    " to get 'README' from test.zip you may write \n"
    "    zzobfuscated test.zip README \n"
};

int 
main (int argc, char* argv[])
{
    if (argc <= 1 || argc > 3 || ! strcmp (argv[1], "--help"))
    {
        printf (usage);
        return 0;
    }
    if (! strcmp (argv[1], "--version"))
    {
	printf (__FILE__" version "ZZIP_PACKAGE" "ZZIP_VERSION"\n");
	return 0;
    }

    if (strlen(argv[1]) > 128) {
        fprintf(stderr, "Please provide a filename shorter than 128 chars.\n");
        exit(1);
    }

    /* obfuscate the file */
    {
        int ch;
        FILE* fin;
        FILE* fout;
        fin  = fopen(argv[1], "rb");
        if (!fin) {
            fprintf(stderr, "Can't open input file \"%s\"\n", argv[1]);
            exit(1);
        }
        fout = fopen((argc == 2) ? "obfuscated" : "obfuscated.dat", "wb");
        if (!fout) {
            fprintf(stderr, "Can't open output file \"obfuscated\"\n");
            exit(1);
        }
        while ((ch = fgetc(fin)) != EOF) {
            ch ^= 0x55;
            fputc(ch, fout);
        }
        fclose(fout);
        fclose(fin);
    }

    /* install our I/O hander */
    zzip_init_io(&our_handlers, 0);
    our_handlers.fd.read = &our_read;

    {
#       define argn 2
        ZZIP_FILE* fp;
        char name[256];
        if (argc == 3) {
            sprintf(name, "obfuscated/%s", argv[argn]);
        } else {
            sprintf(name, "obfuscated");
        }
        fp = zzip_open_ext_io (name, O_RDONLY|O_BINARY, ZZIP_PREFERZIP,
			       our_fileext, &our_handlers);

        if (! fp)
        {
            perror (name);
            exit(1);
        }else{
            char buf[17];
            int n;

        /* read chunks of 16 bytes into buf and print them to stdout */
            while (0 < (n = zzip_read(fp, buf, 16)))
            {
                buf[n] = '\0';
#             ifdef STDOUT_FILENO
                write (STDOUT_FILENO, buf, n);
#             else
                fwrite (buf, 1, n, stdout);
#             endif
            }

            if (n == -1) 
                perror (argv[argn]);
        }
    }
    
    return 0;
} 

/* 
 * Local variables:
 * c-file-style: "stroustrup"
 * End:
 */
