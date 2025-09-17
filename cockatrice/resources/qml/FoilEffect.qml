import QtQuick 2.15
import QtQuick.Controls 2.15

Item {
    id: root
    anchors.fill: parent

    // The image is the texture for the shader
    Image {
        id: cardImage
        objectName: "cardImage" // important for findChild in C++
        visible: false           // hide the image, only use it as a texture
        source: ""
    }

    ShaderEffect {
        anchors.fill: parent
        property variant source: cardImage
        property real gradientOffset: foilWidget.gradientOffset
        property real highlightX: foilWidget.highlightX
        property rect artRectNorm: foilWidget.artRectNormalized
        property bool applyToArtOnly: foilWidget.applyToArtOnly
        property var c0: Qt.rgba(1, 0, 1, 0.35)
        property var c1: Qt.rgba(0, 1, 1, 0.35)
        property var c2: Qt.rgba(0, 1, 0, 0.35)

        fragmentShader: "qrc:/resources/shaders/FoilEffect.qsb"
    }
}
