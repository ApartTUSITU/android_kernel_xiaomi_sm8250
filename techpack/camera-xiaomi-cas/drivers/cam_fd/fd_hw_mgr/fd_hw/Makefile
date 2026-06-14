# SPDX-License-Identifier: GPL-2.0-only

ccflags-y += -I$(srctree)/techpack/camera-xiaomi-cas/include/uapi
ccflags-y += -I$(srctree)/techpack/camera-xiaomi-cas/drivers/cam_utils
ccflags-y += -I$(srctree)/techpack/camera-xiaomi-cas/drivers/cam_req_mgr
ccflags-y += -I$(srctree)/techpack/camera-xiaomi-cas/drivers/cam_core
ccflags-y += -I$(srctree)/techpack/camera-xiaomi-cas/drivers/cam_sync
ccflags-y += -I$(srctree)/techpack/camera-xiaomi-cas/drivers/cam_smmu
ccflags-y += -I$(srctree)/techpack/camera-xiaomi-cas/drivers/cam_cdm
ccflags-y += -I$(srctree)/techpack/camera-xiaomi-cas/drivers/cam_fd
ccflags-y += -I$(srctree)/techpack/camera-xiaomi-cas/drivers/cam_fd/fd_hw_mgr
ccflags-y += -I$(srctree)/techpack/camera-xiaomi-cas/drivers/cam_fd/fd_hw_mgr/fd_hw
ccflags-y += -I$(srctree)/techpack/camera-xiaomi-cas
ccflags-y += -I$(srctree)/techpack/camera-xiaomi-cas/drivers/cam_cpas/include

obj-$(CONFIG_SPECTRA_CAMERA) += cam_fd_hw_dev.o cam_fd_hw_core.o cam_fd_hw_soc.o
