#include <common.h>
#pragma hdrstop

#include <MessageFile.h>

namespace Msg {

static CUnicodePart deleteWhitespace( CUnicodePart str )
{
	return str.TrimSpaces();
}

//////////////////////////////////////////////////////////////////////////

CMessageSection::CMessageSection( CMessageSection&& other ) :
	name( move( other.name ) ),
	keyNames( move( other.keyNames ) ),
	keyValues( move( other.keyValues ) ),
	uniqueKeys( move( other.uniqueKeys ) )
{

}

bool CMessageSection::SetString( CUnicodePart keyName, CUnicodePart newValue )
{
	if( !uniqueKeys.Set( keyName ) ) {
		return false;
	}
	keyNames.Add( keyName );
	keyValues.Add( newValue );
	return true;
}

//////////////////////////////////////////////////////////////////////////

CMessageFile::CMessageFile( CUnicodeView _fileName ) :
	fileName( _fileName )
{
	parseFile();
}

extern const CError Err_BadMessageFile( L"Message file contains an invalid string.\nFile name: %0. File position: %1." );
void CMessageFile::parseFile()
{
	const CUnicodeString fileStr = File::ReadUnicodeText( fileName );

	CMessageSection* currentSection = nullptr;
	const int length = fileStr.Length();
	for( int strPos = 0; strPos < length; ) {
		strPos = skipWhitespaceAndComments( fileStr, strPos );

		CUnicodeString newSectionName;
		if( parseSection( fileStr, strPos, newSectionName ) ) {
			currentSection = &createSection( newSectionName );
			continue;
		}
		if( parseNamedSection( fileStr, strPos, newSectionName ) ) {
			currentSection = &createNamedSection( newSectionName );
			continue;
		}

		CUnicodeString keyName;
		CUnicodeString valueName;
		if( parseKeyValuePair( fileStr, strPos, keyName, valueName ) ) {
			setNewValue( currentSection, keyName, valueName );
			continue;
		}
		check( false, Err_BadMessageFile, fileName, strPos );
	}
}

int CMessageFile::skipWhitespaceAndComments( CUnicodeView str, int pos )
{
	pos = skipWhitespace( str, pos );
	while( str[pos] == L';' || ( str[pos] == L'/' && str[pos + 1] == L'/' ) ) {
		do {
			pos++;
		} while( str[pos] != 0 && str[pos] != L'\n' );
		pos = skipWhitespace( str, pos );
	}

	return pos;
}

int CMessageFile::skipWhitespace( CUnicodeView str, int pos )
{
	while( CUnicodeString::IsCharWhiteSpace( str[pos] ) ) {
		pos++;
	}

	return pos;
}

// Try and get a key-value pair from contents. If contents don't contain a valid pair, return false.
bool CMessageFile::parseKeyValuePair( CUnicodeView str, int& pos, CUnicodeString& key, CUnicodeString& value ) const
{
	assert( key.IsEmpty() );
	assert( value.IsEmpty() );

	const int colonIndex = str.Find( L':', pos );
	if( colonIndex == NotFound || colonIndex == pos ) {
		return false;
	}
	key = str.Mid( pos, colonIndex - pos );
	if( !checkValidKeyName( key ) ) {
		return false;
	}

	pos = colonIndex;
	value = parseValueFromString( str, pos );
	return true;
}

// Parse a given contents and retrieve the message value from it.
// The starting point is in contents at index contentPos in position strPos.
CUnicodeString CMessageFile::parseValueFromString( CUnicodeView contents, int& pos ) const
{
	const bool openQuoteFound = findOpenQuotePos( contents, pos );
	check( openQuoteFound, Err_BadMessageFile, fileName, pos );

	CUnicodeString result;
	do {
		result += parseValueInQuotes( contents, pos );
	} while( findOpenQuotePos( contents, pos ) );
	
	replaceSpecialSymbols( result );
	return result;
}

// Search given contents for an open quotation mark. Return a success of the search.
const wchar_t quote = L'\"';
bool CMessageFile::findOpenQuotePos( CUnicodeView contents, int& pos ) const
{
	pos = skipWhitespaceAndComments( contents, pos + 1 );
	return contents[pos] == quote;
}

// Search given contents for a closed quotation mark. Return the string value between the marks.
extern const CError Err_CloseQuoteNotFound( L"A closed quotation mark has not been found in a message file.\nFile name: %0." );
CUnicodeString CMessageFile::parseValueInQuotes( CUnicodeView contents, int& pos ) const
{
	const int startPos = pos + 1;
	// Search for the quote in a current string.
	for( pos = contents.Find( quote, startPos ); pos != NotFound; pos = contents.Find( quote, pos + 1 ) ) {
		// Skip \" symbols.
		if( pos > 0 && contents[pos - 1] == L'\\' ) {
			continue;
		}
		// A closed quote is found.
		return UnicodeStr( contents.Mid( startPos, pos - startPos ) );
	}

	check( false, Err_CloseQuoteNotFound, fileName );
	return CUnicodeString();
}

// Replace all the special symbols in a value string.
extern const CError Err_InvalidSpecial( L"Message file value contains an invalid special symbol: \\%1.\nFile name: %0." );
void CMessageFile::replaceSpecialSymbols( CUnicodeString& str ) const
{
	for( int slashPos = str.Find( L'\\' ); slashPos != NotFound; slashPos = str.Find( L'\\', slashPos + 1 )  ) {
		// This should never happen as it means that value in the original file ends with \" which can't be the end.
		assert( slashPos + 1 < str.Length() );

		switch( str[slashPos + 1] ) {
			case L'\\':
				str.ReplaceAt( slashPos, L'\\', 2 );
				break;
			case L'n':
				str.ReplaceAt( slashPos, L'\n', 2 );
				break;
			case L'r':
				str.ReplaceAt( slashPos, L'\r', 2 );
				break;
			case L'\"':
				str.ReplaceAt( slashPos, L'\"', 2 );
				break;
			default:
				check( false, Err_InvalidSpecial, fileName, UnicodeStr( str[slashPos + 1] ) );
				break;
		}
	}
}

// Try and get a section name from str. If str doesn't contain a section name, return false.
bool CMessageFile::parseSection( CUnicodeView str, int& pos, CUnicodeString& section )
{
	if( str[pos] != L'[' ) {
		return false;
	}

	const int closePos = str.Find( L']', pos + 1 );
	if( closePos == NotFound ) {
		return false;
	}

	section = str.Mid( pos + 1, closePos - 1 - pos );
	pos = closePos + 1;
	return true;
}

bool CMessageFile::parseNamedSection( CUnicodeView str, int& pos, CUnicodeString& section )
{
	if( str[pos] != L'{' ) {
		return false;
	}

	const int closePos = str.Find( L'}', pos + 1 );
	if( closePos == NotFound ) {
		return false;
	}

	section = str.Mid( pos + 1, closePos - 1 - pos );
	pos = closePos + 1;
	return true;
}

const CError Err_DublicateSection{ L"File contains two sections with the name %1. File name: %0." };
// Create a section with a given name and return a pointer to it.
CMessageSection& CMessageFile::createSection( CUnicodeView name )
{
	check( sectionNames.Set( name ), Err_DublicateSection, fileName, name );
	sections.IncreaseSize( sections.Size() + 1 );
	sections.Last().SetName( name );
	return sections.Last();
}

CMessageSection& CMessageFile::createNamedSection( CUnicodeView name )
{
	namedSections.IncreaseSize( namedSections.Size() + 1 );
	namedSections.Last().SetName( name );
	return namedSections.Last();
}

bool CMessageFile::checkValidKeyName( CUnicodeView keyName )
{
	assert( !keyName.IsEmpty() );
	if( !isCharValidVariableSymbol( keyName[0] ) ) {
		return false;
	}

	const int length = keyName.Length();
	for( int i = 1; i < length; i++ ) {
		if( !isCharValidVariableSymbol( keyName[i] ) && !CUnicodeString::IsCharDigit( keyName[i] ) )
			return false;
	}

	return true;
}

bool CMessageFile::isCharValidVariableSymbol( wchar_t ch )
{
	return ( ch >= L'a' && ch <= L'z' ) || ( ch >= L'A' && ch <= L'Z' ) || ch == L'_';
}

extern const CError Err_UnnamedSection{ L"Key with the name %1 does not belong to a section. File name: %0." };
extern const CError Err_DuplicateKey{ L"Message section %1 contains two keys with the name %2. File name: %0." };
void CMessageFile::setNewValue( CMessageSection* section, CUnicodePart key, const CUnicodeString& value )
{
	check( section != nullptr, Err_UnnamedSection, fileName, key );
	messageCount++;
	totalSize += value.Length() * sizeof( wchar_t );

	const CUnicodePart pureKey = deleteWhitespace( key );
	check( section->SetString( pureKey, value ), Err_DuplicateKey, fileName, UnicodeStr( section->GetName() ), key );
}

//////////////////////////////////////////////////////////////////////////

}	// namespace Msg.
