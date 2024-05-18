
cmake_minimum_required ( VERSION 2.8.12 )

project ( fuseadf C )
set ( fuseadf_VERSION_MAJOR 0 )
set ( fuseadf_VERSION_MINOR 4 )

find_package ( PkgConfig REQUIRED )
pkg_check_modules ( FUSE REQUIRED fuse )
pkg_check_modules ( ADFLIB REQUIRED adflib>=0.9.0 )

add_compile_options (
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
  $<$<CONFIG:RELEASE>:-O2>
  ${FUSE_CFLAGS}
  ${ADFLIB_CFLAGS}
)

add_link_options (
#  $<$<CONFIG:DEBUG>:-lasan>
#  $<$<CONFIG:DEBUG>:-fsanitize=address>
)

option ( FUSEADF_ALLOW_USE_AS_ROOT "Allow using as root" OFF )

add_subdirectory ( src )

# testing
enable_testing()
add_subdirectory ( tests )
