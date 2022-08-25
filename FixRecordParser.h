#pragma once

#include "Record.h"
#include "RecordVectorField.h"
#include "Parser.h"

namespace GreenFrog
{

class FixRecordParser : public Parser
{
protected:
	Record *_record;
	char _delimiter;
	const char* parsePair(const char *input, EventMessage &msg);
	const char* parseRepeatingGroup(const char *input, RecordVectorField *field, size_t recordCount, EventMessage &msg);
	const char* parseValue(const char *input, EventMessage &msg) override;
public:
	FixRecordParser(char delimiter): _record(0), _delimiter(delimiter) {}
	const char* parse(Record *record, const char *input, size_t bytes, EventMessage &msg);
};

}