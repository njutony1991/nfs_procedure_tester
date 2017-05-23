#!/bin/sh
#
#   run tests
#


if [ $# -lt 2 ]
then
    echo "$0 ip volume"
    exit 1
fi

echo "ip: " $1 "volume: " $2 
echo ""

echo "access : Start"
./access $1 $2
echo "access : End"
echo ""

echo "link : Start"
./link $1 $2
echo "link : End"
echo ""

echo "create: Start"
./create $1 $2
echo "create: End"
echo ""

echo "lookup: Start"
./lookup $1 $2
echo "lookup: End"
echo ""

echo "mkdir: Start"
./mkdir $1 $2
echo "mkdir: End"
echo "" 

echo "mknod: Start"
./mknod $1 $2
echo "mknod: End"
echo ""

echo "remove: Start"
./remove $1 $2
echo "remove: End"
echo ""

echo "rmdir: Start"
./rmdir $1 $2
echo "rmdir: End"
echo ""

echo "rename: Start"
./rename $1 $2
echo "rename: End"
echo ""

echo "symlink: Start"
./symlink $1 $2
echo "symlink: End"
echo ""

