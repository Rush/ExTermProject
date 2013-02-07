import QtQuick 2.0

import QtWebKit.experimental 1.0

Item {
    id: text_line
    property QtObject textLine

    width: parent.width

//    WebView {
//        id: webView
//        width: parent.width
//        height: 20
//        Component.onCompleted: webView.loadHtml('<span style="color: white;">WWWWWWWWWWWWWWWWWWWWWW</span>')

//        //experimental.transparentBackground: true
//    }

/*    Image {
            id: image
            width: parent.width; height: parent.height
            source: "file:///home/rush/Obrazy/sleeping_beauty.jpg"
            clip: true      // only makes a difference if mode is PreserveAspectCrop
        }*/


    Connections {
        target: textLine

        onIndexChanged: {
           y = textLine.index * height;
//           rect.y = y;
  //          console.log("Set y " + y + " " + textLine.toString());
        }
    }
}

