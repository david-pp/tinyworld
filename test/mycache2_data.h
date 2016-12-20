#ifndef _MYCACH_USERT
#define _MYCACH_USERT
struct UserT {
	UserT() {
		charid = int();
		name = std::string();
		sex = int();
		age = int();
	}
	bool parseFromKVMap(KVMap& kvs) {
		charid = boost::lexical_cast<int>(kvs["charid"]);
		name = boost::lexical_cast<std::string>(kvs["name"]);
		sex = boost::lexical_cast<int>(kvs["sex"]);
		age = boost::lexical_cast<int>(kvs["age"]);
		return true;
	}
	bool saveToKVMap(KVMap& kvs) const {
		kvs["charid"] = boost::lexical_cast<std::string>(charid);
		kvs["name"] = boost::lexical_cast<std::string>(name);
		kvs["sex"] = boost::lexical_cast<std::string>(sex);
		kvs["age"] = boost::lexical_cast<std::string>(age);
		return true;
	}

	int charid;
	std::string name;
	int sex;
	int age;
};
#endif // _MYCACH_USERT

