#! /bin/sh

MAJOR=`grep MAJOR version.h | { read A B C; echo $C; }`
MINOR=`grep MINOR version.h | { read A B C; echo $C; }`

for i in rkparameters rkparametersblock rkmisc rkpad ; do
    sed -i "s/^\(MAJOR\)=.*/\1=$MAJOR/; s/.*\(MINOR\)=.*/\1=$MINOR/" $i
done
