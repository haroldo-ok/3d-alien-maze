PRJNAME := maze3d
OBJS := data.rel maze3d.rel

all: $(PRJNAME).sms

data.c: data/*
	folder2c data data
	
%.rel : %.c
	sdcc -c -mz80 --peep-file peep-rules.txt $<

$(PRJNAME).sms: $(OBJS)
	sdcc -o $(PRJNAME).ihx -mz80 --no-std-crt0 --data-loc 0xC000 crt0_sms.rel $(OBJS) SMSlib.lib PSGlib.rel
	ihx2sms $(PRJNAME).ihx $(PRJNAME).sms	

clean:
	rm *.sms *.sav *.asm *.sym *.rel *.noi *.map *.lst *.lk *.ihx data.*
