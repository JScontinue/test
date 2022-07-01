#!/bin/bash

#从命令行读取参数；
type=$1
ch="s"

#寻找型号的路径；
for oldPath in `find . -type d | grep "${type}[A-Za-z0-9_-]*$"`
do
	echo "寻找到的路径为：$oldPath"
	newPath=`echo ${oldPath} | sed s/${type}/${type}${ch}/`
#	tr替换要左右的字符数相同，在此是需要增加字符，所有不适用。
#	newPath=`echo ${oldPath} | tr "${type}" "${type}${ch}"`
	echo "生成新的文件路径为：$newPath"
	#复制文件；
	cp -r $oldPath $newPath
	#进入新生成的文件夹；
	cd $newPath
	#把新文件夹的文件重新命名；
	rename s/${type}/${type}${ch}/ *
	#更改文件内的型号，小写，大写；
	for file in `ls ./`
	do
		sed -i s/${type}/${type}${ch}/g $file
		sed -i s/`echo ${type} | tr a-z A-Z`/`echo ${type}${ch} | tr a-z A-Z`/g $file
	done
done
