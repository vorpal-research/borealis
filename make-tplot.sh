#!/bin/bash

TPLOT_DIR=tplot

TRACER_FILE=$1
OUTPUT_FILE=$2

TPLOT_FILE=tplot.in

if [ $# -ne 2 ]; then
    echo "Not enough arguments"
    exit -1
fi

$TPLOT_DIR/convert-to-tplot.awk < $TRACER_FILE > $TPLOT_FILE
$TPLOT_DIR/draw-tplot.sh $TPLOT_FILE $OUTPUT_FILE
rm $TPLOT_FILE

exit 0
