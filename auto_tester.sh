#!/bin/bash

set -e

function usage() {
    echo "usage: $0 -w <wrapper folder path> [--git, --svn] <url>"
}

function error() {
    echo -e "Error:" $1 >&2
    usage >&2
    exit 1
}

ARGS=$(getopt -o "hw:" -l "git:,svn:,help" -n "$0" -- "$@")

if [[ $? != 0 ]]; then
    exit 1
fi

eval set -- "$ARGS"

while true; do
    case "$1" in
        -h|--help)
            usage
            exit;;
        -w)
            wrapper_prefix=$(readlink -f $2)
            shift 2;;
        --svn)
            dir=${2##*/}
            svn checkout $2
            shift 2;;
        --git)
            dir=${2##*/}
            git clone $2
            shift 2;;
        --)
            shift
            break;;
    esac
done

if [[ -z "$wrapper_prefix" ]]; then
    error "wrapper path not specified"
fi
if [[ ! -d "$dir" ]]; then
    error "repo not specified"
fi

cd $dir
ln -fs "$wrapper_prefix/wrapper"
ln -fs "$wrapper_prefix/wrapper.conf"
ln -fs "$wrapper_prefix/log.ini"

make CC=./wrapper CXX=./wrapper
