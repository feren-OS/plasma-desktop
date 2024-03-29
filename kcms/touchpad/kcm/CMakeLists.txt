set(KCM_TOUCHPAD_SRCS
    touchpadconfigcontainer.cpp
    touchpadconfigplugin.cpp
    libinput/touchpadconfiglibinput.cpp
    xlib/customslider.cpp
    xlib/sliderpair.cpp
    xlib/testarea.cpp
    xlib/testbutton.cpp
    xlib/touchpadconfigxlib.cpp
    xlib/customconfigdialogmanager.cpp
    resources.qrc
)

ki18n_wrap_ui(KCM_TOUCHPAD_SRCS
    xlib/ui/pointermotion.ui
    xlib/ui/tap.ui
    xlib/ui/scroll.ui
    xlib/ui/sensitivity.ui
    xlib/ui/kded.ui
    xlib/ui/testarea.ui
)

add_library(kcm_touchpad MODULE ${KCM_TOUCHPAD_SRCS})
kcmutils_generate_desktop_file(kcm_touchpad)

target_link_libraries(kcm_touchpad
    PRIVATE
    Qt::Core
    Qt::Widgets
    Qt::Quick
    Qt::QuickWidgets
    Qt::DBus
    KF5::CoreAddons
    KF5::I18n
    KF5::Completion
    KF5::ConfigWidgets
    KF5::Plasma
    KF5::WindowSystem
    KF5::NotifyConfig
    KF5::XmlGui
    touchpad
)

install(TARGETS kcm_touchpad
        DESTINATION ${KDE_INSTALL_PLUGINDIR}/plasma/kcms/systemsettings
)

add_custom_command(TARGET kcm_touchpad POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E create_symlink ../kcms/systemsettings/kcm_touchpad.so kcm_touchpad_init.so)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/kcm_touchpad_init.so DESTINATION ${KDE_INSTALL_PLUGINDIR}/plasma/kcminit)
