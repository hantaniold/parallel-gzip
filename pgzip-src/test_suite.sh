#!/bin/bash
CUR_DIR=`pwd`
TEST_DIR=${CUR_DIR}/test_suite
URL=https://github.com/SeanHogan/parallel-gzip/tarball/master
TAR=master
SUITE=SeanHogan-parallel-gzip-4f98a28/pgzip-src

#mkdir  $TEST_DIR
#cd $TEST_DIR

#wget -q $URL
#tar -xzf $TAR
#cd $SUITE

# Prepare different zip programs
cd gzip
chmod 700 init_script.sh
./init_script.sh
cd ../pigz
chmod 700 init_script.sh
./init_script.sh
cd ../pgzip
make
cd ..
echo "Output written to tso.txt"
# Make random files of a size, and compress/decompress it with all zip formats
function testZips(){
    python ${CUR_DIR}/makeFile.py ${CUR_DIR}/1MB.txt ${1}MB.txt $1
	echo "pigz ${1}"
    echo "Times for compressing ${1} MB with pigz" >> tso.txt
	(/usr/bin/time  pigz/pigz -b 32 -f -k ${1}MB.txt)  2>> tso.txt
	echo "pgzip ${1}"
    echo "Times for compressing ${1} MB with pgzip" >> tso.txt
	(/usr/bin/time  pgzip/pgzip ${1}MB.txt -b 20000 -f) 2>> tso.txt 
	echo "quickZip ${1}"
    echo "Times for compressing ${1} MB with quickZip" >> tso.txt
    let "size=${1}/8"
	(/usr/bin/time  python ${CUR_DIR}/doQuickZip.py ${1}MB.txt $size) 2>> tso.txt
    python ${CUR_DIR}/doQuickZip.py -d ${1}MB.txt.gz
	echo "gzip ${1}"
	echo "Times for compressing ${1} MB with old gzip" >> tso.txt
	(/usr/bin/time gzip/gzip -f ${1}MB.txt) 2>> tso.txt
	rm *.gz
	

}

testZips 20
testZips 100
testZips 250
testZips 500
testZips 750
testZips 1000
testZips 2000

rm -rf $TEST_DIR
