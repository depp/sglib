#include "pixbuf.h"
#include "type_impl.h"
#include <ApplicationServices/ApplicationServices.h>

struct sg_layout_impl {
    CTLineRef line;
};

struct sg_layout_impl *
sg_layout_impl_new(struct sg_layout *lp)
{
    struct sg_layout_impl *li;
    CFStringRef string = NULL;
    CGColorRef white = NULL;
    CTFontRef font = NULL;
    CFStringRef keys[2], fname;
    CFTypeRef vals[2];
    CFDictionaryRef attr = NULL;
    CFAttributedStringRef attrstring = NULL;
    CTLineRef line = NULL;

    string = CFStringCreateWithBytes(
        kCFAllocatorDefault, (const UInt8 *) lp->text, lp->textlen,
        kCFStringEncodingUTF8, false);
    white = CGColorCreateGenericGray(1.0f, 1.0f);
    if (lp->family) {
        fname = CFStringCreateWithBytes(
            NULL, (UInt8 *) lp->family, strlen(lp->family),
            kCFStringEncodingUTF8, false);
        font = CTFontCreateWithName(fname, lp->size, NULL);
        CFRelease(fname);
    }
    font = CTFontCreateWithName(CFSTR("Helvetica"), lp->size, NULL);

    keys[0] = kCTFontAttributeName;
    vals[0] = font;
    keys[1] = kCTForegroundColorAttributeName;
    vals[1] = white;

    attr = CFDictionaryCreate(
        kCFAllocatorDefault, (const void **) keys, (const void **) vals, 2,
        &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    attrstring = CFAttributedStringCreate(kCFAllocatorDefault, string, attr);
    line = CTLineCreateWithAttributedString(attrstring);

    CFRelease(attrstring);
    CFRelease(attr);
    CFRelease(font);
    CFRelease(white);
    CFRelease(string);

    li = malloc(sizeof(*li));
    if (!li) abort();
    li->line = line;

    return li;
}

void
sg_layout_impl_free(struct sg_layout_impl *li)
{
    if (li->line)
        CFRelease(li->line);
    free(li);
}

static void
sg_layout_copyrect(struct sg_layout_rect *dest, CGRect *src)
{
    int x1, x2, y1, y2;
    x1 = floorf(CGRectGetMinX(*src));
    y1 = floorf(CGRectGetMinY(*src));
    x2 = ceilf(CGRectGetMaxX(*src));
    y2 = ceilf(CGRectGetMaxY(*src));
    dest->x = x1;
    dest->y = y1;
    dest->width = x2 - x1;
    dest->height = y2 - y1;
}

static CGContextRef
sg_layout_dummycontext(void)
{
    static CGContextRef dummy_context;
    static unsigned char dummy_context_data;
    if (!dummy_context) {
        CGColorSpaceRef color_space =
            CGColorSpaceCreateWithName(kCGColorSpaceGenericGray);
        dummy_context = CGBitmapContextCreate(
            &dummy_context_data, 1, 1, 8, 1, color_space, 0);
        CGColorSpaceRelease(color_space);
        if (!dummy_context)
            abort();
    }
    return dummy_context;
}

void
sg_layout_impl_calcbounds(struct sg_layout_impl *li,
                          struct sg_layout_bounds *b)
{
    CGRect ibounds;

    ibounds = CTLineGetImageBounds(li->line, sg_layout_dummycontext());
    b->x = 0;
    b->y = 0;
    sg_layout_copyrect(&b->ibounds, &ibounds);
}

void
sg_layout_impl_render(struct sg_layout_impl *li, struct sg_pixbuf *pbuf,
                      int xoff, int yoff)
{
    CGColorSpaceRef color_space = NULL;
    CGContextRef context = NULL;
    CTLineRef line = li->line;

    color_space = CGColorSpaceCreateWithName(kCGColorSpaceGenericGray);
    context = CGBitmapContextCreate(
        pbuf->data, pbuf->pwidth, pbuf->pheight, 8, pbuf->rowbytes,
        color_space, 0);
    CGContextSetTextPosition(context, xoff, yoff);
    CTLineDraw(line, context);

    CFRelease(color_space);
    CFRelease(context);
}
