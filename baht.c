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
 * Baht is a tool to help index websites.  It can download HTML, parse HTML,  * download images, follow redirects and much more.
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

#ifndef VERSION
 #define VERSION "dev"
#endif

#define SHOW_COMPILE_DATE() \
	fprintf( stderr, "baht v" VERSION " compiled: " __DATE__ ", " __TIME__ "\n" )

#include "vendor/single.h"

#include <curl/curl.h>

#include <gnutls/gnutls.h>
#include <gumbo.h>

#ifndef DEBUG
 #define RUN(c) (c)
#else
 #define RUN(c) \
 (c) || (fprintf(stderr, "%s: %d - %s\n", __FILE__, __LINE__, #c)? 0: 0)
#endif

#if 0
#else
 #include <lua.h>
 #include <lualib.h>
 #include <lauxlib.h>
 #include <luaconf.h>
#endif

#define PROG "baht"

#define INCLUDE_TESTS

#define RERR(...) fprintf( stderr, __VA_ARGS__ ) ? 1 : 1 

#define VPRINTF( ... ) ( verbose ) ? fprintf( stderr, __VA_ARGS__ ) : 0; fflush(stderr);

#ifndef DEBUG
 #define DPRINTF( ... )
#else
 #define DPRINTF( ... ) fprintf(stderr,"[%s: %d] ", __func__, __LINE__ ) && fprintf( stderr, __VA_ARGS__ )
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

#define ROOT_NODE "page.root"


/*Data types*/
typedef struct { char *k, *v; } yamlList;


//...
typedef struct {
	LiteKv *parent;

#if 0
	struct quad { char *src, *complete; int node, len; } rootj;
	struct quad { char *src, *complete; int node, len; } jumpj;
#else
	char * rootString;
	char * jumpString;
	char *key;
	char *rkey;
#endif


	int rootNode;
	int jumpNode;
	int jump;
	int level;

	int keylen;
	int hlistLen;
	int *hlist;
	int tlistLen;

	Table *srctable;
	Table **tlist;
	Table *ctable;
	Table *checktable;
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

struct why { int len; yamlList **list; };

typedef struct wwwResponse {
	int status, len;
	uint8_t *data;
	char *redirect_uri;
//char *uri;
} wwwResponse;

typedef struct nodeset {
	int hash;             //Stored hash
	const char *key;      //Key that the value corresponds to
	const char *string;   //String
} NodeSet;


//
typedef struct completedRequest {
	const char *url;
	const char *strippedHttps;
	const char *statusLine;
	const char *path;
	int status;
} cRequest;


enum pagetype {
	PAGE_NONE
, PAGE_URL
, PAGE_FILE
, PAGE_CMD
};


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

//Set verbosity globally until I get the time to restructure this app
int verbose = 0;

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
	fprintf( stderr, "\nINNER PROC\n===========\n" );
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
int load_page ( const char *file, char **dest, int *destlen, wwwResponse *w ) {

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
#if 0
	*dest = p;
	*destlen = sb.st_size;
	w->len = *destlen;
	w->data = (uint8_t *)*dest;
#else
	w->len = sb.st_size;
	w->data = (uint8_t *)p;
#endif
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
	if ( kv->parent && (kv->parent == pi->parent) && (i >= pi->jump) ) {
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


//why would I do this?  because it works here...
int key_from_ht_exec ( LiteKv *kv, int i, void *p ) {
	
	struct why *w = (struct why *)p;
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
	struct why w = { 0, NULL };
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

	#if 0
	//TODO: What is the point of this?
	for ( int i=0; i<w.len; i++ ) {
		yamlList *y = w.list[ i ];
		//fprintf( stderr, "%s = %s\n", y->k, y->v );
	}
	#endif

	ADD_ELEMENT( w.list, w.len, sizeof( yamlList * ), NULL );
	return w.list;
}


//
yamlList ** find_keys_in_mt ( Table *t, yamlList **tn, int *len ) {
//lt_kdump( t );
	yamlList **sql = NULL;
	int h=0, sqlLen = 0;
	while ( (*tn) ) {
		if ( !(*tn)->v )
			;//fprintf( stderr, "No target value for column %s\n", tn->k );	
		else {
			//fprintf( stderr, "hash of %s: %d\n", (*tn)->v, lt_geti( t, (*tn)->v ) );
			if ( (h = lt_geti( t, (*tn)->v )) == -1 )
				0;
			else {
				yamlList *kv = malloc( sizeof(yamlList) );
				kv->k = (*tn)->k;
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


//Write data to some kind of buffer with something
static size_t WriteDataCallbackCurl (void *p, size_t size, size_t nmemb, void *ud) {
	size_t realsize = size * nmemb;
	Sbuffer *sb = (Sbuffer *)ud;
	uint8_t *ptr = realloc( sb->buf, sb->len + realsize + 1 ); 
	if ( !ptr ) {
		fprintf( stderr, "No additional memory to complete request.\n" );
		return 0;
	}
	sb->buf = ptr;
	memcpy( &sb->buf[ sb->len ], p, realsize );
	sb->len += realsize;
	sb->buf[ sb->len ] = 0;
	return realsize;
}


//Send requests to web pages.
int load_www ( const char *p, char **dest, int *destlen, wwwResponse *r ) {
#if 0
	int c=0, sec, port;
	const char *fp = NULL;

	//Checking for secure or not...
	if ( memcmp( "https", p, 5 ) == 0 ) {
		sec = 1;
		port = 443;
		p = &p[8];
		fp = &p[8];
	}
	else if ( memcmp( "http", p, 4 ) == 0 ) {
		sec = 0;
		port = 80;
		p = &p[7];
		fp = &p[7];
	}
	else {
		sec = 0;
		port = 80;
		fp = p;
	}

	//NOTE: Although it is definitely easier to use CURL to handle the rest of the 
	//request, dealing with TLS at C level is more complicated than it probably should be.  
	//Do either an insecure request or a secure request
	if ( !sec ) {
		//Use libCurl
		CURL *curl = NULL;
		CURLcode res;
		curl_global_init( CURL_GLOBAL_DEFAULT );
		curl = curl_easy_init();
		if ( curl ) {
			Sbuffer sb = { 0, malloc(1) }; 	
			curl_easy_setopt( curl, CURLOPT_URL, p );
			curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, WriteDataCallbackCurl );
			curl_easy_setopt( curl, CURLOPT_WRITEDATA, (void *)&sb );
			curl_easy_setopt( curl, CURLOPT_USERAGENT, ua );
			res = curl_easy_perform( curl );
			if ( res != CURLE_OK ) {
				fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
				curl_easy_cleanup( curl );
			}
			//write(2,sb.buf,sb.len);
			curl_global_cleanup();
			*destlen = sb.len;
			*dest = (char *)sb.buf;
		}	
	}
#if 0
	else {
		fprintf( stderr, PROG "Can't handle TLS requests, yet!\n" );
		exit( 0 );
	}
#else
	else {
		//Define all of this useful stuff
		int err, ret, sd, ii, type, len;
		unsigned int status;
		Socket s = { .server   = 0, .proto    = "tcp" };
		gnutls_session_t session;
		memset( &session, 0, sizeof(gnutls_session_t));
		gnutls_datum_t out;
		gnutls_certificate_credentials_t xcred;
		memset( &xcred, 0, sizeof(gnutls_certificate_credentials_t));
		char *desc = NULL;
		uint8_t msg[ 32000 ] = { 0 };
		char buf[ 4096 ] = { 0 }; 
		char GetMsg[2048] = { 0 };
		char rootBuf[ 128 ] = { 0 };
		const char *root = NULL; 
		const char *site = NULL; 
		const char *urlpath = NULL;
		const char *path = NULL;
		int c=0;

		//Initialize a message here
		const char GetMsgFmt[] = 
			"GET %s HTTP/1.1\r\n"
			"Host: %s\r\n"
			"User-Agent: %s\r\n\r\n"
		;
	
		//Chop the URL very simply and crudely.
		if (( c = memchrat( p, '/', strlen( p ) )) == -1 ) {
			path = "/";
			root = p;
		}
		else {	
			memcpy( rootBuf, p, c );
			path = &p[ c ];
			root = rootBuf;
		}


		//Pack a message
		if ( port != 443 )
			len = snprintf( GetMsg, sizeof(GetMsg) - 1, GetMsgFmt, path, root, ua );
		else {
			char hbbuf[ 128 ] = { 0 };
			//snprintf( hbbuf, sizeof( hbbuf ) - 1, "www.%s:%d", root, port );
			snprintf( hbbuf, sizeof( hbbuf ) - 1, "%s:%d", root, port );
			len = snprintf( GetMsg, sizeof(GetMsg) - 1, GetMsgFmt, path, hbbuf, ua );
		}

		//Do socket connect (but after initial connect, I need the file desc)
		if ( RUN( !socket_connect( &s, root, port ) ) ) {
			return err_set( 0, "%s\n", "Couldn't connect to site... " );
		}

		//GnuTLS
		if ( RUN( !gnutls_check_version("3.4.6") ) ) { 
			return err_set( 0, "%s\n", "GnuTLS 3.4.6 or later is required for this example." );	
		}

		//Is this needed?
		if ( RUN( ( err = gnutls_global_init() ) < 0 ) ) {
			return err_set( 0, "%s\n", gnutls_strerror( err ));
		}

		if ( RUN( ( err = gnutls_certificate_allocate_credentials( &xcred ) ) < 0 )) {
			return err_set( 0, "%s\n", gnutls_strerror( err ));
		}

		if ( RUN( ( err = gnutls_certificate_set_x509_system_trust( xcred )) < 0 )) {
			return err_set( 0, "%s\n", gnutls_strerror( err ));
		}
		/*
		//Set client certs this way...
		gnutls_certificate_set_x509_key_file( xcred, "cert.pem", "key.pem" );
		*/	

		//Initialize gnutls and set things up
		if ( RUN( ( err = gnutls_init( &session, GNUTLS_CLIENT ) ) < 0 )) {
			return err_set( 0, "%s\n", gnutls_strerror( err ));
		}

		if ( RUN( ( err = gnutls_server_name_set( session, GNUTLS_NAME_DNS, root, strlen(root)) ) < 0)) {
			return err_set( 0, "%s\n", gnutls_strerror( err ));
		}

		if ( RUN( ( err = gnutls_set_default_priority( session ) ) < 0) ) {
			return err_set( 0, "%s\n", gnutls_strerror( err ));
		}
		
		if ( RUN( ( err = gnutls_credentials_set( session, GNUTLS_CRD_CERTIFICATE, xcred )) <0) ) {
			return err_set( 0, "%s\n", gnutls_strerror( err ));
		}

		gnutls_session_set_verify_cert( session, root, 0 );
		//fprintf( stderr, "s.fd: %d\n", s.fd );
		gnutls_transport_set_int( session, s.fd );
		gnutls_handshake_set_timeout( session, GNUTLS_DEFAULT_HANDSHAKE_TIMEOUT );

		//This is ass ugly...
		do {
			RUN( ret = gnutls_handshake( session ) );
		} while ( ret < 0 && gnutls_error_is_fatal( ret ) == 0 );

		if ( RUN( ret < 0 ) ) {
			fprintf( stderr, "ret: %d\n", ret );
			if ( ret == GNUTLS_E_CERTIFICATE_VERIFICATION_ERROR ) {	
				type = gnutls_certificate_type_get( session );
				status = gnutls_session_get_verify_cert_status( session );
				err = gnutls_certificate_verification_status_print( status, type, &out, 0 );
				fprintf( stdout, "cert verify output: %s\n", out.data );
				gnutls_free( out.data );
				//jump to end, but I don't do go to
			}
			return err_set( 0, "%s\n", "Handshake failed: %s\n", gnutls_strerror( ret ));
		}
		else {
			desc = gnutls_session_get_desc( session );
			fprintf( stdout, "- Session info: %s\n", desc );
			gnutls_free( desc );
		}

		if (RUN( ( err = gnutls_record_send( session, GetMsg, len ) ) < 0 ))
			return err_set( 0, "%s\n", "GnuTLS 3.4.6 or later is required for this example." );	

		//This is a sloppy quick way to handle EAGAIN
		int statter=0;
		while ( statter < 5 ) {
			if ( RUN( (ret = gnutls_record_recv( session, msg, sizeof(msg))) == 0 ) ) {
				fprintf( stderr, " - Peer has closed the TLS Connection\n" );
				//goto end;
				statter = 5;
			}
			else if ( RUN( ret < 0 && gnutls_error_is_fatal( ret ) == 0 ) ) {
				fprintf( stderr, " Warning: %s\n", gnutls_strerror( ret ) );
				//goto end;
				//statter = 5;
			}
			else if ( RUN( ret < 0 ) ) {
				fprintf( stderr, " Error: %s\n", gnutls_strerror( ret ) );
				//goto end;
			}
			else if ( RUN( ret > 0 ) ) {
				//it looks like we MAY have received a packet here.
				statter = 5;
				break;
			}

			fprintf( stderr, "%d: %d\n", statter, ret );
			statter++;
		}

		if ( ret > 0 ) {
			fprintf( stdout, "Recvd %d bytes:\n", ret );
			fflush( stdout );
			write( 1, msg, ret );
			fflush( stdout );
			fputs( "\n", stdout );
			len = ret;

			//I don't know if I can always assume that this is complete.
			//If the status is 200 OK, and Content-Length is there, keep reading
			const char *ok[] = {
				"HTTP/0.9 200 OK"
			, "HTTP/1.0 200 OK"
			, "HTTP/1.1 200 OK"
			, "HTTP/2.0 200 OK"
			, NULL
			};
			const char **lines = ok;

			//This is a bad message
			int stat = 0;
			while ( *lines ) {
				//fprintf(stderr,"%s\n",*lines);
				if ( memcmp( msg, *lines, strlen(*lines) ) == 0 ) {
					stat = 1;
					char statWord[ 10 ] = {0};
					memcpy( statWord, &msg[ 9 ], 3 );
					r->status = atoi( statWord );
					break;
				}
				lines++;
			}

			//This is a bad message?
			if ( stat ) {
				int pos = 0;
				int r = 0;
				int s = 0;
				int len = 0;
				char lenString[ 24 ] = {0};
				uint8_t bmsg[ 327680 ] = {0};
				int pl = 0;
	
				//
				if ( (pos = memstrat( msg, "Content-Length", ret )) == -1 ) {
					fprintf(stderr,"Error out, no length, somethings' wrong...\n" );
				}
				r = memchrat( &msg[ pos ], '\r', ret - pos );	
				s = memchrat( &msg[ pos ], ' ', ret - pos );	
				memcpy( lenString, &msg[ pos + (s+1) ], r - s );
				len = atoi( lenString );
				//r->len = len;

				//Read everything
				while ( len ) {
					//Seems like the message needs to be malloc'd / realloc'd...
					if ( RUN( (ret = gnutls_record_recv( session, &bmsg[ pl ], sizeof(bmsg))) == 0 ) ) {
						fprintf( stderr, " - Peer has closed the TLS Connection\n" );
						//goto end;
					}
					else if ( RUN( ret < 0 && gnutls_error_is_fatal( ret ) == 0 ) ) {
						fprintf( stderr, " Warning: %s\n", gnutls_strerror( ret ) );
						//goto end;
					}
					else if ( RUN( ret < 0 ) ) {
						fprintf( stderr, " Error: %s\n", gnutls_strerror( ret ) );
						//why would a session be invalidated?
						//goto end;
					}
					else if ( RUN( ret > 0 ) ) {
						//it looks like we MAY have received a packet here.
						pl += ret;
						len -= ret;
						//fprintf(stderr,"%d left...\n", len); 
					}
				}
			#if 0
				//Quick dump
				write( 2, msg, p );
			#endif
				//Set pointers 
				*dest = malloc( pl + 1 ); 
				*destlen = pl;
				memcpy( *dest, bmsg, *destlen );
			}
		}

		//At this point, I've got to get smart and write things into a structure.
		#if 0
		var a = {
			status = int (only 200s should go)
		, redirectUri = char (use this if it's a 302 or something, and try again)
		, data = all of the packet data (the data to parse)
		, len = int (length of response)
		}
		#endif

		if (RUN((err = gnutls_bye(session, GNUTLS_SHUT_RDWR)) < 0 )) {
			return err_set( 0, "%s\n",  gnutls_strerror( ret ) );
		}

	}
#endif
end:
	socket_close( &s );
	gnutls_deinit( session );
	gnutls_certificate_free_credentials( xcred );
	gnutls_global_deinit();	
#endif
	return 1;
}


#if 0
int loadwww2 ( const char *p, char **dest, int *destlen ) {
	//Define all of this useful stuff
	int err, ret, sd, ii, type, len;
	unsigned int status;
	Socket s = { .server   = 0, .proto    = "tcp" };
	gnutls_session_t session;
	memset( &session, 0, sizeof(gnutls_session_t));
	gnutls_datum_t out;
	gnutls_certificate_credentials_t xcred;
	memset( &xcred, 0, sizeof(gnutls_certificate_credentials_t));
	char *desc = NULL;
	uint8_t msg[ 32000 ] = { 0 };
	char buf[ 4096 ] = { 0 }; 
	char GetMsg[2048] = { 0 };
	char rootBuf[ 128 ] = { 0 };
	const char *root = NULL; 
	const char *site = NULL; 
	const char *urlpath = NULL;
	const char *path = NULL;
	int c=0, sec, port;

	//A HEAD can be done first to check for any changes, maybe
	//Then do a GET
	const char GetMsgFmt[] = 
		"GET %s HTTP/1.1\r\n"
		"Host: %s\r\n"
		"User-Agent: %s\r\n\r\n"
	;

	//You also need to chop 'http' and 'https' off of the thing
	//if != 0, it's not secure

//fprintf(stderr, "%s\n", p ); exit( 0 );

	//Chop the URL very simply and crudely.
	if (( c = memchrat( p, '/', strlen( p ) )) == -1 ) {
		path = "/";
		root = p;
	}
	else {	
		memcpy( rootBuf, p, c );
		path = &p[ c ];
		root = rootBuf;
	}

	//Do socket connect (but after initial connect, I need the file desc)
	if ( RUN( !socket_connect( &s, root, port ) ) ) {
		return err_set( 0, "%s\n", "Couldn't connect to site... " );
	}

	//Pack a message
	if ( port != 443 )
		len = snprintf( GetMsg, sizeof(GetMsg) - 1, GetMsgFmt, path, root, ua );
	else {
		char hbbuf[ 128 ] = { 0 };
		//snprintf( hbbuf, sizeof( hbbuf ) - 1, "www.%s:%d", root, port );
		snprintf( hbbuf, sizeof( hbbuf ) - 1, "%s:%d", root, port );
		len = snprintf( GetMsg, sizeof(GetMsg) - 1, GetMsgFmt, path, hbbuf, ua );
	}

	if ( !sec ) {
		;
	}
	else {
		//GnuTLS
		if ( RUN( !gnutls_check_version("3.4.6") ) ) { 
			return err_set( 0, "%s\n", "GnuTLS 3.4.6 or later is required for this example." );	
		}

		//Is this needed?
		if ( RUN( ( err = gnutls_global_init() ) < 0 ) ) {
			return err_set( 0, "%s\n", gnutls_strerror( err ));
		}

		if ( RUN( ( err = gnutls_certificate_allocate_credentials( &xcred ) ) < 0 )) {
			return err_set( 0, "%s\n", gnutls_strerror( err ));
		}

		if ( RUN( ( err = gnutls_certificate_set_x509_system_trust( xcred )) < 0 )) {
			return err_set( 0, "%s\n", gnutls_strerror( err ));
		}
		/*
		//Set client certs this way...
		gnutls_certificate_set_x509_key_file( xcred, "cert.pem", "key.pem" );
		*/	

		//Initialize gnutls and set things up
		if ( RUN( ( err = gnutls_init( &session, GNUTLS_CLIENT ) ) < 0 )) {
			return err_set( 0, "%s\n", gnutls_strerror( err ));
		}

		if ( RUN( ( err = gnutls_server_name_set( session, GNUTLS_NAME_DNS, root, strlen(root)) ) < 0)) {
			return err_set( 0, "%s\n", gnutls_strerror( err ));
		}

		if ( RUN( ( err = gnutls_set_default_priority( session ) ) < 0) ) {
			return err_set( 0, "%s\n", gnutls_strerror( err ));
		}
		
		if ( RUN( ( err = gnutls_credentials_set( session, GNUTLS_CRD_CERTIFICATE, xcred )) <0) ) {
			return err_set( 0, "%s\n", gnutls_strerror( err ));
		}

		gnutls_session_set_verify_cert( session, root, 0 );
		//fprintf( stderr, "s.fd: %d\n", s.fd );
		gnutls_transport_set_int( session, s.fd );
		gnutls_handshake_set_timeout( session, GNUTLS_DEFAULT_HANDSHAKE_TIMEOUT );

		//This is ass ugly...
		do {
			RUN( ret = gnutls_handshake( session ) );
		} while ( ret < 0 && gnutls_error_is_fatal( ret ) == 0 );

		if ( RUN( ret < 0 ) ) {
			fprintf( stderr, "ret: %d\n", ret );
			if ( ret == GNUTLS_E_CERTIFICATE_VERIFICATION_ERROR ) {	
				type = gnutls_certificate_type_get( session );
				status = gnutls_session_get_verify_cert_status( session );
				err = gnutls_certificate_verification_status_print( status, type, &out, 0 );
				fprintf( stdout, "cert verify output: %s\n", out.data );
				gnutls_free( out.data );
				//jump to end, but I don't do go to
			}
			return err_set( 0, "%s\n", "Handshake failed: %s\n", gnutls_strerror( ret ));
		}
		else {
			desc = gnutls_session_get_desc( session );
			fprintf( stdout, "- Session info: %s\n", desc );
			gnutls_free( desc );
		}

		if (RUN( ( err = gnutls_record_send( session, GetMsg, len ) ) < 0 ))
			return err_set( 0, "%s\n", "GnuTLS 3.4.6 or later is required for this example." );	

		//This is a sloppy quick way to handle EAGAIN
		int statter=0;
		while ( statter < 5 ) {
			if ( RUN( (ret = gnutls_record_recv( session, msg, sizeof(msg))) == 0 ) ) {
				fprintf( stderr, " - Peer has closed the TLS Connection\n" );
				//goto end;
				statter = 5;
			}
			else if ( RUN( ret < 0 && gnutls_error_is_fatal( ret ) == 0 ) ) {
				fprintf( stderr, " Warning: %s\n", gnutls_strerror( ret ) );
				//goto end;
				//statter = 5;
			}
			else if ( RUN( ret < 0 ) ) {
				fprintf( stderr, " Error: %s\n", gnutls_strerror( ret ) );
				//goto end;
			}
			else if ( RUN( ret > 0 ) ) {
				//it looks like we MAY have received a packet here.
				statter = 5;
				break;
			}

			fprintf( stderr, "%d: %d\n", statter, ret );
			statter++;
		}

		if ( ret > 0 ) {
			fprintf( stdout, "Recvd %d bytes:\n", ret );
			fflush( stdout );
			write( 1, msg, ret );
			fflush( stdout );
			fputs( "\n", stdout );
			len = ret;
		}

		if (RUN((err = gnutls_bye(session, GNUTLS_SHUT_RDWR)) < 0 )) {
			return err_set( 0, "%s\n",  gnutls_strerror( ret ) );
		}
	}

	//Hey, here's our message
	fprintf(stderr, "MESSAGE:\n" );
	fprintf(stderr,"%d\n", len);
	write( 1, msg, len );

end:
	socket_close( &s );
	gnutls_deinit( session );
	gnutls_certificate_free_credentials( xcred );
	gnutls_global_deinit();	
	return 1;
}
#endif


//
const char sqlfmt[] = 
	"INSERT INTO cw_dealer_inventory ( %s ) VALUES ( %s );"
#if 0
	"INSERT INTO cw_dealer_inventory ( %s ) VALUES ( %s );";
	"INSERT INTO cw_dealer_inventory ( %s ) VALUES ( %s );";
#endif
;


//Option
Option opts[] = {
	//Things on things
  { "-l", "--load",          "Load a file on the command line.", 's' }
 ,{ "-k", "--show-full-key", "Show a full key"  }
 ,{ "-f", "--file",          "Get a file on the command line.", 's' }
 ,{ "-u", "--url",           "Check a URL from the WWW", 's' }

	/*Dump options*/
 ,{ "-s", "--rootstart",    "Define the root start node", 's' }
 ,{ "-e", "--jumpstart",      "Define the root end node", 's' }
 ,{ "-o", "--output",        "Send output to this file", 's' }

	/*...*/
#ifdef SEE_FRAMING 
 ,{ NULL, "--see-parsed-html",  "Dump the parsed HTML and stop." }
 ,{ NULL, "--see-parsed-lua",   "Dump the parsed Lua file and stop." }
 ,{ NULL, "--see-frames",       "Show the frames" }
 ,{ NULL, "--step",             "Step through frames" }
#endif
#if 0
 ,{ "-b", "--backend",       "Choose a backend [mysql, pgsql, mssql]", 's'  }
#else
 ,{ NULL, "--mysql",         "Choose MySQL as the SQL template." }
 ,{ NULL, "--pgsql",         "Choose PostgreSQL as the SQL template." }
 ,{ NULL, "--mssql",         "Choose SQL Server as the SQL template." }
#endif
 ,{ "-c", "--check",          "Check what is being loaded by Lua." }
 ,{ "-v", "--verbose",        "Say more than usual" }
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


//Much like moving through any other parser...
int main( int argc, char *argv[] ) {

	//Show the version and compilation date
	SHOW_COMPILE_DATE();

	//Process some options
	(argc < 2) ? opt_usage(opts, argv[0], "nothing to do.", 0) : opt_eval(opts, argc, argv);

	//Define references
	char *b = NULL, *sc = NULL, *yamlFile=NULL; 
	char *ps[] = { NULL, NULL };
	char **p = ps;
	int len=0;
	int blen=0;
	char *luaFile = NULL;
	wwwResponse www;
	InnerProc pp = { 0 };

	//Define all of that mess up here
//	int rootNode, jumpNode, activeNode;
//	NodeSet *root=NULL, *jump=NULL;
	uint8_t fkbuf[2048] = { 0 }, rkbuf[2048]={0};
	char *fkey=NULL, *rkey=NULL, *pageUrl = NULL;
	Table *tHtml=NULL, *tYaml=NULL;
	yamlList **ky = NULL;
	int pageType=0;

	//Set verbosity
	verbose = opt_set( opts, "--verbose" ); 

	//Load a Lua file
	if ( opt_set( opts, "--load" ) ) {
		luaFile = opt_get( opts, "--load" ).s;
		if ( !luaFile ) {
			return err_print( 0, "%s", "No file specified." );	
		}		
	}
	
	//Show a full key
	optKeydump = opt_set( opts, "--show-full-key" ); 

	//If the user is just parsing, dump the parse and stop
	if ( !luaFile /*&& opt_set(opts,"--see-parsed-html")*/ ) {
		char *wd =NULL;

		if ( opt_set( opts, "--url" ) ) {
			pageType = PAGE_URL;
			wd = "URL:";
			if ( !(pageUrl = opt_get( opts, "--url" ).s) ) {
				return err_print( 0, "No URL specified for dump." ); 
			}
		}
		else if ( opt_set( opts, "--file" ) ) {
			pageType = PAGE_FILE;
			wd = "HTML document at";
			if ( !(pageUrl = opt_get( opts, "--file" ).s) ) {
				return err_print( 0, "No file specified for dump." ); 
			}
		}
		else {
			return err_print( 0, "--see-parsed-html specified but no source given (try --file, --url or --load flags).\n" );
		}

		VPRINTF( "Loading and parsing %s '%s'\n", wd, pageUrl );
	}
	else {
		//
		VPRINTF( "Loading and parsing Lua document at %s\n", luaFile );
		if ( !(tYaml = parse_lua( luaFile )) ) {
			return err_print( 0, "%s", _errbuf  );
		}

	#ifdef SEE_FRAMING
		if ( opt_set( opts, "--see-parsed-lua" ) ) {
			lt_kdump( tYaml );
			lt_free( tYaml );
			free( tYaml );
			return 0;
		}
	#endif

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

	//Load the page from the web (or from file, but right now from web)
	if ( pageType == PAGE_URL && !load_www( pageUrl, &b, &blen, &www ) ) {
		return err_print( 0, "Loading page at '%s' failed.\n", pageUrl );
	}

	//Load the page from file
	if ( pageType == PAGE_FILE && !load_page( pageUrl, &b, &blen, &www ) ) {
		return err_print( 0, "Loading page '%s' failed.\n", pageUrl );
	}

#if 0
	//Load the page via a command 
	if ( !load_by_exec( pageUrl, &b, &blen, &www ) ) {
		return err_print( 0, "Loading page at '%s' failed.\n", pageUrl );
	}
#endif

	//Set length and content buffer
	//www.len = blen;
	//www.data = (uint8_t *)b;

	//Create a hash table of all the HTML
	if ( !(tHtml = parse_html( (char *)www.data, www.len )) ) {
		return err_print( 0, "Couldn't parse HTML to hash Table" );
	}

#ifdef SEE_FRAMING
	//If parsing only, stop here
	if ( opt_set( opts, "--see-parsed-html" ) ) {
		lt_kdump( tHtml );

		//During testing, I didn't need to free anything.
		lt_free( tHtml );
		free( www.data );
		free( tHtml );
		return 0;
	}
#endif

	//Load Lua hashes first, if overriding, use those options
	if ( tYaml ) {
		pp.rootString = lt_text( tYaml, "root.origin" ),
		pp.jumpString = lt_text( tYaml, "root.start" );
	}

	if ( opt_set( opts, "--rootstart" ) ) {
		pp.rootString = opt_get( opts, "--rootstart" ).s; 
	}

	if ( opt_set( opts, "--jumpstart" ) ) {
		pp.jumpString = opt_get( opts, "--jumpstart" ).s; 
	}

	//Find the root node.
	if ( ( pp.rootNode = lt_geti( tHtml, pp.rootString ) ) == -1 ) {
		return err_print( 0, "string '%s' not found.\n", pp.rootString );
	}

	//Find the "jump" node.
	if ( !pp.jumpString ) 
		pp.jumpNode = pp.rootNode;
	else {
		if ( (pp.jumpNode = lt_geti( tHtml, pp.jumpString) ) == -1 ) {
			return err_print( 0, "jump string '%s' not found.\n", pp.jumpString );
		}
	}

#if 0
	SET_INNER_PROC( pp, tHtml, rootNode, jumpNode, fkey, rkey );
#else
	//Sorry :(  This is ass-ugly..
	pp.parent = lt_retkv( tHtml, pp.rootNode );
	pp.srctable = tHtml;
	//pp.jump = pp.jumpNode;
	pp.key  = (char *)lt_get_full_key( tHtml, pp.jumpNode, fkbuf, sizeof(fkbuf) - 1 );
	pp.rkey  = (char *)lt_get_full_key( tHtml, pp.rootNode, rkbuf, sizeof(rkbuf) - 1 );
	pp.keylen = strlen( pp.key );
	//We really shouldn't need this, I initialize to zero somewhere at the top of main()
	pp.level = 0;
	pp.tlist = NULL;
	pp.tlistLen = 0;
	pp.hlist = NULL;
	pp.hlistLen = 0;
	pp.checktable = NULL;
#endif

#ifdef DEBUG
	//Dump the processing structure ahead of processing 
	print_innerproc( &pp );
#endif	

	//Start the extraction process 
	lt_exec( tHtml, &pp, create_frames );

#ifdef DEBUG
	//Dump the processing structure after processing 
	print_innerproc( &pp );
	exit(0);
#endif	

	//If hlistLen is zero, we didn't find the frames...
	if ( !pp.hlistLen ) {
		return err_print( 1, "%s\n", "No frame nodes found!" );	
	}

	//Build individual tables for each.
	for ( int i=0; i<pp.hlistLen; i++ ) {
		//TODO: For our purposes, 5743 is the final node.  Fix this.
		int start = pp.hlist[ i ];
		int end = ( i+1 > pp.hlistLen ) ? 5743 : pp.hlist[ i+1 ]; 

		//Create a table to track occurrences of hashes
		build_ctck( tHtml, start, end - 1 ); 

		//TODO: Simplify this
		Table *th = malloc( sizeof(Table) );
		lt_init( th, NULL, 7777 );
		ADD_ELEMENT( pp.tlist, pp.tlistLen, Table *, th );
		pp.ctable = pp.tlist[ pp.tlistLen - 1 ];

		//Create a new table
		lt_exec_complex( tHtml, start, end - 1, &pp, build_individual );
		lt_lock( pp.ctable );

		if ( optKeydump ) {
			lt_kdump( pp.ctable );
		}
	}

	#ifdef DEBUG
	//Dump the processing structure after processing 
	print_innerproc( &pp );
exit( 0 );
	#endif	

	//Destroy the source table. 
	lt_free( tHtml );

	//Now check that each table has something
	for ( int i=0; i<pp.tlistLen; i++ ) {
		Table *tl = pp.tlist[ i ];

		//TODO: Simplify this, by a large magnitude...
		//yamlList *tn = testNodes;
		int baLen = 0, vLen = 0, mtLen = 0;
	#if 0
		char *babuf=NULL, *vbuf=NULL, *fbuf=NULL;
	#else
		char babuf[ 20000 ] = {0};
		char vbuf[ 100000 ] = {0};
		char fbuf[ 120000 ] = {0};
	#endif

		//I need to loop through the "block" and find each hash
		yamlList **keys = find_keys_in_mt( tl, ky, &mtLen );
	#if 0
		while ( (*keys)->k ) {
			fprintf( stderr, "%s - %s\n", (*keys)->k, (*keys)->v );
			keys++;
		}
	#else
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

		lt_free( tl );
	}

	//for ( ... ) free( hashlist );
	//for ( ... ) free( pp.tlist );
	//free( pp.hlist );
	return 0; 
}


