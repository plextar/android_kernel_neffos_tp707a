
# Linux driver folder
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/
ccflags-y += -I$(srctree)/drivers/input/touchscreen/mediatek/$(CONFIG_TOUCHSCREEN_MTK_FTS_DIRECTORY)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include/
ccflags-y += -I$(srctree)/drivers/base
ccflags-y += -I$(srctree)/drivers/misc/mediatek/hwmon/include

ifneq ($(filter TP7%, $(TARGET_PRODUCT)),)
  EXTRA_CFLAGS += -DTPLINK_PRJ
#  ccflags-y += -DTPLINK_PRJ
else ifneq ($(filter L18%, $(TARGET_PRODUCT)),)
  EXTRA_CFLAGS += -DLENOVO_PRJ
else ifneq ($(filter G55%, $(TARGET_PRODUCT)),)
  EXTRA_CFLAGS += -DCROSS_PRJ
else
  EXTRA_CFLAGS += -DTPLINK_PRJ
#unknown project for FT8006U ic  //[lichenggang] close error string
endif

obj-$(CONFIG_TOUCHSCREEN_MTK_FOCALTECH_TS)    +=  focaltech_ex_fun.o
obj-$(CONFIG_TOUCHSCREEN_MTK_FOCALTECH_TS)    +=  focaltech_flash.o
obj-$(CONFIG_TOUCHSCREEN_MTK_FOCALTECH_TS)    +=  focaltech_gesture.o
obj-$(CONFIG_TOUCHSCREEN_MTK_FOCALTECH_TS)    +=  focaltech_core.o
obj-$(CONFIG_TOUCHSCREEN_MTK_FOCALTECH_TS)    +=  focaltech_esdcheck.o
obj-$(CONFIG_TOUCHSCREEN_MTK_FOCALTECH_TS)    +=  focaltech_ex_mode.o
obj-$(CONFIG_TOUCHSCREEN_MTK_FOCALTECH_TS)    +=  focaltech_proximity.o
obj-$(CONFIG_TOUCHSCREEN_MTK_FOCALTECH_TS)    +=  focaltech_i2c.o
obj-$(CONFIG_TOUCHSCREEN_MTK_FOCALTECH_TS)    +=  focaltech_point_report_check.o

obj-$(CONFIG_TOUCHSCREEN_MTK_FOCALTECH_TS) +=     focaltech_test/
obj-$(CONFIG_TOUCHSCREEN_MTK_FOCALTECH_TS) +=     focaltech_flash/
