#!/bin/bash
#
# download ADFlib test images
#

ADFLIB_DUMPS_URL=https://github.com/lclevy/ADFlib/raw/master/regtests/Dumps

for i in \
    ${ADFLIB_DUMPS_URL}/blank.adf \
    ${ADFLIB_DUMPS_URL}/links.adf \
    ${ADFLIB_DUMPS_URL}/testffs.adf \
    ${ADFLIB_DUMPS_URL}/testhd.adf \
    ${ADFLIB_DUMPS_URL}/testofs.adf
do
    echo "Downloading: $i"
    wget $i
done
