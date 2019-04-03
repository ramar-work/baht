/*web.c - Handle any web requests*/

#include "vendor/single.h"
#include <curl/curl.h>
#include <gnutls/gnutls.h>


typedef struct wwwResponse {
	int status, len;
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
#if 0
		//Use libCurl
fprintf(stderr,"curl hangs...\n" );
		CURL *curl = NULL;
		CURLcode res;
		curl_global_init( CURL_GLOBAL_DEFAULT );
		curl = curl_easy_init();
fprintf(stderr,"curl hangs...\n" );
		if ( curl ) {
			Sbuffer sb = { 0, malloc(1) }; 	
fprintf(stderr,"curl hangs...\n" );
			curl_easy_setopt( curl, CURLOPT_URL, p );
			curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, WriteDataCallbackCurl );
			curl_easy_setopt( curl, CURLOPT_WRITEDATA, (void *)&sb );
			curl_easy_setopt( curl, CURLOPT_USERAGENT, ua );
fprintf(stderr,"curl about to do easy perform...\n" );
			res = curl_easy_perform( curl );
fprintf(stderr,"curl hangs after easy perform...\n" );
			if ( res != CURLE_OK ) {
				fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
				curl_easy_cleanup( curl );
			}
			//write(2,sb.buf,sb.len);
			curl_global_cleanup();
			*destlen = sb.len;
			*dest = (char *)sb.buf;
		}	
#else
	fprintf(stderr,"%s -> %d\n", root, port );
		if ( !socket_connect( &s, root, port ) ) {
			return err_set( 0, "%s\n", "Couldn't connect to site... " );
		}

		//socket connect is the shorter way to do this...

		//the long way to do this is to open a non-blocking socket and get host by name and all...
		
		
#endif
	}
#if 0
	else {
		fprintf( stderr, PROG "Can't handle TLS requests, yet!\n" );
		exit( 0 );
	}
#else
	else {

		//Do socket connect (but after initial connect, I need the file desc)
		if (  !socket_connect( &s, root, port ) ) {
			return err_set( 0, "%s\n", "Couldn't connect to site... " );
		}

		//GnuTLS
		if (  !gnutls_check_version("3.4.6") ) { 
			return err_set( 0, "%s\n", "GnuTLS 3.4.6 or later is required for this example." );	
		}

		//Is this needed?
		if (  ( err = gnutls_global_init() ) < 0 ) {
			return err_set( 0, "%s\n", gnutls_strerror( err ));
		}

		if (  ( err = gnutls_certificate_allocate_credentials( &xcred ) ) < 0 ) {
			return err_set( 0, "%s\n", gnutls_strerror( err ));
		}

		if (  ( err = gnutls_certificate_set_x509_system_trust( xcred )) < 0 ) {
			return err_set( 0, "%s\n", gnutls_strerror( err ));
		}
		/*
		//Set client certs this way...
		gnutls_certificate_set_x509_key_file( xcred, "cert.pem", "key.pem" );
		*/	

		//Initialize gnutls and set things up
		if (  ( err = gnutls_init( &session, GNUTLS_CLIENT ) ) < 0 ) {
			return err_set( 0, "%s\n", gnutls_strerror( err ));
		}

		if (  ( err = gnutls_server_name_set( session, GNUTLS_NAME_DNS, root, strlen(root)) < 0)) {
			return err_set( 0, "%s\n", gnutls_strerror( err ));
		}

		if (  ( err = gnutls_set_default_priority( session ) ) < 0 ) {
			return err_set( 0, "%s\n", gnutls_strerror( err ));
		}
		
		if (  ( err = gnutls_credentials_set( session, GNUTLS_CRD_CERTIFICATE, xcred )) <0 ) {
			return err_set( 0, "%s\n", gnutls_strerror( err ));
		}

		gnutls_session_set_verify_cert( session, root, 0 );
		//fprintf( stderr, "s.fd: %d\n", s.fd );
		gnutls_transport_set_int( session, s.fd );
		gnutls_handshake_set_timeout( session, GNUTLS_DEFAULT_HANDSHAKE_TIMEOUT );

		//This is ass ugly...
		do {
			 ret = gnutls_handshake( session ) ;
		} while ( ret < 0 && gnutls_error_is_fatal( ret ) == 0 );

		if (  ret < 0 ) {
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

		if ( ( err = gnutls_record_send( session, GetMsg, len ) ) < 0 )
			return err_set( 0, "%s\n", "GnuTLS 3.4.6 or later is required for this example." );	

		//This is a sloppy quick way to handle EAGAIN
		int statter=0;
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

		if ((err = gnutls_bye(session, GNUTLS_SHUT_RDWR)) < 0 ) {
			return err_set( 0, "%s\n",  gnutls_strerror( ret ) );
		}

	}
#endif
	return 1;
}


#ifdef IS_TEST
//the things to worry about...
//
// - hanging because of address issues
const char *urls[] = {
	"http://yourmom.com"
 ,NULL
};

int main (int argc, char *argv[]) {
	//
	wwwResponse w;
	const char **j = urls;
	char *buf = NULL;
	int buflen = 0;
	fprintf( stderr, "Running test suite...\n" );

	//Loop	
	while ( *j ) {
		if ( !load_www( *j, &buf, &buflen, &w ) ) {
			fprintf( stderr, "Failed to load URL at '%s'\n", *j );
		}
		j++;
	}

	//Dump the results...
	
	return 0;
}
#endif
