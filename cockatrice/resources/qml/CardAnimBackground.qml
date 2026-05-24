// CardAnimBackground.qml – Qt 6.4+

import QtQuick
import QtQuick.Particles

Item {
    id: root

    Rectangle {
        anchors.fill: parent
        color: "red"
        opacity: 0.3
    }

    // ── Controller injected from C++ ─────────────────────────
    property QtObject cardAnimController: null

    onCardAnimControllerChanged: {
        console.warn("Controller changed:", cardAnimController)

        tryStart()
    }

    onWidthChanged: {
        console.warn("size →", width, "×", height)
        tryStart()
    }

    onHeightChanged: {
        console.warn("size →", width, "×", height)
        tryStart()
    }

    // ── READY CHECK (DO NOT rely on signals) ─────────────────
    function isReady() {
        return cardAnimController !== null &&
            width > 0 &&
            height > 0 &&
            cardComponent &&
            cardComponent.status === Component.Ready
    }

    // ── SINGLE START GATE ────────────────────────────────────
    property bool started: false

    function tryStart() {
        console.warn("tryStart() →",
            "started=", started,
            "controller=", cardAnimController,
            "size=", width, "×", height)

        if (started)
            return

        if (!isReady())
            return

        started = true
        console.warn("tryStart() → STARTING INITIAL BURST")

        Qt.callLater(initialBurst)
    }

    // ── Particle system ──────────────────────────────────────
    ParticleSystem {
        id: particleSys
        anchors.fill: parent

        running: root.cardAnimController &&
            root.cardAnimController.enableParticles

        paused: !root.visible

        ItemParticle {
            delegate: Rectangle {
                width: 1 + Math.random() * 2.5
                height: width
                radius: width * 0.5
                color: "white"
                opacity: 0.2
            }
        }

        Emitter {
            anchors.fill: parent
            emitRate: 10
            lifeSpan: 7000
            lifeSpanVariation: 4000

            velocity: AngleDirection {
                angle: cardAnimController ? cardAnimController.angleDeg : 0
                magnitude: cardAnimController ? cardAnimController.minSpeed * 0.3 : 0
                angleVariation: 28
                magnitudeVariation: 5
            }

            size: 2
            sizeVariation: 1
            endSize: 0
        }
    }

    // ── State ────────────────────────────────────────────────
    property var activeCards: []
    property var cardComponent: null

    Component.onCompleted: {
        console.warn("Component.onCompleted")

        console.warn("loading AnimatedCard.qml")
        cardComponent = Qt.createComponent("AnimatedCard.qml")

        console.warn("AnimatedCard status =", cardComponent.status)

        if (cardComponent.status === Component.Error) {
            console.error(cardComponent.errorString())
            return
        }

        tryStart()
    }

    function initialBurst() {
        var count = Math.min(
            Math.floor(cardAnimController.maxCards * 0.6),
            6
        )

        console.warn("initialBurst:", count)

        for (var i = 0; i < count; i++)
            spawnCard(true)
    }

    // ── Timer ────────────────────────────────────────────────
    Timer {
        id: spawnTimer
        interval: root.cardAnimController
            ? Math.max(150, Math.round(1000 / root.cardAnimController.cardsPerSecond))
            : 1000

        running: root.isReady()
        repeat: true

        onTriggered: {
            if (root.activeCards.length < root.cardAnimController.maxCards)
                root.spawnCard(false)
        }
    }

    Connections {
        target: root.cardAnimController

        function onConfigChanged() {
            spawnTimer.interval = Math.max(
                150,
                Math.round(1000 / root.cardAnimController.cardsPerSecond)
            )

            console.warn("config updated → interval:", spawnTimer.interval)

            tryStart()
        }
    }

    Connections {
        target: cardAnimController
        function onReadyChanged() {
            console.warn("Controller READY signal received")
            tryStart()
        }
    }

    // ── Spawn ────────────────────────────────────────────────
    function spawnCard(midLife) {
        console.warn("spawnCard()")

        if (!isReady()) {
            console.warn("spawnCard blocked (not ready)")
            return
        }

        var data = cardAnimController.nextCard()
        if (!data || !data.id) {
            console.warn("nextCard empty")
            return
        }

        var obj = cardComponent.createObject(root, {
            cardId: data.id
        })

        if (!obj) {
            console.error("createObject failed")
            cardAnimController.returnCard(data.id)
            return
        }

        activeCards.push(obj)

        obj.cardDone.connect(function() {
            var idx = activeCards.indexOf(obj)
            if (idx >= 0)
                activeCards.splice(idx, 1)

            cardAnimController.returnCard(data.id)
            obj.destroy()
        })

        console.warn("spawned:", data.id, "active:", activeCards.length)
    }
}