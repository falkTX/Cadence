#!/bin/bash

MINGW=i686-w64-mingw32
MINGW_PATH=/opt/mingw32

JOBS="-j 4"

if [ ! -f Makefile ]; then
  cd ../..
fi

export PATH=`pwd`/data/windows:$MINGW_PATH/bin:$MINGW_PATH/$MINGW/bin:$PATH
export AR=$MINGW-ar
export CC=$MINGW-gcc
export CXX=$MINGW-g++
export MOC=$MINGW-moc
export RCC=$MINGW-rcc
export UIC=$MINGW-uic
export STRIP=$MINGW-strip
export WINDRES=$MINGW-windres

export PKG_CONFIG_PATH="`pwd`/data/windows:$MINGW_PATH/lib/pkgconfig"
echo $PKG_CONFIG_PATH

export WINEPREFIX=~/.winepy3

export PYTHON_EXE="C:\\\\Python33\\\\python.exe"

export CXFREEZE="wine $PYTHON_EXE C:\\\\Python33\\\\Scripts\\\\cxfreeze"
export PYUIC="wine $PYTHON_EXE C:\\\\Python33\\\\Lib\\\\site-packages\\\\PyQt4\\\\uic\\\\pyuic.py"
export PYRCC="wine C:\\\\Python33\\\\Lib\\\\site-packages\\\\PyQt4\\\\pyrcc4.exe -py3"

export CFLAGS="-DPTW32_STATIC_LIB -I$MINGW_PATH/include"
export CXXFLAGS="-DPTW32_STATIC_LIB -DWIN32 -I$MINGW_PATH/include"

# win32 jack libs
cp -r "$WINEPREFIX/drive_c/Program Files (x86)/Jack/includes/" ./data/windows/
cp -r "$WINEPREFIX/drive_c/Program Files (x86)/Jack/lib/"      ./data/windows/
cp "$WINEPREFIX/drive_c/windows/syswow64/libjack.dll"          ./data/windows/lib/

# Clean build
make clean

# Build PyQt4 resources
make $JOBS UI RES

# Build C++ tools
make $JOBS -C c++/jackmeter    cadence-jackmeter.exe
make $JOBS -C c++/xycontroller cadence-xycontroller.exe
mv c++/*/*.exe data/windows

rm -rf ./data/windows/Catarina
rm -rf ./data/windows/Catia
$CXFREEZE --include-modules=re --target-dir=".\\data\\windows\\Catarina" ".\\src\\catarina.py"
$CXFREEZE --include-modules=re --target-dir=".\\data\\windows\\Catia" ".\\src\\catia.py"

cd data/windows

cp $WINEPREFIX/drive_c/windows/syswow64/python33.dll Catarina/
cp $WINEPREFIX/drive_c/Python33/Lib/site-packages/PyQt4/QtCore4.dll   Catarina/
cp $WINEPREFIX/drive_c/Python33/Lib/site-packages/PyQt4/QtGui4.dll    Catarina/
cp $WINEPREFIX/drive_c/Python33/Lib/site-packages/PyQt4/QtOpenGL4.dll Catarina/
cp $WINEPREFIX/drive_c/Python33/Lib/site-packages/PyQt4/QtSvg4.dll    Catarina/
cp $WINEPREFIX/drive_c/Python33/Lib/site-packages/PyQt4/QtXml4.dll    Catarina/

cp $WINEPREFIX/drive_c/windows/syswow64/python33.dll Catia/
cp $WINEPREFIX/drive_c/Python33/Lib/site-packages/PyQt4/QtCore4.dll   Catia/
cp $WINEPREFIX/drive_c/Python33/Lib/site-packages/PyQt4/QtGui4.dll    Catia/
cp $WINEPREFIX/drive_c/Python33/Lib/site-packages/PyQt4/QtOpenGL4.dll Catia/
cp $WINEPREFIX/drive_c/Python33/Lib/site-packages/PyQt4/QtSvg4.dll    Catia/

rm -rf ./includes/
rm -rf ./lib/

# Testing:
echo "export WINEPREFIX=~/.winepy3"
echo "wine $PYTHON_EXE ../../src/catia.py"
