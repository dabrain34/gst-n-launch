#!/bin/sh

FILEPATH=$1
INSTANCES=$2

FILE_ELEMENT="filesrc location=$FILEPATH"
QUEUE="! queue max-size-bytes=0"

CAPS_X="! video/x-h265, profile=main"
ENC_ELEMENT_X=" ! omxh265enc control-rate=constant filler-data=false b-frames=3 target-bitrate=1000 prefetch-buffer=true"
DEC_ELEMENT_X=" ! h264parse ! omxh264dec internal-entropy-buffers=2"

BRANCH_DEC_ENC_X="$FILE_ELEMENT $DEC_ELEMENT_X $QUEUE $ENC_ELEMENT_X $CAPS_X"


CMD="./gst-n-launch-1.0 "
for i in `/usr/bin/seq 1 $INSTANCES`
do
  CMD+=" -b \" $BRANCH_DEC_ENC_X ! fakesink\""
done
echo $CMD
eval $CMD

