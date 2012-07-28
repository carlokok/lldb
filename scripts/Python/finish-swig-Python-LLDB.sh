#! /bin/sh

# finish-swig-Python.sh
#
# For the Python script interpreter (external to liblldb) to be able to import
# and use the lldb module, there must be two files, lldb.py and _lldb.so, that
# it can find. lldb.py is generated by SWIG at the same time it generates the
# C++ file.  _lldb.so is actually a symlink file that points to the 
# LLDB shared library/framework.
#
# The Python script interpreter needs to be able to automatically find 
# these two files. On Darwin systems it searches in the LLDB.framework, as
# well as in all the normal Python search paths.  On non-Darwin systems
# these files will need to be put someplace where Python will find them.
#
# This shell script creates the _lldb.so symlink in the appropriate place,
# and copies the lldb.py (and embedded_interpreter.py) file to the correct
# directory.
#

# SRC_ROOT is the root of the lldb source tree.
# TARGET_DIR is where the lldb framework/shared library gets put.
# CONFIG_BUILD_DIR is where the build-swig-Python-LLDB.sh  shell script 
#           put the lldb.py file it was generated from running SWIG.
# PYTHON_INSTALL_DIR is where non-Darwin systems want to put the .py and .so
#           files so that Python can find them automatically.
# debug_flag (optional) determines whether or not this script outputs 
#           additional information when running.

SRC_ROOT=$1
TARGET_DIR=$2
CONFIG_BUILD_DIR=$3
PYTHON_INSTALL_DIR=$4
debug_flag=$5

# If we don't want Python, then just do nothing here.
# Note, at present iOS doesn't have Python, so if you're building for iOS be sure to
# set LLDB_DISABLE_PYTHON to 1.

if [ ! $LLDB_DISABLE_PYTHON = "1" ] ; then

if [ -n "$debug_flag" -a "$debug_flag" == "-debug" ]
then
    Debug=1
else
    Debug=0
fi

OS_NAME=`uname -s`
PYTHON_VERSION=`/usr/bin/python --version 2>&1 | sed -e 's,Python ,,' -e 's,[.][0-9],,2' -e 's,[a-z][a-z][0-9],,'`


if [ $Debug == 1 ]
then
    echo "The current OS is $OS_NAME"
    echo "The Python version is $PYTHON_VERSION"
fi

#
#  Determine where to put the files.

if [ ${OS_NAME} == "Darwin" ]
then
    # We are on a Darwin system, so all the lldb Python files can go 
    # into the LLDB.framework/Resources/Python subdirectory.

    if [ ! -d "${TARGET_DIR}/LLDB.framework" ]
    then
        echo "Error:  Unable to find LLDB.framework" >&2
        exit 1
    else
        if [ $Debug == 1 ]
        then
            echo "Found ${TARGET_DIR}/LLDB.framework."
        fi
    fi

    # Make the Python directory in the framework if it doesn't already exist

    framework_python_dir="${TARGET_DIR}/LLDB.framework/Resources/Python/lldb"
else
    # We are on a non-Darwin system, so use the PYTHON_INSTALL_DIR argument,
    # and append the python version directory to the end of it.  Depending on
    # the system other stuff may need to be put here as well.

    framework_python_dir="${PYTHON_INSTALL_DIR}/python${PYTHON_VERSION}/lldb"
fi

#
# Look for the directory in which to put the Python files;  if it does not
# already exist, attempt to make it.
#

if [ $Debug == 1 ]
then
    echo "Python files will be put in ${framework_python_dir}"
fi

python_dirs="${framework_python_dir}"

for python_dir in $python_dirs
do
    if [ ! -d "${python_dir}" ]
    then
        if [ $Debug == 1 ]
        then
            echo "Making directory ${python_dir}"
        fi
        mkdir -p "${python_dir}"
    else
        if [ $Debug == 1 ]
        then
            echo "${python_dir} already exists."
        fi
    fi

    if [ ! -d "${python_dir}" ]
    then
        echo "Error: Unable to find or create ${python_dir}" >&2
        exit 1
    fi
done

# Make the symlink that the script bridge for Python will need in the
# Python framework directory

if [ ! -L "${framework_python_dir}/_lldb.so" ]
then
    if [ $Debug == 1 ]
    then
        echo "Creating symlink for _lldb.so"
    fi
    if [ ${OS_NAME} == "Darwin" ]
    then
        cd "${framework_python_dir}"
        ln -s "../../../LLDB" _lldb.so
    else
        cd "${TARGET_DIR}"
        ln -s "../LLDB" _lldb.so
    fi
else
    if [ $Debug == 1 ]
    then
        echo "${framework_python_dir}/_lldb.so already exists."
    fi
fi


function create_python_package {
    package_dir="${framework_python_dir}$1"
    package_files="$2"
    package_name=`echo $1 | tr '/' '.'`
    package_name="lldb${package_name}"

    if [ ! -d "${package_dir}" ]
    then
        mkdir -p "${package_dir}"
    fi

    for package_file in $package_files
    do
        if [ -f "${package_file}" ]
        then
            cp "${package_file}" "${package_dir}"
            package_file_basename=$(basename "${package_file}")
        fi
    done


    # Create a packate init file if there wasn't one
    package_init_file="${package_dir}/__init__.py"
    if [ ! -f "${package_init_file}" ]
    then
        echo -n "__all__ = [" > "${package_init_file}"
        python_module_separator=""
        for package_file in $package_files
        do
            if [ -f "${package_file}" ]
            then
                package_file_basename=$(basename "${package_file}")
                echo -n "${python_module_separator}\"${package_file_basename%.*}\"" >> "${package_init_file}"
                python_module_separator=", "
            fi
        done
        echo "]" >> "${package_init_file}"
        echo "for x in __all__:" >> "${package_init_file}"
        echo "    __import__('${package_name}.'+x)" >> "${package_init_file}"
    fi


}

# Copy the lldb.py file into the lldb package directory and rename to __init_.py
cp "${CONFIG_BUILD_DIR}/lldb.py" "${framework_python_dir}/__init__.py"

# lldb
package_files="${SRC_ROOT}/source/Interpreter/embedded_interpreter.py"
create_python_package "" "${package_files}"

# lldb/formatters/cpp
package_files="${SRC_ROOT}/examples/synthetic/gnu_libstdcpp.py
${SRC_ROOT}/examples/synthetic/libcxx.py"
create_python_package "/formatters/cpp" "${package_files}"

# lldb/formatters/objc
package_files="${SRC_ROOT}/examples/summaries/cocoa/Selector.py
${SRC_ROOT}/examples/summaries/objc.py
${SRC_ROOT}/examples/summaries/cocoa/Class.py
${SRC_ROOT}/examples/summaries/cocoa/CFArray.py
${SRC_ROOT}/examples/summaries/cocoa/CFBag.py
${SRC_ROOT}/examples/summaries/cocoa/CFBinaryHeap.py
${SRC_ROOT}/examples/summaries/cocoa/CFBitVector.py
${SRC_ROOT}/examples/summaries/cocoa/CFDictionary.py
${SRC_ROOT}/examples/summaries/cocoa/CFString.py
${SRC_ROOT}/examples/summaries/cocoa/NSBundle.py
${SRC_ROOT}/examples/summaries/cocoa/NSData.py
${SRC_ROOT}/examples/summaries/cocoa/NSDate.py
${SRC_ROOT}/examples/summaries/cocoa/NSException.py
${SRC_ROOT}/examples/summaries/cocoa/NSIndexSet.py
${SRC_ROOT}/examples/summaries/cocoa/NSMachPort.py
${SRC_ROOT}/examples/summaries/cocoa/NSNotification.py
${SRC_ROOT}/examples/summaries/cocoa/NSNumber.py
${SRC_ROOT}/examples/summaries/cocoa/NSSet.py
${SRC_ROOT}/examples/summaries/cocoa/NSURL.py"
create_python_package "/formatters/objc" "${package_files}"


# make an empty __init__.py in lldb/runtime
# this is required for Python to recognize lldb.runtime as a valid package
# (and hence, lldb.runtime.objc as a valid contained package)
create_python_package "/runtime" ""

# lldb/runtime/objc
package_files="${SRC_ROOT}/examples/summaries/cocoa/objc_runtime.py"
create_python_package "/runtime/objc" "${package_files}"

# lldb/formatters
# having these files copied here ensures that lldb/formatters is a valid package itself
package_files="${SRC_ROOT}/examples/summaries/cocoa/cache.py
${SRC_ROOT}/examples/summaries/cocoa/metrics.py
${SRC_ROOT}/examples/summaries/cocoa/attrib_fromdict.py
${SRC_ROOT}/examples/summaries/cocoa/Logger.py"
create_python_package "/formatters" "${package_files}"

# lldb/utils
package_files="${SRC_ROOT}/examples/python/symbolication.py"
create_python_package "/utils" "${package_files}"

if [ ${OS_NAME} == "Darwin" ]
then
    # lldb/macosx
    package_files="${SRC_ROOT}/examples/python/crashlog.py
    ${SRC_ROOT}/examples/darwin/heap_find/heap.py"
    create_python_package "/macosx" "${package_files}"

    # Copy files needed by lldb/macosx/heap.py to build libheap.dylib
    heap_dir="${framework_python_dir}/macosx/heap"
    if [ ! -d "${heap_dir}" ]
    then
        mkdir -p "${heap_dir}"
        cp "${SRC_ROOT}/examples/darwin/heap_find/heap/heap_find.cpp" "${heap_dir}"
        cp "${SRC_ROOT}/examples/darwin/heap_find/heap/Makefile" "${heap_dir}"
    fi
fi

fi

exit 0
