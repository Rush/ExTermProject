import QtQuick 2.0
import QtWebKit 3.0
import QtWebKit.experimental 1.0

WebView {
    id: webView
    width: parent.width
    property int boundLineId
    experimental.transparentBackground: true
    experimental.deviceWidth: width
    experimental.deviceHeight: height
//    experimental.preferences.defaultFontSize: parent.font.pixelSize
//    experimental.preferences.defaultFixedFontSize: parent.font.pixelSize
    experimental.useDefaultContentItemSize: true

    experimental.preferredMinimumContentsWidth: 1

    experimental {
        urlSchemeDelegates: [
            UrlSchemeDelegate {
                scheme: "http"
                onReceivedRequest: {
                    reply.data = ""
                    reply.send()
                }
            },
            UrlSchemeDelegate {
                scheme: "https"
                onReceivedRequest: {
                    reply.data = ""
                    reply.send()
                }
            },
            UrlSchemeDelegate {
                scheme: "file"
                onReceivedRequest: {
                    reply.data = ""
                    reply.send()
                }
            },
            UrlSchemeDelegate {
                scheme: "icon"
                onReceivedRequest: {
                    reply.data = ""
                    reply.send()
                }
            }
        ]
    }

    onLoadingChanged: {
        if(!loading) {
            animateOpacity.start();
        }
    }

    NumberAnimation {
        id: animateOpacity
        target: webView
        properties: "opacity"
        from: 0
        to: 1.0
        easing {type: Easing.InQuad}
        duration: 100
    }

    property string html
    onHtmlChanged: loadHtml(html)

    Component.onCompleted: {
        console.log(experimental.urlSchemeDelegates);
     //   console.log(experimental.preferences.caretBrowsingEnabled);
    }
    focus: false
    Keys.onPressed: {
        focus: false
    }
}
