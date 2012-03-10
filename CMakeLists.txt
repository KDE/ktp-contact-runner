project(plasma-ktp-contact-runner)

# Find the required Libaries
find_package(KDE4 REQUIRED)
include(KDE4Defaults)

add_definitions (${QT_DEFINITIONS} ${KDE4_DEFINITIONS})
include_directories(
   ${CMAKE_SOURCE_DIR}
   ${CMAKE_BINARY_DIR}
   ${KDE4_INCLUDES}
   )

# We add our source code here
set(ktp-contact-runner_SRCS ktp-contact-runner.cpp)

kde4_add_plugin(krunner_ktp-contact-runner ${ktp-contact-runner_SRCS})
target_link_libraries(krunner_ktp-contact-runner
                      ${KDE4_PLASMA_LIBS} ${KDE4_KDEUI_LIBS})

install(TARGETS krunner_ktp-contact-runner
        DESTINATION ${PLUGIN_INSTALL_DIR})

install(FILES ktp-contact-runner.desktop
        DESTINATION ${SERVICES_INSTALL_DIR})

