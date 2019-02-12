/* ---------------------------------------------- *
 * cwbuilder
 * =========
 * 
 * Summary
 * -------
 * Builds the database for CheapWhips.net...
 * 
 * Usage
 * -----
 * tbd
 * 
 * Author
 * ------
 * Antonio R. Collins II (ramar@tubularmodular.com, ramar.collins@gmail.com)
 * Copyright 2019, Feb 9th
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
	}
};


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


#if 0
//Gumbo to Table 
int lua_to_table (lua_State *L, int index, Table *t ) {
	static int sd;
	lua_pushnil( L );
	obprintf( stderr, "Current stack depth: %d\n", sd++ );

	while ( lua_next( L, index ) != 0 ) 
	{
		int kt, vt;
		obprintf( stderr, "key, value: " );

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
		else if ( vt == LUA_TTABLE )
		{
			lt_descend( t );
			obprintf( stderr, "Descending because value at %d is table...\n", -1 );
			lua_loop( L );
			lua_to_table( L, index + 2, t ); 
			lt_ascend( t );
			sd--;
		}

		obprintf( stderr, "popping last two values...\n" );
		if ( vt == LUA_TNUMBER || vt == LUA_TSTRING ) {
			lt_finalize( t );
		}
		lua_pop(L, 1);
	}

	lt_lock( t );
	return 1;
}
#endif



Table *tt;
int twoSided = 0;

//Go through and run something on a node and ALL of its descendants
//Returns number of elements found (that match a certain type)
//TODO: Add element count 
//TODO: Add filter for element count
int rr ( GumboNode *node ) {
	//Loop through the body and create a "Table" 
	GumboVector *bc = &node->v.element.children;
	int stat = 0;

	//For first run, comments, cdata, document and template nodes do not matter
	for ( int i=0; i<bc->length; i++ ) {
		//Set up data
		GumboNode *n = bc->data[ i ] ;
		char *itemname = retblock( n );

#if 0
		//Dump data if needed
		//TODO: Put some kind of debug flag on this
		char *type = (char *)print_gumbo_type( n->type );
		fprintf( stderr, "%06d, %04d, %-10s, %s\n", ++gi, i, type, itemname );
#endif

#if 1
		//Handle what to do with the actual node
		//TODO: Handle GUMBO_NODE_[CDATA,COMMENT,DOCUMENT,WHITESPACE,TEMPLATE]
		if ( n->type != GUMBO_NODE_TEXT && n->type != GUMBO_NODE_ELEMENT ) {
			//User selected handling can take place here, usually a blank will do
			twoSided = 0; 
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
				rr( n );
			}

			lt_ascend( tt );
		}
#endif
	}

	lt_lock( tt );
	return 1;
}

struct useless_structure {
	LiteKv *parent;
	char *key;
} ParentInfo;

struct listOfStuff {
	char *listy;
} KVList;


int extract_same ( LiteKv *kv, int i, void *p ) {
	//The parent should be at a specific address and not change during each new invocation
	//The FULL key should also be the same (not like the child couldn't, but its less likely)
	//A data structure can take both of these...
	return 1;
}


//Much like moving through any other parser...
int main() {

	int fn, len;
	struct stat sb;
	char *block = NULL;

#if 0
	block = (char *)yamama;
	len = strlen( block );
#else
	char fb[ 2000000 ] = {0};

	//Read file to memory
	if ( stat( html_file, &sb ) == -1 ) {
		fprintf( stderr, "%s: %s\n", PROG, strerror( errno ) );
		return 1; 
	}

	if ( (fn = open( html_file, O_RDONLY )) == -1 ) {
		fprintf( stderr, "%s: %s\n", PROG, strerror( errno ) );
		return 1; 
	}
	
	if ( read( fn, fb, sb.st_size ) == -1 ) {
		fprintf( stderr, "%s: %s\n", PROG, strerror( errno ) );
		return 1; 
	}

	block = fb;
	len = sb.st_size;
#endif

	//Parse that and do something
	GumboOutput *output = gumbo_parse_with_options( &kGumboDefaultOptions, block, len ); 

	//We can loop through the Gumbo data structure and create a node list that way
	GumboVector *children = &(output->root)->v.element.children;
	GumboNode *body = find_tag( output->root, GUMBO_TAG_BODY );
	if ( !body ) {
		fprintf( stderr, PROG ": no <body> found!\n" );
		return 1;
	}

	//Allocate a Table
	Table t;
	tt = &t;
	if ( !lt_init( &t, NULL, 33300 ) ) {
		fprintf( stderr, PROG ": couldn't allocate table!\n" );
		return 1;
	}

	//Run the parser against everything
	//TODO: Call this something more clear than rr()
	int elements = rr( body );
	lt_dump( tt );




	//Loop through each of the requested nodes
	//This will probably look more like:
	//1. find "mini-root" or "loop" node
	//2. copy a table from "loop" node (or start node) to an end node
	//3. stream to database structure or whatever...
	for ( int i=0; i<sizeof(nodes)/sizeof(NodeSet); i++ ) {
#if 0	
		//1. Read each line (of YAML or whatever) into this structure	
		struct s { char *left, *right; } list = chop_line( *line );
		//1a. This could come from a compiled in data structure, but that's not the
		//best method here...
#endif

		//References
		NodeSet *root = &nodes[ i ].rootNode; 
		NodeSet *jump = &nodes[ i ].jumpNode; 
		//NodeSet *loop = &nodes[ i ].loopNodes; 

		//2. Second step is to simply find the node
		if ( ( root->hash = lt_geti( tt, root->string ) ) == -1 ) {
			fprintf( stderr, PROG ": string '%s' not found.\n", root->string );
			exit( 0 );
		}
		fprintf(stderr, "@%-5d -> %s\n", root->hash, root->string );

		//2A. Jumping basically means that in the root node, there are many elements which we just don't need to worry about.	
		//In this particular case, I need to find the first thing that matches the jump node, and/or input this myself.
		//Let's try matching the jump node.
		if ( ( jump->hash = lt_geti( tt, jump->string ) ) == -1 ) {
			fprintf( stderr, PROG ": string '%s' not found.\n", jump->string );
			exit( 0 );
		}
		fprintf(stderr, "@%-5d -> %s\n", jump->hash, jump->string );
		LiteKv *rootkv = lt_retkv( tt, root->hash );
		fprintf( stderr, "par: %s\n", rootkv->key.v.vchar );		


		//ParentInfo pp = { 
		//lt_exec( tt, ParentInfo, extract_same );

		//Now I know where to start extracting each table, but I don't know:
		// - where to end
		// - how many parents are really left
		//It looks like both of these questions can be answered if I can query my Table structure for where each neighbor is.
		//
		//Get a count of how many elements are there
		//Find all the elements with a matching hash 
	exit( 0 );

#if 0
		//3. If no node, then make a note of it... if node exists, start this
		//4. Extract or Mark the root node
		//5. read that yaml from here and stream to structure
		//NOTE: This will make it easy for me to loop through the "inner node" list
		//and grab each value from the new table.  (and I don't have to re-read
		//files) 
		for ( int fi=0; fi<???; fi++ ) {
			struct s { char *left, *right; } list = chop_line( *line );
		}

		//6. Find the "loop" node.  
		//NOTE: A root node will probably have formatted data sitting SOMEWHERE
		//in it's structure.  It's just a matter of finding that and could be as
		//simple as some basic math (like rootnode + 7 = my start node)
		Table *it = lt_copybetween( node, list->right );
		for ( int nn=0; nn<???; nn++ ) {
		}
#endif

#if 0
		//METHOD A: add entries to a database column structure
		//struct dbRecords { enum record, char *colname, *value; } *db;
		db = malloc( sizeof( dbRecords ) );
		*db = { SQL_CHAR, list->left, list->right };
		db++;
#endif

#if 0
		//METHOD B: write to a file ( will probably change often, and could be slow)
		const char sqlCmd[] = "INSERT INTO table ( ... ) VALUES (  )"; 
		char sqlBuf[ ? ] = { 0 };
		write( fn, sqlCmd, len( sqlCmd ) );	
		snprintf( sqlBuf, sqlFmt, len( sqlBuf ), ... );
		write( fn, sqlBuf, len( sqlBuf ) );
		//open and close of file takes place after...
#endif
	}

	//Free the Gumbo and Table structures
	gumbo_destroy_output( &kGumboDefaultOptions, output );
	lt_free( &t );

	//???
	return 0; 

}
