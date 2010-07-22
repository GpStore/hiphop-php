#! /bin/sh

# prepare directory
rm -rf hphp 
PRE_ROOT=hphp/usr
SHARE_ROOT=$PRE_ROOT/share/hphp
BIN_ROOT=$PRE_ROOT/bin
DOC_ROOT=$PRE_ROOT/share/doc/hphp
mkdir -p $PRE_ROOT
mkdir -p $SHARE_ROOT
mkdir -p $BIN_ROOT
mkdir -p $DOC_ROOT

# copy bin
mkdir $SHARE_ROOT/bin
cp ../bin/*.a $SHARE_ROOT/bin/
cp ../bin/CMakeLists.base.txt $SHARE_ROOT/bin/
cp ../bin/run.sh $SHARE_ROOT/bin/
cp ../bin/mime.hdf $SHARE_ROOT/bin/

# copy CMake
cp -rf ../CMake $SHARE_ROOT/ 

# copy binaries
cp ../src/hphp/hphp $BIN_ROOT/ 
chmod a+x $BIN_ROOT/hphp 
cp ../src/hphpi/hphpi $BIN_ROOT/ 
chmod a+x $BIN_ROOT/hphpi

# copy src
mkdir -p $SHARE_ROOT/src/
cp -rf ../src/runtime $SHARE_ROOT/src/
cp -rf ../src/system $SHARE_ROOT/src/
cp -rf ../src/third_party $SHARE_ROOT/src/
cp -rf ../src/util $SHARE_ROOT/src/

find $SHARE_ROOT/src/runtime -not \( -iname "*.h" -o -iname "*.hpp" -o -iname "*.inc" -o -iname "*.hh" \) -type f -exec rm {} \;
find $SHARE_ROOT/src/third_party -not \( -iname "*.h" -o -iname "*.hpp" -o -iname "*.inc" -o -iname "*.hh" \) -type f -exec rm {} \;
find $SHARE_ROOT/src/util -not \( -iname "*.h" -o -iname "*.hpp" -o -iname "*.inc" -o -iname "*.hh" \) -type f -exec rm {} \;
find $SHARE_ROOT/src -depth -type d -empty -exec rmdir {} \;
rm $SHARE_ROOT/src/system/*.inc

# doc
cp copyright $DOC_ROOT
cp changelog.Debian $DOC_ROOT/changelog.Debian
gzip -9 $DOC_ROOT/changelog.Debian
./gchlog.sh
cp ChangeLog $DOC_ROOT/changelog
gzip -9 $DOC_ROOT/changelog
cp ../README.md $DOC_ROOT/README
gzip -9 $DOC_ROOT/README

# fixing mode
find $SHARE_ROOT -type f -exec chmod a-x {} \;
chmod a+x $SHARE_ROOT/bin/run.sh

