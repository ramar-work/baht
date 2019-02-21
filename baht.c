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
#include "vendor/single.h"
#include <gumbo.h>
#define PROG "p"

#define RERR(...) fprintf( stderr, __VA_ARGS__ ) ? 1 : 1 

#ifndef DEBUG
 #define DPRINTF( ... )
#else
 #define DPRINTF( ... ) \
	fprintf( stderr, __VA_ARGS__ )
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

const char *errMessages[] = {
	NULL
};

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


Nodeblock nodes[] = {
/*.content = "files/carri.html"*/
	{
		.rootNode = {.string = "div^backdrop.div^content_a.div^content_b.center"} 
	 //,.jumpNode = { .string = "div^backdrop.div^content_a.div^content_b.center" } 
	 ,.jumpNode = {.string = "div^backdrop.div^content_a.div^content_b.center.div^thumb_div" }

	//,.deathNode = {.string= "div^backdrop.div^content_a.div^content_b.center.br" }
	//,.miniDeathNode = {.string="div^backdrop.div^content_a.div^content_b.center.input#raw_price_13" }
	}
};


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


typedef struct lazy {
	short ind, len;//, *arr;
} oCount; 


//Things we'll use
static int gi=0;
const char *gumbo_types[] = {
	"document"
, "element"
, "text"
, "cdata"
, "comment"
, "whitespace"
, "template"
};


//Hash this somewhere.
typedef struct { char *k, *v; } yamlList;
yamlList expected_keys[] = {
   { "model" }
  ,{ "year" }
  ,{ "make" }
  ,{ "engine" }
  ,{ "mileage" }
  ,{ "transmission" }
  ,{ "drivetrain" }
  ,{ "abs" }
  ,{ "air_conditioning" }
  ,{ "carfax" }
  ,{ "autocheck" }
  ,{ "autotype" }
  ,{ "price" }
  ,{ "fees" }
  ,{ "kbb" }
  ,{ "engine" }
  ,{ "mpg" }
  ,{ "fuel_type" }
  ,{ "interior" }
  ,{ "exterior" }
  ,{ "vin" }
  ,{ NULL }
};



#define INCLUDE_TESTS
#ifdef INCLUDE_TESTS 
//Test nodes
#ifndef SHORTNAME
 #define ROOT "div^thumb_div."
#else
 #define ROOT ""
#endif
yamlList testNodes[] = {
	//not quite...
   {              "model", "div^thumb_div.table^thumb_table_a.tbody.tr.td^thumb_ymm.text" }
#if 1 
  ,{    "individual_page", "div^thumb_div.table^thumb_table_a.tbody.tr.td^font5 thumb_more_info.a^font6 thumb_more_info_link.attrs.href" }
  ,{            "mileage", "div^thumb_div.table^thumb_table_b.tbody.tr.td^thumb_content_right.div^thumb_table_c.table^font7 thumb_info_text.tbody.1.tr.2.td.text" }
  ,{       "transmission", "div^thumb_div.table^thumb_table_b.tbody.tr.td^thumb_content_right.div^thumb_table_c.table^font7 thumb_info_text.tbody.2.tr.1.td.text" }
  ,{              "price", "div^thumb_div.table^thumb_table_b.tbody.tr.td^thumb_content_right.div^thumb_table_c.table^font7 thumb_info_text.tbody.2.tr.2.td.text" }

#else
  ,{               "year", NULL }
  ,{               "make", NULL }
  ,{    "individual_page", "div^thumb_div.table^thumb_table_a.tbody.tr.td^font5 thumb_more_info.a^font6 thumb_more_info_link.attrs.href" }
  ,{            "mileage", "div^thumb_div.table^thumb_table_b.tbody.tr.td^thumb_content_right.div^thumb_table_c.table^font7 thumb_info_text.tbody.tr.td.text" }
  ,{       "transmission", "div^thumb_div.table^thumb_table_b.tbody.tr.td^thumb_content_right.div^thumb_table_c.table^font7 thumb_info_text.tbody.tr.td.text" }
  ,{         "drivetrain", NULL }
  ,{                "abs", NULL }
  ,{   "air_conditioning", NULL }
  ,{             "carfax", NULL }
  ,{          "autocheck", NULL }
  ,{           "autotype", NULL }
  ,{              "price", "div^thumb_div.table^thumb_table_b.tbody.tr.td^thumb_content_right.div^thumb_table_c.table^font7 thumb_info_text.tbody.tr.td.text" }
  ,{               "fees", NULL }
  ,{                "kbb", NULL }
  ,{        "description", "div^thumb_div.table^thumb_table_b.tbody.tr.td^thumb_content_right.div^thumb_table_c.table^font7 thumb_info_text.tbody.tr.td.p^font3 thumb_description_text.text" }
  ,{             "engine", "div^thumb_div.table^thumb_table_b.tbody.tr.td^thumb_content_right.div^thumb_table_c.table^font7 thumb_info_text.tbody.tr.td.text" }
  ,{                "mpg", NULL }
  ,{          "fuel_type", NULL }
  ,{           "interior", NULL }
  ,{           "exterior", NULL }
  ,{                "vin", NULL }
#endif
  ,{ NULL }
};

#if 0
//A lot of this stuff can be cross checked after the fact...
yamlList attrNodes[] = {
   {              "model", "table^thumb_table_a.tbody.tr.td^thumb_ymm.text" }
#if 1 
  ,{               "year", "`get_year`" }
  ,{               "make", "`get_make`" }
  ,{         "drivetrain", "" }
  ,{                "abs", "" }
  ,{   "air_conditioning", "" }
  ,{             "carfax", "" }
  ,{          "autocheck", "" }
  ,{           "autotype", "" }
  ,{               "fees", "" }
  ,{                "kbb", "" }

//These things MIGHT be able to be checked after the fact...
  ,{             "engine", "`get_engine`" } 
  ,{                "mpg", "`get_mpg`" }
  ,{          "fuel_type", "" }
  ,{           "interior", "" }
  ,{           "exterior", "" }
  ,{                "vin", "" }
#endif
  ,{ NULL }
};
#endif

//Test data
const char *html_1 = "<h1>Hello, World!</h1>";
const char html_file[] = "files/carri.html";
#if 0
const char yamama[] = ""
"<html>"
	"<body>"
		"<h2>Snazzy</h2>"
		"<p>The jakes is coming</p>"
		"<div>"
			"<p>The jakes are still chasing me</p>"
			"<div>"
				"<p>Son, why these dudes still chasing me?</p>"
			"</div>"
		"</div>"
	"</body>"
"</html>"
;
#else
const char yamama[] = ""
"<html>"
	"<body>"
	"<div>"
		"<h2>Snazzy</h2>"
		"<p>The jakes is coming</p>"
		"<div>"
			"<p>The jakes are still chasing me</p>"
			"<div>"
				"<p>Son, why these dudes still chasing me?</p>"
				"<p>Bro, why these dudes still chasing me?</p>"
				"<p>Damn, why these dudes still chasing me?</p>"
			"</div>"
		"</div>"
		"<div>"
			"<p>The jakes are still chasing me</p>"
			"<div>"
				"<p>Son, why these dudes still chasing me?</p>"
				"<p>Bro, why these dudes still chasing me?</p>"
				"<p>Damn, why these dudes still chasing me?</p>"
			"</div>"
		"</div>"
	"</div>"
	"</body>"
"</html>"
;
#endif
#endif




//Return the type name of a node
const char *print_gumbo_type ( GumboNodeType t ) {
	return gumbo_types[ t ];
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

	int fn;
	struct stat sb;
	char *err = malloc( 1024 );
	memset( err, 0, 1024 );

	//Read file to memory
	if ( stat( file, &sb ) == -1 ) {
		//fprintf( stderr, "%s: %s\n", PROG, strerror( errno ) );
		snprintf( err, 1023, "%s: %s\n", PROG, strerror( errno ) );
		*dest = err;
		*destlen = strlen( err );
		return 0; 
	}

	//Open the file
	if ( (fn = open( file, O_RDONLY )) == -1 ) {
		//fprintf( stderr, "%s: %s\n", PROG, strerror( errno ) );
		snprintf( err, 1023, "%s: %s\n", PROG, strerror( errno ) );
		*dest = err;
		*destlen = strlen( err );
		return 0; 
	}

	//Allocate a buffer big enough to just write to memory.
	if ( !(*dest = malloc( sb.st_size + 10 )) ) {
		//fprintf( stderr, "%s: %s\n", PROG, strerror( errno ) );
		snprintf( err, 1023, "%s: %s\n", PROG, strerror( errno ) );
		*dest = err;
		*destlen = strlen( err );
		return 0; 
	}

	//Read the file into buffer	
	if ( read( fn, *dest, sb.st_size ) == -1 ) {
		//fprintf( stderr, "%s: %s\n", PROG, strerror( errno ) );
		snprintf( err, 1023, "%s: %s\n", PROG, strerror( errno ) );
		*dest = err;
		*destlen = strlen( err );
		return 0; 
	}

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
		fprintf( stderr, "%s: %s\n", PROG, strerror( errno ) );
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
		DPRINTF( "%06d, %04d, %-10s, %s\n", ++gi, i, print_gumbo_type(n->type) , itemname );

		//Handle what to do with the actual node
		//TODO: Handle GUMBO_NODE_[CDATA,COMMENT,DOCUMENT,WHITESPACE,TEMPLATE]
		if ( n->type != GUMBO_NODE_TEXT && n->type != GUMBO_NODE_ELEMENT ) {
			0;
			//User selected handling can take place here, usually a blank will do
			//twoSided = 0; 
			//lt_addnullvalue( t, itemname );
		}
		else if ( n->type == GUMBO_NODE_TEXT ) {
			//Clone the node text as a crude solution
			int cl=0;
			unsigned char *mm = trim( (unsigned char *)itemname, " \t\r\n", strlen(itemname), &cl );
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
int parse_html ( Table *tt, char *block, int len ) {

	//We can loop through the Gumbo data structure and create a node list that way
	GumboOutput *output = gumbo_parse_with_options( &kGumboDefaultOptions, (char *)block, len ); 
	GumboVector *children = &(output->root)->v.element.children;
	GumboNode *body = find_tag( output->root, GUMBO_TAG_BODY );
write(2,block,len);

	if ( !body ) {
		fprintf( stderr, PROG ": no <body> found!\n" );
		return 0;
	}

	//Allocate a Table
	if ( !lt_init( tt, NULL, 33300 ) ) {
		fprintf( stderr, PROG ": couldn't allocate table!\n" );
		return 0;
	}

	//Parse the document into a Table
	int elements = gumbo_to_table( body, tt );
//lt_dump( tt );
	//lt_geti( tt );
	lt_lock( tt );

	//Free the gumbo struture 
	gumbo_destroy_output( &kGumboDefaultOptions, output );

	return 1;
}


//This is weird, and probably not efficient, but it will have to do for now.
int extract_same ( LiteKv *kv, int i, void *p ) {
	//The parent should be at a specific address and not change during each new invocation
	//The FULL key should also be the same (not like the child couldn't, but its less likely)
	//A data structure can take both of these...
	InnerProc *pi = (InnerProc *)p;
	DPRINTF( "@%5d: %p %c= %p\n", i, kv->parent, ( kv->parent == pi->parent ) ? '=' : '!', pi->parent );

	//Check that parents are the same... 
	if ( kv->parent && (kv->parent == pi->parent) && (i >= pi->jump) ) {
		//...and that the full hash matches the key.
		char fkBuf[ 2048 ] = {0};
		if ( !lt_get_full_key(pi->srctable, i, (uint8_t *)fkBuf, sizeof(fkBuf)-1) ) {
			return 1;
		}

		DPRINTF( "fk: %s\n", (char *)lt_get_full_key( pi->srctable, i, fkBuf, sizeof(fkBuf) - 1 ) );
		DPRINTF( "%d ? %ld\n", pi->keylen, strlen( (char *)fkBuf ) ); 

		//Check strings and see if they match? (this is kind of a crude check)
		if ( pi->keylen == strlen( fkBuf ) && memcmp( pi->key, fkBuf, pi->keylen ) == 0 ) {
			//save hash here and realloc a stretching int buffer...
			ADD_ELEMENT( pi->hlist, pi->hlistLen, int, i );
		} 
	}
	return 1;
}


//At a specific level, find all of the keys that match
//NOTE: This is only run with tables that have two or more children 
int check_same_level ( LiteKv *kv, int i, void *p ) {
	//Loop through until you hit a terminator???, but what do you do?
	if ( kv->key.type == LITE_TRM )
		return 0;
	else if ( kv->key.type == LITE_TXT ) {
	}
getchar();
	return 1;
}


//
static int stCount = 0;
int build_checktable ( LiteKv *kv, int i, void *p ) {
	InnerProc *pi = (InnerProc *)p;
	Table *kt = pi->checktable;	

	//Only save text keys, and increment the bool count if something was there.
	if ( kv->key.type == LITE_TXT && kv->value.type == LITE_TBL ) {

		//Write and manipulate buffers.
		unsigned char fkBuf[ 2048 ] = { 0 }; 
		char *fk = (char *)lt_get_full_key( pi->srctable, i, fkBuf, sizeof(fkBuf) - 1 );

		//TODO: This is pretty ugly, I see why the . is missed, but I assume that 
		//the embedded nul stops us from getting the right count in other cases.
		int jumpBy = memcmp( fk, pi->rkey, strlen( fk ) ) + 2;
		char *sk = &fk[ jumpBy ];
		int found=0;

		//DPRINTF( stderr, "%d: %s; %d\n", i, fk, lt_geti( kt, fk ) );
		if ( fk && (found = lt_geti( kt, sk )) > -1 )  {
			oCount *pp = (oCount *)lt_userdata_at( kt, found );
			pp->len++;
		}
		else {
			//The first time we find it, it's just 0.  
			//lt_addtextkey( kt, kv->key.v.vchar );
			//fprintf( stderr, "writing val from ptr: %d\n", stCount );
			lt_addtextkey( kt, sk );
			oCount *pp = malloc( sizeof( oCount ) );
			pp->ind = 0;
			pp->len = 0;
			lt_addudvalue( kt, pp );
			lt_finalize( kt );
			lt_lock( kt );
		}
	}
	return 1;
}


//...
int check_checktable ( LiteKv *kv, int i, void *p ) {
	if ( kv->key.type == LITE_TXT ) {
		oCount *ph = kv->value.v.vusrdata;
		fprintf( stderr, "%s -> len: %d\n", kv->key.v.vchar, ph->len );
	}
	return 1;
}


//Approximate where something is, by checking the similarity of its parent
typedef struct simp { 
	LiteTable *parent; //Check the parent at this stage 
	Table *ct;         //The check table as we go
	int tlistLen, ind, max;
	Table **tlist;     //Now, we need more than one check table, doing it this way anyway
	Table *top;      //Can't remember if I can trade ptrs or not...
} Simp;



#if 0
//This function extracts keys at the same level, 
//and will return an int array with indexes where dups are found 
int til_parents_match ( LiteKv *kv, int i, void *p ) {
	Simp *sk = (Simp *)p;
	LiteValue vv = kv->value; 
#if 1
	fprintf( stderr, "%s -> %s\n", kv->key.v.vchar, lt_typename( kv->value.type ) );
#endif
	if ( kv->value.type == LITE_TBL && sk->parent == vv.v.vtable.parent )
		return 0;
	else if ( kv->value.type == LITE_TBL ) {
		fprintf( stderr, "%p %c= %p\n", sk->parent, 
			sk->parent == vv.v.vtable.parent ? '=' : '!', vv.v.vtable.parent ); 
		sk->ind++;
	}
	else if ( kv->key.type == LITE_TRM )
		sk->ind--;
	else if ( !sk->ind && kv->key.type == LITE_TXT ) {
		//You can add ints to the structure?
		//You can check the strings (and should here)
		fprintf( stderr, "%s\n", kv->key.v.vchar );
		//do you keep hashing?  do you check against other values? 
		//using a check table once should do it
		fprintf( stderr, "%p\n", sk->ct );

		int at=0;
		//if the key already exists.
		if ((at = lt_geti( sk->ct, kv->key.v.vchar )) == -1) {
fprintf( stderr, "fuck is wrong with this?\n" );
			lt_addtextkey( sk->ct, kv->key.v.vchar );
			lt_addintvalue( sk->ct, 0 );
			lt_finalize( sk->ct );
		} 
		else {
fprintf( stderr, "fuck is wrong with this?\n" );
		}	
		fprintf( stderr, "%s @ %d\n", kv->key.v.vchar, at );
	}	

	return 1;
}
#endif


//An iterator
int undupify ( LiteKv *kv, int i, void *p ) {
	Simp *sk = (Simp *)p;
	LiteValue vv = kv->value;
	fprintf( stderr, "%p, %p, %ld\n", sk->tlist, sk->top, (sk->tlistLen * sizeof(Table *)) );

	if ( kv->key.type == LITE_TRM ) { 
		sk->ind--;	
	}
	else if ( kv->value.type == LITE_TBL ) {
		sk->ind++;
		if ( sk->ind > sk->max ) {
			Table *tn = malloc(sizeof(Table));
			memset( tn, 0, sizeof(Table));
			lt_init( tn, NULL, 64 ); 
			ADD_ELEMENT( sk->tlist, sk->tlistLen, Table *, tn );
			sk->max++;
		}
	}

	if ( sk->ind == 0 || sk->ind > 0 ) {
		sk->top = sk->tlist[ sk->ind ];
	}

	if ( kv->key.type == LITE_TXT ) {
#if 1
		int at = 0;
		if (( at = lt_geti( sk->top, kv->key.v.vchar )) == -1 ) {
			lt_addtextkey( sk->top, kv->key.v.vchar );		
			lt_addintvalue( sk->top, 0 );
			lt_finalize( sk->top );
			lt_lock( sk->top );
		}
		else {

		}
		lt_dump( sk->top );
#endif
	}	

	return 1;
}


//a simpler way MIGHT be this
//this should return/stop when the terminator's parent matches the checking parent 
int build_ctck ( Table *tt, Table *ct, int start ) {
	LiteKv *kv = lt_retkv( tt, start );
	Simp sk = { &kv->parent->value.v.vtable, ct, 0, 0, 0, NULL };

	//Allocate the first entry
	Table *ft = malloc( sizeof(Table) );
	memset(ft,0,sizeof(Table));
	//TODO: This isn't really safe.  looks cool, though ;)
	sk.tlist = malloc( sizeof( Table * ) );
	*sk.tlist = ft;
	sk.tlistLen++;

	//
	lt_exec_complex( tt, start + 1, tt->count, &sk, undupify );

#if 0
	//Dump the Simp structure...
	//...	
	while ( *sk.tlist ) {
		fprintf( stderr, "%p\n", *sk.tlist );
		Table *t = *sk.tlist;
		lt_dump( t ); 
		sk.tlist++;
	}
#endif
	
	return 1;
}



//Pass through and build a smaller subset of tables
int build_individual ( LiteKv *kv, int i, void *p ) {
	//Set refs
	InnerProc *pi = (InnerProc *)p;
	LiteType kt = kv->key.type, vt = kv->value.type;
	fprintf( stderr, "%s, %s\n", lt_typename(kt), lt_typename(vt) );

	//Save key	
	if ( kt == LITE_INT || kt == LITE_FLT ) 
		lt_addintkey( pi->ctable, (kt==LITE_INT) ? kv->key.v.vint : kv->key.v.vfloat );	
	else if ( kt == LITE_BLB )
		lt_addblobkey( pi->ctable, kv->key.v.vblob.blob, kv->key.v.vblob.size );	
	else if ( kt == LITE_TRM ) {
		pi->level --;
		lt_ascend( pi->ctable );
		return 1;
	}
	else if ( kt == LITE_TXT ) {
	#if 1
		//Add regular old keys, maybe...
		if ( vt != LITE_TBL ) {
			lt_addtextkey( pi->ctable, kv->key.v.vchar );	
		}
		//Check for similar keys
		else {
			if ( kv->value.v.vtable.count > 1 ) {
				printf( "echoing tab\n" );
				//if an int is sent, then I'll know what level I'm at...
				//extract_key( pi->srctable, i );
				//exit( 0 );
				getchar();
			}
		}

	#else
		//Define references
		char *sk=NULL, *fk=NULL, fkBuf[ 2048 ]={0};

		//Get the entire key, and check for existence
		sk = fk = (char *)lt_get_full_key( st, i, (uint8_t *)fkBuf, sizeof(fkBuf) - 1 );

		//TODO: Be extremely careful here, probably should add a check for if
		//the value is a table or not
		//TODO: This is harder than it should be
		int jumpBy = memcmp( fk, pi->rkey, strlen( fk ) ) + 2;
		sk = &fk[ jumpBy ];
		oCount *oy = lt_userdata_at( ckt, lt_geti( ckt, sk ) );

		//If a hash was not found, this is "unique" table, so just write.
		if ( !oy || !oy->len ) {
			lt_addtextkey( ct, sk );
		}
		else {
			//If one was found, make the hash "unique"
			//TODO: Add other methods for this
			char mb[ 2048 ]={ 0 }; 
			snprintf( mb, sizeof(mb) - 1, "%s%d", sk, oy->ind );
			sk = mb;
			lt_addtextkey( ct, sk );
			oy->ind ++;
		}
	#endif
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
		fprintf( stderr, "%p\n", &kv->value.v.vtable );
		lt_descend( pi->ctable );
		return 1;
	}
	lt_finalize( pi->ctable );
	return 1;
}


#if 0
		//Hold all of the database records somewhere.
		DBRecord *records = NULL 

		//Loop through each table in the set
		for ( int ti=0; ti < tablelist.len ; ti++ ) {

			//Mark the node
			Table *mt = &(pp.tlist)[ ti ];
			
			//Using a list of strings from earlier, find each node in the NEW table
			for ( int ii = 0; ii < stringsToHash.len; ii++ ) { 

				//Get the node ref
				char *res = ( char * )find_node( mt, stringsToHash[ ii ] );

				//Save the record
				if ( !res ) 
					;
				else {
					*record = { SQL_TYPE, 'column', res };
					record++;
				}

			}

			//It might save memory to go through all the records after each run and reduce memory usage.
			//db_exec( &record... ) ;  //clearly have no idea how this works...
			//free( record );
			//destroy mini table
			//lt_free( mt );
		}
#endif


yamlList ** find_keys_in_mt ( Table *t, yamlList *tn, int *len ) {
	yamlList **sql = NULL;
	int h=0, sqlLen = 0;
	while ( tn->k ) {
		if ( !tn->v )
			;//fprintf( stderr, "No target value for column %s\n", tn->k );	
		else {
	#if 1
			//fprintf( stderr, "hash of %s: %d\n", tn->v, lt_geti( t, tn->v ) );
			if ( (h = lt_geti( t, tn->v )) == -1 )
				0;
			else {
				yamlList *kv = malloc( sizeof(yamlList) );
				kv->k = tn->k;
				kv->v =	lt_text_at( t, h );  
				ADD_ELEMENT( sql, sqlLen, yamlList *, kv ); 
			}
	#else
			//slower, but potentially easier for me (the user)...
			fprintf( stderr, "Finding value for column: %s\n", tn->k );
			int p;
			char buf[ 2048 ] = {0};
			const char *rootShort = "div^thumb_div.";
			memcpy( buf, rootShort, (p = strlen( rootShort )) );
			memcpy( &buf[ p ], tn->v, strlen( tn->v ) );
			fprintf( stderr, "hash of %s: %d\n", buf, lt_geti( t, buf ) );
			lt_geti( t, buf );
	#endif
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
Option opts[] = {
	{ "-f", "--file", "Get a file on the command line.", 's' }
 ,{ "-u", "--url",  "Get something from the WWW", 's' }
 ,{ "-h", "--help", "Show help" }
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

	//Process some options
	//(argc < 2) ? opt_usage(opts, argv[0], "nothing to do.", 0) : opt_eval(opts, argc, argv);

	//Define references
	int len=0;
	Table tex, src;
	Table *tt = &src;
	char *block = NULL;
	char *srcsrc = NULL;
	char *psrc[] = { NULL, NULL };
	char **p = psrc;

	//Get source somewhere.
	#if 1
	if ( 1 ) {
		//Load from a static buffer in memory somewhere
		block = (char *)yamama;
		len = strlen( block );
		*p = (char *)block;
	}
	else
	#endif
	if ( opt_set( opts, "--file" ) ) {
	#if 0
		srcsrc = (char *)html_file;
	#else
		srcsrc = opt_get( opts, "--file" ).s;
	#endif
		if ( !load_page( srcsrc, &block, &len ) ) {
			return RERR( PROG ": Error loading page: '%s'\n.", srcsrc );	
		}
		*p = (char *)block;
	}
	else if ( opt_set( opts, "--url" ) ) {
		srcsrc = opt_get( opts, "--url" ).s;
		load_www( srcsrc, (unsigned char **)&block, &len );
		*p = (char *)block;
		return RERR( PROG ": URLS just don't work right now.  Sorry... :)" );	
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

	//Loop through things
	while ( *p ) {	
		//*p can point to either a block of bytes, or be a file name or url
		//TODO: add a way to differentiate the types here

		//Create a hash table out of the expected keys.
		if ( !yamlList_to_table( expected_keys, &tex ) )
			return RERR( PROG ": Failed to create hash list.\n" );

		//Create an HTML hash table
		if ( !parse_html( tt, block, len ) )
			return RERR( PROG ": Couldn't parse HTML to hash Table.\n" );

lt_dump( tt ); 
//build the block here, in a different way
Table tpk; lt_init(&tpk, NULL, 200 ); 
build_ctck( tt, &tpk, 0 ); 
exit(0);

		//Set some references
		unsigned char fkbuf[ 2048 ] = { 0 };
		unsigned char rkbuf[ 2048 ] = { 0 };
		int rootNode, jumpNode, activeNode;
		NodeSet *root = &nodes[ 0 ].rootNode,  *jump = &nodes[ 0 ].jumpNode; 

		//Find the root node.
		if ( ( rootNode = lt_geti( tt, root->string ) ) == -1 )
			return RERR( PROG ": string '%s' not found.\n", root->string );

		//Find the "jump" node.
		if ( !jump->string ) 
			jumpNode = rootNode;
		else {
			if ( ( jumpNode = lt_geti( tt, jump->string ) ) == -1 ) {
				return RERR( PROG ": jump string '%s' not found.\n", jump->string );
			}
		}

		//Get parent and do work.
		char *fkey = (char *)lt_get_full_key( tt, jumpNode, fkbuf, sizeof(fkbuf) - 1 );
		char *rkey = (char *)lt_get_full_key( tt, rootNode, rkbuf, sizeof(rkbuf) - 1 );
		SET_INNER_PROC(pp, tt, rootNode, jumpNode, fkey, rkey );

		//Start the extraction process 
		lt_exec( tt, &pp, extract_same );
		lt_reset( tt );

		//Build individual tables for each.
		for ( int i=0; i<pp.hlistLen; i++ ) {
			//TODO: For our purposes, 5743 is the final node.  Fix this.
			int start = pp.hlist[ i ];
			int end = ( i+1 > pp.hlistLen ) ? 5743 : pp.hlist[ i+1 ]; 
			
			//TODO: Add 'lt_get_optimal_hash_size( ... )'
			//int oz = lt_get_optimal_hz( int );

			//TODO: Got to figure out what the issue is here, something having 
			//to do with lt_init.  Allocate a table (or five)
			Table *th = malloc( sizeof(Table) );
			lt_init( th, NULL, 7777 );
			ADD_ELEMENT( pp.tlist, pp.tlistLen, Table *, th );
			pp.ctable = pp.tlist[ pp.tlistLen - 1 ];

			//Create a table to track occurrences of hashes
#if 1
			Table tk;
			lt_init( &tk, NULL, 777 );
			pp.checktable = &tk;

			build_ctck( tt, &tk, start ); 
			//lt_exec_complex( tt, start, end - 1, &pp, build_checktable );
#endif
getchar();
exit( 0 );
			//extract_key( tt, NULL, start, end - 1 );
			//lt_exec_complex( tt, start, end - 1, &pp, ww );
			//lt_exec( pp.checktable, NULL, check_checktable );

			//Create a new table
			lt_exec_complex( tt, start, end - 1, &pp, build_individual );
fprintf( stderr, "%p\n", pp.ctable );
lt_dump( pp.ctable );
exit( 0 );
			//lt_lock( pp.ctable );
			//lt_free( pp.checktable );
exit( 0 );
		}

		//Destroy the source table. 
		lt_free( tt );

		//Now check that each table has something
		for ( int i=0; i<pp.tlistLen; i++ ) {
			Table *tl = pp.tlist[ i ];
			//lt_kdump( tl );

			//Hash check on dem bi	
			yamlList *tn = testNodes;
			int baLen = 0, vLen = 0, mtLen = 0;
			char babuf[ 20000 ] = {0};
			char vbuf[ 100000 ] = {0};
			char fbuf[ 120000 ] = {0};
			const char fmt[] = "INSERT INTO cw_dealer_inventory ( %s ) VALUES ( %s );";
			yamlList **keys = find_keys_in_mt( tl, tn, &mtLen );

			//Then loop through matched keys and values
			while ( (*keys)->k ) {
				//TODO: You may have to convert the resultant text to a typesafe value (int, etc)
				//fprintf( stderr, "%s -> %s\n", (*keys)->k, (*keys)->v );
				//Add bind args first 
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
			} 

			//Create a SQL creation string, if any hashes were found.
			if ( mtLen > 1 ) {
				baLen -= 2, vLen -= 2;
				babuf[ baLen ] = '\0';
				vbuf[ vLen ] = '\0';
				snprintf( fbuf, sizeof(fbuf), fmt, babuf, vbuf );
				fprintf( stderr, "%s\n", fbuf );
			}
		}

		p++;
	}

	//for ( ... ) free( hashlist );
	//for ( ... ) free( pp.tlist );
	//free( pp.hlist );
	return 0; 
}


