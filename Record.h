#pragma once

#include "Field.h"

namespace GreenFrog
{

class Record
{
protected:
	size_t _requiredFieldCnt;
	size_t _requiredFieldCounter;
	size_t _fieldErrorCounter;

	const RecordDefinition *_recordDefinition;
	std::vector<Field*> _fields;

	friend class RecordDefinition;
	int addField(Field *field);
public:
	Record();
	Record(Record&&);

	const std::string& name() const { return _recordDefinition->name();}
	void setRecordDefinition(const RecordDefinition *recordDefinition) { _recordDefinition = recordDefinition; _fields.resize(_recordDefinition->fieldCnt()); }
	const RecordDefinition* recordDefinition() {return _recordDefinition;}

	size_t fieldCnt() const { return _fields.size(); }
	Field* getField(unsigned fieldKey) const { return fieldKey < _fields.size() ? _fields[fieldKey] : 0; }
	Field* getField(const char *begin, const char *end) const;
	Field* getField(const char *fieldName) const;
	int setField(unsigned key, const char *value, size_t valueSize, EventMessage &msg);
	int setField(unsigned key, const char *value, EventMessage &msg) { return setField(key, value, strlen(value), msg); }
	int setField(unsigned key, const std::string &value, EventMessage &msg) {return setField(key, value.data(), value.size(), msg);}
	void reset();

	// are all reqired fields set
	bool isSet() const { return _requiredFieldCounter == _requiredFieldCnt; }
	// list of missing required fields
	size_t missingFields(std::string &fieldNames) const;
	// count field parsing errors
	size_t hasErrors() const { return _fieldErrorCounter; }
	// list of fields with parsing errors
	size_t fieldErrors(std::string &errors) const;

	const Record& operator=(const Record &rhs);

	char *appendJson(char *buffer, char *bufferEnd);
	char *toJson(char *buffer, size_t bufferSize);
	char *appendFix(char *buffer, char *bufferEnd, char delimiter, uint8_t &checkSum);
	char *toFix(char *buffer, size_t bufferSize, char delimiter, uint8_t &checkSum, uint64_t &bodyLength);

	void *target;
private:
	Record(const Record&) = delete;
};

inline
Field* Record::getField(const char *begin, const char *end) const
{
	unsigned fieldKey = _recordDefinition->key(begin, end);
	return fieldKey != RecordDefinition::NotDefined ? _fields[fieldKey] : 0;
}

inline
Field* Record::getField(const char *fieldName) const
{
	return getField(fieldName, fieldName + strlen(fieldName));
}

inline
void Record::reset()
{
	_requiredFieldCounter = _fieldErrorCounter = 0;
	for (Field *field : _fields)
	{
		field->_isSet = field->_hasError = false;
	}
	target = 0;
}

inline
int Record::setField(unsigned fieldKey, const char *value, size_t valueSize, EventMessage &msg)
{
	int result;
	if (fieldKey < _fields.size())
		result = _fields[fieldKey]->set(value, value + valueSize, msg);
	else
	{
		msg.start(_LOCATION_, "fieldKey %u is greater than the number of keys in the record %lu", fieldKey, _fields.size());
		result = CODE_ERROR;
	}
	return result;
}

inline
const Record& Record::operator=(const Record &rhs)
{
	if (_recordDefinition != rhs._recordDefinition) { reset(); return *this; }

	_requiredFieldCounter = rhs._requiredFieldCounter;
	_fieldErrorCounter = rhs._fieldErrorCounter;
	std::vector<Field*>::const_iterator rhsItr = rhs._fields.begin();
	for (Field *field : _fields)
	{
		*field = **rhsItr++;
		if (field->isRequired()) field->setRequiredFieldCounter(&_requiredFieldCounter);
		field->setFieldErrorCounter(&_fieldErrorCounter);
	}
	target = 0;
	return *this;
}

inline
char* Record::appendJson(char *buffer, char *bufferEnd)
{
	*buffer = '{';
	if (_fields.size())
	{
		for (std::vector<Field*>::const_iterator itr = _fields.begin(); itr != _fields.end(); ++itr)
			buffer = (*itr)->toJson(buffer, bufferEnd);
	}
	else
		++buffer;
	*buffer = '}';
	*++buffer = 0;
	return buffer;
}

inline
char* Record::toJson(char *buffer, size_t bufferSize)
{
	appendJson(buffer, buffer + bufferSize);
	return buffer;
}

inline
char* Record::appendFix(char *buffer, char *bufferEnd, char delimiter, uint8_t &checkSum)
{
	for (Field *field : _fields)
	{
		if (field->isSet())
			buffer = field->appendFix(buffer, bufferEnd, delimiter, checkSum);
	}
	return buffer;
}

inline
char *Record::toFix(char *buffer, size_t bufferSize, char delimiter, uint8_t &checkSum, uint64_t &bodyLength)
{
	checkSum = 0;
	char *ptr(buffer - 1);
	ptr = appendFix(ptr, ptr + bufferSize - 1, delimiter, checkSum);
	*++ptr = 0;
	bodyLength = ptr - buffer;
	return buffer;
}

} // namespace GreenFrog