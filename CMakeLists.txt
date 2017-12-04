project(KTPContactRunner)

cmake_minimum_required(VERSION 2.8.12)

include(FeatureSummary)

# KDE Application Version, managed by release script
set(KDE_APPLICATIONS_VERSION_MAJOR "17")
set(KDE_APPLICATIONS_VERSION_MINOR "12")
set(KDE_APPLICATIONS_VERSION_MICRO "0")
set(KTP_CONTACT_RUNNER_VERSION "${KDE_APPLICATIONS_VERSION_MAJOR}.${KDE_APPLICATIONS_VERSION_MINOR}.${KDE_APPLICATIONS_VERSION_MICRO}")
set(IS_KTP_INTERNAL_MODULE TRUE)

find_package(ECM 1.0.0 REQUIRED NO_MODULE)
set (CMAKE_MODULE_PATH
     ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR} ${CMAKE_MODULE_PATH}
)
cmake_policy(SET CMP0002 OLD)

# Find the required Libaries
find_package (Qt5 REQUIRED CONFIG COMPONENTS Widgets Core DBus Network)
find_package (KF5 REQUIRED COMPONENTS I18n Service Runner)
find_package (KTp REQUIRED)

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings)
include(FeatureSummary)

include_directories(
   ${CMAKE_SOURCE_DIR}
   ${CMAKE_BINARY_DIR}
   )

# We add our source code here
set(runner_SRCS src/contactrunner.cpp)

add_library(krunner_ktp_contacts MODULE ${runner_SRCS})
target_link_libraries(krunner_ktp_contacts
                      Qt5::Widgets
                      KF5::I18n
                      KF5::Service
                      KF5::Runner
                      KTp::Models
                      KTp::CommonInternals)

install(TARGETS krunner_ktp_contacts
        DESTINATION ${PLUGIN_INSTALL_DIR})

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/plasma-runner-ktp-contact.desktop.cmake
               ${CMAKE_CURRENT_BINARY_DIR}/plasma-runner-ktp-contact.desktop
               @ONLY)

install (FILES ${CMAKE_CURRENT_BINARY_DIR}/plasma-runner-ktp-contact.desktop
         DESTINATION ${SERVICES_INSTALL_DIR}
)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

