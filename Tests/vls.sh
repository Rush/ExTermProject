#!/bin/bash
IFS="\n"
res=`ls -1 $*`
count=`echo $res|wc -l`
IFS=" "
echo "$count files"
printf "\033]105;HTML;test1;$count;"
echo $res | while read file;do
  iconmime=`kmimetypefinder "$file"|tr -t '/' '-'|head -n1`
  icon=`kiconfinder "$iconmime"|sed 's/48x48/16x16/'`
  echo '<div style="height: 17px"><img src="local://'$icon'" style="height: 100%">'"$file"'<br/></div>'
done
printf "\007\n"