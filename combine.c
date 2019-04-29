//combine.c - combines chunked encoding xfers...
//i should have fixed this instead of looking for a car...
#include "vendor/single.h"
#include "web.c"


//Test combine with two blocks
const char *filenames[] = {
	NULL
//, "html/https___www_google_com_s.html"
, "tests/combine/https___www_httpwatch_co.html"
#if 0
, "tests/combine/https___i_gifer_com_1TM_.html"
, "tests/combine/https___amp_businessinsi.html"
, "tests/combine/https___i_gifer_com_1TM_.html"
#endif
,	NULL
};


//
int main (int argc, char *argv[]) {
	char **f = ( char ** )filenames;
	f++;
	
	while ( *f ) {

		//fprintf( stderr, "%s\n", *f ); f++;	continue;
		//read the file
		uint8_t src[ 2000000 ];
		//uint8_t *src = NULL;
	#if 0
	 	//safety check...
		struct stat stb;
		memset( &stb, 0, sizeof( struct stat ) );
		int fsz = stat( *f, &stb );
		fsz = stb.st_size ;
		fprintf(stderr, "%d ?= %d\n", fsz, (int)sizeof(src) ); f++;
		continue;
	#endif
		memset( src, 0, sizeof(src) );
		int fd = open( *f, O_RDONLY );
		if ( fd == -1 ) { fprintf(stderr,"File open failed.\n" ); exit(0); }
		int br = read( fd, src, sizeof(src));
		if ( br == -1 ) { fprintf(stderr,"File read failed.\n" ); exit(0); }
		close(fd);	
	#if 0	
		//you can walk through this
		Mem mem;
		memset( &mem, 0, sizeof( Mem ) );
		fprintf( stderr, "bytes read is: %d\n", br );	
	#endif

		//when sending this, we have to skip the header
		const char *t = "\r\n\r\n";
		uint8_t *nsrc = src;
		uint8_t *r = malloc(1);
		int tot=0, y=0, pos = memstrat( src, t, br ); //TODO: you could increment t here
		if ( pos == -1 ) { fprintf(stderr,"carriage return not found...\n" ); exit(0); }

		//Move ptrs and increment things
		//this is the start of the headers...
		nsrc += ( pos += 4 ), br -= pos;
		t += 2;

		//find the next \r\n, after reading the bytes
		while ( 1 ) {
			//define a buffer for size
			//search for another \r\n, since this is the boundary
			char tt[ 64 ];
			memset( &tt, 0, sizeof(tt) );
			int sz, szText = memstrat( nsrc, t, br );
			if ( szText == -1 ) {
				break;
			}
			memcpy( tt, nsrc, szText );
			sz = radix_decode( tt, 16 );
		
			#if 0
			//for debugging's sake, we can copy and change \r\n to make it easy
			char sb[ 32 ];
			memset( sb, 0, sizeof(sb) );
			memcpy( sb, nsrc, 10 );
			for ( int i=0; i<sizeof(sb); i++) sb[i]=(sb[i] == '\r'||sb[i] == '\n') ? "|" : sb[i]; 
	fprintf(stderr,"szText: %s\n",tt); 
	fprintf(stderr,"actual sz: %d\n",sz); 
	fprintf(stderr,"nsrc is currently:\n" );
	write( 2, nsrc, 10 );
			#endif	
	//write( 2, nsrc, 10 );getchar();

			//move up nsrc, and get ready to read that
			nsrc += szText + 2;
			r = realloc( r, tot + sz );
			memset( &r[ tot ], 0, sz );
			//move up y + 2, this should be the content 
			memcpy( &r[ tot ], nsrc, sz );
	//fprintf(stderr,"nsrc is (after copy):\n" );write( 2, nsrc, 10 );

			//modify digits
			nsrc += sz + 2, /*move up ptr + "\r\n" and "\r\n"*/
			tot += sz,
			br -= sz + szText + 2;
	//fprintf(stderr,"br is: %d\nnsrc is (at end):\n",br );write( 2, nsrc, 10 );getchar();

			//this has to be finalized as well
			if ( br == 0 ) {
				break;
			}
		}

		//if there is a content length, it should match 'tot'
		//r->clen = tot;
		int wfd = open( "test", O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IXUSR );
		if ( wfd == -1 ) { fprintf(stderr,"file can't be written...\n" ); exit(0); }
		write( wfd, r, tot );
		close( wfd );
		f++;
	}
	return 0;
}
