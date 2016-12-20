#ifndef __COMMON_SERIALIZABLE_H
#define __COMMON_SERIALIZABLE_H

#include <string>
#include <algorithm>
#include <map>
#include "serializable.in.h"

//////////////////////////////////////////////////////
//
// Serializable Interface, Using protobuf(by david++)
//  inherit this, and implement both serialize & 
//  deserialize virtual functions.
//
//////////////////////////////////////////////////////

template <typename ProtoType>
class Serializable 
{
public:
	typedef ProtoType Serializer;

	bool saveToString(std::string& bin)
	{
		Serializer data;
		this->serialize(&data);
		return data.SerializeToString(&bin);
	}

	bool parseFromString(const std::string& bin)
	{
		Serializer data;
		if (data.ParseFromString(bin))
		{
			this->deserialize(&data);
			return true;
		}
		return false;
	}

	virtual void serialize(Serializer* data) = 0;
	virtual void deserialize(const Serializer* data) = 0;

public:
	template <typename NestedType, typename ProtoT>
	void set_nested(NestedType& nested, ProtoT* data)
	{
		nested.serialize(data);
	}

	template <typename SeqCont, typename ProtoT>
	void set_sequence(SeqCont& seq, ProtoT* data)
	{
		for (typename SeqCont::iterator it = seq.begin(); it != seq.end(); ++ it)
			Helper::serialize(*it, data->Add());
	}

	template <typename KeyT, typename ValueT, typename ProtoT>
	void set_map(std::map<KeyT, ValueT>& cont, ProtoT* data)
	{
		for (typename std::map<KeyT, ValueT>::iterator it = cont.begin(); 
				it != cont.end(); ++ it)
		{
			data->Add();
			Helper::serialize_map_key(it->first, data->Mutable(data->size()-1));
			Helper::serialize_map_value(it->second, data->Mutable(data->size()-1));
		}
	}

	template <typename KeyT, typename ValueT, typename ProtoT>
	void set_map(std::multimap<KeyT, ValueT>& cont, ProtoT* data)
	{
		for (typename std::multimap<KeyT, ValueT>::iterator it = cont.begin(); 
				it != cont.end(); ++ it)
		{
			data->Add();
			Helper::serialize_map_key(it->first, data->Mutable(data->size()-1));
			Helper::serialize_map_value(it->second, data->Mutable(data->size()-1));
		}
	}


	template <typename NestedType, typename ProtoT>
	void get_nested(NestedType& nested, const ProtoT& data)
	{
		nested.deserialize(&data);
	}

	template <typename NestedType, typename SeqCont, typename ProtoT>
	void get_sequence(SeqCont& seq, const ProtoT& data)
	{
		for (int i = 0; i < data.size(); ++ i)
		{
			NestedType object;
			Helper::deserialize(object, data.Get(i));
			seq.insert(seq.end(), object);
		}
	}

	template <typename KeyT, typename ValueT, typename ProtoT>
	void get_map(std::map<KeyT, ValueT>& cont, const ProtoT& data)
	{
		for (int i = 0; i < data.size(); ++ i)
		{
			KeyT key;
			ValueT value;
			Helper::deserialize_map_key(key, data.Get(i).key());
			Helper::deserialize_map_value(value, data.Get(i).value());
			cont[key] = value;
		}
	}

	template <typename KeyT, typename ValueT, typename ProtoT>
	void get_map(std::multimap<KeyT, ValueT>& cont, const ProtoT& data)
	{
		for (int i = 0; i < data.size(); ++ i)
		{
			KeyT key;
			ValueT value;
			Helper::deserialize_map_key(key, data.Get(i).key());
			Helper::deserialize_map_value(value, data.Get(i).value());
			cont.insert(std::make_pair(key, value));
		}
	}
};

#define SERIALIZE_SCALAR(var) data->set_##var(var)
#define SERIALIZE_NESTED(var) this->set_nested(var, data->mutable_##var())
#define SERIALIZE_MAP(var) this->set_map(var, data->mutable_##var())
#define SERIALIZE_SEQ(var) this->set_sequence(var, data->mutable_##var())

#define DESERIALIZE_SCALAR(var) this->var = data->var()
#define DESERIALIZE_NESTED(var) this->get_nested(var, data->var())
#define DESERIALIZE_MAP(var) this->get_map(var, data->var())
#define DESERIALIZE_SEQ(var, NestedType) this->get_sequence<NestedType>(var, data->var())


#endif // __COMMON_SERIALIZABLE_H