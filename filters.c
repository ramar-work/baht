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



