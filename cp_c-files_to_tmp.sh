#!/bin/bash
# 拷贝当前目录下所有c文件到/tmp/ccc，然后根据需要拷贝到src文件夹下。方便添加工程。
# copy all the c source file in the current dir to /tmp/ccc
INDENT="        "

if [ "$#" != "1"  ]; then
	echo "$INDENT copy all the c source file in the current dir to /tmp/ccc"
	echo "$INDENT except stm32f10xxxxx.*"
	echo "$INDENT USAGE:"
	echo "$INDENT $INDENT $0 ."
	exit 1
fi

echo 'use iconv to convert GBK to UTF8? (y/n):'
read is_conv

if [ ! -d "/tmp/ccc" ];then
	mkdir /tmp/ccc
else
	rm -rf /tmp/ccc/*
fi

dir="$@"
find $@ |
tail -n +2 |
while read line
do
	filename=${line##*/}
	#if [ ${filename%f10x*.*} = "stm32" ];then
	#	continue
	#fi

	if [ -f $line ];then
		if [ $is_conv = "n" ];then
			echo "bash: cp $line /tmp/ccc/"
			cp $line /tmp/ccc/
		elif [ $is_conv = "n" ];then
			echo "bash: iconv -f gbk -t utf-8 $line > /tmp/ccc/$filename"
			iconv -f gbk -t utf-8 $line > /tmp/ccc/$filename
		else
			echo "please answer y or n, Exit."
			exit 1
		fi
		if [ $? != "0" ];then
			echo ERROR
			exit 1
		fi
		if [ ${filename##*.} = "c" ];then
			echo "OBJS+=${filename%%.c}.o" >> /tmp/ccc/000000000_MAKE_OBJS.txt
		fi
	fi
done
