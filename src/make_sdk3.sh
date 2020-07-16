#!/bin/sh

ROOT=`pwd`
echo "Working dir is '${ROOT}'"

echo "continue ... (y/n) ?" ;read c ;if [ "$c" != "y" ];then exit 0 ;fi

mkdir -p common/build/ipq
echo "mkdir -p common/build/ipq"
mkdir -p apss_proc/out/meta-scripts
echo "mkdir -p apss_proc/out/meta-scripts"
cp qsdk/qca/src/u-boot-2016/tools/pack.py apss_proc/out/meta-scripts/pack_hk.py
echo "cp qsdk/qca/src/u-boot-2016/tools/pack.py apss_proc/out/meta-scripts/pack_hk.py"
sed -i 's#</linux_root_path>#/</linux_root_path>#' contents.xml
echo "sed -i 's#</linux_root_path>#/</linux_root_path>#' contents.xml"
sed -i 's#</windows_root_path>#\\</windows_root_path>#' contents.xml
echo "sed -i 's#</windows_root_path>#\\</windows_root_path>#' contents.xml"
cp qsdk/bin/ipq/openwrt* common/build/ipq
echo "cp qsdk/bin/ipq/openwrt* common/build/ipq"
cp -r apss_proc/out/proprietary/QSDK-Base/meta-tools apss_proc/out/
echo "cp -r apss_proc/out/proprietary/QSDK-Base/meta-tools apss_proc/out/"
cp -rf qsdk/bin/ipq/dtbs/* common/build/ipq/
echo "cp -rf qsdk/bin/ipq/dtbs/* common/build/ipq/"
cp -rf skales/* common/build/ipq/
echo "cp -rf skales/* common/build/ipq/"
cp -rf wlan_proc/build/ms/bin/FW_IMAGES/* common/build/ipq/
echo "cp -rf wlan_proc/build/ms/bin/FW_IMAGES/* common/build/ipq/"
cp qsdk/staging_dir/host/bin/ubinize common/build/ipq/
echo "cp qsdk/staging_dir/host/bin/ubinize common/build/ipq/"
cd common/build
echo "cd common/build"
sed "s/'''$/\n'''/g" -i update_common_info.py
echo "sed \"s/'''$/\n'''/g\" -i update_common_info.py"
sed -i "s/os.chdir(ipq_dir)//" update_common_info.py
echo "sed -i \"s/os.chdir(ipq_dir)//\" update_common_info.py"
sed '/debug/d;/packages/d;/"ipq6018_64"/d;/t32/d;/ret_prep_64image/d;/Required/d;/skales/d;/nosmmu/d;/os.system(cmd)/d;/os.chdir(ipq_dir)/d;/atfdir/d;/noac/d;/single-atf/d;/bl31.mbn/d' -i update_common_info.py
echo "sed '/debug/d;/packages/d;/\"ipq6018_64\"/d;/t32/d;/ret_prep_64image/d;/Required/d;/skales/d;/nosmmu/d;/os.system(cmd)/d;/os.chdir(ipq_dir)/d;/atfdir/d;/noac/d;/single-atf/d;/bl31.mbn/d' -i update_common_info.py"
sed -i -e '/Generating 32 bit atf images/,+17d' update_common_info.py
echo "sed -i -e '/Generating 32 bit atf images/,+17d' update_common_info.py"