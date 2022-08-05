#include "utf8.hpp"

#include <cassert>

namespace stc
{

char32_t decode_utf8(std::string_view &str)
{
	assert(!str.empty());

	char32_t o1 = (char32_t)(unsigned char)str.front();
	if((o1 >> 6) != 0b11 || str.size() <= 1)
		return o1;

	str.remove_prefix(1);
	char32_t o2 = (char32_t)(unsigned char)str.front();
	if((o1 >> 5) != 0b111 || str.size() <= 1)
		return ((o1 & 0b11111) << 6) | (o2 & 0b111111);
	
	str.remove_prefix(1);
	char32_t o3 = (char32_t)(unsigned char)str.front();
	if((o1 >> 4) != 0b1111 || str.size() <= 1)
		return ((o1 & 0b1111) << 12) | ((o2 & 0b111111) << 6) | (o3 & 0b111111);
	
	str.remove_prefix(1);
	char32_t o4 = (char32_t)(unsigned char)str.front();
	return ((o1 & 0b111) << 18) | ((o2 & 0b111111) << 12) | ((o3 & 0b111111) << 6) | (o4 & 0b111111);
}

void encode_utf8(std::string &str, char32_t cp)
{
	if(cp < 0x80)
	{
		str.push_back((char)(unsigned char)cp);
	}
	else if(cp < 0x800)
	{
		str.push_back((char)(unsigned char)((cp >> 6) | 0xC0));
		str.push_back((char)(unsigned char)((cp & 0x3F) | 0x80));
	}
	else if(cp < 0x10000)
	{
		str.push_back((char)(unsigned char)((cp >> 12) | 0xe0));
		str.push_back((char)(unsigned char)(((cp >> 6) & 0x3F) | 0x80));
		str.push_back((char)(unsigned char)((cp & 0x3f) | 0x80));
	}
	else
	{
		str.push_back((char)(unsigned char)((cp >> 18) | 0xF0));
		str.push_back((char)(unsigned char)(((cp >> 12) & 0x3F) | 0x80));
		str.push_back((char)(unsigned char)(((cp >> 6) & 0x3F) | 0x80));
		str.push_back((char)(unsigned char)((cp & 0x3f) | 0x80));
	}
}


size_t utf8_line_column(std::string_view str)
{
	size_t column = 0;
	while(!str.empty() && //move backwards until begin of text
		   str.back() != '\n' && //or LF
		  (str.size() < 2 || str.end()[-1] != '\n' || str.end()[-2] != '\r')) //or CRLF
	{
		unsigned codeunits = 1;
		const char *cp_begin = &str.back();

		while(cp_begin != str.data() && ((unsigned char)*cp_begin >> 6 & 0b11) == 0b10) //find begin of UTF-8 sequence (higest bits != 10)
		{
			codeunits++;
			cp_begin--;
		}

		column++;
		str.remove_suffix(codeunits);
	}

	return column;
}

std::string_view utf8_complete_line(std::string_view str, size_t idx)
{
	const char *begin = str.data() + idx;
	while(begin > str.data() && //move backwards until begin of text
		  begin[-1] != '\n' && //or LF
		 (begin - str.data() < 2 || begin[-1] != '\n' || begin[-2] != '\r')) //or CRLF
	{
		begin--;
	}

	const char *end = str.data() + idx;
	while(end < &str.back() && *end != '\n') //end of text or new line
		end++;

	return std::string_view(begin, end - begin);
}


}