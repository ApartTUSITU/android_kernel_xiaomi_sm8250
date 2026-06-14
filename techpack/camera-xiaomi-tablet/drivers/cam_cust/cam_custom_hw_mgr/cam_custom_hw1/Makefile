# SPDX-License-Identifier: GPL-2.0-only

ccflags-y += -I$(srctree)/techpack/camera-xiaomi-tablet/drivers/cam_utils
ccflags-y += -I$(srctree)/techpack/camera-xiaomi-tablet/drivers/cam_core
ccflags-y += -I$(srctree)/techpack/camera-xiaomi-tablet/drivers/cam_cpas/include
ccflags-y += -I$(srctree)/techpack/camera-xiaomi-tablet/drivers/cam_cust/cam_custom_hw_mgr/include
ccflags-y += -I$(srctree)/techpack/camera-xiaomi-tablet/drivers/cam_cust/cam_custom_hw_mgr/
ccflags-y += -I$(srctree)/techpack/camera-xiaomi-tablet/drivers/cam_cust/cam_custom_hw_mgr/cam_custom_hw1

obj-$(CONFIG_SPECTRA_CAMERA) += cam_custom_sub_mod_soc.o cam_custom_sub_mod_dev.o cam_custom_sub_mod_core.o
