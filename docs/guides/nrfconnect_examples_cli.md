# Using command-line interface in nRF Connect SDK examples

[lock-app](../../examples/lock-app/nrfconnect/README.md) and
[lighting-app](../../examples/lighting-app/nrfconnect/README.md) examples for
the Nordic Semiconductor development kit boards include a command-line interface
with access to application logs and
[Zephyr shell](https://docs.zephyrproject.org/1.13.0/subsystems/shell.html).

To access the console, use serial terminal emulator of your choice, like
`minicom` or `GNU screen`, with baud rate set to `115200`. For example, if you
wish to use `minicom`, run the command below, but be aware that `/dev/ttyACM0`
should be replaced with a device node of your development kit.

    $ minicom -D /dev/ttyACM0 -b 115200

If you reboot the board now, you should see boot logs showing up on the console:

```shell
uart:~$
I: nRF5 802154 radio initialized
I: 8 Sectors of 4096 bytes
I: alloc wra: 0, ff0
I: data wra: 0, 0
*** Booting Zephyr OS build v2.4.0-ncs1  ***
I: Init CHIP stack
...
```

Now you may try some shell commands. For example, the command below will print
information about all running threads:

```shell
uart:~$ kernel threads
Scheduler: 277 since last call
Threads:
 0x20006518 CHIP
        options: 0x0, priority: -1 timeout: 536896912
        state: pending
        stack size 8192, unused 7256, usage 936 / 8192 (11 %)

 0x20004ab0 SDC RX
        options: 0x0, priority: -10 timeout: 536890152
        state: pending
        stack size 1024, unused 848, usage 176 / 1024 (17 %)
...
```

Press the `Tab` key, responsible for the command completion feature, to get a
list of available commands:

```shell
uart:~$
  clear              date               device             flash
  help               history            kernel             nrf_clock_control
  ot                 resize             shell
```

Note that
[OpenThread commands](https://github.com/openthread/openthread/blob/master/src/cli/README.md)
are also accessible on the shell, but they must preceded by the word `ot`. For
example:

```shell
uart:~$ ot masterkey
00112233445566778899aabbccddeeff
Done
```
