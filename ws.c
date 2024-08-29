// WS_C

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include "regex/wregex.h"

#define match(p, s)   _match(p, s, __FILE__, __LINE__)
#define regex( s,p ) match(p, s)

static wregmatch_t* subm_g;
static wregex_t* r_g;
static const char* fnPattern = "[a-zA-Z_]+[0-9a-zA-Z_]*\\.[0-9a-zA-Z_]+";

static char swapOut; // populated with "swap-out" char, such as ' ' ,or '\n' etc
static char swapIn; // populated with replacement char/token.
static int swapout_length; // number of contiguous 'swapOut' tokens to match.
static int swapin_length;  // number of sequential 'swapIn' tokens to splice-and-replace with 'swapOut' match sequence.

static void fillVars( char* swapPattern, char* swapOut, int* swapout_length, char* swapIn, int* swapin_length );
static int ws_replace( char* );
static int _match(const char *p, const char *s, const char *file, int line);
static char* substring( char*, int, int );
static char* getstring( char* );

static char* swapOutPattern( char, int );
static char* swapInStr( char, int );


int main( int argc, char** argv )	{
	
	subm_g = (wregmatch_t*)malloc( sizeof(wregmatch_t) );
	r_g    = (wregex_t*)   malloc( sizeof(wregex_t) );
	
	if( argc==1 )	{
		
		printf( "Usage: ws [filename.ext] \" [x]\\t[x]\" | \"\\t[x] [x]\" | \"\\n[x] [x]\" | ...\nThe [x] modifier for number of consecutive ws-chars to replace, is optional, i.e. passing \"\\t \", will replace a single tab char '\\t' with a single nbsp ' ' char. Exiting.\n" );
		return 1;
	}
	
	if( argc==2 )
		if( !regex( argv[1],fnPattern ) )	{
			
			printf( "If you're going to test the limits of a poor wee cmd-line utility whose creator literally named him after a British acronym for a lavatory, you could at least pass a filename for poor wee 'wc' to try and guess what whitespace-transformations you wanted! C'èst Terriblè!! EXITING!!.\n" );
			return 1;
		}
	
	char* fn = strdup( argv[1] );
	if( argc==2 ) // filename only, gotta guess!!
		swapOut = ' ', swapout_length = 4, swapIn = '\t', swapin_length = 1;
	else
		fillVars( argv[2], &swapOut, &swapout_length, &swapIn, &swapin_length );
	
	char* swapOut_pattern = swapOutPattern( swapOut, swapout_length );
	char* swapIn_str  = swapInStr( swapIn, swapin_length );
	
	FILE* f = fopen( fn, "r" );
	
	int i;
	for ( i = -1; fgetc(f) != (char)-1; ++i )
		;

	int strlen_InStr = i;
	char* _ = (char*)malloc( i + 1 );

	fclose( f );
	f = fopen( fn, "r" );

	_[0] = fgetc( f );
	i = 1;
	if( _[0] != -1 )
		for ( i; _[i-1] != (char)-1; )
			_[i++] = fgetc( f );

	_[--i] = '\0';

	int result;
	int j = 0;
	int k = 0;

	char* _t = _;

	i = 0;
	while( _t[0] > 0 )	{
	
		if ( !regex( _t, swapOut_pattern ) )
			break;

		++i;
		
		j = (int) (subm_g[0].end - _t);
		
		_t += j;
	}
	
	_t = _;
	
	int* _begin = (int*)malloc( sizeof(int) * i );
	int* _end   = (int*)malloc( sizeof(int) * i );

	i = 0;
	while( _t[0] > '\0' )	{

		if ( !regex( _t, swapOut_pattern ) )
			break;

		++i;

		j = (int) (subm_g[0].end - _t);

		*(_begin+k) = (int) (subm_g[0].beg - _t);
		*(_end+k) = (int) (subm_g[0].end - _t);

		++k;
		_t += j;
	}

	_t = _;
	char* transformed;

	int strlen__ = strlen( _ );
	int n = (k * swapin_length) - (k * swapout_length);

	transformed = (char*)malloc( strlen__ + n + 1 );
	transformed[ strlen__ + n ] = '\0';
	transformed[0] = '\0';

	k = 0;
	char temp = 0;
	int offset = 0;

	while( i>k ) {

			offset = *(_begin+k);
			
			temp = _t[offset];
			_t[offset] = '\0';
			
			strcat( transformed, _t );
			strcat( transformed, swapIn_str );

			_t[offset] = temp;

			_t = _t + *(_end+k);
			
			k++;
		}
	;
	
	strcat( transformed, _t );

	if( !i )	{
		
		printf( "Apparently no pattern matches. Exiting.\n" );
		return 0;
	}
	
	//printf( "'ws-replace' reports %d matches/replacements.\n", i );
	printf( "%s", transformed );

	free( _begin );
	free( _end );
	free( _ );
	fclose( f );
	
	return 0;
}

static int _match(const char *p, const char *s, const char *file, int line) {
	
	int e, ep;
	wregex_t *r;
	wregmatch_t *subm;

	r = wrx_comp(p, &e, &ep);
	if(!r) {
		fprintf(stderr, "\n[%s:%d] ERROR......: %s\n%s\n%*c\n", file, line, wrx_error(e), p, ep + 1, '^');
		exit(EXIT_FAILURE);
	}

	if(r->n_subm > 0) {
		subm = calloc(sizeof *subm, r->n_subm);
		if(!subm) {
			fprintf(stderr, "Error: out of memory (submatches)\n");
			wrx_free(r);
			exit(EXIT_FAILURE);
		}
	} else
		subm = NULL;

	e = wrx_exec(r, s, &subm, &r->n_subm);

	if(e < 0) fprintf(stderr, "Error: %s\n", wrx_error(e));

	*subm_g = *subm;
	*r_g = *r;
	
	free(subm);
	wrx_free(r);

	return e;
}

static char* swapInStr( char swapChar, int swapLength )	{
	
	char* s = (char*)malloc( swapLength + 1 );
	
	int i = 0;
	while( i<swapLength )
		s[i++] = swapChar;
	
	s[i] = '\0';
	
	return s;
}

static char* swapOutPattern( char swapChar, int swapLength )	{

	char* s;
	int f = 0;
	
	switch( swapChar )	{
		
		case ' ':
			break;
			
		case '\t':
			swapChar = 't';
			break;
		case '\n':
			swapChar = 'n';
			break;
		case '\r':
			swapChar = 'r';
			break;
		
		default:
			printf( "Unexpected ws-candidate passed to swapPattern( ... ). Exiting.\n" );
			exit( 1 );
	}
	
	int c = ( swapChar==' ' ? 1 : 5 );
	int v = 0;
	if( swapLength > 1 )
		v = 3;
	
	s = (char*)malloc(c + v + 1);

	int j = 0;
	
	if( swapChar != ' ' )	{
		s[j++] = '[';
		s[j++] = '\\';
		//s[j++] = '\\';
	}
	
	s[j++] = swapChar;
	
	if( swapChar != ' ' )
		s[j++] = ']';
	
	if( v )	{
		
		s[j++] = '{';
		s[j++] = '0' + swapLength;
		s[j++] = '}';
	}
	
	s[j] = '\0';

	return s;			
}

static char* substring( char* strPtr, int begin, int end )	{

	// end is exclusive to range: range := [begin,end)
	--end;
	
	char* _ = (char*)malloc( end-begin+1 );
	
	int i = 0;
	int p = end-begin;
	
	while( i<p )
		_[i++] = strPtr[i+begin];
		
	_[i] = '\0';

	return _;
}

static char* getstring( char* in ){
	
	unsigned long long int str_length = strlen( in );
	char* _ = (char*)malloc( str_length+1 );
	
	unsigned long long int i;
	for( i=0; i<str_length; i++ )
		_[i] = in[i];
	
	_[i] = '\0';
	
	return _;
}

static void fillVars( char* swapPattern, char* swapOut, int* swapout_length, char* swapIn, int* swapin_length )	{
	
	char* _ = swapPattern;
	char currChar = *_;
	
	int f = 0;
	int L2 = 0;

	loop:
	
	if( currChar == '\\' )	{
		
		f = 1;
		currChar = *(++_);
	}
	
	if( f = 1 )	{
		
		switch( currChar )	{
			
			case 't':
				currChar = '\t';
				break;
			case 'r':
				currChar = '\r';
				break;
			case 'n':
				currChar = '\n';
				break;
				
			default:
				;
				break;
		}
	}
	
	f = 0;
	
	if( L2==1 )
		goto ret;
		
	*swapOut = currChar;
	
	currChar = *(++_);
	
	if( currChar=='[' )
		++_, *swapout_length=(*_)-'0', ++_;
	else
		*swapout_length = 1;
	
	L2 = 1;

	goto loop;
	
	ret:
	
	
	*swapIn = currChar;
	
	++_;
	
	if( *_=='[' )
		++_, *swapin_length=(*_)-'0', _+=2;
	else
		*swapin_length = 1;

	// basic sanity check
	if( *_ != '\0' )
		printf( "Unexpected EOS byte at line number %s in file %s. Expected NULL char, found %c instead.\n", __LINE__, __FILE__, *_ );
	
	return;
}

