#! /bin/sh
#
# Autotools project bootstrap script
#

${AUTOTOOLS_PATH}aclocal \
&& ${AUTOTOOLS_PATH}autoheader \
&& ${AUTOTOOLS_PATH}automake -Wall --add-missing \
&& ${AUTOTOOLS_PATH}autoconf

# verbose version
#${AUTOTOOLS_PATH}aclocal --verbose \
#${AUTOTOOLS_PATH}autoheader -v \
#&& ${AUTOTOOLS_PATH}automake -Wall -v --add-missing \
#&& ${AUTOTOOLS_PATH}autoconf -v
# --force -i
