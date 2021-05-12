CMAKE_MINIMUM_REQUIRED (VERSION 2.8)
project(htt_util)

set(SRC src/htt_util.cpp)

if(MSVC)
	set(SRC ${SRC} hidapi/windows/hid.c)
	foreach(flag_var
        CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
   if(${flag_var} MATCHES "/MD")
      string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
   endif(${flag_var} MATCHES "/MD")
	endforeach(flag_var)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
else()
	set(SRC ${SRC} hidapi/linux/hid.c)
endif()

include_directories(hidapi/include)

add_executable(htt_util ${SRC})

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/htt_util_factory.cpp AND NOT HTT_PUBLIC_BUILD)
	target_compile_definitions(htt_util PRIVATE -DHTT_UTIL_WITH_FACTORY_COMMANDS)
	target_sources(htt_util PRIVATE src/htt_util_factory.cpp)
endif()

if(MSVC)
	target_link_libraries(htt_util setupapi)
else()
	target_link_libraries(htt_util udev)
endif()
