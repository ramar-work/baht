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
 * 
 * Author
 * ------
 * Antonio R. Collins II (ramar@tubularmodular.com, ramar.collins@gmail.com)
 * Copyright: Tubular Modular Feb 9th, 2019
 * 
 * TODO
 * ----
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

#define SET_INNER_PROC(p, t, rn, jn, fk) \
	InnerProc p = { \
		.parent   = lt_retkv( t, rn ) \
	 ,.srctable = t \
	 ,.jump = jn \
	 ,.key  = fk \
	 ,.keylen = strlen( fk ) \
	 ,.tlist = NULL \
	 ,.tlistLen = 0 \
	 ,.hlist = NULL \
	 ,.hlistLen = 0 \
	}

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
	char *key;
	int keylen;
	int hlistLen;
	int *hlist;
	int tlistLen;
	Table **tlist;
	Table *ctable;
} InnerProc;



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
   {              "model", "table^thumb_table_a.tbody.tr.td^thumb_ymm.text" }
#if 1 
  ,{               "year", NULL }
  ,{               "make", NULL }
  ,{    "individual_page", "table^thumb_table_a.tbody.tr.td^font5 thumb_more_info.a^font6 thumb_more_info_link.attrs.href" }
  ,{            "mileage", "table^thumb_table_b.tbody.tr.td^thumb_content_right.div^thumb_table_c.table^font7 thumb_info_text.tbody.tr.td.text" }
  ,{       "transmission", "table^thumb_table_b.tbody.tr.td^thumb_content_right.div^thumb_table_c.table^font7 thumb_info_text.tbody.tr.td.text" }
  ,{         "drivetrain", NULL }
  ,{                "abs", NULL }
  ,{   "air_conditioning", NULL }
  ,{             "carfax", NULL }
  ,{          "autocheck", NULL }
  ,{           "autotype", NULL }
  ,{              "price", "table^thumb_table_b.tbody.tr.td^thumb_content_right.div^thumb_table_c.table^font7 thumb_info_text.tbody.tr.td.text" }
  ,{               "fees", NULL }
  ,{                "kbb", NULL }
  ,{        "description", "table^thumb_table_b.tbody.tr.td^thumb_content_right.div^thumb_table_c.table^font7 thumb_info_text.tbody.tr.td.p^font3 thumb_description_text.text" }
  ,{             "engine", "table^thumb_table_b.tbody.tr.td^thumb_content_right.div^thumb_table_c.table^font7 thumb_info_text.tbody.tr.td.text" }
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
int load_page ( const char *file, unsigned char **dest, int *destlen ) {

	int fn;
	struct stat sb;

	//Read file to memory
	if ( stat( file, &sb ) == -1 ) {
		fprintf( stderr, "%s: %s\n", PROG, strerror( errno ) );
		return 0; 
	}

	//Open the file
	if ( (fn = open( file, O_RDONLY )) == -1 ) {
		fprintf( stderr, "%s: %s\n", PROG, strerror( errno ) );
		return 0; 
	}

	//Allocate a buffer big enough to just write to memory.
	if ( !(*dest = malloc( sb.st_size + 10 )) ) {
		fprintf( stderr, "%s: %s\n", PROG, strerror( errno ) );
		return 0; 
	}

	//Read the file into buffer	
	if ( read( fn, *dest, sb.st_size ) == -1 ) {
		fprintf( stderr, "%s: %s\n", PROG, strerror( errno ) );
		return 0; 
	}

	*destlen = sb.st_size;
	return 1;
}


//Grab a page and write to buffer
int load_www ( const char *address, unsigned char *dest, int *destlen ) {

	int fn;
	struct stat sb;

	//Can make a raw socket connection, but should use cURL for it...
	//....

#if 0
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

			//This newest change will add an Id or class to a hash
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

		#if 0
			if ( !gv->length ) {
				lt_addtextkey( tt, "text" );	
				lt_addtextvalue( tt, "(nothing)" );	
				lt_finalize( tt );
			}
			else {
		#endif
			if ( gv->length ) {
				gumbo_to_table( n, tt );
			}

			lt_ascend( tt );
		}
	}

	lt_lock( tt );
	return 1;
}


//Put the raw HTML into a hash table 
int parse_html ( Table *tt, unsigned char *block, int len ) {

	//We can loop through the Gumbo data structure and create a node list that way
	GumboOutput *output = gumbo_parse_with_options( &kGumboDefaultOptions, (char *)block, len ); 
	GumboVector *children = &(output->root)->v.element.children;
	GumboNode *body = find_tag( output->root, GUMBO_TAG_BODY );

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


//Pass through and build a smaller subset of tables
int build_individual ( LiteKv *kv, int i, void *p ) {
	//Deref and get the first table in the list.
	InnerProc *pi = (InnerProc *)p;
	//Table *ct = *pi->tlist;
	Table *ct = pi->ctable;

	//Get type of value, save accordingly.
	LiteType kt = kv->key.type;
	LiteType vt = kv->value.type;

	//Save key	
	if ( kt == LITE_INT || kt == LITE_FLT ) 
		lt_addintkey( ct, (kt==LITE_INT) ? kv->key.v.vint : kv->key.v.vfloat );	
	else if ( kt == LITE_BLB )
		lt_addblobkey( ct, kv->key.v.vblob.blob, kv->key.v.vblob.size );	
	else if ( kt == LITE_TXT )
		lt_addtextkey( ct, kv->key.v.vchar );	
	else if ( kt == LITE_TRM ) {
		lt_ascend( ct );
		return 1;
	}

	//Save value 
	if ( vt == LITE_INT || vt == LITE_FLT ) 
		lt_addintvalue( ct, kv->value.v.vint );	
	else if ( vt == LITE_BLB)
		lt_addblobvalue( ct, kv->value.v.vblob.blob, kv->value.v.vblob.size );	
	else if ( vt == LITE_TXT )
		lt_addtextvalue( ct, kv->value.v.vchar );	
	else if ( vt == LITE_TBL ) {
		lt_descend( ct );
		return 1;
	}
	lt_finalize( ct );
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



//Much like moving through any other parser...
int main() {

	//Define references
	int len = 0;
	unsigned char *block = NULL;
	Table tex, src; 
	Table *tt = &src;

	//Create a hash table out of the expected keys.
	if ( !yamlList_to_table( expected_keys, &tex ) ) {
		return RERR( PROG ": Failed to create hash list.\n" );
	}

	//Get source somewhere.
	if ( 1 )
		load_page( html_file, &block, &len );
	else if ( 0 )
		;//load_www( "http://	
	#ifdef DEBUG
	else if ( 0 ) {
		//Load from a static buffer in memory somewhere
		block = (char *)yamama;
		len = strlen( block );
	}
	#endif

	//Create an HTML hash table
	if ( !parse_html( tt, block, len ) ) {
		return RERR( PROG ": Couldn't parse HTML to hash Table.\n" );
	}

	//Set some references
	unsigned char fkbuf[ 2048 ] = { 0 };
	int rootNode, jumpNode, activeNode;
	NodeSet *root = &nodes[ 0 ].rootNode,  *jump = &nodes[ 0 ].jumpNode; 

	//Find the root node.
	if ( ( rootNode = lt_geti( tt, root->string ) ) == -1 ) {
		return RERR( PROG ": string '%s' not found.\n", root->string );
	}

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
	SET_INNER_PROC(pp, tt, rootNode, jumpNode, fkey );

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

		//Create a new table
		lt_exec_complex( tt, start, end - 1, &pp, build_individual );
		lt_lock( pp.ctable );
	}

	//Destroy the source table. 
	lt_free( tt );

	//Now check that each table has something
	for ( int i=0; i<pp.tlistLen; i++ ) {
		Table *tl = pp.tlist[ i ];
		lt_kdump( tl );

#if 1
		//Hash check on dem bi	
		yamlList *tn = testNodes;
		while ( tn->k ) {
		#if 0
			fprintf( stderr, "hash of %s: %d\n", tn->v, lt_geti( tl, tn->v ) );
		#else
			//slower, but easier on me...
			if ( !tn->v )
				;//fprintf( stderr, "No target value for column %s\n", tn->k );	
			else {
				fprintf( stderr, "Finding value for column: %s\n", tn->k );
				int p;
				char buf[ 2048 ] = {0};
				const char *rootShort = "div^thumb_div.";
				memcpy( buf, rootShort, (p = strlen( rootShort )) );
				memcpy( &buf[ p ], tn->v, strlen( tn->v ) );
				fprintf( stderr, "hash of %s: %d\n", buf, lt_geti( tl, buf ) );
			}
		#endif
			tn++;
		}

		//exit( 0 ); 
#endif
	}
exit( 0 );

	//for ( ... ) free( hashlist );
	//for ( ... ) free( pp.tlist );
	//free( pp.hlist );
	return 0; 
}


