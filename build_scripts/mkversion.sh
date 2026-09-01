#! /bin/sh

dst=$1
path=$2
#version="ps_drc_0.0.3.1001"

if [ "$path" = "" ]; then
	dstfile=`echo '.''/'"version.cpp"`
	
	dsttmpl=`echo '.''/'"version.tmpl"`
else
	dstfile=`echo "$path""version.cpp"`	
	dsttmpl=`echo "$path""version.tmpl"`	
fi

rm $dstfile

#A-主版本号:功能有较大变动时累加
a_main_ver=`grep main_ver $dsttmpl | awk -F "=" '{print $2}'`
#B-次版本号:功能有一定增加累加
b_sub_ver=`grep sub_ver $dsttmpl | awk -F "=" '{print $2}'`
#C-分支编号: 0-全系统主干分支 1-DRC主干分支 2-DRSU主干分支 其他-MR的ID号
c_branch_ver=`grep branch_ver $dsttmpl | awk -F "=" '{print $2}'`
#D-测试版本号: 0-正式版本 X-入库SVN号
d_rev_ver=`svn info | grep Revision: | awk '{print $2}'`
if [ "$d_rev_ver" = "" ]; then
	d_rev_ver="none"
	echo "BUILD ERROR FOR NO SVN VERSION"
	exit
fi

#E-希腊字母版本号定义：
#Alpha: 以实现软件功能为主。
#Beta: 已消除了严重的错误。
#RC: 该版本已经相当成熟。
#Release: 最终版本，可简写符号(R)。
#测试版本无此版本号定义
e_rel_ver=`grep rel_ver $dsttmpl | awk -F "=" '{print $2}'`
if [ "$e_rel_ver" = "" ]; then
	psversion=`echo "dr_$dst""_"$a_main_ver"."$b_sub_ver"."$c_branch_ver"."$d_rev_ver`
else
	psversion=`echo "dr_$dst""_"$a_main_ver"."$b_sub_ver"."$c_branch_ver"_"$e_rel_ver"-build-"$d_rev_ver`
fi

platform_ver=`grep pl_ver $dsttmpl | awk -F "=" '{print $2}'`

WCDATE=`date +"%Y-%m-%d %H:%M:%S"`
SYS_TIME=`date +%s`
HOSTNAME=`whoami`
WCMIXED="Not mixed"
WCRANGE=`svn info | grep Rev:`
USERNAME=`svn auth | grep Username: | awk '{print $2}'`
WCURL=`svn info | grep URL | grep svn | awk -F "_CODE" '{print $2}'`
svnstatus=`svn status | grep M | grep -v "version" | grep -v "build" | awk '{print $1}' | sort | uniq -c | grep M | awk '{print $2}'`


if [ "$WCRANGE" = "" ]; then
	WCRANGE="none"
fi

if [ "$WCURL" = "" ]; then
	WCURL="none"
fi

if [ "$svnstatus" = "M" ]; then
	WCMODS="local+"
elif [ "$svnstatus" = "" ]; then
	WCMODS="svn"
else
	WCMODS=$svnstatus
fi

echo "revision is $rev and dstfile is $dstfile"

echo "char acSvnRevision[]     = \""$rev'";' > $dstfile
echo "char acSvnModified[]     = \""$WCMODS'";' >> $dstfile
echo "char acSvnDate[]         = \""$WCDATE'";' >> $dstfile
echo "char acSvnRange[]        = \""$WCRANGE'";' >> $dstfile
echo 'char acSvnMixed[]        = "'$WCMIXED'";' >> $dstfile
echo 'char acSvnUrl[]          = "'$WCURL'";' >> $dstfile

echo "#ifdef GTEST_EN" >> $dstfile
echo 'char acPsVersion[]       = "dr_gtest_2.0.0";    /*digital rail version */' >> $dstfile
echo "#else" >> $dstfile
echo 'char acPsVersion[]       = "'$psversion'";    /*digital rail version */' >> $dstfile
echo "#endif" >> $dstfile
#echo 'char acPlatformVersion[] = "'$platform_ver'";           /*digital rail platform version */' >> $dstfile

echo 'char acSvnUserName[]     = "'$USERNAME'";' >> $dstfile
echo 'char acSvnHostName[]     = "'$HOSTNAME'";' >> $dstfile
echo 'char acSysTime[]         = "'$SYS_TIME'";' >> $dstfile

echo $psversion > dr_version.txt

