
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <iostream>

#include <assert.h>

// ##########################################################################################################
// #																										#
// # Assuming every 'new char[n]' returns 2-alligned adress we can use one bit as SSO-Flag					#
// #  -) 1 --> HeapString, 0 --> ShortString																#
// #  -) little endian (LE): using MSB as flag and discard the LSB (which is allways 0)						#
// #  -) big endian (BE): using LSB as flag as it is allways 0												#
// #																										#
// # Using the last byte of m_data.shortstring for meta data: +---------+---+---+---+---+---+---+---------+ #
// #  -) 1 bit as SSO-Flag (SSO)							  |   MSB	| Empty	|     length    |   LSB   |	#
// #  -) 4 bits as inversed length (L)						  | SSO(LE) | 0 | 0 | L | L | L | L | SSO(BE) |	#
// # 														  +---------+---+---+---+---+---+---+---------+	#
// #  Properties:																							#
// #  -) Works with big endian and little endian (via compiler flag)										#
// #  -) Max heap string length is size_t																	#
// #  -) Max short stirng length is sizeof(String) - 1														#
// # 																										#
// ##########################################################################################################

//Reminder: little endian --> LSB is lowest adressed byte (left) & MSB is hightest adressed byte (right)
#define IS_LITTLE_ENDIAN

//Activating Asserts
#define TEST

namespace Bit
{
	template<typename T>
	union Ptr
	{
		T* as_ptr;
		uintptr_t as_int;
	};

	template<uint8_t bit, typename T>
	static bool IsBitSet(T value)
	{
		return value & (1ull << bit);
	}

	template<uint8_t bit, typename T>
	static void SetBit(T& value)
	{
		value |= (1ull << bit);
	}

	template<uint8_t bit, typename T>
	static void UnsetBit(T& value)
	{
		value &= ~(1ull << bit);
	}

	static const uint8_t CHAR_MSB = 8 * sizeof(char) - 1;
	static const uint8_t CHAR_LSB = 0;
}

class String
{
#pragma region SSO

#if defined(TEST)
public:
#endif // defined(TEST)
	struct HeapStringData
	{
		size_t m_length;
		char* m_string;
		
		void EncodeStringAdress(char* str)
		{
			Bit::Ptr<char> ptr = { str };
#if defined(IS_LITTLE_ENDIAN)
			//Use MSB as flag --> discard LSB
			ptr.as_int = ptr.as_int >> 1;
#else
			//Use LSB as flag --> Set to 1
			++ptr.as_int;
#endif			
			m_string = ptr.as_ptr;
		}

		char* DecodeStringAdress() const
		{
			Bit::Ptr<char> ptr = { m_string };
#if defined(IS_LITTLE_ENDIAN)
			//Use MSB as flag --> discard MSB
			ptr.as_int = ptr.as_int << 1;
#else
			//Used LSB as flag --> Set to 0
			--ptr.as_int;
#endif			
			return ptr.as_ptr;
		}
	};

	static const size_t MAX_SSO_LENGTH = sizeof(HeapStringData) - 1;
	static const uint8_t SSO_FLAG_BIT =
#if defined(IS_LITTLE_ENDIAN)
		//Using the MSB because on little endian it is in the highest adressed byte (most right)
		Bit::CHAR_MSB;
#else
		//Using the LSB because on bit endian it is in the highest adressed byte (most right)
		Bit::CHAR_LSB;
#endif

	union Data
	{
		HeapStringData heapString;
		char shortString[MAX_SSO_LENGTH + 1];
	};

	bool IsSso() const
	{
		return !Bit::IsBitSet<SSO_FLAG_BIT>(m_data.shortString[MAX_SSO_LENGTH]);
	}

	void SetSsoFlag(bool isSso)
	{
		if (isSso)
		{
			Bit::UnsetBit<SSO_FLAG_BIT>(m_data.shortString[MAX_SSO_LENGTH]);
		}
		else
		{
			Bit::SetBit<SSO_FLAG_BIT>(m_data.shortString[MAX_SSO_LENGTH]);
		}
	}

	char GetSsoLength() const
	{
		const char inverseLength = m_data.shortString[MAX_SSO_LENGTH] >> 1ui8;
		return MAX_SSO_LENGTH - inverseLength;
	}

	void SetSsoLength(size_t length)
	{
		const char inverseLength = static_cast<char>(MAX_SSO_LENGTH - length);
		m_data.shortString[MAX_SSO_LENGTH] = inverseLength << 1ui8;
	}

private:
#pragma endregion

	void Init(const char* str, size_t length)
	{
		if (length <= MAX_SSO_LENGTH)
		{
			memcpy(m_data.shortString, str, length + 1u);
			SetSsoLength(length);
			SetSsoFlag(true);
		}
		else
		{
			m_data.heapString.EncodeStringAdress(Clone(str, length));
			m_data.heapString.m_length = length;
			SetSsoFlag(false);
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
			if (!IsSso())
				delete[] m_data.heapString.DecodeStringAdress();

			Init(other.c_str(), other.GetLength());
		}

		return *this;
	}

	~String(void)
	{
		if (!IsSso())
			delete[] m_data.heapString.DecodeStringAdress();
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
		if (IsSso())
		{
			return GetSsoLength();
		}
		else
		{
			return m_data.heapString.m_length;
		}
	}

	const char* c_str(void) const
	{
		if (IsSso())
		{
			return m_data.shortString;
		}
		else
		{
			return m_data.heapString.DecodeStringAdress();
		}
	}

private:
	void Concatenate(const char* otherStr, size_t otherLength)
	{
		assert("Given string may not be null" && (otherStr != nullptr));

		const size_t length = GetLength();
		const size_t newLength = length + otherLength;

		const char* str = c_str();

		if (newLength <= MAX_SSO_LENGTH)
		{
			memcpy(m_data.shortString + length, otherStr, otherLength + 1u);
			SetSsoLength(newLength);
			SetSsoFlag(true);
		}
		else
		{
			char* newString = new char[newLength + 1u];
			memcpy(newString, str, length);

			// copies null terminator as well
			memcpy(newString + length, otherStr, otherLength + 1u);

			if (!IsSso())
				delete[] m_data.heapString.DecodeStringAdress();

			m_data.heapString.EncodeStringAdress(newString);
			m_data.heapString.m_length = newLength;
			SetSsoFlag(false);
		}
	}

	static char* Clone(const char* str, size_t length)
	{
		assert("Given string may not be null" && (str != nullptr));

		char* clone = new char[length + 1u];
		Bit::Ptr<char> ptr = { clone };
		assert("Expecting 'new char[N]' to return 2-alligned address" && (ptr.as_int & 1) == false);

		// copies null terminator as well
		memcpy(clone, str, length + 1u);

		return clone;
	}

	Data m_data;
};


//Reminder: little endian --> LSB is left & MSB is right

int main(void)
{
	const size_t rawLenght = sizeof(String) - 1;
	char* raw = new char[rawLenght + 1];
	raw[rawLenght] = '\0';
	_strset_s(raw, rawLenght + 1, 'a');

	std::cout << "Testing String-SSO!" << std::endl << std::endl;

	{
		String string(raw);
#if defined(TEST)
		String::Data* sso = reinterpret_cast<String::Data*>(&string);

		assert("'explicit String(const char* str)' failed to copy string as SSO!" && strcmp(sso->shortString, raw) == 0);
		assert("'explicit String(const char* str)' failed to set length correctly!" && string.GetLength() == rawLenght);
		assert("'explicit String(const char* str)' failed to store as SSO!" && reinterpret_cast<uintptr_t>(string.c_str()) == reinterpret_cast<uintptr_t>(sso->shortString));
#endif
		std::cout << "'explicit String(const char* str)' works!" << std::endl;
	}

	{
		String string(raw);
		String copy(string);
#if defined(TEST)
		String::Data* ssoCopy = reinterpret_cast<String::Data*>(&copy);

		assert("'String(const String& other)' failed to copy string as SSO!" && strcmp(string.c_str(), copy.c_str()) == 0);
		assert("'String(const String& other)' failed to set length correctly!" && copy.GetLength() == string.GetLength());
		assert("'String(const String& other)' failed to store as SSO!" && reinterpret_cast<uintptr_t>(copy.c_str()) == reinterpret_cast<uintptr_t>(ssoCopy->shortString));
#endif
		std::cout << "'String(const String& other)' works!" << std::endl;
	}

	{
		String concat("");
		concat.Concatenate(raw);
#if defined(TEST)
		String::Data* ssoConcat = reinterpret_cast<String::Data*>(&concat);

		assert("'void String::Concatenate(const char* str)' failed to copy string as SSO!" && strcmp(concat.c_str(), raw) == 0);
		assert("'void String::Concatenate(const char* str)' failed to set length correctly!" && concat.GetLength() == rawLenght);
		assert("'void String::Concatenate(const char* str)' failed to store as SSO!" && reinterpret_cast<uintptr_t>(concat.c_str()) == reinterpret_cast<uintptr_t>(ssoConcat->shortString));
#endif
		std::cout << "'void String::Concatenate(const char* str)' works!" << std::endl;
	}


	const size_t raw2Lenght = 2 * rawLenght;
	char* raw2 = new char[raw2Lenght + 1];
	raw2[raw2Lenght] = '\0';
	_strset_s(raw2, raw2Lenght + 1, 'a');

	std::cout << std::endl << "Testing String-HeapAlloc!" << std::endl << std::endl;

	//
	{
		String concat(raw);
		concat.Concatenate(raw);
#if defined(TEST)
		String::Data* ssoConcat = reinterpret_cast<String::Data*>(&concat);

		assert("'void String::Concatenate(const char* str)' failed to copy string to heap!" && strcmp(concat.c_str(), raw2) == 0);
		assert("'void String::Concatenate(const char* str)' failed set length correctly!" && concat.GetLength() == raw2Lenght);
		assert("'void String::Concatenate(const char* str)' failed to store on heap!" && reinterpret_cast<uintptr_t>(concat.c_str()) != reinterpret_cast<uintptr_t>(ssoConcat->shortString));
#endif
		std::cout << "'void String::Concatenate(const char* str)' works!" << std::endl;
	}

	{
		String string(raw2);
		String assign("");
		assign = string;
#if defined(TEST)
		String::Data* ssoAssign = reinterpret_cast<String::Data*>(&assign);

		assert("'String& operator=(const String& other)' failed to copy string!" && strcmp(string.c_str(), assign.c_str()) == 0);
		assert("'String& operator=(const String& other)' failed set length correctly!" && string.GetLength() == assign.GetLength());
		assert("'String& operator=(const String& other)' failed to store on heap!" && reinterpret_cast<uintptr_t>(assign.c_str()) != reinterpret_cast<uintptr_t>(ssoAssign->shortString));
#endif
		std::cout << "'String& operator=(const String& other)' works!" << std::endl;
	}

	{
		String string(raw2);
		String copy(string);
#if defined(TEST)
		String::Data* ssoCopy = reinterpret_cast<String::Data*>(&copy);

		assert("'String(const String& other)' failed to copy string!" && strcmp(string.c_str(), copy.c_str()) == 0);
		assert("'String(const String& other)' failed set length correctly!" && string.GetLength() == copy.GetLength());
		assert("'String(const String& other)' failed to store on heap!" && reinterpret_cast<uintptr_t>(copy.c_str()) != reinterpret_cast<uintptr_t>(ssoCopy->shortString));
#endif
		std::cout << "'String(const String& other)' works!" << std::endl;
	}

	std::cout << std::endl << "Everything works!" << std::endl << std::endl;
}