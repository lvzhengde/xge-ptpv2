#!/bin/bash  

trap 'kill $!; exit' INT #terminate script running when press CTRL+C 

#set the number of arguments
ARG_NUM=1

#check arguments
if [ $# -ne $ARG_NUM ]; then
  echo -e "\nargument number error: $#"
  echo "the number of arguments should be :$ARG_NUM"
  echo "usage: runcase.sh tc_name"
  echo -e "for example: runcase.sh tc_rtc\n"
  exit 1
fi

elfFile = ptpv2 
waveFile = ptpv2

iverilog -o $(elfFile).out -s $1 -f comp.f 
vvp -n $(elfFile).out -fst
#gtkwave $(waveFile).fst
