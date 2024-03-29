# KI18N Translation Domain for this library
add_definitions(-DTRANSLATION_DOMAIN=\"kcm_smserver\")

set(kcm_smserver_PART_SRCS kcmsmserver.cpp)

kcmutils_generate_module_data(
    kcm_smserver_PART_SRCS
    MODULE_DATA_HEADER smserverdata.h
    MODULE_DATA_CLASS_NAME SMServerData
    SETTINGS_HEADERS smserversettings.h
    SETTINGS_CLASSES SMServerSettings
)

qt_add_dbus_interface(kcm_smserver_PART_SRCS org.freedesktop.login1.Manager.xml login1_manager)

kconfig_add_kcfg_files(kcm_smserver_PART_SRCS smserversettings.kcfgc GENERATE_MOC)

kcoreaddons_add_plugin(kcm_smserver SOURCES ${kcm_smserver_PART_SRCS} INSTALL_NAMESPACE "plasma/kcms/systemsettings")
kcmutils_generate_desktop_file(kcm_smserver)

target_link_libraries(kcm_smserver
    Qt::Core
    Qt::DBus
    KF5::QuickAddons
    KF5::I18n
    KF5::ConfigCore
    KF5::KCMUtils
    PW::KWorkspace)

add_subdirectory(kconf_update)

########### install files ###############

kpackage_install_package(package kcm_smserver kcms)

