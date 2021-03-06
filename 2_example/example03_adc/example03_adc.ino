/*******************************************************************************
Example 3: アナログ入力値を送信する
                                            Copyright (c) 2016 Wataru KUNINO
*******************************************************************************/

#include <ESP8266WiFi.h>                    // ESP8266用ライブラリ
extern "C" {
#include "user_interface.h"                 // ESP8266用の拡張IFライブラリ
}
#include <WiFiUdp.h>                        // UDP通信を行うライブラリ
#define SSID "1234ABCD"                     // 無線LANアクセスポイントのSSID
#define PASS "password"                     // パスワード
#define SENDTO "192.168.0.255"              // 送信先のIPアドレス
#define PORT 1024                           // 送信のポート番号

void setup(){                               // 起動時に一度だけ実行する関数
    Serial.begin(9600);                     // 動作確認のためのシリアル出力開始
    Serial.println("Example 03 ADC");       // 「Example 03」をシリアル出力表示
    WiFi.mode(WIFI_STA);                    // 無線LANをSTAモードに設定
    WiFi.begin(SSID,PASS);                  // 無線LANアクセスポイントへ接続
    while(WiFi.status() != WL_CONNECTED){   // 接続に成功するまで待つ
        delay(500);                         // 待ち時間処理
    }
    Serial.println(WiFi.localIP());         // 本機のIPアドレスをシリアル出力
}

void loop(){                                // 繰り返し実行する関数
    WiFiUDP udp;                            // UDP通信用のインスタンスを定義
    int adc,i;                              // 整数型変数adcとiを定義
    
    adc=system_adc_read();                  // AD変換器から値を取得
    udp.beginPacket(SENDTO, PORT);          // UDP送信先を設定
    udp.println(adc);                       // 変数adcの値を送信
    Serial.println(adc);                    // シリアル出力表示
    udp.endPacket();                        // UDP送信の終了(実際に送信する)
    for(i=0;i<30;i++){                      // 30回の繰り返し処理
        delay(100);                         // 100msの待ち時間処理
    }
}
