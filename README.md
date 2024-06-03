Четырехканальное реле.

Доступно управление через:

* Радио пульты 433 МГц
* HTTP API
* Telegram


Для сборки устройства потребуется:

* ESP32C3-CORE
* Модуль питания 5V700MA
* Релейный модуль на 4 порта с управлением через 5V
* Модуль RX480E-4 433 МГц
* Dupont мелочевка (мама провода, пины, пинхедеры)
* Опционально корпус [STL](stl/Case_ESP32_RX480E_HW316.stl)


Схема подключение компонентов:
![ESP32_RX480E_HW316.png](images/ESP32_RX480E_HW316.png?raw=true "Title")


Вариант исполнения готового устройства:
![photo_2024-06-02_21-40-52.jpg](images/photo_2024-06-02_21-40-52.jpg?raw=true "Title")



В случае если вы хотите использовать управление через Telegram, предварительно необходимо:

* Начать чат с https://t.me/BotFather
* Создать нового бота и получить и сохранить`API Token`
* Начать чат с https://t.me/getmyid_bot
* Узнать и сохранить свой `user ID`


Билд прошвки.

Подготовка окружения

```
sudo apt install git wget flex bison gperf python3 python3-pip python3-venv cmake ninja-build ccache libffi-dev libssl-dev dfu-util libusb-1.0-0
mkdir -p ~/esp
cd ~/esp
git clone --recursive -b 4.4 https://github.com/espressif/esp-idf.git
cd ~/esp/esp-idf
bash install.sh esp32c3
. ~/esp/esp-idf/export.sh
```

Получение прошивки:

```
mkdir -p ~/esp/projects
cd ~/esp/projects
git clone https://github.com/gadonline/ESP32_RX480E_HW316.git
```

Настройка прошивки:

```
cd ~/esp/projects/ESP32_RX480E_HW316
idf.py menuconfig
```

Откроется `Espressif IoT Development Framework Configuration` в котором необходимо:

* Перейти в раздел `Example Connection Configuration`
* Задать `WiFi SSID` и `WiFi Password` вашей сети
* Если необходимо управление через Telegram, необходимо вернутся в верхний уровень и зайти в `Telegram Configuration`
* Задать `Telegram API token`
* В `Telegram allowed chat_id values` через запятую задать `user ID` аккаунтов для которых разрешено управление
* Выйти с сохранением конфига

Билд и прошивка:

```
cd ~/esp/projects/ESP32_RX480E_HW316
idf.py build
idf.py flash
idf.py monitor
```


HTTP API

```
GET /api/relay?metod=on&port=(0-3|all)
GET /api/relay?metod=off&port=(0-3|all)
GET /api/relay?metod=status&port=(0-3|all)
GET /api/relay?metod=set_name&port=(0-3)
```

Управление через Telegram

```
on (0-3|all)
off (0-3|all)
set_name (0-3) relay_name
```
