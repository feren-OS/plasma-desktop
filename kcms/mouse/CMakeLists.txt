if(NOT X11_Xinput_FOUND)
    message(FATAL_ERROR "Xinput not found")
endif()

# KI18N Translation Domain for this library
add_definitions(-DTRANSLATION_DOMAIN=\"kcmmouse\")

add_subdirectory( misc )

## Add common files here.
set(common_SRCS
    inputbackend.cpp
)

include(ECMQtDeclareLoggingCategory)
ecm_qt_declare_logging_category(common_SRCS
    HEADER
        logging.h
    IDENTIFIER
        KCM_MOUSE
    CATEGORY_NAME
        kcm_mouse
    DEFAULT_SEVERITY
        Critical
    DESCRIPTION
        "KCM for mouse input"
    EXPORT
        kcm_mouse
)
ecm_qt_install_logging_categories(
    EXPORT kcm_mouse
    DESTINATION "${KDE_INSTALL_LOGGINGCATEGORIESDIR}"
)

include(backends/x11.cmake)
include(backends/kwin_wl.cmake)

########### next target ###############

add_executable(kapplymousetheme
    kapplymousetheme.cpp
    ${common_SRCS}
    ${backend_SRCS}
)

target_link_libraries(kapplymousetheme
    ${backend_LIBS}
    Qt::Gui
    Qt::DBus
    KF5::CoreAddons
    KF5::ConfigCore
    KF5::I18n
    PW::KWorkspace
)

install(TARGETS kapplymousetheme ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

########### next target ###############

set(common_SRCS
    ${common_SRCS}
    plugin.cpp
    kcm/configcontainer.cpp
    kcm/configplugin.cpp
    kcm/libinput/libinput_config.cpp
    kcm/xlib/xlib_config.cpp
)

ki18n_wrap_ui(common_SRCS kcm/xlib/kcmmouse.ui)

qt_add_resources( common_SRCS kcm/resources.qrc )

add_library(kcm_mouse MODULE
    ${common_SRCS}
    ${backend_SRCS}
)
kcmutils_generate_desktop_file(kcm_mouse)

target_link_libraries(kcm_mouse
    ${backend_LIBS}
    KF5::KCMUtils
    KF5::I18n
    KF5::KIOCore
    KF5::KIOWidgets
    PW::KWorkspace

    Qt::DBus
    Qt::QuickWidgets
)

install(TARGETS kcm_mouse DESTINATION ${KDE_INSTALL_PLUGINDIR}/plasma/kcms/systemsettings)

########### install files ###############


add_custom_command(TARGET kcm_mouse POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E create_symlink ../kcms/systemsettings/kcm_mouse.so kcm_mouse_init.so)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/kcm_mouse_init.so DESTINATION ${KDE_INSTALL_PLUGINDIR}/plasma/kcminit)
