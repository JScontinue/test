#!/bin/bash

<<EOF
echo "菜鸟教程：www.runoob.com" > users
cat users

echo "菜鸟教程：www.runoob.com" >> users
cat users
EOF


wc -l << EOF
	welcome to
	runoob
	www.runoob.com
EOF


cat << EOF
welcomm to
runoob
www.runoob.com
EOF
