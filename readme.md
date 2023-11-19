

## 接続したwifiを決めて`main.cpp`に書き込む

```
#define WIFI_SSID "<wifiのSSIDを入れる>" //NOTE: please change SSID
#define WIFI_PASSWORD "<wifiのpasswordを入れる>" //NOTE: please change password
```
## 実際に送りたいTokenは以下から取得し`main.cpp`に書き込む
> https://notify-bot.line.me/my/

```
const char* token = "<ここでline notifyから得たtokenを入れる>";
```