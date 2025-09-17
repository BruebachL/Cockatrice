import QtQuick 2.15
import QtQuick.Controls 2.15

Rectangle {
    width: 400
    height: 300
    color: "black"

    // The image we want to shade
    Image {
        id: baseImage
        anchors.fill: parent
        source: "image://pixmapProvider/current"
        cache: false
        fillMode: Image.PreserveAspectFit
        visible: false
    }

    // Wrap image so the shader can sample it
    ShaderEffectSource {
        id: imgSource
        sourceItem: baseImage
        hideSource: true
        live: true
    }

    // ShaderEffect applying hue rotation
    ShaderEffect {
        anchors.fill: parent

        // Property names MUST match the uniforms in the QSB
        property real uTime: 0
        property variant source: imgSource

        // Baked shader
        fragmentShader: "qrc:/resources/shaders/HueRotateSimple.qsb"

        // Animate time
        NumberAnimation on uTime {
            from: 0
            to: 1000
            duration: 600000
            loops: Animation.Infinite
        }
    }
}
