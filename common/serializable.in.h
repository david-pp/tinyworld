#ifndef __COMMON_SERIALIZABLE_IN_H
#define __COMMON_SERIALIZABLE_IN_H

namespace Helper
{
	template <typename ObjectT, typename ProtoT>
	inline void serialize_scalar(ObjectT& object, ProtoT* proto)
	{
		*proto = object;
	}

	template <typename ObjectT, typename ProtoT>
	inline void serialize_nested(ObjectT& object, ProtoT* proto)
	{
		object.serialize(proto);
	}


	template <typename ObjectT, typename ProtoT>
	inline void serialize(ObjectT& object, ProtoT* proto)
	{
		serialize_nested(object, proto);
	}

	template <typename KeyT, typename ProtoT>
	inline void serialize_map_key(const KeyT& key, ProtoT* proto)
	{
		const_cast<KeyT*>(&key)->serialize(proto->mutable_key());
	}

	template <typename ValueT, typename ProtoT>
	inline void serialize_map_value(ValueT& value, ProtoT* proto)
	{
		value.serialize(proto->mutable_value());
	}

	template <typename ObjectT, typename ProtoT>
	inline void deserialize(ObjectT& object, const ProtoT& proto)
	{
		object.deserialize(&proto);
	}

	template <typename KeyT, typename ProtoT>
	inline void deserialize_map_key(KeyT& key, const ProtoT& proto)
	{
		key.deserialize(&proto);
	}

	template <typename ValueT, typename ProtoT>
	inline void deserialize_map_value(ValueT& value, const ProtoT& proto)
	{
		value.deserialize(&proto);
	}


#define DECLARE_SERIALIZE_SCALAR(TYPE) \
	template <typename ProtoT> \
	inline void serialize(TYPE& object, ProtoT* data) { \
		serialize_scalar(object, data); \
	} \
	template <typename ProtoT> \
	inline void serialize_map_key(const TYPE& key, ProtoT* proto) { \
		proto->set_key(key); \
	} \
	template <typename ProtoT> \
	inline void serialize_map_value(TYPE& value, ProtoT* proto) { \
		proto->set_value(value); \
	} \
	template <typename ProtoT> \
	inline void deserialize(TYPE& object, const ProtoT& proto) { \
		object = proto; \
	} \
	template <typename ProtoT> \
	inline void deserialize_map_key(TYPE& key, const ProtoT& proto) { \
		key = proto; \
	} \
	template <typename ProtoT> \
	inline void deserialize_map_value(TYPE& value, const ProtoT& proto) { \
		value = proto; \
	}


	DECLARE_SERIALIZE_SCALAR(double);
	DECLARE_SERIALIZE_SCALAR(float);
	DECLARE_SERIALIZE_SCALAR(short);
	DECLARE_SERIALIZE_SCALAR(unsigned short);
	DECLARE_SERIALIZE_SCALAR(int);
	DECLARE_SERIALIZE_SCALAR(unsigned int);
	DECLARE_SERIALIZE_SCALAR(long);
	DECLARE_SERIALIZE_SCALAR(unsigned long);
	DECLARE_SERIALIZE_SCALAR(bool);
	DECLARE_SERIALIZE_SCALAR(std::string);

#undef DECLARE_SERIALIZE_SCALAR
};


#endif // __COMMON_SERIALIZABLE_IN_H