#!/bin/sh

FILEPATH=$1
INSTANCES=$2

SRC="filesrc location=$FILEPATH ! queue"
DEC_ELEMENT_X=" ! h264parse ! omxh264dec internal-entropy-buffers=2"
FAKE_SINK_ELEMENT=" ! fakevideosink sync=TRUE"

BRANCH_DEC_X="$SRC $DEC_ELEMENT_X"

CMD="./gst-n-launch-1.0 "
for i in `/usr/bin/seq 1 $INSTANCES`
do
  CMD+=" -b \"$BRANCH_DEC_X ! filesink location=output_${i}.raw\""
done

echo $CMD
eval $CMD
