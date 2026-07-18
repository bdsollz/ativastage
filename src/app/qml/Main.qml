// AtivaStage — root window (Phase 1, Marco 1.0 placeholder).
// Empty operator window that proves the Qt Quick pipeline renders on both
// platforms. Real UI arrives in Fase 3 (Central de Música), driven by the
// Studio Precision design reference. Do not build product UI here.

import QtQuick
import QtQuick.Window

Window {
    id: root
    width: 960
    height: 600
    visible: true
    title: qsTr("AtivaStage")
    color: "#0b0d10"

    Text {
        anchors.centerIn: parent
        text: qsTr("AtivaStage — prova audiovisual (Fase 1)")
        color: "#e6e8eb"
        font.pixelSize: 20
    }
}
