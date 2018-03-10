
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <iostream>

#include <assert.h>

#define _CRT_SECURE_NO_WARNINGS

//  struct ShortString Data-Layout
//  +-----------+--------+----------------+--------+
//  |   1 Bit   |  7 Bit | 6 bzw. 14 Byte | 1 Byte |
//  | IsSS-Flag | length |     string     |  '\0'  |
//  +-----------+--------+----------------+--------+
//  
//  class String Data-Layout
//  +-----------+--------------------+-------------+
//  | ( LSB = 0 )         char*      |    size_t   |
//  | (2 aligned)       m_string     |   m_length  |
//  +-----------+--------------------+-------------+
//						   |
//						   |		+-----------+----------------+--------+-----------+
//						   +------->|  0-1 Byte | char[m_length] | 1 Byte |  0-1 Byte |
//						    		| Alignment |   raw string   |  '\0'  |    Empty  |
//						    		+-----------+----------------+--------+-----------+

class String
{
#if defined(_DEBUG)
public:
#endif // defined(DEBUG)
	static const size_t MAX_SHORT_STRING_LENGTH = (sizeof(char*) + sizeof(size_t)) - 2;

	union SSO
	{
		SSO(const String* str) : string(str)
		{ }

		const String* string;
		struct ShortString
		{
			char isShortString : 1;
			char length : 7;
			char string[MAX_SHORT_STRING_LENGTH + 1];
		}*shortString;
	};

	void Init(const char* str, size_t length)
	{
		SSO sso(this);

		if (length <= MAX_SHORT_STRING_LENGTH)
		{
			memcpy(&sso.shortString->string, str, length + 1u);
			sso.shortString->length = length;
			sso.shortString->isShortString = 1;
		}
		else
		{
			m_string = Clone(str, length);
			m_length = length;
			sso.shortString->isShortString = 0;
		}
	}

public:
	explicit String(const char* str)
	{
		Init(str, strlen(str));
	}

	String(const String& other)
	{
		Init(other.c_str(), other.GetLength());
	}

	String& operator=(const String& other)
	{
		if (this != &other)
		{
			const SSO sso(this);

			if (!sso.shortString->isShortString)
				FreeAlignedString(m_string, m_length);

			Init(other.c_str(), other.GetLength());
		}

		return *this;
	}

	~String(void)
	{
		const SSO sso(this);

		if (!sso.shortString->isShortString)
			FreeAlignedString(m_string, m_length);
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
		const SSO sso(this);

		return sso.shortString->isShortString ? sso.shortString->length : m_length;
	}

	const char* c_str(void) const
	{
		const SSO sso(this);

		return sso.shortString->isShortString ? sso.shortString->string : m_string;
	}

private:
	void Concatenate(const char* str, size_t otherLength)
	{
		assert("Given string may not be null" && (str != nullptr));

		const size_t thisLength = GetLength();
		const size_t newLength = thisLength + otherLength;

		const SSO sso(this);

		if (newLength <= MAX_SHORT_STRING_LENGTH)
		{
			memcpy(sso.shortString->string + thisLength, str, otherLength + 1u);
			sso.shortString->length += static_cast<char>(otherLength);
			sso.shortString->isShortString = 1;
		}
		else
		{
			char* newString = GetAlignedMemory(newLength);
			memcpy(newString, c_str(), thisLength);
			memcpy(newString + thisLength, str, otherLength + 1u);

			if (!sso.shortString->isShortString)
				FreeAlignedString(m_string, m_length);

			m_string = newString;
			m_length = newLength;
			sso.shortString->isShortString = 0;
		}
	}

	static char* GetAlignmentFlag(char* str, size_t length)
	{
		return str + (length + 1);
	}

	static char* GetAlignedMemory(size_t length)
	{
		char* aligned = new char[length + 3u]; //+1 for '\0'; +1 for alignment; +1 to safe alignmentOffset

		const uintptr_t cloneAdress = reinterpret_cast<uintptr_t>(aligned);

		if (cloneAdress & 1)
		{
			++aligned;
			*GetAlignmentFlag(aligned, length) = 1;
		}
		else
		{
			*GetAlignmentFlag(aligned, length) = 0;
		}

		return aligned;
	}

	//copies the given string to a 2-aligned memory
	static char* Clone(const char* str, size_t length)
	{
		assert("Given string may not be null" && (str != nullptr));

		char* clone = GetAlignedMemory(length); 

		// copies null terminator as well
		memcpy(clone, str, length + 1u);

		return clone;
	}

	static void FreeAlignedString(char* str, size_t length)
	{
		const char isAligned = *GetAlignmentFlag(str, length);

		if (isAligned)
			str--;

		delete[] str;
	}

	char* m_string;
	size_t m_length;
};

int main(void)
{
	uint32_t rawLenght = String::MAX_SHORT_STRING_LENGTH;
	char* raw = new char[rawLenght + 1];
	raw[rawLenght] = '\0';
	_strset_s(raw, rawLenght + 1, 'a');

	std::cout << "Testing String-SSO!" << std::endl << std::endl;


	{
		String string(raw);
		String::SSO sso(&string);

		assert("'explicit String(const char* str)' failed to copy string as SSO!" && strcmp(sso.shortString->string, raw) == 0);
		assert("'explicit String(const char* str)' failed to set length correctly!" && string.GetLength() == rawLenght);
		assert("'explicit String(const char* str)' failed to store as SSO!" && reinterpret_cast<uintptr_t>(string.c_str()) == reinterpret_cast<uintptr_t>(sso.shortString->string));

		std::cout << "'explicit String(const char* str)' works!" << std::endl;
	}

	{
		String string(raw);
		String copy(string);
		String::SSO ssoCopy(&copy);

		assert("'String(const String& other)' failed to copy string as SSO!" && strcmp(string.c_str(), copy.c_str()) == 0);
		assert("'String(const String& other)' failed to set length correctly!" && copy.GetLength() == string.GetLength());
		assert("'String(const String& other)' failed to store as SSO!" && reinterpret_cast<uintptr_t>(copy.c_str()) == reinterpret_cast<uintptr_t>(ssoCopy.shortString->string));

		std::cout << "'String(const String& other)' works!" << std::endl;
	}

	{
		String concat("");
		concat.Concatenate(raw);
		String::SSO ssoConcat(&concat);

		assert("'void String::Concatenate(const char* str)' failed to copy string as SSO!" && strcmp(concat.c_str(), raw) == 0);
		assert("'void String::Concatenate(const char* str)' failed to set length correctly!" && concat.GetLength() == rawLenght);
		assert("'void String::Concatenate(const char* str)' failed to store as SSO!" && reinterpret_cast<uintptr_t>(concat.c_str()) == reinterpret_cast<uintptr_t>(ssoConcat.shortString->string));

		std::cout << "'void String::Concatenate(const char* str)' works!" << std::endl;
	}


	uint32_t raw2Lenght = 2 * String::MAX_SHORT_STRING_LENGTH;
	char* raw2 = new char[raw2Lenght + 1];
	raw2[raw2Lenght] = '\0';
	_strset_s(raw2, raw2Lenght + 1, 'a');

	std::cout << std::endl << "Testing String-HeapAlloc!" << std::endl << std::endl;

	//
	{
		String concat(raw);
		concat.Concatenate(raw);
		String::SSO ssoConcat(&concat);

		assert("'void String::Concatenate(const char* str)' failed to copy string to heap!" && strcmp(concat.c_str(), raw2) == 0);
		assert("'void String::Concatenate(const char* str)' failed set length correctly!" && concat.GetLength() == raw2Lenght);
		assert("'void String::Concatenate(const char* str)' failed to store on heap!" && reinterpret_cast<uintptr_t>(concat.c_str()) != reinterpret_cast<uintptr_t>(ssoConcat.shortString->string));

		std::cout << "'void String::Concatenate(const char* str)' works!" << std::endl;
	}

	{
		String string(raw2);
		String assign("");
		assign = string;
		String::SSO ssoAssign(&assign);

		assert("'String& operator=(const String& other)' failed to copy string!" && strcmp(string.c_str(), assign.c_str()) == 0);
		assert("'String& operator=(const String& other)' failed set length correctly!" && string.GetLength() == assign.GetLength());
		assert("'String& operator=(const String& other)' failed to store on heap!" && reinterpret_cast<uintptr_t>(assign.c_str()) != reinterpret_cast<uintptr_t>(ssoAssign.shortString->string));

		std::cout << "'String& operator=(const String& other)' works!" << std::endl;
	}

	{
		String string(raw2);
		String copy(string);
		String::SSO ssoCopy(&copy);

		assert("'String(const String& other)' failed to copy string!" && strcmp(string.c_str(), copy.c_str()) == 0);
		assert("'String(const String& other)' failed set length correctly!" && string.GetLength() == copy.GetLength());
		assert("'String(const String& other)' failed to store on heap!" && reinterpret_cast<uintptr_t>(copy.c_str()) != reinterpret_cast<uintptr_t>(ssoCopy.shortString->string));

		std::cout << "'String(const String& other)' works!" << std::endl;
	}

	std::cout << std::endl << "Everything works!" << std::endl;
}