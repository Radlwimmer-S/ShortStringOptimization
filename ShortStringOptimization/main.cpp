
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <iostream>

#include <assert.h>

#define _CRT_SECURE_NO_WARNINGS

class String
{
#if defined(_DEBUG)
public:
#endif // defined(DEBUG)

	//sizeof(String) - sizeof(SSO::length) - sizeof( '\0' )
	static const size_t MAX_SHORT_STRING_LENGTH = (sizeof(char*) + sizeof(size_t)) - sizeof(uint32_t) - 1;

	union SSO
	{
		SSO(const String* str) : string(str)
		{ }

		const String* string;
		struct ShortString
		{
			uint32_t length;
			char string[MAX_SHORT_STRING_LENGTH + 1];
		}*shortString;
	};

	void Init(const char* str, size_t length)
	{
		if (length <= MAX_SHORT_STRING_LENGTH)
		{
			SSO sso(this);
			memcpy(&sso.shortString->string, str, length + 1u);
		}
		else
		{
			m_string = Clone(str, length);
		}
	}

	uint32_t GetShortLength(void) const
	{
		const SSO sso(this);

		return sso.shortString->length;
	}

public:
	explicit String(const char* str)
		: m_length(strlen(str))
	{
		Init(str, m_length);
	}

	String(const String& other)
		: m_length(other.GetShortLength())
	{
		Init(other.c_str(), m_length);
	}

	String& operator=(const String& other)
	{
		if (this != &other)
		{
			if (GetShortLength() > MAX_SHORT_STRING_LENGTH)
				delete[] m_string;

			m_length = other.GetShortLength();
			Init(other.c_str(), m_length);
		}

		return *this;
	}

	~String(void)
	{
		if (GetLength() > MAX_SHORT_STRING_LENGTH)
			delete[] m_string;
	}

	void Concatenate(const char* str)
	{
		Concatenate(str, strlen(str));
	}

	void Concatenate(const String& str)
	{
		Concatenate(str.c_str(), str.GetLength());
	}

	String& operator+=(const char* str)
	{
		Concatenate(str);
		return *this;
	}

	String& operator+=(const String& str)
	{
		Concatenate(str);
		return *this;
	}

	String operator+(const char* str) const
	{
		String temp(*this);
		temp += str;
		return temp;
	}

	String operator+(const String& str) const
	{
		String temp(*this);
		temp += str;
		return temp;
	}

	explicit operator const char*(void) const
	{
		return c_str();
	}

	// returns length of string without null terminator
	size_t GetLength(void) const
	{
		return GetShortLength();
	}

	const char* c_str(void) const
	{
		const SSO sso(this);

		//Return SSO-Pointer
		if (sso.shortString->length <= MAX_SHORT_STRING_LENGTH)
			return sso.shortString->string;

		return m_string;
	}

private:
	void Concatenate(const char* str, size_t otherLength)
	{
		assert("Given string may not be null" && (str != nullptr));

		const uint32_t length = GetShortLength();

		//Was SSO and will stay SSO
		if (length + otherLength <= MAX_SHORT_STRING_LENGTH)
		{
			const SSO sso(this);

			memcpy(sso.shortString->string + length, str, otherLength);

			m_length += otherLength;

			return;
		}
		//Concatinated doesn't fit SSO
		else
		{
			const size_t neededBytes = length + otherLength + 1u;
			char* newString = new char[neededBytes];

			memcpy(newString, c_str(), length);

			// copies null terminator as well
			memcpy(newString + length, str, otherLength + 1u);

			if (length > MAX_SHORT_STRING_LENGTH)
				delete[] m_string;

			m_string = newString;

			m_length = length + otherLength;
		}
	}

	static char* Clone(const char* str, size_t length)
	{
		assert("Given string may not be null" && (str != nullptr));

		char* clone = new char[length + 1u];

		// copies null terminator as well
		memcpy(clone, str, length + 1u);

		return clone;
	}

	size_t m_length;
	char* m_string;
};


int main(void)
{
	uint32_t rawLenght = String::MAX_SHORT_STRING_LENGTH;
	char* raw = new char[rawLenght + 1];
	raw[rawLenght] = '\0';
	_strset_s(raw, rawLenght + 1, 'a');

	std::cout << "Testing String-SSO!" << std::endl << std::endl;

	//

	String string(raw);
	String::SSO sso(&string);

	assert("'explicit String(const char* str)' failed to copy string as SSO!" && strcmp(sso.shortString->string, raw) == 0);
	assert("'explicit String(const char* str)' failed to set length correctly!" && string.GetLength() == rawLenght);
	assert("'explicit String(const char* str)' failed to store as SSO!" && reinterpret_cast<uintptr_t>(string.c_str()) == reinterpret_cast<uintptr_t>(sso.shortString->string));

	std::cout << "'explicit String(const char* str)' works!" << std::endl;

	//

	String copy(string);
	String::SSO ssoCopy(&copy);

	assert("'String(const String& other)' failed to copy string as SSO!" && strcmp(string.c_str(), copy.c_str()) == 0);
	assert("'String(const String& other)' failed to set length correctly!" && copy.GetLength() == string.GetLength());
	assert("'String(const String& other)' failed to store as SSO!" && reinterpret_cast<uintptr_t>(copy.c_str()) == reinterpret_cast<uintptr_t>(ssoCopy.shortString->string));

	std::cout << "'String(const String& other)' works!" << std::endl;

	//

	String concat("");
	String::SSO ssoConcat(&concat);
	concat.Concatenate(raw);

	assert("'void String::Concatenate(const char* str)' failed to copy string as SSO!" && strcmp(concat.c_str(), raw) == 0);
	assert("'void String::Concatenate(const char* str)' failed to set length correctly!" && concat.GetLength() == rawLenght);
	assert("'void String::Concatenate(const char* str)' failed to store as SSO!" && reinterpret_cast<uintptr_t>(concat.c_str()) == reinterpret_cast<uintptr_t>(ssoConcat.shortString->string));

	std::cout << "'void String::Concatenate(const char* str)' works!" << std::endl;

	//

	uint32_t raw2Lenght2 = 2 * String::MAX_SHORT_STRING_LENGTH;
	char* raw2 = new char[raw2Lenght2 + 1];
	raw2[raw2Lenght2] = '\0';
	_strset_s(raw2, raw2Lenght2 + 1, 'a');

	std::cout << std::endl << "Testing String-HeapAlloc!" << std::endl << std::endl;

	//

	concat.Concatenate(raw);

	assert("'void String::Concatenate(const char* str)' failed to copy string to heap!" && strcmp(concat.c_str(), raw2) == 0);
	assert("'void String::Concatenate(const char* str)' failed set length correctly!" && concat.GetLength() == raw2Lenght2);
	assert("'void String::Concatenate(const char* str)' failed to store on heap!" && reinterpret_cast<uintptr_t>(concat.c_str()) != reinterpret_cast<uintptr_t>(ssoConcat.shortString->string));

	std::cout << "'void String::Concatenate(const char* str)' works!" << std::endl;

	//

	copy = concat;

	assert("'String& operator=(const String& other)' failed to copy string!" && strcmp(concat.c_str(), copy.c_str()) == 0);
	assert("'String& operator=(const String& other)' failed set length correctly!" && concat.GetLength() == copy.GetLength());
	assert("'String& operator=(const String& other)' failed to store on heap!" && reinterpret_cast<uintptr_t>(copy.c_str()) != reinterpret_cast<uintptr_t>(ssoCopy.shortString->string));

	std::cout << "'String& operator=(const String& other)' works!" << std::endl;

	//

	String copy2(copy);
	String::SSO ssoCopy2(&copy2);

	assert("'String(const String& other)' failed to copy string!" && strcmp(copy2.c_str(), copy.c_str()) == 0);
	assert("'String(const String& other)' failed set length correctly!" && copy2.GetLength() == copy.GetLength());
	assert("'String(const String& other)' failed to store on heap!" && reinterpret_cast<uintptr_t>(copy2.c_str()) != reinterpret_cast<uintptr_t>(ssoCopy2.shortString->string));

	std::cout << "'String(const String& other)' works!" << std::endl;

	std::cout << std::endl << "Everything works!" << std::endl;
}