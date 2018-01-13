/*******************************************************************************
esp32 27 goo lcd2

Google カレンダー(予定表) から予定を取得する

クラウド（Google Apps Script）で実行したスクリプトを本機で受け取ります。
予め本フォルダ内のGoogleCalendar.jsを https://script.google.com/ へ転送し、
ウェブアプリケーションを公開しておく必要があります。

                                           Copyright (c) 2017-2018 Wataru KUNINO
********************************************************************************/

#include <WiFi.h>
#include <LiquidCrystal.h>                  // LCDへの表示を行うライブラリ
#include "HTTPSRedirect.h"                  // リダイレクト接続用ライブラリ

#define CQ_PUB_IOT_EXPRESS                  // CQ出版 IoT Express 用
#define SSID "1234ABCD"                     // 無線LANアクセスポイントのSSID
#define PASS "password"                     // パスワード
#define GScriptId "★ここにGoogle Apps Scriptのトークンを記入してください★"

#define PIN_BUZZER 12                       // GPIO 12にスピーカを接続
#define PIN_LED 2                           // GPIO 2(24番ピン)にLEDを接続
#define TIMEOUT 7000                        // タイムアウト 7秒
#define SLEEP_P 10*60*1000000               // スリープ時間 10分(uint32_t)
#define HTTPTO "script.google.com"          // HTTPSアクセス先
#define HTRED "script.googleusercontent.com"// HTTPSリダイレクト先
#define PORT 443                            // HTTPSポート番号

LiquidCrystal lcd(17,26,13,14,15,16);       // CQ出版 IoT Express 用 LCD開始
String url = String("/macros/s/") + String(GScriptId) + "/exec";
unsigned long TIME=0;                       // 1970年からmillis()＝0までの秒数
char date[20]="2000/01/01,00:00:00";        // 日時保持用
char buf[8][17];                            // データ保持用バッファ8件分
int buf_n=0;                                // データ保持件数
int buf_i=0;                                // 表示中データ番号の保持
int chime=0;                                // チャイム音の鳴音回数

int wifi_on(){                              // 無線LANの接続
    unsigned long time=millis();            // 初期化開始時のタイマー値を保存
    lcdisp("Google ｶﾚﾝﾀﾞ LCD");             // タイトルをLCDに表示する
    WiFi.mode(WIFI_STA);                    // 無線LANを【STA】モードに設定
    WiFi.begin(SSID,PASS);                  // 無線LANアクセスポイントへ接続
    lcd.setCursor(0,1);                     // カーソル位置を液晶の左下へ
    while(WiFi.status() != WL_CONNECTED){   // 接続に成功するまで待つ
        if(millis()-time > TIMEOUT){        // 7秒経過時
            lcdisp("AP ｾﾂｿﾞｸ ｼｯﾊﾟｲ",1);     // Wi-Fi APへの接続を失敗した
            WiFi.disconnect(true);          // WiFiアクセスポイントを切断する
            WiFi.mode(WIFI_OFF);            // 無線LANをOFFに設定する
            return 0;                       // 終了
        }
        lcd.print(".");                     // 接続進捗を表示
        delay(500);                         // 待ち時間処理
    }
    lcdisp("Connected",1);                  // 接続成功表示
    return 1;
}

void wifi_off(){                            // 無線LANの切断
    WiFi.disconnect(true);                  // WiFiアクセスポイントを切断する
    WiFi.mode(WIFI_OFF);                    // 無線LANをOFFに設定する
    delay(10);                              // 処理完了待ち時間
}

void wifi_ntp(){                            // Wi-Fi接続とNTPアクセスの実行関数
    if(!wifi_on()) return;                  // 無線LANの接続
    TIME=getNtp();                          // NTP時刻を取得
    TIME-=millis()/1000;                    // カウント済み内部タイマー値を減算
    wifi_off();                             // 無線LANの切断
}

void wifi_google(){                         // Googleカレンダから予定を取得する
    String data;                            // 受信データ
    String event;                           // イベント名
    char disp[49];                          // 液晶表示データ3バイト×16文字分
    int sp,ep;                              // 文字列位置用の変数
    int hour,min;                           // 時刻
    
    if(!wifi_on()) return;                  // 無線LANの接続
    HTTPSRedirect client(PORT);             // リダイレクト可能なHTTP接続client
    if(!client.connect(HTTPTO,PORT)){       // HTTP接続の実行(失敗時はスリープ)
        wifi_off();                         // 見つからなければ無線LANを切断する
        return;
    }
    data=client.getData(url,HTTPTO,HTRED);  // データ受信
    wifi_off();                             // 無線LANの切断
    Serial.println(data);                   // 受信データをシリアルへ出力
    sp=data.indexOf("|Length,");            // 受信データから文字列Lengthを検索
    if(sp>=0) sp+=8; else return;           // 見つけた場合は、spに8を加算
    buf_n=data.substring(sp).toInt();       // 予定数(Length値)を保存
    if(buf_n>8) buf_n=8;                    // 最大件数8を超過していた時は8に
    lcd.clear();                            // 画面を消去
    memset(buf,0,8*17);                     // バッファメモリの消去
    for(int i=0; i<buf_n; i++){             // 予定回数の繰り返し
        sp=data.indexOf("|",sp+1)+1;        // 次の区切りの文字位置を変数spへ
        hour=data.substring(sp).toInt();    // 文字位置spの予定時間（時）を取得
        sp=data.indexOf(",",sp+1)+1;        // カンマの次の文字位置を変数spへ
        min=data.substring(sp).toInt();     // 文字位置spの予定時間（分）を取得
        sp=data.indexOf(",",sp+1)+1;        // カンマの次の文字位置を変数spへ
        ep=data.indexOf("|",sp+1);          // 次の区切り文字の位置を変数epへ
        event=data.substring(sp,ep);        // sp～epの範囲の文字列を変数eventへ
        memset(disp,0,48);                  // 文字配列変数dispの初期化
        snprintf(disp,7,"%02d:%02d,",hour,min); // dispへ時刻の文字を代入
        event.getBytes((byte*)disp+6,42);   // 時刻の後ろに変数eventの内容を代入
        lcdisp_utf_to_ank(disp);            // UTF8をLCD用文字ANKコードへ変換
        strncpy(buf[i],disp,16);            // LCD用文字データをバッファへコピー
    }
}

void setup() {
    pinMode(PIN_LED,OUTPUT);                // LEDを接続したポートを出力に
    pinMode(PIN_BUZZER,OUTPUT);             // ブザーを接続したポートを出力に
    Serial.begin(115200);                   // 動作確認のためのシリアル出力開始
    lcd.begin(16, 2);                       // 液晶の初期化(16桁×2行)
    chimeBellsSetup(PIN_BUZZER);            // ブザー/LED用するPWM制御部の初期化
    wifi_ntp();                             // 時刻情報の取得
    wifi_google();                          // Googleカレンダから予定を取得する
}

void loop() {
    delay(100);                             // 待ち時間処理
    unsigned long time=millis();            // ミリ秒の取得
    if(time%1000 > 100) return;             // 以下は1秒に一回の処理
    time2txt(date,TIME+time/1000);          // 時刻を文字配列変数dateへ代入
    if(time%21600000ul<100){                // 6時間に1回の処理
        wifi_ntp();                         // wifi_ntpを実行
    }
    if(time%(SLEEP_P/1000)<100){            // SLEEP_P間隔の処理
        wifi_google();                      // Googleカレンダから予定を取得する
        buf_i=0;                            // 表示用の予定番号をリセット
    }
    if(!strncmp(date+11,buf[0],5)){         // 予定の時刻と一致
        if(!chime) chime = 2;               // 0→2 1→1
        if(!strncmp(buf[0]+6,"LED=",4)){
            int led = atoi(buf[0]+10) % 2;  // 「LED=」の数値を変数ledへ代入
            digitalWrite(PIN_LED,led);      // LEDの点灯または消灯
        }
    }
    chime=chimeBells(PIN_BUZZER,chime);     // chimeが0以外の時にチャイム鳴音
    if(buf_n <= 1){                         // 予定件数が0件または1件の時
        lcdisp(date+5);                     // dateの6文字目以降をLCDへ表示
        if(buf_n==1){                       // 予定件数が1件の時
            lcdisp(buf[0],1);               // LCDの2行目に予定を表示
        }
        return;
    }
    buf_i++;
    if(buf_i >= buf_n) buf_i=1;             // 表示番号が上限に達したらリセット
    lcdisp(buf[0]);                         // 1行目に1件目の予定を表示
    lcdisp(buf[buf_i],1);                   // 2行目に2件目以降の予定を表示
}
