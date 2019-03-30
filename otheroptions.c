#if 0
	//Get source somewhere.
	#if 0
	//Load from a static buffer in memory somewhere
	if ( 1 ) {
		block = (char *)yamama;
		len = strlen( block );
		*p = (char *)block;
	}
	#endif

	if ( opt_set( opts, "--file" ) ) {
	#if 1
		if ( !( luaFile = opt_get( opts, "--file" ).s) ) {
			return err_print( 0, "%s", "No file specified." );	
		}
	#else
		if ( !(sc = opt_get( opts, "--file" ).s) ) {
			return err_print( 0, "%s", "No file specified." );	
		}

		if ( !load_page( sc, &b, &len ) ) {
			return err_print( 0, "Error loading page '%s' - %s.", sc, _errbuf );
		}

		*p = (char *)b;
	#endif
	}

	if ( opt_set( opts, "--url" ) ) {
		//sc = opt_get( opts, "--url" ).s;
		if ( !(sc = opt_get( opts, "--url" ).s) ) {
			return err_print( 0, "%s", "No URL specified." );
		}
		//fprintf( stderr, "%s\n", sc ); exit(0);	
		if ( !load_www( sc, &b, &len ) ) {
			return err_print( 0, "Error loading URL '%s' - %s.", sc, _errbuf );
		}

		fprintf(stderr, "%d\n", len ); exit( 0 );

		*p = (char *)b;
	}


	#if 0
	//Open a directory?
	if ( opt_set( opts, "--directory" ) ) {
		srcsrc = opt_get( opts, "--directory" ).s;
		;//load_www( srcsrc, &block, &len );
		//Copy things to things
		*p = (char *)block;
	}
	#endif
#endif

#if 1
#else
	//Let's stop real quick and figure out a consistent data structure for use with this.
	//Supposing I have multiple of this thing, I want something that looks like:
	DType {
		const char *src;      //the content that will be parsed by gumbo
		const char *yaml;     //yaml(or whatever) to help me frame that
		RecordSet records[];  //???, I think this was incase I read from db... not sure how important it is...
		int srctype; ( enum WWW, FILE, INPUT, ... )
		
		//could go ahead and include all needed tables as well...
		//same with root and jump
		//same with all of those damned buffers...
		//could even do the same with sql, provided everything were going to the same place.
		//we'd have a MASSIVE set of sql...
		//however, would it be easier to combine these at the shell level?
		//I could run them in parallel, but I'd still have issues...
	}

	#if 0
	if ( !opt_set( opts, "--backend" ) ) 
		sqlfmt = "INSERT INTO cw_dealer_inventory ( %s ) VALUES ( %s );";
	else {
		const char efmt[] = "SQL backend not chosen (try --mssql, --mysql, or --pgsql)" );
		return err_print( 0, "%s", efmt );
	}
	#else
	if ( opt_set( opts, "--mysql" ) ) 
		sqlfmt = "INSERT INTO cw_dealer_inventory ( %s ) VALUES ( %s );";
	else if ( opt_set( opts, "--pgsql" ) ) 
		sqlfmt = "INSERT INTO cw_dealer_inventory ( %s ) VALUES ( %s );";
	else if ( opt_set( opts, "--mssql" ) ) 
		sqlfmt = "INSERT INTO cw_dealer_inventory ( %s ) VALUES ( %s );";
	else {
		return err_print( 0, "%s", "SQL backend not chosen (try --mssql, --mysql, or --pgsql)" );
	} 
	#endif
#endif

#if 0
	//???
	if ( !*p ) {
		return err_print( 0, "No --file or --url specified.  Nothing to do." );
	}
#endif
