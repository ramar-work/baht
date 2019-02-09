/* ---------------------------------------------- *
cwbuilder

Builds the database for CheapWhips.net...
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


static unsigned int ind=0;
char *gumbo_type = NULL;
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


char *print_gumbo_type ( GumboNodeType t ) {
	if ( t == GUMBO_NODE_DOCUMENT )
		gumbo_type = "document";
	else if ( t == GUMBO_NODE_ELEMENT )
		gumbo_type = "element";
	else if ( t == GUMBO_NODE_TEXT )
		gumbo_type = "text";
	else if ( t == GUMBO_NODE_CDATA )
		gumbo_type = "cdata";
	else if ( t == GUMBO_NODE_COMMENT )
		gumbo_type = "comment";
	else if ( t == GUMBO_NODE_WHITESPACE )
		gumbo_type = "whitespace";
	else if ( t == GUMBO_NODE_TEMPLATE ) {
		gumbo_type = "template";
	}
	return gumbo_type;
}


//Find body
GumboNode* find_tag ( GumboNode *node, GumboTag t ) {
	GumboVector *children = &node->v.element.children;
	//GumboNode *element = NULL;

	for ( int i=0; i<children->length; i++ ) {
		//Get data "endpoints"
		GumboNode *gn = children->data[ i ] ;
		const char *gtagname = gumbo_normalized_tagname( gn->v.element.tag );
		const char *gtype = (char *)print_gumbo_type( gn->type );

		//Dump everything
		//fprintf( stderr, "%02d, %-10s, %s\n", i, gtype, gtagname );

		//I need to move through the body only
		if ( gn->v.element.tag == t ) {
			return gn;
		}
	}
	return NULL;
}


//Go through and run something on a node and ALL of its descendants
int rr ( GumboNode *node ) {
	//Loop through the body and create a "Table" 
	GumboVector *bc = &node->v.element.children;

	//???
	for ( int i=0; i<bc->length; i++ ) {
		//Get data "endpoints"
		GumboNode *n = bc->data[ i ] ;
		const char *tagname = gumbo_normalized_tagname( n->v.element.tag );
		const char *type = (char *)print_gumbo_type( n->type );

		//Dump everything
		fprintf( stderr, "%02d, %-10s, %s\n", i, type, tagname );

		//Check if there's children, if so, descend, if not, print what's there 
		if ( n->type != GUMBO_NODE_ELEMENT ) {
			//what do I do?
		}
		else {
			//get a count of the children?	
			GumboVector *gv = &n->v.element.children;
			//GumboNode *nn = gv->data[ 0 ];
			//fprintf( stderr, "%d\n", gv->length );
			//regardless of tag type, all attributes need to be saved, not sure how
			//to loop through this yet

			//then, keep running and going deeper in the tree until it's all parsed
			if ( !gv->length ) {
			//fprintf( stderr, "%s\n", gumbo_normalized_tagname( nn->v.element.tag ) ); 
			//fprintf( stderr, "%s\n", print_gumbo_type( nn->type ) );
				//img, or some other unclosable tag
			}
			else if ( gv->length == 1 ) {

			}
			else {
				//re-run for each node?
				rr( n );
			}	
		}	
	}

	return 0;
}


//Much like moving through any other parser...
int main() {

	int fn, len;
	struct stat sb;
	char *block = NULL;

#if 1
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
	//GumboNode *head = find_tag( output->root, GUMBO_TAG_HEAD );
	if ( !body ) {
		fprintf( stderr, PROG ": no <body> found!\n" );
		return 1;
	}

	//Loop through the body and create a "Table" 
	GumboVector *bc = &body->v.element.children;
	for ( int i=0; i<bc->length; i++ ) {
		//Get data "endpoints"
		GumboNode *gn = bc->data[ i ] ;
		const char *gtagname = gumbo_normalized_tagname( gn->v.element.tag );
		const char *gtype = (char *)print_gumbo_type( gn->type );

		//Dump everything
		fprintf( stderr, "%02d, %-10s, %s\n", i, gtype, gtagname );

		//Check if there's children, if so, descend, if not, print what's there 
		if ( gn->type != GUMBO_NODE_ELEMENT ) {
			//what do I do?
		}
		else {
			//get a count of the children?	
			GumboVector *gv = &gn->v.element.children;
			fprintf( stderr, "%d\n", gv->length );
			//if it's over 1, then we'll loop thru
			//if it's 1, what type is the node? 
			GumboNode *nn = gv->data[ 0 ];
			fprintf( stderr, "%s\n", gumbo_normalized_tagname( nn->v.element.tag ) ); 
			fprintf( stderr, "%s\n", print_gumbo_type( nn->type ) );
			//fprintf( stderr, "%s\n", nn->v.text.text );
		}	
		const char *gtext = (char *)print_gumbo_type( gn->type );
	}


	//Free the Gumbo structure
	gumbo_destroy_output( &kGumboDefaultOptions, output );

	//???
	return 0; 

}
