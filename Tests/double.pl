#!/usr/bin/perl
printf "single width double height top\033#1 - after\n";
printf "single width double height bottom\033#2 - after\n";
printf "single width line\033#5 - after\n";
printf "double width line\033#6 - after\n";

printf "double height line-top\033#3 - after\n";
printf "double height line-bottm\033#4 - after\n";

printf "double height line-top\033#3 - disable\033#5\n";


printf "double width line\033#6 - after";