# DAPLink
CMSIS-DAP (DAPLink) porting to SWM341

# 目录用途
## DAPLink
DAPLink 源码，支持 HID 和 WINUSB 传输协议。

## UserBoot
根据跳冒设置选择执行 HID DAPLink 还是 WINUSB DAPLink。

## bin
UserBoot、HID DAPLink 和 WINUSB DAPLink 的二进制文件，可使用 SWMProg 将它们一起烧录到 SWM341 芯片中。

## LCEDA PCB
DAPLink 硬件原理图和 PCB，使用 LCEDA 绘制。