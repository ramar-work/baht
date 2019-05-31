/* ---------------------------------------------- *
baht 
====

Summary
-------
A web scraper, which assists in building databases.


Usage
-----
tbd


Flags 
-----
-DDEBUG - Show debugging information 
-DHASHTYPE_* - Compile with different hash schemes? 
-DSEE_FRAMING * - Compile with the ability to see dumps of hashes


Author
------
Antonio R. Collins II (ramar@tubularmodular.com, ramar.collins@gmail.com)
Copyright: Tubular Modular Feb 9th, 2019
Last Updated: Fri May 31 11:35:59 EDT 2019


Description
-----------
Baht is a tool to help index websites.  It can download HTML, parse HTML,  
download images, follow redirects and much more.

Example Usage is something like:
<pre>
$ baht -l <file> 
</pre>

If no file is present, a profile can be built on the command line like:
<pre>
$ baht [ -f <file>, -u <url> ] -e "{ model = '...', type = '...' }"
</pre>

Baht is a dumb robot, meaning that it you still have to tell it how to
handle whatever is coming back from the server.

Baht works by using a string that resolves a certain part of the DOM, 
and retrieving that node.  Here is an example of a car page that is
indexed by an application I wrote:


 
TODO
----
- exclude classes (or other dom objects).  Some sites use extra styling
	that hurts being able to move through a template.


Nice to Have
-------------
- add the option to read directly from memory (may save time)
- add the option to read from hashes from text file (better than recompiling if
 something goes wrong)

 * ---------------------------------------------- */
#include "vendor/single.h"
#include <gnutls/gnutls.h>
#include <gumbo.h>

#ifndef DEBUG
 #define RUN(c) (c)
#else
 #define RUN(c) \
 (c) || (fprintf(stderr, "%s: %d - %s\n", __FILE__, __LINE__, #c)? 0: 0)
#endif

#define LT_DEVICE 1 

#define PROG "baht"

#define INCLUDE_TESTS

#define KEYBUFLEN 2048

#define ROOT_NODE "page.root"

#define RERR(...) fprintf( stderr, __VA_ARGS__ ) ? 1 : 1 

#define VPRINTF( ... ) ( verbose ) ? fprintf( stderr, __VA_ARGS__ ) : 0; fflush(stderr);

#define SHOW_COMPILE_DATE() \
	fprintf( stderr, "baht v" VERSION " compiled: " __DATE__ ", " __TIME__ "\n" )

#define ADD_ELEMENT( ptr, ptrListSize, eSize, element ) \
	if ( ptr ) \
		ptr = realloc( ptr, sizeof( eSize ) * ( ptrListSize + 1 ) ); \
	else { \
		ptr = malloc( sizeof( eSize ) ); \
	} \
	*(&ptr[ ptrListSize ]) = element; \
	ptrListSize++;

#ifndef VERSION
 #define VERSION "dev"
#endif

#ifndef DEBUG
 #define DPRINTF( ... )
#else
 #define DPRINTF( ... ) fprintf(stderr,"[%s: %d] ", __func__, __LINE__ ) && fprintf( stderr, __VA_ARGS__ )
#endif


//Data types
typedef struct { char *k, *v; } yamlList;

typedef struct { char *fragment, *complete; int node, len; } Quad;

//TODO: How do I document this?
typedef struct {
	Quad root, jump; 
	char *framestart, *framestop; //These are just fragments
	int hlistLen, tlistLen;
	int *hlist;
	LiteKv *parent;
	Table *srctable;
	Table *ctable;
	Table *checktable;
	Table **tlist;
} InnerProc;

typedef struct stretchBuffer {
	int len;
	uint8_t *buf;
} Sbuffer;

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

typedef struct why { 
	int len; 
	yamlList **list; 
	yamlList *ptr;  //when looping this can be used to signify things
} yamlSet;

typedef struct wwwResponse {
	int status, len, clen, ctype, chunked;
	uint8_t *data, *body;
	char *redirect_uri;
} wwwResponse;


typedef struct { 
	int secure, port, fragment; 
	char *addr; 
} wwwType;

typedef struct {
	const char *name;	
	int (*exec)( char *s, char **d, int *, void *, const char ** ); 
	const char *retrkey;	
	int isStatic;
} Filter;

//Void pointer array for the purposes of this thing
typedef struct Ref { const char *key; void *value; } Ref;

typedef struct {
	char *block, **dest;
	int  *destlen;
	Ref **ref;
	yamlList **y;	
} FilterArgs;

enum pagetype {
	PAGE_NONE
, PAGE_URL
, PAGE_FILE
, PAGE_CMD
};

//Global pointer for the purposes of adding to this thing.
Ref **refs = NULL;

//Another global pointer to make it easier to use this
Ref nullref = { NULL, NULL };

//Length of the thing
int reflen = 0;

//An error string buffer (useless)
char _errbuf[2048] = {0};

//A pointer for downloads
const char download_dir[] = ".";

//Set verbosity globally until I get the time to restructure this app
int verbose = 0;

//Global error handler in place of exceptions or a better thought error handler.  
//TODO: Obviously, multi-threading is not going to be very kind to this app.  Change this ASAP. 
int died=0;

//...
int global_dump_html = 0;
char *global_html_dump_file = NULL;

//User-Agent
const char ua[] = "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/51.0.2704.103 Safari/537.36";

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
	died = 1;
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


void print_innerproc( InnerProc *pi ) {
#ifndef DEBUG
	0;
#else
	fprintf( stderr, "\n\n===========\n" );
	fprintf( stderr, "parent:   %p\n", pi->parent );
	fprintf( stderr, "root fragment: %s\n", pi->root.fragment ); 
	fprintf( stderr, "root complete: %s\n", pi->root.complete ); 
	fprintf( stderr, "root node: %d\n", pi->root.node ); 
	fprintf( stderr, "root len: %d\n", pi->root.len ); 
	fprintf( stderr, "jump fragment: %s\n", pi->jump.fragment ); 
	fprintf( stderr, "jump complete: %s\n", pi->jump.complete ); 
	fprintf( stderr, "jump node: %d\n", pi->jump.node ); 
	fprintf( stderr, "jump len: %d\n", pi->jump.len ); 
	fprintf( stderr, "\n" );

	fprintf( stderr, "hlistLen: %d\n", pi->hlistLen );
	fprintf( stderr, "tlistLen: %d\n", pi->tlistLen );
	fprintf( stderr, "hlist:    %p\n", pi->hlist );
	fprintf( stderr, "srctable:   %p\n", pi->srctable );
	fprintf( stderr, "ctable:     %p\n", pi->ctable );
	fprintf( stderr, "checktable: %p\n", pi->checktable );
	fprintf( stderr, "tlist:      %p\n", pi->tlist );
#endif
}


void print_tlist ( Table **tlist, int len, Table *ptr ) {
	//Print
	for ( int i=0; i < len; i++ ) {
		Table *t = tlist[i];
		//fprintf( stderr, "%p%s, ", t, sk->top == t ? "(*)" : "" );
		fprintf( stderr, "%p%s, ", t, (ptr == t) ? "(*)" : "" );
	}
	fprintf( stderr, "\n" );
}


void print_yamlList ( yamlList **yy ) {
	while ( *yy && (*yy)->k ) {
		fprintf( stderr, "%s - %s\n", (*yy)->k, (*yy)->v );
		yy++;
	}
}


void print_quad ( Quad *d ) {
	fprintf( stderr, "fragment: %s\n", d->fragment ); 
	fprintf( stderr, "complete: %s\n", d->complete ); 
	fprintf( stderr, "node:     %d\n", d->node ); 
	fprintf( stderr, "len:      %d\n", d->len ); 
}


void print_ref ( void ) {
	Ref **r = refs;
	for ( int i=0; i<reflen; i++  ) {
		fprintf( stderr, "key: %s, ", r[i]->key );
		fprintf( stderr, "value: %s\n", r[i]->value );
	}
}


//Get/set void pointers for the purpose of these filters
Ref *filter_ref ( const char *udName, void *p ) {
	if ( p ) { 	
		Ref *a = malloc(sizeof(Ref));
		a->key = udName;
		a->value = p;
		ADD_ELEMENT( refs, reflen, Ref *, a );
		return a;
	}
	else {
		Ref **r = refs;
		for ( int i=0; i<reflen; i++  ) {
			if ( strcmp( r[i]->key, udName ) == 0 ) {
				return r[i];
			}
		}
	}
	return &nullref;
}


//this will crash if you don't use malloc'd strings (I wonder if [] would work)
char *strreplace( char **affect, char *find, char *rep ) {
	char *ff = *affect;
	while ( *ff ) { 	
		//TODO: Support multiple characters.  
		//TODO: Supporting removal is also good, but might need to 
		//happen elsewhere... especially since I don't plan on using a copy.
		*ff = ( *ff == *find ) ? *rep : *ff;
		ff++; 
	}
	return *affect;
}


//read files with this
int readFd (const char *filename, uint8_t **b, int *blen) {
	int fd, len;
	struct stat sb;
	memset( &sb, 0, sizeof( struct stat ) );
	
	//TODO: None of these should be fatal, but for ease they're going to be
	//Read file to memory
	if ( stat( filename, &sb ) == -1 ) {
		return err_set( 0, "%s", strerror( errno ) );
	}

	//Open the filename
	if ( (fd = open( filename, O_RDONLY )) == -1 ) {
		return err_set( 0, "%s", strerror( errno ) );
	}

	//Allocate a buffer big enough to just write to memory.
	if ( !(*b = malloc( sb.st_size + 10 )) ) {
		return err_set( 0, "%s", strerror( errno ) );
	}

	memset( *b, 0, sb.st_size+10 );	

	//Read the filename into buffer	
	if ( (len = read( fd, *b, sb.st_size )) == -1 ) {
		free( *b );
		return err_set( 0, "%s", strerror( errno ) );
	}

	//Close the filename?
	if ( close( fd ) == -1 ) {
		free( *b );
		return err_set( 0, "%s\n", strerror( errno ) );
	}

	if ( blen ) {
		*blen = len;
	}
	return 1;
}


//write files with this
int writeFd (const char *filename, uint8_t *b, int blen) {
	int fd = open( filename, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IXUSR );
	//automatically overwrite whatever may be there...
	if ( fd == -1 ) {
		fprintf( stderr, "%s: %s, %d - %s\n", __FILE__,__func__,__LINE__,strerror(errno) );
		return 0;
	}
	//write ought to block, and I'm fine with that...
	if ( write( fd, b, blen ) == -1 ) {
		fprintf( stderr, "%s: %s, %d - %s\n", __FILE__,__func__,__LINE__,strerror(errno) );
		return 0;
	}
	//close really doesn't need to be caught, but let's do it anyway
	if ( close( fd ) == -1 ) {
		fprintf( stderr, "%s: %s, %d - %s\n", __FILE__,__func__,__LINE__,strerror(errno) );
		return 1;
	}
	return 1;
}


//Include the web handling logic
#include "web.c"

//Filters
#include "filters.c"
const Filter filterSet[] = {
	{ "asdf",       asdf_filter }
, { "reverse" ,   rev_filter }
, { "download",   download_filter, "source_url" /*","*/ }
, { "follow",     follow_filter, "source_url" }
, { "checksum",   checksum_filter }
, { "lstr" ,      lstr_filter }
, { "rstr" ,      rstr_filter }
, { "mstr" ,      mstr_filter }
, { "replace" ,   replace_filter }
, { "lcase" ,     lcase_filter }
, { "trim" ,      trim_filter }
, { NULL }
};


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
int load_page ( const char *file, wwwResponse *w ) {
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

	//Close the file?
	if ( close( fn ) == -1 ) {
		free( p );
		return err_set( 0, "%s\n", strerror( errno ) );
	}

	//Set fake w
	w->status = 200;
	w->len = sb.st_size;
	w->data = (uint8_t *)p;
	w->body = (uint8_t *)p;
	w->redirect_uri = NULL;
	return 1;
}


//Return appropriate block
char *retblock ( GumboNode *node ) {
	//Give me some food for thought on what to do
	if ( node->type == GUMBO_NODE_DOCUMENT )
		return "d";
	else if ( node->type == GUMBO_NODE_CDATA )
		return "a";
	else if ( node->type == GUMBO_NODE_COMMENT )
		return "c";
	else if ( node->type == GUMBO_NODE_WHITESPACE )
		return "w";
	else if ( node->type == GUMBO_NODE_TEMPLATE )
		return "t";
	else if ( node->type == GUMBO_NODE_TEXT )
		return (char *)node->v.text.text;
	else if ( node->type == GUMBO_NODE_ELEMENT ) {
		return (char *)gumbo_normalized_tagname( node->v.element.tag );
	}
	return NULL;
}


//Convert a Gumbo "map" to hash table.
//TODO: Add element count and filter for element count
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
		if ( n->type != GUMBO_NODE_TEXT && n->type != GUMBO_NODE_ELEMENT )
			0;
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
Table *parse_html ( char *b, int len ) {
	Table *tt = NULL;

	if ( global_dump_html ) {
		write( 1, b, len );
		//TODO: None of the resources are reclaimed, perhaps return and die?
		exit( 0 ); 
	}

	//We can loop through the Gumbo data structure and create a node list that way
	GumboOutput *output = gumbo_parse_with_options( &kGumboDefaultOptions, (char *)b, len );
	GumboVector *children = &(output->root)->v.element.children;
	GumboNode *body = find_tag( output->root, GUMBO_TAG_BODY );

	//Allocate a table
	if ( !(tt = malloc(sizeof(Table))) ) {
		err_set( 0, "%s", "could not allocate HTML source table!" );
		return NULL;
	}

	//Stop with blank bodies
	if ( !body ) {
		err_set( 0, "%s", "no <body> tag found!" );
		return NULL;
	}

	//Allocate a Table
	if ( !lt_init( tt, NULL, 33300 ) ) {
		err_set( 0, "%s", "couldn't allocate table!" ); 	
		return NULL;
	}

	//Parse the document into a Table
	int elements = gumbo_to_table( body, tt );
	//DEBUG: What does the HTML mapped table look like?
	//lt_dump( tt ); lt_geti( tt );
	lt_lock( tt );

	//Free the gumbo struture 
	gumbo_destroy_output( &kGumboDefaultOptions, output );
	return tt;
}


//Pull the framing stuff
//NOTE: "Frames" can be automatically generated if the markup lends itself to it.
int create_frames ( LiteKv *kv, int i, void *p ) {
	//A data structure can take both of these...
	InnerProc *pi = (InnerProc *)p;
	#if 0
	DPRINTF("@%5d: %p %c= %p\n", i, kv->parent, ( kv->parent == pi->parent ) ? '=' : '!', pi->parent );
	#endif

	//Check that parents are the same... 
	if ( kv->parent && (kv->parent == pi->parent) && (i >= pi->jump.node ) ) {
		//...and that the full hash matches the key.
		char fkBuf[ 2048 ] = {0};
		if ( !lt_get_full_key(pi->srctable, i, (uint8_t *)fkBuf, sizeof(fkBuf)-1) ) {
			return 1;
		}
		//Check strings and see if they match? (this is kind of a crude check)
		if ( pi->jump.len == strlen(fkBuf) && memcmp(pi->jump.complete, fkBuf, pi->jump.len ) == 0 ) {
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


//TODO: Document this more fully.  I wonder if it's really needed...
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
		lt_ascend( pi->ctable );
		return 1;
	}

	//Save value 
	if ( vt == LITE_INT || vt == LITE_FLT ) 
		lt_addintvalue( pi->ctable, kv->value.v.vint );	
	else if ( vt == LITE_BLB )
		lt_addblobvalue( pi->ctable, kv->value.v.vblob.blob, kv->value.v.vblob.size );	
	else if ( vt == LITE_TXT ) {
		if ( kv->value.v.vchar )
			lt_addtextvalue( pi->ctable, kv->value.v.vchar );	
		else {
			//TODO: Is this valid handling?
			lt_addtextvalue( pi->ctable, "" );
		}
	}
	else if ( vt == LITE_TBL ) {
		lt_descend( pi->ctable );
		return 1;
	}

	lt_finalize( pi->ctable );
	return 1;
}


//chop to yamlList, removing all characters in remove...
yamlList ** string_to_yamlList ( const char *tmp, const char *delims, const char *rem ) {

	int f=0; 
	yamlSet w = { 0, NULL };
	yamlList *tp = NULL;
	
	Mem set;
	memset( &set, 0, sizeof(Mem) );	

	if ( !(w.list = malloc(sizeof(yamlList **)) ) ) {
		return NULL;
	}

	while ( strwalk( &set, tmp, delims ) ) {
		char buf[ 4096 ];
		memset( &buf, 0, sizeof(buf) );

		if ( !f++ ) {
			tp = malloc(sizeof(yamlList));
			memset( tp, 0, sizeof(yamlList));
			memcpy( buf, &tmp[ set.pos ], set.size );
			tp->k = strdup( buf );
		}
		else {
			memcpy( buf, &tmp[ set.pos ], set.size );
			tp->v = strdup( buf );
			ADD_ELEMENT( w.list, w.len, sizeof( yamlList * ), tp ); 
			f = 0;
		}
	}

	ADD_ELEMENT( w.list, w.len, sizeof( yamlList * ), NULL );
	return w.list;
}


//Return the value associated with a specific key
char *find_in_yamlList ( yamlList **y, const char *key ) {
	yamlList **yy = y;	
	while ( *yy && (*yy)->k ) {
		//fprintf( stderr, "%s - %s\n", (*yy)->k, (*yy)->v );
		if ( strcmp( key, (*yy)->k ) == 0 ) {
			return (*yy)->v;
		}
		yy++;
	}
	return NULL;
}


//TODO: Document this fully.  It is a necessary function...
yamlList ** find_keys_in_mt ( Table *t, yamlList **tn, int *len ) {
	yamlList **sql = NULL;
	int h=0, sqlLen = 0;
	//TODO: All I support right now are simple pipes.
	//Booleans and lambdas may come if time avails...
	const char *delims[] = { "|", "||", "&&", "() =>" };

	while ( (*tn) ) {
		char *k = (*tn)->k, *v = (*tn)->v;
		if ( v ) {
			char tmp[ 1024 ];
			char *fv = NULL;
			char **filters = NULL;
			int flen = 0;

			//Find all the filters for this particular node
			if ( memchrat( v, '|', strlen(v) ) == -1 )
				fv = v;
			else {
				//TODO: Die here if a filter does not exist... 
				//Or at least let the user know what happened...
				Mem set;
				memset(&set,0,sizeof(Mem));
				int f=0;

				while ( strwalk( &set, v, "|" ) ) {
					//the first one SHOULD always be the hash (and the filters go from there)
					int xlen = 0;
					uint8_t *x = trim( (uint8_t *)&v[ set.pos ], " \t", set.size, &xlen ); 
					//DEBUG: See filters and their arguments...
					//write( 2, "[", 1 );write( 2, x, xlen );write( 2, "]\n", 2 );

					if ( !f++ ) {
						memset( tmp, 0, sizeof(tmp));
						memcpy( tmp, x, xlen );
						fv = tmp;
					}
					else {
						char *filter=NULL, buf[1024];
						memset( &buf, 0, sizeof( buf ) );
						memcpy( buf, x, xlen );
						filter = strdup( buf );
						ADD_ELEMENT( filters, flen, char *, filter ); 
					}
				}
				ADD_ELEMENT( filters, flen, char *, NULL );
			}

			//Check that the node actually exists in the HTML map
			if ( (h = lt_geti( t, fv )) > -1 ) {
				yamlList *kv = malloc( sizeof(yamlList) );
				memset( kv, 0, sizeof(yamlList) );
				kv->k = k;

				//Find that node's filters
				if ( !filters ) 
					kv->v =	lt_text_at( t, h );  
				else {
					char *str = lt_text_at( t, h );
					char *src = malloc( strlen( str ) + 1 );
					memset( src, 0, strlen(str) + 1 );
					memcpy( src, str, strlen( str ));

					//If a filter does not exist, no real reason to quit, but you can
					while ( *filters ) {
						Filter *fltrs = (Filter *)filterSet;
						char *aFilter = *filters;
						char **args = NULL;
						char fbuf[ 64 ];
						memset( &fbuf, 0, sizeof(fbuf) );

						//parse the args here
						int co = memchrocc( *filters, '"', strlen( *filters ) );
						if ( co && (co % 2) == 0 ) {
							//whew...
							char *m = *filters;
							int f = 0, argslen = 0;
							m += memchrat( *filters, '"', strlen( *filters ) );
							memcpy( fbuf, *filters, memchrat( *filters, ' ', strlen(*filters)) ); 
							aFilter = fbuf;
							args = malloc( 1 );

							Mem super;
							memset( &super, 0, sizeof(Mem) );	
							while ( strwalk( &super, m, "\"" ) ) {
								if ( f++ ) {
									char dbuf[ 1024 ];
									memcpy( dbuf, &m[super.pos], super.size );
									ADD_ELEMENT( args, argslen, char *, strdup( dbuf ) );
									memset( &dbuf, 0, sizeof(dbuf) ); 
									f = 0;
								}
							}
							ADD_ELEMENT( args, argslen, char *, NULL );
							//DEBUG: What filters were picked up?
							//char **sa = args;while( *sa ){fprintf(stderr,"arg%d: %s\n",f++,*sa);sa++;}
						}

						//find the filter and run it
						while ( fltrs->name ) {
							if ( strcmp( aFilter, fltrs->name ) == 0 && fltrs->exec ) {
								char *block = NULL;
								int len = 0;
								int status = fltrs->exec( src, &block, &len, NULL, (const char **)args );
								free( src );
								src = malloc( len );	
								memset( src, 0, len );
								memcpy( src, block, len );  
							}
							fltrs++;
						}
						filters++;	
					}

					kv->v =	src;
				}

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


//
int str_addformatted ( char **buf, int *pos, const char *fmt, char *src ) {
	//allocate and copy
	char *a = malloc( strlen(fmt) + strlen(src) );
	memset( a, 0, strlen(fmt) + strlen(src) );
	snprintf( a, strlen(fmt) + strlen(src), fmt, src );

	//allocate additional space in the persistent buffer
	*buf = realloc( *buf, *pos + strlen( a ) );
	memset( &(*buf)[ *pos ], 0, strlen( a ) );
	memcpy( &(*buf)[ *pos ], a, strlen( a ) );
	*pos += strlen( a );	
	free( a );
	return 1;
}


//Option
Option opts[] = {
	//These ought to always be around
  { "-f", "--file",       "Get a file on the command line.", 's' }
 ,{ "-u", "--url",        "Check a URL from the WWW", 's' }
 ,{ "-o", "--output",     "Send output to this file", 's' }
 ,{ "-t", "--tmp",        "Use this as a source folder for any downloads.", 's' }
 ,{ NULL, "--filter-opts","Filter options",'s' }

	//These will probably end up being deprecated.
	//But make sense for scripts and AI
 ,{ "-s", "--rootstart",     "Define the root start node", 's' }
 ,{ "-e", "--jumpstart",     "Define the root end node", 's' }
 ,{ "-n", "--nodes",         "Define node key-value pairs (separated by comma)", 's' }
 ,{ NULL, "--nodefile",      "Define node key-value pairs from a file", 's' }

	//Most of this is for debugging	
#ifdef SEE_FRAMING 
 ,{ "-k", "--show-full-key",    "Show a full key"  }
 ,{ NULL, "--see-raw-html",     "Dump received HTML to file.", 's' }
 ,{ NULL, "--see-parsed-html",  "Dump the parsed HTML and stop." }
 ,{ NULL, "--see-crude-frames", "Show the frames at first pass" }
 ,{ NULL, "--see-nice-frames",  "Show the frames at second pass" }
 ,{ NULL, "--step",             "Step through frames" }
#endif

#if 0
 ,{ "-b", "--backend",       "Choose a backend [mysql, pgsql, mssql]", 's'  }
#else
 ,{ NULL, "--mysql",         "Choose MySQL as the SQL template." }
 ,{ NULL, "--pgsql",         "Choose PostgreSQL as the SQL template." }
 ,{ NULL, "--mssql",         "Choose SQL Server as the SQL template." }
#endif
 ,{ "-v", "--verbose",        "Say more than usual" }
 ,{ "-h", "--help",           "Show help" }
 ,{ .sentinel = 1 }
};


//Much like moving through any other parser...
int main ( int argc, char *argv[] ) {
	//Show the version and compilation date
	SHOW_COMPILE_DATE();

	//Process some options
	(argc < 2) ? opt_usage(opts, argv[0], "nothing to do.", 0) : opt_eval(opts, argc, argv);

	//Define all of that mess up here
	char *pagePtr = NULL;
	Table *tHtml=NULL; 
	int pageType=0;
	yamlList **ky = NULL;

	//TODO: Use dynamic buffers instead... code ought to be cleaner.
	uint8_t fkbuf[KEYBUFLEN] = { 0 }; 
	uint8_t rkbuf[KEYBUFLEN] = { 0 };

	//Define and initailize these large structures.
	wwwResponse www;
	InnerProc pp;
	memset( &www, 0, sizeof(wwwResponse) ); 
	memset( &pp, 0, sizeof(InnerProc) ); 

	//Set your globals here (yes, I said globals...)	
	//TODO: There must be a better way to handle this.
	refs = malloc( 1 );
	verbose = opt_set( opts, "--verbose" ); 

	//Set any filter options here
	if ( opt_set( opts, "--filter-opts" ) ) {
		//Chops a comma-seperated list of filters...
		char *filteropts = opt_get( opts, "--filter-opts" ).s;
		yamlList **xy = string_to_yamlList( filteropts, "=,", NULL );
		print_yamlList( xy ); 
		while ( *xy && (*xy)->k ) {
			char *aa = strdup( (*xy)->v );
			filter_ref( (*xy)->k, (void *)aa );
			xy++;
		}
	}

	//After setting filter options, we need to check for a couple of defaults...
	if ( !opt_set( opts, "--filter-opts" ) || !filter_ref( "download_dir", NULL ) ) {
		filter_ref( "download_dir", (char *)download_dir );
	}

	//Load a webpage
	if ( opt_set( opts, "--url" ) ) {
		pageType = PAGE_URL;
		if ( !(pagePtr = opt_get( opts, "--url" ).s) ) {
			return err_print( 0, "No URL specified for dump." ); 
		}
		VPRINTF( "Loading and parsing URL: '%s'\n", pagePtr );
	}

	//Load an HTML file 
	if ( opt_set( opts, "--file" ) ) {
		pageType = PAGE_FILE;
		if ( !(pagePtr = opt_get( opts, "--file" ).s) ) {
			return err_print( 0, "No file specified for dump." ); 
		}
		VPRINTF( "Loading and parsing HTML document at '%s'\n", pagePtr );
	}

	//Also chop the nodes from here	
	if ( opt_set(opts, "--nodes") || opt_set(opts, "--nodefile") ) { 
		//chop command line args by a comma, or files by : and \n
		const char types[] = { 'c', 'f' };
		const char *args[] = { opt_get(opts,"--nodes").s,  opt_get(opts,"--nodefile").s };
		const char *delims = "=,";// { "=,", ":\n" };
		int kylen=0; 
		
		for ( int i=0; i<sizeof(args)/sizeof(char *); i++ ) {
			if ( args[ i ] ) {
				char *tmp = NULL;
				if ( types[ i ] == 'c' )
					tmp = (char *)args[ i ];
				else {
					const char *file = args[ i ];
					char *p = NULL, *q = NULL;
					int len = 0, qlen = 0;
					if ( !readFd( file, (uint8_t **)&p, &len ) ) {
						err_print( 0, "%s", _errbuf  );
						goto destroy;
					}

					//Allocate a new buffer for raw, usable text
					if ( !(q = malloc( len + 1 )) ) {
						err_print( 0, "%s", "Malloc of new buffer failed...\n" );	
						goto destroy;
					}
				
					//TODO: This is not thread safe, fix this...
					Mem mset;
					memset(&mset,0,sizeof(Mem));
					memset( q, 0, len + 1 );

					//Copy only uncommented lines here...
					while ( strwalk( &mset, p, "\n" ) ) {
						if ( *p == '#' && mset.pos == 0 )
							continue;
						if ( mset.chr == '\n' && p[mset.pos] == '#' )
							continue;
						if ( mset.chr == '\n' && p[mset.pos] == '\n' )
							continue;
						//write( 2, &p[mset.pos], mset.size );write( 2, "\n", 1 );
						//TODO: this is unsafe, mset.size + 1 needs a check
						memcpy( &q[ qlen ], &p[mset.pos], mset.size + 1 );
						qlen += mset.size + 1;
					}
		
					//Destroy the original buffer and set ref to the new one.
					free( p );
					//TODO: Resize the new buffer, because it will most likely always be 
					//smaller than the original...
					//q = realloc( q, newSize ); 
					tmp = q;
				}

				//Now walk through the memory.
				if ( tmp ) {
					//TODO: This needs to be implemented soon.
				#if 0
					strreplace( &tmp, "\n", "," ); fprintf(stderr,"tmp: %s\n", tmp );
					//string_to_yamlList( tmp, delims, NULL );
				#else
					//turn text blocks into continuous strings (get rid of \n and use ',')
					char *ff = tmp;
					while ( *ff ) { *ff = (*ff == '\n') ? ',' : *ff; ff++; }
					//TODO: this could be U/B... watch out
					ff--, *ff = '\0';
				#endif

					int f=0; 
					yamlList *tp = NULL;

					//TODO: strwalk is not thread safe... fix it
					Mem set; 
					memset(&set,0,sizeof(Mem));
					while ( strwalk( &set, tmp, delims ) ) {
						char buf[ 1024 ];
						memset( &buf, 0, 1024 );
						if ( !f++ ) {
							tp = malloc(sizeof(yamlList));
							memset( tp, 0, sizeof(yamlList));
							memcpy( buf, &tmp[ set.pos ], set.size );
							tp->k = strdup( buf );
						}
						else {
							memcpy( buf, &tmp[ set.pos ], set.size );
							tp->v = strdup( buf );
							ADD_ELEMENT( ky, kylen, sizeof( yamlList * ), tp ); 
							f = 0;
						}
					}
				}
			}
		}
		ADD_ELEMENT( ky, kylen, sizeof( yamlList * ), NULL ); 
	}

	//Load the page from the web (or from file, but right now from web)
	if ( pageType == PAGE_URL && !load_www( pagePtr, &www ) ) {
		err_print( 0, "Loading page at '%s' failed.\n", pagePtr );
		goto destroy;
	}

	//Load the page from file
	if ( pageType == PAGE_FILE && !load_page( pagePtr, &www ) ) {
		err_print( 0, "Loading page '%s' failed.\n", pagePtr );
		goto destroy;
	}

#if 0
	//Load the page via a command 
	if ( !load_by_exec( pagePtr, &www ) ) {
		return err_print( 0, "Loading page at '%s' failed.\n", pagePtr );
	}
#endif

	//Parse global HTML dump option and extract file as well
	if ( opt_set( opts, "--see-raw-html" ) ) {
		global_dump_html = 1;
		global_html_dump_file = opt_get( opts, "--see-raw-html" ).s; 
	}

	//Create a hash table of all the HTML
	if ( !(tHtml = parse_html( (char *)www.data, www.len )) ) {
		err_print( 0, "Couldn't parse HTML to hash Table" );
		goto destroy;
	}

	#ifdef SEE_FRAMING
	//If parsing only, stop here
	if ( opt_set( opts, "--see-parsed-html" ) ) {
		if ( !opt_set( opts, "--file" ) && !opt_set( opts, "--url" ) ) 
			return err_print( 0, "--see-parsed-html specified but no source given (try --file or --url flags).\n" );
		lt_kdump( tHtml );
		died = 0;
		goto destroy;
	}
	#endif

	//If there are no nodes, we can't really do anything.
	if ( !ky ) {
		err_print( 1, "No key-value pairs defined!" );
		goto destroy;
	}

	//Likewise, also try to load values from nodefiles
	if ( ky ) { //This check is useless, but keeps the code consistent
		//print_yamlList( ky ); exit(0);
		pp.root.fragment = find_in_yamlList( ky, "root_origin"	);
		pp.jump.fragment = find_in_yamlList( ky, "jump_start"	);
		VPRINTF( "root: %s\njump: %s\n", pp.root.fragment, pp.jump.fragment );
	}

	//Then try checking command line opts for values
	if ( opt_set( opts, "--rootstart" ) )
		pp.root.fragment = opt_get( opts, "--rootstart" ).s; 

	if ( opt_set( opts, "--jumpstart" ) )
		pp.jump.fragment = opt_get( opts, "--jumpstart" ).s; 

	#if 0
	//TODO: Leave this alone for now, but it will be important later.
	if ( opt_set( opts, "--framestart" ) ) {
		pp.framestart = opt_get( opts, "--framestart" ).s; 
	}

	if ( opt_set( opts, "--framestop" ) ) {
		pp.framestop = opt_get( opts, "--framestop" ).s; 
	}
	#endif

	//Check for a string pointing to a root node.
	if ( !pp.root.fragment ) {
		err_print( 0, "no root fragment specified.\n", pp.root.fragment );
		goto destroy;
	}

	//Find the root node.
	if ( ( pp.root.node = lt_geti( tHtml, pp.root.fragment ) ) == -1 ) {
		err_print( 0, "string '%s' not found.\n", pp.root.fragment );
		goto destroy;
	}

	//Find the "jump" node.
	if ( !pp.jump.fragment ) 
		pp.jump.node = pp.root.node;
	else {
		//Assume that the jump fragment is full first... 
		if ( (pp.jump.node = lt_geti( tHtml, pp.jump.fragment) ) == -1 ) {
			int len = 0;	
			char buf[ KEYBUFLEN * 2 ];
			memset( &buf, 0, KEYBUFLEN * 2 );
			memcpy( buf, pp.root.fragment, (len = strlen( pp.root.fragment )) );
			memcpy( &buf[ len++ ], ".", 1 );
			memcpy( &buf[ len ], pp.jump.fragment, strlen( pp.jump.fragment ) );
			len += strlen( pp.jump.fragment );

			//...test the root and jump fragments together otherwise.
			if ( (pp.jump.node = lt_geti( tHtml, buf ) ) == -1 ) {
				err_print( 1, "Couldn't find frame start strings '%s' or '%s'.\n", pp.jump.fragment, buf );
				goto destroy;
			}
		}
	}

	//Set the rest of the InnerProc members
	pp.parent = lt_retkv( tHtml, pp.root.node );
	pp.srctable = tHtml;
	pp.jump.complete = (char *)lt_get_full_key( tHtml, pp.jump.node, fkbuf, sizeof(fkbuf) - 1 );
	pp.root.complete = (char *)lt_get_full_key( tHtml, pp.root.node, rkbuf, sizeof(rkbuf) - 1 );
	pp.jump.len = strlen( pp.jump.complete );
	pp.root.len = strlen( pp.root.complete );

	//DEBUG: Dump the processing structure ahead of processing 
	//print_innerproc( &pp );

	//Start the extraction process 
	lt_exec( tHtml, &pp, create_frames );

	//DEBUG: Dump the processing structure after processing 
	//print_innerproc( &pp );

	//If hlistLen is zero, we didn't find the frames...
	if ( !pp.hlistLen ) {
		err_print( 1, "%s\n", "No frame nodes found!" );	
		goto destroy;
	}

	//Build individual tables for each.
	for ( int i=0; i<pp.hlistLen; i++ ) {
		int start = pp.hlist[ i ];
		int end = ( i+1 == pp.hlistLen ) ? -1 : pp.hlist[ i+1 ]; 

		//TODO: This is a source of great frustration.  Without explicitly
		//specifying where the code should stop, I am at a loss as how to
		//limit the set of nodes... 
		//I need something like:
		//int end = ( i+1 > pp.hlistLen ) ? endOfFrames( ... ) : pp.hlist[ i+1 ]; 
		if ( end == -1 ) break;

		//Create a table to track occurrences of hashes
		build_ctck( tHtml, start, end - 1 ); 

		//TODO: Simplify this
		Table *th = malloc( sizeof(Table) );
		memset(th,0,sizeof(Table));
		lt_init( th, NULL, 7777 );
		ADD_ELEMENT( pp.tlist, pp.tlistLen, Table *, th );
		pp.ctable = pp.tlist[ pp.tlistLen - 1 ];

		//Create a new table
		//TODO: Why -3?  
		lt_exec_complex( tHtml, start, end - 3, &pp, build_individual );
		lt_lock( pp.ctable );

		#ifdef SEE_FRAMING
		//Dump the "crude" frames...
		if ( opt_set( opts, "--see-crude-frames" ) ) {
			lt_kdump( pp.ctable );
			if ( opt_set( opts, "--step" ) ) {
				fprintf( stderr, "Press Enter to continue..." );
				getchar();
			}
		}
		#endif
	}

	//DEBUG: Dump the processing structure after processing 
	//print_innerproc( &pp );

	//Destroy the source table. 
	lt_free( tHtml );

	//This is supposed to be used to create our data string.
	//TODO: Support more formats besides SQL, such as CSV, Excel, ODF...
	for ( int i=0; i<pp.tlistLen; i++ ) {
		Table *tHtmllite = pp.tlist[ i ];
		int baLen = 0, vLen = 0, mtLen = 0;
		char *babuf=malloc(1), *vbuf=malloc(1);

		//I need to loop through the "block" and find each hash
		yamlList **keys = find_keys_in_mt( tHtmllite, ky, &mtLen );
		//DEBUG: See these values.
		//print_yamlList( keys ); exit( 0 );

		//Then loop through matched keys and values
		while ( (*keys)->k ) {
			if ( (*keys)->v ) {
				str_addformatted( &babuf, &baLen, "%s, ", (*keys)->k );
				str_addformatted( &vbuf, &vLen, "\"%s\", ", (*keys)->v );
			} 
			keys++; 
		}
		
		//Terminate and let the user know there was a problem.
		//Create a format string, if hashes were found.  Terminate if not.
		if ( baLen < 2 || vLen < 2 || mtLen == 1 ) {
			err_print( 1, "No nodes found in source." );
			goto destroy;
		}
		else {
			const char fmt[] = "INSERT INTO cw_dealer_inventory ( %s ) VALUES ( %s );\n";
			babuf[ baLen-2 ] = '\0', vbuf[ vLen-2 ] = '\0';
			fprintf( stdout, fmt, babuf, vbuf );
		}

		free(babuf); 
		free(vbuf);
		lt_free( tHtmllite );
	}

destroy:
	//for ( ... ) free( hashlist );
	//for ( ... ) free( pp.tlist );
	//free( pp.hlist );
	//Print error, then destroy everything?
	//throw err
	//free things...
	if ( www.data ) {
		free( www.data );
	}

	if ( tHtml ) {
		lt_free( tHtml );
		free( tHtml );
	}

	return died;
}
