# hanreader
Firmware for reading Swedish HAN-port using the SmartyReader from http://weigu.lu/.

[![License: LGPL v3](https://img.shields.io/badge/License-LGPL_v2.1-blue.svg)](https://www.gnu.org/licenses/lgpl-2.1)

## Getting started
1. Clone the repo.
1. Copy `src/config_TEMPLATE.h` to `src/config.h`.
1. Edit `src/config.h` to reflect your environment and settings.
1. Build and Upload the target `d1_mini_pro`
1. Edit `platformio.ini` to make OTA work.

## Reading logs

The logs are sent using UDP packages. I use Packet Sender (https://packetsender.com/) and configures it to listen to port 6666/udp.

## Usage & Contributions

Feel free to use. Don't hesitate to create a pull request with your improvements - there is room for them! :) 
