/*******************************************************************************
Example 09m: 湿度センサ HDC1000 [Ambient対応版] [変化量に応じて送信]
                                            Copyright (c) 2016 Wataru KUNINO
********************************************************************************
本サンプルで使用するクラウドサービスAmbient
http://ambidata.io/
*******************************************************************************/

#include <ESP8266WiFi.h>                    // ESP8266用ライブラリ
extern "C" {
#include "user_interface.h"                 // ESP8266用の拡張IFライブラリ
}
#include <WiFiUdp.h>                        // UDP通信を行うライブラリ
#include "Ambient.h"                        // Ambient用のライブラリの組み込み
#define PIN_LED 13                          // IO 13(5番ピン)にLEDを接続する
#define SSID "1234ABCD"                     // 無線LANアクセスポイントのSSID
#define PASS "password"                     // パスワード
#define AmbientChannelId 100                // チャネルID(整数)
#define AmbientWriteKey "0123456789abcdef"  // ライトキー(16桁の16進数)
#define SENDTO "192.168.0.255"              // 送信先のIPアドレス
#define PORT 1024                           // 送信のポート番号
#define SLEEP_P 20*60*1000000               // スリープ時間 20分(uint32_t)
#define SLEEP_N 36                          // 最長スリープ時間 SLEEP_P×SLEEP_N
#define DEADZONE 0.3                        // 前回値との相違に対する閾値(℃)
#define DEVICE "humid_3,"                   // デバイス名(5文字+"_"+番号+",")
void sleep();

Ambient ambient;
WiFiClient client;
float temp,hum;                             // センサ用の浮動小数点数型変数
float mem;                                  // RTCメモリからの数値データ保存用
extern int WAKE_COUNT;
unsigned long start_ms;                     // 初期化開始時のタイマー値を保存

void setup(){                               // 起動時に一度だけ実行する関数
    int waiting=0;                          // アクセスポイント接続待ち用
    start_ms=millis();                      // 開始時のタイマー値を保存
    pinMode(PIN_LED,OUTPUT);                // LED用ポートを出力に
    digitalWrite(PIN_LED,HIGH);             // LEDの点灯
    hdcSetup();                             // 湿度センサの初期化
    Serial.begin(9600);                     // 動作確認のためのシリアル出力開始
    Serial.println("Example 09m");          // 「Example 09」をシリアル出力表示
    temp=getTemp();                         // 温度を取得して変数tempに代入
    hum =getHum();                          // 湿度を取得して変数humに代入
    Serial.print(temp,2);                   // シリアル出力表示
    Serial.print(", ");                     // シリアル出力表示
    Serial.println(hum,2);                  // シリアル出力表示
    mem = fabs(readRtcInt()-temp);          // RTCメモリの温度値と比較する
    if( WAKE_COUNT % SLEEP_N &&             // SLEEP_Nが0以外 かつ
        mem <= DEADZONE                     // 閾値以下 のときに
    )sleep();                               // スリープを実行
    WiFi.mode(WIFI_STA);                    // 無線LANをSTAモードに設定
    WiFi.begin(SSID,PASS);                  // 無線LANアクセスポイントへ接続
    while(WiFi.status() != WL_CONNECTED){   // 接続に成功するまで待つ
        delay(100);                         // 待ち時間処理
        waiting++;                          // 待ち時間カウンタを1加算する
        if(waiting%10==0)Serial.print('.'); // 進捗表示
        if(waiting > 300)sleep();           // 300回(30秒)を過ぎたらスリープ
    }
    Serial.println(WiFi.localIP());         // 本機のIPアドレスをシリアル出力
    ambient.begin(AmbientChannelId, AmbientWriteKey, &client);  // Ambient開始
}

void loop(){
    WiFiUDP udp;                            // UDP通信用のインスタンスを定義
    char s[6];
    float v;

    if( temp<-100. || hum<0.) sleep();      // 不適切な値の時はスリープへ移行
    udp.beginPacket(SENDTO, PORT);          // UDP送信先を設定
    udp.print(DEVICE);                      // デバイス名を送信
    udp.print(temp,1);                      // 変数tempの値を送信
    udp.print(", ");                        // 「,」カンマを送信
    udp.println(hum,1);                     // 変数humの値を送信
    udp.endPacket();                        // UDP送信の終了(実際に送信する)
    /* Ambientへ 温湿度を送信する */
    dtostrf(temp,-5,2,s);                   // 温度を文字列に変換
    ambient.set(1,s);                       // Ambient(データ1)へ温度を送信
    dtostrf(hum,-5,2,s);                    // 湿度を文字列に変換
    ambient.set(2,s);                       // Ambient(データ2)へ湿度を送信
    /* Ambientへ 起動回数を送信する */
    if( mem<0. ) mem=0.;                    // 下限値を0に
    if( mem>200.) mem=200.;                 // 上限値を200に
    dtostrf(mem,-5,2,s);                    // 前回との差異を文字列へ変換
    ambient.set(3,s);                       // Ambient(データ3)へ差異を送信
    if(WAKE_COUNT>9e4) WAKE_COUNT=9e4;      // 上限値を90000に
    itoa(WAKE_COUNT,s,10);                  // 前回からの測定間隔数を文字列へ変換
    ambient.set(4,s);                       // Ambient(データ4)へ測定間隔数を送信
    /* Ambientへ 電池電圧値を送信する */
    v=(float)system_adc_read();             // AD変換器から値を取得
    v *= 5. / 1023.;                        // 電圧値へ変換
    dtostrf(v,-5,3,s);
    ambient.set(8,s);                       // Ambient(データ8)へ送信
    /* Ambientへ 起動時間を送信する */
    dtostrf(((float)(millis()-start_ms))/1000.,-5,3,s);
    ambient.set(7,s);                       // Ambient(データ7)へ測定間隔数を送信
    ambient.send();                         // Ambient送信の終了(実際に送信する)
    WAKE_COUNT=1;                           // 起動回数をリセット
    writeRtcInt(temp);                      // 温度値をRTCメモリへ保存
    delay(200);                             // 送信待ち時間
    sleep();
}

void sleep(){
/*	測定間隔調整＝20分／30分／60分の自動切り替え
	（前回の送信から60分以上経過したら30分間隔、90分以上で60分間隔）

	WAKE_COUNT	1	2	3	4	5	6	7...
	累積時間	0	20	40	60	90	150	210...
	測定間隔	20	20	20	30	60	60
	
	uint32_t SLEEP_P;
	if(WAKE_COUNT <= 3)		SLEEP_P = 20*60*1000000;
	else if(WAKE_COUNT==4) 	SLEEP_P = 30*60*1000000;
	else 					SLEEP_P = 60*60*1000000;
*/
    digitalWrite(PIN_LED,LOW);              // LEDの消灯
    ESP.deepSleep(SLEEP_P,WAKE_RF_DEFAULT); // スリープモードへ移行する
    while(1){                               // 繰り返し処理
        delay(100);                         // 100msの待ち時間処理
    }                                       // 繰り返し中にスリープへ移行
}
