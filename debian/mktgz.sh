#! /bin/sh

# extract files
./extract.sh

# determine version and architecture
VERSION=`sed 's/hphp *(\(.*\)).*/\1/g' changelog.Debian | head -1`
ARCH=`uname -m`

case $ARCH in
"i686")
	ARCH=i386
	;;
"x86_64")
	ARCH=amd64
	;;
*)
	;;
esac

echo "Version=${VERSION}, Arch=${ARCH}"

# make tgz
cd hphp; fakeroot tar -cf ../hphp_${VERSION}_${ARCH}.tar *; cd ..
gzip -9 hphp_${VERSION}_${ARCH}.tar
