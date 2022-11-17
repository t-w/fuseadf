
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
)

add_test ( test_adfimage test_adfimage )

target_link_libraries ( test_adfimage PUBLIC
  ${ADFLIB_LDFLAGS}
  ${CHECK_LIBRARIES}
  # -lcheck_pic
  -pthread
  #-lrt
  #-lm
  #-lsubunit
)
