#pragma once

#include "RecordDefinition.h"

#include <string>
#include <vector>

namespace GreenFrog
{

class Field
{
protected:
	const FieldDefinition *_fieldDefinition;

	std::string _value;
	size_t *_requiredFieldCounter;
	size_t *_fieldErrorCounter;
	bool _isNull;
	bool _isSet;
	bool _hasError;
	friend class Record;

	void set(const char *begin, const char *end);
	void setError(const char *begin, const char *end);
	void setFlags(bool setFlag, bool hasError);
public:
	static const int BoolNull = -1;
	static const Datetime DatetimeNull;
	static const double DoubleNull;
	static const int64_t IntegerNull = 0xFFFFFFFFFFFFFFFFUL;
	static const std::string StringNull;
	static const std::string StringTrue;
	static const std::string StringFalse;
	static const std::vector<std::string> StringVectorNull;
	static const std::vector<Record*> RecordVectorNull;

	Field(const FieldDefinition *fieldDefinition = 0) :
		_fieldDefinition(fieldDefinition), _requiredFieldCounter(0), _fieldErrorCounter(0), _isNull(true), _isSet(false), _hasError(false) {}
	void initialize(const FieldDefinition *fieldDefinition) {_fieldDefinition = fieldDefinition;}
	void setRequiredFieldCounter(size_t *requiedFieldCounter) { _requiredFieldCounter = requiedFieldCounter; }
	void setFieldErrorCounter(size_t *fieldErrorCounter) { _fieldErrorCounter = fieldErrorCounter; }
	bool hasError() const { return _hasError; }
	bool isRequired() const {return _fieldDefinition->isRequired();}
	bool isSet() const {return _isSet;}
	unsigned key() const {return _fieldDefinition->key();}
	FieldType type() const {return _fieldDefinition->type();}
	const std::string& name() const {return _fieldDefinition->name();}
	void reset();
	const std::string& valueAsString() { return _isSet ? _value : StringNull; }
	
	virtual int set(const char *begin, const char *end, EventMessage &msg);
	int set(const char *value, EventMessage &msg) { return set(value, value + strlen(value), msg); }
	virtual char* toJson(char *buffer, char *bufferEnd);
	virtual char* appendFix(char *buffer, char *bufferEnd, char delimiter, uint8_t &checkSum);

	virtual const Field& operator=(const Field &rhs);

	virtual ~Field() {}
};

class DatetimeField : public Field
{
protected:
	Datetime _datetime;
public:
	int set(const char *begin, const char *end, EventMessage &msg) override;
	void set(const Datetime &datetime);
	const Datetime& valueAsDatetime() const { return _isSet ? _datetime : DatetimeNull; }
	char* toJson(char *buffer, char *bufferEnd) override;

	const Field& operator=(const Field &rhs) override;
};

class DoubleField : public Field
{
protected:
	double _double;
public:
	int set(const char *begin, const char *end, EventMessage &msg) override;
	void set(double value);
	double valueAsDouble() const { return _isSet ? _double : DoubleNull; }
	int64_t valueAsInt() const { return _isSet ? static_cast<int64_t>(_double) : IntegerNull; }

	const Field& operator=(const Field &rhs) override;
};

class IntegerField : public Field
{
protected:
	int64_t _integer;
public:
	int set(const char *begin, const char *end, EventMessage &msg) override;
	void set(int64_t value);
	double valueAsDouble() const { return _isSet ? static_cast<double>(_integer) : DoubleNull; }
	int64_t valueAsInteger() const { return _isSet ? _integer : IntegerNull; }

	const Field& operator=(const Field &rhs) override;
};

class StringField : public Field
{
public:
	void set(const std::string &value);
	char* toJson(char *buffer, char *bufferEnd) override;
};

class StringVectorField : public Field
{
protected:
	std::vector<std::string> _values;
public:
	int set(const char *begin, const char *end, EventMessage &msg) override;
	void set(const std::vector<std::string> &values);
	std::vector<std::string>& valueAsStringVector() { return _values; }
	char* toJson(char *buffer, char *bufferEnd) override;

	const Field& operator=(const Field &rhs) override;
};

/*
** Field
*/
inline
void Field::reset()
{
	if (_isSet)
	{
		_isSet = false;
		if (_requiredFieldCounter)--*_requiredFieldCounter;
	}
	if (_hasError)
	{
		_hasError = false;
		if (_fieldErrorCounter)--*_fieldErrorCounter;
	}
}

inline
void Field::set(const char *begin, const char *end)
{
	if (begin < end)
	{
		_value.assign(begin, end - begin);
		_isNull = false;
	}
	else
	{
		_value.clear();
		_isNull = true;
	}
	if (!_isSet)
	{
		_isSet = true;
		if (_requiredFieldCounter)++*_requiredFieldCounter;
	}
	if (_hasError)
	{
		_hasError = false;
		if (_fieldErrorCounter)--*_fieldErrorCounter;
	}
}

inline
void Field::setError(const char *begin, const char *end)
{
	_value.assign(begin, end - begin);
	if (_isSet)
	{
		_isSet = false;
		if (_requiredFieldCounter) --*_requiredFieldCounter;
	}
	if (!_hasError)
	{
		_hasError = true;
		if (_fieldErrorCounter)++*_fieldErrorCounter;
	}
}

inline
void Field::setFlags(bool setFlag, bool hasError)
{
	if (_isSet != setFlag)
	{
		_isSet = setFlag;
		if (_requiredFieldCounter)
		{
			if(setFlag) --*_requiredFieldCounter;
			else ++*_requiredFieldCounter;
		}
	}
	if (_hasError != hasError)
	{
		_hasError = hasError;
		if (_fieldErrorCounter)
		{
			if(hasError) ++*_fieldErrorCounter;
			else --*_fieldErrorCounter;
		}
	}
}
/*
** DatetimeField
*/

inline
void DatetimeField::set(const Datetime &datetime)
{
	_datetime = datetime;
	char buffer[64];
	char *ptr(buffer - 1);
	ptr = append(ptr, ptr + sizeof(buffer), _fieldDefinition->format().data(), _datetime);
	Field::set(buffer, ++ptr);
}

/*
** DoubleField
*/
inline
void DoubleField::set(double value)
{
	_double = value;
	char buffer[32];
	char *ptr(buffer - 1);
	ptr = append(ptr, ptr + sizeof(buffer), _double);
	Field::set(buffer, ++ptr);
}

/*
** IntegerField
*/
inline
void IntegerField::set(int64_t value)
{
	_integer = value;
	char buffer[24];
	char *ptr(buffer - 1);
	ptr = append(ptr, ptr + sizeof(buffer), _integer);
	Field::set(buffer, ++ptr);
}

/*
** StringField
*/
inline
void StringField::set(const std::string &value)
{
	Field::set(value.data(), value.data() + value.size());
}

/*
** StringVectorField
*/
inline
void StringVectorField::set(const std::vector<std::string> &values)
{
	_values = values;
	_value.clear();
	for (std::vector<std::string>::iterator itr = _values.begin(); itr != _values.end(); ++itr)
	{
		if (_value.size()) _value.append(",", 1);
		_value.append(*itr);
	}
	if (!_isSet)
	{
		_isSet = true;
		if (_requiredFieldCounter)++*_requiredFieldCounter;
	}
	_isNull = _values.size() == 0;
}

} // namespace GreenFrog