# aws-mqtt-websocket-wifimanager-TFT1.4
AWS IOT メッセージモニター

Platform IDE for Visual Studio Codeで Arduinoフレームワークを用いて作成した ESP32 プログラム。

MH-ET Live ESP32 Minikit (ESP-WROOM-32)と WeMos D1 mini TFT 1.4 Shield を用いて AWS IoT に publish した情報を 小型TFT ディスプレイに表示するプログラムである。トピック "iot/#" を subscribeして、受信したメッセージを表示している。AWS IoTコンソールで表示される内容を手元で表示するものである。

もう少し詳しい説明が[薫染庵 途上日誌](http://kunsen.net/)にある。

WiFiManagerを用いて WiFi接続を行っている。

MQTTで指定のトピックを subscribeしておき、そのトピックでAWS IoTにpublishされたメッセージを受信して表示している。

# 設定
aws-mqtt-websocket.cpp 中の次のインクルードファイルはこのリポジトリに含まれていない。
* myAWSus-east2.h

AWS IoTと通信するために以下のマクロを定義する必要がある。

* MY_AWS_REGION
* MY_AWS_ENDPOINT
* MY_AWS_IAM_KEY
* MY_AWS_IAM_SECRET_KEY

# ファイル
各ファイルの主な役割は以下の通り。

* main.cpp
    * メインプログラム
        * トピック "iot/#" を subscribeして、受信したメッセージを表示
    * [ArduinoJson](https://github.com/bblanchon/ArduinoJson)でJSONの処理をしている。
    * [ArduinoLog](https://github.com/thijse/Arduino-Log/)を使ってデバッグメッセージ出力を制御している。

* wifiManager.cpp
    * [WiFiManager](https://github.com/tzapu/WiFiManager) を使って、WiFi接続をする

* aws-mqtt-websocket.cpp
    * AWS IoT と MQTT over Websocketを使って通信するためのコード
    * [aws-mqtt-websockets](https://github.com/odelot/aws-mqtt-websockets) を利用している

* wemos-tft-display.cpp
    * WeMos D1 mini TFT 1.4 Shield　に表示するためのルーチンを含んでいる
    * Adafruit GFX ライブラリ、Adafruit ST7735 ライブラリを利用している

* setupDeviceName.cpp
    * デバイス名を設定するための補助コード(このプログラムではあまり意味はない)


# ライブラリ

以下のバージョンのライブラリで動作確認をした。


```
Dependency Graph
|-- <Adafruit ST7735 and ST7789 Library> 1.2.4
| |-- <Adafruit GFX Library> 1.2.9
| | |-- <SPI> 1.0
| |-- <SPI> 1.0
|-- <PubSubClient> 2.6
|-- <SPI> 1.0
|-- <DNSServer> 1.1.0
| |-- <WiFi> 1.0
|-- <Wire> 1.0
|-- <WebServer> 1.0
| |-- <FS> 1.0
| |-- <WiFi> 1.0
|-- <Adafruit GFX Library> 1.2.9
| |-- <SPI> 1.0
|-- <ArduinoJson> 5.13.2
|-- <WifiManager> 1.0.0
| |-- <WebServer> 1.0
| | |-- <FS> 1.0
| | |-- <WiFi> 1.0
| |-- <DNSServer> 1.1.0
| | |-- <WiFi> 1.0
| |-- <ESPmDNS> 1.0
| | |-- <WiFi> 1.0
| |-- <WiFi> 1.0
|-- <ArduinoLog> 1.0.2
|-- <mylan>
|-- <WebSockets> 2.1.0
| |-- <WiFiClientSecure> 1.0
| | |-- <WiFi> 1.0
| |-- <WiFi> 1.0
| |-- <SPI> 1.0
|-- <WiFi> 1.0
|-- <aws-mqtt-websockets>
| |-- <WebSockets> 2.1.0
| | |-- <WiFiClientSecure> 1.0
| | | |-- <WiFi> 1.0
| | |-- <WiFi> 1.0
| | |-- <SPI> 1.0
| |-- <aws-sdk-arduino-ESP32>
| | |-- <WiFiClientSecure> 1.0
| | | |-- <WiFi> 1.0
| | |-- <WiFi> 1.0
|-- <Paho> 1.0.0
| |-- <SPI> 1.0
| |-- <WiFi> 1.0
```