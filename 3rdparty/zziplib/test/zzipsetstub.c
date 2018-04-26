#include <stdio.h>
#include <stdlib.h>

static const char usage[] =
{
    "zzipsetstub <zipfile> <zipsfxstub>... \n"
    " overwrite the header of the zipfile with the sfxstub code.\n"
    " this is usually the last step in creating a selfextract archive\n"
    " or an application with all its data appended as a zip.\n"
};

int 
main (int argc, char ** argv)
{
    int argn;
    if (argc <= 2)
    {
        printf (usage);
        exit (0);
    }

    {
	char buf[17]; int n;
	char* zipfile = 0; FILE* zipFILE = 0;
	char* sfxfile = 0; FILE* sfxFILE = 0;
    
	for (argn=1; argn < argc; argn++)
	{
	    if (argv[argn][0] == '-') continue;
	    if (! zipfile) { zipfile = argv[argn]; continue; }
	    if (! sfxfile) { sfxfile = argv[argn]; continue; }
	    /* superflous argument */
	}

	zipFILE = fopen (zipfile, "r+b");
	if (! zipFILE) { perror (zipfile); return 1; }

	sfxFILE = fopen (sfxfile, "rb");
	if (! sfxFILE) { perror (sfxfile); return 1; }
    
	while (0 < (n = fread(buf, 1, 16, sfxFILE)))
	{
	    buf[n] = '\0';
	    fwrite (buf, 1, n, zipFILE);
	}
	
	if (n == -1) 
	    perror (argv[argn]);

	fclose (sfxFILE);
	fclose (zipFILE);
    }
    
    return 0;
} 

/* 
 * Local variables:
 * c-file-style: "stroustrup"
 * End:
 */
