#pragma once

namespace Msg {

//////////////////////////////////////////////////////////////////////////
// A single section in a .msg file.
class CMessageSection {
public:
	explicit CMessageSection() = default;
	CMessageSection( CMessageSection&& other );

	// Section name.
	CStringView GetName() const
		{ return name; }
	void SetName( CUnicodeView newValue )
		{ name = Str( newValue ); }

	// Set key value. If the key was already present, do not set the value and return false.
	bool SetString( CUnicodePart keyName, CUnicodePart newValue );

	// Get all section keys.
	const CArray<CUnicodeString>& GetKeyNames() const
		{ return keyNames; }
	// Get all section values.
	const CArray<CUnicodeString>& GetKeyValues() const
		{ return keyValues; }

private:
	CString name;
	CArray<CUnicodeString> keyNames;
	CArray<CUnicodeString> keyValues;
	// Set of all keys to ensure that each key is unique.
	CHashTable<CUnicodeString> uniqueKeys;

	// Copying is prohibited.
	CMessageSection( CMessageSection& ) = delete;
	void operator=( CMessageSection& ) = delete;
};

//////////////////////////////////////////////////////////////////////////

// .msg file wrapper.
// All the message files are divided into sections in brackets or braces.
// Each section contains a message name and its value in quotes separated by a colon. Example:
// MessageName: "MessageText"
// Several values in a row are concatenated.
// Values can contain a newline escape symbol: "\r\n". This symbol is replaced by an actual newline.
// Comments in the file start with either ; or //
class CMessageFile {
public:
	explicit CMessageFile( CUnicodeView fileName );

	// Total count of all messages.
	int MessageCount() const
		{ return messageCount; }
	// Size of all the messages in bytes.
	int MessageBinarySize() const
		{ return totalSize; }

	// Enumerate all sections.
	CArrayView<CMessageSection> GetUnnamedSections() const
		{ return sections; }
	CArrayView<CMessageSection> GetNamedSections() const
		{ return namedSections; }

private:
	// Name of the file.
	CUnicodeString fileName;
	// List of unnamed sections in the file.
	CArray<CMessageSection> sections;
	// List of named sections in the file.
	CArray<CMessageSection> namedSections;
	// A set of existing section names to ensure that each section is unique.
	CHashTable<CUnicodeString> sectionNames;
	// Size of all the message values in bytes.
	int totalSize = 0;
	// Count of all the messages in the file.
	int messageCount = 0;

	void parseFile();
	static int skipWhitespace( CUnicodeView str, int pos );
	static int skipWhitespaceAndComments( CUnicodeView str, int pos );
	bool parseKeyValuePair( CUnicodeView contents, int& pos, CUnicodeString& key, CUnicodeString& value ) const;
	CUnicodeString parseValueFromString( CUnicodeView contents, int& pos ) const;
	bool findOpenQuotePos( CUnicodeView contents, int& pos ) const;
	CUnicodeString parseValueInQuotes( CUnicodeView contents, int& pos ) const;

	void setNewValue( CMessageSection* section, CUnicodePart key, const CUnicodeString& value );

	void replaceSpecialSymbols( CUnicodeString& str ) const;
	static bool parseSection( CUnicodeView str, int& pos, CUnicodeString& section );
	static bool parseNamedSection( CUnicodeView str, int& pos, CUnicodeString& section );
	CMessageSection& createSection( CUnicodeView name );
	CMessageSection& createNamedSection( CUnicodeView name );

	static bool checkValidKeyName( CUnicodeView keyName );
	static bool isCharValidVariableSymbol( wchar_t ch );
};

//////////////////////////////////////////////////////////////////////////

}	// namespace Msg.

