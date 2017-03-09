#include "tinyreflection.h"

struct Player {
    int id = 0;
    std::string name;
    int score = 0;
};

struct Register {
    Register() {
        StructFactory::instance().declare<Player>("Player").version(2016)
                .property("id", &Player::id, 1)
                .property("name", &Player::name, 2)
                .property("score", &Player::score, 3);
    }
};

Register reg__;

void test_serializer() {
    Player p;

    std::string bin = tiny::serialize(Player());

    tiny::deserialize(p, bin);
}


void test_r() {

    Player p;

    auto reflection = StructFactory::instance().structByType<Player>();
    reflection->set<int>(p, "id", 100);
    reflection->set<std::string>(p, "name", "david");

    std::cout << p.id << std::endl;
    std::cout << p.name << std::endl;

    std::cout << reflection->get<int>(p, "id") << std::endl;
    std::cout << reflection->get<std::string>(p, "name") << std::endl;

    for (auto prop : reflection->propertyIterator()) {
        if (prop->type() == typeid(int))
            std::cout << prop->name() << ":" << reflection->get<int>(p, prop->name()) << std::endl;
        else if (prop->type() == typeid(std::string))
            std::cout << prop->name() << ":" << reflection->get<std::string>(p, prop->name())  << std::endl;
    }
}


int main() {
    test_r();
    test_serializer();
}