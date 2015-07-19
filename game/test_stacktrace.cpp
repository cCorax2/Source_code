#include "stdafx.h"

#include <gtest/gtest.h>
#include <execinfo.h>
#include <cxxabi.h>

TEST( utils, stacktrace )
{
	void* 		array[200];
	std::size_t size; 
	char** 		strings;

	size = backtrace( array, 200 );
	strings = backtrace_symbols( array, size );

	std::size_t funcnamesize = 2048; 
	char* 		funcname = (char*)malloc( funcnamesize );

	EXPECT_TRUE( size > 0 );

	for ( std::size_t i=0; i<size; ++i )
	{
		char* begin_name = 0;
		char* begin_offset = 0;
		char* end_offset = 0;

		for ( char* p = strings[i]; *p; ++p )
		{
			if ( *p == '<' ) 
			{
				begin_name = p;
			}	
			else if ( *p == '+' ) 
			{
				begin_offset = p;
			}
			else if ( *p == '>' && begin_offset ) 
			{
				end_offset = p;	
				break;
			}
		}

		if ( begin_name && begin_offset && end_offset && begin_name < begin_offset )
		{
			*begin_name++ = '\0';
			*begin_offset++ = '\0';
			*end_offset = '\0';

			int status; 
			char* ret = abi::__cxa_demangle( begin_name, funcname, &funcnamesize, &status );

			if ( status == 0 )
			{
				funcname = ret; 
				//printf( " %s : %s+%s\n", strings[i], funcname, begin_offset );
			}
			else
			{
				//printf( " %s : %s()+%s\n", strings[i], begin_name, begin_offset );	
			}
		}
		else
		{
			//printf( "Symbol> %s\n", strings[i] );
		}
	}

	free( funcname );
	free( strings );
}

TEST( utils, stacktrace2 )
{
	void* 		array[200];
	std::size_t size; 
	char** 		strings;

	size = backtrace( array, 200 );
	strings = backtrace_symbols( array, size );

	EXPECT_TRUE( size > 0 );

	for ( std::size_t i=0; i<size; ++i )
	{
		printf( "Symbol> %s\n", strings[i] );
	}

	free( strings );
}
