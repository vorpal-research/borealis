# /bin/env bash

dir=$1
from=$2
to=$3

from_upper=$(echo $from | tr [:lower:] [:upper:])
to_upper=$(echo $to | tr [:lower:] [:upper:])

set -v

echo $from
echo $to

echo $from_upper
echo $to_upper

sed -e "s/$from/$to/g" -e "s/$from_upper/$to_upper/g" $dir/$from.h > $dir/$to.h
sed -e "s/$from/$to/g" -e "s/$from_upper/$to_upper/g" $dir/$from.cpp > $dir/$to.cpp

