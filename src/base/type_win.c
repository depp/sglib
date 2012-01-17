
void
sg_layout_calcbounds(struct sg_layout *lp, struct sg_layout_bounds *b)
{
    b->x = 0;
    b->y = 0;
    b->ibounds.x = 0;
    b->ibounds.y = 0;
    b->ibounds.width = 16;
    b->ibounds.height = 16;
}

/* Render the layout at the given location in a pixel buffer.  */
void
sg_layout_render(struct sg_layout *lp, struct sg_pixbuf *pbuf,
                 int xoff, int yoff)
{

}
