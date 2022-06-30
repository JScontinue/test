#!/bin/bash

<<EOF
demoFun(){
	echo "这是我的第一个shell函数!"
}

echo "--------------函数开始执行---------------"
demoFun
echo "--------------函数执行完毕---------------"




funWithReturn(){
	echo "这个函数会对输入的两个数字进行相加运算……"
	echo "输入第一个数字："
	read num1
	echo "输入第二个数字："
	read num2
	echo "两个数字分别为$num1 和 $num2 !"
	return $(($num1+$num2))
}
funWithReturn
echo "输入的两个数字之和为 $? !"
EOF


funWithParam(){
	echo "第一个参数为 $1 !"
	echo "第二个参数为 $2 !"
	echo "第十个参数为 $10 !"
	echo "第十个参数为 ${10} !"
	echo "第十一个参数为 ${11} !"
	echo "参数总数有 $# !"
	echo "作为一个字符串输出所有参数 $* !"
}
funWithParam 1 3 5 7 9 11 13 15 17 19 21 23 25
