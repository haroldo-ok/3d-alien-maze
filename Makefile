PRJNAME := maze3d
OBJS := data.rel maze3d.rel

all: $(PRJNAME).sms

data.c: data/* data/monster_full_tiles.psgcompr data/monster_half_tiles.psgcompr
	folder2c data data
	
data/monster_full_tiles.psgcompr: data/img/monster_full.png
	BMP2Tile.exe data/img/monster_full.png -noremovedupes -8x16 -palsms -fullpalette -savetiles data/monster_full_tiles.psgcompr -savepalette data/monster_full_palette.bin

data/monster_half_tiles.psgcompr: data/img/monster_half.png
	BMP2Tile.exe data/img/monster_half.png -noremovedupes -8x16 -palsms -fullpalette -savetiles data/monster_half_tiles.psgcompr
	
%.rel : %.c
	sdcc -c -mz80 --peep-file lib/peep-rules.txt $<

$(PRJNAME).sms: $(OBJS)
	sdcc -o $(PRJNAME).ihx -mz80 --no-std-crt0 --data-loc 0xC000 lib/crt0_sms.rel $(OBJS) SMSlib.lib
	ihx2sms $(PRJNAME).ihx $(PRJNAME).sms	

clean:
	rm *.sms *.sav *.asm *.sym *.rel *.noi *.map *.lst *.lk *.ihx data.*
