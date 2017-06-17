#!/bin/bash
esptool.py --port /dev/tty.wchusbserial410 write_flash -fs 4m -ff 40m \
    0x00000 boot_v1.6.bin 0x1000 user1.bin \
    0x7C000 esp_init_data_default.bin 0x7E000 blank.bin
