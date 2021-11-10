#!/bin/bash

export CNN_PERF=0
export VIV_VX_ENABLE_TP=1
export VIV_VX_PROFILE=0
if [ ! -f /dev/galcore ] ;then
  rmmod galcore
  echo "nothing"
fi
sleep 1
insmod ../vip_driver/sdk/drivers/galcore.ko showArgs=1 irqLine=21 registerMemBase=0xf8800000 contiguousSize=0x200000

