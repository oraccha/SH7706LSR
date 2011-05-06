#
# Copyright (C) 2011 National Institute of Advanced Industrial
# Science and Technology (AIST), Japan.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version
# 2 of the License.
#
ifneq ($(KERNELRELEASE),)
obj-m		:= shmin_led.o
else
KERNELDIR	?= /lib/modules/$(shell uname -r)/build

all:
	$(MAKE) -C $(KERNELDIR) M=$(shell pwd) modules

clean:
	-rm -f *.ko *.o *.mod.c .*.cmd *.symvers modules.order
	-rm -rf .tmp_versions
endif
