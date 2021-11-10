#!/bin/bash

export VIVANTE_SDK_DIR=../vip_driver/sdk
export VIV_VX_ENABLE_SAVE_NETWORK_BINARY=1
export LD_LIBRARY_PATH=/usr/lib/arm-linux-gnueabihf/lapack:/usr/lib/arm-linux-gnueabihf:/lib/arm-linux-gnueabihf:/usr/lib:/lib:../vip_driver/sdk/drivers:/usr/lib/arm-linux-gnueabihf/blas/
export VSI_USE_IMAGE_PROCESS=1
#export VNN_LOOP_TIME=300
export VIVANTE_SDK_DIR=../vip_driver/sdk
./bitonic
