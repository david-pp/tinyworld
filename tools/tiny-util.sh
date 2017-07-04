#!/usr/bin/env bash

createdir()
{
    OUTPUTDIR=$1

    mkdir -p $OUTPUTDIR
    mkdir -p $OUTPUTDIR/include
    mkdir -p $OUTPUTDIR/src
    mkdir -p $OUTPUTDIR/example
    mkdir -p $OUTPUTDIR/test
}

copy() {
    cp $1 $2
    echo "cp $1 $2"
}

copy_licence()
{
    OUTPUTDIR=$1

    copy LICENSE $OUTPUTDIR/
}

copy_serializer()
{
    OUTPUTDIR=$1

    copy common/tinyworld.h                    $OUTPUTDIR/include
    copy common/tinyreflection.h               $OUTPUTDIR/include
    copy common/tinyserializer.h               $OUTPUTDIR/include
    copy common/tinyserializer_proto.h         $OUTPUTDIR/include
    copy common/tinyserializer_proto_mapping.h $OUTPUTDIR/include
    copy common/tinyserializer_proto_dyn.h     $OUTPUTDIR/include
    copy common/archive.proto                  $OUTPUTDIR/include

    copy example/player.proto                  $OUTPUTDIR/example
    copy example/demo_reflection.cpp           $OUTPUTDIR/example
    copy example/demo_serialize.cpp            $OUTPUTDIR/example
    copy example/demo_serialize_dyn.cpp        $OUTPUTDIR/example

    copy test/test_serialize.cpp               $OUTPUTDIR/test
    copy test/catch.hpp                        $OUTPUTDIR/test

    copy docs/tiny-serializer.md               $OUTPUTDIR/README.md
    copy docs/tiny-serializer-design.md        $OUTPUTDIR/
}

copy_orm()
{
    OUTPUTDIR=$1

    copy common/tinylogger.h              $OUTPUTDIR/include
    copy common/sharding.h                $OUTPUTDIR/include
    copy common/hashkit.h                 $OUTPUTDIR/include
    copy common/url.h                     $OUTPUTDIR/include
    copy common/pool.h                    $OUTPUTDIR/include
    copy common/pool_sharding.h           $OUTPUTDIR/include
    copy common/tinymysql.h               $OUTPUTDIR/include
    copy common/tinydb.h                  $OUTPUTDIR/include
    copy common/tinyorm.h                 $OUTPUTDIR/include
    copy common/tinyorm_mysql.h           $OUTPUTDIR/include
    copy common/tinyorm_mysql.in.h        $OUTPUTDIR/include
    copy common/tinyorm_soci.h            $OUTPUTDIR/include
    copy common/tinyorm_soci.in.h         $OUTPUTDIR/include

    copy common/url.cpp                   $OUTPUTDIR/src
    copy common/tinymysql.cpp             $OUTPUTDIR/src
    copy common/tinyorm.cpp               $OUTPUTDIR/src
    copy common/hashkit.cpp               $OUTPUTDIR/src


    copy example/player.h                 $OUTPUTDIR/example
    copy example/demo_orm.cpp             $OUTPUTDIR/example
}