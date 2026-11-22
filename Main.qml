import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: window
    width: 980
    height: 720
    visible: true
    title: "Qt 局域网 WebSocket 服务端"
    color: "#eef2f5"

    property var logLines: []

    Connections {
        target: backend
        function onLogMessage(message) {
            window.logLines = [message].concat(window.logLines).slice(0, 200)
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 14

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 100
            radius: 14
            color: "#173f35"
            RowLayout {
                anchors.fill: parent
                anchors.margins: 20
                ColumnLayout {
                    Label { text: "Qt 局域网通信服务"; color: "white"; font.pixelSize: 24; font.bold: true }
                    Label { text: "Qt 6.5.3 · HTTP 网页服务 + WebSocket 双向通信"; color: "#c9e4d9" }
                }
                Item { Layout.fillWidth: true }
                Label {
                    text: backend.running ? "● 运行中" : "● 已停止"
                    color: backend.running ? "#8df0b3" : "#ffb4a9"
                    font.pixelSize: 18
                    font.bold: true
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 10
            Button {
                text: "启动服务"
                enabled: !backend.running
                onClicked: backend.start()
            }
            Button {
                text: "停止服务"
                enabled: backend.running
                onClicked: backend.stop()
            }
            Label { text: backend.status; color: "#344054"; Layout.fillWidth: true; elide: Text.ElideRight }
            Label { text: "在线设备：" + backend.clientCount; font.bold: true; color: "#176b45" }
        }

        GroupBox {
            title: "局域网访问地址"
            Layout.fillWidth: true
            ColumnLayout {
                anchors.fill: parent
                Label { text: "网页地址（手机/电脑浏览器打开）"; font.bold: true }
                Repeater {
                    model: backend.localAddresses
                    delegate: RowLayout {
                        Layout.fillWidth: true
                        Label { text: "http://" + modelData + ":8080"; font.pixelSize: 16; color: "#176b45"; Layout.fillWidth: true }
                        Button { text: "复制"; onClicked: backend.copyText("http://" + modelData + ":8080") }
                    }
                }
                Label { text: "WebSocket：ws://本机局域网IP:8888"; color: "#667085" }
                Label { text: "请确保手机和电脑连接同一个 Wi-Fi，且路由器没有开启客户端隔离。"; color: "#667085"; wrapMode: Text.WordWrap }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            GroupBox {
                title: "已连接设备"
                Layout.preferredWidth: 300
                Layout.fillHeight: true
                ListView {
                    anchors.fill: parent
                    model: backend.clients
                    clip: true
                    delegate: Rectangle {
                        width: ListView.view.width
                        height: 58
                        color: index % 2 ? "#f5f7f8" : "white"
                        radius: 6
                        Column {
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.left: parent.left
                            anchors.leftMargin: 8
                            Text { text: modelData.address; font.bold: true; color: "#263238" }
                            Text { text: "连接时间：" + modelData.connectedAt; color: "#667085"; font.pixelSize: 11 }
                        }
                    }
                }
            }
            GroupBox {
                title: "向所有设备发送消息"
                Layout.fillWidth: true
                Layout.fillHeight: true
                ColumnLayout {
                    anchors.fill: parent
                    TextArea {
                        id: messageInput
                        Layout.fillWidth: true
                        Layout.preferredHeight: 90
                        placeholderText: "输入要发送给所有网页客户端的消息..."
                        wrapMode: TextArea.Wrap
                    }
                    Button {
                        text: "群发消息"
                        enabled: backend.running && backend.clientCount > 0 && messageInput.text.trim().length > 0
                        onClicked: {
                            backend.broadcast(messageInput.text)
                            messageInput.clear()
                        }
                    }
                    Label { text: "通信日志"; font.bold: true }
                    ListView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        model: window.logLines
                        delegate: Text {
                            width: ListView.view.width
                            text: modelData
                            color: "#344054"
                            font.pixelSize: 12
                            wrapMode: Text.Wrap
                            padding: 3
                        }
                    }
                }
            }
        }
    }
}
