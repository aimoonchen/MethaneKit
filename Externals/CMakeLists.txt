set(CMAKE_DISABLE_FIND_PACKAGE_Doxygen TRUE)
set(CMAKE_DISABLE_FIND_PACKAGE_HarfBuzz TRUE)
set(CMAKE_DISABLE_FIND_PACKAGE_PkgConfig TRUE)
set(CMAKE_DISABLE_FIND_PACKAGE_ZLIB TRUE)
set(CMAKE_DISABLE_FIND_PACKAGE_PNG TRUE)
set(CMAKE_DISABLE_FIND_PACKAGE_BZip2 TRUE)
set(CMAKE_DISABLE_FIND_PACKAGE_HHC TRUE)
set(CMAKE_DISABLE_FIND_PACKAGE_LATEX TRUE)
set(CMAKE_DISABLE_FIND_PACKAGE_Perl TRUE)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}")

# CMake libraries
include(CPM) # keep 1-st
include(CMakeModules)
include(CMRC)
include(FindPkgConfig)

# Set Windows SDK for external libraries build
if(WIN32)
    include(FindWindowsSDK)
    if(WINDOWSSDK_FOUND)
        message(STATUS "${WINDOWSSDK_LATEST_NAME} from directory '${WINDOWSSDK_LATEST_DIR}' is used for External libraries build")
        get_windowssdk_include_dirs(${WINDOWSSDK_LATEST_DIR} WINSDK_INCLUDE_DIRS)
        get_windowssdk_library_dirs(${WINDOWSSDK_LATEST_DIR} WINSDK_LINK_DIRS)
        include_directories(${WINSDK_INCLUDE_DIRS})
        link_directories(${WINSDK_LINK_DIRS})
    else()
        message(FATAL_ERROR "Windows SDK was not found!")
    endif()
endif()

# Cross-platform C++ libraries
include(BoostNowide)
include(FMT)
include(CLI11)
include(FreeType2)
include(HLSLpp)
include(MagicEnum)
include(TaskFlow)

if(METHANE_APPS_BUILD_ENABLED)
    include(FTXUI)
endif()

if(METHANE_TRACY_PROFILING_ENABLED)
    include(Tracy)
endif()

if(METHANE_TESTS_BUILD_ENABLED)
    include(Catch2)
endif()

if(METHANE_ITT_INSTRUMENTATION_ENABLED)
    include(IttApi)
endif()

if (NOT METHANE_OPEN_IMAGE_IO_ENABLED)
    include(STB)
endif()

# DirectX API C++ libraries
if(METHANE_GFX_API EQUAL METHANE_GFX_DIRECTX)
    include(DirectXHeaders)
    include(DirectXTex)
endif()

# Vulkan API C++ libraries
if (METHANE_GFX_API EQUAL METHANE_GFX_VULKAN)
    include(VulkanHeaders)
    include(SPIRVCross)
endif()

# Pre-built binary tools
include(DirectXShaderCompilerBinary)

if (METHANE_GFX_API EQUAL METHANE_GFX_METAL)
    if (METHANE_METAL_SHADER_CONVERTER_ENABLED)
        # Metal Shader Converter is used to convert DXIL bytecode (compiled by DXC) to Metal bytecode library
        include(MetalShaderConverterBinary)
    else()
        # SPIRV-Cross tool is used to convert SPIRV bytecode (compiled by DXC) to Metal shaders on MacOS
        include(SPIRVCrossBinary)
    endif()
endif()

if (METHANE_OPEN_IMAGE_IO_ENABLED)
    include(OpenImageIOBinary)
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} PARENT_SCOPE)
