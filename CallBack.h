#pragma once

/*
** CallbackList
**   - subscribe and unsubscribe for data using any method
*/

#include <list>

template <typename ... Value>
class CallbackList
{
private:
	struct CallbackIf
	{
		virtual void callback(Value...) = 0;
		virtual bool equal(const void *obj, const void *fptr) const {return false;}
		virtual bool equal(const void *obj, const void *fptr, void *userPtr) const { return false; }
		virtual ~CallbackIf() {}
	};

	template<typename Type, typename ValueType>
	struct Callback : public CallbackIf
	{
		Type *_obj;
		void (ValueType::*_fptr)(Value...);

		Callback(Type *obj, void (ValueType::*fptr)(Value...)): _obj(obj), _fptr(fptr) {}
		bool equal(const void *obj, const void *fptr) const override {void *p; memcpy(&p, &_fptr, sizeof(p)); return _obj == obj && p == fptr;}
		void callback(Value ... value) override {(_obj->*_fptr)(value...);}
	};

	template<typename Type, typename ValueType, typename UserPtrType>
	struct CallbackWithUserPtr : public CallbackIf
	{
		Type *_obj;
		void (ValueType::*_fptr)(Value...);
		UserPtrType _userPtr;

		CallbackWithUserPtr(Type *obj, void (ValueType::*fptr)(Value...), UserPtrType userPtr) : _obj(obj), _fptr(fptr), _userPtr(userPtr) {}
		bool equal(const void *obj, const void *fptr, void *userPtr) const override { void *p; memcpy(&p, &_fptr, sizeof(p)); return _obj == obj && p == fptr && _userPtr = userPtr; }
		void callback(Value ... value) override { (_obj->*_fptr)(value..., _userPtr); }
	};

	std::list<CallbackIf*> _callbacks;
public:
	template<typename Type, typename ValueType>
	bool subscribe(Type *obj, void (ValueType::*fptr)(Value...));

	template<typename Type, typename ValueType>
	bool unsubscribe(Type *obj, void (ValueType::*fptr)(Value...));

	template<typename Type, typename ValueType, typename UserPtrType>
	bool subscribe(Type *obj, void (ValueType::*fptr)(Value...), UserPtrType userPtr);

	template<typename Type, typename ValueType, typename UserPtrType>
	bool unsubscribe(Type *obj, void (ValueType::*fptr)(Value...), UserPtrType userPtr);

	void distrubute(Value... value)
	{
		for(CallbackIf *cb : _callbacks)
			cb->callback(value...);
	}
};

template <typename ... Value>
template <typename Type, typename ValueType>
bool CallbackList<Value...>::subscribe(Type *obj, void (ValueType::*fptr)(Value...))
{
	for (CallbackIf *cb : _callbacks)
	{
		void *p;
		memcpy(&p, &fptr, sizeof(p));
		if(! cb->equal(obj, p)) continue;
		return false;
	}
	_callbacks.push_back(new Callback<Type, ValueType>(obj, fptr));
	return true;
}

template <typename ... Value>
template <typename Type, typename ValueType>
bool CallbackList<Value...>::unsubscribe(Type *obj, void (ValueType::*fptr)(Value...))
{
	bool result(false);
	for (typename std::list<CallbackList<Value...>::CallbackIf*>::iterator itr(_callbacks.begin()); itr != _callbacks.end(); ++itr)
	{
		CallbackIf *cb = *itr;
		void *p;
		memcpy(&p, &fptr, sizeof(p));
		if(cb->equal(obj, p))
		{
			_callbacks.erase(itr);
			delete cb;
			result = true;
			break;
		}
	}
	return result;
}

template <typename ... Value>
template <typename Type, typename ValueType, typename UserPtrType>
bool CallbackList<Value...>::subscribe(Type *obj, void (ValueType::*fptr)(Value...), UserPtrType userPtr)
{
	for (CallbackIf *cb : _callbacks)
	{
		void *p;
		memcpy(&p, &fptr, sizeof(p));
		if (!cb->equal(obj, p, userPtr)) continue;
		return false;
	}
	_callbacks.push_back(new Callback<Type, ValueType, UserPtrType>(obj, fptr, userPtr));
	return true;
}

template <typename ... Value>
template <typename Type, typename ValueType, typename UserPtrType>
bool CallbackList<Value...>::unsubscribe(Type *obj, void (ValueType::*fptr)(Value...), UserPtrType userPtr)
{
	bool result(false);
	for (CallbackIf *cb : _callbacks)
	{
		void *p;
		memcpy(&p, &fptr, sizeof(p));
		if (!cb->equal(obj, p, userPtr)) continue;
		_callbacks.erase(itr);
		delete cb;
		result = true;
		break;
	}
	return result;
}
