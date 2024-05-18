
if ( FUSEADF_ALLOW_USE_AS_ROOT )
    message ( STATUS "Allowing the use of fuseadf as root." )
    add_compile_definitions ( ALLOW_USE_AS_ROOT )
else()
    message ( STATUS "Disallowing the use of fuseadf as root." )
endif ( FUSEADF_ALLOW_USE_AS_ROOT )


configure_file (config.h.cmake.in config.h)

include_directories(${PROJECT_BINARY_DIR}/src)

add_executable ( fuseadf
  adffs.c
  adffs.h
  adffs_fuse_api.h
  adffs_log.c
  adffs_log.h
  adffs_util.c
  adffs_util.h
  adfimage.c
  adfimage.h
  fuseadf.c
  log.c
  log.h )

target_link_libraries ( fuseadf PUBLIC
  ${ADFLIB_LDFLAGS}
  ${FUSE_LDFLAGS}
)

#install(TARGETS fuseadf DESTINATION /usr/local/bin)
install(TARGETS fuseadf)
