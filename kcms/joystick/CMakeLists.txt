# KI18N Translation Domain for this library
add_definitions(-DTRANSLATION_DOMAIN=\"kcm5_joystick\")


########### next target ###############

set(kcm_joystick_PART_SRCS 
   joystick.cpp 
   joywidget.cpp 
   poswidget.cpp 
   joydevice.cpp 
   caldialog.cpp )


add_library(kcm_joystick MODULE ${kcm_joystick_PART_SRCS})
kcmutils_generate_desktop_file(kcm_joystick)

target_link_libraries(kcm_joystick
    KF5::Completion
    KF5::KCMUtils
    KF5::I18n
    KF5::KIOWidgets
)

install(TARGETS kcm_joystick  DESTINATION ${KDE_INSTALL_PLUGINDIR}/plasma/kcms/systemsettings_qwidgets )

