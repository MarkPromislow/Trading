#include "Parser.h"

namespace GreenFrog
{

void Parser::failedAt(size_t &line, size_t &character)
{
	line = 1;
	character = 0;
	for (const char *ptr = _begin; ptr < _failedAt; ++ptr)
	{
		++character;
		if (*ptr == '\n')
		{
			++line;
			character = 0;
		}
	}
}

} // namespace GreenFrog