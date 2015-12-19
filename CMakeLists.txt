project                     (sample)
cmake_minimum_required      (VERSION 2.8)
set                         (CMAKE_INCLUDE_SYSTEM_FLAG_CXX "-isystem")

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Release' as none was specified.")
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
    "MinSizeRel" "RelWithDebInfo")
endif()

find_path               (NANOGUI_INCLUDE_DIR    nanogui/nanogui.h)
find_library            (NANOGUI_LIBRARY        nanogui)
find_path               (EIGEN_INCLUDE_DIR      eigen3/Eigen/Core)
find_path               (NANOVG_INCLUDE_DIR     nanovg.h)
find_path               (PYBIND11_INCLUDE_DIR   pybin11/pybind11.h)
include_directories     (${NANOGUI_INCLUDE_DIR}
                         SYSTEM ${EIGEN_INCLUDE_DIR}/eigen3
                         ${NANOVG_INCLUDE_DIR}
                         ${PYBIND11_INCLUDE_DIR})

if                      (APPLE)
  set                   (CMAKE_CXX_FLAGS "-std=c++11 -stdlib=libc++")
  # Find standard libraries needed for OpenGL/GLFW
  find_library          (cocoa_library Cocoa)
  find_library          (opengl_library OpenGL)
  find_library          (corevideo_library CoreVideo)
  find_library          (iokit_library IOKit)
  set                   (extra_libs ${cocoa_library} ${opengl_library} ${corevideo_library} ${iokit_library})
  set                   (extra_source src/darwin.mm)

  find_path             (GLFW3_INCLUDE_DIR      GLFW/glfw3.h)
  find_library          (GLFW3_LIBRARY          glfw3)
  include_directories   (${GLFW3_INCLUDE_DIR})
elseif                  ("${CMAKE_SYSTEM}" MATCHES "Linux")
  set                   (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
  set                   (extra_libs GL Xxf86vm Xrandr Xinerama Xcursor Xi X11 pthread dl)

  find_path             (GLFW3_INCLUDE_DIR      GLFW/glfw3.h)
  find_library          (GLFW3_LIBRARY          glfw)
  include_directories   (${GLFW3_INCLUDE_DIR})
endif                   ()

# Threads
find_package            (Threads)

# Add a CMake parameter for choosing a desired Python version
set                         (SAMPLE_PYTHON_VERSION "" CACHE STRING "Python version to use for compiling sample module")

find_package                (PythonLibs   ${HENSON_PYTHON_VERSION} REQUIRED)
find_package                (PythonInterp ${HENSON_PYTHON_VERSION} REQUIRED)

include_directories         (${PYTHON_INCLUDE_DIR} include)

add_library                 (sample_lib SHARED sample.cpp)
target_link_libraries       (sample_lib ${NANOGUI_LIBRARY} ${GLFW3_LIBRARY}
                                        ${OGDF_LIBRARY} ${COIN_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})
set_target_properties       (sample_lib PROPERTIES OUTPUT_NAME sample)


foreach                     (m sample_lib)
set_target_properties       (${m} PROPERTIES PREFIX "")
set_target_properties       (${m} PROPERTIES SUFFIX ".so")

# Optimize for a small binary size
if                          (NOT ${U_CMAKE_BUILD_TYPE} MATCHES DEBUG)
  set_target_properties     (${m} PROPERTIES COMPILE_FLAGS "-Os")
endif                       ()

# Strip unnecessary sections of the binary on Linux/Mac OS
if                          (APPLE)
  set_target_properties     (${m} PROPERTIES MACOSX_RPATH ".")
  set_target_properties     (${m} PROPERTIES LINK_FLAGS "-undefined dynamic_lookup -dead_strip")
  if                        (NOT ${U_CMAKE_BUILD_TYPE} MATCHES DEBUG)
      add_custom_command    (TARGET ${m} POST_BUILD COMMAND strip -u -r ${CMAKE_CURRENT_BINARY_DIR}/${m}.so)
  endif                     ()
else                        ()
  if                        (NOT ${U_CMAKE_BUILD_TYPE} MATCHES DEBUG)
    add_custom_command      (TARGET ${m} POST_BUILD COMMAND strip ${CMAKE_CURRENT_BINARY_DIR}/${m}.so)
  endif                     ()
endif                       ()
endforeach                  ()
