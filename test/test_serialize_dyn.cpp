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
            .property("type", &Weapon::type, 1)
            .property("name", &Weapon::name, 2);

    StructFactory::instance().declare<Player>("Player")
            .property("id", &Player::id, 1)
            .property("name", &Player::name, 2)
            .property("weapon", &Player::weapon, 3)
            .property("weapons_map", &Player::weapons_map, 4);
}

int main() {

}
