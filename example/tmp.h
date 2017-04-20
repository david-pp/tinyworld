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