import QtQuick 2.15
import QtQuick.Controls 2.15

Item {
    id: root
    anchors.fill: parent

    Image {
        id: cardImage
        anchors.fill: parent
        source: foilWidget ? foilWidget.cardImageUrl : ""
        fillMode: Image.PreserveAspectFit
    }

    ShaderEffect {
        id: foil
        anchors.fill: cardImage

        property real gradientOffset: foilWidget ? foilWidget.gradientOffset : 0
        property real highlightX: foilWidget ? foilWidget.highlightX : 0.5
        property rect artRectNorm: foilWidget ? foilWidget.artRectNormalized : Qt.rect(0, 0, 1, 1)
        property bool applyToArtOnly: foilWidget ? foilWidget.applyToArtOnly : true
        property var c0: Qt.rgba(1, 0, 1, 0.35)
        property var c1: Qt.rgba(0, 1, 1, 0.35)
        property var c2: Qt.rgba(0, 1, 0, 0.35)

        property variant source: cardImage

        fragmentShader: (Qt.version && parseInt(Qt.version.split(".")[0]) < 6)
            ? "qrc:/resources/shaders/FoilEffect.frag"
            : "qrc:/resources/shaders/FoilEffect.qsb"

        // Optional QML-driven animation (smooth gradient) if you remove C++ timer
        NumberAnimation on gradientOffset {
            from: 0
            to: 1
            duration: 2000
            loops: Animation.Infinite
            easing.type: Easing.Linear
        }
    }
}
