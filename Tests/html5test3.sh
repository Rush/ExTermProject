#!/bin/bash
printf "\033]105;HTML;test1;20;"
echo '<img src="local:///home/rush/misa.jpg" style="height: 100%">'
printf "\007"
read