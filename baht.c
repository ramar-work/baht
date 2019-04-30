/* ---------------------------------------------- *
 * baht 
 * =========
 * 
 * Summary
 * -------
 * A web scraper, which assists in building databases.
 * 
 * Usage
 * -----
 * tbd
 *
 * Flags 
 * -----
 * -DDEBUG - Show debugging information 
 * -DHASHTYPE_* - Compile with different hash schemes? 
 * -DSEE_FRAMING * - Compile with the ability to see dumps of hashes
 * 
 * Author
 * ------
 * Antonio R. Collins II (ramar@tubularmodular.com, ramar.collins@gmail.com)
 * Copyright: Tubular Modular Feb 9th, 2019
 *
 * Description
 * -----------
 * Baht is a tool to help index websites.  It can download HTML, parse HTML,  
 * download images, follow redirects and much more.
 *
 * Example Usage is something like:
 * <pre>
 * $ baht -l <file> 
 * </pre>
 *
 * If no file is present, a profile can be built on the command line like:
 * <pre>
 * $ baht [ -f <file>, -u <url> ] -e "{ model = '...', type = '...' }"
 * </pre>
 *
 * Baht is a dumb robot, meaning that it you still have to tell it how to
 * handle whatever is coming back from the server.  The easiest way is to
 * write a Lua file defining which elements are most important on a page.
 * 
 * Baht works by using a string that resolves a certain part of the DOM, 
 * and retrieving that node.  Here is an example of a car page that is
 * indexed by an application I wrote:
 * <pre> 
root = {
 origin= "div^backdrop.div^content_a.div^content_b.center"
,start= "div^backdrop.div^content_a.div^content_b.center.div^thumb_div"
,stop= "div^backdrop.div^content_a.div^content_b.center.br"
}, 

elements = {
 model= "div^thumb_div.table^thumb_table_a.tbkdy.tr.td^thumb_ymm.text"
, year= "$( get first word?  can use Lua or something )"
, make= "$( get middle word )"
	-- More data is listed but you get the idea.
  ...  
}
 * </pre> 
 *
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
#include "vendor/single.h"
#include <gnutls/gnutls.h>
#include <gumbo.h>
//TODO: These headers should no longer exist in a future version
#include <curl/curl.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <luaconf.h>

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
	int (*exec)( char *s, char **d, int *, void *, yamlList ** ); 
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

//User-Agent
const char ua[] = "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/51.0.2704.103 Safari/537.36";

//SQL string
const char fmt[] = "INSERT INTO cw_dealer_inventory ( %s ) VALUES ( %s );";

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

const char sqlfmt[] = 
	"INSERT INTO cw_dealer_inventory ( %s ) VALUES ( %s );"
#if 0
	"INSERT INTO cw_dealer_inventory ( %s ) VALUES ( %s );";
	"INSERT INTO cw_dealer_inventory ( %s ) VALUES ( %s );";
#endif
;

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



//Debugging stuff.
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


void print_ref ( void ) {
	Ref **r = refs;
	for ( int i=0; i<reflen; i++  ) {
		fprintf( stderr, "key: %s, ", r[i]->key );
		fprintf( stderr, "value: %s\n", r[i]->value );
	}
}



//this will crash if you don't use malloc'd strings (I wonder if [] would work)
char *strreplace( char **affect, char *find, char *rep ) {
	char *ff = *affect;
	while ( *ff ) { 	
		//TODO: Support multiple characters.  
		//TODO: Supporting removal is also good, but might need to happen elsewhere... especially since I don't plan on using a copy.
		*ff = ( *ff == *find ) ? *rep : *ff;
		ff++; 
	}
	return *affect;
}
#if 0
char *bacon = "asdfg\none, two, three\nyes";
char *ib = strdup( bacon );
strreplace( &ib, "\n", "," );
fprintf(stderr,"tmp: %s\n", ib );
exit(0);
#endif


//Include the web handling logic
#define _BAHTWEB
#include "web.c"


//Filters
#include "filters.c"
const Filter filterSet[] = {
	{ "asdf",       asdf_filter }
, { "reverse" ,   rev_filter }
, { "download",   download_filter, "source_url" /*","*/ }
, { "follow",     follow_filter, "source_url" }
, { "checksum",   checksum_filter }
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
//int load_page ( const char *file, char **dest, int *destlen, wwwResponse *w ) {
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
	w->redirect_uri = NULL;
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
Table *parse_html ( char *b, int len ) {

	Table *tt = NULL;

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
	#if 0
	lt_dump( tt );
	lt_geti( tt );
	#endif
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
	#if 0
		//This looks like it is building something
		DPRINTF( "fk: %s\n", 
			(char *)lt_get_full_key( pi->srctable, i, (uint8_t *)fkBuf, sizeof(fkBuf) - 1 ) );
		DPRINTF( "%d ? %ld\n", pi->keylen, strlen( (char *)fkBuf ) ); 
	#endif
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
		//(*pi->level) --;
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
		//(*pi->level) ++;
		//fprintf( stderr, "%p\n", &kv->value.v.vtable );
		lt_descend( pi->ctable );
		return 1;
	}

	lt_finalize( pi->ctable );
	return 1;
}


//why would I do this?  because it works here...
int key_from_ht_exec ( LiteKv *kv, int i, void *p ) {
	
	//struct why *w = (struct why *)p;
	yamlSet *w = (yamlSet *)p;
	LiteType kt = kv->key.type, vt = kv->value.type;

	//DPRINTF( "%d %p\n", w->len, y );
	//DPRINTF( "%s,%s\n", lt_typename( kt ), lt_typename( vt ) );

	if ( kt == LITE_TXT && vt == LITE_TXT ) {
		yamlList *tmp = malloc(sizeof(yamlList));
		memset( tmp, 0, sizeof(yamlList));
		tmp->k = kv->key.v.vchar; //add key
		tmp->v = kv->value.v.vchar; //add value 
		ADD_ELEMENT( w->list, w->len, sizeof( yamlList * ), tmp ); 
	}

	if ( kt == LITE_TRM ) {
		return 0;
	}

	return 1;
}


//Assuming I use a hash table, I want to create a simple array of keys and values
yamlList **keys_from_ht ( Table *t ) {
	yamlSet w = { 0, NULL };
	int h=0; 
	//Find the elements key first
	if (( h = lt_geti( t, "elements" )) == -1 ) {
		VPRINTF( "Could not find 'elements' key in %s\n", "$LUA_FILE" );
		return NULL;	
	}

	//Loop from this key if found.
	if ( !lt_exec_complex( t, h, t->count, &w, key_from_ht_exec ) ) {
		VPRINTF( "Something failed...\n" );
	}

	ADD_ELEMENT( w.list, w.len, sizeof( yamlList * ), NULL );
	return w.list;
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
#if 0
const char *string="a,b|c,d|e,f";
yamlList **y = string_to_yamlList( string, ",|", NULL );
print_yamllist( y );
exit( 0 );
#endif


void add_to_yamlList ( yamlList **y, const char *key, const char *value ) {
	//move w.len back one
}


void find_in_yamlList ( yamlList **y, const char *key ) {
	0;
}

void find_in_yamlset ( yamlSet *y, const char *key ) {
	0;
}


void free_yamlset ( yamlSet *y ) {
	0;
}


//
yamlList ** find_keys_in_mt ( Table *t, yamlList **tn, int *len ) {
	yamlList **sql = NULL;
	int h=0, sqlLen = 0;
	const char *delims[] = { "|", "||", "&&", "() =>" };

	while ( (*tn) ) {
		char *k = (*tn)->k, *v = (*tn)->v;
#if 0
		if ( !v ) fprintf( stderr, "No target value for column %s\n", k );	
#else
		if ( v ) {
#endif
			//Use pipes for fancy things
			char tmp[ 1024 ];
			char *fv = NULL;
			char **filters = NULL;
			int flen = 0;

			if ( memchrat( v, '|', strlen(v) ) == -1 )
				fv = v;
			else {
				//find all the filters and set fv
				//die here if a filter does not exist...
				Mem set;
				memset(&set,0,sizeof(Mem));
				int f=0;

				while ( strwalk( &set, v, "|" ) ) {
					//the first one SHOULD always be the hash (and the filters go from there)
					int xlen = 0;
					uint8_t *x = trim( (uint8_t *)&v[ set.pos ], " \t", set.size, &xlen ); 
					//write( 2, "[", 1 );write( 2, x, xlen );write( 2, "]\n", 2 );

					if ( !f++ ) {
						memset( tmp, 0, sizeof(tmp));
						memcpy( tmp, x, xlen );
						fv = tmp;
					}
					else {
						//add each to a thing
						char buf[ 1024 ];
						memset( &buf, 0, sizeof( buf ) );
						memcpy( buf, x, xlen );
						char *filter = strdup( buf );
						//check the filter availability here?
						ADD_ELEMENT( filters, flen, char *, filter ); 
					}
				}

				ADD_ELEMENT( filters, flen, char *, NULL );
			}

			//get the hash	
			if ( (h = lt_geti( t, fv )) > -1 ) {
				yamlList *kv = malloc( sizeof(yamlList) );
				memset( kv, 0, sizeof(yamlList) );
				kv->k = k;

				//Now cycle through each filter
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
					#if 0
						FilterArgs args = { src, &block, &len, NULL, NULL };
						args.ref = getYamlList( yamlList, args... );
						args.list = getYamlList( yamlList, args... );
					#else
					#endif

						//find the filter and run it
						while ( fltrs->name ) {
							if ( strcmp( *filters, fltrs->name ) == 0 && fltrs->exec ) {
								char *block = NULL;
								int len = 0;
							#if 0
								int status = fltrs->exec( &args );
							#else
								int status = fltrs->exec( src, &block, &len, NULL, NULL );
							#endif
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
	term->k = NULL; //term->v = NULL;
	ADD_ELEMENT( sql, sqlLen, yamlList *, term );
	*len = sqlLen;
	return sql;
}


//
int expandbuf ( char **buf, char *src, int *pos ) {
	//allocate additional space
	realloc( *buf, strlen( src ) );
	memset( &buf[ *pos ], 0, strlen( src ) );
	memcpy( &buf[ *pos ], src, strlen( src ) );
	*pos += strlen( src );	
	return 1;
}


//Gumbo to Table 
int lua_to_table (lua_State *L, int index, Table *t ) {
	static int sd;
	lua_pushnil( L );

	while ( lua_next( L, index ) != 0 ) {
		int kt, vt;
#if 0
		//This should pop both keys...
		obprintf( stderr, "%s, %s\n", lua_typename( L, lua_type(L, -2 )), lua_typename( L, lua_type(L, -1 )));

		//Keys
		if (( kt = lua_type( L, -2 )) == LUA_TNUMBER )
			obprintf( stderr, "key: %lld\n", (long long)lua_tointeger( L, -2 ));
		else if ( kt  == LUA_TSTRING )
			obprintf( stderr, "key: %s\n", lua_tostring( L, -2 ));

		//Values
		if (( vt = lua_type( L, -1 )) == LUA_TNUMBER )
			obprintf( stderr, "val: %lld\n", (long long)lua_tointeger( L, -1 ));
		else if ( vt  == LUA_TSTRING )
			obprintf( stderr, "val: %s\n", lua_tostring( L, -1 ));
#endif

		//Get key (remember Lua indices always start at 1.  Hence the minus.
		if (( kt = lua_type( L, -2 )) == LUA_TNUMBER )
			lt_addintkey( t, lua_tointeger( L, -2 ) - 1);
		else if ( kt  == LUA_TSTRING )
			lt_addtextkey( t, (char *)lua_tostring( L, -2 ));

		//Get value
		if (( vt = lua_type( L, -1 )) == LUA_TNUMBER )
			lt_addintvalue( t, lua_tointeger( L, -1 ));
		else if ( vt  == LUA_TSTRING )
			lt_addtextvalue( t, (char *)lua_tostring( L, -1 ));
		else if ( vt == LUA_TTABLE ) {
			lt_descend( t );
			//obprintf( stderr, "Descending because value at %d is table...\n", -1 );
			//lua_loop( L );
			lua_to_table( L, index + 2, t ); 
			lt_ascend( t );
			sd--;
		}

		//obprintf( stderr, "popping last two values...\n" );
		if ( vt == LUA_TNUMBER || vt == LUA_TSTRING ) {
			lt_finalize( t );
		}
		lua_pop(L, 1);
	}

	lt_lock( t );
	return 1;
}


//
int lua_load_file( lua_State *L, const char *file, char **err ) {
	if ( luaL_dofile( L, file ) != 0 ) {
		fprintf( stderr, "Error occurred!\n" );
		//The entire stack needs to be cleared...
		if ( lua_gettop( L ) > 0 ) {
			fprintf( stderr, "%s\n", lua_tostring(L, 1) );
			return ( snprintf( *err, 1023, "%s\n", lua_tostring( L, 1 ) ) ? 0 : 0 );
		}
	}
	return 1;	
}


//
void lua_dumptable ( lua_State *L, int *pos, int *sd ) {
	lua_pushnil( L );
	//fprintf( stderr, "*pos = %d\n", *pos );

	while ( lua_next( L, *pos ) != 0 ) {
		//Fancy printing
		//fprintf( stderr, "%s", &"\t\t\t\t\t\t\t\t\t\t"[ 10 - *sd ] );
		//PRETTY_TABS( *sd );
		fprintf( stderr, "[%3d:%2d] => ", *pos, *sd );

		//Print both left and right side
		for ( int i = -2; i < 0; i++ ) {
			int t = lua_type( L, i );
			const char *type = lua_typename( L, t );
			if ( t == LUA_TSTRING )
				fprintf( stderr, "(%8s) %s", type, lua_tostring( L, i ));
			else if ( t == LUA_TFUNCTION )
				fprintf( stderr, "(%8s) %p", type, (void *)lua_tocfunction( L, i ) );
			else if ( t == LUA_TNUMBER )
				fprintf( stderr, "(%8s) %lld", type, (long long)lua_tointeger( L, i ));
			else if ( t == LUA_TBOOLEAN)
				fprintf( stderr, "(%8s) %s", type, lua_toboolean( L, i ) ? "true" : "false" );
			else if ( t == LUA_TTHREAD )
				fprintf( stderr, "(%8s) %p", type, lua_tothread( L, i ) );
			else if ( t == LUA_TLIGHTUSERDATA || t == LUA_TUSERDATA )
				fprintf( stderr, "(%8s) %p", type, lua_touserdata( L, i ) );
			else if ( t == LUA_TNIL ||  t == LUA_TNONE )
				fprintf( stderr, "(%8s) %p", type, lua_topointer( L, i ) );
			else if ( t == LUA_TTABLE ) {
				fprintf( stderr, "(%8s) %p\n", type, lua_topointer( L, i ) );
				(*sd) ++, (*pos) += 2;
				lua_dumptable( L, pos, sd );
				(*sd) --, (*pos) -= 2;
				//PRETTY_TABS( *sd );
				fprintf( stderr, "}" );
			}
			fprintf( stderr, "%s", ( i == -2 ) ? " -> " : "\n" );
		}

		lua_pop( L, 1 );
	}
	return;
}


//parse_lua
Table *parse_lua ( const char *file ) {
	Table *tt = NULL;
	lua_State *L = NULL;

	//Initialize something for Lua
	if ( !( tt = malloc(sizeof(Table))) ) {
		return NULL;
	}

	//This can fail too
	if ( !lt_init( tt, NULL, 127 ) ) {
		return NULL;
	}

	//Catch Lua environment allocation failures.
	if ( !(L = luaL_newstate()) ) {
		err_set( 0, "%s\n", "Lua failed to initialize." );
		return NULL;
	}

	//And check for load errors.
	luaL_openlibs( L );
	if ( luaL_dofile( L, file ) != 0 ) {
		if ( lua_gettop( L ) > 0 ) {
			err_set( 0, "error happened with Lua: %s\n", lua_tostring( L, 1 ) );
			return NULL;
		}
	}

	//Convert to Table
	lua_to_table( L, 1, tt );
	//lt_dump( t );

	return tt;
}


//Option
Option opts[] = {
	//These ought to always be around
  { "-l", "--load",       "Load a file on the command line.", 's' }
 ,{ "-f", "--file",       "Get a file on the command line.", 's' }
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
 ,{ NULL, "--see-parsed-html",  "Dump the parsed HTML and stop." }
 ,{ NULL, "--see-parsed-lua",   "Dump the parsed Lua file and stop." }
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

#if 0
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




//Much like moving through any other parser...
int main( int argc, char *argv[] ) {
	//Show the version and compilation date
	SHOW_COMPILE_DATE();

	//Process some options
	(argc < 2) ? opt_usage(opts, argv[0], "nothing to do.", 0) : opt_eval(opts, argc, argv);

	//Define references
	char *b=NULL, *sc=NULL, *yamlFile=NULL; 
	char *ps[] = { NULL, NULL };
	char **p = ps;
	int len=0;
	int blen=0;
	char *luaFile = NULL;

	//Define and initailize these large structures.
	wwwResponse www;
	InnerProc pp;
	refs = malloc( 1 );
	memset( &www, 0, sizeof(wwwResponse) ); 
	memset( &pp, 0, sizeof(InnerProc) ); 

#if 0
	int ph=300;
	wwwResponse h;
	h.status = 150;
	filter_ref( "we_good", "hi" );
	filter_ref( "there", &ph );
	filter_ref( "here", &h );
	print_ref ( );
	exit( 0 );
#endif

	//Define all of that mess up here
	uint8_t fkbuf[KEYBUFLEN] = { 0 }, rkbuf[KEYBUFLEN]={0};
	char *pageUrl = NULL;
	Table *tHtml=NULL, *tYaml=NULL;
	yamlList **ky = NULL;
	int kylen=0; 
	int pageType=0;

	yamlList **fy = NULL;


	//Set verbosity
	verbose = opt_set( opts, "--verbose" ); 

	//Set any filter options here
	if ( opt_set( opts, "--filter-opts" ) ) {
		//chop the list and start adding things
		//prefix, suffix, transform, etc can all be thrown in here
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

	//Load a Lua file
	if ( opt_set( opts, "--load" ) ) {
		luaFile = opt_get( opts, "--load" ).s;
		if ( !luaFile ) {
			return err_print( 0, "%s", "No file specified." );	
		}
		
		VPRINTF( "Loading and parsing Lua document at %s\n", luaFile );
		//
		if ( !(tYaml = parse_lua( luaFile )) ) {
			return err_print( 0, "%s", _errbuf  );
		}

	#ifdef SEE_FRAMING
		if ( opt_set( opts, "--see-parsed-lua" ) ) {
			lt_kdump( tYaml );
			died = 0;
			goto destroy;
		}
	#endif
	}
	
	//Load a webpage
	if ( opt_set( opts, "--url" ) ) {
		pageType = PAGE_URL;
		if ( !(pageUrl = opt_get( opts, "--url" ).s) ) {
			return err_print( 0, "No URL specified for dump." ); 
		}
		VPRINTF( "Loading and parsing URL: '%s'\n", pageUrl );
	}

	//Load an HTML file 
	if ( opt_set( opts, "--file" ) ) {
		pageType = PAGE_FILE;
		if ( !(pageUrl = opt_get( opts, "--file" ).s) ) {
			return err_print( 0, "No file specified for dump." ); 
		}
		VPRINTF( "Loading and parsing HTML document at '%s'\n", pageUrl );
	}

	//Now get the hash map keys to map HTML to needed nodes
	if ( tYaml ) {
		//Shouldn't I catch this?
		ky = keys_from_ht( tYaml );

#if 0
		//The hash functions bring back duplicates.
		if ( lt_text( tYaml, "page.url" ) != NULL )
			pageUrl = lt_text( tYaml, "page.url" ), pageType = PAGE_URL;
		else 
#endif
		if ( lt_text( tYaml, "page.file" ) != NULL )
			pageUrl = lt_text( tYaml, "page.file" ), pageType = PAGE_FILE;
		else if ( lt_text( tYaml, "page.command" ) != NULL ) {
			pageUrl = lt_text( tYaml, "page.command" ), pageType = PAGE_CMD;
		}
	}


#if 1
	//Also chop the nodes from here	
	if ( opt_set(opts, "--nodes") || opt_set(opts, "--nodefile") ) { 
		//chop command line args by a comma, or files by : and \n
		const char types[] = { 'c', 'f' };
		const char *args[] = { opt_get(opts,"--nodes").s,  opt_get(opts,"--nodefile").s };
		const char *delims = "=,";// { "=,", ":\n" };
		
		for ( int i=0; i<sizeof(args)/sizeof(char *); i++ ) {
			if ( args[ i ] ) {
				fprintf(stderr,"proc %s: '%s'\n", (types[i] == 'c') ? "arg" : "file", args[ i ] );	
				char *tmp = NULL;
				if ( types[ i ] == 'c' )
					tmp = (char *)args[ i ];
				//TODO: This is ridiculous...
				else {
					const char *file = args[ i ];
					char *p = NULL;
#if 0
					if ( !readFd( file, (uint8_t **)&p, NULL ) ) {
						err_print( 0, "%s", "something bad happened at readFd." );
						goto destroy;
					}
#else
					struct stat sb = {0};
					int fn = 0;
	
					//TODO: None of these should be fatal, but for ease they're going to be
					//Read file to memory
					if ( stat( file, &sb ) == -1 ) {
						err_print( 0, "%s", strerror( errno ) );
						goto destroy;
					}

					//Open the file
					if ( (fn = open( file, O_RDONLY )) == -1 ) {
						err_print( 0, "%s", strerror( errno ) );
						goto destroy;
					}

					//Allocate a buffer big enough to just write to memory.
					if ( !(p = malloc( sb.st_size + 10 )) ) {
						err_print( 0, "%s", strerror( errno ) );
						goto destroy;
					}
	
					memset(p,0,sb.st_size+10);	

					//Read the file into buffer	
					if ( read( fn, p, sb.st_size ) == -1 ) {
						free( p );
						err_print( 0, "%s", strerror( errno ) );
						goto destroy;
					}

					//Close the file?
					if ( close( fn ) == -1 ) {
						free( p );
						err_print( 0, "%s\n", strerror( errno ) );
						goto destroy;
					}
#endif
					tmp = p;
				}
		
			#if 0
				//always dump p when debugging
				fprintf(stderr,"tmp:\n" ); fflush( stderr);
				write( 2, tmp, strlen(tmp) );
				fprintf(stderr,"\n" ); fflush( stderr);
			#endif

				//Now walk through the memory.
				if ( tmp ) {
#if 0
					strreplace( &tmp, "\n", "," );
fprintf(stderr,"tmp: %s\n", tmp );
					//string_to_yamlList( tmp, delims, NULL );
#else
					//turn text blocks into continuous strings (get rid of \n and use ',')
					char *ff = tmp;
					while ( *ff ) { *ff = (*ff == '\n') ? ',' : *ff; ff++; }
					//this could be U/B...
					ff--, *ff = '\0';



					Mem set; //TODO: This must not be / is not thread safe... fix it
					int f=0; 
					yamlList *tp = NULL;
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
#endif
				}
			}
		}

#if 0
		yamlList *tp = malloc( sizeof(yamlList) );
		memset( tp, 0, sizeof(yamlList));
		tp->k = NULL, tp->v = NULL;
#endif
		ADD_ELEMENT( ky, kylen, sizeof( yamlList * ), NULL ); 
#if 0
	print_yamllist( yy );
#endif
	}
#endif

	//Load the page from the web (or from file, but right now from web)
	if ( pageType == PAGE_URL && !load_www( pageUrl, &www ) ) {
		err_print( 0, "Loading page at '%s' failed.\n", pageUrl );
		goto destroy;
	}

	//Load the page from file
	if ( pageType == PAGE_FILE && !load_page( pageUrl, &www ) ) {
		err_print( 0, "Loading page '%s' failed.\n", pageUrl );
		goto destroy;
	}

#if 0
fprintf(stderr,"%s: %d\n", __FILE__, __LINE__ );
print_ref( );
fprintf(stderr,"%s: %d\n", __FILE__, __LINE__ );
Ref *a = filter_ref( "source_url", NULL );
fprintf( stderr, "%s\n", (char *)a->value );
Ref *c = filter_ref( "download_dir", NULL );
fprintf( stderr, "%s\n", c->value );
exit( 0 );
#endif


#if 0
	//Load the page via a command 
	if ( !load_by_exec( pageUrl, &b, &blen, &www ) ) {
		return err_print( 0, "Loading page at '%s' failed.\n", pageUrl );
	}
#endif

	//Create a hash table of all the HTML
	if ( !(tHtml = parse_html( (char *)www.data, www.len )) ) {
		err_print( 0, "Couldn't parse HTML to hash Table" );
		goto destroy;
	}

#ifdef SEE_FRAMING
	//If parsing only, stop here
	if ( opt_set( opts, "--see-parsed-html" ) ) {
		if ( !opt_set( opts, "--file" ) && !opt_set( opts, "--url" ) && !luaFile )
			return err_print( 0, "--see-parsed-html specified but no source given (try --file, --url or --load flags).\n" );
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

	//Load Lua hashes first, if overriding, use those options
	if ( tYaml ) {
		pp.root.fragment = lt_text( tYaml, "root.origin" );
		pp.jump.fragment = lt_text( tYaml, "root.start" );
	}

	if ( opt_set( opts, "--rootstart" ) ) {
		pp.root.fragment = opt_get( opts, "--rootstart" ).s; 
	}

	if ( opt_set( opts, "--jumpstart" ) ) {
		pp.jump.fragment = opt_get( opts, "--jumpstart" ).s; 
	}

	#if 0
	if ( opt_set( opts, "--framestart" ) ) {
		pp.framestart = opt_get( opts, "--framestart" ).s; 
	}

	if ( opt_set( opts, "--framestop" ) ) {
		pp.framestop = opt_get( opts, "--framestop" ).s; 
	}
	#endif

	//Find the root node.
	if ( !pp.root.fragment ) {
		err_print( 0, "no root fragment specified.\n", pp.root.fragment );
		goto destroy;
	}

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

	//Dump the processing structure ahead of processing 
	print_innerproc( &pp );

	//Start the extraction process 
	lt_exec( tHtml, &pp, create_frames );

	//Dump the processing structure after processing 
	print_innerproc( &pp );

	//If hlistLen is zero, we didn't find the frames...
	if ( !pp.hlistLen ) {
		err_print( 1, "%s\n", "No frame nodes found!" );	
		goto destroy;
	}

	//Build individual tables for each.
	for ( int i=0; i<pp.hlistLen; i++ ) {
		int start = pp.hlist[ i ];
		//int end = ( i+1 > pp.hlistLen ) ? tHtml->count : pp.hlist[ i+1 ]; 
		int end = ( i+1 == pp.hlistLen ) ? lt_countall( tHtml ) : pp.hlist[ i+1 ]; 
		//Create a table to track occurrences of hashes
		build_ctck( tHtml, start, end - 1 ); 

		//TODO: Simplify this
		Table *th = malloc( sizeof(Table) );
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

	//Dump the processing structure after processing 
	print_innerproc( &pp );

	//Destroy the source table. 
	lt_free( tHtml );

	//This is supposed to be used to create our data string.
	//We can stream to a number of formats, and should anticipate that.
	//CSV, SQL, even Excel (or at least ODF) ought to be doable.
	//That said, this looks like a job for a Function Pointer...
	for ( int i=0; i<pp.tlistLen; i++ ) {
		Table *tHtmllite = pp.tlist[ i ];

		//TODO: Simplify this, by a large magnitude...
		int baLen = 0, vLen = 0, mtLen = 0;
	#if 0
		char *babuf=NULL, *vbuf=NULL, *fbuf=NULL;
	#else
		char babuf[ 20000 ] = {0};
		char vbuf[ 100000 ] = {0};
		char fbuf[ 120000 ] = {0};
	#endif

		//I need to loop through the "block" and find each hash
		yamlList **keys = find_keys_in_mt( tHtmllite, ky, &mtLen );
	#if 0
		print_yamllist( keys ); exit( 0 );
	#endif

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

	if ( tYaml ) { 
		lt_free( tYaml );
		free( tYaml );
	}
	return died;
}


