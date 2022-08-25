#include "FixRecordParser.h"

namespace GreenFrog
{

const char* FixRecordParser::parsePair(const char *input, EventMessage &msg)
{
	// tag = value <delimiter>

	// tag
	const char *tagBegin(input);
	while (*input != '=')
	{
		if (!(++input < _end))
		{
			msg.start(_LOCATION_, "end of input parsing tag");
			_failedAt = _end;
			return 0;
		}
	}
	const char *tagEnd(input);
	Field *field = _record->getField(tagBegin, tagEnd);

	// skip =
	if (!(++input < _end))
	{
		char tag[32];
		msg.start(_LOCATION_, "tag [%s]: end of input parsing value", strcpy_s(tag, sizeof(tag), tagBegin, tagEnd));
		_failedAt = _end;
		return 0;
	}

	// value
	const char *begin(input);
	while (*input != _delimiter)
	{
		if (!(++input < _end))
		{
			char tag[32];
			msg.start(_LOCATION_, "tag [%s]: end of input parsing value", strcpy_s(tag, sizeof(tag), tagBegin, tagEnd));
			_failedAt = _end;
			return 0;
		}
	}
	if (field)
	{
		if (field->type() != FieldType::RECORDVECTOR)
		{
			if (field->set(begin, input, msg))
			{
				msg.prefix(_LOCATION_, "tag [%s]", field->name().data());
				_failedAt = begin;
				return 0;
			}
		}
		else
		{
			// parse repeating group
			int64_t count;
			if (GreenFrog::parse(begin, input, count) != input)
			{
				char value[32];
				msg.start(_LOCATION_, "tag [%s]: unable to parse number of repeating groups from [%s]", field->name().data(), strcpy_s(value, sizeof(value), begin, input));
				_failedAt = begin;
				return 0;
			}
			if (!(++input < _end) && count)
			{
				msg.start(_LOCATION_, "tag [%s]: end of input parsing repeating group", field->name().data());
				_failedAt = _end;
				return 0;
			}
			if (!(input = parseRepeatingGroup(input, static_cast<RecordVectorField*>(field), count, msg)))
			{
				msg.prefix(_LOCATION_, "tag [%s]", field->name().data());
				return 0;
			}
		}
	}

	return ++input;
}

const char* FixRecordParser::parseRepeatingGroup(const char *input, RecordVectorField *recordVectorField, size_t count, EventMessage &msg)
{
	CharRange delimiterTag, tag;
	Record *record(0);
	Field *field;
	for (;;)
	{
		// tag
		tag._begin = input;
		while (*input != '=')
		{
			if (!(++input < _end))
			{
				msg.start(_LOCATION_, "end of input parsing tag");
				_failedAt = _end;
				return 0;
			}
		}
		tag._end = input;

		// skip =
		if (!(++input < _end))
		{
			char fieldName[32];
			msg.start(_LOCATION_, "tag [%s]: end of input parsing value", strcpy_s(fieldName, sizeof(fieldName), tag._begin, tag._end));
			_failedAt = _end;
			return 0;
		}

		if (!record || delimiterTag == tag)
		{
			delimiterTag = tag;
			record = recordVectorField->nextRecord();
		}
		if(! (field = record->getField(tag._begin, tag._end))) break;

		// value
		const char *begin(input);
		while (*input != _delimiter)
		{
			if (!(++input < _end))
			{
				msg.start(_LOCATION_, "tag [%s]: end of input parsing value", field->name().data());
				_failedAt = _end;
				return 0;
			}
		}

		if (field->type() != FieldType::RECORDVECTOR)
		{
			if (field->set(begin, input, msg))
			{
				msg.prefix(_LOCATION_, "tag [%s]", field->name().data());
				_failedAt = begin;
				return 0;
			}
			++input;
		}
		else
		{
			// parse repeating group
			int64_t count;
			if (GreenFrog::parse(begin, input, count) != input)
			{
				char value[32];
				msg.start(_LOCATION_, "tag [%s]: unable to parse number of repeating groups from [%s]", field->name().data(), strcpy_s(value, sizeof(value), begin, input));
				_failedAt = begin;
				return 0;
			}
			if (!(++input < _end) && count)
			{
				msg.start(_LOCATION_, "tag [%s]: end of input parsing repeating group", field->name().data());
				_failedAt = _end;
				return 0;
			}
			if (!(input = parseRepeatingGroup(input, static_cast<RecordVectorField*>(field), count, msg)))
			{
				msg.prefix(_LOCATION_, "tag [%s]", field->name().data());
				return 0;
			}
		}
	}
	if (count != recordVectorField->valueAsRecordVector().size())
	{
		msg.start(_LOCATION_, "expected %lu groups: found %lu", count, recordVectorField->valueAsRecordVector().size());
		_failedAt = delimiterTag._begin;
		return 0;
	}
	return tag._begin - 1;
}

const char* FixRecordParser::parseValue(const char *input, EventMessage &msg)
{
	while (input < _end)
	{
		if(! (input = parsePair(input, msg)))
			break;
	}
	return input;
}

const char *FixRecordParser::parse(Record *record, const char *input, size_t bytes, EventMessage &msg)
{
	_record = record;
	_begin = input;
	_end = input + bytes;
	while (input < _end)
	{
		if (!(input = parsePair(input, msg)))
		{
			if (_failedAt)
			{
				size_t lineCnt, characterCnt;
				failedAt(lineCnt, characterCnt);
				msg.prefix(_LOCATION_, "parseValue: line %lu character %lu", lineCnt, characterCnt);
			}
			msg.prefix(_LOCATION_, "parsePair");
			return 0;
		}
	}
	return input;
}

} // namespace GreenFrog