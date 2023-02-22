## Bootloader for k1921vk035
Bootloader for K1921VK035. Is used with flasher tool [k1921vkx_flasher](https://github.com/DCVostok/k1921vkx_flasher). Bootloader firmware is loaded to flash NVR region with size 3 kB. 

### Enter to bootloader.
1. Pulldown `BOOTEN_PIN`
2. Reset MCU
3. Send byte `0x7F` to `UART0` before timeout occurs (by default is `500ms`)
4. Wait answer `PACKET_DEVICE_SIGN` (by default is `0x7EA3`)
### Support cmds
* CMD_GET_INFO
* CMD_GET_CFGWORD
* CMD_SET_CFGWORD
* CMD_WRITE_PAGE
* CMD_READ_PAGE
* CMD_ERASE_FULL
* CMD_ERASE_PAGE
* CMD_EXIT
## Upload bootloder

1. Set pin SERVEN to 3.3v
2. Hard Reset 
3. Do full service erase
```
pio run -t service_full_erase
```
1. Unset pin SERVEN
2. Hard Reset
3. Enable bootloader
```
pio run -t enable_boot
```
1. Hard Reset
2. Upload bootloader firmware to NVR region
```
pio run -t upload
```

Note: If BMOEDIS bit is reset, uploading the main firmware via jtag/swd damages the bootloader firmware.

# Testing
Test use flasher tool [k1921vkx_flasher](https://github.com/DCVostok/k1921vkx_flasher).

## Run test 
Tests runs on Vostok UNO-vn035
 ```
 cd test/test_firmware
 pio run -t run_tests --upload-port COM6
 ```
