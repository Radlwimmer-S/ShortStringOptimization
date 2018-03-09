
#include <string.h>
#include <assert.h>


class String
{
public:
	explicit String(const char* str)
		: m_length(strlen(str))
		, m_string(Clone(str, m_length))
	{
	}

	String(const String& other)
		: m_length(other.m_length)
		, m_string(Clone(other.m_string, other.m_length))
	{
	}

	String& operator=(const String& other)
	{
		if (this != &other)
		{
			delete[] m_string;
			m_length = other.m_length;
			m_string = Clone(other.m_string, other.m_length);
		}

		return *this;
	}

	~String(void)
	{
		delete[] m_string;
	}

	void Concatenate(const char* str)
	{
		Concatenate(str, strlen(str));
	}

	void Concatenate(const String& str)
	{
		Concatenate(str.m_string, str.m_length);
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
		return m_length;
	}

	const char* c_str(void) const
	{
		return m_string;
	}

private:
	void Concatenate(const char* str, size_t length)
	{
		assert("Given string may not be null" && (str != nullptr));
	
		const size_t neededBytes = m_length + length + 1u;
		char* newString = new char[neededBytes];
		memcpy(newString, m_string, m_length);

		// copies null terminator as well
		memcpy(newString + m_length, str, length + 1u);

		delete[] m_string;
		m_string = newString;
		m_length += length;
	}

	static char* Clone(const char* str, size_t length)
	{
		assert("Given string may not be null" && (str != nullptr));

		char* clone = new char [length + 1u];

		// copies null terminator as well
		memcpy(clone, str, length + 1u);

		return clone;
	}	

	size_t m_length;
	char* m_string;
};
