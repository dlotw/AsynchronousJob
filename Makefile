obj-m += sys_submitjob.o

INC=/lib/modules/$(shell uname -r)/build/arch/x86/include

sys_submitjob-y :=core.o functions.o features.o

all: submitjob xproducer

submitjob:
	make -Wall -Werror -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

xproducer: xproducer.c
	gcc -Wall -Werror -I$(INC)/generated/uapi -I$(INC)/uapi xproducer.c -o xproducer

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -f xproducer
