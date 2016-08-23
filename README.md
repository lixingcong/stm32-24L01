## linux下STM32开发环境
### 基础教程请见[@embbnux](https://github.com/embbnux/)的博客:

[GCC安装以及工程Makefile建立](http://www.embbnux.com/2014/02/01/linux_stm32_gcc_makefile/)

[程序烧录 openocd+openjtag](
http://www.embbnux.com/2014/02/01/linux_stm32_use_openocd_openjtag/)

### ubuntu

本仓库是以我手上stm32f103zet为例，512K ROM，属于high-density型号。
如不是这个型号，请务必修改下列2个文件。至于怎么改，请参考网上的内存分布地址。或者参考stm32f10x.h中宏定义的地址

	vi Makefile.common
    # TypeOfMCU=STM32F10X_xx
    vi linker.ld
    # MEMORY部分

#### arm-none-eabi-gcc

使用官方仓库中的release binary而不是APT源中的arm-none-eabi-gcc
[GCC ARM Embedded](https://launchpad.net/gcc-arm-embedded)
下载二进制文件解压，然后往系统变量path写入arm-none-eabi-gcc路径

为什么使用官方仓库？因为它包含了newlib，而apt不包含它，这个newlib库专门为小型嵌入式设备支持标准的C语言IO操作，例如fopen fprintf等，而不必编译系统的库函数导致code空间大增。

[stackflow讨论newlib问题](http://stackoverflow.com/questions/26931979/gnu-arm-nano-specs-not-found)
> I solved the problem. I was just using arm-none-eabi-xxx packages provided by Ubuntu, not original ones - that was the problem. All you need to do is simply download packages from toolchain's website and install them. They work just fine!


#### Eclipse project

在Eclipse中新建一个C工程为"Makefile project"指定toolchain为Other toolchain。

然后打开工程属性properties，左侧定位C/C++ Build->Enviorment
右侧将C_INCLUDE_PATH改为arm-none-eabi-gcc的include路径，例如我的是

	/home/li/programs/toolchain/gcc-arm-none-eabi-5_3-2016q1/arm-none-eabi/include
    
新建一个PATH变量，填入系统path加上arm-none-eabi-gcc的绝对路径,下面就是该变量的值。等价于在shell执行export PATH=$PATH:/home/......

	${PATH}:/home/li/programs/toolchain/gcc-arm-none-eabi-5_3-2016q1/bin
    
这样可以右键工程名->Build Project来实现调用Makefile编译。

由于是CROSS-compile交叉编译，实际使用中还是发现不少type not resolved错误，属于正常现象，可以将其关闭：project settings中的code analysis右边“使用工程设置”->取消symbol not resolved和type not resolved

#### Debug via JLink
安装[GNU ARM Ecplise插件](https://gnuarmeclipse.github.io/)

安装时候选择合适的套件安装，我只安装了
- CodeRed Debug Perspective
- JLink Debugging
- STM32Fx Project Templates

记得要更新CDT的插件，把Option plugins的这个也装上
- C/C++ GBD Hardware Debugging

去[Jlink官网](https://www.segger.com/jlink-software.html)下载专用的Linux驱动。这样就能在linux下正确识别jlink了。

在Eclipse中Debug Configurations，新建一个GDB SEGGER J-link项目，然后在修改

右侧Debugger标签中
- Executebale填入/usr/bin/JLinkGDBServer
- GDB Client Setup中填入arm-none-eabi-gdb的绝对位置

Startup标签中
- SWO cpu freq为72000000Hz, SWO freq为3000000Hz, Port Mask为0x1

然后安装一个库，防止arm-none-eabi-gdb启动失败提示无法加载libncurses.so

	sudo apt-get install lib32ncurses5
    
实际上要参考GNU ARM Ecplise插件官网的使用说明，原文非常难看懂，字体好模糊。。。

#### make clean

由于实际应用中很少修改libstm32.a这个库文件，为节省编译时间，我将clean动作设定为不删掉libstm32.a，想彻底clean可以执行

	make cleanall

#### use JLink

安装 openocd

	sudo apt-get install openocd

插上jlink，添加白名单。
	
    lsusb
    # 记下当前的jlink的硬件id，例如，ID 1366:0101
	sudo gedit /etc/udev/rules.d/xxxxxx.rules
    # 添加一行
	SYSFS{idProduct}=="0101", SYSFS{idVendor}=="1366", MODE="666", GROUP="plugdev"
    
这样权限666,使用openocd就不用sudo了. 
拔下，插上一次.

本仓库使用原作者的perl脚本，直接执行就能烧写bin文件

	./do_flash.pl bin/main.bin    
    
我直接往Makefile加入烧录命令了，下次直接make flash就可以实现上面这句功能

	make flash

### Mac OS

You need some tools as follows:

* `stm32flash` </br>
Flash program for the STM32 ARM processors using the ST serial bootloader over UART or I2C
* `coreutils` </br>
Readlink
* `gcc-arm-none-eabi`

#### stm32flash
install some tools that `make` need:

    $ brew install autoconf
    $ brew install automake
    $ brew install pkg-config


Then install stm32flash:


    $ git clone https://github.com/ARMinARM/stm32flash
    $ cd stm32flash
    $ make


install coreutils

	$ brew install coreutils

install gcc-arm-none-eabi

	$ brew install gcc-arm-none-eabi

change $(TOP)

    $ cd stm32_development_on_linux
    $ vi Makefile.common
	#Change line 32 to:
	TOP=$(shell greadlink -f "$(dir $(lastword $(MAKEFILE_LIST)))")




