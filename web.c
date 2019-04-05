/*web.c - Handle any web requests*/

#include "vendor/single.h"
#include <curl/curl.h>
#include <gnutls/gnutls.h>

#define ADD_ELEMENT( ptr, ptrListSize, eSize, element ) \
	if ( ptr ) \
		ptr = realloc( ptr, sizeof( eSize ) * ( ptrListSize + 1 ) ); \
	else { \
		ptr = malloc( sizeof( eSize ) ); \
	} \
	*(&ptr[ ptrListSize ]) = element; \
	ptrListSize++;


typedef struct wwwResponse {
	int status, len, clen, pad;
	uint8_t *data;
	char *redirect_uri;
//char *uri;
} wwwResponse;


typedef struct stretchBuffer {
	int len;
	uint8_t *buf;
} Sbuffer;


//Error buffer
char _errbuf[2048] = {0};

//User-Agent
const char ua[] = "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/51.0.2704.103 Safari/537.36";

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
int err_set ( int status, char *fmt, ... ) {
	va_list ap;
	va_start( ap, fmt ); 
	vsnprintf( _errbuf, sizeof( _errbuf ), fmt, ap );  
	va_end( ap );
	return status;
}

void timer_handler (int signum ) {
	static int count =0;
	printf( "timer expired %d times\n", ++count );
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


int get_status (char *msg, int mlen) {

	const char *ok[] = {
		"HTTP/0.9 200 OK"
	, "HTTP/1.0 200 OK"
	, "HTTP/1.1 200 OK"
	, "HTTP/2.0 200 OK"
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


int load_www ( const char *p, char **dest, int *destlen, wwwResponse *r ) {
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

	//HEADs and GETs
	const char GetMsgFmt[] = 
		"GET %s HTTP/1.1\r\n"
		"Host: %s\r\n"
		"User-Agent: %s\r\n\r\n"
	;

	//Pack a message
	if ( port != 443 )
		len = snprintf( GetMsg, sizeof(GetMsg) - 1, GetMsgFmt, path, root, ua );
	else {
		char hbbuf[ 128 ] = { 0 };
		//snprintf( hbbuf, sizeof( hbbuf ) - 1, "www.%s:%d", root, port );
		snprintf( hbbuf, sizeof( hbbuf ) - 1, "%s:%d", root, port );
		len = snprintf( GetMsg, sizeof(GetMsg) - 1, GetMsgFmt, path, hbbuf, ua );
	}

	//NOTE: Although it is definitely easier to use CURL to handle the rest of the 
	//request, dealing with TLS at C level is more complicated than it probably should be.  
	//Do either an insecure request or a secure request
	if ( !sec ) {
#if 1
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
	fprintf(stderr,"%s -> %d\n", root, port );
		//socket connect is the shorter way to do this...
		struct addrinfo hints, *servinfo, *pp;
		int rv;
		int sockfd;
		char s[ INET6_ADDRSTRLEN ];
		char b[ 10 ] = { 0 };
		snprintf( b, 10, "%d", port );	
		memset( &hints, 0, sizeof( hints ) );
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		fprintf(stderr,"Attempting connection to '%s':%d\n", root, port);
		fprintf(stderr,"Full path (%s)\n", p );
		//fprintf(stderr, "%s\n", p );

	#if 0
		//I'd have to set an alarm here, which means an interrupt
		struct sigaction sa;
		struct itimerval timer;
		memset( &sa, 0, sizeof(sa) );
		sa.sa_handler = &timer_handler;
		sigaction( SIGVTALRM, &sa, NULL );
		timer.it_value.tv_sec = 0;	
		timer.it_value.tv_usec = 250000;
		//if we had an interval, we would set that here via it_interval
		setitimer( ITIMER_VIRTUAL, &timer, NULL );	
	#endif

		//Get the address info of the domain here.
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
		int clen=0;
		int br=0;
		char *msg = malloc(1);
		while ( 1 ) {
			int b;

			//Stop reading
			if ( (clen && br > -1 && rb == (clen + br)) ) { 
				fprintf(stderr, "%s\n", "No new bytes sent.  Jump out of loop." );
				break;
			}

			if (( b = recv( sockfd, buf, sizeof(buf), 0 )) == -1 ) {
				fprintf(stderr, "Error receiving mesaage to: %s\n", s );
				break;
			}

			if ( !first++ ) {
				clen = get_content_length( buf, b );
				br = memstrat( buf, "\r\n\r\n", b );
				fprintf(stderr, "found content length and eoh: %d, %d\n", clen, br );	
			}

			if ( !b ) {
				fprintf(stderr, "%s\n", "No new bytes sent.  Jump out of loop." );
				break;
			}

			//read into a bigger buffer	
			msg = realloc( msg, rb + b );
			memcpy( &msg[ rb ], buf, b ); 

			//you need to do realloc magic here...
			fprintf(stderr,"received %d bytes this invocation\n", b );
			fprintf(stderr,"received %d total bytes\n", rb );
			rb += b;
		}
		#if 1
		write( 2, msg, rb );
		#endif 	
		*dest = msg;
		*destlen = rb;
#endif
	}
	else {
		//Do socket connect (but after initial connect, I need the file desc)
		if ( !socket_connect( &s, root, port ) ) {
			return err_set( 0, "%s\n", "Couldn't connect to site... " );
		}

		//GnuTLS
		if ( !gnutls_check_version("3.4.6") ) { 
			return err_set( 0, "%s\n", "GnuTLS 3.4.6 or later is required for this example." );	
		}

		//Is this needed?
		if ( ( err = gnutls_global_init() ) < 0 ) {
			return err_set( 0, "%s\n", gnutls_strerror( err ));
		}

		if ( ( err = gnutls_certificate_allocate_credentials( &xcred ) ) < 0 ) {
			return err_set( 0, "%s\n", gnutls_strerror( err ));
		}

		if ( ( err = gnutls_certificate_set_x509_system_trust( xcred )) < 0 ) {
			return err_set( 0, "%s\n", gnutls_strerror( err ));
		}
		/*
		//Set client certs this way...
		gnutls_certificate_set_x509_key_file( xcred, "cert.pem", "key.pem" );
		*/	

		//Initialize gnutls and set things up
		if ( ( err = gnutls_init( &session, GNUTLS_CLIENT ) ) < 0 ) {
			return err_set( 0, "%s\n", gnutls_strerror( err ));
		}

		if ( ( err = gnutls_server_name_set( session, GNUTLS_NAME_DNS, root, strlen(root)) < 0)) {
			return err_set( 0, "%s\n", gnutls_strerror( err ));
		}

		if ( ( err = gnutls_set_default_priority( session ) ) < 0 ) {
			return err_set( 0, "%s\n", gnutls_strerror( err ));
		}
		
		if ( ( err = gnutls_credentials_set( session, GNUTLS_CRD_CERTIFICATE, xcred )) <0 ) {
			return err_set( 0, "%s\n", gnutls_strerror( err ));
		}

		gnutls_session_set_verify_cert( session, root, 0 );
		//fprintf( stderr, "s.fd: %d\n", s.fd );
		gnutls_transport_set_int( session, s.fd );
		gnutls_handshake_set_timeout( session, GNUTLS_DEFAULT_HANDSHAKE_TIMEOUT );

fprintf(stderr,"sizeof: %ld\n", sizeof(buf)); exit(0);
		//This is ass ugly...
		do {
			 ret = gnutls_handshake( session ) ;
		} while ( ret < 0 && gnutls_error_is_fatal( ret ) == 0 );

		if ( ret < 0 ) {
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

		//This is the initial request... wtf were you thinking?
		if ( ( err = gnutls_record_send( session, GetMsg, len ) ) < 0 )
			return err_set( 0, "%s\n", "GnuTLS 3.4.6 or later is required for this example." );	

		//This is a sloppy quick way to handle EAGAIN
		int statter=0;
		int first = 0;
		char *msg = malloc(1);
		int buflen;
		int rb = 0;
		int chunked = 0;
		char **ptrarr = NULL;
		int ptrarrlen = 0;
		int *szarr = NULL;
		int szarrlen = 0;
	#if 1
//fprintf(stderr, "%d\n", sz); exit(0);

		while ( 1 ) {

			//receive
			if ((ret = gnutls_record_recv( session, buf, sizeof(buf))) == 0 ) {
				fprintf( stderr, " - Peer has closed the TLS Connection\n" );
				break;	
			}
			else if ( ret < 0 && gnutls_error_is_fatal( ret ) == 0 ) {
				fprintf( stderr, " Warning: %s\n", gnutls_strerror( ret ) );
				//goto end;
				//statter = 5;
				continue;
			}
			else if ( ret < 0 ) {
				fprintf( stderr, " Error: %s\n", gnutls_strerror( ret ) );
				break;	
			}
			else if ( ret > 0 ) {
				//Assuming that we've received packets, let's start reading
				fprintf( stdout, "Recvd %d bytes:\n", ret );
				//Don't move forward if it's anything but 200 OK.
				//There are more creative ways to do this...
				if ( !first++ ) {
					r->status = get_status( (char *)buf, ret );
					r->clen = get_content_length( buf, ret );
					//is it chunked or not?
					chunked = memstrat( buf, "Transfer-Encoding: chunked", ret ) > -1;
					//find the crlf
					int crlf = memstrat( buf, "\r\n\r\n", ret );
					if ( crlf == -1 )
						return err_set( 0, "%s\n", "No CRLF sequence found, response malformed." );
					else  {
						char *m = &buf[ crlf + 4 ];
						//parse the chunked length
						int lenp = memstrat( m, "\r\n", ret - (crlf+4));	
						//do i save this length and re-request?
						char lenpbuf[ 10 ] = {0};
						memcpy( lenpbuf, m, lenp );
						//convert hex to something else
						int sz = 0;
						//do we read a chunk of this size?
						//it seems like i still need to send something back
					}
					fprintf( stderr, "Got status: %d\n", r->status );
					fprintf( stderr, "Got clen: %d\n", r->clen );
					//"Transfer-Encoding: chunked"
					//"transfer-encoding: chunked"
					//"Transfer-Encoding: Chunked"
					fprintf( stderr, "is chunked?: %s\n", chunked ? "t" : "f" );
write(2, buf, ret );
//exit( 0 );
				}

				if ( ret == 5 ) {
					if ( memcmp( buf, "0\r\n\r\n", 5 ) == 0 ) {
						fprintf(stderr, "the last one came in" );
						break;
					}
					else {
						fprintf(stderr, "not sure what happened.." );
					} 
				}
			}

			//read into a bigger buffer	
#if 0
			msg = realloc( msg, rb + ret );
			memcpy( &msg[ rb ], buf, ret ); 
#else
			msg = malloc( ret + 1 );
			memcpy( msg, buf, ret );
			ADD_ELEMENT( ptrarr, ptrarrlen, char *, msg );
			ADD_ELEMENT( szarr, szarrlen, int, ret );
#endif
		#if 0
			fprintf( stderr,"recvd %d bytes... ", ret );
			write( 2, &msg[ rb ], 10 );
			write( 2, &msg[ rb ], ret );
		#endif
			rb += ret;

			//if it's chunked, try sending a 100-continue
			if ( chunked ) {
				const char *cont = "HTTP/1.1 100 Continue\r\n\r\n";
				//int err = gnutls_record_send( session, cont, strlen(cont)); 
				fprintf(stderr, "%s (%d)\n", gnutls_strerror( err ), err );
			}
		}

		if ((err = gnutls_bye(session, GNUTLS_SHUT_RDWR)) < 0 ) {
			//return err_set( 0, "%s\n",  gnutls_strerror( ret ) );
		}

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
	#else
		//This looks like it is just grabbing the header...
		while ( statter < 5 ) {
			if (  (ret = gnutls_record_recv( session, msg, sizeof(msg))) == 0 ) {
				fprintf( stderr, " - Peer has closed the TLS Connection\n" );
				//goto end;
				statter = 5;
			}
			else if (  ret < 0 && gnutls_error_is_fatal( ret ) == 0 ) {
				fprintf( stderr, " Warning: %s\n", gnutls_strerror( ret ) );
				//goto end;
				//statter = 5;
			}
			else if (  ret < 0 ) {
				fprintf( stderr, " Error: %s\n", gnutls_strerror( ret ) );
				//goto end;
			}
			else if (  ret > 0 ) {
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
					if (  (ret = gnutls_record_recv( session, &bmsg[ pl ], sizeof(bmsg))) == 0 ) {
						fprintf( stderr, " - Peer has closed the TLS Connection\n" );
						//goto end;
					}
					else if (  ret < 0 && gnutls_error_is_fatal( ret ) == 0 ) {
						fprintf( stderr, " Warning: %s\n", gnutls_strerror( ret ) );
						//goto end;
					}
					else if (  ret < 0 ) {
						fprintf( stderr, " Error: %s\n", gnutls_strerror( ret ) );
						//why would a session be invalidated?
						//goto end;
					}
					else if (  ret > 0 ) {
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
	#endif

	#if 0 
		if ((err = gnutls_bye(session, GNUTLS_SHUT_RDWR)) < 0 ) {
			return err_set( 0, "%s\n",  gnutls_strerror( ret ) );
		}
	#endif

	}
	return 1;
}


#ifdef IS_TEST
//the things to worry about...
//
// - hanging because of address issues
//const char *urls[] = {
struct U { char *url; int i; wwwResponse www; } urls[] = {
#if 0
	"http://www.ramarcollins.com"
#else
	{	"https://www.google.com/search?client=opera&q=gnutls+error+string&sourceid=opera&ie=UTF-8&oe=UTF-8" }
//"https://www.importmotorsportsnc.com/cars-for-sale"
	//"https://cabarruscountycars.com/legacy" //contains clen
	//"https://www.heritagerides.com/inventory.aspx"
#endif
 ,{ NULL }
};

int main (int argc, char *argv[]) {
	//
	wwwResponse w;
	struct U *j = urls;
	char *buf = NULL;
	int buflen = 0;
	fprintf( stderr, "Running test suite...\n" );

	//Loop	
	while ( j->url ) {
		if ( !( j->i = load_www( j->url, &buf, &buflen, &j->www ) ) ) {
			fprintf( stderr, "Failed to load URL at '%s'\n", j->url );
		}
		j++;
	}

	//Dump the results...
	
	return 0;
}
#endif
