//filters.c

//Just return the letters 'asdf'
int asdf_filter ( char *block, char **dest, int *destlen ) {
	const int len = 5;
	*dest = malloc( len );
	memset( *dest, 0, len );
	memcpy( *dest, "asdf", 4 );	
	*destlen = len - 1;
	return 1;	
}


//Return the block in reverse 
int rev_filter ( char *block, char **dest, int *destlen ) {
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
int download_filter ( char *block, char **dest, int *destlen ) {
	wwwResponse http;
	wwwType t;
	char *dir = ( downloadDir ) ? downloadDir : ".";
	char *fname = rand_alnum( 24 );

	//???
	char fb[ 2048 ];
	memset( &fb, 0, sizeof(fb) );
	memcpy( fb, dir, strlen(dir) );
	memcpy( &fb[ strlen(dir) ], "/", 1 );

	//extract the block and work
	select_www( block, &t );
	if ( t.fragment ) {
		//turn the block into a full url using the source, you can't do this without a void *
	}

	//...
	fprintf( stderr, "sec:  %d\n", t.secure );
	fprintf( stderr, "port: %d\n", t.port );
	fprintf( stderr, "frag: %d\n", t.fragment );
	//memcpy( &fb[ strlen(dir) + 1 ], block, strlen(block) ); //hmm, this is a little unsafe...
#if 1
	fprintf(stderr,"Running %s, %s: %d with string '%s'\n", __func__,__FILE__,__LINE__,block);
	fprintf(stderr,"press enter to continue...\n" );
	getchar();
#endif

	//free the things in http that the things do the things... yep
	if ( !load_www( block, &http ) ) {
		fprintf( stderr,"load_www() failed.\n" );
		*dest = "", *destlen = 1;
		return 1;
	}

	//only move forward if http.status == 200
	if ( http.status != 200 ) {
		fprintf( stderr,"status not 200 OK, download failed.\n" );
		*dest = "", *destlen = 1;
		return 1;
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
int follow_filter ( char *block, char **dest, int *destlen ) {
	//this needs another run of baht, and then still needs to do things
	wwwResponse http;
	wwwType t;
	memset(&http,0,sizeof(wwwResponse));
	memset(&t,0,sizeof(wwwType));
	//check the url, start w/ '/' or 'h' (or 'w')
	//is this even a url? if it's not, this is a fail
fprintf(stderr,"URL :=> %s\n", block);
	select_www( block, &t );
	if ( t.fragment ) {
		//turn the block into a full url using the source, you can't do this without a void *
	}

	//Now load the page
	if ( !load_www( block, &http ) ) {
		fprintf( stderr,"load_www() failed.\n" );
		*dest = "", *destlen = 1;
		return 1;
	}

	//check the status, if it's a redirect, it should be done from load_www
	if ( http.status != 200 ) {
		fprintf( stderr,"status not 200 OK, download failed.\n" );
		*dest = "", *destlen = 1;
		return 1;
	}

	//return things
	*dest = (char *)http.body;
	*destlen = http.clen;
	return 1;
}


//checksum (generate a checksum from either an image or binary)
int checksum_filter ( char *block, char **dest, int *destlen ) {
	*dest = block;
	*destlen = strlen(block);
	return 1;
}




