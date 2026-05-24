// AnimatedCard.qml  –  Qt 6.4+
//
// KEY: Do NOT use "NumberAnimation on property {}" shorthand for any
// property that is also set via createObject({...}).  The shorthand
// animation starts at component instantiation, *before* createObject
// commits the property map, so the engine raises "Could not set initial
// property X" and the card never moves.
//
// Instead: declare animations with an explicit target+property and call
// .start() inside Component.onCompleted, by which point all createObject
// properties are committed.

import QtQuick

Item {
    id: card
    anchors.fill: parent   // full-screen canvas; children use absolute coords

    // ── Spawn parameters ─────────────────────────────────────────────────────
    property string cardId:          ""
    property real   cardAspectR:     1.4
    property real   startX:          0
    property real   startY:          0
    property real   axisX:           1.0
    property real   axisY:           0.0
    property real   perpX:           0.0
    property real   perpY:           1.0
    property real   cardSpeed:       50
    property int    lifetimeMs:      10000
    property real   initProgress:    0.0
    property real   cardScale:       1.0
    property real   initRotation:    0
    property real   rotSpeed:        5
    property real   turbAmp:         20
    property real   turbFreq:        0.3
    property real   turbPhase:       0
    property real   peakOpacity:     0.85
    property real   fadeInFrac:      0.15
    property real   fadeOutFrac:     0.22
    property int    trailLen:        4
    property int    trailIntervalMs: 220
    property real   trailOpDecay:    0.55
    property real   trailScDecay:    0.90
    property bool   showGlow:        true
    property real   glowOp:          0.20
    property bool   showShadow:      true
    property real   shadowOff:       4

    signal cardDone()

    // ── Live state ────────────────────────────────────────────────────────────
    property real progress:     0.0
    property real posX:         0.0
    property real posY:         0.0
    property real currentRot:   0.0
    property real currentOp:    0.0
    property real currentScale: 1.0

    readonly property real cw: 110.0 * cardScale
    readonly property real ch: cw * cardAspectR
    readonly property real fw: cw * (currentScale / Math.max(cardScale, 0.001))
    readonly property real fh: ch * (currentScale / Math.max(cardScale, 0.001))

    // ── Explicit animations (NOT the "on property" shorthand) ─────────────────
    NumberAnimation {
        id: progressAnim
        target: card
        property: "progress"
        easing.type: Easing.Linear
        onFinished: {
            console.warn("AnimatedCard: done →", card.cardId)
            card.cardDone()
        }
    }

    NumberAnimation {
        id: rotAnim
        target: card
        property: "currentRot"
        easing.type: Easing.Linear
    }

    // ── Start animations AFTER createObject properties are committed ──────────
    Component.onCompleted: {
        // Seed live state from (now-committed) spawn parameters
        progress    = initProgress
        currentRot  = initRotation
        currentScale = cardScale

        var remaining = 1.0 - initProgress
        progressAnim.from     = initProgress
        progressAnim.to       = 1.0
        progressAnim.duration = Math.max(1, Math.round(lifetimeMs * remaining))
        progressAnim.start()

        rotAnim.from     = initRotation
        rotAnim.to       = initRotation + rotSpeed * (lifetimeMs / 1000.0)
        rotAnim.duration = Math.max(1, lifetimeMs)
        rotAnim.start()

        console.warn("AnimatedCard: started id=", cardId,
                     "startX=", startX, "startY=", startY,
                     "speed=", cardSpeed, "lifetime=", lifetimeMs,
                     "initProgress=", initProgress)
    }

    // ── Position + opacity driven by progress ─────────────────────────────────
    onProgressChanged: {
        var totalDist = cardSpeed * (lifetimeMs / 1000.0)
        var t = progress
        var turb = turbAmp * Math.sin(t * turbFreq * 2.0 * Math.PI + turbPhase)

        posX = startX + axisX * totalDist * t + perpX * turb
        posY = startY + axisY * totalDist * t + perpY * turb

        var inF  = Math.max(fadeInFrac,  0.001)
        var outF = Math.max(fadeOutFrac, 0.001)

        if (t < inF) {
            var inT  = t / inF
            currentOp    = peakOpacity * inT * inT * (3.0 - 2.0 * inT)
            currentScale = cardScale * (0.65 + 0.35 * inT)
        } else if (t > 1.0 - outF) {
            var outT = (1.0 - t) / outF
            currentOp    = peakOpacity * outT * outT * (3.0 - 2.0 * outT)
            currentScale = cardScale
        } else {
            currentOp    = peakOpacity
            currentScale = cardScale
        }
    }

    // ── Trail sampling ────────────────────────────────────────────────────────
    property var trailX:   []
    property var trailY:   []
    property var trailRot: []

    Timer {
        interval: card.trailLen > 0 ? card.trailIntervalMs : 9999999
        running:  card.trailLen > 0
        repeat:   true
        onTriggered: {
            card.trailX   = [card.posX].concat(card.trailX).slice(0, card.trailLen + 1)
            card.trailY   = [card.posY].concat(card.trailY).slice(0, card.trailLen + 1)
            card.trailRot = [card.currentRot].concat(card.trailRot).slice(0, card.trailLen + 1)
        }
    }

    // ── Trail ghosts ──────────────────────────────────────────────────────────
    Repeater {
        model: card.trailLen
        delegate: Image {
            readonly property int  gi:  index
            readonly property real gx:  card.trailX.length  > gi + 1 ? card.trailX[gi + 1]  : card.posX
            readonly property real gy:  card.trailY.length  > gi + 1 ? card.trailY[gi + 1]  : card.posY
            readonly property real gr:  card.trailRot.length > gi + 1 ? card.trailRot[gi + 1] : card.currentRot
            readonly property real gsc: card.currentScale * Math.pow(card.trailScDecay, gi + 1)
            readonly property real gop: card.currentOp    * Math.pow(card.trailOpDecay,  gi + 1)

            x: gx; y: gy
            width:    card.cw * gsc / Math.max(card.cardScale, 0.001)
            height:   card.ch * gsc / Math.max(card.cardScale, 0.001)
            rotation: gr
            opacity:  gop
            z:        -(gi + 1)

            source:       "image://cardanim/" + card.cardId
            fillMode:     Image.Stretch
            smooth:       true
            asynchronous: true
        }
    }

    // ── Shadow ────────────────────────────────────────────────────────────────
    Rectangle {
        visible:  card.showShadow
        x:        card.posX + card.shadowOff * 1.5
        y:        card.posY + card.shadowOff * 1.5
        width:    card.fw
        height:   card.fh
        rotation: card.currentRot
        color:    "#000000"
        opacity:  card.currentOp * 0.40
        radius:   5
        z:        0
    }

    // ── Glow halo ─────────────────────────────────────────────────────────────
    Image {
        visible:  card.showGlow
        readonly property real pad: card.fw * 0.20
        x:        card.posX - pad * 0.5
        y:        card.posY - pad * 0.5
        width:    card.fw + pad
        height:   card.fh + pad
        rotation: card.currentRot
        opacity:  card.currentOp * card.glowOp * 2.0
        z:        1
        source:   "image://cardanim/" + card.cardId
        fillMode: Image.Stretch
        smooth:   true
        asynchronous: true
        layer.enabled: true
        layer.mipmap:  true    // cheap soft-focus via mip downsampling
    }

    // ── Main card ─────────────────────────────────────────────────────────────
    Image {
        x:        card.posX
        y:        card.posY
        width:    card.fw
        height:   card.fh
        rotation: card.currentRot
        opacity:  card.currentOp
        z:        2
        source:   "image://cardanim/" + card.cardId
        fillMode: Image.Stretch
        smooth:   true
        asynchronous: true
        onStatusChanged: {
            if (status === Image.Error)
                console.warn("AnimatedCard: image error for", card.cardId)
        }
    }
}
