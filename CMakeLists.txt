
cmake_minimum_required ( VERSION 2.6 )

project ( fuseadf C )
set ( fuseadf_VERSION_MAJOR 0 )
set ( fuseadf_VERSION_MINOR 3 )

find_package( PkgConfig REQUIRED )
#find_package ( fuse REQUIRED )
pkg_check_modules ( FUSE REQUIRED fuse )
pkg_check_modules ( ADFLIB REQUIRED adflib )

#set ( extraFlags "-Wall -Wextra -Weffc++ -pedantic -pedantic-errors -std=c++14" )

#add_compile_definitions(
#  $<$<CONFIG:DEBUG>:DEBUG>
#)

add_compile_options(
  -Wall
  -Wextra
  -Wconversion
  -Wsign-conversion
  -pedantic
  -pedantic-errors
  -Werror-implicit-function-declaration
  -Werror=incompatible-pointer-types
#  $<$<CONFIG:DEBUG>:-g3>
#  $<$<CONFIG:DEBUG>:-Og>
  $<$<CONFIG:DEBUG>:-ggdb>
#  $<$<CONFIG:DEBUG>:-fsanitize=address>
#  $<$<CONFIG:RELEASE>:-O3>
  ${FUSE_CFLAGS}
  ${ADFLIB_CFLAGS}
  )

add_link_options(
#  $<$<CONFIG:DEBUG>:-lasan>
#  $<$<CONFIG:DEBUG>:-fsanitize=address>
)

ADD_SUBDIRECTORY ( src )

# testing
enable_testing()
ADD_SUBDIRECTORY ( tests )
