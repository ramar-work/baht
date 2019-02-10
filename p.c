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
 * 
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
char *print_gumbo_type ( GumboNodeType t ) {
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


//Go through and run something on a node and ALL of its descendants
int rr ( GumboNode *node ) {
	//Loop through the body and create a "Table" 
	GumboVector *bc = &node->v.element.children;

	//For first run, comments, cdata, document and template nodes do not matter
	for ( int i=0; i<bc->length; i++ ) {
		//Set up data
		GumboNode *n = bc->data[ i ] ;

#if 1
		//Dump data if needed
		//TODO: Put some kind of debug flag on this
		char *type = print_gumbo_type( n->type );
		char *itemname = retblock( n );
		fprintf( stderr, "%06d, %04d, %-10s, %s\n", ++gi, i, type, itemname );
#endif

#if 1
		//Handle what to do with the actual node
		//TODO: Handle GUMBO_NODE_[CDATA,COMMENT,DOCUMENT,WHITESPACE,TEMPLATE]
		if ( n->type != GUMBO_NODE_TEXT && n->type != GUMBO_NODE_ELEMENT )
			; //User selected handling can take place here, usually a blank will do
		else if ( n->type == GUMBO_NODE_TEXT ) {
			; //Handle/save the text reference here
		}
		else if ( n->type == GUMBO_NODE_ELEMENT ) {
			GumboVector *gv = &n->v.element.children;
			if ( !gv->length )
				0;
			else {
				rr( n );
			}
		}
#endif
	}

	return 0;
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

	//Run the parser against everything
	//TODO: Call this something more clear than rr()
	rr( body );

	//Free the Gumbo structure
	gumbo_destroy_output( &kGumboDefaultOptions, output );

	//???
	return 0; 

}
