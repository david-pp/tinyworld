#!/usr/bin/env bash

SCRIPTNAME=$0
OUTPUTDIR=tiny-serializer

usage() {
    echo "Usage: $SCRIPTNAME DIR"
}

if [[ $# -lt 1 ]]; then
    usage
    exit 0
fi

OUTPUTDIR=$1

mkdir -p $OUTPUTDIR
mkdir -p $OUTPUTDIR/include
mkdir -p $OUTPUTDIR/example
mkdir -p $OUTPUTDIR/test


copy() {
    cp $1 $2
    echo "cp $1 $2"
}

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

copy LICENSE $OUTPUTDIR/