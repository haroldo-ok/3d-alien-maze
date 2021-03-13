PRJNAME := maze3d
OBJS := data.rel maze3d.rel

all: $(PRJNAME).sms

data.c: data/* data/font.1bpp data/minimap_tiles.psgcompr \
	data/monster_full_tiles.psgcompr data/monster_half_tiles.psgcompr data/monster_quarter_tiles.psgcompr \
	data/defeat_tiles.psgcompr data/title_tiles.psgcompr 
	folder2c data data
	
data/monster_full_tiles.psgcompr: data/img/monster_full.png
	BMP2Tile.exe data/img/monster_full.png -noremovedupes -8x16 -palsms -fullpalette -savetiles data/monster_full_tiles.psgcompr -savepalette data/monster_full_palette.bin

data/monster_half_tiles.psgcompr: data/img/monster_half.png
	BMP2Tile.exe data/img/monster_half.png -noremovedupes -8x16 -palsms -fullpalette -savetiles data/monster_half_tiles.psgcompr

data/monster_quarter_tiles.psgcompr: data/img/monster_quarter.png
	BMP2Tile.exe data/img/monster_quarter.png -noremovedupes -8x16 -palsms -fullpalette -savetiles data/monster_quarter_tiles.psgcompr

data/title_tiles.psgcompr: data/img/title.png
	BMP2Tile.exe data/img/title.png -palsms -fullpalette -savetiles data/title_tiles.psgcompr -savetilemap data/title_tilemap.bin -savepalette data/title_palette.bin
	
data/defeat_tiles.psgcompr: data/img/defeat.png
	BMP2Tile.exe data/img/defeat.png -palsms -fullpalette -savetiles data/defeat_tiles.psgcompr -savetilemap data/defeat_tilemap.bin -savepalette data/defeat_palette.bin

data/minimap_tiles.psgcompr: data/img/minimap.png
	BMP2Tile.exe data/img/minimap.png -noremovedupes -savetiles data/minimap_tiles.psgcompr
	
data/font.1bpp: data/img/font.png
	BMP2Tile.exe data/img/font.png -noremovedupes -savetiles data/font.1bpp
	
%.vgm: %.wav
	psgtalk -r 512 -u 1 -m vgm $<

%.rel : %.c
	sdcc -c -mz80 --peep-file lib/peep-rules.txt $<

$(PRJNAME).sms: $(OBJS)
	sdcc -o $(PRJNAME).ihx -mz80 --no-std-crt0 --data-loc 0xC000 lib/crt0_sms.rel $(OBJS) SMSlib.lib lib/PSGlib.rel
	ihx2sms $(PRJNAME).ihx $(PRJNAME).sms	

clean:
	rm *.sms *.sav *.asm *.sym *.rel *.noi *.map *.lst *.lk *.ihx data.*
