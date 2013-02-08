import QtQuick 2.0
import QtWebKit 3.0
import QtWebKit.experimental 1.0
import ExTermIO 1.0

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

    ExTermIO { id: io }

    experimental {
        urlSchemeDelegates: [
            UrlSchemeDelegate {
                scheme: "local"
                onReceivedRequest: {
                    console.log("FILE request");
                    io.getUrl(request.url, function(err, value, contentType) {
                        reply.data = value;
                        reply.contentType = contentType;
                        reply.send();
                    });
                }
            },
            UrlSchemeDelegate {
                scheme: "http"
                onReceivedRequest: {
                    console.log("HTTP request");
                    reply.data = ""
                    reply.send()
                }
            },
            UrlSchemeDelegate {
                scheme: "https"
                onReceivedRequest: {
                    console.log("HTTPS request");
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

    onLoadProgressChanged: {
        console.log("loadProgress" + loadProgress);
    }

    onLoadingChanged: {
        console.log("Loading" + loading);
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
        easing {type: Easing.OutCubic}
        duration: 500
    }

    property string html
    onHtmlChanged: loadHtml(html)

    Component.onCompleted: {
        console.log("Completed");
        console.log(experimental.urlSchemeDelegates);
     //   console.log(experimental.preferences.caretBrowsingEnabled);
    }
    focus: false
    Keys.onPressed: {
        focus: false
    }
}
