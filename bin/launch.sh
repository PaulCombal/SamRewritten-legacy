#!/bin/bash

SCRIPT_PATH=`dirname $(realpath $0)`
export LD_LIBRARY_PATH=${SCRIPT_PATH}

${SCRIPT_PATH}/SamRewritten $@
