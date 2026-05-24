// CardAnimBackground.qml  –  Qt 6.4+
//
// Full rendering surface.  QQuickWidget is now OPAQUE (no transparency needed)
// because this file draws the background image itself via "image://homebg/bg".
// Layers bottom→top:
//   1. Background image  (Ken Burns zoom+pan)
//   2. Dark vignette     (static gradient overlay)
//   3. Color-shift fog   (dominant-color gradient that breathes)
//   4. Bokeh circles     (large soft blobs that drift very slowly)
//   5. Dust stream       (tiny fast particles following card direction)
//   6. Motes             (medium glowing particles that twinkle)
//   7. Flying cards      (the main feature)
//   8. Card name label   (bottom-right)

import QtQuick
import QtQuick.Particles

Item {
    id: root

    // ── Readiness ─────────────────────────────────────────────────────────────
    function isReady() {
        return typeof cardAnimController !== "undefined"
            && cardAnimController !== null
            && width > 0 && height > 0
            && cardComponent !== null
            && cardComponent.status === Component.Ready
    }

    // ═══════════════════════════════════════════════════════════════════════════
    // LAYER 1 — BACKGROUND IMAGE with Ken Burns
    // ═══════════════════════════════════════════════════════════════════════════
    Item {
        id: bgLayer
        anchors.fill: parent
        clip: true
        z: -100

        Image {
            id: bgImage
            // Oversized so panning never reveals edges
            width:  parent.width  * 1.14
            height: parent.height * 1.14
            anchors.centerIn: parent

            // Cache-busting: backgroundVersion changes → new URL → fresh image
            source: (typeof cardAnimController !== "undefined" && cardAnimController !== null)
                    ? "image://homebg/bg?" + cardAnimController.backgroundVersion
                    : "image://homebg/bg?0"

            fillMode: Image.PreserveAspectCrop
            smooth:   true
            asynchronous: false   // background is pre-loaded; avoid flash

            // ── Ken Burns: slow zoom ──────────────────────────────────────────
            SequentialAnimation on scale {
                loops: Animation.Infinite
                NumberAnimation { from: 1.00; to: 1.08; duration: 18000; easing.type: Easing.InOutSine }
                NumberAnimation { from: 1.08; to: 1.00; duration: 18000; easing.type: Easing.InOutSine }
            }

            // ── Ken Burns: slow pan ───────────────────────────────────────────
            SequentialAnimation on anchors.horizontalCenterOffset {
                loops: Animation.Infinite
                NumberAnimation { from:  0; to:  30; duration: 22000; easing.type: Easing.InOutSine }
                NumberAnimation { from: 30; to: -30; duration: 22000; easing.type: Easing.InOutSine }
                NumberAnimation { from:-30; to:   0; duration: 22000; easing.type: Easing.InOutSine }
            }
            SequentialAnimation on anchors.verticalCenterOffset {
                loops: Animation.Infinite
                NumberAnimation { from:  0; to: -20; duration: 19000; easing.type: Easing.InOutSine }
                NumberAnimation { from:-20; to:  20; duration: 19000; easing.type: Easing.InOutSine }
                NumberAnimation { from: 20; to:   0; duration: 19000; easing.type: Easing.InOutSine }
            }
        }

        // Darken the background uniformly so cards pop out
        Rectangle {
            anchors.fill: parent
            color: "#000000"
            opacity: 0.38
        }
    }

    // ═══════════════════════════════════════════════════════════════════════════
    // LAYER 2 — VIGNETTE (static corner darkening)
    // ═══════════════════════════════════════════════════════════════════════════
    Item {
        anchors.fill: parent
        z: -90

        // Left + right edges
        Rectangle {
            anchors.fill: parent
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.00; color: "#88000000" }
                GradientStop { position: 0.25; color: "#00000000" }
                GradientStop { position: 0.75; color: "#00000000" }
                GradientStop { position: 1.00; color: "#88000000" }
            }
        }
        // Top + bottom edges
        Rectangle {
            anchors.fill: parent
            gradient: Gradient {
                orientation: Gradient.Vertical
                GradientStop { position: 0.00; color: "#66000000" }
                GradientStop { position: 0.20; color: "#00000000" }
                GradientStop { position: 0.80; color: "#00000000" }
                GradientStop { position: 1.00; color: "#99000000" }
            }
        }
    }

    // ═══════════════════════════════════════════════════════════════════════════
    // LAYER 3 — COLOUR-SHIFT FOG  (breathing gradient in dominant card colours)
    // ═══════════════════════════════════════════════════════════════════════════
    Rectangle {
        id: fogLayer
        anchors.fill: parent
        z: -80

        // Opacity pulses gently so the colour wash breathes
        opacity: 0.0
        SequentialAnimation on opacity {
            loops: Animation.Infinite
            NumberAnimation { to: 0.10; duration: 6000; easing.type: Easing.InOutSine }
            NumberAnimation { to: 0.04; duration: 6000; easing.type: Easing.InOutSine }
        }

        // Diagonal gradient from the two dominant colours
        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop {
                position: 0.0
                // Color comes from C++ dominant-color extraction; fall back to
                // a neutral teal so the effect is always visible.
                color: (typeof cardAnimController !== "undefined" && cardAnimController !== null
                        && cardAnimController.dominantColor1 !== undefined)
                       ? cardAnimController.dominantColor1 : "#1a5f3c"
            }
            GradientStop {
                position: 1.0
                color: (typeof cardAnimController !== "undefined" && cardAnimController !== null
                        && cardAnimController.dominantColor2 !== undefined)
                       ? cardAnimController.dominantColor2 : "#3d8a28"
            }
        }
    }

    // ═══════════════════════════════════════════════════════════════════════════
    // LAYER 4 — BOKEH CIRCLES  (large blurry blobs, almost stationary)
    // ═══════════════════════════════════════════════════════════════════════════
    Repeater {
        model: 6
        delegate: Rectangle {
            z: -70
            readonly property real baseSize: 80 + index * 55   // 80 → 355 px
            readonly property real baseX: [0.1, 0.8, 0.45, 0.2, 0.75, 0.55][index] * root.width
            readonly property real baseY: [0.3, 0.2, 0.7,  0.8, 0.65, 0.1 ][index] * root.height

            width:  baseSize
            height: baseSize
            radius: baseSize / 2
            x:      baseX - baseSize / 2
            y:      baseY - baseSize / 2

            // Alternating dominant colours
            color: (index % 2 === 0)
                   ? ((typeof cardAnimController !== "undefined"
                       && cardAnimController !== null
                       && cardAnimController.dominantColor1 !== undefined)
                      ? cardAnimController.dominantColor1 : "#1a5f3c")
                   : ((typeof cardAnimController !== "undefined"
                       && cardAnimController !== null
                       && cardAnimController.dominantColor2 !== undefined)
                      ? cardAnimController.dominantColor2 : "#3d8a28")

            // layer.mipmap gives a naturally blurred, out-of-focus look
            layer.enabled: true
            layer.mipmap:  true

            // Breathe: opacity pulses on a per-blob offset cycle
            SequentialAnimation on opacity {
                loops: Animation.Infinite
                NumberAnimation { to: 0.07; duration: 4000 + index * 700; easing.type: Easing.InOutSine }
                NumberAnimation { to: 0.02; duration: 4000 + index * 700; easing.type: Easing.InOutSine }
            }

            // Drift: slow figure-8-ish movement
            SequentialAnimation on x {
                loops: Animation.Infinite
                NumberAnimation { to: baseX + 40; duration: 9000 + index * 1300; easing.type: Easing.InOutSine }
                NumberAnimation { to: baseX - 40; duration: 9000 + index * 1300; easing.type: Easing.InOutSine }
            }
            SequentialAnimation on y {
                loops: Animation.Infinite
                NumberAnimation { to: baseY + 30; duration: 7000 + index * 1700; easing.type: Easing.InOutSine }
                NumberAnimation { to: baseY - 30; duration: 7000 + index * 1700; easing.type: Easing.InOutSine }
            }
        }
    }

    // ═══════════════════════════════════════════════════════════════════════════
    // LAYER 5 — DUST STREAM  (fast tiny particles following card direction)
    // ═══════════════════════════════════════════════════════════════════════════
    ParticleSystem {
        id: dustSystem
        anchors.fill: parent
        z: -60
        running: isReady() && cardAnimController.enableParticles
        paused:  !root.visible

        ItemParticle {
            delegate: Rectangle {
                readonly property real sz: 0.8 + Math.random() * 1.8
                width:   sz;   height: sz
                radius:  sz / 2
                color:   "white"
                opacity: 0.15 + Math.random() * 0.25
            }
        }

        Emitter {
            anchors.fill: parent
            emitRate:          18
            lifeSpan:          5500
            lifeSpanVariation: 2500
            velocity: AngleDirection {
                angle:              isReady() ? cardAnimController.angleDeg : 0
                angleVariation:     35
                magnitude:          isReady() ? cardAnimController.minSpeed * 0.55 : 15
                magnitudeVariation: isReady() ? cardAnimController.minSpeed * 0.30 : 8
            }
            size: 1.5; sizeVariation: 1; endSize: 0
        }
    }

    // ═══════════════════════════════════════════════════════════════════════════
    // LAYER 6 — MOTES  (larger glowing particles that twinkle)
    // ═══════════════════════════════════════════════════════════════════════════
    ParticleSystem {
        id: moteSystem
        anchors.fill: parent
        z: -55
        running: isReady() && cardAnimController.enableParticles
        paused:  !root.visible

        ItemParticle {
            delegate: Item {
                readonly property real sz: 3.5 + Math.random() * 5.5
                width: sz * 3; height: sz * 3   // extra room for the glow

                // Soft halo
                Rectangle {
                    anchors.centerIn: parent
                    width:   parent.sz * 3
                    height:  width
                    radius:  width / 2
                    color:   "white"
                    opacity: 0
                    layer.enabled: true
                    layer.mipmap:  true

                    SequentialAnimation on opacity {
                        running: true; loops: Animation.Infinite
                        NumberAnimation { to: 0.18; duration: 900 + Math.random() * 1400; easing.type: Easing.InOutSine }
                        NumberAnimation { to: 0.0;  duration: 900 + Math.random() * 1400; easing.type: Easing.InOutSine }
                    }
                }

                // Bright core
                Rectangle {
                    anchors.centerIn: parent
                    width:   parent.sz
                    height:  parent.sz
                    radius:  width / 2
                    color:   "white"
                    opacity: 0

                    SequentialAnimation on opacity {
                        running: true; loops: Animation.Infinite
                        // Offset phase so core & halo pulse slightly out-of-sync
                        PauseAnimation { duration: Math.random() * 600 }
                        NumberAnimation { to: 0.85; duration: 700 + Math.random() * 1000; easing.type: Easing.InOutSine }
                        NumberAnimation { to: 0.0;  duration: 700 + Math.random() * 1000; easing.type: Easing.InOutSine }
                    }
                }
            }
        }

        Emitter {
            anchors.fill: parent
            emitRate:          2.5
            lifeSpan:          9000
            lifeSpanVariation: 5000
            velocity: AngleDirection {
                // Drift mostly in card direction but with wide scatter
                angle:              isReady() ? cardAnimController.angleDeg : 0
                angleVariation:     80
                magnitude:          isReady() ? cardAnimController.minSpeed * 0.18 : 6
                magnitudeVariation: isReady() ? cardAnimController.minSpeed * 0.12 : 4
            }
            size: 12; sizeVariation: 8; endSize: 0
        }
    }

    // ═══════════════════════════════════════════════════════════════════════════
    // LAYER 7 — FLYING CARDS
    // ═══════════════════════════════════════════════════════════════════════════
    property var  activeCards:   []
    property var  cardComponent: null
    property bool started:       false

    Component.onCompleted: {
        console.warn("CardAnimBackground: onCompleted  size:", width, "×", height)
        cardComponent = Qt.createComponent("AnimatedCard.qml")
        if (cardComponent.status === Component.Error)
            console.error("AnimatedCard.qml load error:", cardComponent.errorString())
        else
            console.warn("AnimatedCard.qml ready")
        tryStart()
    }

    onWidthChanged:  { if (width  > 0) tryStart() }
    onHeightChanged: { if (height > 0) tryStart() }

    Connections {
        target: cardAnimController
        function onReadyChanged() { root.tryStart() }
        function onConfigChanged() {
            spawnTimer.interval = Math.max(150,
                Math.round(1000 / cardAnimController.cardsPerSecond))
        }
        // Reload background when backgroundVersion bumps
        function onBackgroundVersionChanged() {
            bgImage.source = "image://homebg/bg?" + cardAnimController.backgroundVersion
        }
    }

    function tryStart() {
        if (started || !isReady()) return
        started = true
        console.warn("CardAnimBackground: starting — burst of",
                     Math.min(Math.floor(cardAnimController.maxCards * 0.6), 6))
        Qt.callLater(initialBurst)
    }

    function initialBurst() {
        var count = Math.min(Math.floor(cardAnimController.maxCards * 0.6), 6)
        for (var i = 0; i < count; i++) spawnCard(true)
    }

    Timer {
        id: spawnTimer
        interval: isReady()
                  ? Math.max(150, Math.round(1000 / cardAnimController.cardsPerSecond))
                  : 1000
        running:  root.started
        repeat:   true
        onTriggered: {
            if (root.activeCards.length < cardAnimController.maxCards)
                root.spawnCard(false)
        }
    }

    function spawnCard(midLife) {
        if (!isReady()) return

        var data = cardAnimController.nextCard()
        if (!data || !data.id) return

        var ctrl     = cardAnimController
        var angleRad = ctrl.angleDeg * Math.PI / 180.0
        var axisX    = Math.cos(angleRad)
        var axisY    = Math.sin(angleRad)
        var perpX    = -Math.sin(angleRad)
        var perpY    =  Math.cos(angleRad)

        var speed       = ctrl.minSpeed + Math.random() * (ctrl.maxSpeed - ctrl.minSpeed)
        var lifetime    = ctrl.minLifetimeSecs + Math.random() * (ctrl.maxLifetimeSecs - ctrl.minLifetimeSecs)
        var scale       = ctrl.minScale + Math.random() * (ctrl.maxScale - ctrl.minScale)
        var initialRot  = (Math.random() * 2 - 1) * ctrl.maxInitialRotDeg
        var rotSpd      = (Math.random() * 2 - 1) * ctrl.maxRotSpeedDeg
        var turbPhase   = Math.random() * 2 * Math.PI

        var diagonal    = Math.sqrt(root.width * root.width + root.height * root.height)
        var perpOffset  = (Math.random() - 0.5) * diagonal

        var cardW       = ctrl.baseCardWidth * scale
        var cardH       = cardW * data.aspectRatio
        var totalDist   = speed * lifetime

        var anchorX     = root.width  * 0.5 + perpX * perpOffset
        var anchorY     = root.height * 0.5 + perpY * perpOffset

        var initP       = midLife ? (Math.random() * 0.60) : 0.0
        var startX      = anchorX - axisX * totalDist * (0.55 - initP) - cardW * 0.5
        var startY      = anchorY - axisY * totalDist * (0.55 - initP) - cardH * 0.5

        var capturedId  = data.id
        var obj = cardComponent.createObject(root, {
            cardId:          capturedId,
            cardAspectR:     data.aspectRatio,
            startX:          startX,
            startY:          startY,
            axisX:           axisX,
            axisY:           axisY,
            perpX:           perpX,
            perpY:           perpY,
            cardSpeed:       speed,
            lifetimeMs:      Math.round(lifetime * 1000),
            initProgress:    initP,
            cardScale:       scale,
            initRotation:    initialRot,
            rotSpeed:        rotSpd,
            turbAmp:         ctrl.turbAmplitude,
            turbFreq:        ctrl.turbFrequency,
            turbPhase:       turbPhase,
            peakOpacity:     ctrl.peakOpacity,
            fadeInFrac:      ctrl.fadeInFrac,
            fadeOutFrac:     ctrl.fadeOutFrac,
            trailLen:        ctrl.trailLength,
            trailIntervalMs: ctrl.trailIntervalMs,
            trailOpDecay:    ctrl.trailOpacityDecay,
            trailScDecay:    ctrl.trailScaleDecay,
            showGlow:        ctrl.enableGlow,
            glowOp:          ctrl.glowOpacity,
            showShadow:      ctrl.enableShadow,
            shadowOff:       ctrl.shadowOffsetPx,
        })

        if (!obj) {
            console.error("createObject failed for", capturedId)
            cardAnimController.returnCard(capturedId)
            return
        }

        var capturedObj = obj
        obj.cardDone.connect(function() {
            var idx = root.activeCards.indexOf(capturedObj)
            if (idx >= 0) root.activeCards.splice(idx, 1)
            cardAnimController.returnCard(capturedId)
            capturedObj.destroy()
        })
        root.activeCards.push(obj)
    }

    // ═══════════════════════════════════════════════════════════════════════════
    // LAYER 8 — CARD NAME LABEL  (bottom-right corner)
    // ═══════════════════════════════════════════════════════════════════════════
    Item {
        id: cardNameLabel
        anchors.right:  parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 18
        z: 200

        visible: (typeof cardAnimController !== "undefined"
                  && cardAnimController !== null
                  && cardAnimController.cardNameText !== ""
                  && cardAnimController.cardNameText !== undefined)

        // Measure text to size the pill
        Text {
            id: nameText
            text: (typeof cardAnimController !== "undefined"
                   && cardAnimController !== null
                   && cardAnimController.cardNameText !== undefined)
                  ? cardAnimController.cardNameText : ""
            font.pointSize: 13
            font.bold: true
            color: "white"
            anchors.centerIn: parent.pillBg
        }

        // Pill background
        Rectangle {
            id: pillBg
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            width:  nameText.implicitWidth  + 22
            height: nameText.implicitHeight + 14
            color:  "#bb000000"
            radius: 8

            Text {
                anchors.centerIn: parent
                text:      nameText.text
                font:      nameText.font
                color:     "white"
            }
        }
    }
}
