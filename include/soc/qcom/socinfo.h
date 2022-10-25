/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2009-2021, The Linux Foundation. All rights reserved.
 * Copyright (c) 2023, Qualcomm Innovation Center, Inc. All rights reserved.
 */

#ifndef _ARCH_ARM_MACH_MSM_SOCINFO_H_
#define _ARCH_ARM_MACH_MSM_SOCINFO_H_

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/of_fdt.h>
#include <linux/of.h>

#include <asm/cputype.h>

enum feature_code {
	/* External feature code */
	SOCINFO_FC_UNKNOWN = 0x0,
	SOCINFO_FC_AA,
	SOCINFO_FC_AB,
	SOCINFO_FC_AC,
	SOCINFO_FC_AD,
	SOCINFO_FC_AE,
	SOCINFO_FC_AF,
	SOCINFO_FC_AG,
	SOCINFO_FC_AH,
	SOCINFO_FC_EXT_RESERVE,

	/* Internal feature code */
	SOCINFO_FC_Y0 = 0xf1,
	SOCINFO_FC_Y1,
	SOCINFO_FC_Y2,
	SOCINFO_FC_Y3,
	SOCINFO_FC_Y4,
	SOCINFO_FC_Y5,
	SOCINFO_FC_Y6,
	SOCINFO_FC_Y7,
	SOCINFO_FC_INT_RESERVE
};

enum pcode {
	SOCINFO_PCODE_UNKNOWN = 0,
	SOCINFO_PCODE_0,
	SOCINFO_PCODE_1,
	SOCINFO_PCODE_2,
	SOCINFO_PCODE_3,
	SOCINFO_PCODE_4,
	SOCINFO_PCODE_5,
	SOCINFO_PCODE_6,
	SOCINFO_PCODE_7,
	SOCINFO_PCODE_8,
	SOCINFO_PCODE_RESERVE = 0x7fffffff
};

enum socinfo_parttype {
	SOCINFO_PART_GPU,
	SOCINFO_PART_VIDEO,
	SOCINFO_PART_CAMERA,
	SOCINFO_PART_DISPLAY,
	SOCINFO_PART_AUDIO,
	SOCINFO_PART_MODEM,
	SOCINFO_PART_WLAN,
	SOCINFO_PART_COMP,
	SOCINFO_PART_SENSORS,
	SOCINFO_PART_NPU,
	SOCINFO_PART_SPSS,
	SOCINFO_PART_NAV,
	SOCINFO_PART_COMPUTE_1,
	SOCINFO_PART_DISPLAY_1,
	SOCINFO_PART_MAX_PARTTYPE
};

/*
 * SOC version type with major number in the upper 16 bits and minor
 * number in the lower 16 bits.  For example:
 *   1.0 -> 0x00010000
 *   2.3 -> 0x00020003
 */
#define SOCINFO_VERSION_MAJOR(ver) (((ver) & 0xffff0000) >> 16)
#define SOCINFO_VERSION_MINOR(ver) ((ver) & 0x0000ffff)
#define SOCINFO_VERSION(maj, min)  ((((maj) & 0xffff) << 16)|((min) & 0xffff))

#ifdef CONFIG_OF
#define of_board_is_cdp()	of_machine_is_compatible("qcom,cdp")
#define of_board_is_sim()	of_machine_is_compatible("qcom,sim")
#define of_board_is_rumi()	of_machine_is_compatible("qcom,rumi")
#define of_board_is_fluid()	of_machine_is_compatible("qcom,fluid")
#define of_board_is_liquid()	of_machine_is_compatible("qcom,liquid")
#define of_board_is_dragonboard()	\
	of_machine_is_compatible("qcom,dragonboard")
#define of_board_is_cdp()	of_machine_is_compatible("qcom,cdp")
#define of_board_is_mtp()	of_machine_is_compatible("qcom,mtp")
#define of_board_is_qrd()	of_machine_is_compatible("qcom,qrd")
#define of_board_is_xpm()	of_machine_is_compatible("qcom,xpm")
#define of_board_is_skuf()	of_machine_is_compatible("qcom,skuf")
#define of_board_is_sbc()	of_machine_is_compatible("qcom,sbc")

#define machine_is_msm8974()	of_machine_is_compatible("qcom,msm8974")

#define early_machine_is_msm8916()	\
	of_flat_dt_is_compatible(of_get_flat_dt_root(), "qcom,msm8916")
#define early_machine_is_apq8084()	\
	of_flat_dt_is_compatible(of_get_flat_dt_root(), "qcom,apq8084")
#define early_machine_is_msm8996()	\
	of_flat_dt_is_compatible(of_get_flat_dt_root(), "qcom,msm8996")
#define early_machine_is_msm8996_auto()	\
	of_flat_dt_is_compatible(of_get_flat_dt_root(), "qcom,msm8996-cdp")
#define early_machine_is_sm8150()	\
	of_flat_dt_is_compatible(of_get_flat_dt_root(), "qcom,sm8150")
#define early_machine_is_sa8150()	\
	of_flat_dt_is_compatible(of_get_flat_dt_root(), "qcom,sa8150")
#define early_machine_is_kona()	\
	of_flat_dt_is_compatible(of_get_flat_dt_root(), "qcom,kona")
#define early_machine_is_kona_7230_iot()	\
	of_flat_dt_is_compatible(of_get_flat_dt_root(), "qcom,kona-7230-iot")
#define early_machine_is_lito()	\
	of_flat_dt_is_compatible(of_get_flat_dt_root(), "qcom,lito")
#define early_machine_is_orchid()	\
	of_flat_dt_is_compatible(of_get_flat_dt_root(), "qcom,orchid")
#define early_machine_is_bengal()	\
	of_flat_dt_is_compatible(of_get_flat_dt_root(), "qcom,bengal")
#define early_machine_is_bengalp()	\
	of_flat_dt_is_compatible(of_get_flat_dt_root(), "qcom,bengalp")
#define early_machine_is_khaje()	\
	of_flat_dt_is_compatible(of_get_flat_dt_root(), "qcom,khaje")
#define early_machine_is_khajep()	\
	of_flat_dt_is_compatible(of_get_flat_dt_root(), "qcom,khajep")
#define early_machine_is_khajeq()	\
	of_flat_dt_is_compatible(of_get_flat_dt_root(), "qcom,khajeq")
#define early_machine_is_khajeg()       \
	of_flat_dt_is_compatible(of_get_flat_dt_root(), "qcom,khajeg")
#define early_machine_is_lagoon()	\
	of_flat_dt_is_compatible(of_get_flat_dt_root(), "qcom,lagoon")
#define early_machine_is_scuba()	\
	of_flat_dt_is_compatible(of_get_flat_dt_root(), "qcom,scuba")
#define early_machine_is_scubaiot()        \
	of_flat_dt_is_compatible(of_get_flat_dt_root(), "qcom,scuba-iot")
#define early_machine_is_scubapiot()        \
	of_flat_dt_is_compatible(of_get_flat_dt_root(), "qcom,scubap-iot")
#define early_machine_is_sdmshrike()	\
	of_flat_dt_is_compatible(of_get_flat_dt_root(), "qcom,sdmshrike")
#define early_machine_is_sm6150()	\
	of_flat_dt_is_compatible(of_get_flat_dt_root(), "qcom,sm6150")
#define early_machine_is_qcs405()	\
	of_flat_dt_is_compatible(of_get_flat_dt_root(), "qcom,qcs405")
#define early_machine_is_sdxprairie()	\
	of_flat_dt_is_compatible(of_get_flat_dt_root(), "qcom,sdxprairie")
#define early_machine_is_sdmmagpie()	\
	of_flat_dt_is_compatible(of_get_flat_dt_root(), "qcom,sdmmagpie")
#define early_machine_is_sdm660()	\
	of_flat_dt_is_compatible(of_get_flat_dt_root(), "qcom,sdm660")
#define early_machine_is_bengal_iot()	\
	of_flat_dt_is_compatible(of_get_flat_dt_root(), "qcom,bengal-iot")
#define early_machine_is_bengalp_iot()	\
	of_flat_dt_is_compatible(of_get_flat_dt_root(), "qcom,bengalp-iot")
#define early_machine_is_msm8937()	\
	of_flat_dt_is_compatible(of_get_flat_dt_root(), "qcom,msm8937")
#define early_machine_is_msm8917()	\
	of_flat_dt_is_compatible(of_get_flat_dt_root(), "qcom,msm8917")
#define early_machine_is_sdm439()	\
	of_flat_dt_is_compatible(of_get_flat_dt_root(), "qcom,sdm439")
#define early_machine_is_sdm429()	\
	of_flat_dt_is_compatible(of_get_flat_dt_root(), "qcom,sdm429")
#define early_machine_is_qm215()	\
	of_flat_dt_is_compatible(of_get_flat_dt_root(), "qcom,qm215")
#define early_machine_is_msm8953()	\
	of_flat_dt_is_compatible(of_get_flat_dt_root(), "qcom,msm8953")
#define early_machine_is_sdm450()	\
	of_flat_dt_is_compatible(of_get_flat_dt_root(), "qcom,sdm450")
#define early_machine_is_qcm2150()	\
	of_flat_dt_is_compatible(of_get_flat_dt_root(), "qcom,qcm2150")
#else
#define of_board_is_sim()		0
#define of_board_is_rumi()		0
#define of_board_is_fluid()		0
#define of_board_is_liquid()		0
#define of_board_is_dragonboard()	0
#define of_board_is_cdp()		0
#define of_board_is_mtp()		0
#define of_board_is_qrd()		0
#define of_board_is_xpm()		0
#define of_board_is_skuf()		0
#define of_board_is_sbc()		0

#define machine_is_msm8974()		0

#define early_machine_is_msm8916()	0
#define early_machine_is_apq8084()	0
#define early_machine_is_msm8996()	0
#define early_machine_is_sm8150()	0
#define early_machine_is_sa8150()	0
#define early_machine_is_kona()		0
#define early_machine_is_lito()		0
#define early_machine_is_orchid()	0
#define early_machine_is_bengal()	0
#define early_machine_is_bengalp()	0
#define early_machine_is_khaje()	0
#define early_machine_is_khajep()	0
#define early_machine_is_khajeq()	0
#define early_machine_is_khajeg()	0
#define early_machine_is_lagoon()	0
#define early_machine_is_scuba()	0
#define early_machine_is_scubaiot()	0
#define early_machine_is_scubapiot()	0
#define early_machine_is_sdmshrike()	0
#define early_machine_is_sm6150()	0
#define early_machine_is_qcs405()	0
#define early_machine_is_sdxprairie()	0
#define early_machine_is_sdmmagpie()	0
#define early_machine_is_sdm660()	0
#define early_machine_is_bengal_iot()	0
#define early_machine_is_bengalp_iot()	0
#define early_machine_is_msm8937()	0
#define early_machine_is_msm8917()	0
#define early_machine_is_sdm439()	0
#define early_machine_is_sdm429()	0
#define early_machine_is_qm215()	0
#define early_machine_is_msm8953()	0
#define early_machine_is_sdm450()	0
#define early_machine_is_qcm2150()	0
#endif

#define PLATFORM_SUBTYPE_MDM	1
#define PLATFORM_SUBTYPE_INTERPOSERV3 2
#define PLATFORM_SUBTYPE_SGLTE	6

#define SMEM_IMAGE_VERSION_TABLE	469
#define SMEM_HW_SW_BUILD_ID		137
#define SMEM_ID_VENDOR1                 135
enum msm_cpu {
	MSM_CPU_UNKNOWN = 0,
	MSM_CPU_8960,
	MSM_CPU_8960AB,
	MSM_CPU_8064,
	MSM_CPU_8974,
	MSM_CPU_8974PRO_AA,
	MSM_CPU_8974PRO_AB,
	MSM_CPU_8974PRO_AC,
	MSM_CPU_8916,
	MSM_CPU_8084,
	MSM_CPU_8996,
	MSM_CPU_SDM660,
	MSM_CPU_SM8150,
	MSM_CPU_SA8150,
	MSM_CPU_KONA,
	MSM_CPU_KONA_IOT,
	MSM_CPU_LITO,
	MSM_CPU_ORCHID,
	MSM_CPU_BENGAL,
	MSM_CPU_BENGALP,
	MSM_CPU_KHAJE,
	MSM_CPU_KHAJEP,
	MSM_CPU_KHAJEQ,
	MSM_CPU_KHAJEG,
	MSM_CPU_LAGOON,
	MSM_CPU_SCUBA,
	MSM_CPU_SCUBAIOT,
	MSM_CPU_SCUBAPIOT,
	MSM_CPU_SDMSHRIKE,
	MSM_CPU_SM6150,
	MSM_CPU_QCS405,
	SDX_CPU_SDXPRAIRIE,
	MSM_CPU_SDMMAGPIE,
	MSM_CPU_BENGAL_IOT,
	MSM_CPU_BENGALP_IOT,
	MSM_CPU_8937,
	MSM_CPU_8917,
	MSM_CPU_SDM439,
	MSM_CPU_SDM429,
	MSM_CPU_QM215,
	MSM_CPU_8953,
	MSM_CPU_SDM450,
	MSM_CPU_QCM2150,
};

struct msm_soc_info {
	enum msm_cpu generic_soc_type;
	char *soc_id_string;
};

enum pmic_model {
	PMIC_MODEL_PM8058	= 13,
	PMIC_MODEL_PM8028	= 14,
	PMIC_MODEL_PM8901	= 15,
	PMIC_MODEL_PM8027	= 16,
	PMIC_MODEL_ISL_9519	= 17,
	PMIC_MODEL_PM8921	= 18,
	PMIC_MODEL_PM8018	= 19,
	PMIC_MODEL_PM8015	= 20,
	PMIC_MODEL_PM8014	= 21,
	PMIC_MODEL_PM8821	= 22,
	PMIC_MODEL_PM8038	= 23,
	PMIC_MODEL_PM8922	= 24,
	PMIC_MODEL_PM8917	= 25,
	PMIC_MODEL_UNKNOWN	= 0xFFFFFFFF
};

enum subset_part_type {
	PART_UNKNOWN      = 0,
	PART_GPU          = 1,
	PART_VIDEO        = 2,
	PART_CAMERA       = 3,
	PART_DISPLAY      = 4,
	PART_AUDIO        = 5,
	PART_MODEM        = 6,
	PART_WLAN         = 7,
	PART_COMP         = 8,
	PART_SENSORS      = 9,
	PART_NPU          = 10,
	PART_SPSS         = 11,
	PART_NAV          = 12,
	PART_COMP1        = 13,
	PART_DISPLAY1     = 14,
	NUM_PARTS_MAX,
};

enum subset_cluster_type {
	CLUSTER_CPUSS      = 0,
	NUM_CLUSTERS_MAX,
};

#define HARDWARE_PLATFORM_UNKNOWN 0
#define HARDWARE_PLATFORM_CMI  1
#define HARDWARE_PLATFORM_UMI  2
#define HARDWARE_PLATFORM_LMI  3
#define HARDWARE_PLATFORM_CAS  7
#define HARDWARE_PLATFORM_APOLLO  8
#define HARDWARE_PLATFORM_ALIOTH  9
#define HARDWARE_PLATFORM_THYME  10
#define HARDWARE_PLATFORM_ENUMA  11
#define HARDWARE_PLATFORM_ELISH  12
#define HARDWARE_PLATFORM_PSYCHE  13
#define HARDWARE_PLATFORM_MUNCH  15
#define HARDWARE_PLATFORM_DAGU  16
#define HARDWARE_PLATFORM_PIPA  4

#define HW_MAJOR_VERSION_B  9
#define HW_MINOR_VERSION_B  1

#define HW_MAJOR_VERSION_SHIFT 16
#define HW_MAJOR_VERSION_MASK  0xFFFF0000
#define HW_MINOR_VERSION_SHIFT 0
#define HW_MINOR_VERSION_MASK  0x0000FFFF
#define HW_COUNTRY_VERSION_MASK 0xFFF00000
#define HW_COUNTRY_VERSION_SHIFT 20
#define HW_BUILD_VERSION_MASK 0x000F0000
#define HW_BUILD_VERSION_SHIFT 16

typedef enum {
	CountryCN = 0,
	CountryGlobal = 1,
	CountryIndia = 2,
	INVALID,
} CountryType;

uint32_t get_hw_version_platform(void);
uint32_t get_hw_country_version(void);
uint32_t get_hw_version_major(void);
uint32_t get_hw_version_minor(void);
uint32_t get_hw_version_build(void);
const char *product_name_get(void);

enum msm_cpu socinfo_get_msm_cpu(void);
uint32_t socinfo_get_id(void);
uint32_t socinfo_get_version(void);
uint32_t socinfo_get_raw_id(void);
char *socinfo_get_build_id(void);
char *socinfo_get_id_string(void);
uint32_t socinfo_get_platform_type(void);
uint32_t socinfo_get_platform_subtype(void);
uint32_t socinfo_get_platform_version(void);
uint32_t socinfo_get_serial_number(void);
uint32_t socinfo_get_cluster_info(enum subset_cluster_type cluster);
bool socinfo_get_part_info(enum subset_part_type part);
enum pmic_model socinfo_get_pmic_model(void);
uint32_t socinfo_get_pmic_die_revision(void);
int __init socinfo_init(void) __must_check;
int socinfo_get_feature_code(void);
int socinfo_get_pcode(void);
char *socinfo_get_partinfo_details(unsigned int part_id);

#endif
