ifeq ($(CONFIG_BOARD_XIAOMI_SM8250),y)
ifeq ($(CONFIG_BUILD_ARM64_DT_OVERLAY),y)

ifeq ($(CONFIG_BOARD_ALIOTH),y)
dtbo-$(CONFIG_BOARD_ALIOTH) += \
                alioth-sm8250-overlay.dtbo
alioth-sm8250-overlay.dtbo-base := kona.dtb kona-v2.dtb kona-v2.1.dtb
endif

ifeq ($(CONFIG_BOARD_APOLLO),y)
dtbo-$(CONFIG_BOARD_APOLLO) += \
                apollo-sm8250-overlay.dtbo
apollo-sm8250-overlay.dtbo-base := kona.dtb kona-v2.dtb kona-v2.1.dtb
endif

ifeq ($(CONFIG_BOARD_CAS),y)
dtbo-$(CONFIG_BOARD_CAS) += \
                cas-sm8250-overlay.dtbo
cas-sm8250-overlay.dtbo-base := kona.dtb kona-v2.dtb kona-v2.1.dtb
endif

ifeq ($(CONFIG_BOARD_CMI),y)
dtbo-$(CONFIG_BOARD_CMI) += \
                cmi-sm8250-overlay.dtbo
cmi-sm8250-overlay.dtbo-base := kona.dtb kona-v2.dtb kona-v2.1.dtb
endif

ifeq ($(CONFIG_BOARD_ELISH),y)
dtbo-$(CONFIG_BOARD_ELISH) += \
                elish-sm8250-overlay.dtbo
elish-sm8250-overlay.dtbo-base := kona.dtb kona-v2.dtb kona-v2.1.dtb
endif

ifeq ($(CONFIG_BOARD_ENUMA),y)
dtbo-$(CONFIG_BOARD_ENUMA) += \
                enuma-sm8250-overlay.dtbo
enuma-sm8250-overlay.dtbo-base := kona.dtb kona-v2.dtb kona-v2.1.dtb
endif

ifeq ($(CONFIG_BOARD_LMI),y)
dtbo-$(CONFIG_BOARD_LMI) += \
                lmi-sm8250-overlay.dtbo
lmi-sm8250-overlay.dtbo-base := kona.dtb kona-v2.dtb kona-v2.1.dtb
endif

ifeq ($(CONFIG_BOARD_THYME),y)
dtbo-$(CONFIG_BOARD_THYME) += \
                thyme-sm8250-overlay.dtbo
thyme-sm8250-overlay.dtbo-base := kona.dtb kona-v2.dtb kona-v2.1.dtb
endif

ifeq ($(CONFIG_BOARD_UMI),y)
dtbo-$(CONFIG_BOARD_UMI) += \
                umi-sm8250-overlay.dtbo
umi-sm8250-overlay.dtbo-base := kona.dtb kona-v2.dtb kona-v2.1.dtb
endif

endif
endif

ifeq ($(CONFIG_BOARD_XIAOMI_SM7250),y)
ifeq ($(CONFIG_BUILD_ARM64_DT_OVERLAY),y)

ifeq ($(CONFIG_BOARD_MONET),y)
dtbo-$(CONFIG_BOARD_MONET) += \
                moent-sm7250-overlay.dtbo
dtb-$(CONFIG_BOARD_MONET) += \
                moent-sm7250.dtb
monet-sm7250-overlay.dtbo-base := lito.dtb lito-v2.dtb
endif

ifeq ($(CONFIG_BOARD_PICASSO),y)
dtbo-$(CONFIG_BOARD_PICASSO) += \
                picasso-sm7250-overlay.dtbo
dtb-$(CONFIG_BOARD_PICASSO) += \
                picasso-sm7250.dtb
picasso-sm7250-overlay.dtbo-base := lito.dtb lito-v2.dtb
endif

ifeq ($(CONFIG_BOARD_VANGOGH),y)
dtbo-$(CONFIG_BOARD_VANGOGH) += \
                vangogh-sm7250-overlay.dtbo
dtb-$(CONFIG_BOARD_VANGOGH) += \
                vangogh-sm7250.dtb
vangogh-sm7250-overlay.dtbo-base := lito.dtb lito-v2.dtb
endif

endif
else
ifeq ($(CONFIG_BUILD_ARM64_DT_OVERLAY),y)
        dtbo-$(CONFIG_ARCH_KONA) += \
                kona-cdp-overlay.dtbo \
                kona-cdp-lcd-overlay.dtbo \
                kona-cdp-lcd-tron-overlay.dtbo \
                kona-mtp-overlay.dtbo \
                kona-mtp-ws-overlay.dtbo \
		kona-sa-mtp-overlay.dtbo \
                kona-xr-overlay.dtbo \
                kona-rumi-overlay.dtbo \
                kona-qrd-overlay.dtbo \
                kona-xrfusion-overlay.dtbo \
                kona-xrfusion-ult-overlay.dtbo \
                kona-arglass-overlay.dtbo \
                kona-xrsku4-overlay.dtbo \
                kona-hdk-overlay.dtbo

kona-cdp-overlay.dtbo-base := kona.dtb kona-v2.dtb kona-v2.1.dtb
kona-cdp-lcd-overlay.dtbo-base := kona.dtb kona-v2.dtb kona-v2.1.dtb
kona-cdp-lcd-tron-overlay.dtbo-base := kona.dtb kona-v2.dtb kona-v2.1.dtb
kona-mtp-overlay.dtbo-base := kona.dtb kona-v2.dtb kona-v2.1.dtb
kona-mtp-ws-overlay.dtbo-base := kona.dtb kona-v2.dtb kona-v2.1.dtb
kona-sa-mtp-overlay.dtbo-base := kona.dtb kona-v2.dtb kona-v2.1.dtb
kona-xr-overlay.dtbo-base := kona.dtb kona-v2.dtb kona-v2.1.dtb
kona-rumi-overlay.dtbo-base := kona.dtb kona-v2.dtb kona-v2.1.dtb
kona-qrd-overlay.dtbo-base := kona.dtb kona-v2.dtb kona-v2.1.dtb
kona-xrfusion-overlay.dtbo-base := kona.dtb kona-v2.dtb kona-v2.1.dtb
kona-xrfusion-ult-overlay.dtbo-base := kona.dtb kona-v2.dtb kona-v2.1.dtb
kona-arglass-overlay.dtbo-base := kona.dtb kona-v2.dtb kona-v2.1.dtb
kona-xrsku4-overlay.dtbo-base := kona.dtb kona-v2.dtb kona-v2.1.dtb
kona-hdk-overlay.dtbo-base := kona.dtb kona-v2.dtb kona-v2.1.dtb
else
dtb-$(CONFIG_ARCH_KONA) += kona-rumi.dtb \
        kona-mtp.dtb \
        kona-mtp-ws.dtb \
        kona-mtp-sa.dtb \
        kona-xr.dtb \
        kona-xrfusion.dtb \
        kona-xrfusion-ult.dtb \
        kona-arglass.dtb \
        kona-xrsku4.dtb \
        kona-cdp.dtb \
        kona-cdp-lcd.dtb \
        kona-cdp-lcd-tron.dtb \
        kona-qrd.dtb \
        kona-v2-rumi.dtb \
        kona-v2-mtp.dtb \
        kona-v2-mtp-ws.dtb \
        kona-v2-mtp-sa.dtb \
        kona-v2-cdp.dtb \
        kona-v2-qrd.dtb \
        kona-v2-xrfusion.dtb \
        kona-v2-xrfusion-ult.dtb \
        kona-v2-arglass.dtb \
        kona-v2-xrsku4.dtb \
        kona-hdk.dtb \
        kona-v2.1-mtp.dtb \
        kona-v2.1-mtp-ws.dtb \
        kona-v2.1-mtp-sa.dtb \
        kona-v2.1-cdp.dtb \
        kona-v2.1-qrd.dtb \
        kona-v2.1-hdk.dtb \
        kona-v2.1-xrfusion.dtb \
        kona-v2.1-xrfusion-ult.dtb \
        kona-v2.1-arglass.dtb \
        kona-v2.1-xrsku4.dtb \
        qrb5165-iot-rb5.dtb \
        qrb5165m-iot-rb5.dtb \
        qrb5165n-iot-rb5.dtb \
        qrb5165n-v2-iot-rb5.dtb \
        kona-v2.1-iot-rb5.dtb \
        kona-7230-iot-rb5.dtb \
        kona-7230m-iot-rb5.dtb

endif

ifeq ($(CONFIG_BUILD_ARM64_DT_OVERLAY),y)
dtbo-$(CONFIG_ARCH_LITO) += lito-rumi-overlay.dtbo \
                lito-mtp-overlay.dtbo \
                lito-v2-mtp-overlay.dtbo \
                lito-cdp-overlay.dtbo \
                lito-v2-cdp-overlay.dtbo \
                lito-atp-overlay.dtbo \
                lito-v2-atp-overlay.dtbo \
                lito-v2-atp-lcd-overlay.dtbo \
                lito-qrd-overlay.dtbo \
                lito-v2-qrd-overlay.dtbo \
                litomagnus-mtp-overlay.dtbo \
                litomagnus-cdp-overlay.dtbo \
                orchid-mtp-overlay.dtbo \
                orchid-cdp-overlay.dtbo

lito-rumi-overlay.dtbo-base := lito.dtb lito-v2.dtb
lito-mtp-overlay.dtbo-base := lito.dtb
lito-v2-mtp-overlay.dtbo-base := lito-v2.dtb
lito-cdp-overlay.dtbo-base := lito.dtb
lito-v2-cdp-overlay.dtbo-base := lito-v2.dtb
lito-atp-overlay.dtbo-base := lito.dtb
lito-v2-atp-overlay.dtbo-base := lito-v2.dtb
lito-v2-atp-lcd-overlay.dtbo-base := lito-v2.dtb
lito-qrd-overlay.dtbo-base := lito.dtb
lito-v2-qrd-overlay.dtbo-base := lito-v2.dtb
litomagnus-mtp-overlay.dtbo-base := litomagnus.dtb
litomagnus-cdp-overlay.dtbo-base := litomagnus.dtb
orchid-mtp-overlay.dtbo-base := orchid.dtb
orchid-cdp-overlay.dtbo-base := orchid.dtb
else
dtb-$(CONFIG_ARCH_LITO) += lito-rumi.dtb \
                lito-mtp.dtb \
                lito-cdp.dtb \
                lito-atp.dtb \
                lito-qrd.dtb \
                lito-v2-mtp.dtb \
                lito-v2-cdp.dtb \
                lito-v2-atp.dtb \
                lito-v2-atp-lcd.dtb \
                lito-v2-qrd.dtb \
                litomagnus-mtp.dtb \
                litomagnus-cdp.dtb \
                orchid-mtp.dtb \
                orchid-cdp.dtb
endif

ifeq ($(CONFIG_BUILD_ARM64_DT_OVERLAY),y)
dtbo-$(CONFIG_ARCH_LAGOON) += \
		lagoon-rumi-overlay.dtbo \
		lagoon-mtp-overlay.dtbo \
		lagoon-mtp-usbc-overlay.dtbo \
		lagoon-cdp-overlay.dtbo \
		lagoon-atp-overlay.dtbo \
		lagoon-qrd-overlay.dtbo

lagoon-rumi-overlay.dtbo-base := lagoon.dtb
lagoon-mtp-overlay.dtbo-base := lagoon.dtb
lagoon-mtp-usbc-overlay.dtbo-base := lagoon.dtb
lagoon-cdp-overlay.dtbo-base := lagoon.dtb
lagoon-atp-overlay.dtbo-base := lagoon.dtb
lagoon-qrd-overlay.dtbo-base := lagoon.dtb
else
dtb-$(CONFIG_ARCH_LAGOON) += \
		lagoon-rumi.dtb \
		lagoon-mtp.dtb \
		lagoon-mtp-usbc.dtb \
		lagoon-cdp.dtb \
		lagoon-atp.dtb \
		lagoon-qrd.dtb
endif

ifeq ($(CONFIG_BUILD_ARM64_DT_OVERLAY),y)
        dtbo-$(CONFIG_ARCH_BENGAL) += \
                bengal-rumi-overlay.dtbo \
                bengal-qrd-overlay.dtbo \
                bengal-idp-overlay.dtbo \
                bengal-idp-nopmi-overlay.dtbo \
                bengal-idp-usbc-overlay.dtbo \
                bengalp-idp-overlay.dtbo \
                bengal-idp-1gb-overlay.dtbo \
                bengal-idp-2gb-overlay.dtbo \
                bengal-idp-usbc-1gb-overlay.dtbo \
                bengal-idp-usbc-2gb-overlay.dtbo \
                bengal-iot-idp-overlay.dtbo \
                bengalp-iot-idp-overlay.dtbo \
                bengal-iot-idp-2gb-overlay.dtbo \
                bengal-iot-idp-usbc-2gb-overlay.dtbo \
                bengal-iot-idp-usbc-overlay.dtbo \
                bengalp-iot-idp-2gb-overlay.dtbo \
                bengalp-iot-idp-usbc-2gb-overlay.dtbo \
                bengalp-iot-idp-usbc-overlay.dtbo

bengal-rumi-overlay.dtbo-base := bengal.dtb
bengal-qrd-overlay.dtbo-base := bengal.dtb
bengal-idp-overlay.dtbo-base := bengal.dtb
bengal-idp-nopmi-overlay.dtbo-base := bengal.dtb
bengal-idp-usbc-overlay.dtbo-base := bengal.dtb
bengalp-idp-overlay.dtbo-base := bengalp.dtb
bengal-idp-1gb-overlay.dtbo-base := bengal-1gb.dtb
bengal-idp-2gb-overlay.dtbo-base := bengal-2gb.dtb
bengal-idp-usbc-1gb-overlay.dtbo-base := bengal-1gb.dtb
bengal-idp-usbc-2gb-overlay.dtbo-base := bengal-2gb.dtb
bengal-iot-idp-overlay.dtbo-base := bengal-iot.dtb
bengalp-iot-idp-overlay.dtbo-base := bengalp-iot.dtb
bengal-iot-idp-2gb-overlay.dtbo-base := bengal-iot-2gb.dtb
bengal-iot-idp-usbc-2gb-overlay.dtbo-base := bengal-iot-2gb.dtb
bengal-iot-idp-usbc-overlay.dtbo-base := bengal-iot.dtb
bengalp-iot-idp-2gb-overlay.dtbo-base := bengalp-iot-2gb.dtb
bengalp-iot-idp-usbc-2gb-overlay.dtbo-base := bengalp-iot-2gb.dtb
bengalp-iot-idp-usbc-overlay.dtbo-base := bengalp-iot.dtb
else
dtb-$(CONFIG_ARCH_BENGAL) += bengal-rumi.dtb \
                bengal-qrd.dtb \
		bengal-idp.dtb \
		bengal-idp-nopmi.dtb \
		bengal-idp-usbc.dtb \
		bengalp-idp.dtb \
		bengal-idp-1gb.dtb \
		bengal-idp-2gb.dtb \
		bengal-idp-usbc-1gb.dtb \
		bengal-idp-usbc-2gb.dtb \
		bengal-iot-idp.dtb \
		bengalp-iot-idp.dtb \
		bengal-iot-idp-2gb.dtb \
		bengal-iot-idp-usbc-2gb.dtb \
		bengal-iot-idp-usbc.dtb \
		bengalp-iot-idp-2gb.dtb \
		bengalp-iot-idp-usbc-2gb.dtb \
		bengalp-iot-idp-usbc.dtb \
		bengalp-iot-idp-2gb.dtb
endif

ifeq ($(CONFIG_BUILD_ARM64_DT_OVERLAY),y)
        dtbo-$(CONFIG_ARCH_SCUBA) += \
		scuba-rumi-overlay.dtbo \
		scuba-idp-overlay.dtbo \
		scuba-idp-usbc-overlay.dtbo \
		scuba-qrd-eldo-overlay.dtbo \
		scuba-qrd-non-eldo-overlay.dtbo \
		scuba-iot-idp-overlay.dtbo \
		scuba-iot-idp-usbc-overlay.dtbo \
		scuba-iot-qrd-eldo-overlay.dtbo \
		scuba-iot-qrd-non-eldo-overlay.dtbo \
		scuba-iot-rb1-overlay.dtbo \
		scubalite-iot-idp-overlay.dtbo

scuba-rumi-overlay.dtbo-base := scuba.dtb scubap.dtb scuba-2gb.dtb
scuba-idp-overlay.dtbo-base := scuba.dtb scubap.dtb scuba-2gb.dtb
scuba-qrd-eldo-overlay.dtbo-base := scuba.dtb scubap.dtb scuba-2gb.dtb
scuba-qrd-non-eldo-overlay.dtbo-base := scuba.dtb scubap.dtb scuba-2gb.dtb
scuba-idp-usbc-overlay.dtbo-base := scuba.dtb scubap.dtb scuba-2gb.dtb
scuba-iot-idp-overlay.dtbo-base := scuba-iot.dtb scuba-iot-2gb.dtb scubap-iot.dtb scubap-iot-idp.dtb scubap-iot-idp-2gb.dtb
scuba-iot-qrd-eldo-overlay.dtbo-base := scuba-iot.dtb scubap-iot.dtb scuba-iot-2gb.dtb
scuba-iot-qrd-non-eldo-overlay.dtbo-base := scuba-iot.dtb scubap-iot.dtb scuba-iot-2gb.dtb
scuba-iot-rb1-overlay.dtbo-base := scubap-iot-2gb.dtb
scuba-iot-idp-usbc-overlay.dtbo-base := scuba-iot.dtb scubap-iot.dtb scuba-iot-2gb.dtb
scubalite-iot-idp-overlay.dtbo-base := scuba-iot-2gb.dtb scubap-iot-2gb.dtb
else
dtb-$(CONFIG_ARCH_SCUBA) += scuba-rumi.dtb \
	scuba-idp.dtb \
	scuba-idp-usbc.dtb \
	scuba-qrd-eldo.dtb \
	scuba-qrd-non-eldo.dtb \
	scubap-idp.dtb \
	scubap-idp-2gb.dtb \
	scuba-idp-2gb.dtb \
	scuba-idp-usbc-2gb.dtb \
	scuba-iot-idp.dtb \
	scuba-iot-idp-usbc.dtb \
	scubap-iot-idp.dtb \
	scuba-iot-qrd-eldo.dtb \
	scuba-iot-qrd-non-eldo.dtb \
	scubap-iot-rb1-2gb.dtb \
	scubap-iot-idp-2gb.dtb \
	scuba-iot-idp-2gb.dtb \
	scuba-iot-idp-usbc-2gb.dtb \
	scubalite-iot-idp-2gb.dtb \
	scubaplite-iot-idp-2gb.dtb
endif

ifeq ($(CONFIG_BUILD_ARM64_DT_OVERLAY),y)
        dtbo-$(CONFIG_ARCH_KHAJE) += \
		khaje-atp-overlay.dtbo \
		khaje-qrd-overlay.dtbo \
		khaje-qrd-hvdcp3p5-overlay.dtbo \
		khaje-qrd-nowcd9375-overlay.dtbo \
		khaje-qrd-nopmi-overlay.dtbo \
		khaje-idp-overlay.dtbo \
		khaje-idp-nopmi-overlay.dtbo \
		khaje-idp-usbc-overlay.dtbo \
		khaje-idp-pm8010-overlay.dtbo \
		khaje-idps-display-90hz-overlay.dtbo \
		khajep-atp-overlay.dtbo \
		khajep-qrd-overlay.dtbo \
		khajep-qrd-hvdcp3p5-overlay.dtbo \
		khajep-qrd-nowcd9375-overlay.dtbo \
		khajep-qrd-nopmi-overlay.dtbo \
		khajep-idp-overlay.dtbo \
		khajep-idp-nopmi-overlay.dtbo \
		khajep-idp-usbc-overlay.dtbo \
		khajep-idp-pm8010-overlay.dtbo \
		khajep-idps-display-90hz-overlay.dtbo \
		khajeq-atp-overlay.dtbo \
		khajeq-qrd-overlay.dtbo \
		khajeq-qrd-hvdcp3p5-overlay.dtbo \
		khajeq-qrd-nowcd9375-overlay.dtbo \
		khajeq-qrd-nopmi-overlay.dtbo \
		khajeq-idp-overlay.dtbo \
		khajeq-idp-nopmi-overlay.dtbo \
		khajeq-idp-usbc-overlay.dtbo \
		khajeq-idp-pm8010-overlay.dtbo \
		khajeq-idps-display-90hz-overlay.dtbo \
		khajeg-atp-overlay.dtbo \
		khajeg-qrd-overlay.dtbo \
		khajeg-qrd-hvdcp3p5-overlay.dtbo \
		khajeg-qrd-nowcd9375-overlay.dtbo \
		khajeg-qrd-nopmi-overlay.dtbo \
		khajeg-idp-overlay.dtbo \
		khajeg-idp-nopmi-overlay.dtbo \
		khajeg-idp-usbc-overlay.dtbo \
		khajeg-idp-pm8010-overlay.dtbo \
		khajeg-idps-display-90hz-overlay.dtbo

khaje-atp-overlay.dtbo-base := khaje.dtb
khaje-idp-overlay.dtbo-base := khaje.dtb
khaje-idp-nopmi-overlay.dtbo-base := khaje.dtb
khaje-idp-usbc-overlay.dtbo-base := khaje.dtb
khaje-idp-pm8010-overlay.dtbo-base := khaje.dtb
khaje-idps-display-90hz-overlay.dtbo-base := khaje.dtb
khaje-qrd-overlay.dtbo-base := khaje.dtb
khaje-qrd-hvdcp3p5-overlay.dtbo-base := khaje.dtb
khaje-qrd-nopmi-overlay.dtbo-base := khaje.dtb
khaje-qrd-nowcd9375-overlay.dtbo-base := khaje.dtb
khajep-atp-overlay.dtbo-base := khajep.dtb
khajep-idps-display-90hz-overlay.dtbo-base := khajep.dtb
khajep-idp-overlay.dtbo-base := khajep.dtb
khajep-idp-nopmi-overlay.dtbo-base := khajep.dtb
khajep-idp-usbc-overlay.dtbo-base := khajep.dtb
khajep-idp-pm8010-overlay.dtbo-base := khajep.dtb
khajep-qrd-overlay.dtbo-base := khajep.dtb
khajep-qrd-hvdcp3p5-overlay.dtbo-base := khajep.dtb
khajep-qrd-nowcd9375-overlay.dtbo-base := khajep.dtb
khajep-qrd-nopmi-overlay.dtbo-base := khajep.dtb
khajeq-atp-overlay.dtbo-base := khajeq.dtb
khajeq-qrd-overlay.dtbo-base := khajeq.dtb
khajeq-qrd-hvdcp3p5-overlay.dtbo-base := khajeq.dtb
khajeq-qrd-nowcd9375-overlay.dtbo-base := khajeq.dtb
khajeq-qrd-nopmi-overlay.dtbo-base := khajeq.dtb
khajeq-idps-display-90hz-overlay.dtbo-base := khajeq.dtb
khajeq-idp-overlay.dtbo-base := khajeq.dtb
khajeq-idp-nopmi-overlay.dtbo-base := khajeq.dtb
khajeq-idp-pm8010-overlay.dtbo-base := khajeq.dtb
khajeq-idp-usbc-overlay.dtbo-base := khajeq.dtb
khajeg-atp-overlay.dtbo-base := khajeg.dtb
khajeg-qrd-overlay.dtbo-base := khajeg.dtb
khajeg-qrd-hvdcp3p5-overlay.dtbo-base := khajeg.dtb
khajeg-qrd-nowcd9375-overlay.dtbo-base := khajeg.dtb
khajeg-qrd-nopmi-overlay.dtbo-base := khajeg.dtb
khajeg-idp-overlay.dtbo-base := khajeg.dtb
khajeg-idp-nopmi-overlay.dtbo-base := khajeg.dtb
khajeg-idp-pm8010-overlay.dtbo-base := khajeg.dtb
khajeg-idp-usbc-overlay.dtbo-base := khajeg.dtb
khajeg-idps-display-90hz-overlay.dtbo-base := khajeg.dtb
else
dtb-$(CONFIG_ARCH_KHAJE) += khaje-idp.dtb \
		khaje-qrd.dtb \
		khaje-qrd-hvdcp3p5.dtb \
		khaje-qrd-nowcd9375.dtb \
		khaje-idp-nopmi.dtb \
		khaje-idp-usbc.dtb \
		khaje-idp-pm8010.dtb \
		khaje-qrd-nopmi.dtb \
		khaje-idps-display-90hz.dtb \
		khaje-atp.dtb \
		khajep-atp.dtb \
		khajep-qrd.dtb \
		khajep-qrd-hvdcp3p5.dtb \
		khajep-idps-display-90hz.dtb \
		khajep-idp-nopmi.dtb \
		khajep-idp-usbc.dtb \
		khajep-idp-pm8010.dtb \
		khajep-qrd-nowcd9375.dtb \
		khajep-qrd-nopmi.dtb \
		khajeq-atp.dtb \
		khajeq-idps-display-90hz.dtb \
		khajeq-idp-nopmi.dtb \
		khajeq-idp-usbc.dtb \
		khajeq-idp-pm8010.dtb \
		khajeq-qrd.dtb \
		khajeq-qrd-nowcd9375.dtb \
		khajeq-qrd-hvdcp3p5.dtb \
		khajeq-qrd-nopmi.dtb \
		khajeg-atp.dtb \
		khajeg-qrd.dtb \
		khajeg-qrd-nowcd9375.dtb \
		khajeg-qrd-hvdcp3p5.dtb \
		khajeg-qrd-nopmi.dtb \
		khajeg-idp.dtb \
		khajeg-idp-nopmi.dtb \
		khajeg-idp-usbc.dtb \
		khajeg-idp-pm8010.dtb \
		khajeg-idps-display-90hz.dtb
endif

ifeq ($(CONFIG_BUILD_ARM64_DT_OVERLAY),y)
        dtbo-$(CONFIG_ARCH_SDM660) += \
                sdm660-mtp-external-codec-overlay.dtbo \
                sdm660-mtp-internal-codec-overlay.dtbo \
                sdm660-cdp-external-codec-overlay.dtbo \
                sdm660-cdp-internal-codec-overlay.dtbo \
                sdm660-qrd-external-codec-overlay.dtbo \
                sdm660-qrd-dsi-rm67195-amoled-overlay.dtbo \
                sdm660-rcm-external-codec-overlay.dtbo \
                sdm660-rcm-internal-codec-overlay.dtbo \
                sda660-mtp-external-codec-overlay.dtbo \
                sda660-cdp-external-codec-overlay.dtbo \
                sda660-rcm-external-codec-overlay.dtbo

sdm660-mtp-external-codec-overlay.dtbo-base := sdm660-pm660l.dtb

sdm660-mtp-internal-codec-overlay.dtbo-base := sdm660-pm660l.dtb

sdm660-cdp-external-codec-overlay.dtbo-base := sdm660-pm660l.dtb

sdm660-cdp-internal-codec-overlay.dtbo-base := sdm660-pm660l.dtb

sdm660-qrd-external-codec-overlay.dtbo-base := sdm660-pm660l.dtb

sdm660-qrd-dsi-rm67195-amoled-overlay.dtbo-base := sdm660-pm660a.dtb

sdm660-rcm-external-codec-overlay.dtbo-base := sdm660-pm660l.dtb

sdm660-rcm-internal-codec-overlay.dtbo-base := sdm660-pm660l.dtb

sda660-mtp-external-codec-overlay.dtbo-base := sda660-pm660l.dtb

sda660-cdp-external-codec-overlay.dtbo-base := sda660-pm660l.dtb

sda660-rcm-external-codec-overlay.dtbo-base := sda660-pm660l.dtb
else
dtb-$(CONFIG_ARCH_SDM660) += sdm660-sim.dtb \
sdm660-internal-codec-cdp.dtb \
        sdm660-internal-codec-mtp.dtb \
        sdm660-internal-codec-rcm.dtb \
        sdm660-cdp.dtb \
        sdm660-mtp.dtb \
        sdm660-qrd.dtb \
        sdm660-rcm.dtb \
        sdm660-pm660a-cdp.dtb \
        sdm660-pm660a-mtp.dtb \
        sdm660-pm660a-qrd.dtb \
        sdm660-pm660a-rcm.dtb \
        sdm660-internal-codec-pm660a-cdp.dtb \
        sdm660-internal-codec-pm660a-mtp.dtb \
        sdm660-internal-codec-pm660a-rcm.dtb \
        sdm660-pm660a-sim.dtb \
        sda660-cdp.dtb \
        sda660-mtp.dtb \
        sda660-rcm.dtb \
        sda660-pm660a-cdp.dtb \
        sda660-pm660a-mtp.dtb \
        sda660-pm660a-rcm.dtb \
        sda660-pm660a-qrd-hdk.dtb \
        sdm660-headset-jacktype-no-cdp.dtb \
        sdm660-headset-jacktype-no-rcm.dtb \
        sdm660-pm660a-headset-jacktype-no-cdp.dtb \
        sdm660-pm660a-headset-jacktype-no-rcm.dtb \
        sdm660-usbc-audio-mtp.dtb \
        sdm660-usbc-audio-rcm.dtb \
	sdm660-qrd-dsi-rm67195-amoled.dtb
endif

ifeq ($(CONFIG_BUILD_ARM64_DT_OVERLAY),y)

dtbo-$(CONFIG_ARCH_SDM439) += sdm439-mtp-overlay.dtbo \
        sdm439-cdp-overlay.dtbo \
        sdm439-qrd-overlay.dtbo \
        sdm439-external-codec-mtp-overlay.dtbo \
        sdm439-rcm-overlay.dtbo

dtbo-$(CONFIG_ARCH_SDM429) += sdm429-mtp-overlay.dtbo \
	sdm429-cdp-overlay.dtbo \
	sdm429-qrd-overlay.dtbo

dtbo-$(CONFIG_ARCH_QM215) +=qm215-qrd-overlay.dtbo \
	qcm2150-qrd-overlay.dtbo \
	qm215-qrd-smb1360-overlay.dtbo

sdm439-mtp-overlay.dtbo-base := sdm439.dtb \
	sda439.dtb \
	msm8937-interposer-sdm439.dtb

sdm439-cdp-overlay.dtbo-base := sdm439.dtb \
        sda439.dtb \
        msm8937-interposer-sdm439.dtb

sdm439-qrd-overlay.dtbo-base := sdm439.dtb \
        msm8937-interposer-sdm439.dtb

sdm439-external-codec-mtp-overlay.dtbo-base := sdm439.dtb
sdm439-rcm-overlay.dtbo-base := sdm439.dtb


sdm429-mtp-overlay.dtbo-base := sdm429.dtb \
	sda429.dtb \
	msm8937-interposer-sdm429.dtb

sdm429-cdp-overlay.dtbo-base := sdm429.dtb \
	sda429.dtb \
	msm8937-interposer-sdm429.dtb

sdm429-qrd-overlay.dtbo-base := sdm429.dtb \
	msm8937-interposer-sdm429.dtb

qm215-qrd-overlay.dtbo-base := qm215.dtb
qcm2150-qrd-overlay.dtbo-base := qcm2150.dtb
qm215-qrd-smb1360-overlay.dtbo-base := qm215.dtb
else

dtb-$(CONFIG_ARCH_SDM439) += sdm439-mtp.dtb \
	sdm439-cdp.dtb \
	sdm439-qrd.dtb \
	sda439-mtp.dtb \
	sda439-cdp.dtb \
	sdm439-external-codec-mtp.dtb \
	sdm439-rcm.dtb
dtb-$(CONFIG_ARCH_QM215) += qm215-qrd.dtb \
	qcm2150-qrd.dtb \
	qm215-qrd-smb1360.dtb

dtb-$(CONFIG_ARCH_SDM429) += sdm429-mtp.dtb \
	sdm429-cdp.dtb \
	sdm429-qrd.dtb \
	sda429-mtp.dtb \
	sda429-cdp.dtb
endif
endif

always		:= $(dtb-y)
subdir-y	:= $(dts-dirs)
clean-files    := *.dtb *.dtbo
