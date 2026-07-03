#!/bin/sh

###########################################################################
#    spdrwc                                                               #
#    Copyright (C) 2026 Aleksey Chernov <valexlin@gmail.com>              #
#                                                                         #
#  This program is free software: you can redistribute it and/or modify   #
#  it under the terms of the GNU General Public License as published by   #
#  the Free Software Foundation, either version 3 of the License, or      #
#  (at your option) any later version.                                    #
#                                                                         #
#  This program is distributed in the hope that it will be useful,        #
#  but WITHOUT ANY WARRANTY; without even the implied warranty of         #
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          #
#  GNU General Public License for more details.                           #
#                                                                         #
#  You should have received a copy of the GNU General Public License      #
#  along with this program.  If not, see <https://www.gnu.org/licenses/>. #
###########################################################################


#set -x -v

# You can specify clang-format executable in .clang_format_exe.conf
#  via CLANG_FORMAT_EXE variable

dirs="src"
regex_pat=".*\.(h|c|cpp)"

die()
{
    echo $*
    exit 1
}

do_file()
{
    local f=$1
    local ret=
    echo -n "Processing file \"${f}\"... "
    ${CLANG_FORMAT_EXE} -i "${f}" > /dev/null 2>&1
    ret=$?
    test ${ret} -eq 0 && echo "done" || echo "fail"
    return $ret
}

check_clang_format_executable() {
    local cf_ver_out=`${CLANG_FORMAT_EXE} --version 2>/dev/null`
    local cf_ver=`echo ${cf_ver_out} | grep '^clang-format version ' | sed -e 's/^clang-format\ version\ \(.*\)$/\1/'`
    if [ "x${cf_ver}" = "x" ]
    then
        echo "Failed to parse clang-format version string!"
        echo "  returned string: \"${cf_ver_out}\""
        return 1
    fi
    echo "Using clang-format ${cf_ver}"
    return 0
}

export -f die
export -f do_file

if [ ! -f .clang-format ]
then
    die "You can only run this script in <top_srcdir>!"
fi

if [ -f .clang_format_exe.conf ]
then
    source ./.clang_format_exe.conf
fi
if [ "x${CLANG_FORMAT_EXE}" = "x" ]
then
    CLANG_FORMAT_EXE=clang-format
fi
check_clang_format_executable || die "Invalid clang-format"
export CLANG_FORMAT_EXE

uname_s=`uname -s`
is_darwin=no
test "x${uname_s}" = "xDarwin" && is_darwin="yes"

if [ "x${is_darwin}" = "xyes" ]
then
    for dir in ${dirs}
    do
        find -E ${dir} -depth -maxdepth 5 -type f -iregex ${regex_pat} -exec bash -c 'do_file "$0"' '{}' \;
    done
else
    for dir in ${dirs}
    do
        find ${dir} -depth -maxdepth 5 -type f -regextype posix-egrep -iregex ${regex_pat} -exec bash -c 'do_file "$0"' '{}' \;
    done
fi
