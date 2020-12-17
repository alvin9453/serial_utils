# serial_utils
Simple stty C utilities ( Refer to busybox )

## Functions

- Get all stty settings (i.e. setting `stty -a -F <device>`)
- Get input/output spped baud

## Usage
```
  $ gcc serial.c -o serial
  $ ./serial <device_name in path /dev/tty>
```
