

configure_file (config.h.cmake.in config.h)

include_directories(${PROJECT_BINARY_DIR}/src)

add_executable ( fuseadf
  adffs.c
  adffs.h
  adffs_fuse_api.h
  adffs_log.c
  adffs_log.h
  adfimage.c
  adfimage.h
  fuseadf.c
  log.c
  log.h )

target_link_libraries ( fuseadf PUBLIC
  ${ADFLIB_LDFLAGS}
  ${FUSE_LDFLAGS}
)
