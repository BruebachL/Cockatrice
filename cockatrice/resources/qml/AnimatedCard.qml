// AnimatedCard.qml  –  Qt 6.4+  (NO QtQuick.Effects, NO inline GLSL)
//
// Effects strategy for Qt 6.4 compatibility:
//   Shadow  → dark Rectangle offset behind the card
//   Glow    → same Image rendered at 1.25× scale with low opacity underneath
//   Trail   → plain Images with decaying opacity (no shader blur needed)
//
// If you upgrade to Qt 6.5, swap the shadow/glow Items for a single
// MultiEffect { shadowEnabled: true; blurEnabled: true } on the main Image.

import QtQuick

Item {
    id: card
    anchors.fill: parent   // full-screen canvas; children use absolute coords

    // ── Spawn parameters ──────────────────────────────────────────────────────
    // NOT 'required' — these are set via createObject({...}) property map.
    property string cardId:       ""
    property real   cardAspectR:  1.4
    property real   startX:       0
    property real   startY:       0
    property real   axisX:        1.0
    property real   axisY:        0.0
    property real   perpX:        0.0
    property real   perpY:        1.0
    property real   cardSpeed:    50
    property int    lifetimeMs:   10000
    property real   initProgress: 0.0
    property real   cardScale:    1.0
    property real   initRotation: 0
    property real   rotSpeed:     5       // deg / sec
    property real   turbAmp:      20
    property real   turbFreq:     0.3
    property real   turbPhase:    0
    property real   peakOpacity:  0.85
    property real   fadeInFrac:   0.15
    property real   fadeOutFrac:  0.22
    property int    trailLen:     4
    property int    trailIntervalMs: 220
    property real   trailOpDecay: 0.55
    property real   trailScDecay: 0.90
    property bool   showGlow:     true
    property real   glowOp:       0.20
    property bool   showShadow:   true
    property real   shadowOff:    4

    signal cardDone()

    // ── Live state ────────────────────────────────────────────────────────────
    property real progress:     initProgress
    property real posX:         startX
    property real posY:         startY
    property real currentRot:   initRotation
    property real currentOp:    0.0
    property real currentScale: cardScale

    // Base card pixel dimensions (at cardScale == 1)
    readonly property real cw: 110.0 * cardScale
    readonly property real ch: cw * cardAspectR

    // Scaled dimensions for the current frame
    readonly property real fw: cw * (currentScale / cardScale)
    readonly property real fh: ch * (currentScale / cardScale)

    // ── Main animation driver ─────────────────────────────────────────────────
    NumberAnimation on progress {
        id: progressAnim
        from:     card.initProgress
        to:       1.0
        duration: Math.max(1, Math.round(card.lifetimeMs * (1.0 - card.initProgress)))
        running:  true
        easing.type: Easing.Linear
        onFinished: {
            console.warn("AnimatedCard: card", card.cardId, "finished")
            card.cardDone()
        }
    }

    NumberAnimation on currentRot {
        from:     card.initRotation
        to:       card.initRotation + card.rotSpeed * (card.lifetimeMs / 1000.0)
        duration: card.lifetimeMs
        running:  true
        easing.type: Easing.Linear
    }

    onProgressChanged: {
        var totalDist = cardSpeed * (lifetimeMs / 1000.0)
        var t = progress
        var turb = turbAmp * Math.sin(t * turbFreq * 2.0 * Math.PI + turbPhase)

        posX = startX + axisX * totalDist * t + perpX * turb
        posY = startY + axisY * totalDist * t + perpY * turb

        // Smoothstep opacity envelope + scale-pop on fade-in
        if (t < fadeInFrac) {
            var inT = fadeInFrac > 0 ? t / fadeInFrac : 1.0
            currentOp    = peakOpacity * inT * inT * (3.0 - 2.0 * inT)
            currentScale = cardScale   * (0.65 + 0.35 * inT)
        } else if (t > 1.0 - fadeOutFrac) {
            var outT = fadeOutFrac > 0 ? (1.0 - t) / fadeOutFrac : 1.0
            currentOp    = peakOpacity * outT * outT * (3.0 - 2.0 * outT)
            currentScale = cardScale
        } else {
            currentOp    = peakOpacity
            currentScale = cardScale
        }
    }

    // ── Trail position sampling ───────────────────────────────────────────────
    property var trailX:   []
    property var trailY:   []
    property var trailRot: []

    Timer {
        interval: card.trailLen > 0 ? card.trailIntervalMs : 9999999
        running:  card.trailLen > 0
        repeat:   true
        onTriggered: {
            // Prepend current position; cap length
            var tx = [card.posX].concat(card.trailX)
            var ty = [card.posY].concat(card.trailY)
            var tr = [card.currentRot].concat(card.trailRot)
            var maxLen = card.trailLen + 1
            card.trailX   = tx.slice(0, maxLen)
            card.trailY   = ty.slice(0, maxLen)
            card.trailRot = tr.slice(0, maxLen)
        }
    }

    // ── Trail ghost Images ────────────────────────────────────────────────────
    // Plain Images with decaying opacity — works on all Qt 6.4+ backends.
    // For a blur effect on Qt 6.5+, wrap each Image in an Item with
    // layer.enabled:true and a MultiEffect { blurEnabled:true; blur: … }
    Repeater {
        model: card.trailLen

        delegate: Image {
            // 'index' is an implicit context property in Repeater delegates
            readonly property int  gi:  index   // 0 = most recent ghost
            readonly property real gx:  card.trailX.length  > gi + 1 ? card.trailX[gi + 1]  : card.posX
            readonly property real gy:  card.trailY.length  > gi + 1 ? card.trailY[gi + 1]  : card.posY
            readonly property real gr:  card.trailRot.length > gi + 1 ? card.trailRot[gi + 1] : card.currentRot
            readonly property real gsc: card.currentScale * Math.pow(card.trailScDecay, gi + 1)
            readonly property real gop: card.currentOp    * Math.pow(card.trailOpDecay,  gi + 1)

            x:        gx
            y:        gy
            width:    card.cw * gsc / card.cardScale
            height:   card.ch * gsc / card.cardScale
            rotation: gr
            opacity:  gop
            z:        -(gi + 1)

            source:       "image://cardanim/" + card.cardId
            fillMode:     Image.Stretch
            smooth:       true
            asynchronous: true
        }
    }

    // ── Shadow (Qt 6.4 compatible — dark rounded rect) ───────────────────────
    Rectangle {
        visible:  card.showShadow
        x:        card.posX + card.shadowOff * 1.2
        y:        card.posY + card.shadowOff * 1.2
        width:    card.fw
        height:   card.fh
        rotation: card.currentRot
        color:    "#000000"
        opacity:  card.currentOp * 0.38
        radius:   4
        z:        0
    }

    // ── Glow halo (Qt 6.4 compatible — oversized semi-transparent copy) ───────
    // Rendered behind the card. The larger, desaturated duplicate creates a
    // convincing luminous fringe without any shader math.
    Image {
        visible:  card.showGlow
        readonly property real pad: card.fw * 0.18
        x:        card.posX - pad * 0.5
        y:        card.posY - pad * 0.5
        width:    card.fw + pad
        height:   card.fh + pad
        rotation: card.currentRot
        opacity:  card.currentOp * card.glowOp * 2.2
        z:        1

        source:       "image://cardanim/" + card.cardId
        fillMode:     Image.Stretch
        smooth:       true
        asynchronous: true

        // Soft-scale the glow layer via QML's built-in layer compositing
        layer.enabled: true
        layer.smooth:  true
        // mipmap on the layer gives a naturally blurred, softer look at no cost
        layer.mipmap:  true
    }

    // ── Main card image ───────────────────────────────────────────────────────
    Image {
        id:       mainImg
        x:        card.posX
        y:        card.posY
        width:    card.fw
        height:   card.fh
        rotation: card.currentRot
        opacity:  card.currentOp
        z:        2

        source:       "image://cardanim/" + card.cardId
        fillMode:     Image.Stretch
        smooth:       true
        asynchronous: true

        onStatusChanged: {
            if (status === Image.Error)
                console.warn("AnimatedCard: image load failed for", card.cardId, source)
            else if (status === Image.Ready)
                console.warn("AnimatedCard: image ready for card", card.cardId)
        }
    }
}
