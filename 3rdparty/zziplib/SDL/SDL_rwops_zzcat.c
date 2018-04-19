/*
 *      Copyright (c) 2001 Guido Draheim <guidod@gmx.de>
 *      Use freely under the restrictions of the ZLIB License
 *
 *      (this example uses errno which might not be multithreaded everywhere)
 */

#include <SDL_rwops_zzip.h>
#include <stdlib.h> /* exit */

/* mostly a copy from zzcat.c */

int main (int argc, char** argv)
{
    static const char usage[] =
	" zzcat <file>... \n"
	"  - prints the file to stdout. the file can be a normal file\n"
	"  or an inflated part of a zip-archive \n"
	;

    int argn;
    if (argc <= 1)
    {
        printf (usage);
        exit (0);
    }

    for (argn=1; argn < argc; argn++)
    {
	SDL_RWops* rwops;

	rwops = SDL_RWFromZZIP (argv[argn], "rb");
        if (! rwops)
        {
	    perror (argv[argn]);
            continue;
        }else{
            char buf[17];
            int n;

            /* read chunks of 16 bytes into buf and print them to stdout */
            while (0 < (n = SDL_RWread(rwops, buf, 1, 16)))
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

	    SDL_RWclose (rwops);
        }
    }

    return 0;
}



