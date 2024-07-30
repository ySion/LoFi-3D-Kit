if (NOT EXISTS "F:/MyProjects/LoFi-Kit/cmake-build-release/install_manifest.txt")
    message(FATAL_ERROR "Cannot find install manifest: \"F:/MyProjects/LoFi-Kit/cmake-build-release/install_manifest.txt\"")
endif()

file(READ "F:/MyProjects/LoFi-Kit/cmake-build-release/install_manifest.txt" files)
string(REGEX REPLACE "\n" ";" files "${files}")
foreach(file ${files})
    message(STATUS "Uninstalling \"$ENV{DESTDIR}${file}\"")
    execute_process(
        COMMAND C:/Program Files/JetBrains/CLion 2024.1.4/bin/cmake/win/x64/bin/cmake.exe -E remove "$ENV{DESTDIR}${file}"
        OUTPUT_VARIABLE rm_out
        RESULT_VARIABLE rm_retval
    )
    if(NOT ${rm_retval} EQUAL 0)
        message(FATAL_ERROR "Problem when removing \"$ENV{DESTDIR}${file}\"")
    endif (NOT ${rm_retval} EQUAL 0)
endforeach()
