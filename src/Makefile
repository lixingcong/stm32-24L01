# src Makefile
include ../Makefile.common

# Do not remove startup.o below, or it could not load.
VPATH=startup
OBJS=startup.o
# use newlib
OBJS+=newlib_stubs.o
CFLAGS_USE_NEWLIB=-ffunction-sections -fdata-sections -fno-builtin

# Write down your source folder and obj-files
VPATH+=demo
#	OBJS+=test_USB_ldd.o
	OBJS+=test_new_stack.o
#	OBJS+=test_a7190.o

VPATH+=drivers
OBJS+=usart.o
OBJS+=sys.o
OBJS+=SPI1.o
#OBJS+=SPI2.o
OBJS+=delay.o
#OBJS+=my_console.o
OBJS+=timer2.o

VPATH+=drivers/pc_control
OBJS+=usart_scanf_irq.o
#OBJS+=execute_PC_cmd.o
#OBJS+=parse_PC_cmd.o

VPATH+=drivers/a7190
OBJS+=spi1_irq.o
OBJS+=A7190.o

VPATH+=drivers/route_table
#OBJS+=route_table.o
#OBJS+=apl_custom_function.o
#OBJS+=route_ping.o

#VPATH+=drivers/lmx2581
#OBJS+=lmx2581.o
#OBJS+=ctl_lmx2581.o

#VPATH+=drivers/usb
#OBJS+=usb_desc.o
#OBJS+=usb_prop.o
#OBJS+=usb_pwr.o
#OBJS+=hw_config.o
#OBJS+=usb_istr.o
#OBJS+=usb_endp.o
#OBJS+=usbio.o

OBJS+=stm32f10x_it.o

all: src
src: app.a

app.a: $(OBJS)
	cd startup && $(AR) cr ../app.a $(OBJS)
$(OBJS):%.o: %.c
	$(CC) -c $< -o startup/$*.o $(CFLAGS) $(CFLAGS_USE_NEWLIB)


.PHONY: src clean tshow

clean:
	rm -f app.a 
	rm -f startup/*.o
tshow:
	@echo "-------------------------------------------------"
	@echo "optimize settings: $(InfoTextLib), $(InfoTextSrc)"
	@echo "-------------------------------------------------"
