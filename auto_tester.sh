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

function run_make() {
    ln -fs "$wrapper_prefix/wrapper"
    ln -fs "$wrapper_prefix/wrapper.conf"
    ln -fs "$wrapper_prefix/log.ini"

    make CC=./wrapper CXX=./wrapper || true
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
            dir=$(readlink -f $(basename $2))
            svn checkout $2
            shift 2;;
        --git)
            dir=$(readlink -f $(basename $2))
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

cd "$dir"

for conf in $(find . -name configure); do
    configure_dir=$(dirname "$conf")

    echo -e "\nRunning configure in $(readlink -f $configure_dir):\n\n"

    cd "$configure_dir"
    ./configure || true
    cd "$dir"
done

for mfile in $(find . -name Makefile -o -name makefile -o -name GNUmakefile); do
    makefile_dir=$(dirname "$mfile")

    echo -e "\nCompiling $(readlink -f $makefile_dir):\n\n"

    cd "$makefile_dir"
    run_make
    cd "$dir"
done
