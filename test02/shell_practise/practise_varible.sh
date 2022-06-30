#!/bin/bash

<<EOF
#your_name="qinjx"
#echo $your_name
#echo ${your_name}

#for skill in Ada Coffe Action Java;do
#	echo "I am good at ${skill}Script"
#done

#your_name="ton"
#echo $your_name
#your_name="alibaba"
#echo $your_name

myUrl="https://www.google.com"
readonly myUrl
myUrl="https://www.runoob,com"
EOF

myUrl="https://www.runoob.com"
unset myUrl
echo $myUrl

