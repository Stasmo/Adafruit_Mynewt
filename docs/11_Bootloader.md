# Working with the Mynewt Bootloader

ToDo: Notes on how to enter bootloader to recover bricked devices, how to update over serial or BLE, etc.

## Enabling `BOOT_SERIAL` Serial Bootloader Support

Mynet has an option to enable 'serial' support in the bootloader, meaning
that when a firmware image isn't present you can still use a standard
serial connection to flash firmware onto the device (at the expense of a
slightly larger bootloader).

To enable `BOOT_SERIAL` support in your bootloader project, add the
`BOOT_SERIAL` flag to your target as follows:

```
$ newt target set bootloader syscfg=BOOT_SERIAL=1
```

This adds the following entry to syscfg.yml in the `bootloader` target:

```
syscfg.vals:
    BOOT_SERIAL: 1
```

#### Update `apps/boot` Dependencies

You also need to make a small modification to the `apps/boot` project's
pkg.yml file. The following `pkg.dep` entry needs to be commented out since
`BOOT_SERIAL` requires `console/full`:

```
#    - libs/console/stub
```

#### Add BOOT_SERIAL Macros to BSP

The following macros are used by `libs/boot_serial` and must be defined in
your BSP:

```
/* BOOT_SERIAL pins */
/* DFU pin is set to 0.07 on the BLEFRIEND32 */
#define BOOT_SERIAL_DETECT_PIN      (7)
/* 0 = No pullup, 1 = Pull Up, 2 = Pull Down */
#define BOOT_SERIAL_DETECT_PIN_CFG  (1)
/* Board has external pullup and the tact switch sets the pin to GND */
#define BOOT_SERIAL_DETECT_PIN_VAL  (0)
```

## Build bootloader

At this point you can build the bootloader:

```
$ newt build bootloader
```

Once built you can check the size to make sure it's under 32KB:

```
$ newt size bootloader
```

The bootloader can then be flashed with:

```
$ newt load bootloader
```

## Using `BOOT_SERIAL`

To use the serial bootloader you must set the `BOOT_SERIAL_DETECT_PIN` to an
appropriate state (set via `BOOT_SERIAL_DETECT_PIN_VAL`). If the bootloader
detects that the selected pin is in an appropriate state, it will wait for
serial commands on the console.

To check if the serial bootloader is working you can request a list of
images on the device as follows:

```
$ newtmgr -c serial1 image list
Images:
    0 : 0.3.4.0
```

You can use `newtmgr` to upload a firmware image over serial, as shown below
(this code assume you have already setup a serial port for `newtmgr` to use
as a connection entry):

```
$ newtmgr -c serial1 image upload bin/bleuart/apps/bleuart/bleuart.img
```
