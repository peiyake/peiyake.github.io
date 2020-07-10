#!/bin/sh

ROOT=`pwd`
echo "Working dir is '${ROOT}'"

echo "continue ... (y/n) ?" ;read c ;if [ "$c" != "y" ];then exit 0 ;fi

cp -rf $ROOT/NHSS.QSDK.11.1/apss_proc/out/proprietary/Wifi/qsdk-qca-wifi $ROOT/
echo "cp -rf $ROOT/NHSS.QSDK.11.1/apss_proc/out/proprietary/Wifi/qsdk-qca-wifi $ROOT/"
cp -rf $ROOT/NHSS.QSDK.11.1/apss_proc/out/proprietary/Wifi/qsdk-qca-wlan $ROOT/
echo "cp -rf $ROOT/NHSS.QSDK.11.1/apss_proc/out/proprietary/Wifi/qsdk-qca-wlan $ROOT/"
cp -rf $ROOT/NHSS.QSDK.11.1/apss_proc/out/proprietary/Wifi/qsdk-qca-art $ROOT/
echo "cp -rf $ROOT/NHSS.QSDK.11.1/apss_proc/out/proprietary/Wifi/qsdk-qca-art $ROOT/"
cp -rf $ROOT/NHSS.QSDK.11.1/apss_proc/out/proprietary/Wifi/qsdk-ieee1905-security $ROOT/
echo "cp -rf $ROOT/NHSS.QSDK.11.1/apss_proc/out/proprietary/Wifi/qsdk-ieee1905-security $ROOT/"
cp -rf $ROOT/NHSS.QSDK.11.1/apss_proc/out/proprietary/Wifi/qsdk-whc $ROOT/
echo "cp -rf $ROOT/NHSS.QSDK.11.1/apss_proc/out/proprietary/Wifi/qsdk-whc $ROOT/"
cp -rf $ROOT/NHSS.QSDK.11.1/apss_proc/out/proprietary/Wifi/qsdk-whcpy $ROOT/
echo "cp -rf $ROOT/NHSS.QSDK.11.1/apss_proc/out/proprietary/Wifi/qsdk-whcpy $ROOT/"

cp -rf $ROOT/NHSS.QSDK.11.1/apss_proc/out/proprietary/QSDK-Base/qca-lib $ROOT/
echo "cp -rf $ROOT/NHSS.QSDK.11.1/apss_proc/out/proprietary/QSDK-Base/qca-lib $ROOT/"
cp -rf $ROOT/NHSS.QSDK.11.1/apss_proc/out/proprietary/QSDK-Base/qca-mcs-apps $ROOT/   
echo "cp -rf $ROOT/NHSS.QSDK.11.1/apss_proc/out/proprietary/QSDK-Base/qca-mcs-apps $ROOT/"
cp -rf $ROOT/NHSS.QSDK.11.1/apss_proc/out/proprietary/QSDK-Base/qsdk-qca-nss $ROOT/             
echo "cp -rf $ROOT/NHSS.QSDK.11.1/apss_proc/out/proprietary/QSDK-Base/qsdk-qca-nss $ROOT/"          
cp -rf $ROOT/NHSS.QSDK.11.1/apss_proc/out/proprietary/QSDK-Base/qca-nss-userspace $ROOT/       
echo "cp -rf $ROOT/NHSS.QSDK.11.1/apss_proc/out/proprietary/QSDK-Base/qca-nss-userspace $ROOT/"
cp -rf $ROOT/NHSS.QSDK.11.1/apss_proc/out/proprietary/QSDK-Base/athtestcmd $ROOT/                  
echo "cp -rf $ROOT/NHSS.QSDK.11.1/apss_proc/out/proprietary/QSDK-Base/athtestcmd $ROOT/"

cp -rf $ROOT/NHSS.QSDK.11.1/apss_proc/out/proprietary/BLUETOPIA/qca-bluetopia $ROOT/
echo "cp -rf $ROOT/NHSS.QSDK.11.1/apss_proc/out/proprietary/BLUETOPIA/qca-bluetopia $ROOT/"

cp -rf $ROOT/WLAN.HK.2.2/wlan_proc/src/components/qca-wifi-fw-src-component-cmn-WLAN.HK.*.tgz $ROOT/ 
echo "cp -rf $ROOT/WLAN.HK.2.2/wlan_proc/src/components/qca-wifi-fw-src-component-cmn-WLAN.HK.*.tgz $ROOT/" 
cp -rf $ROOT/WLAN.HK.2.2/wlan_proc/pkg/wlan_proc/bin/QCA6018/qca-wifi-fw-QCA6018_v1.0-WLAN.HK.*.tar.bz2 $ROOT/
echo "cp -rf $ROOT/WLAN.HK.2.2/wlan_proc/pkg/wlan_proc/bin/QCA6018/qca-wifi-fw-QCA6018_v1.0-WLAN.HK.*.tar.bz2 $ROOT/"

cp -rf $ROOT/WLAN.BL.3.12/cnss_proc/src/components/qca-wifi-fw-src-component-cmn-WLAN.BL.*.tgz $ROOT/
echo "cp -rf $ROOT/WLAN.BL.3.12/cnss_proc/src/components/qca-wifi-fw-src-component-cmn-WLAN.BL.*.tgz $ROOT/"
cp -rf $ROOT/WLAN.BL.3.12/cnss_proc/src/components/qca-wifi-fw-src-component-halphy_tools-WLAN.BL.*.tgz $ROOT/
echo "cp -rf $ROOT/WLAN.BL.3.12/cnss_proc/src/components/qca-wifi-fw-src-component-halphy_tools-WLAN.BL.*.tgz $ROOT/"
cp -rf $ROOT/WLAN.BL.3.12/cnss_proc/bin/QCA9888/hw.2/qca-wifi-fw-QCA9888_hw_2-WLAN.BL.3.12-00028-S-1.tar.bz2 $ROOT/
echo "cp -rf $ROOT/WLAN.BL.3.12/cnss_proc/bin/QCA9888/hw.2/qca-wifi-fw-QCA9888_hw_2-WLAN.BL.3.12-00028-S-1.tar.bz2 $ROOT/"
cp -rf $ROOT/WLAN.BL.3.12/cnss_proc/bin/AR900B/hw.2/qca-wifi-fw-AR900B_hw_2-WLAN.BL.3.12-00028-S-1.tar.bz2 $ROOT/
echo "cp -rf $ROOT/WLAN.BL.3.12/cnss_proc/bin/AR900B/hw.2/qca-wifi-fw-AR900B_hw_2-WLAN.BL.3.12-00028-S-1.tar.bz2 $ROOT/"
cp -rf $ROOT/WLAN.BL.3.12/cnss_proc/bin/QCA9984/hw.1/qca-wifi-fw-QCA9984_hw_1-WLAN.BL.3.12-00028-S-1.tar.bz2 $ROOT/
echo "cp -rf $ROOT/WLAN.BL.3.12/cnss_proc/bin/QCA9984/hw.1/qca-wifi-fw-QCA9984_hw_1-WLAN.BL.3.12-00028-S-1.tar.bz2 $ROOT/"
cp -rf $ROOT/WLAN.BL.3.12/cnss_proc/bin/IPQ4019/hw.1/qca-wifi-fw-IPQ4019_hw_1-WLAN.BL.3.12-00028-S-1.tar.bz2 $ROOT/
echo "cp -rf $ROOT/WLAN.BL.3.12/cnss_proc/bin/IPQ4019/hw.1/qca-wifi-fw-IPQ4019_hw_1-WLAN.BL.3.12-00028-S-1.tar.bz2 $ROOT/"

cp -rf $ROOT/CNSS.PS.3.12/qca-wifi-fw-AR9887_hw_1-CNSS.PS.*.tar.bz2 $ROOT/
echo "cp -rf $ROOT/CNSS.PS.3.12/qca-wifi-fw-AR9887_hw_1-CNSS.PS.*.tar.bz2 $ROOT/"
cp -rf $ROOT/CNSS.PS.3.12/qca-wifi-fw-AR9888_hw_2-CNSS.PS.*.tar.bz2 $ROOT/
echo "cp -rf $ROOT/CNSS.PS.3.12/qca-wifi-fw-AR9888_hw_2-CNSS.PS.*.tar.bz2 $ROOT/"
