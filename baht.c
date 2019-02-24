/* ---------------------------------------------- *
 * baht 
 * =========
 * 
 * Summary
 * -------
 * A robot scraper thingy, which helps build the database 
 * for CheapWhips.net...
 * 
 * Usage
 * -----
 * tbd
 *
 * Flags 
 * -----
 * -DDEBUG - Show debugging information 
 * -DHASHTYPE_* - Compile with different hash schemes? 
 * 
 * Author
 * ------
 * Antonio R. Collins II (ramar@tubularmodular.com, ramar.collins@gmail.com)
 * Copyright: Tubular Modular Feb 9th, 2019
 * 
 * TODO
 * ----

 - either curl or single to make requests and return what came back
 - directory listing to pull multiple files or locations?
 - change hashing to use different level indicators (e.g. div[1,2] vs div)


 - email when done
 - stream to db
 - add options 
 - test out hashes against the table with other types of markup
 - add a way to jump to a specific hash and do the search from there (easiest
   way is to copy the table)
	 Table *nt = lt_copybetween( Table tt, int start, int end );  
	 free( tt );
 - add the option to read directly from memory (may save time)
 - add the option to read from hashes from text file (better than recompiling if
   something goes wrong)

 * ---------------------------------------------- */
#if 0 
#include <stdio.h> 
#include <unistd.h> 
#include <errno.h> 
#include <sys/stat.h> 
#include <sys/types.h> 
#include <fcntl.h> 
#include <string.h> 
#endif
#define LT_DEVICE 1 

#include "vendor/single.h"
#include <gumbo.h>

#if 0
#else
 #include <lua.h>
 #include <lualib.h>
 #include <luaconf.h>
#endif

#define PROG "p"

#define INCLUDE_TESTS

#define RERR(...) fprintf( stderr, __VA_ARGS__ ) ? 1 : 1 

#ifndef DEBUG
 #define DPRINTF( ... )
#else
 #define DPRINTF( ... ) fprintf( stderr, __VA_ARGS__ )
#endif

#define ADD_ELEMENT( ptr, ptrListSize, eSize, element ) \
	if ( ptr ) \
		ptr = realloc( ptr, sizeof( eSize ) * ( ptrListSize + 1 ) ); \
	else { \
		ptr = malloc( sizeof( eSize ) ); \
	} \
	*(&ptr[ ptrListSize ]) = element; \
	ptrListSize++;

#define SET_INNER_PROC(p, t, rn, jn, fk, rk) \
	InnerProc p = { \
		.parent   = lt_retkv( t, rn ) \
	 ,.srctable = t \
	 ,.jump = jn \
	 ,.key  = fk \
	 ,.rkey  = rk \
	 ,.level = 0 \
	 ,.keylen = strlen( fk ) \
	 ,.tlist = NULL \
	 ,.tlistLen = 0 \
	 ,.hlist = NULL \
	 ,.hlistLen = 0 \
	 ,.checktable = NULL \
	}


/*Data types*/
typedef struct { char *k, *v; } yamlList;


//...
typedef struct useless_structure {
	LiteKv *parent;
	Table *srctable;
	int rootNode;
	int jumpNode;
	int jump;
	int level;
	char *key;
	char *rkey;
	int keylen;
	int hlistLen;
	int *hlist;
	int tlistLen;
	Table **tlist;
	Table *ctable;
	Table *checktable;
} InnerProc;


//Approximate where something is, by checking the similarity of its parent
typedef struct simp { 
	LiteTable *parent; //Check the parent at this stage 
	int tlistLen, ind, max;
	Table **tlist;     //Now, we need more than one check table, doing it this way anyway
	Table *top;      //Can't remember if I can trade ptrs or not...
} Simp;


//?
typedef struct lazy {
	short ind, len;//, *arr;
} oCount; 


typedef struct nodeset {
	int hash;             //Stored hash
	const char *key;      //Key that the value corresponds to
	const char *string;   //String
} NodeSet;


typedef struct nodeblock {
	//The content to digest
	//const uint8_t *html;

	//The "root element" that encompasses the elements we want to loop through
	NodeSet rootNode;

	//A possible node to jump to
	NodeSet jumpNode;

	//A set of elements containing entries
	NodeSet *loopNodes;

	//Set of tables (each of the nodes, copied)
	Table *tlist;
} Nodeblock;


#ifdef INCLUDE_TESTS 
 #include "test.c"
#endif


//An error string buffer (useless)
char _errbuf[2048] = {0};


//Things we'll use
const char *gumbo_types[] = {
	"document"
, "element"
, "text"
, "cdata"
, "comment"
, "whitespace"
, "template"
};


const char *errMessages[] = {
	NULL
};


//Return the type name of a node
const char *print_gumbo_type ( GumboNodeType t ) {
	return gumbo_types[ t ];
}


//Use this to return from functions
int err_set ( int status, char *fmt, ... ) {
	va_list ap;
	va_start( ap, fmt ); 
	vsnprintf( _errbuf, sizeof( _errbuf ), fmt, ap );  
	va_end( ap );
	return status;
}


//Use this to return from main()
int err_print ( int status, char *fmt, ... ) {
	fprintf( stderr, PROG ": " );
	va_list ap;
	va_start( ap, fmt ); 
	//write in the "base" arg first (the program name)
	//then do each of the others...
	vfprintf( stderr, fmt, ap );  
	va_end( ap );
	fprintf( stderr, "\n" );
	return status;
}

//Debugging stuff.
void print_innerproc( InnerProc *pi ) {
	fprintf( stderr, "parent:   %p\n", pi->parent );
	fprintf( stderr, "rootNode: %d\n", pi->rootNode );
	fprintf( stderr, "jumpNode: %d\n", pi->jumpNode );
	fprintf( stderr, "jump:     %d\n", pi->jump );
	fprintf( stderr, "level:    %d\n", pi->level );
	fprintf( stderr, "key:      %s\n", pi->key );
	fprintf( stderr, "rkey:     %s\n", pi->rkey );
	fprintf( stderr, "keylen:   %d\n", pi->keylen );
	fprintf( stderr, "hlistLen: %d\n", pi->hlistLen );
	fprintf( stderr, "tlistLen: %d\n", pi->tlistLen );
	fprintf( stderr, "hlist:    %p\n", pi->hlist );

	fprintf( stderr, "srctable:   %p\n", pi->srctable );
	fprintf( stderr, "ctable:     %p\n", pi->ctable );
	fprintf( stderr, "checktable: %p\n", pi->checktable );
	fprintf( stderr, "tlist:      %p\n", pi->tlist );
}


//
void print_tlist ( Table **tlist, int len, Table *ptr ) {
	//Print
	for ( int i=0; i < len; i++ ) {
		Table *t = tlist[i];
		//fprintf( stderr, "%p%s, ", t, sk->top == t ? "(*)" : "" );
		fprintf( stderr, "%p%s, ", t, (ptr == t) ? "(*)" : "" );
	}
	fprintf( stderr, "\n" );
}


//Find a specific tag within a nodeset 
GumboNode* find_tag ( GumboNode *node, GumboTag t ) {
	GumboVector *children = &node->v.element.children;

	for ( int i=0; i<children->length; i++ ) {
		//Get data "endpoints"
		GumboNode *gn = children->data[ i ] ;
		const char *gtagname = gumbo_normalized_tagname( gn->v.element.tag );
		const char *gtype = (char *)print_gumbo_type( gn->type );

		//I need to move through the body only
		if ( gn->v.element.tag == t ) {
			return gn;
		}
	}
	return NULL;
}


//Load a page and write to buffer
int load_page ( const char *file, char **dest, int *destlen ) {

	int fn, len=0;
	struct stat sb;
	char *p=NULL;

	//Read file to memory
	if ( stat( file, &sb ) == -1 )
		return err_set( 0, "%s", strerror( errno ) );

	//Open the file
	if ( (fn = open( file, O_RDONLY )) == -1 ) 
		return err_set( 0, "%s", strerror( errno ) );

	//Allocate a buffer big enough to just write to memory.
	if ( !(p = malloc( sb.st_size + 10 )) ) 
		return err_set( 0, "%s", strerror( errno ) );

	//Read the file into buffer	
	if ( read( fn, p, sb.st_size ) == -1 ) {
		free( p );
		return err_set( 0, "%s", strerror( errno ) );
	}

	//Free the error message buffer
	*dest = p;
	*destlen = sb.st_size;
	return 1;
}


//Grab a URI and write to buffer
int load_www ( const char *address, unsigned char **dest, int *destlen ) {
	int fn;
	struct stat sb;
#if 0
	//Can make a raw socket connection, but should use cURL for it...
	//....
	//connect( );
	//I can use single for this... do a HEAD, then do a GET
	//socket_connect( &sk (	

	//Read the file into buffer	
	if ( read( fn, dest, sb.st_size ) == -1 ) {
		fprintf( stderr, "%s: %s\n", strerror( errno ) );
		return 0; 
	}
#endif
	*destlen = 0;
	return 1;
}


//Build table from yamlList
int yamlList_to_table ( yamlList *list, Table *t ) {

	//TODO: Get a count of elements for efficient hashing
	if ( !lt_init( t, NULL, 256 ) ) {
		return 0;
	}

	//Loop through the list and do some work.
	//for ( int i=0; i<sizeof(list)/sizeof(yamlList); i++ ) {
	yamlList *x = list;
	while ( x->k ) {
	#if 1
		lt_addblobkey( t, (uint8_t *)x->k, strlen( x->k ) );
		lt_addintvalue( t, 1 );
	#else
		lt_addttkey( t, x->k );
		lt_addintvalue( t, 1 );
	#endif	
		lt_finalize( t );
		x++;
	}
	
	//Dump the built hash.
	//lt_dump( t );
	return 1;

}


//Return appropriate block
char *retblock ( GumboNode *node ) {

	char *iname = NULL;
	//Give me some food for thought on what to do
	if ( node->type == GUMBO_NODE_DOCUMENT )
		iname = "d";
	else if ( node->type == GUMBO_NODE_CDATA )
		iname = "a";
	else if ( node->type == GUMBO_NODE_COMMENT )
		iname = "c";
	else if ( node->type == GUMBO_NODE_WHITESPACE )
		iname = "w";
	else if ( node->type == GUMBO_NODE_TEMPLATE )
		iname = "t";
	else if ( node->type == GUMBO_NODE_TEXT )
		iname = (char *)node->v.text.text;
	else if ( node->type == GUMBO_NODE_ELEMENT ) {
		iname = (char *)gumbo_normalized_tagname( node->v.element.tag );
	}

	return iname;
}


//Go through and run something on a node and ALL of its descendants
//Returns number of elements found (that match a certain type)
//TODO: Add element count 
//TODO: Add filter for element count
int gumbo_to_table ( GumboNode *node, Table *tt ) {
	//Loop through the body and create a "Table" 
	GumboVector *bc = &node->v.element.children;
	int stat = 0;

	//For first run, comments, cdata, document and template nodes do not matter
	for ( int i=0; i<bc->length; i++ ) {
		//Set up data
		GumboNode *n = bc->data[ i ] ;
		char *itemname = retblock( n );
		//DPRINTF( "%06d, %04d, %-10s, %s\n", ++gi, i, print_gumbo_type(n->type) , itemname );

		//Handle what to do with the actual node
		//TODO: Handle GUMBO_NODE_[CDATA,COMMENT,DOCUMENT,WHITESPACE,TEMPLATE]
		if ( n->type != GUMBO_NODE_TEXT && n->type != GUMBO_NODE_ELEMENT ) {
			//User selected handling can take place here, usually a blank will do
			//twoSided = 0; 
			//lt_addnullvalue( t, itemname );
			0;
		}
		else if ( n->type == GUMBO_NODE_TEXT ) {
			//Clone the node text as a crude solution
			int cl=0;
			uint8_t *mm = trim( (uint8_t *)itemname, " \t\r\n", strlen(itemname), &cl );
			char *buf = malloc( cl + 1 );
			memset( buf, 0, cl + 1 );
			memcpy( buf, mm, cl );	
			//Handle/save the text reference here
			lt_addtextkey( tt, "text" );
			lt_addtextvalue( tt, buf ); 
			lt_finalize( tt );
			free( buf );	
		}
		else if ( n->type == GUMBO_NODE_ELEMENT ) {
			GumboVector *gv = &n->v.element.children;
			GumboVector *gattr = &n->v.element.attributes;
			GumboAttribute *attr = NULL;
			char item_cname[ 2048 ];
			int maxlen = sizeof( item_cname ) - 1;
			memset( item_cname, 0, maxlen ); 
			char *iname = itemname;

			//the number of children is not enough.
			//you need to check the children and see if any are elemnts, or even better yet,
			//if they share the same tag name (and same hash name)

	#if 0
			fprintf( stderr, "L: %d\n", gv->length );
				getchar();
	#else
			//Add an id or class to a hash
			if ( gattr->length ) {
				if ( ( attr = gumbo_get_attribute( gattr, "id" ) ) ) {
					//fprintf( stderr, "id is: #%s\n", attr->value );	
					snprintf( item_cname, maxlen, "%s#%s", itemname, attr->value ); 	
					iname = item_cname;
				}

				if ( ( attr = gumbo_get_attribute( gattr, "class" ) ) ) {
					//fprintf( stderr, "class is: .%s\n", attr->value );	
					snprintf( item_cname, maxlen, "%s^%s", itemname, attr->value ); 	
					iname = item_cname;
				}
			}
	#endif

			//Should always add this first
			lt_addtextkey( tt, iname );
			lt_descend( tt );
	
			//node elements should all have attributes, I need a list of them, then
			//need them written out
			if ( gattr->length ) {
				lt_addtextkey( tt, "attrs" );
				lt_descend( tt );
				for ( int i=0; i < gattr->length; i++ ) {
					GumboAttribute *ga = gattr->data[ i ];
					lt_addtextkey( tt, ga->name );
					lt_addtextvalue( tt, ga->value );
					lt_finalize( tt );
				}
				lt_ascend( tt );
			}

			if ( gv->length ) {
				gumbo_to_table( n, tt );
			}

			lt_ascend( tt );
		}
	}

	return 1;
}


//Put the raw HTML into a hash table 
int parse_html ( Table *tt, char *b, int len ) {

	//We can loop through the Gumbo data structure and create a node list that way
	GumboOutput *output = gumbo_parse_with_options( &kGumboDefaultOptions, (char *)b, len );
	GumboVector *children = &(output->root)->v.element.children;
	GumboNode *body = find_tag( output->root, GUMBO_TAG_BODY );

	//Stop with blank bodies
	if ( !body )
		return err_set( 0, "%s", "no <body> tag found!" );

	//Allocate a Table
	if ( !lt_init( tt, NULL, 33300 ) )
		return err_set( 0, "%s", "couldn't allocate table!" ); 	

	//Parse the document into a Table
	int elements = gumbo_to_table( body, tt );
	#if 0
	lt_dump( tt );
	lt_geti( tt );
	#endif
	lt_lock( tt );

	//Free the gumbo struture 
	gumbo_destroy_output( &kGumboDefaultOptions, output );
	return 1;

}


//Pull the framing stuff
int create_frame ( LiteKv *kv, int i, void *p ) {
	//A data structure can take both of these...
	InnerProc *pi = (InnerProc *)p;
	#if 0
	DPRINTF("@%5d: %p %c= %p\n", i, kv->parent, ( kv->parent == pi->parent ) ? '=' : '!', pi->parent );
	#endif

	//Check that parents are the same... 
	if ( kv->parent && (kv->parent == pi->parent) && (i >= pi->jump) ) {
		//...and that the full hash matches the key.
		char fkBuf[ 2048 ] = {0};
		if ( !lt_get_full_key(pi->srctable, i, (uint8_t *)fkBuf, sizeof(fkBuf)-1) ) {
			return 1;
		}
	#if 0
		DPRINTF( "fk: %s\n", 
			(char *)lt_get_full_key( pi->srctable, i, fkBuf, sizeof(fkBuf) - 1 ) );
		DPRINTF( "%d ? %ld\n", pi->keylen, strlen( (char *)fkBuf ) ); 
	#endif
		//Check strings and see if they match? (this is kind of a crude check)
		if ( pi->keylen == strlen(fkBuf) && memcmp(pi->key, fkBuf, pi->keylen) == 0 ) {
			//save hash here and realloc a stretching int buffer...
			ADD_ELEMENT( pi->hlist, pi->hlistLen, int, i );
		}
	}
	return 1;
}


//An iterator
int undupify ( LiteKv *kv, int i, void *p ) {
	Simp *sk = (Simp *)p;
	LiteValue vv = kv->value;
	//printf_tlist( sk->tlist, sk->tlistLen );

	//The key should be here...
	if ( kv->key.type == LITE_TXT ) {
		int at = 0, *inc = NULL;
		if (( at = lt_geti( sk->top, kv->key.v.vchar )) > -1 ) {
			inc = (int *)lt_userdata_at( sk->top, at );
			(*inc)++;
			//fprintf( stderr, "int is: %d\n", *inc );
			//fprintf( stderr, "dupkey found, writing '%s%d'\n", kv->key.v.vchar, *inc );
		
			//write a new value
			char buf[1024] = {0};
			snprintf( buf, 1023, "%s%d", kv->key.v.vchar, *inc );
			free( kv->key.v.vchar );
			kv->key.v.vchar = strdup( buf );
		}
		else {
			lt_addtextkey( sk->top, kv->key.v.vchar );		
			inc = malloc( sizeof(int) );
			*inc = 0;
			lt_addudvalue( sk->top, inc );
			lt_finalize( sk->top );
			lt_lock( sk->top );
		}
		//fprintf( stderr, "key (%d) %s\n", at, kv->key.v.vchar );
		//lt_dump( sk->top );
	}	

	//Then you figure out what to do.
	if ( kv->key.type == LITE_TRM ) { 
		sk->ind--;	
		sk->tlistLen--;
		free( sk->top );
		sk->top = NULL;
		sk->top = sk->tlist[ sk->ind ];
	}
	else if ( kv->value.type == LITE_TBL ) {
		sk->ind++;
		Table *tn = malloc(sizeof(Table));
		memset( tn, 0, sizeof(Table));
		lt_init( tn, NULL, 64 ); 
		ADD_ELEMENT( sk->tlist, sk->tlistLen, Table *, tn );
		sk->top = sk->tlist[ sk->ind ];
	}

	return 1;
}


//a simpler way MIGHT be this
//this should return/stop when the terminator's parent matches the checking parent 
int build_ctck ( Table *tt, int start, int end ) {
	LiteKv *kv = lt_retkv( tt, start );
	Simp sk = { &kv->parent->value.v.vtable, 0, 0, 0, NULL };

	//Allocate the first entry
	Table *ft = malloc( sizeof(Table) );
	memset(ft,0,sizeof(Table));
	lt_init( ft, NULL, 64 );
	sk.tlist = malloc( sizeof( Table * ) );
	sk.top = *sk.tlist = ft;
	sk.tlistLen++;

	//Run through and stuff...
	lt_exec_complex( tt, start, end, &sk, undupify );
	//lt_dump( tt );
	return 1;
}


//Pass through and build a smaller subset of tables
int build_individual ( LiteKv *kv, int i, void *p ) {
	//Set refs
	InnerProc *pi = (InnerProc *)p;
	LiteType kt = kv->key.type, vt = kv->value.type;

	//Save key	
	if ( kt == LITE_INT || kt == LITE_FLT ) 
		lt_addintkey( pi->ctable, (kt==LITE_INT) ? kv->key.v.vint : kv->key.v.vfloat );	
	else if ( kt == LITE_BLB )
		lt_addblobkey( pi->ctable, kv->key.v.vblob.blob, kv->key.v.vblob.size );	
	else if ( kt == LITE_TXT )
		lt_addtextkey( pi->ctable, kv->key.v.vchar );	
	else if ( kt == LITE_TRM ) {
		pi->level --;
		lt_ascend( pi->ctable );
		return 1;
	}

	//Save value 
	if ( vt == LITE_INT || vt == LITE_FLT ) 
		lt_addintvalue( pi->ctable, kv->value.v.vint );	
	else if ( vt == LITE_BLB)
		lt_addblobvalue( pi->ctable, kv->value.v.vblob.blob, kv->value.v.vblob.size );	
	else if ( vt == LITE_TXT )
		lt_addtextvalue( pi->ctable, kv->value.v.vchar );	
	else if ( vt == LITE_TBL ) {
		pi->level ++;
		//fprintf( stderr, "%p\n", &kv->value.v.vtable );
		lt_descend( pi->ctable );
		return 1;
	}
	lt_finalize( pi->ctable );
	return 1;
}


//
yamlList ** find_keys_in_mt ( Table *t, yamlList *tn, int *len ) {
	yamlList **sql = NULL;
	int h=0, sqlLen = 0;
	while ( tn->k ) {
		if ( !tn->v )
			;//fprintf( stderr, "No target value for column %s\n", tn->k );	
		else {
			//fprintf( stderr, "hash of %s: %d\n", tn->v, lt_geti( t, tn->v ) );
			if ( (h = lt_geti( t, tn->v )) == -1 )
				0;
			else {
				yamlList *kv = malloc( sizeof(yamlList) );
				kv->k = tn->k;
				kv->v =	lt_text_at( t, h );  
				ADD_ELEMENT( sql, sqlLen, yamlList *, kv ); 
			}
		}
		tn++;
	}
	yamlList *term = malloc( sizeof(yamlList) );
	term->k = NULL;
	ADD_ELEMENT( sql, sqlLen, yamlList *, term );
	*len = sqlLen;
	return sql;
}


//Option
Option opts[] = {
	{ "-f", "--file",          "Get a file on the command line.", 's' }
 ,{ "-u", "--url",           "Get something from the WWW", 's' }
 ,{ "-y", "--yaml",          "Use the tags from this YAML file", 's' }
 ,{ "-k", "--show-full-key", "Show a full key"  }
 ,{ "-s", "--sql",           "Dump SQL"  }
#if 0
 ,{ "-b", "--backend",       "Choose a backend [mysql, pgsql, mssql]", 's'  }
#else
 ,{ NULL, "--mysql",         "Choose MySQL as the SQL template." }
 ,{ NULL, "--pgsql",         "Choose PostgreSQL as the SQL template." }
 ,{ NULL, "--mssql",         "Choose SQL Server as the SQL template." }
#endif
 ,{ "-h", "--help",           "Show help" }
 ,{ .sentinel = 1 }
};
#if 1
int optKeydump = 0;
#else
//Command loop
struct Cmd {
	const char *cmd;
	int (*exec)( Option *, char *, Passthru *pt );
} Cmds[] = {
	{ "--kill"     , kill_cmd  }
 ,{ "--file"     , file_cmd  }
 ,{ "--start"    , start_cmd }
 ,{ NULL         , NULL      }
};
#endif


int expandbuf ( char **buf, char *src, int *pos ) {
	//allocate additional space
	realloc( *buf, strlen( src ) );
	memset( &buf[ *pos ], 0, strlen( src ) );
	memcpy( &buf[ *pos ], src, strlen( src ) );
	*pos += strlen( src );	
	return 1;
}


int loadyamlfile ( const char *file ) {
	//load a yaml file
	//you can combine supernodes and subnodes (e.g. page.url or elements.model)
	//or you can keep them in some kind of list
	//that's weird, but it does work
	return 0;
}


//Much like moving through any other parser...
int main( int argc, char *argv[] ) {

	//Process some options
	(argc < 2) ? opt_usage(opts, argv[0], "nothing to do.", 0) : opt_eval(opts, argc, argv);

	//Define references
	Table tex, src;
	Table *tt = &src;
	char *b = NULL, *sc = NULL, *yamlFile=NULL; 
	char *ps[] = { NULL, NULL };
	char **p = ps;
	int len=0;

	//Get source somewhere.
	#if 0
	if ( 1 ) {
		//Load from a static buffer in memory somewhere
		block = (char *)yamama;
		len = strlen( block );
		*p = (char *)block;
	}
	else
	#endif
	if ( opt_set( opts, "--file" ) ) {
		if ( !(sc = opt_get( opts, "--file" ).s) ) {
			return err_print( 0, "%s", "No file specified." );	
		}

		if ( !load_page( sc, &b, &len ) ) {
			return err_print( 0, "Error loading page '%s' - %s.", sc, _errbuf );
		}

		*p = (char *)b;
	}
	else if ( opt_set( opts, "--url" ) ) {
		sc = opt_get( opts, "--url" ).s;
		load_www( sc, (unsigned char **)&b, &len );
		*p = (char *)b;
		return err_print( 0, "URLS don't work right now, sorry." );
	}
	#if 0
	//Open a directory?
	else if ( opt_set( opts, "--directory" ) ) {
		srcsrc = opt_get( opts, "--directory" ).s;
		;//load_www( srcsrc, &block, &len );
		//Copy things to things
		*p = (char *)block;
	}
	#endif

#if 1
	optKeydump = opt_set( opts, "--show-full-key" ); 
#else

	if ( opt_set( opts, "--yaml" ) ) {
		if ( !(yamlFile = opt_get(opts, "--yaml").s) ) {
			return err_print( 0, "%s\n", "--yaml flag used, but no YAML file specified." );
		}

	#if 0	
		//Parse the yaml file here...
		if ( !(x = parseYaml( yamlFile )) ) {
			return err_print( 0, "%s\n", "YAML parsing failed." );
		}
	#endif
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

	if ( !*p ) {
		return err_print( 0, "No --file or --url specified.  Nothing to do." );
	}

	//Loop through things
	while ( *p ) {	
		//*p can point to either a block of bytes, or be a file name or url
		//TODO: add a way to differentiate the types here
		//TODO: Define all of those vars up here...

		//Create a hash table out of the expected keys.
		if ( !yamlList_to_table( expected_keys, &tex ) )
			return err_print( 0, "%s", "Failed to create hash list." );

		//Create an HTML hash table
		if ( !parse_html( tt, *p, strlen( *p )) )
			return err_print( 0, "Couldn't parse HTML to hash Table:\n%s", *p );

		//Set some references
		uint8_t fkbuf[2048] = { 0 }, rkbuf[2048]={0};
		int rootNode, jumpNode, activeNode;
		NodeSet *root = &nodes[ 0 ].rootNode, *jump = &nodes[ 0 ].jumpNode; 

		//Find the root node.
		if ( ( rootNode = lt_geti( tt, root->string ) ) == -1 )
			return err_print( 0, "string '%s' not found.\n", root->string );

		//Find the "jump" node.
		if ( !jump->string ) 
			jumpNode = rootNode;
		else {
			if ( ( jumpNode = lt_geti( tt, jump->string ) ) == -1 ) {
				return err_print( 0, "jump string '%s' not found.\n", jump->string );
			}
		}

		//Get parent and do work.
		char *fkey = (char *)lt_get_full_key( tt, jumpNode, fkbuf, sizeof(fkbuf) - 1 );
		char *rkey = (char *)lt_get_full_key( tt, rootNode, rkbuf, sizeof(rkbuf) - 1 );
		SET_INNER_PROC(pp, tt, rootNode, jumpNode, fkey, rkey );

		//Start the extraction process 
		lt_exec( tt, &pp, create_frame );

		//Build individual tables for each.
		for ( int i=0; i<pp.hlistLen; i++ ) {
			//TODO: For our purposes, 5743 is the final node.  Fix this.
			int start = pp.hlist[ i ];
			int end = ( i+1 > pp.hlistLen ) ? 5743 : pp.hlist[ i+1 ]; 

			//Create a table to track occurrences of hashes
			build_ctck( tt, start, end - 1 ); 

			//TODO: Simplify this
			Table *th = malloc( sizeof(Table) );
			lt_init( th, NULL, 7777 );
			ADD_ELEMENT( pp.tlist, pp.tlistLen, Table *, th );
			pp.ctable = pp.tlist[ pp.tlistLen - 1 ];

			//Create a new table
			lt_exec_complex( tt, start, end - 1, &pp, build_individual );
			lt_lock( pp.ctable );

			if ( optKeydump ) {
				lt_kdump( pp.ctable );
			}
		}

		//Destroy the source table. 
		lt_free( tt );

		//Now check that each table has something
		for ( int i=0; i<pp.tlistLen; i++ ) {
			Table *tl = pp.tlist[ i ];

			//TODO: Simplify this, by a large magnitude...
			yamlList *tn = testNodes;
			int baLen = 0, vLen = 0, mtLen = 0;
		#if 0
			char *babuf=NULL, *vbuf=NULL, *fbuf=NULL;
		#else
			char babuf[ 20000 ] = {0};
			char vbuf[ 100000 ] = {0};
			char fbuf[ 120000 ] = {0};
		#endif
			const char fmt[] = "INSERT INTO cw_dealer_inventory ( %s ) VALUES ( %s );";
			yamlList **keys = find_keys_in_mt( tl, tn, &mtLen );

			//Then loop through matched keys and values
			while ( (*keys)->k ) {
			#if 0
				expandbuf( &babuf, &baLen, "%s, ", (*keys)->k );
				expandbuf( &vbuf, &vLen, "\"%s\", ", (*keys)->v );
				keys++;
			#else
				memcpy( &babuf[ baLen ], (*keys)->k, strlen( (*keys)->k ) ); 
				baLen += strlen( (*keys)->k ); 

				//Add actual words
				memcpy( &vbuf[ vLen ], "\"", 1 );
				memcpy( &vbuf[ vLen + 1 ], (*keys)->v, strlen( (*keys)->v ) ); 
				vLen += strlen( (*keys)->v ) + 1;
				memcpy( &vbuf[ vLen ], "\"", 1 );
				vLen ++;

				//Add a comma
				memcpy( &babuf[ baLen ], ", ", 2 );
				memcpy( &vbuf[ vLen ], ", ", 2 );
				keys++, baLen+= 2, vLen+= 2; 
			#endif
			} 

			//Create a SQL creation string, if any hashes were found.
			if ( mtLen > 1 ) {
			#if 0
				babuf[ baLen-2 ]='\0', vbuf[ vLen-2 ]='\0';
				snprintf( fbuf, sizeof(fbuf), fmt, babuf, vbuf );
				free( babuf );
				free( vbuf );
			#else
				baLen -= 2, vLen -= 2;
				babuf[ baLen ] = '\0';
				vbuf[ vLen ] = '\0';
				snprintf( fbuf, sizeof(fbuf), fmt, babuf, vbuf );
			#endif
				fprintf( stdout, "%s\n", fbuf );
			}

			lt_free( tl );
		}

		p++;
	}

	//for ( ... ) free( hashlist );
	//for ( ... ) free( pp.tlist );
	//free( pp.hlist );
	return 0; 
}


