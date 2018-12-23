# h83069f_toy_os
Toy operating system that runs on [h8/3069f](http://akizukidenshi.com/catalog/g/gK-01271/). The implementation is based on [kozos](http://kozos.jp/kozos/) developed by [@kozossakai](https://twitter.com/kozossakai)

## Prepare for build
Your host computer is assumed to be MacOS. Here, `binutils` and `gcc` for H8 are installed under `/user/local/h8-elf`.
Other needed libraries are also installed under `/user/local/*`

```
$ git clone https://github.com/sykwer/h83069f_toy_os.git
```

### Install binutils-2.21.1 for H8
```
$ curl -O https://ftp.gnu.org/gnu/binutils/binutils-2.21.1.tar.bz2
$ tar jxf binutils-2.21.1.tar.bz2
$ cd binutils-2.21.1
$ ./configure --target=h8300-elf --disable-nls --prefix=/usr/local/h8-elf --disable-werror
$ make
$ sudo make install
```

### Install gmp-4.3.2
```
$ curl -O https://ftp.gnu.org/gnu/gmp/gmp-4.3.2.tar.bz2
$ tar jxf gmp-4.3.2.tar.bz2
$ cd gmp-4.3.2
$ ./configure --prefix=/usr/local/gmp CC=gcc-4.5.2
$ make
$ sudo make install
```

### Install mpfr-2.4.2
```
$ curl -O https://ftp.gnu.org/gnu/mpfr/mpfr-2.4.2.tar.bz2
$ tar jxf mpfr-2.4.2.tar.bz2
$ cd mpfr-2.4.2
$ ./configure --prefix=/usr/local/mpfr --with-gmp=/usr/local/gmp CC=gcc-4.5.2
$ make
$ sudo make install
```

### Install mpc-0.8.2
```
$ curl -O http://www.multiprecision.org/downloads/mpc-0.8.2.tar.gz
$ tar jxf mpc-0.8.2.tar.gz
$ cd mpc-0.8.2
$ ./configure --prefix=/usr/local/mpc --with-gmp=/usr/local/gmp --with-mpfr=/usr/local/mpfr CC=gcc-4.3.2
$ make
$ sudo make install
```

### Install gcc-4.5.2 for H8
```
$ curl -O https://ftp.gnu.org/gnu/gcc/gcc-4.5.2/gcc-4.5.2.tar.bz2
$ tar jxf gcc-4.5.2.tar.bz2
$ mkdir build && cd build
$ ../gcc-4.5.2/configure --target=h8300-elf --disable-nls --disable-threads --disable-shared --disable-libssp --enable-languages=c --with-gmp=/usr/local/gmp/lib --with-mpfr=/usr/local/mpfr/lib --with-mpc=/usr/local/mpc/lib --prefix=/usr/local/h8-elf
$ make
$ sudo make install
```

### Install USB serial driver
(Of cource, you can use any proper device driver), you can download PL2303 USB to Serial Driver v3.2.0 from [this site](https://www.mac-usb-serial.com/). After installing the device driver and connecting the USB serial connecter between the host PC and the micro computer, you can check it working properly by the command below.
```
$ ls -l /dev/cu*  # /dev/cu.Repleo-PL2303-00001014 (output maybe different)
```

### Install ROM writer (named `h8write`)
```
$ mkdir -p bin/tool && cd bin/tool
$ curl -OL http://mes.sourceforge.jp/h8/h8write.c
$ gcc -Wall -o h8write h8write.c
```

### Install lsx
`lsx` is used to transfer OS elf file to the micro computer by xmodem protocol.
```
$ brew install lrzsz
```

## Build
### Build and write bootloader to ROM
Switch DIP-switch to `on on off on` (ROM write mode).

Build and write bootloader to ROM (If the output of `ls -l /dev/cu*` is different from README,
you need to modify Makefile of bootloader)
```
$ cd src/bootload
$ make
$ make image
$ make write
```

### Build OS
```
$ cd src/os
$ make
```

## Run
Change directory to where the built OS image is.
```
$ cd bin/os
```

Switch DIP-switch to `on off on off` (mode five).

Connect by `cu` command
```
cu -l /dev/cu.Repleo-PL2303-00001014
```

Push the reset button, and you can see message output from bootloader.

Type in `load` and press enter to send the command.

Then execute `lsx` command to send the os image
```
~+ lsx kozos
```

Finally, the OS will start if you type in `run` and press enter to send the command.


