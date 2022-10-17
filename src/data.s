	.section .rodata

	.globl color_pixels
	.globl color_cmap
	.globl height_pixels

	.align 1
color_pixels:
	.incbin "data/color.raw"
	.align 1
color_cmap:
	.incbin "data/color.pal"
	.align 1
height_pixels:
	.incbin "data/height.raw"
