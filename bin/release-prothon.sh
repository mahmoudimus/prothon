#!/bin/sh

if [ "$#" != "2" ]; then
	echo "Usage: $0 <rev> <ver>"
	exit 1
fi

REV=$1
VER=$2

svn copy -r $REV -m "Tag $VER" svn://svn.prothon.org/prothon/trunk \
	svn://svn.prothon.org/prothon/tags/v$VER

svn export svn://svn.prothon.org/prothon/tags/v$VER \
	prothon-$VER-b$REV

tar zcf prothon-$VER-b$REV.tar.gz prothon-$VER-b$REV

rm -rf prothon-$VER-b$REV

