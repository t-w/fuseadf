
#find_package(Check REQUIRED)
pkg_check_modules ( CHECK REQUIRED check )

#include_directories(${CHECK_INCLUDE_DIRS})
#link_directories(${CHECK_LIBRARY_DIRS})

#add_compile_options(-pthread)

#configure_file (config.h.cmake.in config.h)

#include_directories(${PROJECT_BINARY_DIR}/src)
include_directories(${CHECK_INCLUDE_DIR})


add_executable ( test_adfimage
  test_adfimage.c
  ../src/adfimage.c
  ../src/adfimage.h
  ../src/log.c
  ../src/log.h
)

add_executable ( test_time_to_time_t
  test_time_to_time_t.c
  ../src/adffs_util.c
  ../src/adffs_util.h
)


add_test ( test_adfimage test_adfimage )
add_test ( test_time_to_time_t test_time_to_time_t )


target_link_libraries ( test_adfimage PUBLIC
  ${ADFLIB_LDFLAGS}
  ${CHECK_LIBRARIES}
  # -lcheck_pic
  -pthread
  #-lrt
  #-lm
  #-lsubunit
)

target_link_libraries ( test_time_to_time_t PUBLIC
  #${ADFLIB_LDFLAGS}
  ${CHECK_LIBRARIES}
  # -lcheck_pic
  -pthread
  #-lrt
  #-lm
  #-lsubunit
)
