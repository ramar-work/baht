/* ---------------------------------------------- *
cwbuilder

Builds the database for CheapWhips.net...
 * ---------------------------------------------- */

#include <stdio.h> 
#include <unistd.h> 
#include <errno.h> 
#include <sys/stat.h> 
#include <sys/types.h> 
#include <fcntl.h> 
#include <string.h> 
#include <gumbo.h>
#define PROG "p"


const char *html_1 = "<h1>Hello, World!</h1>";
const char html_file[] = "files/carri.html";
const char yamama[] = ""
"<html>"
	"<body>"
		"<h2>Snazzy</h2>"
		"<p>The jakes is coming</p>"
	"</body>"
"</html>"
;

/*
1; json
html {
	head { ... }
	body {
		h2
		p
	}
}

*/

/*
//An example iterator
void mit ( GumboNode *node ) {
	GumboVector *vc = &node->v.element.children;

	for ( unsigned int i=0; i < vc->length; i++ ) {
		//Print the type of node and the content (if printable)	
		fprintf( stderr, "%d -> %s %p\n", i, 
			gumbo_normalized_tagname( (vc->data[ i ]).type ), 
			vc->data[ i ] );
	}
}
*/



static unsigned int ind=0;
char *gumbo_type = NULL;

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

	//Loop through the body and create a "Table" 
	GumboVector *bc = &body->v.element.children;
	for ( int i=0; i<bc->length; i++ ) {
		//Get data "endpoints"
		GumboNode *gn = bc->data[ i ] ;
		const char *gtagname = gumbo_normalized_tagname( gn->v.element.tag );
		const char *gtype = (char *)print_gumbo_type( gn->type );
		//Dump everything
		fprintf( stderr, "%02d, %-10s, %s\n", i, gtype, gtagname );
	}


	//Free the Gumbo structure
	gumbo_destroy_output( &kGumboDefaultOptions, output );

	//???
	return 0; 

}
