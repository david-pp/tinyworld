struct PlayerBinDescriptor {
	typedef Player ObjectType;
	typedef bin::Player ProtoType;
	
	static void obj2proto(ProtoType& proto, const ObjectType& object) {
		proto.set_id(object.id);
		proto.set_name(object.name);
		proto.set_score(object.score);
		proto.set_bin(object.bin);
		proto.set_v_int8(object.v_int8);
		proto.set_v_uint8(object.v_uint8);
		proto.set_v_int16(object.v_int16);
		proto.set_v_uint16(object.v_uint16);
		proto.set_v_int32(object.v_int32);
		proto.set_v_uint32(object.v_uint32);
		proto.set_v_int64(object.v_int64);
		proto.set_v_uint64(object.v_uint64);
		proto.set_v_float(object.v_float);
		proto.set_v_double(object.v_double);
		proto.set_v_bool(object.v_bool);
		proto.set_v_string(object.v_string);
		proto.set_v_char(object.v_char);
		proto.set_v_bytes(object.v_bytes);
		proto.set_v_bytes_tiny(object.v_bytes_tiny);
		proto.set_v_bytes_medium(object.v_bytes_medium);
		proto.set_v_bytes_long(object.v_bytes_long);
	}
	
	static void proto2obj(ObjectType& object, const ProtoType& proto) {
		object.id = proto.id();
		strncpy(object.name, proto.name().c_str(), sizeof(object.name)-1);
		object.score = proto.score();
		object.bin = proto.bin();
		object.v_int8 = proto.v_int8();
		object.v_uint8 = proto.v_uint8();
		object.v_int16 = proto.v_int16();
		object.v_uint16 = proto.v_uint16();
		object.v_int32 = proto.v_int32();
		object.v_uint32 = proto.v_uint32();
		object.v_int64 = proto.v_int64();
		object.v_uint64 = proto.v_uint64();
		object.v_float = proto.v_float();
		object.v_double = proto.v_double();
		object.v_bool = proto.v_bool();
		object.v_string = proto.v_string();
		strncpy(object.v_char, proto.v_char().c_str(), sizeof(object.v_char)-1);
		object.v_bytes = proto.v_bytes();
		object.v_bytes_tiny = proto.v_bytes_tiny();
		object.v_bytes_medium = proto.v_bytes_medium();
		object.v_bytes_long = proto.v_bytes_long();
	}
};
