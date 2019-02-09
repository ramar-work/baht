/* ... */
#include <stdio.h>
//You might have to load the full CURL lib, and write your own plugin in C/C++

//All links to call for this little test
struct SL { 
	char *page, *href; 
	int sentinel;
}; 

//All of the data and stuff...
#include "data.c"


//why C?  easier to do this data thing
int main (int argc, char *argv[]) {
	struct SL *slp = links;
	while ( !slp->sentinel ) {
		//printf( "%s\n", slp->page );
		char cmd[2048] = {0};
		//wget try 3 times, dump headers at the top of the file
		const char *fmt = "wget --server-response --tries=3 -O files/%s.html %s 2> files/%s.header";
		snprintf( cmd, 2047, fmt, slp->page, slp->href, slp->page );
		FILE *f = popen( cmd, "r" );
		pclose( f );
		slp++;
	}
	return 0;	
}
