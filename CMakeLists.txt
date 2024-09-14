CMAKE_MINIMUM_REQUIRED (VERSION 3.5)
PROJECT ( jieba VERSION "1.0.0" LANGUAGES CXX )

include(FetchContent)
FetchContent_Declare(
  limunp
  GIT_REPOSITORY  "https://github.com/yanyiwu/limonp.git"
  GIT_TAG "v0.9.0"
)

FetchContent_MakeAvailable(limunp)

add_library(jieba INTERFACE)
target_include_directories ( jieba INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/cppjieba>$<INSTALL_INTERFACE:include> )

set ( EXPORT_CMAKE_DIR "lib/cmake/jieba" )

install(
    FILES
        ${PROJECT_SOURCE_DIR}/include/cppjieba/DictTrie.hpp
		${PROJECT_SOURCE_DIR}/include/cppjieba/FullSegment.hpp
		${PROJECT_SOURCE_DIR}/include/cppjieba/HMMModel.hpp
		${PROJECT_SOURCE_DIR}/include/cppjieba/HMMSegment.hpp
		${PROJECT_SOURCE_DIR}/include/cppjieba/Jieba.hpp
		${PROJECT_SOURCE_DIR}/include/cppjieba/KeywordExtractor.hpp
		${PROJECT_SOURCE_DIR}/include/cppjieba/MixSegment.hpp
		${PROJECT_SOURCE_DIR}/include/cppjieba/MPSegment.hpp
		${PROJECT_SOURCE_DIR}/include/cppjieba/PosTagger.hpp
		${PROJECT_SOURCE_DIR}/include/cppjieba/PreFilter.hpp
		${PROJECT_SOURCE_DIR}/include/cppjieba/QuerySegment.hpp
		${PROJECT_SOURCE_DIR}/include/cppjieba/SegmentBase.hpp
		${PROJECT_SOURCE_DIR}/include/cppjieba/SegmentTagged.hpp
		${PROJECT_SOURCE_DIR}/include/cppjieba/TextRankExtractor.hpp
		${PROJECT_SOURCE_DIR}/include/cppjieba/Trie.hpp
		${PROJECT_SOURCE_DIR}/include/cppjieba/Unicode.hpp
    DESTINATION include
)

install ( TARGETS jieba EXPORT jiebaexport ARCHIVE DESTINATION "lib/$<CONFIG>" )
#install ( FILES "$<TARGET_FILE_DIR:jieba>/jieba.pdb" EXPORT jiebaexport DESTINATION "lib/$<CONFIG>" OPTIONAL )
install ( EXPORT jiebaexport FILE "jiebaTargets.cmake" DESTINATION "${EXPORT_CMAKE_DIR}" NAMESPACE "jieba::" )

include ( CMakePackageConfigHelpers )

set ( pkgconfin "${CMAKE_CURRENT_BINARY_DIR}/jiebaConfig.cmake.in" )
file ( WRITE "${pkgconfin}" "@PACKAGE_INIT@

if(NOT TARGET jieba::jieba)
    include(\"\${CMAKE_CURRENT_LIST_DIR}/jiebaTargets.cmake\")
    get_target_property ( configs jieba::jieba IMPORTED_CONFIGURATIONS )
        foreach (config RELWITHDEBINFO)
		if (NOT \${config} IN_LIST configs)
			set_property ( TARGET jieba::jieba PROPERTY MAP_IMPORTED_CONFIG_\${config} Release )
		endif ()
	endforeach ()
endif()" )

configure_package_config_file ( "${pkgconfin}" "${CMAKE_CURRENT_BINARY_DIR}/jiebaConfig.cmake" INSTALL_DESTINATION "${EXPORT_CMAKE_DIR}" )
install ( FILES "${CMAKE_CURRENT_BINARY_DIR}/jiebaConfig.cmake" DESTINATION "${EXPORT_CMAKE_DIR}" )

write_basic_package_version_file ( "jiebaConfigVersion.cmake" COMPATIBILITY ExactVersion )
install ( FILES "${CMAKE_CURRENT_BINARY_DIR}/jiebaConfigVersion.cmake" DESTINATION "${EXPORT_CMAKE_DIR}" )