#!/usr/bin/env bash
#!/usr/bin/env bash

source $PWD/tools/tiny-util.sh

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

createdir $OUTPUTDIR
copy_serializer $OUTPUTDIR
copy_orm $OUTPUTDIR
copy_licence $OUTPUTDIR
