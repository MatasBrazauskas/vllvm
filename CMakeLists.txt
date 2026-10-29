cmake_minimum_required(VERSION 3.28)

# Must come before project(): that is when CMake locks in the compiler, so a later
# set(CMAKE_CXX_COMPILER ...) is silently ignored.
set(CPP_COMPILER "clang++")
set(CMAKE_CXX_COMPILER ${CPP_COMPILER})

project(vllvm LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Lets clangd (CLion's engine, and any editor) resolve LLVM headers from the real
# compile flags.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# clang++ picks the highest-numbered directory under /usr/lib/gcc/<triple>/ to take
# its libstdc++ from. If the matching libstdc++-<ver>-dev headers aren't installed,
# every compile dies on "'memory' file not found". Pin clang to a version that has
# headers instead.
include(CheckCXXSourceCompiles)
check_cxx_source_compiles("#include <memory>\nint main() {}" HAVE_WORKING_LIBSTDCXX)

if (NOT HAVE_WORKING_LIBSTDCXX)
    set(GCC_INSTALL_DIR "")
    file(GLOB CXX_HEADER_DIRS "/usr/include/c++/*")
    foreach (HEADER_DIR IN LISTS CXX_HEADER_DIRS)
        if (EXISTS "${HEADER_DIR}/memory")
            get_filename_component(GCC_VERSION "${HEADER_DIR}" NAME)
            file(GLOB CANDIDATES "/usr/lib/gcc/*/${GCC_VERSION}")
            foreach (CANDIDATE IN LISTS CANDIDATES)
                if (IS_DIRECTORY "${CANDIDATE}")
                    set(GCC_INSTALL_DIR "${CANDIDATE}")
                endif ()
            endforeach ()
        endif ()
    endforeach ()

    if (GCC_INSTALL_DIR STREQUAL "")
        message(FATAL_ERROR
                "${CPP_COMPILER} cannot find the C++ standard library headers, and no "
                "usable GCC toolchain was found under /usr/lib/gcc. Install libstdc++-dev.")
    endif ()

    message(STATUS "Pinning C++ standard library to ${GCC_INSTALL_DIR}")
    add_compile_options("--gcc-install-dir=${GCC_INSTALL_DIR}")
    add_link_options("--gcc-install-dir=${GCC_INSTALL_DIR}")
endif ()

# Ubuntu/Debian ship LLVM only under versioned prefixes and there may be no
# unversioned llvm-config on PATH, so search the prefixes directly. REQUIRED makes a
# missing LLVM a hard error instead of silently empty compile flags.
file(GLOB LLVM_PREFIXES "/usr/lib/llvm-*/lib/cmake/llvm")
list(REVERSE LLVM_PREFIXES)
find_package(LLVM REQUIRED CONFIG HINTS ${LLVM_PREFIXES})
message(STATUS "Using LLVM ${LLVM_PACKAGE_VERSION} from ${LLVM_DIR}")

file(GLOB SOURCE_FILES CONFIGURE_DEPENDS "src/*.cpp")

add_executable(vllvm ${SOURCE_FILES})

target_include_directories(vllvm PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")

# SYSTEM must precede the scope keyword, and it keeps the IDE from reporting
# warnings out of LLVM's own headers.
target_include_directories(vllvm SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS})

target_compile_definitions(vllvm PRIVATE ${LLVM_DEFINITIONS})

# Deliberately NOT using `llvm-config --cxxflags`: it emits -std=c++17, which would
# land after CMake's -std=c++23 and silently downgrade the whole project.
llvm_map_components_to_libnames(LLVM_LIBS core support)
target_link_libraries(vllvm PRIVATE ${LLVM_LIBS})
