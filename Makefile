
all: maze3d.sms

maze3d.sms: maze3d.c
	zcc +sms maze3d.c -o maze3d.sms -m
	map2sym maze3d.map maze3d.sym
#	smshead maze3d.sms
	
clean:
	rm *.sms *.sav *.asm *.sym *.rel *.noi *.map *.lst *.lk *.ihx data.*
