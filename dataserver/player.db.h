class PlayerDBDescriptor : public DBDescriptor {
public:
	typedef Player ObjectType;
	
	PlayerDBDescriptor() {
		table = "PLAYER";
		keys = {"ID"};
		fields = {
			{"ID", "int(10)     unsigned NOT NULL default '0'"},
			{"NAME", "varchar(32) NOT NULL default ''"},
			{"SCORE", "float NOT NULL default '0'"},
			{"BIN", "text"},
			{"V_INT8", "tinyint(3)  NOT NULL default '0'"},
			{"V_UINT8", "tinyint(3)  unsigned NOT NULL default '0'"},
			{"V_INT16", "smallint(5) NOT NULL default '0'"},
			{"V_UINT16", "smallint(5) unsigned NOT NULL default '0'"},
			{"V_INT32", "int(10)     NOT NULL default '0'"},
			{"V_UINT32", "int(10)     unsigned NOT NULL default '0'"},
			{"V_INT64", "bigint(20)  NOT NULL default '0'"},
			{"V_UINT64", "bigint(20)  unsigned NOT NULL default '0'"},
			{"V_FLOAT", "float NOT NULL default '0'"},
			{"V_DOUBLE", "double NOT NULL default '0'"},
			{"V_BOOL", "bool NOT NULL default '0'"},
			{"V_STRING", "text"},
			{"V_CHAR", "varchar(32) NOT NULL default ''"},
			{"V_BYTES", "blob"},
			{"V_BYTES_TINY", "tinyblob"},
			{"V_BYTES_MEDIUM", "mediumblob"},
			{"V_BYTES_LONG", "longblob"},
		};
	};
	
	void object2Query(mysqlpp::Query &query, const void* objptr) {
		const ObjectType& object = *(ObjectType*)objptr;
		query << object.id;
		query << ",";
		query << mysqlpp::quote << object.name;
		query << ",";
		query << object.score;
		query << ",";
		query << mysqlpp::quote << object.bin;
		query << ",";
		query << (int)object.v_int8;
		query << ",";
		query << (int)object.v_uint8;
		query << ",";
		query << object.v_int16;
		query << ",";
		query << object.v_uint16;
		query << ",";
		query << object.v_int32;
		query << ",";
		query << object.v_uint32;
		query << ",";
		query << object.v_int64;
		query << ",";
		query << object.v_uint64;
		query << ",";
		query << object.v_float;
		query << ",";
		query << object.v_double;
		query << ",";
		query << object.v_bool;
		query << ",";
		query << mysqlpp::quote << object.v_string;
		query << ",";
		query << mysqlpp::quote << object.v_char;
		query << ",";
		query << mysqlpp::quote << object.v_bytes;
		query << ",";
		query << mysqlpp::quote << object.v_bytes_tiny;
		query << ",";
		query << mysqlpp::quote << object.v_bytes_medium;
		query << ",";
		query << mysqlpp::quote << object.v_bytes_long;
	};
	
	void pair2Query(mysqlpp::Query &query, const void* objptr) {
		const ObjectType& object = *(ObjectType*)objptr;
		query << "`ID`=" << object.id;
		query << ",";
		query << "`NAME`=" << mysqlpp::quote << object.name;
		query << ",";
		query << "`SCORE`=" << object.score;
		query << ",";
		query << "`BIN`=" << mysqlpp::quote << object.bin;
		query << ",";
		query << "`V_INT8`=" << (int)object.v_int8;
		query << ",";
		query << "`V_UINT8`=" << (int)object.v_uint8;
		query << ",";
		query << "`V_INT16`=" << object.v_int16;
		query << ",";
		query << "`V_UINT16`=" << object.v_uint16;
		query << ",";
		query << "`V_INT32`=" << object.v_int32;
		query << ",";
		query << "`V_UINT32`=" << object.v_uint32;
		query << ",";
		query << "`V_INT64`=" << object.v_int64;
		query << ",";
		query << "`V_UINT64`=" << object.v_uint64;
		query << ",";
		query << "`V_FLOAT`=" << object.v_float;
		query << ",";
		query << "`V_DOUBLE`=" << object.v_double;
		query << ",";
		query << "`V_BOOL`=" << object.v_bool;
		query << ",";
		query << "`V_STRING`=" << mysqlpp::quote << object.v_string;
		query << ",";
		query << "`V_CHAR`=" << mysqlpp::quote << object.v_char;
		query << ",";
		query << "`V_BYTES`=" << mysqlpp::quote << object.v_bytes;
		query << ",";
		query << "`V_BYTES_TINY`=" << mysqlpp::quote << object.v_bytes_tiny;
		query << ",";
		query << "`V_BYTES_MEDIUM`=" << mysqlpp::quote << object.v_bytes_medium;
		query << ",";
		query << "`V_BYTES_LONG`=" << mysqlpp::quote << object.v_bytes_long;
	};
	
	void key2Query(mysqlpp::Query &query, const void* objptr){
		const ObjectType& object = *(ObjectType*)objptr;
		query << "`ID`=" << object.id;
	};
	
	bool record2Object(mysqlpp::Row& record, void* objptr){
		if (record.size() != fields.size()) return false;
		ObjectType& object = *(ObjectType*)objptr;
		object.id = record[0];
		strncpy(object.name, record[1].data(), sizeof(object.name)-1);
		object.score = record[2];
		object.bin.assign(record[3].data(), record[3].size());
		object.v_int8 = record[4];
		object.v_uint8 = record[5];
		object.v_int16 = record[6];
		object.v_uint16 = record[7];
		object.v_int32 = record[8];
		object.v_uint32 = record[9];
		object.v_int64 = record[10];
		object.v_uint64 = record[11];
		object.v_float = record[12];
		object.v_double = record[13];
		object.v_bool = record[14];
		object.v_string.assign(record[15].data(), record[15].size());
		strncpy(object.v_char, record[16].data(), sizeof(object.v_char)-1);
		object.v_bytes.assign(record[17].data(), record[17].size());
		object.v_bytes_tiny.assign(record[18].data(), record[18].size());
		object.v_bytes_medium.assign(record[19].data(), record[19].size());
		object.v_bytes_long.assign(record[20].data(), record[20].size());
	};
	
};
