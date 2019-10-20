#!/bin/bash

MAKE_PATH=$(dirname $(realpath $0))
SRC_FILES=$(find ${MAKE_PATH}/src -type f -iname *.cpp -print)

g++ -std=c++17 -g \
    $(pkg-config --cflags --libs gtk+-3.0) -rdynamic -export-dynamic -pthread -Wall -lpthread -lgmodule-2.0 -lsteam_api -lcurl -lyajl -ldl \
    ${SRC_FILES} \
    -L${MAKE_PATH}/bin \
    -o ${MAKE_PATH}/bin/SamRewritten \

echo "If there weren't any compilation errors, you can launch SamRewritten with ./bin/launch.sh"
