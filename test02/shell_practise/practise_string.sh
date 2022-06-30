#!/bin/bash

<<EOF
your_name="runoob"
str="Holle, I know you are \"$your_name\"! \n"
echo -e $str



your_name="runoob"

greeting="hello, "$your_name" !"
greeting_1="hello, ${your_name} !"
echo $greeting $greeting_1

greeting_2='hello, '$your_name' !'
greeting_3='hello, ${your_name} !'
echo $greeting_2 $greeting_3



string="abcd"
echo ${#string}


string="abcd"
echo ${#string[0]}


string="runoob is a great site"
echo ${string:1:4}


string="runoob if a great site"
echo `expr index "$string" io`


EOF
array_name=(value0 happy bad sit down)
array_name[n]=valuen

valuen=${araay_name[n]}

#echo ${array_name[@]}

#length=${#array_name[@]}
#length=${#array_name[*]}
lengthn=${#array_name[n]}
echo $lengthn

