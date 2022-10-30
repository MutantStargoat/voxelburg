	.section .rodata

	.globl color_pixels
	.globl color_cmap
	.globl color_gba_cmap
	.globl height_pixels
	.globl spr_game_pixels
	.globl spr_game_cmap
	.globl menuscr_pixels
	.globl menuscr_cmap
	.globl menuscr_gba_cmap
	.globl spr_menu_pixels
	.globl spr_menu_cmap
	.globl spr_logo_pixels
	.globl spr_logo_cmap

	.align 1
color_pixels:
	.incbin "data/color.raw"

	.align 1
color_cmap:
	.incbin "data/color.pal"

	.align 1
color_gba_cmap:
	.incbin "data/color.gpal"

	.align 1
height_pixels:
	.incbin "data/height.raw"

	.align 1
spr_game_pixels:
	.incbin "data/spr_game.raw"

	.align 1
spr_game_cmap:
	.incbin "data/spr_game.pal"

	.align 1
menuscr_pixels:
	.incbin "data/menuscr.raw"

	.align 1
menuscr_cmap:
	.incbin "data/menuscr.pal"

	.align 1
menuscr_gba_cmap:
	.incbin "data/menuscr.gpal"

	.align 1
spr_menu_pixels:
	.incbin "data/spr_menu.raw"

	.align 1
spr_menu_cmap:
	.incbin "data/spr_menu.pal"

	.align 1
spr_logo_pixels:
	.incbin "data/spr_logo.raw"

	.align 1
spr_logo_cmap:
	.incbin "data/spr_logo.pal"
