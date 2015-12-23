#RICOH THETA S Full Remote Controler <BR>(for which a ESP8266 was used)
RICOH THETA S Remote Control Software (Full Control Edition) for Switch Science ESP-WROOM-02 Dev.board <BR>
##Example of making hardware
https://twitter.com/san_san_santa/status/668861299040718849
##Commentary
説明動画<BR>
[![](http://img.youtube.com/vi/05vk-4FECPw/0.jpg)](https://www.youtube.com/watch?v=05vk-4FECPw)<BR>
<BR>
ESP8266(ESP-WROOM-02)を利用した THETA S の フル操作リモコンです。<BR>
フル操作といいつつインターバル撮影の枚数指定は自分にとって無限枚固定でよいので設定できません。<BR>
スマホより応答がよいリモコンになっておりスマホ要らずです。<BR>
ただ操作できるというだけでなく、以下の特徴があります。<BR>
  ・AUTOモードでWBを指定できます。<BR>
  ・音量設定ができます。<BR>
  ・起動時のモードが指定できるので、素早く撮影を開始できます。<BR>
    (THETA S電源On後、最初にリモコンに接続したときのモードを設定可能)<BR>
  ・単独でTHETA Sの登録ができます（SSID,Passwordは出荷状態であること）。<BR>
<BR>
以下の部材は必須です。（結線はソースコードのファイルヘッダ参照）<BR>
  ・ESP-WROOM-02 Dev.board : https://www.switch-science.com/catalog/2500/ <BR>
  ・I2C LCD Display        : https://www.switch-science.com/catalog/1405/ <BR>
  ・5Way Tactil Switch     : https://www.switch-science.com/catalog/979/ <BR>
<BR>
電源（充電回路とバッテリー）には以下を使用しています。<BR>
  ・LiPo Charger/Booster   : https://www.switch-science.com/catalog/1007/<BR>
  ・Li-Po Battery          : https://www.switch-science.com/catalog/821/<BR>
<BR>
ケースはBREOというお菓子のものを利用しています（ただし、このケースはすでに絶版）<BR>
電源スイッチ、リード線、5方向スイッチのキャップなどはさらに別途用意してください。<BR>
<BR>
##ソフトウェア書き込み方法（開発環境セットアップなど）
本プログラムのベースとしたシングルボタン版は以下<BR>
https://github.com/katsuya-san/ESP8266-THETA-S-SimpleRemoteControler.git<BR>
基本的なことは上記を踏襲しています。<BR>
<BR>
##License
The MIT License (MIT)

Copyright (c) 2015 Katsuya Yamamoto

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.


