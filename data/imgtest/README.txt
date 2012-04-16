Image format test
=================

This is a set of images of several different kinds.  The purpose is to
test the capability and correctness of image loaders.

The original image was created as an 8-bit RGBA image in GIMP.  The
remaining images were derived from it using GIMP, Image Magick, and
pngquant.

    Filename	    Color   Alpha   Depth
    -------------------------------------
    png_i4.png      palette -       4
    png_i8.png	    palette -	    8
    png_ia4.png	    palette alpha   4
    png_ia8.png	    palette alpha   8
    png_rgb8.png    RGB	    -	    8
    png_rgb16.png   RGB	    -	    16
    png_rgba8.png   RGB	    alpha   8
    png_rgba16.png  RGB	    alpha   16
    png_y1.png	    gray    -	    1
    png_y2.png	    gray    -	    2
    png_y4.png	    gray    -	    4
    png_y8.png	    gray    -	    8
    png_y16.png	    gray    -	    16
    png_ya8.png	    gray    alpha   8
    png_ya16.png    gray    alpha   16
