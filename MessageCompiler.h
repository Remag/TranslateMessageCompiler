#pragma once
#include <MessageFile.h>

namespace Msg {

//////////////////////////////////////////////////////////////////////////

class CMessageCompiler {
public:
	explicit CMessageCompiler( CUnicodeView fileName );

	void Compile( CUnicodeView srcOutputName, CUnicodeView binOutputName ) const;

private:
	CMessageFile input;

	int getFirstNamedSectionId() const;

	void createIncOutput( CUnicodeView name ) const;
	void fillSectionDeclaration( CString& result ) const;
	void fillIncOutput( CString& result ) const;
	static void fillIncOutput( CArrayView<CMessageSection> sections, CString& result );
	void createSrcOutput( CUnicodeView name ) const;
	void fillSectionDefinition( CString& result ) const;
	void fillSrcOutput( CString& result ) const;
	static void fillSrcOutput( CArrayView<CMessageSection> sections, CString& result, int& totalKeyCount );
	void createBinOutput( CUnicodeView name ) const;
	void writeBinSectionNames( CArchiveWriter& binOutput ) const;
	void writeBinMessages( CArchiveWriter& binOutput ) const;

	static int fillBinOutput( CArrayView<CMessageSection> sections, CArchiveWriter& result );
};

//////////////////////////////////////////////////////////////////////////

}	// namespace Msg.

