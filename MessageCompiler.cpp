#include <common.h>
#pragma hdrstop

#include <MessageCompiler.h>

namespace Msg {

CError Err_MessageFileNotFound{ L"Message file not found!\r\nFile name: %0" };
//////////////////////////////////////////////////////////////////////////

CMessageCompiler::CMessageCompiler( CUnicodeView fileName ) :
	input( fileName )
{
	check( FileSystem::FileExists( fileName ), Err_MessageFileNotFound, fileName );
}

void CMessageCompiler::Compile( CUnicodeView srcOutputName, CUnicodeView binOutputName ) const
{
	createIncOutput( srcOutputName );
	createSrcOutput( srcOutputName );
	createBinOutput( binOutputName );
}

static const CStringView fileGeneralPrefix = "// Automatically generated message ID definitions.\r\nnamespace Msg {\r\n\r\n";
static const CStringView fileGeneralSuffix = "//////////////////////////////////////////////////////////////////////////\r\n\r\n}	// namespace Msg.\r\n";
void CMessageCompiler::createIncOutput( CUnicodeView name ) const
{
	CString incOutput = Str( fileGeneralPrefix );
	fillSectionDeclaration( incOutput );
	fillIncOutput( incOutput );
	incOutput += fileGeneralSuffix;

	CUnicodeString headerName( name );
	FileSystem::ReplaceExt( headerName, L"h" );
	File::WriteText( headerName, incOutput );
}

static const CStringView srcDeclTemplate = "extern const int %0;\r\n";
static const CStringView sectionNameSuffix = "Section";
static const CStringView sectionDeclarationSuffix = "//////////////////////////////////////////////////////////////////////////\r\n\r\n";
void CMessageCompiler::fillSectionDeclaration( CString& result ) const
{
	for( const auto& section : input.GetUnnamedSections() ) {
		if( !section.GetName().IsEmpty() ) {
			result += srcDeclTemplate.SubstParam( section.GetName() + sectionNameSuffix );
		}
	}
	for( const auto& section : input.GetNamedSections() ) {
		result += srcDeclTemplate.SubstParam( section.GetName() + sectionNameSuffix );
	}
	result += sectionDeclarationSuffix;
}

// Fill the key data in the header.
void CMessageCompiler::fillIncOutput( CString& result ) const
{
	fillIncOutput( input.GetUnnamedSections(), result );
	fillIncOutput( input.GetNamedSections(), result );
}

static const CStringView sectionPrefixTemplate = "namespace %0 {\r\n\r\n";
static const CStringView sectionSuffixTemplate = "\r\n}	// namespace %0.\r\n\r\n";
void CMessageCompiler::fillIncOutput( CArrayView<CMessageSection> sections, CString& result )
{
	for( const auto& section : sections ) {
		const auto sectionName = section.GetName();
				
		if( !sectionName.IsEmpty() ) {
			result += sectionPrefixTemplate.SubstParam( sectionName );
		}

		for( const auto& key : section.GetKeyNames() ) {
			result += srcDeclTemplate.SubstParam( key );
		}

		if( !sectionName.IsEmpty() ) {
			result += sectionSuffixTemplate.SubstParam( sectionName );
		}
	}
}

static const CStringView srcPrefixTemplate = "#include <common.h>\r\n#pragma hdrstop\r\n\r\n#include <%0.h>\r\n\r\n";
void CMessageCompiler::createSrcOutput( CUnicodeView name ) const
{
	const CUnicodeString nameExt = FileSystem::GetNameExt( name );
	CString srcOutput = srcPrefixTemplate.SubstParam( nameExt ) + fileGeneralPrefix;
	fillSectionDefinition( srcOutput );
	fillSrcOutput( srcOutput );
	srcOutput += fileGeneralSuffix;

	CUnicodeString nameCopy( name );
	FileSystem::ReplaceExt( nameCopy, L"cpp" );
	File::WriteText( nameCopy, srcOutput );
}

static const CStringView srcDefTemplate = "extern const int %0 = %1;\r\n";
void CMessageCompiler::fillSectionDefinition( CString& result ) const
{
	int sectionId = 0;
	for( const auto& section : input.GetUnnamedSections() ) {
		if( !section.GetName().IsEmpty() ) {
			result += srcDefTemplate.SubstParam( section.GetName() + sectionNameSuffix, sectionId );
			sectionId++;
		}
	}
	for( const auto& section : input.GetNamedSections() ) {
		result += srcDefTemplate.SubstParam( section.GetName() + sectionNameSuffix, sectionId );
		sectionId++;
	}
	result += sectionDeclarationSuffix;
}

// Fill the key data in the source file and report that the result contains non-trivial data.
void CMessageCompiler::fillSrcOutput( CString& result ) const
{
	int totalKeyCount = 0;
	fillSrcOutput( input.GetUnnamedSections(), result, totalKeyCount );
	fillSrcOutput( input.GetNamedSections(), result, totalKeyCount );
}

void CMessageCompiler::fillSrcOutput( CArrayView<CMessageSection> sections, CString& result, int& totalKeyCount )
{
	for( const auto& section : sections ) {
		const auto sectionName = section.GetName();

		if( !sectionName.IsEmpty() ) {
			result += sectionPrefixTemplate.SubstParam( sectionName );
		}

		for( const auto& key : section.GetKeyNames() ) {
			result += srcDefTemplate.SubstParam( key, totalKeyCount );
			totalKeyCount++;
		}

		if( !sectionName.IsEmpty() ) {
			result += sectionSuffixTemplate.SubstParam( sectionName );
		}
	}
}

void CMessageCompiler::createBinOutput( CUnicodeView name ) const
{
	CFileWriter binOutputFile( name, FCM_CreateAlways );
	// We know the exact size of a message and can set the archive buffer accordingly.
	const int binarySize = input.MessageBinarySize();
	CArchiveWriter binOutput( binOutputFile, binarySize );
	writeBinSectionNames( binOutput );
	writeBinMessages( binOutput );
}

void CMessageCompiler::writeBinSectionNames( CArchiveWriter& binOutput ) const
{
	const auto sections = input.GetNamedSections();
	const auto size = sections.Size();
	CMap<CString, int> sectionIds;
	sectionIds.ReserveBuffer( size );
	int namedSectionId = getFirstNamedSectionId();
	for( const auto& section : sections ) {
		sectionIds.Add( section.GetName(), namedSectionId );
		namedSectionId++;
	}

	binOutput << sectionIds;
}

void CMessageCompiler::writeBinMessages( CArchiveWriter& binOutput ) const
{
	binOutput << input.MessageCount();
	int namedMessageId = fillBinOutput( input.GetUnnamedSections(), binOutput );
	fillBinOutput( input.GetNamedSections(), binOutput );

	int namedSectionId = getFirstNamedSectionId();
	for( const auto& section : input.GetNamedSections() ) {
		for( const auto& key : section.GetKeyNames() ) {
			binOutput << key;
			binOutput << namedSectionId;
			binOutput << namedMessageId;
			namedMessageId++;
		}
		namedSectionId++;
	}
}

int CMessageCompiler::getFirstNamedSectionId() const
{
	return input.GetUnnamedSections().Size();
}

// Serialize message values and return the number of processed values.
int CMessageCompiler::fillBinOutput( CArrayView<CMessageSection> sections, CArchiveWriter& result )
{
	int messageCount = 0;
	for( const auto& section : sections ) {
		for( const auto& value : section.GetKeyValues() ) {
			result << value;
			messageCount++;
		}
	}
	return messageCount;
}

//////////////////////////////////////////////////////////////////////////

}	// namespace Msg.
