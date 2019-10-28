#!/bin/bash

if [ ! -f ../bin/samrewritten ]; then
    echo "Building SamRewritten first..."
    pushd $(dirname $(realpath $0))/..
    make
    popd
fi

rm -rf AppDir
./linuxdeploy-x86_64.AppImage --appdir AppDir
grep -v Icon samrewritten.desktop > AppDir/myapp.desktop
echo Icon=myapp >> AppDir/myapp.desktop
cp ../assets/icon_256.png AppDir/myapp.png

echo '#!/bin/bash

SCRIPT_PATH=`dirname $(realpath $0)`/usr/bin
export LD_LIBRARY_PATH=${SCRIPT_PATH}/usr/lib

cd ${SCRIPT_PATH}/..
${SCRIPT_PATH}/samrewritten $@' > AppDir/AppRun
chmod +x AppDir/AppRun

cp -r ../glade AppDir/usr
cp ../bin/samrewritten AppDir/usr/bin
cp ../bin/libsteam_api.so AppDir/usr/lib

LD_LIBRARY_PATH=AppDir/usr/lib ./linuxdeploy-x86_64.AppImage --appdir AppDir --output appimage