#include "tinyreflection.h"
#include "tinyserializer.h"


struct Player {
    int id = 0;
    std::string name;
    int score = 0;
    float salary = 20000.25;

    std::vector<int> vec = {21, 22 , 23, 24, 25};

    std::vector<std::string> vec2 = {"david", "LL", "JJ"};

    std::vector<std::vector<std::string> > vec3 = {
            {"david1", "LL", "JJ"},
            {"david2", "LL", "JJ"},
            {"david3", "LL", "JJ"}
    };

    void dump() {
        std::cout << std::dec << id << "," << name << "," << score << "," << salary << std::endl;

        std::cout << "vec = [";
        std::for_each(vec.begin(), vec.end(), [](int v) {
            std::cout << v << ",";
        });
        std::cout << "]" << std::endl;

        std::cout << "vec2 = [";
        std::for_each(vec2.begin(), vec2.end(), [](std::string& v) {
            std::cout << v << ",";
        });
        std::cout << "]" << std::endl;

        std::cout << "vec3 = [";
        std::for_each(vec3.begin(), vec3.end(), [](std::vector<std::string>& v) {
            std::cout << "[";
            std::for_each(v.begin(), v.end(), [](std::string& v1) {
                std::cout << v1 << ",";
            });
            std::cout << "]," ;
        });
        std::cout << "]" << std::endl;
    }
};

struct Register {
    Register() {
        StructFactory::instance().declare<Player>("Player").version(2016)
                .property("id", &Player::id, 1)
                .property("name", &Player::name, 2)
                .property("score", &Player::score, 3)
                .property("salary", &Player::salary, 4)
                .property("vec", &Player::vec, 5)
                .property("vec2", &Player::vec2, 6)
                .property("vec3", &Player::vec3, 7);
    }
};

Register reg__;

//
// TinyArchiver  ar;
// TextArchiver  ar;
// JsonArchiver  ar;
// XMLArchiver   ar;
// ProtoArchiver ar;
//
// Player p;
//
// ar << p << p << p;
// ar >> p >> p >> p;
//
//
// tiny::serialize<TinySerializer>(p)
// tiny::deserialize<TinySerializer>(p, bin);
//
// TinyArchiver ar;
//

void test_serializer() {
    Player p;
    p.id = 1024;
    p.name = "david wang";
    p.score = 100;

    std::string bin = tiny::serialize(p);

    p.dump();

    p.id = 0;
    p.salary = 0;
    p.vec.clear();
    p.vec2.clear();
    p.vec3.clear();
    tiny::deserialize(p, bin);

    hexdump(bin);

    p.dump();
}


int main() {
//    test_r();
    test_serializer();
}