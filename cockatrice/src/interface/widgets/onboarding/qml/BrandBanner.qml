import QtQuick

// Two stacked instances of the same shader. BannerShaderConfig writes new
// mode/speed/seed into whichever bank (A/B) is currently invisible, then
// flips frontIsA -- the Behavior below does the actual crossfade, nothing
// in C++ animates this.
Item {
    ShaderEffect {
        id: effectA
        anchors.fill: parent
        opacity: bannerConfig.frontIsA ? 1.0 : 0.0
        Behavior on opacity { NumberAnimation { duration: 450; easing.type: Easing.InOutCubic } }

        property real iTime: bannerConfig.time
        property real uAspect: bannerConfig.aspect
        property real uMode: bannerConfig.modeA
        property real uSpeed: bannerConfig.speedA
        property real uSeed: bannerConfig.seedA
        property vector4d uColorA: Qt.vector4d(bannerConfig.colorA.r, bannerConfig.colorA.g,
            bannerConfig.colorA.b, 1.0)
        property vector4d uColorB: Qt.vector4d(bannerConfig.colorB.r, bannerConfig.colorB.g,
            bannerConfig.colorB.b, 1.0)
        property vector4d uAccent: Qt.vector4d(bannerConfig.accent.r, bannerConfig.accent.g,
            bannerConfig.accent.b, 1.0)

        fragmentShader: "qrc:/onboarding/shaders/brand_banner.frag.qsb"
    }

    ShaderEffect {
        id: effectB
        anchors.fill: parent
        opacity: bannerConfig.frontIsA ? 0.0 : 1.0
        Behavior on opacity { NumberAnimation { duration: 450; easing.type: Easing.InOutCubic } }

        property real iTime: bannerConfig.time
        property real uAspect: bannerConfig.aspect
        property real uMode: bannerConfig.modeB
        property real uSpeed: bannerConfig.speedB
        property real uSeed: bannerConfig.seedB
        property vector4d uColorA: Qt.vector4d(bannerConfig.colorA.r, bannerConfig.colorA.g,
            bannerConfig.colorA.b, 1.0)
        property vector4d uColorB: Qt.vector4d(bannerConfig.colorB.r, bannerConfig.colorB.g,
            bannerConfig.colorB.b, 1.0)
        property vector4d uAccent: Qt.vector4d(bannerConfig.accent.r, bannerConfig.accent.g,
            bannerConfig.accent.b, 1.0)

        fragmentShader: "qrc:/onboarding/shaders/brand_banner.frag.qsb"
    }
}