#!/bin/bash
#
# Download test disk images
#


download_fred_fish_disks()
{
    # floppy disk image with more than 1 level of subdirectories
    wget "https://ftp.grandis.nu/turran/FTP/TOSEC/Collections/Commodore%20Amiga%20-%20Collections%20-%20Fred%20Fish/Amiga%20Library%20Disk%20%230049%20(1987)(Fred%20Fish)(PD)%5bWB%5d.zip"

    #http -d -o fishdisk0049.zip \
    #  "https://ftp.grandis.nu/turran/FTP/TOSEC/Collections/Commodore%20Amiga%20-%20Collections%20-%20Fred%20Fish/Amiga%20Library%20Disk%20%230049%20(1987)(Fred%20Fish)(PD)%5bWB%5d.zip"

    FFDISK49="Amiga Library Disk #0049 (1987)(Fred Fish)(PD)[WB]"
    unzip "${FFDISK49}.zip"
    mv -v "${FFDISK49}.adf" ffdisk0049.adf
    rm -fv "${FFDISK49}.zip"
}


download_adflib_test_images()
{
    ADFLIB_DUMPS_URL=https://github.com/adflib/ADFlib/raw/master/tests/data/Dumps

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
}



CUR_DIR=`pwd`

[ ! -d testdata ] && { mkdir -v testdata ; }
[ ! -d testdata ] && { echo "Cannot create testdata/ - aborting..." ; exit 1 ; }

cd testdata
download_fred_fish_disks
download_adflib_test_images
cd "${CUR_DIR}"
