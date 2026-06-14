# SPDX-License-Identifier: GPL-2.0-only

ccflags-y += -I$(srctree)/techpack/camera-xiaomi-tablet/include/uapi
ccflags-y += -I$(srctree)/techpack/camera-xiaomi-tablet/drivers/cam_core
ccflags-y += -I$(srctree)/techpack/camera-xiaomi-tablet/drivers/cam_cpas/include
ccflags-y += -I$(srctree)/techpack/camera-xiaomi-tablet/drivers/cam_req_mgr
ccflags-y += -I$(srctree)/techpack/camera-xiaomi-tablet/drivers/cam_sensor_module/cam_cci
ccflags-y += -I$(srctree)/techpack/camera-xiaomi-tablet/drivers/cam_sensor_module/cam_sensor_io
ccflags-y += -I$(srctree)/techpack/camera-xiaomi-tablet/drivers/cam_sensor_module/cam_sensor_utils
ccflags-y += -I$(srctree)/techpack/camera-xiaomi-tablet/drivers/cam_smmu/
ccflags-y += -I$(srctree)/techpack/camera-xiaomi-tablet/drivers/cam_utils

obj-$(CONFIG_SPECTRA_CAMERA) += cam_sensor_dev.o cam_sensor_core.o cam_sensor_soc.o
