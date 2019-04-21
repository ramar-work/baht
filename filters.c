//filters.c

//just return the letters 'asdf'
int asdf_filter ( char *block, char **dest, int *destlen ) {
	const int len = 5;
	*dest = malloc( len );
	memset( *dest, 0, len );
	memcpy( *dest, "asdf", 4 );	
	*destlen = len - 1;
	return 1;	
}


//return the block in reverse 
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


//extract body
int extract_body ( wwwResponse *w ) {
	//get extraction length
	//if it's chunked, we're in trouble anyway... because I'm not breaking down the message	
	return 1;		
}


//download (download assets)
int download_filter ( char *block, char **dest, int *destlen ) {
	wwwResponse http;
	char *dir = ( downloadDir ) ? downloadDir : ".";
	char fb[ 2048 ];
	memset( &fb, 0, sizeof(fb) );
	memcpy( fb, dir, strlen(dir) );
	memcpy( &fb[ strlen(dir) ], "/", 1 );
fprintf(stderr,"Running %s, %s: %d with string '%s'\n", __func__,__FILE__,__LINE__,block);
fprintf(stderr,"press enter to continue...\n" );
getchar();
	if ( !load_www( block, &http ) ) {
		//return the empty string
		fprintf(stderr,"load_www() failed.\n");
		*dest = "";
		*destlen = 1;
		return 1;
	}

	//only move forward if http.status == 200
	if ( http.status != 200 ) {
		fprintf(stderr,"status not 200 OK, download failed.\n");
		*dest = "";
		*destlen = 1;
		return 1;
	}

	//to write images (and pretty much anything else for that matter) you
	//need to extract only the content worth worrying about (&data[ mlen - clen ])
	//writing an 'extract_body' function will be helpful, since we could have xfer chunked
	//requests to worry about...

	//basename on http addr is the simplest naming scheme...
	//another way, although slightly more complicated is to use a generated filename 
	//said filename is returned on success, and the output code knows to write a reference
	//to it, it can be further chopped if necessary


	//writeFd( "myfile", http.data, http.len );
	
	*dest = block;
	*destlen = strlen(block);
	return 1;
}


//follow (this should attempt to follow redirects)
int follow_filter ( char *block, char **dest, int *destlen ) {
	//this needs another run of baht, and then still needs to do things
	wwwResponse w;
	wwwType t;
	memset(&w,0,sizeof(wwwResponse));
	memset(&t,0,sizeof(wwwType));
	//check the url, start w/ '/' or 'h' (or 'w')
	//is this even a url? if it's not, this is a fail
	select_www( block, &t );
	if ( t.fragment ) {
		//turn the block into a full url using the source, you can't do this without a void *
	}
	//Now load the page
	//load_www( block, &w );
	*dest = block;
	*destlen = strlen(block);
	return 1;
}


//checksum (generate a checksum from either an image or binary)
int checksum_filter ( char *block, char **dest, int *destlen ) {
	*dest = block;
	*destlen = strlen(block);
	return 1;
}




