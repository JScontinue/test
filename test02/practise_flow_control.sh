#!/bin/bash

a=10
b=20
if [ $a == $b ]
then
	echo "a 等于 b"
elif [ $a -gt $b ]
then
	echo "a 大于 b"
elif [ $a -lt $b ]
then
	echo "a 小于 b"
else
	echo "没有符合的条件"
fi



num1=$[2*3]
num2=$[1+5]
if test $[num1] -eq $[num2]
then
	echo '两个数字相等'
else
	echo '两个数字不相等！'
fi



for loop in 1 3 5 7 0
do
	echo "The value is: $loop"
done



for str in This is a string
do
	echo $str
done



int=1
while(( $int<=5 ))
do
	echo $int
	let "int++"
done


<<EOF
echo '按下<CTRL-D> 退出'
echo -n '输入你最喜欢的网站名：'
while read FILM
do
	echo "是的！$FILM 是一个好网站"
done
EOF


a=0
until [ ! $a -lt 10 ]
do
	echo $a
	a=`expr $a + 1`
done


<<EOF
echo '输入1到4之间的数字'
echo '你输入的数值为：'
read aNum
case $aNum in
	1) echo '你选择了1'
	;;
	2) echo '你选择了2'
	;;
	3) echo '你选择了3'
	;;
	4) echo '你选择了4'
	;;
	*) echo '你没有输入1到4之间的数字'
	;;
esac
EOF


site="runoob"

case "$site" in
	"runoob") echo "菜鸟教程"
	;;
	"google") echo "Google搜索"
	;;
	"taobao") echo "淘宝网"
	;;
esac


while :
do
	echo -n "输入 1 到 5 之间的数字："
	read aNum
	case $aNum in
		1|2|3|4|5) echo "你输入的数字为 $aNum!"
		;;
		*) echo "你输入的数字不是 1 到 5 之间的！ 游戏结束"
			break
		;;
	esac
done

while :
do
	echo -n "输入 1 到 5 之间的数字："
	read aNum
	case $aNum in
		1|2|3|4|5) echo "你输入的数字为 $aNum!"
		;;
		*) echo "你输入的数字不是 1 到 5 之间的！"
			continue
			echo "游戏结束"
			;;
	esac
done
