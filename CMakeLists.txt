CMAKE_MINIMUM_REQUIRED (VERSION 3.17)
PROJECT ( jieba VERSION "1.0.0" LANGUAGES CXX )

if (NOT NO_BUILD)
	set ( NO_BUILD OFF )
endif ()

set ( SOURCE_SUBDIR "." )
if (NO_BUILD)
	set (SOURCE_SUBDIR "fake_nonexistent_to_avoid_accidental_build")
	message ( STATUS "Only headers and dict will be installed" )
endif()

include(FetchContent)
FetchContent_Declare(
  limunp
  SOURCE_SUBDIR "${SOURCE_SUBDIR}"
  GIT_REPOSITORY  "https://github.com/yanyiwu/limonp.git"
  GIT_TAG "v0.9.0"
)

FetchContent_MakeAvailable ( limunp )

add_library ( jieba INTERFACE )
add_library ( jieba::jieba ALIAS jieba )
target_include_directories ( jieba INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/cppjieba>$<INSTALL_INTERFACE:include> )

install ( DIRECTORY include/cppjieba/ DESTINATION include )
if (NO_BUILD)
	install ( DIRECTORY "${FETCHCONTENT_BASE_DIR}/limunp-src/include/" DESTINATION include )
endif()

file (GLOB JIEBA_DICTS "${CMAKE_CURRENT_SOURCE_DIR}/dict/*.*" )

add_library ( jiebadict INTERFACE )
add_library ( jieba::jiebadict ALIAS jiebadict )
target_sources ( jiebadict INTERFACE $<INSTALL_INTERFACE:dict> )
install ( FILES ${JIEBA_DICTS} DESTINATION dict )

set ( EXPORT_CMAKE_DIR "lib/cmake/jieba" )

install ( TARGETS jieba jiebadict EXPORT jiebaexport ARCHIVE DESTINATION lib )
#install ( FILES "$<TARGET_FILE_DIR:jieba>/jieba.pdb" EXPORT jiebaexport DESTINATION "lib/$<CONFIG>" OPTIONAL )
install ( EXPORT jiebaexport FILE "jieba-targets.cmake" DESTINATION "${EXPORT_CMAKE_DIR}" NAMESPACE "jieba::" )

include ( CMakePackageConfigHelpers )

set ( pkgconfin "${CMAKE_CURRENT_BINARY_DIR}/jieba-config.cmake.in" )
file ( WRITE "${pkgconfin}" "@PACKAGE_INIT@
include(\"\${CMAKE_CURRENT_LIST_DIR}/jieba-targets.cmake\")" )

configure_package_config_file ( "${pkgconfin}" "${CMAKE_CURRENT_BINARY_DIR}/jieba-config.cmake" INSTALL_DESTINATION "${EXPORT_CMAKE_DIR}" )
install ( FILES "${CMAKE_CURRENT_BINARY_DIR}/jieba-config.cmake" DESTINATION "${EXPORT_CMAKE_DIR}" )

write_basic_package_version_file ( "jieba-config-version.cmake" COMPATIBILITY ExactVersion )
install ( FILES "${CMAKE_CURRENT_BINARY_DIR}/jieba-config-version.cmake" DESTINATION "${EXPORT_CMAKE_DIR}" )