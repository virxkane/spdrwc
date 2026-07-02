#=============================================================================
# Copyright 2005-2011 Kitware, Inc.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# * Redistributions of source code must retain the above copyright
#   notice, this list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
#
# * Neither the name of Kitware, Inc. nor the names of its
#   contributors may be used to endorse or promote products derived
#   from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#=============================================================================

# Original File named Qt5LinguistToolsMacros.cmake
# Modified (2016) by Chernov A.A. <valexlin@gmail.com>

find_package(Qt6LinguistTools REQUIRED)

function(QT6_UPDATE_TRANSLATIONS _qm_files)
	set(options)
	set(oneValueArgs)
	set(multiValueArgs OPTIONS)

	cmake_parse_arguments(_LUPDATE "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
	set(_lupdate_files ${_LUPDATE_UNPARSED_ARGUMENTS})
	set(_lupdate_options ${_lupdate_options} -locations absolute)

	set(_my_sources)
	set(_my_tsfiles)
	foreach(_file ${_lupdate_files})
		get_filename_component(_ext ${_file} EXT)
		get_filename_component(_abs_FILE ${_file} ABSOLUTE)
		if(_ext MATCHES "ts")
			list(APPEND _my_tsfiles ${_abs_FILE})
		elseif(_ext MATCHES "cpp")
			list(APPEND _my_sources ${_abs_FILE})
		elseif(_ext MATCHES "cxx")
			list(APPEND _my_sources ${_abs_FILE})
		elseif(_ext MATCHES "ui")
			list(APPEND _my_sources ${_abs_FILE})
		endif()
	endforeach()
	foreach(_ts_file ${_my_tsfiles})
		if(_my_sources)
			# make a list file to call lupdate on, so we don't make our commands too
			# long for some systems
			get_filename_component(_ts_name ${_ts_file} NAME_WE)
			set(_ts_lst_file "${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/${_ts_name}_lst_file")
			set(_lst_file_srcs)
			foreach(_lst_file_src ${_my_sources})
				set(_lst_file_srcs "${_lst_file_src}\n${_lst_file_srcs}")
			endforeach()

			get_directory_property(_inc_DIRS INCLUDE_DIRECTORIES)
			foreach(_pro_include ${_inc_DIRS})
				get_filename_component(_abs_include "${_pro_include}" ABSOLUTE)
				set(_lst_file_srcs "-I${_pro_include}\n${_lst_file_srcs}")
			endforeach()

			file(WRITE ${_ts_lst_file} "${_lst_file_srcs}")
		endif()

		# command update ts files.
		add_custom_command(OUTPUT "${_ts_name}.ts.stamp"
			COMMAND ${QT_CMAKE_EXPORT_NAMESPACE}::lupdate ${_lupdate_options} "@${_ts_lst_file}" -ts ${_ts_file}
			COMMAND echo 1 > "${_ts_name}.ts.stamp"
			DEPENDS ${_my_sources} ${_ts_lst_file} VERBATIM)

		get_filename_component(qm ${_ts_file} NAME_WE)
		get_source_file_property(output_location ${_ts_file} OUTPUT_LOCATION)
		if(output_location)
			file(MAKE_DIRECTORY "${output_location}")
			set(qm "${output_location}/${qm}.qm")
		else()
			set(qm "${CMAKE_CURRENT_BINARY_DIR}/${qm}.qm")
		endif()

		# command to compile ts to qm.
		add_custom_command(OUTPUT ${qm}
			COMMAND ${QT_CMAKE_EXPORT_NAMESPACE}::lrelease ${_ts_file} -qm ${qm}
			DEPENDS "${_ts_file}" "${_ts_name}.ts.stamp" VERBATIM
		)
		list(APPEND ${_qm_files} ${qm})

		# create ts-file if not exist yet
		if (NOT EXISTS ${_ts_file})
			message(STATUS "File ${_ts_file} not exists, creating...")
			get_target_property(Qt6_LUPDATE_EXECUTABLE_LOCATION ${Qt6_LUPDATE_EXECUTABLE} IMPORTED_LOCATION)
			if (NOT Qt6_LUPDATE_EXECUTABLE_LOCATION)
				set(Qt6_LUPDATE_EXECUTABLE_LOCATION ${Qt6_LUPDATE_EXECUTABLE})
			endif()
			execute_process(COMMAND ${QT_CMAKE_EXPORT_NAMESPACE}::lupdate ${_lupdate_options} "@${_ts_lst_file}" -ts "${_ts_file}"
				WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
				RESULT_VARIABLE _lupdate_result
				ERROR_VARIABLE _lupdate_output
				OUTPUT_VARIABLE _lupdate_output)
			if(_lupdate_result EQUAL 0)
				message(STATUS "OK")
			else()
				message(FATAL_ERROR "lupdate failed with code: ${_lupdate_result}\n${_lupdate_output}")
			endif()
		endif()
	endforeach()
	set(${_qm_files} ${${_qm_files}} PARENT_SCOPE)
endfunction()
