project(KTPContactRunner)

set(KTP_CONTACT_RUNNER_VERSION "0.8.0")
set(IS_KTP_INTERNAL_MODULE TRUE)

set(CMAKE_MODULE_PATH
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules
    ${CMAKE_MODULE_PATH})
cmake_policy(SET CMP0002 OLD)

# Find the required Libaries
find_package(KDE4 REQUIRED)
find_package(TelepathyQt4 0.9.1 REQUIRED)
find_package(KTp REQUIRED)
include(KDE4Defaults)


add_definitions (${QT_DEFINITIONS} ${KDE4_DEFINITIONS})
include_directories(
   ${CMAKE_SOURCE_DIR}
   ${CMAKE_BINARY_DIR}
   ${KDE4_INCLUDES}
   ${TELEPATHY_QT4_INCLUDE_DIR}
   ${KTP_INCLUDE_DIR}
   )

# We add our source code here
set(runner_SRCS src/contactrunner.cpp)

kde4_add_plugin(krunner_ktp_contacts ${runner_SRCS})
target_link_libraries(krunner_ktp_contacts
                      ${KDE4_PLASMA_LIBS}
                      ${KDE4_KIO_LIBS}
                      ${TELEPATHY_QT4_LIBRARIES}
                      ${KTP_MODELS_LIBRARIES}
                      ${KTP_LIBRARIES})

install(TARGETS krunner_ktp_contacts
        DESTINATION ${PLUGIN_INSTALL_DIR})

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/plasma-runner-ktp-contact.desktop.cmake
               ${CMAKE_CURRENT_BINARY_DIR}/plasma-runner-ktp-contact.desktop
               @ONLY)

install (FILES ${CMAKE_CURRENT_BINARY_DIR}/plasma-runner-ktp-contact.desktop
         DESTINATION ${SERVICES_INSTALL_DIR}
)

