#!/bin/bash

#保存刚开始的路径
path=`pwd`
#从命令行读取参数；
type=$1
ch="s"
#参数变为大写
type1=`echo ${type} | tr a-z A-Z`
ch1=`echo ${ch} | tr a-z A-Z`

#寻找型号的路径并重定向搭配temp文件
touch temp
find . -type d | grep "${type}[A-Za-z0-9_-]*$" > temp
cat temp

#把temp读取的每行赋给oldPath变量
cat temp | while read oldPath
do
	echo "寻找到的路径为：$oldPath"
	newPath=`echo ${oldPath} | sed s/${type}/${type}${ch}/`
#	tr替换要左右的字符数相同，在此是需要增加字符，所有不适用。
#	newPath=`echo ${oldPath} | tr "${type}" "${type}${ch}"`
	#复制文件；
	cp -r $oldPath $newPath
	echo "生成新的文件路径为：$newPath"
	#进入新生成的文件夹；
	cd $newPath
	#把新文件夹的文件重新命名；
	rename s/${type}/${type}${ch}/ *
	#把文件名保存到file变量中
	file=`ls ./`
	#更改文件内的型号，小写，大写
	for file1 in $file
	do
		sed -i s/${type}/${type}${ch}/g $file1
		sed -i s/${type1}/${type1}${ch1}/g $file1
	done
	#返回刚开始的路径
	cd $path
done


