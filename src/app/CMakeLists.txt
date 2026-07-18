# AtivaStage GUI executable (Qt Quick).
# Only built when ATIVASTAGE_BUILD_APP=ON (Qt is resolved via CMAKE_PREFIX_PATH,
# set by the mac-*/win-* presets). Phase 1, Marco 1.0: minimal empty window that
# proves the Qt Quick toolchain + packaging path end to end. No engines wired yet.

qt_add_executable(app
    src/main.cpp
)

set_target_properties(app PROPERTIES OUTPUT_NAME AtivaStage)

qt_add_qml_module(app
    URI AtivaStage
    VERSION 1.0
    QML_FILES
        qml/Main.qml
)

target_link_libraries(app PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
)

# macOS .app bundle metadata (Info.plist basics). Bundle id per ADR-0003.
if(APPLE)
    set_target_properties(app PROPERTIES
        MACOSX_BUNDLE ON
        MACOSX_BUNDLE_BUNDLE_NAME "AtivaStage"
        MACOSX_BUNDLE_GUI_IDENTIFIER "br.com.ativa.ativastage"
        MACOSX_BUNDLE_BUNDLE_VERSION "${ATIVASTAGE_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${ATIVASTAGE_VERSION}"
    )
endif()
