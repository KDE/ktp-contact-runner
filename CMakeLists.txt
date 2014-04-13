project(KTPContactRunner)

set(KTP_CONTACT_RUNNER_VERSION "0.7.80")
set(IS_KTP_INTERNAL_MODULE TRUE)

find_package(ECM 0.0.11 REQUIRED NO_MODULE)
set (CMAKE_MODULE_PATH
     "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules"
     ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR}
)
cmake_policy(SET CMP0002 OLD)

# Find the required Libaries
find_package (Qt5 REQUIRED CONFIG COMPONENTS Core)
find_package (KF5 REQUIRED CONFIG COMPONENTS Runner I18n KDE4Support)
find_package (TelepathyQt5 0.9.2.1 REQUIRED)
find_package (KTp REQUIRED)

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings)
include(ECMPackageConfigHelpers)
include(ECMMarkNonGuiExecutable)
include(ECMPackageConfigHelpers)
include(ECMInstallIcons)
include(CMakePackageConfigHelpers)
include(WriteBasicConfigVersionFile)
include(CheckIncludeFiles)
include(FeatureSummary)

include_directories(
   ${CMAKE_SOURCE_DIR}
   ${CMAKE_BINARY_DIR}
   ${TELEPATHY_QT5_INCLUDE_DIR}
   ${KTP_INCLUDE_DIR}
   )

# We add our source code here
set(runner_SRCS src/contactrunner.cpp)

add_library(krunner_ktp_contacts MODULE ${runner_SRCS})
target_link_libraries(krunner_ktp_contacts
                      KF5::Runner
                      KF5::KDE4Support
                      ${TELEPATHY_QT5_LIBRARIES}
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

