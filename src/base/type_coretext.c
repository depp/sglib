#include "pixbuf.h"
#include "type_impl.h"
#include <ApplicationServices/ApplicationServices.h>

static const float FRAME_HEIGHT = 2048.0f;

struct sg_layout_impl {
    /* Simple (no wrap) version */
    CTLineRef line;

    /* Version which supports wrapping */
    CTFramesetterRef framesetter;
    CTFrameRef frame;
    float xoff, yoff;
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
    CTFramesetterRef framesetter = NULL;
    CTFrameRef frame = NULL;
    CGRect bounds;
    CGMutablePathRef path;

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

    if (lp->width >= 0) {
        bounds = CGRectMake(0.0, 0.0, lp->width, FRAME_HEIGHT);
        path = CGPathCreateMutable();
        CGPathAddRect(path, NULL, bounds);
        framesetter = CTFramesetterCreateWithAttributedString(attrstring);
        CFRelease(path);
        frame = CTFramesetterCreateFrame(
            framesetter, CFRangeMake(0, CFStringGetLength(string)), path, NULL);
    } else {
        line = CTLineCreateWithAttributedString(attrstring);
    }

    CFRelease(attrstring);
    CFRelease(attr);
    CFRelease(font);
    CFRelease(white);
    CFRelease(string);

    li = malloc(sizeof(*li));
    if (!li) abort();
    li->line = line;
    li->framesetter = framesetter;
    li->frame = frame;

    return li;
}

void
sg_layout_impl_free(struct sg_layout_impl *li)
{
    if (li->line)
        CFRelease(li->line);
    if (li->framesetter)
        CFRelease(li->framesetter);
    if (li->frame)
        CFRelease(li->frame);
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
    CGContextRef cxt = sg_layout_dummycontext();
    CGRect ibounds, r;
    CFArrayRef array;
    CGPoint *origins;
    CFIndex n, i;
    CTLineRef line;
    if (li->frame) {
        array = CTFrameGetLines(li->frame);
        n = CFArrayGetCount(array);
        if (n > 0) {
            origins = malloc(sizeof(*origins) * n);
            if (!origins) abort();
            CTFrameGetLineOrigins(li->frame, CFRangeMake(0, n), origins);
            line = CFArrayGetValueAtIndex(array, 0);
            ibounds = CTLineGetImageBounds(line, cxt);
            ibounds.origin.x += origins[0].x;
            ibounds.origin.y += origins[0].y;
            for (i = 1; i < n; ++i) {
                line = CFArrayGetValueAtIndex(array, i);
                r = CTLineGetImageBounds(line, cxt);
                r.origin.x += origins[i].x;
                r.origin.y += origins[i].y;
                ibounds = CGRectUnion(ibounds, r);
            }
            li->xoff = -origins[0].x;
            li->yoff = -origins[0].y;
            ibounds.origin.x += li->xoff;
            ibounds.origin.y += li->yoff;
            free(origins);
        } else {
            ibounds = CGRectZero;
        }
    } else {
        ibounds = CTLineGetImageBounds(li->line, cxt);
    }
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

    color_space = CGColorSpaceCreateWithName(kCGColorSpaceGenericGray);
    context = CGBitmapContextCreate(
        pbuf->data, pbuf->pwidth, pbuf->pheight, 8, pbuf->rowbytes,
        color_space, 0);

    if (li->frame) {
        xoff += li->xoff;
        yoff += li->yoff;
        CGContextTranslateCTM(context, xoff, yoff);
        CTFrameDraw(li->frame, context);
    } else {
        CGContextSetTextPosition(context, xoff, yoff);
        CTLineDraw(li->line, context);
    }

    CFRelease(color_space);
    CFRelease(context);
}
