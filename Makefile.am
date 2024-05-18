#SUBDIRS = src . tests doc
SUBDIRS = src . tests

EXTRA_DIST = autogen.sh

# the thing below  should be in doc/Makefile.am, but for some reason
# adding 'doc'to SUBDIRS results in the error on 'make distcheck':
#    "No rule to make target 'distdir'.  Stop."
#
# To investigate/fix.

dist_doc_DATA = \
	README \
	AUTHORS \
	COPYING \
	ChangeLog

nobase_dist_doc_DATA = \
	doc/notes.txt

dist_man_MANS = \
	doc/fuseadf.1
