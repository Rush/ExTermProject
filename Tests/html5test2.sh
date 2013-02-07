#!/bin/bash
printf "\033]105;HTML;test1;20;"
echo -n '<img src="data:image/jpeg;base64,'
cat "/home/rush/misa.jpg" | base64
echo -n '" style="height: 100%">'
printf "\007"
