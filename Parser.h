#pragma once

#include "EventMessage.h"

namespace GreenFrog
{

class Parser
{
protected:
	const char *_begin;
	const char *_end;
	const char *_failedAt;

	const char* parseString(const char *input, const char *&begin, const char *&end, EventMessage &msg);
public:
	// methods used by other parsers
	void setEnd(const char *end) {_end = end;}
	virtual const char* parseValue(const char *input, EventMessage &msg) = 0;
	const char* failedAt() { return _failedAt; }
	void failedAt(size_t &line, size_t &character);

	virtual ~Parser() {}
};

inline
const char* Parser::parseString(const char *input, const char *&begin, const char *&end, EventMessage &msg)
{
	// "..."
	
	// skip white space
	for (; isspace(*input);)
	{
		if (!(++input < _end))
		{
			msg.start(_LOCATION_, "end of input before start of string '\"'");
			_failedAt = _end;
			return 0;
		}
	}

	if (*input != '"')
	{
		msg.start(_LOCATION_, "expecting start of string '\"': found '%s'", *input);
		_failedAt = input;
		return 0;
	}

	begin = ++input;

	for (; *input != '"';)
	{
		if (*input == '\\') ++input;
		if (!(++input < _end))
		{
			msg.start(_LOCATION_, "end of input before end of string '\"'");
			_failedAt = input;
			return 0;
		}
	}

	end = input;

	return ++input;
}

} // namespace GreenFrog