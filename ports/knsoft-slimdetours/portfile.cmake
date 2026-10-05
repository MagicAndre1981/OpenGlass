vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO KNSoft/KNSoft.SlimDetours
    REF 0255fac24b12781e0f0c35e0ae71bd7fc45411ee
    SHA512 c79893823b40d3973d0ccbbe47dc15335f56291556b6e806280d1258bf0a8f4e18b7f6814130301b33c937024072a810d27e67dc50f6452c7709ff5ea561c16a
    PATCHES patches/disable-ndk-nuget.patch
)

set(SLIMDETOURS_VCXPROJ "${SOURCE_PATH}/Source/KNSoft.SlimDetours/KNSoft.SlimDetours.vcxproj")
file(READ "${SLIMDETOURS_VCXPROJ}" SLIMDETOURS_VCXPROJ_CONTENTS)
string(REPLACE "\r\n" "\n" SLIMDETOURS_VCXPROJ_CONTENTS "${SLIMDETOURS_VCXPROJ_CONTENTS}")
string(REPLACE "</Project>" "  <ItemDefinitionGroup Condition=\"'$(Configuration)'=='Release'\">\n    <ClCompile>\n      <DebugInformationFormat>OldStyle</DebugInformationFormat>\n    </ClCompile>\n  </ItemDefinitionGroup>\n</Project>" SLIMDETOURS_VCXPROJ_CONTENTS "${SLIMDETOURS_VCXPROJ_CONTENTS}")
string(REPLACE "</Project>" "  <ItemDefinitionGroup>\n    <ClCompile>\n      <AdditionalIncludeDirectories>$(VcpkgIncludePath);%(AdditionalIncludeDirectories)</AdditionalIncludeDirectories>\n    </ClCompile>\n  </ItemDefinitionGroup>\n</Project>" SLIMDETOURS_VCXPROJ_CONTENTS "${SLIMDETOURS_VCXPROJ_CONTENTS}")
file(WRITE "${SLIMDETOURS_VCXPROJ}" "${SLIMDETOURS_VCXPROJ_CONTENTS}")

if(VCPKG_TARGET_ARCHITECTURE STREQUAL "x86")
    set(MSBUILD_PLATFORM "Win32")
    set(KNSOFT_ARCH "x86")
elseif(VCPKG_TARGET_ARCHITECTURE STREQUAL "x64")
    set(MSBUILD_PLATFORM "x64")
    set(KNSOFT_ARCH "x64")
elseif(VCPKG_TARGET_ARCHITECTURE STREQUAL "arm64")
    set(MSBUILD_PLATFORM "ARM64")
    set(KNSOFT_ARCH "ARM64")
elseif(VCPKG_TARGET_ARCHITECTURE STREQUAL "arm64ec")
    set(MSBUILD_PLATFORM "ARM64EC")
    set(KNSOFT_ARCH "ARM64EC")
else()
    message(FATAL_ERROR "Unsupported architecture: ${VCPKG_TARGET_ARCHITECTURE}")
endif()

set(SLIMDETOURS_MSBUILD_OPTIONS "/p:VcpkgIncludePath=${CURRENT_INSTALLED_DIR}\\include")

if(NOT DEFINED VCPKG_BUILD_TYPE OR VCPKG_BUILD_TYPE STREQUAL "release")
    vcpkg_build_msbuild(
        PROJECT_PATH "${SOURCE_PATH}/Source/KNSoft.SlimDetours/KNSoft.SlimDetours.vcxproj"
        PLATFORM "${MSBUILD_PLATFORM}"
        OPTIONS ${SLIMDETOURS_MSBUILD_OPTIONS} "/p:Configuration=Release"
    )

    file(INSTALL "${SOURCE_PATH}/Source/KNSoft.SlimDetours/OutDir/${KNSOFT_ARCH}/Release/KNSoft.SlimDetours.lib"
        DESTINATION "${CURRENT_PACKAGES_DIR}/lib")
endif()

if(NOT DEFINED VCPKG_BUILD_TYPE OR VCPKG_BUILD_TYPE STREQUAL "debug")
    vcpkg_build_msbuild(
        PROJECT_PATH "${SOURCE_PATH}/Source/KNSoft.SlimDetours/KNSoft.SlimDetours.vcxproj"
        PLATFORM "${MSBUILD_PLATFORM}"
        OPTIONS ${SLIMDETOURS_MSBUILD_OPTIONS} "/p:Configuration=Debug"
    )

    file(INSTALL "${SOURCE_PATH}/Source/KNSoft.SlimDetours/OutDir/${KNSOFT_ARCH}/Debug/KNSoft.SlimDetours.lib"
        DESTINATION "${CURRENT_PACKAGES_DIR}/debug/lib")
endif()

file(INSTALL "${SOURCE_PATH}/Source/KNSoft.SlimDetours/"
    DESTINATION "${CURRENT_PACKAGES_DIR}/include/KNSoft/SlimDetours"
    FILES_MATCHING PATTERN "*.h" PATTERN "*.inl")

file(REMOVE_RECURSE
    "${CURRENT_PACKAGES_DIR}/include/KNSoft/SlimDetours/IntDir"
    "${CURRENT_PACKAGES_DIR}/include/KNSoft/SlimDetours/OutDir")

file(INSTALL "${SOURCE_PATH}/LICENSE" DESTINATION "${CURRENT_PACKAGES_DIR}/share/knsoft-slimdetours" RENAME copyright)
