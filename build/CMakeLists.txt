#######################################
# CMAKE libmusicxml2
#######################################
project(libmusicxml2)
cmake_minimum_required(VERSION 3.4.0)
if(CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_CONFIGURATION_TYPES Debug Release)
endif()

#######################################
# versions management
set (XMLVERSION 3.1)

set (VERSION 3.2.0)
macro (get_major_minor_patch version)
	string( REGEX REPLACE "([0-9]*)\\.([0-9]*)\\.([0-9]*)" "\\1"  VERSION_MAJOR ${version} )
	string( REGEX REPLACE "([0-9]*)\\.([0-9]*)\\.([0-9]*)" "\\2"  VERSION_MINOR ${version} )
	string( REGEX REPLACE "([0-9]*)\\.([0-9]*)\\.([0-9]*)" "\\3"  VERSION_PATCH ${version} )
endmacro()
get_major_minor_patch( ${VERSION} )
set (SOVERS ${VERSION_MAJOR})
set (STRVERS "v.3.20")
set (SSTRVERS "3.20")
message (STATUS "Configuring version ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

get_filename_component(ROOT ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)


#######################################
option ( FMWK   "Generates a framework on MacOS by default"	off )
option ( GENTOOLS   "Generates tools"	 on )
option ( GENSTATIC  "Generates the static library"   on )

if (GENTOOLS)
	message (STATUS "Generate tools (force required static library generation)")
	set (GENSTATIC on)
endif()

#######################################
get_filename_component(ROOT ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)
set (DOCDIR ${ROOT}/doc)
set (DOC    ${DOCDIR}/presentation/libmusicxml2.pdf 
			${DOCDIR}/userSGuideToXml2ly/userSGuideToXml2ly.pdf 
			${DOCDIR}/maintainerSGuideToXml2ly/MaintainerSGuideToXml2ly.pdf)
set (SCHEMA ${ROOT}/dtds/${XMLVERSION})

set (BINDIR ${CMAKE_CURRENT_SOURCE_DIR}/bin)
set (LIBDIR ${CMAKE_CURRENT_SOURCE_DIR}/lib)


#######################################
set (CMAKE_CXX_STANDARD 11)

if(UNIX)
	add_definitions(-Wall -DGCC -O3 -Wno-overloaded-virtual)
endif()


#########################################
# mac os specific part
#########################################
if(APPLE)
	if ( IOS )
		message (STATUS "Generates project for iOS - Use -DIOS=no to change.")		
		set (CMAKE_XCODE_EFFECTIVE_PLATFORMS "iPhoneOS")
		set (CMAKE_OSX_ARCHITECTURES "arm64") # unsupported on iOS 11.4.0: armv7 armv7s
		set (CMAKE_OSX_SYSROOT "iphoneos")
		set (CMAKE_CXX_FLAGS "-miphoneos-version-min=11.2")
		set (libtype STATIC)
		set (FMWK "false")
	else()
		set ( DEFS ${DEFS} -DMACOS)
		set (CMAKE_C++_FLAGS -mmacosx-version-min=10.13)
		set (CMAKE_LDFLAGS -mmacosx-version-min=10.13)
		set (CMAKE_OSX_DEPLOYMENT_TARGET 10.13)
		set (MACOSX_DEPLOYMENT_TARGET 10.13)
	endif()
  # to debug:
  #add_definitions(-O0 -g) # JMI

#########################################
# windows specific part
#########################################
elseif(WIN32)
  add_definitions(-DWINVER=0x0400 -DWIN32)
  if(MSVC)
    add_definitions(-DMSVC)
  endif()
endif()


#######################################
# set directories, src and headers.
set (SRCDIR  ${ROOT}/src)
set (SAMPLEDIR ${ROOT}/samples)
set (SRCFOLDERS  factory files elements guido lib operations parser visitors)

foreach(folder ${SRCFOLDERS})
  set(SRC ${SRC} "${SRCDIR}/${folder}/*.cpp")          # add source files
endforeach()

foreach(folder ${SRCFOLDERS})
  set(HEADERS ${HEADERS} "${SRCDIR}/${folder}/*.h")    # add header files
endforeach()

set(SRC ${SRC}  "${SRCDIR}/interface/libmusicxml.cpp" "${SRCDIR}/interface/musicxml2guido.cpp")
set(HEADERS $HEADERS "${SRCDIR}/interface/libmusicxml.h" "${SRCDIR}/interface/musicxml2guido.h")
set (SRCFOLDERS  ${SRCFOLDERS}  interface)
file (GLOB CORESRC RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${SRC})
file (GLOB COREH   RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${HEADERS})

foreach(folder ${SRCFOLDERS})
  set(INCL ${INCL} "${SRCDIR}/${folder}")        # add include folders
endforeach(folder)


#######################################
# set includes
include_directories( ${INCL})
set_source_files_properties (${COREH} PROPERTIES HEADER_FILE_ONLY TRUE)


#######################################
if(WIN32 OR MSYS)
  enable_language(RC)
  set(LIBCONTENT ${CORESRC} ${COREH} ${ROOT}/win32/libmusicxml/libmusicxml.rc)
else()
  set(LIBCONTENT ${CORESRC} ${COREH})
endif()

set(target musicxml2)
set(staticlib musicxml2static)

if (NOT IOS)
add_library(${target} SHARED ${LIBCONTENT})
set_target_properties (${target} PROPERTIES
			VERSION ${VERSION}
			SOVERSION ${SOVERS}
			RUNTIME_OUTPUT_DIRECTORY  ${LIBDIR}
			RUNTIME_OUTPUT_DIRECTORY_RELEASE  ${LIBDIR}
			ARCHIVE_OUTPUT_DIRECTORY  ${LIBDIR}
			ARCHIVE_OUTPUT_DIRECTORY_RELEASE  ${LIBDIR}
			LIBRARY_OUTPUT_DIRECTORY ${LIBDIR}
			LIBRARY_OUTPUT_DIRECTORY_RELEASE ${LIBDIR}
			PUBLIC_HEADER "${COREH}"
			DEFINE_SYMBOL LIBMUSICXML_EXPORTS
			XCODE_ATTRIBUTE_ENABLE_BITCODE "NO"
)
endif()

if (NOT WIN32 AND GENSTATIC)
add_library(${staticlib} STATIC ${LIBCONTENT})
set_target_properties (${staticlib} PROPERTIES
			VERSION ${VERSION}
			SOVERSION ${SOVERS}
			ARCHIVE_OUTPUT_DIRECTORY  ${LIBDIR}
			ARCHIVE_OUTPUT_DIRECTORY_RELEASE  ${LIBDIR}
			LIBRARY_OUTPUT_DIRECTORY ${LIBDIR}
			LIBRARY_OUTPUT_DIRECTORY_RELEASE ${LIBDIR}
			PUBLIC_HEADER "${COREH}"
			DEFINE_SYMBOL LIBMUSICXML_EXPORTS
			XCODE_ATTRIBUTE_ENABLE_BITCODE "NO"
			OUTPUT_NAME musicxml2
)
endif()

if (FMWK AND APPLE AND NOT IOS)
set_target_properties (${target} PROPERTIES
      OUTPUT_NAME libmusicxml2
      FRAMEWORK ${FMWK}
      FRAMEWORK_VERSION ${VERSION}
      MACOSX_FRAMEWORK_SHORT_VERSION_STRING ${STRVERS}
      MACOSX_FRAMEWORK_BUNDLE_VERSION ${SSTRVERS}
      MACOSX_FRAMEWORK_IDENTIFIER ${target}
)
endif()

#######################################
# set sample targets
set (SAMPLES xmlversion RandomMusic RandomChords xmlfactory partsummary xml2guido xmliter countnotes readunrolled xml2midi xmlread xmltranspose xmlclone)
set (TOOLS  xml2guido xmlread xmltranspose xmlversion)

if (APPLE)
  set (LINKTO ${staticlib})
else()
  set (LINKTO ${target})
endif()


if(NOT IOS AND GENTOOLS)
foreach(sample ${SAMPLES})
	add_executable( ${sample} ${SAMPLEDIR}/${sample}.cpp )
	target_link_libraries( ${sample} ${LINKTO})
	set_target_properties (${sample} PROPERTIES
		RUNTIME_OUTPUT_DIRECTORY  ${BINDIR}
		RUNTIME_OUTPUT_DIRECTORY_RELEASE  ${BINDIR})
	add_dependencies(${sample} ${target})
endforeach(sample)
endif()


#######################################
# install setup
if (PACK)
	set (CMAKE_INSTALL_PREFIX .)
endif()

set (DOCDIR  ${CMAKE_INSTALL_PREFIX}/share/libmusicxml)
if (APPLE)
  set (BINDEST bin)
  set (LIBDEST lib)
elseif (WIN32)
  set (BINDEST bin)
  set (LIBDEST bin)
  set (DOCDIR  .)
else ()
  set (LIBDEST lib)
  set (BINDEST bin)
endif ()


if (NOT IOS)
message (STATUS "Install location is ${CMAKE_INSTALL_PREFIX}")		
set (INSTALLED ${target})
if (NOT WIN32)
if (GENSTATIC)
	set (INSTALLED ${INSTALLED}  ${staticlib})
endif()
endif()
install ( TARGETS ${INSTALLED}
	RUNTIME DESTINATION ${BINDEST} 
    LIBRARY DESTINATION ${LIBDEST} 
    ARCHIVE DESTINATION ${LIBDEST} 
    FRAMEWORK DESTINATION lib CONFIGURATIONS Release
    PUBLIC_HEADER DESTINATION include/libmusicxml
)

if (GENTOOLS)
install ( TARGETS ${TOOLS}
    RUNTIME DESTINATION ${BINDEST}
    LIBRARY DESTINATION ${LIBDEST}
    ARCHIVE DESTINATION ${LIBDEST}
)
endif()

####################################
# install misc files
install (
    DIRECTORY ${SCHEMA} DESTINATION ${DOCDIR}/schema
    FILE_PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
    DIRECTORY_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
      GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    PATTERN ".DS_Store" EXCLUDE
)

install (
    FILES ${DOC} DESTINATION ${DOCDIR}/doc
    PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)

install (
    FILES ${ROOT}/packages/README.html DESTINATION .
    PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)

install (
    FILES ${ROOT}/CHANGELOG.txt  DESTINATION .
    PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)


####################################
# install VS redistributables
if (WIN32)
if (${CMAKE_GENERATOR} STREQUAL "Visual Studio 14 2015 Win64")
	set (VS "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/redist/x64/Microsoft.VC140.CRT")
elseif (${CMAKE_GENERATOR} STREQUAL "Visual Studio 15 2017 Win64")
	set (VS "C:/Program Files (x86)/Microsoft Visual Studio/2017/Community/VC/Redist/MSVC/14.16.27012/x64/Microsoft.VC141.CRT")
else()
	set (VS "/unknown/visual/studio/version")
endif()
install (
    FILES  "${VS}/msvcp140.dll" "${VS}/vcruntime140.dll" DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
    PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)
endif()


####################################
# cpack section
####################################
set (CPACKDEST libMusicXML-${VERSION})
set (CPACK_PACKAGE_NAME   "libMusicXML")
set (CPACK_PACKAGE_VENDOR "GRAME")
set (CPACK_PACKAGE_DESCRIPTION_SUMMARY "libMusicXML - a library and tools to support the MusicXML format.")
set (CPACK_PACKAGE_VERSION   ${VERSION})
set (CPACK_PACKAGE_VERSION_MAJOR   ${VERSION_MAJOR})
set (CPACK_PACKAGE_VERSION_MINOR   ${VERSION_MINOR})
set (CPACK_PACKAGE_VERSION_PATCH   ${VERSION_PATCH})

set (CPACK_MONOLITHIC_INSTALL true)
set (CPACK_RESOURCE_FILE_LICENSE  ${ROOT}/license.txt)
set (CPACK_PACKAGE_INSTALL_DIRECTORY  ${CPACKDEST})

####################################
# NSIS specific section
set (CPACK_NSIS_INSTALL_ROOT "C:\\\\Program Files")
set (CPACK_NSIS_MODIFY_PATH on)
#set (CPACK_NSIS_MUI_ICON "${ROOT}/Resources/Images/FaustLiveIcon.ico")

include (CPack)


endif()
