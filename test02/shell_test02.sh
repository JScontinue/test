#!/bin/bash

type=$1
proConfig=ProjectConfig.mk
kernelVer=LINUX_KERNEL_VERSION
path1="device"
path=`pwd`
fileSuffix="defconfig"
exclusive="debug"
lcmDefine="CONFIG_CUSTOM_KERNEL_LCM"

#进入./device目录
cd ${path1}
#搜索${type}型号的目录，进入目录
cd `find . -type d | grep "${type}[A-Za-z0-9_-]*$"`
touch temp
#在mk文件中过滤出kernel的版本所在行，输入重定向到temp文件
grep ${kernelVer} ${proConfig} > temp
#找到内核版本赋值给version
version=`cut -d " " -f 3 temp`
rm temp
cd ${path}

cd ${version}
touch temp
#寻找${type}*${fileSuffix}的文件，去掉包含debug的文件，把${lcmDefine}所在行输入到temp文件
find . -name "*${type}*${fileSuffix}" | grep -v "${exclusive}" | xargs grep "${lcmDefine}" > temp
#把lcm的型号信息过滤出来并赋值给list
list=`cut -d "\"" -f 2 temp`
#列举lcm的信息
for str in ${list}
do
	echo "LCM_NAME is: ${str}"
	echo "IC is: `echo ${str} | cut -d "_" -f 1` "
	echo "Module is: `echo ${str} | cut -d "_" -f 2`"
	echo "lane is: `echo ${str} | cut -d "_" -f 3`"
	echo "resolution is: `echo ${str} | cut -d "_" -f 4`"
done
rm temp
cd ${path}
