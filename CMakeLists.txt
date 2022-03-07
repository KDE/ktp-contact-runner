cmake_minimum_required(VERSION 3.16)

# KDE Application Version, managed by release script
set(RELEASE_SERVICE_VERSION_MAJOR "22")
set(RELEASE_SERVICE_VERSION_MINOR "03")
set(RELEASE_SERVICE_VERSION_MICRO "70")
set(KTP_CONTACT_RUNNER_VERSION "${RELEASE_SERVICE_VERSION_MAJOR}.${RELEASE_SERVICE_VERSION_MINOR}.${RELEASE_SERVICE_VERSION_MICRO}")

project(KTPContactRunner VERSION ${KTP_CONTACT_RUNNER_VERSION})

set(IS_KTP_INTERNAL_MODULE TRUE)

find_package(ECM 1.0.0 REQUIRED NO_MODULE)
set (CMAKE_MODULE_PATH
     ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR} ${CMAKE_MODULE_PATH}
)

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
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/plasma-runner-ktp-contact.desktop.cmake
               ${CMAKE_CURRENT_BINARY_DIR}/plasma-runner-ktp-contact.desktop
               @ONLY)
kcoreaddons_desktop_to_json(krunner_ktp_contacts ${CMAKE_CURRENT_BINARY_DIR}/plasma-runner-ktp-contact.desktop)
target_link_libraries(krunner_ktp_contacts
                      Qt5::Widgets
                      KF5::I18n
                      KF5::Service
                      KF5::Runner
                      KTp::Models
                      KTp::CommonInternals)

install(TARGETS krunner_ktp_contacts DESTINATION "${KDE_INSTALL_PLUGINDIR}/kf5/krunner")

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

