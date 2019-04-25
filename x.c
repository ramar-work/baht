//converts hex numbers into regular decimal
#include "vendor/single.h"

int main (int argc, char *argv[]) {
	if ( argc < 2 ) { fprintf(stderr, "no args... (specify <string>" ); exit(0);}
	//count back wards, increase the mult 
	const int hex[127] = {['0']=0,1,2,3,4,5,6,7,8,9,
		['a']=10,['b']=11,['c']=12,13,14,15,['A']=10,11,12,13,14,15};
	char *digits = argv[ 1 ];
	int len=strlen(digits);
	int mult=1;	
	int tot=0;
	digits += len; //strlen( digits ); 
	while ( len-- ) {
		//TODO: cut ascii characters not in a certain range...
		tot += ( hex[ *(--digits) ] * mult);
		//16 ^ 0 = 1    = n * ((16^0) or 1) = n
		//16 ^ 1 = 16   = n * ((16^1) or 16) = n
		//16 ^ 2 = 
		//printf("%d\n", n ); getchar();
		mult *= 16;
	}

	fprintf(stderr,"%d\n",tot);
	return 0;
}
