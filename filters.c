//filters.c
#define FILTER(name) \
	name##_filter (char *block, char **dest, int *destlen, void *p, const char **y)

//Just return the letters 'asdf'
int FILTER(asdf) {
	const int len = 5;
	*dest = malloc( len );
	memset( *dest, 0, len );
	memcpy( *dest, "asdf", 4 );	
	*destlen = len - 1;
	return 1;	
}
#if 0
#endif


//Return the block in reverse 
int FILTER(rev) {
	int i = 0;
	int len = strlen( block );
	char *b = &block[ len - 1 ];
	char *d = malloc( len + 1 );
	memset( d, 0, len + 1 );

	while ( len-- ) d[ i++ ] = *(b--);
	*dest = d;
	*destlen = i;
	return 1;	
}
#if 0
#endif


//download (download assets)
int FILTER(download) {
	wwwResponse http;
	wwwType t;
	char *dir = filter_ref( "download_dir", NULL )->value;
	char *root = filter_ref( "source_url", NULL )->value;
	char *fname = rand_alnum( 24 );
	char *fb = NULL;

	//negotiate the url
	select_www( block, &t );
	char *protocol = ( t.secure ) ? "https://" : "http://";
	fb = ( !t.fragment ) ? block : strcmbd( "", protocol, root, block );

	//free the things in http that the things do the things... yep
	if ( !load_www( fb, &http ) ) {
		fprintf( stderr,"load_www() failed.\n" );
		( t.fragment ) ? free( fb ) : 0;
		*dest = "", *destlen = 1;
		return 1;
	}

	//free the full http address
	if ( t.fragment ) { 
		free( fb );
	} 

	//only move forward if http.status == 200
	if ( http.status != 200 ) {
		fprintf( stderr,"status not 200 OK, download failed.\n" );
		*dest = "", *destlen = 1;
		return 0;
	}

	//write out files
	if ( !writeFd( fname, http.body, http.clen ) ) {
		*dest = "", *destlen = 1;
		return 0;
	}

	//return the filename...
#if 1
	*dest = fname, *destlen = strlen( fname ) - 1;
#else
	*dest = block, *destlen = strlen(block);
#endif
	return 1;
}
#if 0
#endif


//follow (this should attempt to follow redirects)
int FILTER(follow) {
	//this needs another run of baht, and then still needs to do things
	wwwResponse http;
	wwwType t;
	memset( &http, 0, sizeof(wwwResponse) );
	memset( &t, 0, sizeof(wwwType) );
	char *dir = filter_ref( "download_dir", NULL )->value;
	char *root = filter_ref( "source_url", NULL )->value;
	char *fb = NULL;
	char *protocol = NULL;

	//negotiate the url
	select_www( block, &t );
	protocol = ( t.secure ) ? "https://" : "http://";
	fb = ( !t.fragment ) ? block : strcmbd( "", protocol, root, block );

	//Now load the page
	if ( !load_www( block, &http ) ) {
		fprintf( stderr,"load_www() failed.\n" );
		( t.fragment ) ? free( fb ) : 0;
		*dest = "", *destlen = 1;
		return 1;
	}

	//check the status, if it's a redirect, it should be done from load_www
	if ( http.status != 200 ) {
		fprintf( stderr,"status not 200 OK, download failed.\n" );
		( t.fragment ) ? free( fb ) : 0;
		*dest = "", *destlen = 1;
		return 1;
	}

#if 1
	//write the file for test purposes
	char *fname = rand_alnum( 24 );
	if ( !writeFd( fname, http.body, http.clen ) ) {
		( t.fragment ) ? free( fb ) : 0;
		*dest = "", *destlen = 1;
		return 0;
	}
#endif

	//return things
	*dest = (char *)http.body;
	*destlen = http.clen;
	( t.fragment ) ? free( fb ) : 0;
	return 1;
}
#if 0
#endif


//checksum (generate a checksum from either an image or binary)
//int checksum_filter ( char *block, char **dest, int *destlen, void *p, const char **y ) {
int FILTER(checksum) {
	*dest = block;
	*destlen = strlen(block);
	return 1;
}
#if 0
#endif


//trim the strings
int FILTER(trim) {
	int nlen= 0;
	int len = strlen( block );
	uint8_t *tr;

	if ( !*y || **y == '\0' ) { 
		*dest = block, *destlen = strlen(block);
		fprintf( stderr, "'trim' got no arguments.\n" );
		return 1;
	}

	tr = lt_trim( (uint8_t *)block, (char *)*y, len, &nlen );
	//lt_trim does not add a terminating zero, so we do it
	*dest = malloc( ++nlen  );
	memset( *dest, 0, nlen );
	memcpy( *dest, tr, nlen - 1 );
	*destlen = nlen;
	return 1;
}


//return the right of a string with some characters
int FILTER(rstr) {
	//increment, and find the character
	//while ( *block && *(block++) != **y ) nt--;
	//*dest = (!nt) ? &block[0] : block;
	//*destlen = strlen( (!nt) ? &block[0] : block ); 
	int mlen=0, len=strlen(block);
	while ( *block ) {
		if (*block == **y) { 
			block++, mlen++;
			break;
		}
		block++, mlen++;
	}

	if ( *block ) {
		block -= mlen;
	}

	//copy
	*destlen = len -= mlen;
	*dest = malloc( len );
	memset( *dest, 0, len );
	memcpy( *dest, &block[ mlen ], len );
//fprintf(stderr,"%d, %d\n", mlen, len ); write(2,*dest,len);
	//...?
	memset( block, 0, len + mlen );	
	return 1;
}
#if 0
#endif


//return the left of a string with some characters
//int lstr_filter ( char *block, char **dest, int *destlen, void *p, const char **y ) {
int FILTER(lstr) {
	//fprintf( stderr, "\narg @lstr: %c\n", **y );
	int mlen=0, len = strlen(block);

	while ( *block ) {
		if (*block == **y) { 
			break;
		}
		block++, mlen++;
	}

	if ( *block ) {
		block -= mlen;
		memset( &block[mlen], 0, strlen(block) - len );
	}

	//you can do as much memory stuff as you want...
	*destlen = mlen;
	*dest = malloc( mlen );
	memset( *dest, 0, mlen );
	memcpy( *dest, block, *destlen );
	//write(2,*dest,*destlen);
	return 1;
}
#if 0
#endif


//return the middle of a string with some characters
//int mstr_filter ( char *block, char **dest, int *destlen, void *p, const char **y ) {
int FILTER(mstr) {
	*dest = block;
	*destlen = strlen(block);
	return 1;
}
#if 0
#endif


//replace
int FILTER(replace) {
	//run memstrat, look for the first occurrence unless given a 'global' argument
	int len, ct = 0, pos = 0;
	char *newbuf = NULL;
	const char **countMe = y;
	const char *find, *replace;
	while ( *countMe ) ct++, countMe++;

	//if no arg or incorrect arg count, stop
	if ( !*y || ct < 2 ) {
		*dest = block, *destlen = strlen(block);
		fprintf( stderr, "'replace' got incorrect argument count.\n" );
		return 1;
	}

	//set things
	find = y[0];
	replace = y[1];

	//if you can't find it, stop
	if (( pos = memstrat( block, find, strlen(block) )) == -1 ) {
		*dest = block, *destlen = strlen(block);
		return 1;
	} 

	//if malloc fails, stop
	len = (strlen(block) - strlen(find)) + strlen(replace);
	if (( newbuf = malloc( len ) ) == NULL) {
		*dest = block, *destlen = strlen(block);
		return 1;
	}

	//copy and be happy
	int sp = 0;
	int op = 0;
	memset( newbuf, 0, len );	
	if ( pos ) {
		memcpy( &newbuf[ sp ], &block[ op ], pos );
		sp += pos, op += pos;
	}	

	if ( !strlen(replace) ) 
		op += strlen(find);
	else {
		memcpy( &newbuf[ sp ], replace, strlen( replace ) ); 
		sp += strlen( replace ), op += strlen(find);
	}

	if ( sp < len ) {
		//fprintf(stderr,"last byte size: %ld\n", strlen(block)-op);
		memcpy( &newbuf[ sp ], &block[ op ], strlen(block) - op ); 	
		sp += strlen(block) - op; 	
	}

	//TODO: I feel like I shouldn't have to do this...
	memset( block, 0, strlen(block) );
	*dest = newbuf;
	*destlen = len;
	return 1;
}
#if 0
#endif


