#!/bin/bash

if [ $# -ne 2 ]
then
    echo "usage: $0 <wrapper folder path> <github repo address>"
    exit 1
fi

git clone $2

dir=${2##*/}
wrapper_prefix=$(readlink -f $1)

cd $dir
ln -fs "$wrapper_prefix/wrapper"
ln -fs "$wrapper_prefix/wrapper.conf"
ln -fs "$wrapper_prefix/log.ini"

make CC=./wrapper CXX=./wrapper
