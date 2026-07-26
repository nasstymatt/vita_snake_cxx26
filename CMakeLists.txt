cmake_minimum_required(VERSION 3.28)

# ---------------------------------------------------------------------------
# Toolchain
# ---------------------------------------------------------------------------
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    if(DEFINED ENV{VITASDK})
        set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
    else()
        message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
    endif()
endif()

project(snake LANGUAGES CXX)

include("${VITASDK}/share/vita.cmake" REQUIRED)

set(VITA_APP_NAME "Snake CXX26")
set(VITA_TITLEID  "VSDK00007")
set(VITA_VERSION  "01.00")

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:-fmodules-ts>")

# Vita toolchain injects -Wl,-q into CMAKE_CXX_FLAGS; strip it for
# module-only (-c) invocations, since those never link.
string(REPLACE "-Wl,-q" "" MODULE_FLAGS "${CMAKE_CXX_FLAGS}")
separate_arguments(MODULE_FLAGS_LIST UNIX_COMMAND "${MODULE_FLAGS}")

# ---------------------------------------------------------------------------
# Standard library module (import std;)
#
# The path is derived from the compiler's own reported version instead of
# being hardcoded, so this keeps working across `vitasdk-update` bumps.
# ---------------------------------------------------------------------------
execute_process(
    COMMAND ${CMAKE_CXX_COMPILER} -dumpversion
    OUTPUT_VARIABLE VITA_GCC_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
set(VITA_STD_MODULE_SRC
    "$ENV{VITASDK}/arm-vita-eabi/include/c++/${VITA_GCC_VERSION}/bits/std.cc"
)

if(NOT EXISTS "${VITA_STD_MODULE_SRC}")
    message(FATAL_ERROR
        "Could not find std module source at ${VITA_STD_MODULE_SRC}. "
        "Your VitaSDK GCC layout may differ from what this script expects."
    )
endif()

add_custom_command(
    OUTPUT "${CMAKE_BINARY_DIR}/gcm.cache/std.gcm" "${CMAKE_BINARY_DIR}/std_module.o"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/gcm.cache"
    COMMAND ${CMAKE_CXX_COMPILER} ${MODULE_FLAGS_LIST} -std=c++26 -fmodules-ts -fsearch-include-path
            -c "${VITA_STD_MODULE_SRC}" -o "${CMAKE_BINARY_DIR}/std_module.o"
    DEPENDS "${VITA_STD_MODULE_SRC}"
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    COMMENT "Compiling C++26 Standard Library Module..."
)

set(STD_GCM "${CMAKE_BINARY_DIR}/gcm.cache/std.gcm")

# ---------------------------------------------------------------------------
# Project module compilation
#
# Each module is compiled to its own .gcm (interface) + .o (object) pair.
# Modules that `import` another project module must depend on that module's
# .gcm so GCC can resolve the import before it needs it.
# ---------------------------------------------------------------------------
function(compile_vita_module FILE_NAME GCM_NAME)
    add_custom_command(
        OUTPUT "${CMAKE_BINARY_DIR}/gcm.cache/${GCM_NAME}.gcm"
               "${CMAKE_BINARY_DIR}/${GCM_NAME}.o"
        COMMAND ${CMAKE_CXX_COMPILER} ${MODULE_FLAGS_LIST} -std=c++26 -fmodules-ts
                --embed-dir=${CMAKE_SOURCE_DIR}
                "-I${CMAKE_SOURCE_DIR}/vendor"
                -c "${CMAKE_SOURCE_DIR}/${FILE_NAME}" -o "${CMAKE_BINARY_DIR}/${GCM_NAME}.o"
        DEPENDS "${CMAKE_SOURCE_DIR}/${FILE_NAME}" ${ARGN}
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
        COMMENT "Building module ${GCM_NAME}..."
    )
endfunction()

# Module graph, declared as data instead of five hand-written calls.
# Format per entry: "source path|module name|comma-separated deps"
# Leave deps empty if the module only imports std.
#
# NOTE: deps use "," not ";" on purpose. CMake list variables are just
# ";"-joined strings under the hood, so a literal ";" inside one of these
# quoted entries would get split into a *separate* list item at set()-time
# regardless of quoting, silently corrupting the whole list. "," has no
# special meaning to CMake's list machinery, so it round-trips safely.
#
# To add a module: add one line here. Nothing else in this file changes.
set(VITA_MODULES
    "src/vita/process.cppm|vita.process|"
    "src/vita/math.cppm|vita.math|"
    "src/vita/ctrl.cppm|vita.ctrl|vita.math"
    "src/vita/gfx.cppm|vita.gfx|vita.math,vita.process|"
		"src/vita/ui.cppm|vita.ui|vita.math,vita.gfx|"
)

set(MODULE_OBJECTS "${CMAKE_BINARY_DIR}/std_module.o")

foreach(entry IN LISTS VITA_MODULES)
    string(REPLACE "|" ";" parts "${entry}")
    list(GET parts 0 mod_src)
    list(GET parts 1 mod_name)
    list(GET parts 2 mod_deps_raw)

    # Turn the comma-separated deps back into a real CMake list now that
    # we're safely past the point where a ";" could get misinterpreted.
    string(REPLACE "," ";" mod_deps "${mod_deps_raw}")

    # Every module implicitly depends on std.gcm being ready.
    set(dep_gcms "${STD_GCM}")
    foreach(dep IN LISTS mod_deps)
        if(dep)
            list(APPEND dep_gcms "${CMAKE_BINARY_DIR}/gcm.cache/${dep}.gcm")
        endif()
    endforeach()

    compile_vita_module("${mod_src}" "${mod_name}" ${dep_gcms})
    list(APPEND MODULE_OBJECTS "${CMAKE_BINARY_DIR}/${mod_name}.o")
endforeach()

# The .o files above come from add_custom_command, not a compiler invocation
# CMake tracked itself — mark them so CMake treats them as prebuilt objects
# it can just pass straight to the linker.
set_source_files_properties(${MODULE_OBJECTS}
    PROPERTIES GENERATED TRUE EXTERNAL_OBJECT TRUE
)

# ---------------------------------------------------------------------------
# Executable
#
# Module objects are handed directly to the executable target — no
# intermediate static library. A static lib here would just archive these
# objects and immediately unpack them again at link time, so skipping it
# removes a build step for no loss of functionality.
# ---------------------------------------------------------------------------
add_executable(${PROJECT_NAME}
    src/main.cpp
    ${MODULE_OBJECTS}
)

# ---------------------------------------------------------------------------
# vitaGL + Vita system libraries
#
# We link our own build of vitaGL (~/.local/opt/vitaGL), NOT the SDK's
# prebuilt libvitaGL.a. The SDK copy is a different build (config and/or
# source revision) that SIGSEGVs this app at runtime even with ImGui removed;
# our build runs. (It is NOT missing the legacy fixed-function pipeline -- it
# exports glMatrixMode/glVertexPointer/etc.; the exact cause of the SDK
# build's crash was not pinned down, only that swapping it in breaks the app.)
#
# imgui-vita's render backend (ImGui_ImplVitaGL_RenderDrawData) uses vitaGL's
# "mapped" fast-path (vglVertexPointerMapped/vglDrawObjects), which does NOT
# render on this vitaGL build -- draws are silently dropped while plain
# glDrawArrays/glDrawElements work. Confirmed empirically with BOTH the vdpm
# prebuilt libimgui.a and a locally-built one from up-to-date source, so it is
# not a stale-binary bug. vita.ui therefore ignores that backend's draw call
# and renders ImGui itself via the standard glDrawElements path (see ui.cppm).
# We still use the backend for NewFrame/Init (font upload + input), which work.
# ---------------------------------------------------------------------------
set(VITAGL_DIR "$ENV{HOME}/.local/opt/vitaGL" CACHE PATH "Path to vitaGL install")
target_include_directories(${PROJECT_NAME} PRIVATE "${VITAGL_DIR}/source")

# imgui and vitaGL have interdependencies with each other and with the Sce
# *_stub libraries (imgui's vitagl/touch backends call glGetIntegerv,
# sceTouch*, sceAppUtil*, ...). Static archives resolve left-to-right, so a
# plain flat list forces a fragile ordering. Wrap the archives in a linker
# group so ld keeps re-scanning them until all cross-references settle.
set(VITA_LIBS
    -Wl,--start-group
    "${VITAGL_DIR}/libvitaGL.a"
    vitashark
    SceShaccCgExt
    SceShaccCg_stub
    taihen_stub
    SceGxm_stub
    SceDisplay_stub
    SceLibKernel_stub
    SceAppMgr_stub
    SceAppUtil_stub
    SceCommonDialog_stub
    SceSysmodule_stub
    SceKernelDmacMgr_stub
    SceCtrl_stub
    SceTouch_stub
    mathneon
    zip
    z
    m
    stdc++
    -Wl,--end-group
)

target_link_libraries(${PROJECT_NAME} ${VITA_LIBS})

# ---------------------------------------------------------------------------
# Packaging
# ---------------------------------------------------------------------------
vita_create_self(${PROJECT_NAME}.self ${PROJECT_NAME})

vita_create_vpk(${PROJECT_NAME}.vpk ${VITA_TITLEID} ${PROJECT_NAME}.self
    VERSION ${VITA_VERSION}
    NAME ${VITA_APP_NAME}
    FILE sce_sys/icon0.png sce_sys/icon0.png
)
