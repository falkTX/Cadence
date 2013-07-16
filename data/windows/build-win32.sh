#!/bin/bash

set -e

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

export WINEPREFIX=~/.winepy3

export PYTHON_EXE="C:\\\\Python33\\\\python.exe"

export CXFREEZE="wine $PYTHON_EXE C:\\\\Python33\\\\Scripts\\\\cxfreeze"
export PYUIC="wine $PYTHON_EXE C:\\\\Python33\\\\Lib\\\\site-packages\\\\PyQt4\\\\uic\\\\pyuic.py"
export PYRCC="wine C:\\\\Python33\\\\Lib\\\\site-packages\\\\PyQt4\\\\pyrcc4.exe -py3"

export CFLAGS="-DPTW32_STATIC_LIB -I$MINGW_PATH/include"
export CXXFLAGS="-DPTW32_STATIC_LIB -DWIN32 -I$MINGW_PATH/include"

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
cp ./src/catarina.py ./src/catarina.pyw
cp ./src/catia.py ./src/catia.pyw
$CXFREEZE --include-modules=re --target-dir=".\\data\\windows\\Catarina" ".\\src\\catarina.pyw"
$CXFREEZE --include-modules=re --target-dir=".\\data\\windows\\Catia" ".\\src\\catia.pyw"
rm -f ./src/catarina.pyw
rm -f ./src/catia.pyw

cd data/windows
mv cadence-jackmeter.exe    Cadence-JackMeter.exe
mv cadence-xycontroller.exe Cadence-XYController.exe

cp $WINEPREFIX/drive_c/windows/syswow64/python33.dll Catarina/
cp $WINEPREFIX/drive_c/Python33/Lib/site-packages/PyQt4/QtCore4.dll   Catarina/
cp $WINEPREFIX/drive_c/Python33/Lib/site-packages/PyQt4/QtGui4.dll    Catarina/
cp $WINEPREFIX/drive_c/Python33/Lib/site-packages/PyQt4/QtOpenGL4.dll Catarina/
cp $WINEPREFIX/drive_c/Python33/Lib/site-packages/PyQt4/QtSvg4.dll    Catarina/
cp $WINEPREFIX/drive_c/Python33/Lib/site-packages/PyQt4/QtXml4.dll    Catarina/
cp -r $WINEPREFIX/drive_c/Python33/Lib/site-packages/PyQt4/plugins/imageformats/ Catarina/
cp -r $WINEPREFIX/drive_c/Python33/Lib/site-packages/PyQt4/plugins/iconengines/  Catarina/

cp $WINEPREFIX/drive_c/windows/syswow64/python33.dll Catia/
cp $WINEPREFIX/drive_c/Python33/Lib/site-packages/PyQt4/QtCore4.dll   Catia/
cp $WINEPREFIX/drive_c/Python33/Lib/site-packages/PyQt4/QtGui4.dll    Catia/
cp $WINEPREFIX/drive_c/Python33/Lib/site-packages/PyQt4/QtOpenGL4.dll Catia/
cp $WINEPREFIX/drive_c/Python33/Lib/site-packages/PyQt4/QtSvg4.dll    Catia/
cp -r $WINEPREFIX/drive_c/Python33/Lib/site-packages/PyQt4/plugins/imageformats/ Catia/
cp -r $WINEPREFIX/drive_c/Python33/Lib/site-packages/PyQt4/plugins/iconengines/  Catia/

rm -rf ./includes/
rm -rf ./lib/

# Build unzipfx
make -C unzipfx-catarina -f Makefile.win32
make -C unzipfx-catia -f Makefile.win32

# Create static build
rm -f Catarina.zip Catia.zip
zip -r -9 Catarina.zip Catarina
zip -r -9 Catia.zip Catia

rm -f Catarina.exe Catia.exe
cat unzipfx-catarina/unzipfx2cat.exe Catarina.zip > Catarina.exe
cat unzipfx-catia/unzipfx2cat.exe Catia.zip > Catia.exe
chmod +x Catarina.exe
chmod +x Catia.exe

# Cleanup
make -C unzipfx-catarina -f Makefile.win32 clean
make -C unzipfx-catia -f Makefile.win32 clean
rm -f Catarina.zip Catia.zip
rm -f unzipfx-*/*.exe

# Final Zip
rm -rf Cadence-0.8-beta2
mkdir -p Cadence-0.8-beta2
cp *.exe Cadence-0.8-beta2
cp README Cadence-0.8-beta2
zip -r -9 Cadence-0.8-beta2-win32.zip Cadence-0.8-beta2

# Testing:
echo "export WINEPREFIX=~/.winepy3"
echo "wine $PYTHON_EXE ../../src/catia.py"
