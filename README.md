# ESP8266-THETA-S-FullRemoteControler
RICOH THETA S Remote Control Software (Full Control Edition) for Switch Science ESP-WROOM-02 Dev.board <BR>
<BR>
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
  ・単独でTHETA Sの登録ができます（SSID,Passwordは出荷状態であること）。<BR>
<BR>
以下の部材は必須です。（結線はソースコードのファイルヘッダ参照）<BR>
  ・ESP-WROOM-02 Dev.board : https://www.switch-science.com/catalog/2500/ <BR>
  ・I2C LCD Display        : https://www.switch-science.com/catalog/1405/ <BR>
  ・5Way Tactil Switch     : https://www.switch-science.com/catalog/979/ <BR>
<BR>
電源（充電回路とバッテリー）には以下を使用しています。<BR>
  ・Li-Po Battery          : https://www.switch-science.com/catalog/1007/<BR>
  ・LiPo Charger/Booster   : https://www.switch-science.com/catalog/821/<BR>
<BR>
ケースはBREOというお菓子のものを利用しています（ただし、このケースはすでに絶版）<BR>
電源スイッチ、リード線、5方向スイッチのキャップなどはさらに別途用意してください。<BR>
<BR>
本プログラムのベースとしたシングルボタン版は以下<BR>
https://github.com/katsuya-san/ESP8266-THETA-S-SimpleRemoteControler.git<BR>
基本的なことは上記を踏襲しています。<BR>
<BR>


