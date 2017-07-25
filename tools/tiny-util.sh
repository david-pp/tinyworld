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
#    cat $1 | iconv -f utf8 -t gb2312 > $2
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

    copy common/tinyworld.h                    $OUTPUTDIR/include/tinyworld.h
    copy common/tinyreflection.h               $OUTPUTDIR/include/tinyreflection.h
    copy common/tinyserializer.h               $OUTPUTDIR/include/tinyserializer.h
    copy common/tinyserializer_proto.h         $OUTPUTDIR/include/tinyserializer_proto.h
    copy common/tinyserializer_proto_mapping.h $OUTPUTDIR/include/tinyserializer_proto_mapping.h
    copy common/tinyserializer_proto_dyn.h     $OUTPUTDIR/include/tinyserializer_proto_dyn.h
    copy common/archive.proto                  $OUTPUTDIR/include/archive.proto

    copy example/player.proto                  $OUTPUTDIR/example/player.proto
    copy example/demo_reflection.cpp           $OUTPUTDIR/example/demo_reflection.cpp
    copy example/demo_serialize.cpp            $OUTPUTDIR/example/demo_serialize.cpp
    copy example/demo_serialize_dyn.cpp        $OUTPUTDIR/example/demo_serialize_dyn.cpp

    copy test/test_serialize.cpp               $OUTPUTDIR/test/test_serialize.cpp
    copy test/catch.hpp                        $OUTPUTDIR/test/catch.hpp

    copy docs/tiny-serializer.md               $OUTPUTDIR/README.md
    copy docs/tiny-serializer-design.md        $OUTPUTDIR/tiny-serializer-design.md
}

copy_orm()
{
    OUTPUTDIR=$1

    copy common/tinylogger.h              $OUTPUTDIR/include/tinylogger.h
    copy common/sharding.h                $OUTPUTDIR/include/sharding.h
    copy common/hashkit.h                 $OUTPUTDIR/include/hashkit.h
    copy common/url.h                     $OUTPUTDIR/include/url.h
    copy common/pool.h                    $OUTPUTDIR/include/pool.h
    copy common/pool_sharding.h           $OUTPUTDIR/include/pool_sharding.h
    copy common/tinymysql.h               $OUTPUTDIR/include/tinymysql.h
    copy common/tinydb.h                  $OUTPUTDIR/include/tinydb.h
    copy common/tinyorm.h                 $OUTPUTDIR/include/tinyorm.h
    copy common/tinyorm_mysql.h           $OUTPUTDIR/include/tinyorm_mysql.h
    copy common/tinyorm_mysql.in.h        $OUTPUTDIR/include/tinyorm_mysql.in.h
    copy common/tinyorm_soci.h            $OUTPUTDIR/include/tinyorm_soci.h
    copy common/tinyorm_soci.in.h         $OUTPUTDIR/include/tinyorm_soci.in.h

    copy common/url.cpp                   $OUTPUTDIR/src/url.cpp
    copy common/tinymysql.cpp             $OUTPUTDIR/src/tinymysql.cpp
    copy common/tinyorm.cpp               $OUTPUTDIR/src/tinyorm.cpp
    copy common/hashkit.cpp               $OUTPUTDIR/src/hashkit.cpp


    copy example/player.h                 $OUTPUTDIR/example/player.h
    copy example/demo_orm.cpp             $OUTPUTDIR/example/demo_orm.cpp

    mkdir -p $OUTPUTDIR/example/tinyobj
    copy example/tinyobj/tinyobj.h       $OUTPUTDIR/example/tinyobj/tinyobj.h
    copy example/tinyobj/tinyobj.cpp     $OUTPUTDIR/example/tinyobj/tinyobj.cpp
    copy example/tinyobj/tinyplayer.xml  $OUTPUTDIR/example/tinyobj/tinyplayer.xml
    copy example/tinyobj/CMakeLists.txt  $OUTPUTDIR/example/tinyobj/CMakeLists.txt

    mkdir -p $OUTPUTDIR/tools
    copy tools/tinyobj.py                $OUTPUTDIR/tools/tinyobj.py
}