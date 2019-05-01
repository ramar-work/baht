//filters.c
//Just return the letters 'asdf'
int asdf_filter ( char *block, char **dest, int *destlen, void *p, const char **y ) {
	const int len = 5;
	*dest = malloc( len );
	memset( *dest, 0, len );
	memcpy( *dest, "asdf", 4 );	
	*destlen = len - 1;
	return 1;	
}


//Return the block in reverse 
int rev_filter ( char *block, char **dest, int *destlen, void *p, const char **y ) {
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


//download (download assets)
int download_filter ( char *block, char **dest, int *destlen, void *p, const char **y ) {
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

	//...
#if 0
	fprintf( stderr, "sec:  %d\n", t.secure );
	fprintf( stderr, "port: %d\n", t.port );
	fprintf( stderr, "frag: %d\n", t.fragment );
	//memcpy( &fb[ strlen(dir) + 1 ], block, strlen(block) ); //hmm, this is a little unsafe...
	fprintf(stderr,"Running %s, %s: %d with string '%s'\n", __func__,__FILE__,__LINE__,fb);
	fprintf(stderr,"press enter to continue...\n" );
	getchar();
#endif

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


//follow (this should attempt to follow redirects)
int follow_filter ( char *block, char **dest, int *destlen, void *p, const char **y ) {
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


//checksum (generate a checksum from either an image or binary)
int checksum_filter ( char *block, char **dest, int *destlen, void *p, const char **y ) {
	*dest = block;
	*destlen = strlen(block);
	return 1;
}


//return the right of a string with some characters
int rstr_filter ( char *block, char **dest, int *destlen, void *p, const char **y ) {
	//this function should only have one argument,
	//even better yet if it's a macro, but I'm not sure that can happen the way I'm doing it
	//fprintf( stderr, "\narg @rstr: %c\n", **y );

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


//return the left of a string with some characters
int lstr_filter ( char *block, char **dest, int *destlen, void *p, const char **y ) {
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


//return the middle of a string with some characters
int mstr_filter ( char *block, char **dest, int *destlen, void *p, const char **y ) {
	*dest = block;
	*destlen = strlen(block);
	return 1;
}







