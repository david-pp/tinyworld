#include "tinyserializer.h"
#include "tinyserializer_proto.h"
#include "tinyserializer_proto_dyn.h"

//
// 类型定义
//
struct Weapon {
    uint32_t type = 0;
    std::string name = "";
};

struct Player {
    uint32_t id = 0;
    std::string name = "";
    Weapon weapon;
    std::map<uint32_t, std::vector<Weapon>> weapons_map;

    void init() {
        id = 1024;
        name = "david";

        weapon.type = 22;
        weapon.name = "Sword";

        Weapon w;
        w.type = 22;
        w.name = "Blade";
        weapons_map = {
                {1, {w, w, w}},
                {2, {w, w, w}},
        };
    }

    void dump() const {
        std::cout << "id   = " << id << std::endl;
        std::cout << "name = " << name << std::endl;

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
};

//
// 反射
//
RUN_ONCE(Player) {

    StructFactory::instance().declare<Weapon>("Weapon")
            .property<ProtoDynSerializer>("type", &Weapon::type, 1)
            .property<ProtoDynSerializer>("name", &Weapon::name, 2);

    StructFactory::instance().declare<Player>("Player")
            .property<ProtoDynSerializer>("id", &Player::id, 1)
            .property<ProtoDynSerializer>("name", &Player::name, 2)
            .property<ProtoDynSerializer>("weapon", &Player::weapon, 3)
            .property<ProtoDynSerializer>("weapons_map", &Player::weapons_map, 4);
}

template <template <typename> class SerializerT>
struct GenericStructFactory : public StructFactory {
    void reg() {
        declare<Weapon>("Weapon")
                .template property<SerializerT>("type", &Weapon::type, 1)
                .template property<SerializerT>("name", &Weapon::name, 2);

        declare<Player>("Player")
                .template property<SerializerT>("id", &Player::id, 1)
                .template property<SerializerT>("name", &Player::name, 2)
                .template property<SerializerT>("weapon", &Player::weapon, 3)
                .template property<SerializerT>("weapons_map", &Player::weapons_map, 4);
    }
};

GenericStructFactory<ProtoDynSerializer> proto_dyn;


#define USE_STRUCT_FACTORY

RUN_ONCE(PlayerSerial) {

#ifdef USE_STRUCT_FACTORY
//    ProtoMappingFactory::instance()
//                    .mapping<Weapon>("WeaponDyn2Proto")
//                    .mapping<Player>("PlayerDyn2Proto");

    proto_dyn.reg();

    ProtoMappingFactory::instance()
            .mapping<Weapon>("", proto_dyn)
            .mapping<Player>("", proto_dyn);

#else
    ProtoMappingFactory::instance().declare<Weapon>("Weapon", "WeaponDyn3Proto")
            .property<ProtoDynSerializer>("type", &Weapon::type, 1)
            .property<ProtoDynSerializer>("name", &Weapon::name, 2);

    ProtoMappingFactory::instance().declare<Player>("Player", "PlayerDyn3Proto")
            .property<ProtoDynSerializer>("id", &Player::id, 1)
            .property<ProtoDynSerializer>("name", &Player::name, 2)
            .property<ProtoDynSerializer>("weapon", &Player::weapon, 3)
            .property<ProtoDynSerializer>("weapons_map", &Player::weapons_map, 4);
#endif
}

void test_2() {

    std::cout << "----------" << __PRETTY_FUNCTION__ << "----\n\n";

    std::string data;

    {
        Player p;
        p.init();
        data = serialize<ProtoDynSerializer>(p);
        p.dump();
    }

    hexdump(data);

    {
        Player p;
        if (deserialize<ProtoDynSerializer>(p, data))
            p.dump();
    }
}

int main() {
    ProtoMappingFactory::instance().createAllProtoDescriptor();
    ProtoMappingFactory::instance().createAllProtoDefine();

    try {
        test_2();
    } catch (const std::exception &e) {
        std::cout << e.what() << std::endl;
    }
}
