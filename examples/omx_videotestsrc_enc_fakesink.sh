#!/bin/sh

INSTANCES=$1
SRC="videotestsrc num-buffers=100"
ENC_ELEMENT_X=" ! omxh265enc control-rate=constant filler-data=false b-frames=3 target-bitrate=1000 prefetch-buffer=true"
FAKE_SINK_ELEMENT=" ! fakesink"


BRANCH_ENC_DEC_X="$SRC $ENC_ELEMENT_X $FAKE_SINK_ELEMENT"

CMD="./gst-n-launch-1.0 "
for i in `/usr/bin/seq 1 $INSTANCES`
do
  CMD+=" -b \"$BRANCH_ENC_DEC_X\""
  #CMD+=" -b \"$BRANCH_ENC_DEC_X \""
done

echo $CMD
eval $CMD
