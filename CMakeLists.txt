# If we are not building as a part of LLVM, build LLDB as an
# standalone project, using LLVM as an external library:
if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  project(lldb)
  cmake_minimum_required(VERSION 2.8)

  set(LLDB_PATH_TO_LLVM_SOURCE "" CACHE PATH
    "Path to LLVM source code. Not necessary if using an installed LLVM.")
  set(LLDB_PATH_TO_LLVM_BUILD "" CACHE PATH
    "Path to the directory where LLVM was built or installed.")

  if (LLDB_PATH_TO_LLVM_SOURCE)
    if (NOT EXISTS "${LLDB_PATH_TO_LLVM_SOURCE}/cmake/config-ix.cmake")
      message(FATAL_ERROR "Please set LLDB_PATH_TO_LLVM_SOURCE to the root "
              "directory of LLVM source code.")
    else()
      get_filename_component(LLVM_MAIN_SRC_DIR ${LLDB_PATH_TO_LLVM_SOURCE}
                             ABSOLUTE)
      list(APPEND CMAKE_MODULE_PATH "${LLVM_MAIN_SRC_DIR}/cmake/modules")
    endif()
  endif()

  list(APPEND CMAKE_MODULE_PATH "${LLDB_PATH_TO_LLVM_BUILD}/share/llvm/cmake")

  get_filename_component(PATH_TO_LLVM_BUILD ${LLDB_PATH_TO_LLVM_BUILD}
                         ABSOLUTE)

  include(AddLLVM)
  include("${LLDB_PATH_TO_LLVM_BUILD}/share/llvm/cmake/LLVMConfig.cmake")
  include(HandleLLVMOptions)

  set(PACKAGE_VERSION "${LLVM_PACKAGE_VERSION}")

  set(LLVM_MAIN_INCLUDE_DIR "${LLVM_MAIN_SRC_DIR}/include")
  set(LLVM_BINARY_DIR ${CMAKE_BINARY_DIR})

  set(CMAKE_INCLUDE_CURRENT_DIR ON)
  include_directories("${PATH_TO_LLVM_BUILD}/include"
                      "${LLVM_MAIN_INCLUDE_DIR}")
  link_directories("${PATH_TO_LLVM_BUILD}/lib")

  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

  set(LLDB_BUILT_STANDALONE 1)
endif()

set(LLDB_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(LLDB_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})

if (CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
  message(FATAL_ERROR "In-source builds are not allowed. CMake would overwrite "
"the makefiles distributed with LLVM. Please create a directory and run cmake "
"from there, passing the path to this source directory as the last argument. "
"This process created the file `CMakeCache.txt' and the directory "
"`CMakeFiles'. Please delete them.")
endif()

macro(add_lldb_library name)
  llvm_process_sources(srcs ${ARGN})
  if (MSVC_IDE OR XCODE)
    string(REGEX MATCHALL "/[^/]+" split_path ${CMAKE_CURRENT_SOURCE_DIR})
    list(GET split_path -1 dir)
    file(GLOB_RECURSE headers
      ../../include/lldb${dir}/*.h)
    set(srcs ${srcs} ${headers})
  endif()
  if (MODULE)
    set(libkind MODULE)
  elseif (SHARED_LIBRARY)
    set(libkind SHARED)
  else()
    set(libkind)
  endif()
  add_library(${name} ${libkind} ${srcs})
  if (LLVM_COMMON_DEPENDS)
    add_dependencies(${name} ${LLVM_COMMON_DEPENDS})
  endif()

  target_link_libraries(${name} ${LLVM_USED_LIBS})
  llvm_config(${name} ${LLVM_LINK_COMPONENTS})
  target_link_libraries(${name} ${LLVM_COMMON_LIBS})
  link_system_libs(${name})

  install(TARGETS ${name}
    LIBRARY DESTINATION lib${LLVM_LIBDIR_SUFFIX}
    ARCHIVE DESTINATION lib${LLVM_LIBDIR_SUFFIX})
  set_target_properties(${name} PROPERTIES FOLDER "lldb libraries")
endmacro(add_lldb_library)

macro(add_lldb_executable name)
  add_llvm_executable(${name} ${ARGN})
  set_target_properties(${name} PROPERTIES FOLDER "lldb executables")
endmacro(add_lldb_executable)

include_directories(BEFORE
  ${CMAKE_CURRENT_BINARY_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  )

install(DIRECTORY include/
  DESTINATION include
  FILES_MATCHING
  PATTERN "*.h"
  PATTERN ".svn" EXCLUDE
  )

#add_subdirectory(include)
#add_subdirectory(scripts)
add_subdirectory(source)
#add_subdirectory(test)