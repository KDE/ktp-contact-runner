project(KTPContactRunner)

set(IS_KTP_INTERNAL_MODULE TRUE)

set(CMAKE_MODULE_PATH
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules
    ${CMAKE_MODULE_PATH})

# Find the required Libaries
find_package(KDE4 REQUIRED)
find_package(TelepathyQt4 REQUIRED)
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
set(runner_SRCS contactrunner.cpp)

kde4_add_plugin(krunner_ktp_contacts ${runner_SRCS})
target_link_libraries(krunner_ktp_contacts
                      ${KDE4_PLASMA_LIBS}
                      ${KDE4_KDEUI_LIBS}
                      ${KDE4_KIO_LIBS}
                      ${TELEPATHY_QT4_LIBRARIES}
                      ${KTP_MODELS_LIBRARIES})

install(TARGETS krunner_ktp_contacts
        DESTINATION ${PLUGIN_INSTALL_DIR})

install(FILES plasma-runner-ktp-contact.desktop
        DESTINATION ${SERVICES_INSTALL_DIR})

