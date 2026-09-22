| Supported Targets | ESP32 | ESP32-C2 | ESP32-C3 | ESP32-C6 | ESP32-S2 | ESP32-S3 |
| ----------------- | ----- | -------- | -------- | -------- | -------- | -------- |

# Wi-Fi SoftAP & Station Example (ESP32-P4 + ESP32-C6)

This project is the ESP-IDF `wifi/softap_sta` example (Wi-Fi NAT router: SoftAP + Station with
NAPT) ported to the **ESP32-P4 + ESP32-C6 (ESP-Hosted over SDIO)** hardware combination.

> The ESP32-P4 has **no Wi-Fi radio of its own**. All `esp_wifi_*()` calls made by this
> application are forwarded over SDIO to the ESP32-C6 by the `esp_wifi_remote` + `esp_hosted`
> components; the C6 owns the radio and runs both the SoftAP and the Station.

The application:

* always keeps the SoftAP up (default SSID `ELECROW` / password `12345678`), and prints its IP;
* enables NAPT on the AP netif as soon as the AP is up, so it no longer depends on the station;
* keeps retrying to join the upstream router forever, and reconnects automatically if the router
  disappears;
* pushes the upstream DNS server to the AP DHCP server every time the station gets an IP.

---

## 1. 中文说明

### 1.1 两个 Wi-Fi 角色

| 角色 | 默认 SSID / 密码 | 说明 |
| ---- | ---------------- | ---- |
| SoftAP（本机热点） | `ELECROW` / `12345678` | 手机、电脑连这个 |
| Station（上联） | `yanfa1` / `1223334444yanfa` | 设备去连这个路由器，从它获取 IP 并共享上网 |

只有 Station 成功连上上游路由器后，SoftAP 的客户端才能上网（NAPT 转发）。

### 1.2 配置上游路由器（关键步骤）

`idf.py menuconfig` → `Example Configuration`：

* `STA Configuration` → `WiFi Remote AP SSID` / `WiFi Remote AP Password`：填**你自己的路由器**
  （必须是 **2.4 GHz** 网络，ESP32-C6 不支持 5 GHz）。
* `SoftAP Configuration` → `WiFi AP SSID` / `WiFi AP Password`：本机热点，随意。

也可以直接改 `main/Kconfig.projbuild` 里的 `default`，或改 `sdkconfig.defaults` 后删除
`sdkconfig` 重新生成。

### 1.3 预期串口日志

```
I (xxx) WiFi SoftAP: SoftAP "ELECROW" password:"12345678" channel:1, connect your phone to it
I (xxx) WiFi SoftAP: SoftAP IP:192.168.4.1, gateway:192.168.4.1, mask:255.255.255.0
I (xxx) WiFi SoftAP: NAPT enabled on the softAP interface
I (xxx) WiFi Sta: Station started, connecting to "yanfa1" ...
I (xxx) WiFi Sta: Got IP:192.168.1.123                 <-- 关键：Station 拿到 IP
I (xxx) WiFi SoftAP: AP DHCP clients now use DNS 192.168.1.1
I (xxx) WiFi SoftAP: Client aa:bb:cc:dd:ee:ff got IP:192.168.4.2
I (xxx) WiFi Sta: === STA online, NAPT enabled ===
```

### 1.4 手机连上热点却上不了网？按这个顺序排查

1. **先看设备有没有连上路由器**：日志里没有 `WiFi Sta: Got IP:`，就说明设备本身还没上外网，
   和手机无关。此时会打印 `Failed to connect to SSID:"xxx"`，请检查：
   * SSID / 密码是否正确（`menuconfig` → `WiFi Remote AP SSID/Password`）；
   * 路由器是不是 **5 GHz**（C6 只能连 2.4 GHz）；
   * 路由器是否开启了 MAC 过滤 / AP 隔离。
2. **确认 NAPT 已使能**：必须出现 `NAPT enabled on the softAP interface`。没有它，手机即使
   拿到 192.168.4.x 的地址也无法上网（`Failed to enable NAPT` 通常是
   `CONFIG_LWIP_IPV4_NAPT` / `CONFIG_LWIP_IP_FORWARD` 没打开，本项目 `sdkconfig.defaults`
   已默认打开）。
3. **确认 DNS 已下发给手机**：出现 `AP DHCP clients now use DNS x.x.x.x`。如果手机能 ping 通
   IP 但打不开网页，多半就是 DNS 问题，手机端"忘记网络"后重连一次。
4. **检查网段冲突**：如果上游路由器本身就是 `192.168.4.x`，会和 SoftAP 默认的
   `192.168.4.1/24` 冲突。请把 SoftAP 改成别的网段（例如 `192.168.10.1/24`，用
   `esp_netif_dhcps_stop()` + `esp_netif_set_ip_info()` + `esp_netif_dhcps_start()`）。
5. **手机侧**：Android 会缓存"已连接，但无法访问互联网"的状态，请先"忘记"该热点再重连。
6. **SDIO / ESP-Hosted 通信问题**：如果日志里出现 `sdmmc_init_ocr: send_op_cond ... 0x107`、
   `ESP-Hosted link not yet up` 之类，说明 P4 与 C6 之间的 SDIO 没通（C6 里没有烧写匹配版本的
   esp_hosted slave 固件，或 GPIO/上拉电阻不对）。此时连 SoftAP 都不会出现。

### 1.5 编译与烧写

```bash
idf.py set-target esp32p4
idf.py build
idf.py -p COM3 flash monitor
```

> 注意：主机端的 `espressif/esp_hosted` 版本要和 C6 从机固件的版本一致（本工程为 2.7.0）。

---

## 2. How to use example (upstream documentation)

### Configure the project

Open the project configuration menu (`idf.py menuconfig`).

In the `Example Configuration` menu:

* Set the Wi-Fi SoftAP configuration.
    * Set `WiFi AP SSID`.
    * Set `WiFi AP Password`.

* Set the Wi-Fi STA configuration.
    * Set `WiFi Remote AP SSID`.
    * Set `WiFi Remote AP Password`.

### Build and Flash

Run `idf.py -p PORT flash monitor` to build, flash and monitor the project.

(To exit the serial monitor, type ``Ctrl-]``.)

## Example Output

```
I (680) WiFi SoftAP: ESP_WIFI_MODE_AP
I (690) WiFi SoftAP: wifi_init_softap finished. SSID:myssid password:mypassword channel:1
I (690) WiFi Sta: ESP_WIFI_MODE_STA
I (690) WiFi Sta: wifi_init_sta finished.
I (800) wifi:mode : sta (58:bf:25:e0:41:00) + softAP (58:bf:25:e0:41:01)
I (820) WiFi Sta: Station started
I (2400) wifi:connected with myssid_c3, aid = 1, channel 1, 40U, bssid = 84:f7:03:60:86:1d
I (3920) WiFi Sta: Got IP:192.168.5.2
I (3920) WiFi Sta: connected to ap SSID:myssid_c3 password:mypassword_c3
I (3922) WiFi SoftAP: AP DHCP clients now use DNS 192.168.5.1
```

## Troubleshooting

For any technical queries, please open an [issue](https://github.com/espressif/esp-idf/issues) on GitHub. We will get back to you soon.
