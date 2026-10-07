cmake_minimum_required(VERSION 3.23)

project(ktx_oiio_playground LANGUAGES CXX)

find_package(OpenImageIO REQUIRED
# In case you have installed OIIO in a custom location:
  HINTS /home/walcht/OpenImageIO/dist/lib/cmake/OpenImageIO
)
find_package(Ktx 5.0.0 REQUIRED)

# Global compiler options/configuration
if (SANITIZE AND CMAKE_BUILD_TYPE STREQUAL "Debug")
  if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-fsanifize="${SANITIZE}")
    add_link_options(-fsanifize="${SANITIZE}")
    add_compile_options (-fno-omit-frame-pointer -fPIC)
  elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if (NOT MINGW)
      # finds writes past size of std::vector (won't be detected without this)
      add_compile_definitions (_GLIBCXX_SANITIZE_VECTOR=1)
      add_compile_options(-fsanifize="${SANITIZE}")
      add_link_options(-fsanifize="${SANITIZE}")
      add_compile_options (-fno-omit-frame-pointer -fPIC)
    else()
      message(WARNING "Address sanitation is not supported on MinGW/GNU consider switching to MINGW/Clang")
    endif()
  endif()
elseif (SANITIZE AND MSVC)
  message(WARNING "Address sanitation on Windows MSVC is not supported") 
endif()

file(GLOB src_files "src/*.cpp")
foreach(src_file ${src_files})
  set_property(TARGET ${CURR_TARGET} PROPERTY CXX_STANDARD 20)
  get_filename_component(CURR_TARGET ${src_file} NAME_WE)
  add_executable(${CURR_TARGET} ${src_file})

  target_compile_options(${CURR_TARGET} PRIVATE
    $<IF:$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:CLANG>>,-Wall -Wextra -Wpedantic,>)
  target_link_libraries(${CURR_TARGET} PUBLIC
    OpenImageIO::OpenImageIO
    OpenImageIO::OpenImageIO_Util
    KTX::ktx
  )
endforeach()

# Copy ktx2 samples to build dir
message(STATUS "copying resources/** io ${CMAKE_BINARY_DIR}/resources ...")
add_custom_command(TARGET ${CURR_TARGET} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          ${CMAKE_SOURCE_DIR}/resources
          ${CMAKE_BINARY_DIR}/resources)

# Create output dir where output ktx2/pngs will be written
add_custom_command(TARGET ${CURR_TARGET} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/output)
