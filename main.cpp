#include <common.h>
#pragma hdrstop

#include <MessageCompiler.h>

int wmain( int argc, wchar_t* argv[] )
{
	assert( argc == 4 );
	CUnicodeView msgFile = argv[1];
	CUnicodeView srcOutputFile = argv[2];
	CUnicodeView binOutputFile = argv[3];

	try {
		Msg::CMessageCompiler compiler( msgFile );
		compiler.Compile( srcOutputFile, binOutputFile );
	} catch( CException& e ) {
		Log::Exception( e );
		return -1;
	}
	return 0;
}

