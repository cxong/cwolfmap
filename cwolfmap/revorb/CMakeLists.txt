cmake_minimum_required(VERSION 3.31.6)
project(revorb C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

option(INTERNAL_OGG "Force use of vendored libogg" ON)
option(INTERNAL_VORBIS "Force use of vendored libvorbis" ON)

if(NOT INTERNAL_OGG OR NOT INTERNAL_VORBIS)
	find_package(PkgConfig)

	if(PkgConfig_FOUND AND NOT INTERNAL_OGG)
		pkg_check_modules(ogg IMPORTED_TARGET GLOBAL ogg)
		add_library(Ogg::ogg ALIAS PkgConfig::ogg)
	else()
		set(INTERNAL_OGG ON)
	endif()

	if(PkgConfig_FOUND AND NOT INTERNAL_VORBIS)
		pkg_check_modules(vorbis IMPORTED_TARGET GLOBAL vorbis)
		add_library(vorbis ALIAS PkgConfig::vorbis)
	else()
		set(INTERNAL_VORBIS ON)
	endif()
endif()

if(INTERNAL_OGG)
	add_library(ogg STATIC libogg/src/bitwise.c libogg/src/framing.c)
	target_include_directories(ogg PUBLIC libogg/include)
	target_compile_options(ogg PRIVATE -w)
	add_library(Ogg::ogg ALIAS ogg)

	if(NOT WIN32 AND NOT APPLE)
		include(CheckIncludeFiles)
		check_include_files(inttypes.h INCLUDE_INTTYPES_H)
		check_include_files(stdint.h INCLUDE_STDINT_H)
		check_include_files(sys/types.h INCLUDE_SYS_TYPES_H)
		set(SIZE16 int16_t)
		set(USIZE16 uint16_t)
		set(SIZE32 int32_t)
		set(USIZE32 uint32_t)
		set(SIZE64 int64_t)
		set(USIZE64 uint64_t)

		configure_file(libogg/include/ogg/config_types.h.in "${CMAKE_CURRENT_BINARY_DIR}/libogg/include/ogg/config_types.h" @ONLY)
		target_include_directories(ogg PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/libogg/include")
	endif()
endif()

if(INTERNAL_VORBIS)
	add_library(vorbis STATIC
		libvorbis/lib/mdct.c
		libvorbis/lib/smallft.c
		libvorbis/lib/block.c
		libvorbis/lib/envelope.c
		libvorbis/lib/window.c
		libvorbis/lib/lsp.c
		libvorbis/lib/lpc.c
		libvorbis/lib/analysis.c
		libvorbis/lib/synthesis.c
		libvorbis/lib/psy.c
		libvorbis/lib/info.c
		libvorbis/lib/floor1.c
		libvorbis/lib/floor0.c
		libvorbis/lib/res0.c
		libvorbis/lib/mapping0.c
		libvorbis/lib/registry.c
		libvorbis/lib/codebook.c
		libvorbis/lib/sharedbook.c
		libvorbis/lib/lookup.c
		libvorbis/lib/bitrate.c
		libvorbis/lib/envelope.h
		libvorbis/lib/lpc.h
		libvorbis/lib/lsp.h
		libvorbis/lib/codebook.h
		libvorbis/lib/misc.h
		libvorbis/lib/psy.h
		libvorbis/lib/masking.h
		libvorbis/lib/os.h
		libvorbis/lib/mdct.h
		libvorbis/lib/smallft.h
		libvorbis/lib/highlevel.h
		libvorbis/lib/registry.h
		libvorbis/lib/scales.h
		libvorbis/lib/window.h
		libvorbis/lib/lookup.h
		libvorbis/lib/lookup_data.h
		libvorbis/lib/codec_internal.h
		libvorbis/lib/backends.h
		libvorbis/lib/bitrate.h
	)
	target_include_directories(vorbis PUBLIC libvorbis/include PRIVATE libvorbis/lib)
	target_compile_options(vorbis PRIVATE -w)
	target_link_libraries(vorbis PUBLIC Ogg::ogg)
	if(NOT WIN32)
		target_link_libraries(cwolfmap m)
	endif()
endif()

add_subdirectory(revorb)

add_executable(revorb_test main.c)
target_compile_options(revorb_test PRIVATE -w)
target_link_libraries(revorb_test revorb)
