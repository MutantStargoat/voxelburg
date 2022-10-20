	.section .rodata

	.globl color_pixels
	.globl color_cmap
	.globl height_pixels
	.globl spr_game_pixels
	.globl spr_game_cmap

	.align 1
color_pixels:
	.incbin "data/color.raw"
	.align 1
color_cmap:
	.incbin "data/color.pal"
	.align 1
height_pixels:
	.incbin "data/height.raw"

	.align 1
spr_game_pixels:
	.incbin "data/spr_game.raw"
	.align 1
spr_game_cmap:
	.incbin "data/spr_game.pal"
