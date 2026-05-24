import QtQuick

Item {
    id: root
    anchors.fill: parent

    // ─────────────────────────────────────────────
    // CONFIG
    // ─────────────────────────────────────────────
    property int cols: 10
    property int rows: 3
    property real spacing: 140

    property real waveSpeed: 1.8
    property real waveFreq: 0.55
    property real waveAmp: 35.0

    property real time: 0

    property int targetCount: cols * rows
    property var gridData: []

    // ─────────────────────────────────────────────
    // TIME DRIVER
    // ─────────────────────────────────────────────
    Timer {
        interval: 16
        running: true
        repeat: true
        onTriggered: root.time += 0.016
    }

    // ─────────────────────────────────────────────
    // SIGNAL HOOKS
    // ─────────────────────────────────────────────
    Connections {
        target: cardAnimController

        function onReadyChanged() {
            console.warn("[QML] readyChanged → tryAddCards")
            Qt.callLater(tryAddCards)
        }

        function onCardAvailableChanged() {
            console.warn("[QML] cardAvailableChanged → tryAddCards")
            Qt.callLater(tryAddCards)
        }
    }

    Component.onCompleted: {
        console.warn("[QML] Component.onCompleted → initial fill attempt")
        Qt.callLater(tryAddCards)
    }

    // ─────────────────────────────────────────────
    // INCREMENTAL GRID BUILDER
    // ─────────────────────────────────────────────
    function tryAddCards() {
        console.warn("[QML] tryAddCards | current:", gridData.length, "target:", targetCount)

        var safety = 0

        while (gridData.length < targetCount && safety < 20) {
            safety++

            var c = cardAnimController.nextCard()

            if (!c || !c.id) {
                console.warn("[QML] no card available yet — stopping fill (have:", gridData.length, ")")
                return
            }

            console.warn("[QML] adding card:", c.id)

            gridData.push({
                id: c.id,
                aspectRatio: c.aspectRatio || 1.4,
                col: gridData.length % cols,
                row: Math.floor(gridData.length / cols)
            })
        }

        console.warn("[QML] tryAddCards DONE →", gridData.length)

        // force model refresh
        gridDataChanged()
    }

    // ─────────────────────────────────────────────
    // VISUAL GRID
    // ─────────────────────────────────────────────
    Repeater {
        model: root.gridData

        delegate: Item {
            id: cell

            property real cx: root.width / 2 + (modelData.col - root.cols / 2) * root.spacing
            property real cy: root.height / 2 + (modelData.row - root.rows / 2) * root.spacing

            property real dist:
                Math.sqrt(
                    Math.pow(modelData.col - root.cols / 2, 2) +
                    Math.pow(modelData.row - root.rows / 2, 2)
                )

            property real phase: dist * 0.65 + root.time * root.waveSpeed
            property real wave: Math.sin(phase * root.waveFreq) * root.waveAmp
            property real tilt: Math.cos(phase * root.waveFreq) * 18.0

            x: cx
            y: cy + wave
            rotation: tilt
            scale: 1.0 + wave * 0.002
            opacity: 0.92

            Image {
                anchors.centerIn: parent

                width: 110
                height: 154

                source: modelData.id
                    ? "image://cardanim/" + modelData.id
                    : ""

                fillMode: Image.PreserveAspectFit
                smooth: true
                asynchronous: true

                layer.enabled: true
                layer.mipmap: true
            }
        }
    }
}