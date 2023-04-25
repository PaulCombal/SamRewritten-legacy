#!/bin/bash

if [ ! -f ../bin/samrewritten ]; then
    echo "Building SamRewritten first..."
    pushd $(dirname $(realpath $0))/..
    make
    popd
fi

export LINUXDEPLOY="squashfs-root/usr/bin/linuxdeploy"

rm -rf AppDir
chmod +x linuxdeploy-x86_64.AppImage
./linuxdeploy-x86_64.AppImage --appimage-extract
$LINUXDEPLOY --appdir AppDir 
grep -v Icon samrewritten.desktop > AppDir/myapp.desktop
echo Icon=myapp >> AppDir/myapp.desktop
cp ../assets/icon_256.png AppDir/myapp.png

echo '#!/bin/bash

SCRIPT_PATH=`dirname $(realpath $0)`/usr/bin
export LD_LIBRARY_PATH=${SCRIPT_PATH}/usr/lib
export LC_NUMERIC=en_US.UTF-8

cd ${SCRIPT_PATH}/..
${SCRIPT_PATH}/samrewritten $@' > AppDir/AppRun
chmod +x AppDir/AppRun

mkdir AppDir/usr/assets
cp -r ../glade AppDir/usr
cp ../bin/samrewritten AppDir/usr/bin
cp ../bin/libsteam_api.so AppDir/usr/lib
cp ../assets/icon_256.png AppDir/usr/assets/

LD_LIBRARY_PATH=AppDir/usr/lib $LINUXDEPLOY --appdir AppDir
rm AppDir/usr/lib/libglib-2.0.so.0

$LINUXDEPLOY-plugin-appimage --appdir AppDir
rm -rf squashfs-root
