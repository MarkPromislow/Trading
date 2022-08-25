#include "Field.h"

namespace GreenFrog
{

namespace
{
const Datetime& datetimeNull()
{
	static Datetime datetime;
	memset(&datetime, 0, sizeof(Datetime));
	return datetime;
}

char* toJsonTag(char *buffer, char *bufferEnd, const std::string &tag)
{
	*++buffer = '"';
	for (const char *ptr(tag.data()), *end(tag.data() + tag.size()); ptr < end;)
		*++buffer = *ptr++;
	*++buffer = '"';
	*++buffer = ':';
	return buffer;
}
}

const Datetime Field::DatetimeNull(datetimeNull());
const double Field::DoubleNull(std::numeric_limits<double>::quiet_NaN());
const std::string Field::StringNull("null");
const std::string Field::StringTrue("true");
const std::string Field::StringFalse("false");
const std::vector<std::string> Field::StringVectorNull;
const std::vector<Record*> Field::RecordVectorNull;

int Field::set(const char *begin, const char *end, EventMessage &msg)
{
	set(begin, end);
	return 0;
}

const Field& Field::operator=(const Field &rhs)
{
	_value = rhs._value;
	_isNull = rhs._isNull;
	_isSet = rhs._isSet;
	_hasError = rhs._hasError;

	return *this;
}

char *Field::toJson(char *buffer, char *bufferEnd)
{
	if (_isSet)
	{
		buffer = toJsonTag(buffer, bufferEnd, _fieldDefinition->name());
		for (const char *itr(_value.data()), *end(_value.data() + _value.size()); itr < end;)
			*++buffer = *itr++;
		*++buffer = ',';
	}
	return buffer;
}

char* Field::appendFix(char *buffer, char *bufferEnd, char delimiter, uint8_t &checkSum)
{
	if (_isSet)
	{
		size_t bytes = bufferEnd - buffer;
		const std::string &fileName(name());
		if(bytes > fileName.size()) bytes = fileName.size();
		for(const char *itr(fileName.data()), *end(fileName.data() + bytes); itr < end;)
			checkSum += *++buffer = *itr++;
		if(buffer < bufferEnd) checkSum += *++buffer = '=';
		bytes = bufferEnd - buffer;
		if(bytes > _value.size()) bytes = _value.size();
		for(const char *itr(_value.data()), *end(_value.data() + bytes); itr < end;)
			checkSum += *++buffer = *itr++;
		if(buffer < bufferEnd) checkSum += *++buffer = delimiter;
	}
	return buffer;
}

/*
** DatetimeField
*/
int DatetimeField::set(const char *begin, const char *end, EventMessage &msg)
{
	int result(0);
	Field::set(begin, end);
	if (parse(begin, end, _fieldDefinition->format().data(), _datetime) != end)
	{
		if (begin < end && strcasecmp(_value.data(), "null"))
		{
			setFlags(false, true);
			msg.start(_LOCATION_, "field %s: set Datetime from [%s]: failed", _fieldDefinition->name().data(), _value.data());
			result = DATA_ERROR;
		}
		else
			setFlags(false, false);
	}
	return 0;
}

char* DatetimeField::toJson(char *buffer, char *bufferEnd)
{
	if (_isSet)
	{
		buffer = toJsonTag(buffer, bufferEnd, _fieldDefinition->name());
		*++buffer = '"';
		for (const char *itr(_value.data()), *end(_value.data() + _value.size()); itr < end;)
			*++buffer = *itr++;
		*++buffer = '"';
		*++buffer = ',';
	}
	return buffer;
}

const Field& DatetimeField::operator=(const Field &rhs)
{
	Field::operator=(rhs);
	_datetime = static_cast<const DatetimeField*>(&rhs)->_datetime;
	return *this;
}

/*
** DoubleField
*/
int DoubleField::set(const char *begin, const char *end, EventMessage &msg)
{
	if (parse(begin, end, _double) != end)
	{
		setError(begin, end);
		char value[64];
		msg.start(_LOCATION_, "field %s: set Double from [%s]: failed", _fieldDefinition->name().data(), strcpy_s(value, sizeof(value), begin, end));
		return DATA_ERROR;
	}
	Field::set(begin, end);
	return 0;
}

const Field& DoubleField::operator=(const Field &rhs)
{
	Field::operator=(rhs);
	_double = static_cast<const DoubleField*>(&rhs)->_double;
	return *this;

}

/*
** IntegerField
*/
int IntegerField::set(const char *begin, const char *end, EventMessage &msg)
{
	if (parse(begin, end, _integer) != end)
	{
		setError(begin, end);
		char value[64];
		msg.prefix(_LOCATION_, "field %s: set Integer from [%s]: failed", _fieldDefinition->name().data(), strcpy_s(value, sizeof(value), begin, end));
		return DATA_ERROR;
	}
	Field::set(begin, end);
	return 0;
}

const Field& IntegerField::operator=(const Field &rhs)
{
	Field::operator=(rhs);
	_integer = static_cast<const IntegerField*>(&rhs)->_integer;
	return *this;
}

/*
** StringField
*/
char* StringField::toJson(char *buffer, char *bufferEnd)
{
	if (_isSet)
	{
		buffer = toJsonTag(buffer, bufferEnd, _fieldDefinition->name());
		*++buffer = '"';
		for (const char *itr(_value.data()), *end(_value.data() + _value.size()); itr < end;)
			*++buffer = *itr++;
		*++buffer = '"';
		*++buffer = ',';
	}
	return buffer;
}

const Field& StringVectorField::operator=(const Field &rhs)
{
	Field::operator=(rhs);
	_values = static_cast<const StringVectorField*>(&rhs)->_values;
	return *this;
}

/*
** StringVectorField
*/
int StringVectorField::set(const char *begin, const char *end, EventMessage &msg)
{
	if (!_isSet)
	{
		_value.clear();
		_values.clear();
		_isSet = true;
		if (_requiredFieldCounter) ++_requiredFieldCounter;
	}
	else if (_value.size())
		_value.append(',', 1);
	size_t size(end - begin);
	_value.append(begin, size);
	_values.emplace_back(begin, size);
	_isNull = false;
	return 0;
}

char* StringVectorField::toJson(char *buffer, char *bufferEnd)
{
	if (_isSet)
	{
		buffer = toJsonTag(buffer, bufferEnd, _fieldDefinition->name());
		*++buffer = '[';
		if (_values.size())
		{
			for (const std::string value : _values)
			{
				*++buffer = '"';
				for (const char *itr(value.data()), *end(value.data() + value.size()); itr < end;)
					*++buffer = *itr++;
				*++buffer = '"';
				*++buffer = ',';
			}
			*buffer = ']';
		}
		else
			*++buffer = ']';
		*++buffer = ',';
	}
	return buffer;
}

} // namespace GreenFrog