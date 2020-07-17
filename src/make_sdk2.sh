#!/bin/sh

ROOT=`pwd`
echo "Working dir is '${ROOT}'"

echo "continue ... (y/n) ?" ;read c ;if [ "$c" != "y" ];then exit 0 ;fi

mkdir -p qsdk/dl
echo "mkdir -p qsdk/dl"
cp -rf apss_proc/out/proprietary/Wifi/qsdk-ieee1905-security/* qsdk
echo "cp -rf apss_proc/out/proprietary/Wifi/qsdk-ieee1905-security/* qsdk"
cp -rf apss_proc/out/proprietary/Wifi/qsdk-qca-art/* qsdk
echo "cp -rf apss_proc/out/proprietary/Wifi/qsdk-qca-art/* qsdk"
cp -rf apss_proc/out/proprietary/Wifi/qsdk-qca-wifi/* qsdk
echo "cp -rf apss_proc/out/proprietary/Wifi/qsdk-qca-wifi/* qsdk"
cp -rf apss_proc/out/proprietary/Wifi/qsdk-qca-wlan/* qsdk
echo "cp -rf apss_proc/out/proprietary/Wifi/qsdk-qca-wlan/* qsdk"
cp -rf wlan_proc/src/components/qca-wifi-fw-src-component-cmn-WLAN.HK.* qsdk/dl/
echo "cp -rf wlan_proc/src/components/qca-wifi-fw-src-component-cmn-WLAN.HK.* qsdk/dl/"
cp -rf wlan_proc/pkg/wlan_proc/bin/QCA6018/qca-wifi-fw-QCA6018_v1.0-WLAN.HK.* qsdk/dl/
echo "cp -rf wlan_proc/pkg/wlan_proc/bin/QCA6018/qca-wifi-fw-QCA6018_v1.0-WLAN.HK.* qsdk/dl/"
tar xf cnss_proc/src/components/qca-wifi-fw-src-component-cmn-WLAN.BL.*.tgz -C qsdk/dl
echo "tar xf cnss_proc/src/components/qca-wifi-fw-src-component-cmn-WLAN.BL.*.tgz -C qsdk/dl"
tar xf cnss_proc/src/components/qca-wifi-fw-src-component-halphy_tools-WLAN.BL.*.tgz -C qsdk/dl
echo "tar xf cnss_proc/src/components/qca-wifi-fw-src-component-halphy_tools-WLAN.BL.*.tgz -C qsdk/dl"
cp -rf cnss_proc/src/components/* qsdk/dl
echo "cp -rf cnss_proc/src/components/* qsdk/dl"
cp -rf cnss_proc/bin/QCA9888/hw.2/* qsdk/dl
echo "cp -rf cnss_proc/bin/QCA9888/hw.2/* qsdk/dl"
cp -rf cnss_proc/bin/AR900B/hw.2/* qsdk/dl
echo "cp -rf cnss_proc/bin/AR900B/hw.2/* qsdk/dl"
cp -rf cnss_proc/bin/QCA9984/hw.1/* qsdk/dl
echo "cp -rf cnss_proc/bin/QCA9984/hw.1/* qsdk/dl"
cp -rf cnss_proc/bin/IPQ4019/hw.1/* qsdk/dl
echo "cp -rf cnss_proc/bin/IPQ4019/hw.1/* qsdk/dl"
cp -rf qca-wifi-fw-AR988* qsdk/dl
echo "cp -rf qca-wifi-fw-AR988* qsdk/dl"
cp -rf qca-wifi-fw-AR9887* qsdk/dl
echo "cp -rf qca-wifi-fw-AR9887* qsdk/dl"
cp -rf apss_proc/out/proprietary/QSDK-Base/meta-tools/ qsdk
echo "cp -rf apss_proc/out/proprietary/QSDK-Base/meta-tools/ qsdk"
cp -rf apss_proc/out/proprietary/QSDK-Base/common-tools/* qsdk/
echo "cp -rf apss_proc/out/proprietary/QSDK-Base/common-tools/* qsdk/"
cp -rf apss_proc/out/proprietary/QSDK-Base/qsdk-qca-nss/* qsdk/
echo "cp -rf apss_proc/out/proprietary/QSDK-Base/qsdk-qca-nss/* qsdk/"
cp -rf apss_proc/out/proprietary/QSDK-Base/qca-lib/* qsdk/
echo "cp -rf apss_proc/out/proprietary/QSDK-Base/qca-lib/* qsdk/"
cp -rf apss_proc/out/proprietary/RSRC-NSS-MDUMP/* qsdk
echo "cp -rf apss_proc/out/proprietary/RSRC-NSS-MDUMP/* qsdk"
cp -rf apss_proc/out/proprietary/QSDK-Base/qca-mcs-apps/* qsdk
echo "cp -rf apss_proc/out/proprietary/QSDK-Base/qca-mcs-apps/* qsdk"
cp -rf apss_proc/out/proprietary/QSDK-Base/qca-nss-userspace/* qsdk
echo "cp -rf apss_proc/out/proprietary/QSDK-Base/qca-nss-userspace/* qsdk"
cp -rf apss_proc/out/proprietary/QSDK-Base/qca-time-services/* qsdk
echo "cp -rf apss_proc/out/proprietary/QSDK-Base/qca-time-services/* qsdk"
cp -rf apss_proc/out/proprietary/QSDK-Base/qca-qmi-framework/* qsdk
echo "cp -rf apss_proc/out/proprietary/QSDK-Base/qca-qmi-framework/* qsdk"
cp -rf apss_proc/out/proprietary/QSDK-Base/gpio-debug/* qsdk
echo "cp -rf apss_proc/out/proprietary/QSDK-Base/gpio-debug/* qsdk"
cp -rf apss_proc/out/proprietary/QSDK-Base/qca-diag/* qsdk
echo "cp -rf apss_proc/out/proprietary/QSDK-Base/qca-diag/* qsdk"
cp -rf apss_proc/out/proprietary/QSDK-Base/athtestcmd/* qsdk
echo "cp -rf apss_proc/out/proprietary/QSDK-Base/athtestcmd/* qsdk"

tar xjf apss_proc/out/proprietary/QSDK-Base/qca-IOT/qca-IOT.tar.bz2 -C qsdk
echo "tar xjf apss_proc/out/proprietary/QSDK-Base/qca-IOT/qca-IOT.tar.bz2 -C qsdk"
cp -rf apss_proc/out/proprietary/QSDK-Base/qca-cnss-daemon/* qsdk
echo "cp -rf apss_proc/out/proprietary/QSDK-Base/qca-cnss-daemon/* qsdk"
cp apss_proc/out/proprietary/QSDK-Base/qca-nss-fw-eip-cp/BIN-EIP197.CP.* qsdk/dl/
echo "cp apss_proc/out/proprietary/QSDK-Base/qca-nss-fw-eip-cp/BIN-EIP197.CP.* qsdk/dl/"
sed -i '/QCAHKSWPL_SILICONZ/c\PKG_VERSION:=WLAN.HK.2.2-02237-QCAHKSWPL_SILICONZ-1' qsdk/qca/feeds/qca-cp/net/qca-cyp/Makefile
echo "sed -i '/QCAHKSWPL_SILICONZ/c\PKG_VERSION:=WLAN.HK.2.2-02237-QCAHKSWPL_SILICONZ-1' qsdk/qca/feeds/qca-cp/net/qca-cyp/Makefile"
cp -rf apss_proc/out/proprietary/BLUETOPIA/qca-bluetopia/* qsdk
echo "cp -rf apss_proc/out/proprietary/BLUETOPIA/qca-bluetopia/* qsdk"