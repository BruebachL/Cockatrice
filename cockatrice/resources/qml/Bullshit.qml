import QtQuick

Item {
    id: root
    anchors.fill: parent

    property real time: 0.0

    Timer {
        interval: 16
        running: true
        repeat: true
        onTriggered: root.time += 0.016
    }

    ShaderEffect {
        anchors.fill: parent

        property real iTime: root.time
        property vector2d iResolution: Qt.vector2d(width, height)

        fragmentShader: "qrc:/resources/qml/shaders/rect.frag.qsb"
    }
}