#! /bin/sh

MAJOR=`grep '^#def.*MAJOR' rkflashtool.c | rev | cut -b 1-2 | rev | tr -d ' '`
MINOR=`grep '^#def.*MINOR' rkflashtool.c | rev | cut -b 1-2 | rev | tr -d ' '`

NAME=rkflashtool-$MAJOR.$MINOR
DIR=$NAME-src

rm -rf $DIR
mkdir $DIR

cp -a \
    Makefile \
    rkflashtool.c \
    README \
    $DIR

tar cvzf $DIR.tar.gz $DIR
tar cvjf $DIR.tar.bz2 $DIR
tar cvJf $DIR.tar.xz $DIR
zip -9r $DIR.zip $DIR

rm -rf $DIR

echo trying win32/win64 cross-builds...

rm -f rkflashtool.exe
make MACH=mingw CROSSPREFIX=i686-w64-mingw32- || exit 1

zip -9 $NAME-win32-bin.zip rkflashtool.exe

rm -f rkflashtool.exe
make MACH=mingw CROSSPREFIX=x86_64-w64-mingw32- || exit 1

zip -9 $NAME-win64-bin.zip rkflashtool.exe

rm -f rkflashtool.exe
