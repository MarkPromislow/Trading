#include "Record.h"

namespace GreenFrog
{

Record::Record(Record &&record) :
	_requiredFieldCnt(record._requiredFieldCnt),
	_requiredFieldCounter(record._requiredFieldCounter),
	_fieldErrorCounter(record._fieldErrorCounter),
	_recordDefinition(record._recordDefinition),
	_fields(std::move(record._fields)),
	target(record.target)
{
	for (Field *field : _fields)
	{
		if(field->isRequired()) field->setRequiredFieldCounter(&_requiredFieldCounter);
		field->setFieldErrorCounter(&_fieldErrorCounter);
	}
}

int Record::addField(Field *field)
{
	if (field->isRequired())
	{
		field->setRequiredFieldCounter(&_requiredFieldCounter);
		++_requiredFieldCnt;
	}
	field->setFieldErrorCounter(&_fieldErrorCounter);
	_fields[field->key()] = field;
	return 0;
}

Record::Record() : _requiredFieldCnt(0), _requiredFieldCounter(0), _fieldErrorCounter(0), _recordDefinition(0), target(0) {}

size_t Record::fieldErrors(std::string &errors) const
{
	errors.clear();
	if (_fieldErrorCounter)
	{
		static std::string delimiter(", ");
		for (const Field *field : _fields)
		{
			if (field->hasError())
			{
				if (errors.size()) errors += delimiter;
				errors += field->name() + " [" + field->_value + ']';
			}
		}
	}
	return _fieldErrorCounter;
}

size_t Record::missingFields(std::string &fieldNames) const
{
	fieldNames.clear();
	size_t missingCnt;
	if ((missingCnt = _requiredFieldCnt - _requiredFieldCounter))
	{
		static std::string delimiter(", ");
		for (const Field *field : _fields)
		{
			if (field->isRequired() && !field->isSet())
			{
				if (fieldNames.size()) fieldNames += delimiter;
				fieldNames += field->name();
			}
		}
	}
	return missingCnt;
}

} // namespace GreenFrog