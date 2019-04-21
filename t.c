#include "vendor/single.h"

const char *mimes[] = {
 "application/x-aac"                                                            
,"audio/flac"                                                                  
,"image/jpeg"                                                                 
,"application/vnd.oasis.opendocument.spreadsheet"                            
,"nothing,"
,NULL
};

int main(int argc, char *argv[]) {
	const char **m = mimes;
	while ( *m ) {	
		int p = mti_fmt( *m );
		fprintf( stderr, "mime: %d\n", p  );
		fprintf( stderr, "filetype: %s, ", mti_getf( p )  );
		fprintf( stderr, "mimetype: %s\n", mti_getm( p )  );
		m++;
	}
	return 0;
}
