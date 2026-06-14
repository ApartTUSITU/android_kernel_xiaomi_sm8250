# SPDX-License-Identifier: GPL-2.0-only

ccflags-y += -I$(srctree)/techpack/camera-xiaomi-tablet/include/uapi
ccflags-y += -I$(srctree)/techpack/camera-xiaomi-tablet/drivers/cam_core
ccflags-y += -I$(srctree)/techpack/camera-xiaomi-tablet/drivers/cam_isp/isp_hw_mgr/include
ccflags-y += -I$(srctree)/techpack/camera-xiaomi-tablet/drivers/cam_req_mgr
ccflags-y += -I$(srctree)/techpack/camera-xiaomi-tablet/drivers/cam_smmu/
ccflags-y += -I$(srctree)/techpack/camera-xiaomi-tablet/drivers/cam_sync
ccflags-y += -I$(srctree)/techpack/camera-xiaomi-tablet/drivers/cam_utils
ccflags-y += -I$(srctree)/techpack/camera-xiaomi-tablet/drivers/cam_cdm/

obj-$(CONFIG_SPECTRA_CAMERA) += isp_hw_mgr/
obj-$(CONFIG_SPECTRA_CAMERA) += cam_isp_dev.o cam_isp_context.o
