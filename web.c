/*web.c - Handle any web requests*/

/*Write defines here*/
//#define SHOW_RESPONSE
//#define WRITE_RESPONSE
#define SHOW_REQUEST
#define VERBOSE 
#define SSL_DEBUG
#define INCLUDE_TIMEOUT
/*Stop*/

#ifndef SSL_DEBUG 
 #define SSLPRINTF( ... )
 #define EXIT(x)
 #define MEXIT(x,m)
#else
 #define SSLPRINTF( ... ) fprintf( stderr, __VA_ARGS__ ) ; fflush(stderr);
 #define EXIT(x) \
	fprintf(stderr,"Forced Exit.\n"); exit(x);
 #define MEXIT(x,m) \
	fprintf(stderr,m); exit(x);
#endif

#ifndef SHOW_REQUEST
 #define DUMPRST( b, blen )
#else
 #define DUMPRST( b, blen ) write( 2, b, blen );
#endif

#ifndef SHOW_RESPONSE
 #define DUMPRSP( b, blen )
#else
 #define DUMPRSP( b, blen ) write( 2, b, blen );
#endif

#ifndef WRITE_RESPONSE
 #define WRITEF( fn, b, blen )
#else
 #define WRITEF( fn, b, blen ) write_to_file( fn, b, blen );
#endif

//This allows me to compile this file as an executable
#ifndef _BAHTWEB
#include "vendor/single.h"
#include <curl/curl.h>
#include <gnutls/gnutls.h>

#ifndef DEBUG
 #define RUN(c) (c)
#else
 #define RUN(c) \
 (c) || (fprintf(stderr, "%s: %d - %s\n", __FILE__, __LINE__, #c)? 0: 0)
#endif

#ifndef VERBOSE
 #define VPRINTF( ... )
#else
 #define VPRINTF( ... ) fprintf( stderr, __VA_ARGS__ ) ; fflush(stderr);
#endif

#define FPATH "tests/colombo/"

#ifndef FPATH
 #define FPATH "./"
#endif

#define ADD_ELEMENT( ptr, ptrListSize, eSize, element ) \
	if ( ptr ) \
		ptr = realloc( ptr, sizeof( eSize ) * ( ptrListSize + 1 ) ); \
	else { \
		ptr = malloc( sizeof( eSize ) ); \
	} \
	*(&ptr[ ptrListSize ]) = element; \
	ptrListSize++;


typedef struct wwwResponse {
	int status, len, clen, ctype, chunked;
	uint8_t *data, *body;
	char *redirect_uri;
} wwwResponse;


typedef struct { 
	int secure, port, fragment; 
	char *addr; 
} wwwType;

typedef struct stretchBuffer {
	int len;
	uint8_t *buf;
} Sbuffer;


//Error buffer
char _errbuf[2048] = {0};

//User-Agent
const char ua[] = "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/51.0.2704.103 Safari/537.36";

int err_set ( int status, char *fmt, ... ) {
	va_list ap;
	va_start( ap, fmt ); 
	vsnprintf( _errbuf, sizeof( _errbuf ), fmt, ap );  
	va_end( ap );
	return status;
}

#endif


//base16 decoder
int radix_decode( char *number, int radix ) {
	const int hex[127] = {['0']=0,1,2,3,4,5,6,7,8,9,
		['a']=10,['b']=11,['c']=12,13,14,15,['A']=10,11,12,13,14,15};
	int tot=0, mult=1, len=strlen(number);
	number += len; //strlen( digits ); 
	while ( len-- ) {
		//TODO: cut ascii characters not in a certain range...
		tot += ( hex[ (int)*(--number) ] * mult);
		mult *= radix;
	}

	return tot;
}


//Extracts the HTTP message body from the message
int extract_body ( wwwResponse *r ) {
	//when sending this, we have to skip the header
	const char *t = "\r\n\r\n";
	uint8_t *nsrc = r->data; 
	uint8_t *rr = malloc(1);
	int pos, tot=0, y=0, br = r->len;

	if (( pos = memstrat( r->data, t, br ) ) == -1 ) {
		fprintf( stderr, "carriage return not found...\n" ); 
		return 0;
	}

	//Move ptrs and increment things, this is the start of the headers...
	pos += 4;
	nsrc += pos; 
	br -= pos;
	t += 2;
	r->body = &r->data[ pos ];

	//chunked
	if ( !r->chunked ) {
		return 1;
	} 

	//find the next \r\n, after reading the bytes
	while ( 1 ) {
		//define a buffer for size
		//search for another \r\n, since this is the boundary
		int sz, szText;
		char tt[ 64 ];
		memset( &tt, 0, sizeof(tt) );
		if ((szText = memstrat( nsrc, t, br )) == -1 ) {
			break;
		}
		memcpy( tt, nsrc, szText );
		sz = radix_decode( tt, 16 );
	
		//move up nsrc, and get ready to read that
		nsrc += szText + 2;
		rr = realloc( rr, tot + sz );
		memset( &rr[ tot ], 0, sz );
		memcpy( &rr[ tot ], nsrc, sz );

		//modify digits
		nsrc += sz + 2, /*move up ptr + "\r\n" and "\r\n"*/
		tot += sz,
		br -= sz + szText + 2;

		//this has to be finalized as well
		if ( br == 0 ) {
			break;
		}
	}

#if 0
	r->body = tt;
	r->clen = tot;
#else
	//Trade pointers and whatnot
	int ndSize = pos + strlen("\r\n\r\n") + tot;
	uint8_t *newData = malloc( ndSize );
	memset( newData, 0, ndSize );
	memcpy( &newData[ 0 ], r->data, ndSize - tot );
	memcpy( &newData[ ndSize - tot ], rr, tot );

	//Free stuff
	free( r->data );
	free( rr  );

	//Set stuff
	r->data = NULL;	
	r->data = newData;
	r->len  = ndSize;
	r->body = &newData[ ndSize - tot ];
	r->clen = tot;
#endif
	return 1;
}


#if 0
//16 ^ 0 = 1    = n * ((16^0) or 1) = n
//16 ^ 1 = 16   = n * ((16^1) or 16) = n
//16 ^ 2 = ...
//printf("%d\n", n ); getchar();
int main() {
int p;
struct intpair { char *c; int i; } tests[] = {
 { NULL }
,{ "fe",  254 }	
,{ "c",   12	}
,{ "77fe",30718 }
,{ "87fe",34814 }
#if 0
,{ "87fecc1c",2281622556}
#endif
,{ NULL	}
};
struct intpair *a = tests;

while ( (++a)->c ) 
	printf("%d == %d = %s\n", 
	p=radix_decode( a->c, 16 ),a->i,p==a->i?"true":"false");
exit( 0);
#endif

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

//
void timer_handler (int signum) {
	static int count =0;
	fprintf( stdout, "timer expired %d times\n", ++count );
	//when sockets die, they die
	//no cleanup, this is bad, but I can deal with it for now, and even come up with a way to clean up provided data strucutres are in one place...
	exit( 0 );	
}

  // get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa) {
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


int get_content_length (char *msg, int mlen) {
	int pos;
	if ( (pos = memstrat( msg, "Content-Length", mlen )) == -1 ) { 
		return -1;
	}

	int r = memchrat( &msg[ pos ], '\r', mlen - pos );	
	int s = memchrat( &msg[ pos ], ' ', mlen - pos );	
	char lenString[ 24 ] = { 0 };
	memcpy( lenString, &msg[ pos + (s+1) ], r - s );

	//Return this if not found.
	return atoi( lenString );
}


int get_content_type (char *msg, int mlen) {
	int pos;
	if ( (pos = memstrat( msg, "Content-Type", mlen )) == -1 ) { 
		return -1;
	}

	int r = memchrat( &msg[ pos ], '\r', mlen - pos );	
	int s = memchrat( &msg[ pos ], ' ', mlen - pos );	
	char ctString[ 1024 ] = { 0 };
	memcpy( ctString, &msg[ pos + (s+1) ], r - s );
	//fprintf( stderr, "content-type: %s\n", ctString );exit(0);
	return mti_geti( ctString );
}



int get_status (char *msg, int mlen) {
	//NOTE: status line will not always have an 'OK'
	const char *ok[] = {
		"HTTP/0.9 200"
	, "HTTP/1.0 200"
	, "HTTP/1.1 200"
	, "HTTP/2.0 200"
	, NULL
	};
	const char **lines = ok;
	int stat = 0;
	int status = 0;

	//just safe?
	while ( *lines ) {
		if ( memcmp( msg, *lines, strlen(*lines) ) == 0 ) {
			stat = 1;
			char statWord[ 10 ] = {0};
			memcpy( statWord, &msg[ 9 ], 3 );
			return atoi( statWord );
			break;
		}
		lines++;
	}

	return -1;
}


//???
void print_www ( wwwResponse *r ) {
	fprintf( stderr, "status: %d\n", r->status );
	fprintf( stderr, "len:    %d\n", r->len );
	fprintf( stderr, "clen:   %d\n", r->clen );
	fprintf( stderr, "ctype:  %d\n", r->ctype );
	fprintf( stderr, "chunked:%d\n", r->chunked );
	fprintf( stderr, "redrUri:%s\n", r->redirect_uri	 );
#if 1
	fprintf( stderr, "\nbody\n" ); write( 2, r->body, 200 );
	fprintf( stderr, "\ndata\n" ); write( 2, r->data, 400 );
#endif
}


//...
void select_www( const char *addr, wwwType *t ) {
	//Checking for secure or not...
	if ( memcmp( "https", addr, 5 ) == 0 ) {
		t->secure = 1;
		t->port = 443;
		t->fragment = 0;
		//p = &p[8];
		//fp = &p[8];
	}
	else if ( memcmp( "http", addr, 4 ) == 0 ) {
		t->secure = 0;
		t->port = 80;
		t->fragment = 0;
		//p = &p[7];
		//fp = &p[7];
	}
	else {
		t->fragment = 1;
		//t->secure = 0;
		//t->port = 0;
		//fp = p;
	}
}


//Load webpages via HTTP/S
int load_www ( const char *p, wwwResponse *r ) {
	const char *fp = NULL;
	wwwType t;
	memset(&t,0,sizeof(wwwType));
#if 1
	select_www( p, &t );
	if ( t.fragment ) /*load_www can never make a request like this*/
		return 0;
	else { 
		p = ( t.secure ) ? &p[8] : &p[7];
		fp = p;
	}
#else
	int sec, port;
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
#endif
	int err, ret, sd, ii, type, len;
	int chunked=0;
	unsigned int status;
	Socket s = { .server = 0, .proto = "tcp" };
	gnutls_session_t session;
	memset( &session, 0, sizeof(gnutls_session_t));
	gnutls_datum_t out;
	gnutls_certificate_credentials_t xcred;
	memset( &xcred, 0, sizeof(gnutls_certificate_credentials_t));
	char *desc = NULL;
	//uint8_t msg[ 32000 ] = { 0 };
	//uint8_t *msg = malloc(1);
	uint8_t *msg = NULL;
	char buf[ 4096 ] = { 0 }; 
	char GetMsg[2048] = { 0 };
	char rootBuf[ 128 ] = { 0 };
	const char *root = NULL; 
	const char *site = NULL; 
	const char *urlpath = NULL;
	const char *path = NULL;
	int c=0; 

	//Chop the URL very simply and crudely.
	if (( c = memchrat( p, '/', strlen( p ) )) == -1 )
		path = "/", root = p;
	else {	
		memcpy( rootBuf, p, c );
		path = &p[ c ];
		root = rootBuf;
	}

	//HEADs and GETs
	const char GetMsgFmt[] = 
		"GET %s HTTP/1.1\r\n"
		"Host: %s\r\n"
		"User-Agent: %s\r\n\r\n"
	;

	//Pack a message
	if ( t.port != 443 )
		len = snprintf( GetMsg, sizeof(GetMsg) - 1, GetMsgFmt, path, root, ua );
	else {
		char hbbuf[ 128 ] = { 0 };
		//snprintf( hbbuf, sizeof( hbbuf ) - 1, "www.%s:%d", root, t->port );
		snprintf( hbbuf, sizeof( hbbuf ) - 1, "%s:%d", root, t.port );
		len = snprintf( GetMsg, sizeof(GetMsg) - 1, GetMsgFmt, path, hbbuf, ua );
	}

	//Show the response if asked
	DUMPRST( GetMsg, len ); 
	//EXIT(0);

	//NOTE: Although it is definitely easier to use CURL to handle the rest of the 
	//request, dealing with TLS at C level is more complicated than it probably should be.  
	//Do either an insecure request or a secure request
	if ( !t.secure ) {
#if 0
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
			write(2,sb.buf,sb.len);
			curl_global_cleanup();
			*destlen = sb.len;
			*dest = (char *)sb.buf;
		}	
#else

		//socket connect is the shorter way to do this...
		struct addrinfo hints, *servinfo, *pp;
		int rv;
		int sockfd;
		char s[ INET6_ADDRSTRLEN ];
		char b[ 10 ] = { 0 };
		snprintf( b, 10, "%d", t.port );	
		memset( &hints, 0, sizeof( hints ) );
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		VPRINTF("Attempting connection to '%s':%d\n", root, t.port);
		VPRINTF("Full path (%s)\n", p );
		//fprintf(stderr, "%s\n", p );

	#ifdef INCLUDE_TIMEOUT 
		//Set up a timer to kill requests that are taking too long.
		//TODO: There is an alternate way to do with with socket(), I think
		struct itimerval timer;
		memset( &timer, 0, sizeof(timer) );
	#if 0
		signal( SIGVTALRM, timer_handler );
	#else
		struct sigaction sa;
		memset( &sa, 0, sizeof(sa) );
		sa.sa_handler = &timer_handler;
		sigaction( SIGVTALRM, &sa, NULL );
	#endif
		//Set timeout to 3 seconds (remember: no cleanup takes place...)
		timer.it_interval.tv_sec = 0;	
		timer.it_interval.tv_usec = 0;
		//NOTE: For timers to work, both it_interval and it_value must be filled out...
		timer.it_value.tv_sec = 2;	
		timer.it_value.tv_usec = 0;
		//if we had an interval, we would set that here via it_interval
		if ( setitimer( ITIMER_VIRTUAL, &timer, NULL ) == -1 ) {
			fprintf( stderr, "Set Timer Error: %s\n", strerror( errno ) );
			return 0;
		}
	#if 0	
		int ffd = open( "/dev/null", O_RDWR );
		for (int p=0;p<30000000;p++) write(ffd,"asdf",4);
	#endif
	#endif

		//Get the address info of the domain here.
		//TODO: Use Bind or another library for this.  Apparently there is no way to detect timeout w/o using a signal...
		if ( (rv = getaddrinfo( root, b, &hints, &servinfo )) != 0 ) {
			fprintf( stderr, "getaddrinfo: %s\n", gai_strerror( rv ) );
			return 0;
		}

		//Loop and find the right address
		for ( pp = servinfo; pp != NULL; pp = pp->ai_next ) {
			if ((sockfd = socket( pp->ai_family, pp->ai_socktype, pp->ai_protocol )) == -1) {	
				fprintf(stderr,"client: socket error: %s\n",strerror(errno));
				continue;
			}

			//fprintf(stderr, "%d\n", sockfd);
			if (connect(sockfd, pp->ai_addr, pp->ai_addrlen) == -1) {
				close( sockfd );
				fprintf(stderr,"client: connect: %s\n",strerror(errno));
				continue;
			}

			break;
		}

		//If we completely failed to connect, do something.
		if ( pp == NULL ) {
			fprintf(stderr, "client: failed to connect\n");
			return 1;
		}

		//Get the internet address
		inet_ntop(pp->ai_family, get_in_addr((struct sockaddr *)pp->ai_addr),s, sizeof s);
	#ifdef INCLUDE_TIMEOUT 
		struct sigaction da;
		da.sa_handler = SIG_DFL;
		sigaction( SIGVTALRM, &da, NULL );
	#endif

    fprintf(stderr,"Client connected to: %s\n", s );
		freeaddrinfo(servinfo);

		int sb=0, rb=0;
		int mlen = strlen(GetMsg);
		//And do the send and read?
		//If we send a lot of data, like a giant post, then this should be reflected here...
		while ( 1 ) {
			int b;
			if (( b = send( sockfd, GetMsg, mlen, 0 )) == -1 ) {
				fprintf(stderr, "Error sending mesaage to: %s\n", s );
				//
			}
		
			sb += b;
			if ( sb != mlen ) {
				fprintf(stderr, "%d total bytes sent\n", sb );
				continue;
			}

			break;
		}

 
		//WE most likely will receive a very large page... so do that here...	
		int first=0;
		int crlf=-1;
		msg = malloc(1);

		while ( 1 ) {
			uint8_t xbuf[ 4096 ];
			memset( xbuf, 0, sizeof(xbuf) );
			int blen = recv( sockfd, xbuf, sizeof(xbuf), 0 );

			if ( blen == -1 ) {
				SSLPRINTF( "Error receiving mesaage to: %s\n", s );
				break;
			}
			else if ( !blen ) {
				SSLPRINTF( "%s\n", "No new bytes sent.  Jump out of loop." );
				break;
			}

			if ( !first++ ) {
				r->status = get_status( (char *)xbuf, blen );
				r->clen = get_content_length( (char *)xbuf, blen );
				r->ctype = get_content_type( (char *)xbuf, blen );
				r->chunked = memstrat( xbuf, "Transfer-Encoding: chunked", blen ) > -1;
				if ( r->chunked ) {
					SSLPRINTF( "%s\n", "Chunked not implemented for HTTP" );
					return err_set( 0, "%s\n", "Chunked not implemented for HTTP" );
				}
				if (( crlf = memstrat( xbuf, "\r\n\r\n", blen ) ) == -1 ) {
					0;//this means that something went wrong... 
				}
				//fprintf(stderr, "found content length and eoh: %d, %d\n", r->clen, crlf );
				crlf += 4;
				//write( 2, xbuf, blen );
			}

			//read into a bigger buffer	
			msg = realloc( msg, r->len + blen );
			memcpy( &msg[ r->len ], xbuf, blen ); 
			r->len += blen;

			//info about the session
			//SSLPRINTF( "recvd: %d , clen: %d , mlen: %d\n", r->len - crlf, r->clen, r->len );

			if ( !r->clen ) {
				return err_set( 0, "%s\n", "No length specified, parser error!." );
			}
			else if ( r->clen && ( r->len - crlf ) == r->clen ) {
				SSLPRINTF( "Full HTTP message received\n" );
				break;
			}
		}
		//r->data = (uint8_t *)msg;
		//r->data = msg;
#endif
	}
	else {
		//Do socket connect (but after initial connect, I need the file desc)
		if ( !socket_connect( &s, root, t.port ) ) {
			return err_set( 0, "%s\n", "Couldn't connect to site... " );
		}

		if ( RUN( !gnutls_check_version("3.4.6") ) ) {
			return err_set( 0, "%s\n", "GnuTLS 3.4.6 or later is required for this example." );	
		}

		if ( RUN( ( err = gnutls_global_init() ) < 0 ) ) {
			return err_set( 0, "%s\n", gnutls_strerror( err ));
		}

		if ( RUN( ( err = gnutls_certificate_allocate_credentials( &xcred ) ) < 0 )) {
			return err_set( 0, "%s\n", gnutls_strerror( err ));
		}

		if ( RUN( (err = gnutls_certificate_set_x509_system_trust( xcred )) < 0 )) {
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

		if ( RUN( (err=gnutls_set_default_priority( session ) ) < 0) ) {
			return err_set( 0, "%s\n", gnutls_strerror( err ));
		}
		
		if ( RUN( ( err = gnutls_credentials_set( session, GNUTLS_CRD_CERTIFICATE, xcred )) < 0) ) {
			return err_set( 0, "%s\n", gnutls_strerror( err ));
		}

		//Set random handshake details
		gnutls_session_set_verify_cert( session, root, 0 );
		gnutls_transport_set_int( session, s.fd );
		gnutls_handshake_set_timeout( session, GNUTLS_DEFAULT_HANDSHAKE_TIMEOUT );

		//Do the first handshake
		do {
			 RUN( ret = gnutls_handshake( session ) );
		} while ( ret < 0 && gnutls_error_is_fatal( ret ) == 0 );

		//Check the status of said handshake
		if ( ret < 0 ) {
			fprintf( stderr, "ret: %d\n", ret );
			if ( RUN( ret == GNUTLS_E_CERTIFICATE_VERIFICATION_ERROR ) ) {	
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
			//consider dumping the session info here...
			fprintf( stdout, "- Session info: %s\n", desc );
			gnutls_free( desc );
		}

		//This is the initial request... wtf were you thinking?
		if ( RUN(( err = gnutls_record_send( session, GetMsg, len ) ) < 0 ) )
			return err_set( 0, "%s\n", "GnuTLS 3.4.6 or later is required for this example." );	

		//This is a sloppy quick way to handle EAGAIN
		msg = malloc(1);
		int first = 0;
		int crlf = -1;
	#ifdef SSL_DEBUG
		char **ptrarr = NULL;
		int ptrarrlen = 0;
		int *szarr = NULL;
		int szarrlen = 0;
	#endif
	
		//there should probably be another condition used...
		while ( 1 ) {
			char xbuf[ 4096 ];
			memset( xbuf, 0, sizeof(xbuf) ); 
			int ret = gnutls_record_recv( session, xbuf, sizeof(xbuf)); 
			SSLPRINTF( "gnutls_record_recv returned %d\n", ret );

			//receive
			if ( !ret ) {
				SSLPRINTF( "Peer has closed the TLS Connection\n" );
				break;
			}
			else if ( ret < 0 && gnutls_error_is_fatal( ret ) == 0 ) {
				SSLPRINTF( "Warning: %s\n", gnutls_strerror( ret ) );
				continue;
			}
			else if ( ret < 0 ) {
				SSLPRINTF( "Error: %s\n", gnutls_strerror( ret ) );
				break;	
			}
			else if ( ret > 0 ) {
				SSLPRINTF( "Recvd %d bytes:\n", ret );
				if ( !first++ ) {
					//Get status, content-length or xfer-encoding if available 
					r->status = get_status( (char *)xbuf, ret );
					r->clen = get_content_length( xbuf, ret );
					r->ctype = get_content_type( (char *)xbuf, ret );
					r->chunked = memstrat( xbuf, "Transfer-Encoding: chunked", ret ) > -1;
					if ((crlf = memstrat( xbuf, "\r\n\r\n", ret )) == -1 ) {
						SSLPRINTF( "%s\n", "No CRLF sequence found, response malformed." );
						return err_set( 0, "%s\n", "No CRLF sequence found, response malformed." );
					}
		
					//Increment the crlf by the length of "\r\n\r\n"	
					crlf += 4;
					if ( r->chunked ) {
						char *m = &xbuf[ crlf ];
						//parse the chunked length
						int lenp = memstrat( m, "\r\n", ret - crlf );	
						//to save chunk length, I need to convert to hex to decimal
						char lenpxbuf[ 10 ] = {0};
						memcpy( lenpxbuf, m, lenp );
						int sz = 0; //atoi( lenpxbuf );
					}
					
					SSLPRINTF( "Got %s.",r->chunked ?"chunked message.":"message w/ content-length.");
					SSLPRINTF( "Got status: %d\n", r->status );
					SSLPRINTF( "Got clen: %d\n", r->clen );
					SSLPRINTF( "%s\n", "Initial message:" );
				  write(2, xbuf, ret );
				}

				//Finalize chunked messages.
				if ( r->chunked && ret == 5 ) {
					if ( memcmp( xbuf, "0\r\n\r\n", 5 ) == 0 ) {
						fprintf(stderr, "the last one came in" );
fprintf( stderr, "%s, %d: my code ran.... but why stop?", __FILE__, __LINE__ ); 
						break;
					}
					else {
						fprintf(stderr, "not sure what happened.." );
					} 
				}
			}
fprintf( stderr, "%s, %d: my code ran.... but why stop?", __FILE__, __LINE__ ); 

		#ifdef SSL_DEBUG
			char *xmsg = malloc( ret + 1 );
			memcpy( xmsg, xbuf, ret );
			ADD_ELEMENT( ptrarr, ptrarrlen, char *, xmsg );
			ADD_ELEMENT( szarr, szarrlen, int, ret );
		#endif

			//Read into a bigger buffer	
			if ( !(msg = realloc( msg, r->len + ( ret + 1 ) )) ) {
				SSLPRINTF( "%s\n", "Realloc of destination buffer failed." );
				return err_set( 0, "%s\n", "Realloc of destination buffer failed." );
			}
			memcpy( &msg[ r->len ], xbuf, ret );
			r->len += ret;

			//If it's chunked, try sending a 100-continue
			if ( r->chunked ) {
				const char *cont = "HTTP/1.1 100 Continue\r\n\r\n";
				//int err = gnutls_record_send( session, cont, strlen(cont)); 
				fprintf(stderr, "%s (%d)\n", gnutls_strerror( err ), err );
			}
			else {
				if ( !r->clen ) {
					SSLPRINTF( "%s\n", "No length specified, parser error!." );
					return err_set( 0, "%s\n", "No length specified, parser error!." );
				}
				else if ( r->clen && ( r->len - crlf ) == r->clen ) {
					SSLPRINTF( "Full message received: clen: %d, mlen: %d", r->clen, r->len );
					break;
				}
				SSLPRINTF( "recvd: %d , clen: %d , mlen: %d\n", r->len - crlf, r->clen, r->len );
			}
		} /* end while */

		int tries=0;
		//why does this hang?
		while (tries++ < 3 && (err = gnutls_bye(session, GNUTLS_SHUT_WR)) != GNUTLS_E_SUCCESS ) ;
		//if ((err = gnutls_bye(session, GNUTLS_SHUT_RDWR)) < 0 ) {

		if ( err != GNUTLS_E_SUCCESS ) {
			return err_set( 0, "%s\n",  gnutls_strerror( ret ) );
		} 	
			
	#if 0	
	#ifdef SSL_DEBUG
		//This worked...
		int sz=0;
		for ( int x = 0; x<szarrlen; x++ ) sz += szarr[ x ];
		fprintf( stderr, "Total bytes recvd: %d\n", sz );

		for ( int x = 0; x<ptrarrlen; x++ ) {
			char *pt = ptrarr[ x ];
			int st = szarr[ x ];
			fprintf(stderr, ",{ size: %d, data: ", st);
			write( 2, "'", 1 );
			write( 2, pt, 10 );
			write( 2, "' }\n", 4 );
		}
	#endif
	#endif
	}

	//Now, both requests ought to be done.  Set things here.
	r->data = msg;
	extract_body( r );
	return 1;
}


#ifdef WRITE_RESPONSE
char bytes[ 24 ];
char *fname ( char *ptr ) {
	memset( &bytes, 0, sizeof( bytes ) );
	int sl = strlen( ptr );
	int l = sizeof(bytes) < sl ? sizeof(bytes) : sl; 
	for ( int i=0; i < l; i++ ) {
		bytes[ i ] = ( memchr( "./:", ptr[ i ], 3 ) ) ? '_' : ptr[ i ]; 
	}
	return bytes;
}


int write_to_file ( const char *filename, uint8_t *buf, int buflen ) {
	char fn[ 2048 ];
	memset( fn, 0, sizeof(fn) );
	memcpy( fn, FPATH, strlen(FPATH));
	memcpy( &fn[ strlen(FPATH) ], filename, strlen(filename));
	memcpy( &fn[ strlen(FPATH) + strlen(filename) ], ".html", 5 );

	int fd = open( fn, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR );
	if ( fd == -1 ) {
		VPRINTF( "open failed: %s\n", strerror( errno ) );
		return 0;
	}

	int nb = write( fd, buf, buflen );
	if ( nb == -1 ) {
		VPRINTF( "write failed: %s\n", strerror( errno ) );
		return 0;
	}

	if ( close( fd ) == -1 ) {
		VPRINTF( "write failed: %s\n", strerror( errno ) );
		return 0;
	}

	return 1;
}
#endif


#ifdef IS_TEST

#if 0
 #include "tests/urlHttp.c"
 #include "tests/urlHttps.c"
 #include "tests/urlShort.c"
 #include "tests/url.c"
#endif

typedef struct { char *url; int i; wwwResponse www; } Url;
Url urls[] = {
	{ "dummy" }
, { "http://www.ramarcollins.com" /* ... */ }
, {	"https://www.httpwatch.com/httpgallery/chunked/chunkedimage.aspx?0.46782549706004284" }
#if 0
  { "https://amp.businessinsider.com/images/592f4169b74af41b008b5977-750-563.jpg" }
#endif

#if 0
	//This is some script that I can use to test with memcmp
,	{ "https://jigsaw.w3.org/HTTP/ChunkedScript" }
, { "//upload.wikimedia.org/wikipedia/commons/thumb/1/1d/Nissan_GT-R_PROTO.jpg/220px-Nissan_GT-R_PROTO.jpg" }
#endif

#if 0
	//These are all binary images... just write them out and call it a day
, { "https://amp.businessinsider.com/images/592f4169b74af41b008b5977-750-563.jpg" }
, { "https://i.gifer.com/1TM.gif" }
, { "http://blogs.smithsonianmag.com/smartnews/files/2013/06/5008578082_2b5f4434bd_z.jpg" }
#endif

#if 0
// #include "tests/urlHttps.c"
#endif

#if 0
	//These are simple SSL pages
, {	"https://www.google.com/search?client=opera&q=gnutls+error+string&sourceid=opera&ie=UTF-8&oe=UTF-8" }
//these seem to work...
, { "http://www.ramarcollins.com" }
, { "https://cabarruscountycars.com/legacy" } //contains clen
, {	"https://www.google.com/search?client=opera&q=gnutls+error+string&sourceid=opera&ie=UTF-8&oe=UTF-8" }

//so why not these? (they do, but they reject bots... so you'll need js)
, { "https://www.heritagerides.com/inventory.aspx" /*This one just doesn't work... (js block)*/ }
, { "https://www.importmotorsportsnc.com/cars-for-sale" }
#endif

, { NULL }
};



int main (int argc, char *argv[]) {
	//...
	wwwResponse w;
	char *buf = NULL;
	int buflen = 0;
	VPRINTF( "Running test suite...\n" );

	//Loop	
	if ( 1 ) {
		Url *j = urls;
		j++; //always skip the first, since it's a dummy to make changing data easier...
		while ( j->url ) {
		#if 0
			fprintf( stderr, "Trying URL at '%s'\n", j->url );
		#else
			//if ( !( j->i = load_www( j->url, &buf, &buflen, &j->www ) ) ) {
			if ( !( j->i = load_www( j->url, /*&buf, &buflen,*/ &j->www ) ) ) {
				fprintf( stderr, "Failed to load URL at '%s'\n", j->url );
			}
			fprintf( stderr, "Load URL at '%s'\n", j->url );
		#endif
			j++;
		}
	}

	//Dump the results...
	if ( 0 ) {
		Url *j = urls;
		j++;

		while ( j->url ) {
			wwwResponse *w = &j->www;
			memset( w, 0, sizeof(wwwResponse) );
			fprintf( stderr, "Site:             %s\n", j->url );
			fprintf( stderr, "Status:           %d\n", w->status );
			fprintf( stderr, "Content Length:   %d\n", w->clen );
			fprintf( stderr, "Message Length:   %d\n", w->len );
			fprintf( stderr, "\n" );
			//DUMPF( 2, w->data, w->len );
			//fprintf(stderr,"%s\n",fname( j->url ));
			//WRITEF( fname( j->url ), w->data, w->len ); 
			WRITEF( fname( j->url ), w->body, w->clen ); 
			free( w->data );
			j++;
		}
	}
	else {
		const char bannerFmt[] = "%-50s\t%-6s\t%-6s\t%-6s\n";
		const char argFmt[] = "%-50s\t%-6d\t%-6d\t%-6d\n";
		char t[81];
		memset( t, 0, sizeof(t) );
		memset( t, 61 /* '=' */, 80 );
		fprintf( stderr, bannerFmt, "Site", "Status", "Clen", "Mlen" );
		fprintf( stderr, "%s\n", t );
		   
		Url *j = urls;
		j++;
		while ( j->url ) {
			wwwResponse *w = &j->www;
			fprintf( stderr, argFmt, j->url, w->status, w->clen, w->len );
			//WRITEF( fname( j->url ), w->data, w->len ); 
			WRITEF( fname( j->url ), w->body, w->clen ); 
			free( w->data );
			j++;
		}
	}
	
	return 0;
}
#endif
