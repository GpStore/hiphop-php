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

# make deb
mkdir -p hphp/DEBIAN
SIZE=`du -sx --exclude DEBIAN hphp`
SIZE="${SIZE%%\t*}"
`sed "s/HPHP_VERSION/${VERSION}/" control | sed "s/HPHP_ARCH/${ARCH}/" | sed "s/HPHP_SIZE/${SIZE}/" > hphp/DEBIAN/control`
fakeroot dpkg -b hphp
mv hphp.deb hphp_${VERSION}_${ARCH}.deb
