#include "type_impl.h"
#include <CoreText/CoreText.h>
#include <CoreGraphics/CoreGraphics.h>

struct sg_layout_impl {
    CTLineRef line;
};

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
sg_layout_calcbounds(struct sg_layout *lp, struct sg_layout_bounds *b)
{
    CFStringRef string = NULL;
    CGColorRef white = NULL;
    CTFontRef font = NULL;
    CFStringRef keys[2];
    CFTypeRef vals[2];
    CFDictionaryRef attr = NULL;
    CFAttributedStringRef attrstring = NULL;
    CTLineRef line = NULL;
    CGRect ibounds;

    string = CFStringCreateWithBytes(
        kCFAllocatorDefault, (const UInt8 *) lp->text, lp->textlen,
        kCFStringEncodingUTF8, false);
    white = CGColorCreateGenericGray(1.0f, 1.0f);
    font = CTFontCreateWithName(CFSTR("Helvetica"), 16.0f, NULL);

    keys[0] = kCTFontAttributeName;
    vals[0] = font;
    keys[1] = kCTForegroundColorAttributeName;
    vals[1] = white;

    attr = CFDictionaryCreate(
        kCFAllocatorDefault, (const void **) keys, (const void **) vals, 2,
        &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    attrstring = CFAttributedStringCreate(kCFAllocatorDefault, string, attr);
    line = CTLineCreateWithAttributedString(attrstring);
    ibounds = CTLineGetImageBounds(line, sg_layout_dummycontext());
    b->x = 0;
    b->y = 0;
    sg_layout_copyrect(&b->ibounds, &ibounds);

    CFRelease(attrstring);
    CFRelease(attr);
    CFRelease(font);
    CFRelease(white);
    CFRelease(string);

    if (!lp->impl) {
        lp->impl = malloc(sizeof(struct sg_layout_impl));
        if (!lp->impl)
            abort();
        lp->impl->line = NULL;
    }
    lp->impl->line = line;
}

/* FIXME: lp->impl could be NULL? */
void
sg_layout_render(struct sg_layout *lp, struct sg_pixbuf *pbuf,
                 int xoff, int yoff)
{
    CGColorSpaceRef color_space = NULL;
    CGContextRef context = NULL;
    CTLineRef line = lp->impl->line;

    color_space = CGColorSpaceCreateWithName(kCGColorSpaceGenericGray);
    context = CGBitmapContextCreate(
        pbuf->data, pbuf->pwidth, pbuf->pheight, 8, pbuf->rowbytes,
        color_space, 0);
    CGContextSetTextPosition(context, xoff, yoff);
    CTLineDraw(line, context);

    CFRelease(color_space);
    CFRelease(context);
    CFRelease(line);

    lp->impl->line = NULL;
}
