#include "tinyreflection.h"
#include "tinyserializer.h"
#include "tinyserializer_proto.h"

#include "player.pb.h"
#include "test_msg.pb.h"

//
// 类型定义
//
struct Weapon {
    uint32_t type = 0;
    std::string name = "";

    //
    // 侵入式序列化支持
    //
    std::string serialize() const {
        WeaponProto proto;
        proto.set_type(type);
        proto.set_name(name);
        return proto.SerializeAsString();
    }

    bool deserialize(const std::string &data) {
        WeaponProto proto;
        if (proto.ParseFromString(data)) {
            type = proto.type();
            name = proto.name();
            return true;
        }
        return false;
    }
};

struct Player {
    uint32_t id = 0;
    std::string name = "";
    std::vector<uint32_t> quests;
    Weapon weapon;
    std::map<uint32_t, Weapon> weapons;

    std::map<uint32_t, std::vector<Weapon>> weapons_map;

    void dump() const {
        std::cout << "id   = " << id << std::endl;
        std::cout << "name = " << name << std::endl;

        std::cout << "quests = [";
        for (auto v : quests)
            std::cout << v << ",";
        std::cout << "]" << std::endl;

        std::cout << "weapon = {" << weapon.type << "," << weapon.name << "}" << std::endl;

        std::cout << "weapons_map = {" << std::endl;
        for (auto &kv : weapons_map) {
            std::cout << "\t" << kv.first << ": [";
            for (auto &p : kv.second) {
                std::cout << "{" << p.type << "," << p.name << "},";
            }
            std::cout << "]" << std::endl;
        }
        std::cout << "}" << std::endl;
    }

    void init() {
        id = 1024;
        name = "david";
        quests.push_back(1);
        quests.push_back(2);

        weapon.type = 0;
        weapon.name = "Sword";

        for (uint32_t i = 0; i < 3; i++) {
            weapons[i].type = i;
            weapons[i].name = "Shield";
        }

        Weapon w;
        w.type = 22;
        w.name = "Blade";
        weapons_map = {
                {1, {w, w, w}},
                {2, {w, w, w}},
        };

    }
};


template<>
struct ProtoSerializer<Player> {
    std::string serialize(const Player &p) const {
        PlayerProto proto;
        proto.set_id(p.id);
        proto.set_name(p.name);

        // 复杂对象
        proto.set_weapons_map(::serialize(p.weapons_map));

        return proto.SerializeAsString();
    }

    bool deserialize(Player &p, const std::string &data) const {
        PlayerProto proto;
        if (proto.ParseFromString(data)) {
            p.id = proto.id();
            p.name = proto.name();

            // 复杂对象
            ::deserialize(p.weapons_map, proto.weapons_map());
            return true;
        }
        return false;
    }
};


void test_basic() {

    std::cout << "--------" << __PRETTY_FUNCTION__ << std::endl;

    {
        uint32_t v1 = 1024;

        std::string bin = serialize(v1);

        uint32_t v2 = 0;
        deserialize(v2, bin);

        std::cout << v2 << std::endl;
    }

    {
        Cmd::LoginRequest msg1;
        msg1.set_id(2);
        msg1.set_name("david");
        msg1.set_password("12356676");
        msg1.set_type(1024);

        std::string bin = serialize(msg1);

        Cmd::LoginRequest msg2;
        deserialize(msg2, bin);
        std::cout << msg2.ShortDebugString() << std::endl;
    }

    {
        Cmd::LoginRequest msg1;
        msg1.set_id(2);
        msg1.set_name("david");
        msg1.set_password("12356676");
        msg1.set_type(1024);

        std::vector<Cmd::LoginRequest> messages = {
                msg1, msg1, msg1
        };

        std::string data = serialize(messages);

        std::vector<Cmd::LoginRequest> m2;
        deserialize(m2, data);

        for (auto &v : m2) {
            std::cout << v.ShortDebugString() << std::endl;
        }
    }
}

void test_userdefined() {
    std::cout << "--------" << __PRETTY_FUNCTION__ << std::endl;

    {
        Weapon w;
        w.type = 22;
        w.name = "Blade";

        std::string data = serialize(w);

        Weapon w2;

        deserialize(w2, data);

        std::cout << w2.type << " - " << w2.name << std::endl;
    }

    {
        Player p;
        p.init();

        std::string data = serialize(p);

        Player p2;

        deserialize(p2, data);

        p2.dump();
    }
}

void test_archiver() {
    std::cout << "--------" << __PRETTY_FUNCTION__ << std::endl;


    std::string data;

    // 序列化
    {
        PlayerProto p;
        p.set_id(1024);
        p.set_name("david");

        std::vector<PlayerProto> pvec{p, p, p};

        std::map<uint32_t, PlayerProto> pmap{
                {1, p},
                {2, p},
                {3, p}
        };

        std::vector<std::map<uint32_t, PlayerProto> > vecmap{pmap, pmap, pmap};

        ProtoArchiver ar;
        ar << p << p << p;        // 支持：proto生成的类型
        ar << pvec;               // 支持：std::vector
        ar << pmap;               // 支持：std::map
        ar << vecmap;             // 支持：STL的各种组合和嵌套
        ar << ar;                 // 支持：自己? YES!

//        ar.SerializeToString(&data);
        data = ::serialize(ar);
    }

    // 反序列化
    {
        PlayerProto p1, p2, p3;
        std::vector<PlayerProto> pvec;
        std::map<uint32_t, PlayerProto> pmap;
        std::vector<std::map<uint32_t, PlayerProto> > vecmap;

        ProtoArchiver ar, ar2;
//        if (ar.ParseFromString(data)) {
        if (::deserialize(ar, data)) {
            ar >> p1 >> p2 >> p3;
            ar >> pvec;
            ar >> pmap;
            ar >> vecmap;
            ar >> ar2;
        }

        std::cout << "--- p1 ----" << std::endl;
        {
            std::cout << p1.ShortDebugString() << std::endl;
        }
        std::cout << "--- p2 ----" << std::endl;
        {
            std::cout << p2.ShortDebugString() << std::endl;
        }
        std::cout << "--- p3 ----" << std::endl;
        {
            std::cout << p3.ShortDebugString() << std::endl;
        }

        std::cout << "--- vector<P> ----" << std::endl;
        {
            for (auto &p : pvec) {
                std::cout << p.ShortDebugString() << std::endl;
            }
        }

        std::cout << "--- map<P> ----" << std::endl;
        {
            for (auto &v : pmap) {
                std::cout << v.first << " : " << v.second.ShortDebugString() << std::endl;
            }
        }

        std::cout << "--- vector<map<P>> ----" << std::endl;
        {
            for (auto &item : vecmap) {
                for (auto &v: item)
                    std::cout << v.first << " : " << v.second.ShortDebugString() << std::endl;
                std::cout << std::endl;
            }
        }

        std::cout << "---- ar ----" << std::endl;
        {
            std::cout << ar2.DebugString() << std::endl;
        }
    }
}

int main() {
    std::cout << std::is_base_of<google::protobuf::Message, ArchiveProto>::value << std::endl;

    std::cout << ProtoCase<int8_t>::value << std::endl;
    std::cout << ProtoCase<ArchiveProto>::value << std::endl;
    std::cout << ProtoCase<std::vector<int>>::value << std::endl;
    std::cout << ProtoCase<Player>::value << std::endl;

    test_basic();
    test_userdefined();
    test_archiver();
}