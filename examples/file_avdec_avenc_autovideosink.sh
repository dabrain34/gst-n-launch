#!/bin/bash

PATH=$(cd "$(dirname "$1")"; pwd)/$(basename "$1")
FILEPATH=$1
INSTANCES=$2

FILE_ELEMENT="filesrc location=$FILEPATH"
DECODE_BRANCH=" ! decodebin"
CAPS_FILTER=
ENC_ELEMENT=" ! avenc_mpeg2video"
DEC_ELEMENT="! h264parse ! avdec_h264"
SINK_ELEMENT="! h264parse ! autovideosink"
BRANCH1="$FILE_ELEMENT $CAPS_FILTER $DEC_ELEMENT $ENC_ELEMENT"
#BRANCH2="$FILE_ELEMENT $CAPS_FILTER $DECODE_BRANCH $ENC_ELEMENT $DEC_ELEMENT $SINK_ELEMENT"


CMD="./gst-n-launch-1.0"
for i in `/usr/bin/seq 1 $INSTANCES`
do
  CMD+=" -b \"$BRANCH1 ! filesink location=output_${i}.mpeg2\""
done

echo $CMD
eval $CMD
