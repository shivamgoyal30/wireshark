# CMakeLists.txt
#
# Wireshark - Network traffic analyzer
# By Gerald Combs <gerald@wireshark.org>
# Copyright 1998 Gerald Combs
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#

include(UseABICheck)

if(WIN32)
	set(WSUTIL_PLATFORM_FILES
		file_util.c
		inet_ntop.c
		inet_pton.c
	)
ENDIF(WIN32)

set(WSUTIL_FILES
	adler32.c
	aes.c
	airpdcap_wep.c
	base64.c
	bitswap.c
	buffer.c
	cfutils.c
	clopts_common.c
	cmdarg_err.c
	copyright_info.c
	crash_info.c
	crc10.c
	crc16.c
	crc16-plain.c
	crc32.c
	crc6.c
	crc7.c
	crc8.c
	crc11.c
	des.c
	eax.c
	filesystem.c
	g711.c
	md4.c
	md5.c
	mpeg-audio.c
	nstime.c
	os_version_info.c
	plugins.c
	privileges.c
	sha1.c
	sober128.c
	strnatcmp.c
	str_util.c
	rc4.c
	report_err.c
	tempfile.c
	time_util.c
	type_util.c
	u3.c
	unicode-utils.c
	ws_mempbrk.c
	ws_version_info.c
	${WSUTIL_PLATFORM_FILES}
)

#
# XXX - we're assuming MSVC doesn't require a flag to enable SSE 4.2
# support, and that, if the compiler supports a flag for SSE 4.2
# support, the intrinsics are supported iff we can include the
# <nmmintrin.h> flag.
#
# We only check for the GCC-style -msse4.2 flag and the Sun C
# -xarch=sse4_2 flag.
#
if(CMAKE_C_COMPILER_ID MATCHES "MSVC")
	set(HAVE_SSE4_2 TRUE)
	set(SSE4_2_FLAG "")
else()
	message(STATUS "Checking for c-compiler flag: -msse4.2")
	check_c_compiler_flag(-msse4.2 HAVE_SSE4_2)
	if(HAVE_SSE4_2)
		set(SSE4_2_FLAG "-msse4.2")
	else()
		message(STATUS "Checking for c-compiler flag: -xarch=sse4_2")
		check_c_compiler_flag(-xarch=sse4_2 HAVE_SSE4_2)
		if(HAVE_SSE4_2)
			set(SSE4_2_FLAG "-xarch=sse4_2")
		endif()
	endif()
endif()
if(HAVE_SSE4_2)
	#
	# Make sure we have the necessary header for the SSE4.2 intrinsics
	# and that we can use it.
	#

	#
	# Does this add the SSE4.2 flags to the beginning of CFLAGS?
	#
	cmake_push_check_state()
	set(CMAKE_REQUIRED_FLAGS "${SSE4_2_FLAG}")
	check_include_file("nmmintrin.h" CAN_USE_SSE4_2)
	cmake_pop_check_state()
	if(NOT CAN_USE_SSE4_2)
		#
		# OK, it's not working for some reason, perhaps because
		# ${SSE4_2_FLAG} above didn't enable SSE 4.2 support.
		#
		set(HAVE_SSE4_2 FALSE)
	endif()
endif()
if(HAVE_SSE4_2)
	set(WSUTIL_FILES ${WSUTIL_FILES} ws_mempbrk_sse42.c)
endif()

if(NOT HAVE_GETOPT_LONG)
	set(WSUTIL_FILES ${WSUTIL_FILES} wsgetopt.c)
endif()

if(NOT HAVE_INET_ATON)
	set(WSUTIL_FILES ${WSUTIL_FILES} inet_aton.c)
endif()

if(NOT HAVE_POPCOUNT)
	set(WSUTIL_FILES ${WSUTIL_FILES} popcount.c)
endif()

if(NOT HAVE_STRPTIME)
	set(WSUTIL_FILES ${WSUTIL_FILES} strptime.c)
endif()

if(APPLE)
	#
	# We assume that APPLE means OS X so that we have the OS X
	# frameworks.
	#
	FIND_LIBRARY (APPLE_CORE_FOUNDATION_LIBRARY CoreFoundation)
endif()

set(wsutil_LIBS
	${APPLE_CORE_FOUNDATION_LIBRARY}
	${GMODULE2_LIBRARIES}
	${GLIB2_LIBRARIES}
	${GCRYPT_LIBRARIES}
	${WIN_WSOCK32_LIBRARY}
)
IF(WIN32)
	set(wsutil_LIBS ${wsutil_LIBS} "ws2_32.lib")
ENDIF(WIN32)

set(CLEAN_FILES
	${WSUTIL_FILES}
)

if (WERROR)
	set_source_files_properties(
		${CLEAN_FILES}
		PROPERTIES
		COMPILE_FLAGS -Werror
	)
else()
	#
	# Set the property to an empty string, so that if we try
	# to get it below, it succeeds.
	#
	set_source_files_properties(
		${CLEAN_FILES}
		PROPERTIES
		COMPILE_FLAGS ""
	)
endif()
if (HAVE_SSE4_2)
	get_source_file_property(
		WS_MEMPBRK_SSE42_COMPILE_FLAGS
		ws_mempbrk_sse42.c
		COMPILE_FLAGS
	)
	set_source_files_properties(
		ws_mempbrk_sse42.c
		PROPERTIES
		COMPILE_FLAGS "${WS_MEMPBRK_SSE42_COMPILE_FLAGS} ${SSE4_2_FLAG}"
	)
endif()

add_library(wsutil ${LINK_MODE_LIB}
	${WSUTIL_FILES}
	${CMAKE_BINARY_DIR}/image/libwsutil.rc
)

add_dependencies(wsutil gitversion)

set(FULL_SO_VERSION "0.0.0")

set_target_properties(wsutil PROPERTIES
	PREFIX "lib"
	COMPILE_DEFINITIONS "WS_BUILD_DLL"
	LINK_FLAGS "${WS_LINK_FLAGS}"
	VERSION ${FULL_SO_VERSION} SOVERSION 0
	FOLDER "Libs")

if(ENABLE_APPLICATION_BUNDLE)
	set_target_properties(wsutil PROPERTIES
		LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/run/Wireshark.app/Contents/Frameworks
	)
endif()

ABICHECK(libwsutil)

add_custom_command(OUTPUT libwsutil.abi.tar.gz
	COMMAND cp ../config.h ${ABICHECK_TMPDIR}/
	COMMAND ${ABICHECK_COMMAND}
	COMMAND cp ${CMAKE_CURRENT_BINARY_DIR}/abi_dumps/libwsutil/libwsutil_* ${CMAKE_CURRENT_BINARY_DIR}/libwsutil.abi.tar.gz
	COMMAND rm -rf ${ABICHECK_TMPDIR}/* ${CMAKE_CURRENT_BINARY_DIR}/abi_dumps
	DEPENDS ${HEADERS} wsutil)

target_link_libraries(wsutil ${wsutil_LIBS})

if(NOT ${ENABLE_STATIC})
	install(TARGETS wsutil
		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
		RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}
		ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	)
endif()

add_definitions( -DTOP_SRCDIR=\"${CMAKE_SOURCE_DIR}\" )

#
# Editor modelines  -  http://www.wireshark.org/tools/modelines.html
#
# Local variables:
# c-basic-offset: 8
# tab-width: 8
# indent-tabs-mode: t
# End:
#
# vi: set shiftwidth=8 tabstop=8 noexpandtab:
# :indentSize=8:tabSize=8:noTabs=false:
#
