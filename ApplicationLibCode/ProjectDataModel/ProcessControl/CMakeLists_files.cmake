set(SOURCE_GROUP_HEADER_FILES ${CMAKE_CURRENT_LIST_DIR}/RimProcess.h
                              ${CMAKE_CURRENT_LIST_DIR}/RimProcessMonitor.h
)

set(SOURCE_GROUP_SOURCE_FILES ${CMAKE_CURRENT_LIST_DIR}/RimProcess.cpp
                              ${CMAKE_CURRENT_LIST_DIR}/RimProcessMonitor.cpp
)

list(APPEND CODE_HEADER_FILES ${SOURCE_GROUP_HEADER_FILES})

list(APPEND CODE_SOURCE_FILES ${SOURCE_GROUP_SOURCE_FILES})

list(APPEND COMMAND_QT_MOC_HEADERS
     ${CMAKE_CURRENT_LIST_DIR}/RimProcessMonitor.h
)

source_group(
  "ProjectDataModel\\ProcessControl"
  FILES ${SOURCE_GROUP_HEADER_FILES} ${SOURCE_GROUP_SOURCE_FILES}
        ${CMAKE_CURRENT_LIST_DIR}/CMakeLists_files.cmake
)
