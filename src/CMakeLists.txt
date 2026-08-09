
# Find includes in corresponding current build & source directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

set(SRC_LIST
        main.cpp
        spdrw-worker.cpp
        spdrw-controller.cpp
        spdrw-arduino.cpp
)

qt6_add_resources(SPDRWC_RCS spdrwc.qrc)

if (WIN32)
  add_executable(spdrwc WIN32 ${SRC_LIST} ${SPDRWC_RCS})
  set_target_properties(spdrwc PROPERTIES WIN32_EXECUTABLE FALSE)
else()
  add_executable(spdrwc ${SRC_LIST} ${SPDRWC_RCS})
endif (WIN32)

target_link_libraries(spdrwc PRIVATE Qt6::Core Qt6::SerialPort)
install(TARGETS spdrwc RUNTIME DESTINATION bin)
