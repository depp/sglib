/* Copyright 2012-2013 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "sg/error.h"
#include "sg/pixbuf.h"
#include "sg/type.h"
#include <ApplicationServices/ApplicationServices.h>

static const float FRAME_HEIGHT = 2048.0f;

struct sg_textbitmap {
    sg_textalign_t alignment;

    /* Simple (no wrap) version */
    CTLineRef line;

    /* Version which supports wrapping */
    CTFramesetterRef framesetter;
    CTFrameRef frame;
};

/* Create a string attribute dictionary from the given parameters.  */
static CFDictionaryRef
sg_textbitmap_makeattr(const char *fontname, double fontsize,
                       sg_textalign_t alignment)
{
    CGColorRef white = NULL;
    CFStringRef fname = NULL;
    CTFontRef font = NULL;
    CTTextAlignment ctAlignment;
    CTParagraphStyleSetting setting;
    CTParagraphStyleRef paragraphStyle = NULL;
    CFStringRef keys[3];
    CFTypeRef vals[3];
    CFDictionaryRef attr = NULL;

    fontname = "Helvetica";

    white = CGColorCreateGenericGray(1.0f, 1.0f);
    if (!white) goto error;

    fname = CFStringCreateWithBytes(
        NULL, (UInt8 *) fontname, strlen(fontname),
        kCFStringEncodingUTF8, false);
    if (!fname) goto error;
    font = CTFontCreateWithName(fname, fontsize, NULL);
    if (!font) goto error;

    switch (alignment) {
    default:
    case SG_TEXTALIGN_LEFT: ctAlignment = kCTLeftTextAlignment; break;
    case SG_TEXTALIGN_RIGHT: ctAlignment = kCTRightTextAlignment; break;
    case SG_TEXTALIGN_CENTER: ctAlignment = kCTCenterTextAlignment; break;
    }
    setting.spec = kCTParagraphStyleSpecifierAlignment;
    setting.valueSize = sizeof(ctAlignment);
    setting.value = &ctAlignment;
    paragraphStyle = CTParagraphStyleCreate(&setting, 1);
    if (!paragraphStyle) goto error;

    keys[0] = kCTFontAttributeName;
    vals[0] = font;
    keys[1] = kCTForegroundColorAttributeName;
    vals[1] = white;
    keys[2] = kCTParagraphStyleAttributeName;
    vals[2] = paragraphStyle;

    attr = CFDictionaryCreate(
        kCFAllocatorDefault, (const void **) keys, (const void **) vals, 2,
        &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    if (!attr) goto error;

    CFRelease(paragraphStyle);
    CFRelease(font);
    CFRelease(fname);
    CFRelease(white);
    return attr;

error:
    if (attr) CFRelease(attr);
    if (paragraphStyle) CFRelease(paragraphStyle);
    if (font) CFRelease(font);
    if (fname) CFRelease(fname);
    if (white) CFRelease(white);
    return NULL;
}

/* Create an attributed string from the given attributes and UTF-8 text.  */
static CFAttributedStringRef
sg_textbitmap_makestring(const char *text, size_t textlen,
                         CFDictionaryRef attributes)
{
    CFStringRef string = NULL;
    CFAttributedStringRef attrstring = NULL;

    string = CFStringCreateWithBytes(
        kCFAllocatorDefault, (const UInt8 *) text, textlen,
        kCFStringEncodingUTF8, false);
    if (!string) goto error;

    attrstring = CFAttributedStringCreate(
        kCFAllocatorDefault, string, attributes);
    if (!attrstring) goto error;

    CFRelease(string);
    return attrstring;

error:
    if (string) CFRelease(string);
    if (attrstring) CFRelease(attrstring);
    return NULL;
}

/* Create a frame based text bitmap.  */
static int
sg_textbitmap_makeframe(struct sg_textbitmap *bitmap,
                        CFAttributedStringRef string, double width)
{
    CGRect bounds;
    CTFramesetterRef framesetter = NULL;
    CTFrameRef frame = NULL;
    CGMutablePathRef path = NULL;

    bounds = CGRectMake(0.0, 0.0, width, FRAME_HEIGHT);
    path = CGPathCreateMutable();
    if (!path) goto error;
    CGPathAddRect(path, NULL, bounds);
    framesetter = CTFramesetterCreateWithAttributedString(string);
    if (!framesetter) goto error;
    frame = CTFramesetterCreateFrame(
        framesetter, CFRangeMake(0, CFAttributedStringGetLength(string)),
        path, NULL);
    if (!frame) goto error;

    CFRelease(path);
    bitmap->framesetter = framesetter;
    bitmap->frame = frame;
    return 0;

error:
    if (frame) CFRelease(frame);
    if (framesetter) CFRelease(framesetter);
    if (path) CFRelease(path);
    return -1;
}

/* Create a line based text bitmap.  */
static int
sg_textbitmap_makeline(struct sg_textbitmap *bitmap,
                       CFAttributedStringRef string)
{
    CTLineRef line;
    line = CTLineCreateWithAttributedString(string);
    if (!line)
        return -1;
    bitmap->line = line;
    return 0;
}

struct sg_textbitmap *
sg_textbitmap_new_simple(const char *text, size_t textlen,
                         const char *fontname, double fontsize,
                         sg_textalign_t alignment, double width,
                         struct sg_error **err)
{
    struct sg_textbitmap *bitmap = NULL;
    CFDictionaryRef attr = NULL;
    CFAttributedStringRef str = NULL;

    attr = sg_textbitmap_makeattr(fontname, fontsize, alignment);
    if (!attr) goto error;
    str = sg_textbitmap_makestring(text, textlen, attr);
    if (!str) goto error;

    bitmap = calloc(1, sizeof(*bitmap));
    if (!bitmap) goto error;

    bitmap->alignment = alignment;
    if (width > 0.0) {
        if (sg_textbitmap_makeframe(bitmap, str, width))
            goto error;
    } else {
        if (sg_textbitmap_makeline(bitmap, str))
            goto error;
    }

    CFRelease(str);
    CFRelease(attr);
    return bitmap;

error:
    if (str) CFRelease(str);
    if (attr) CFRelease(attr);
    free(bitmap);
    sg_error_nomem(err);
    return NULL;
}

void
sg_textbitmap_free(struct sg_textbitmap *bitmap)
{
    if (bitmap->line)
        CFRelease(bitmap->line);
    if (bitmap->framesetter)
        CFRelease(bitmap->framesetter);
    if (bitmap->frame)
        CFRelease(bitmap->frame);
    free(bitmap);
}

static void
sg_textbitmap_copyrect(struct sg_textrect *dest, CGRect *src)
{
    dest->x0 = floorf(CGRectGetMinX(*src));
    dest->y0 = floorf(CGRectGetMinY(*src));
    dest->x1 = ceilf(CGRectGetMaxX(*src));
    dest->y1 = ceilf(CGRectGetMaxY(*src));
}

static CGContextRef
sg_textbitmap_dummycontext(void)
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

static CGRect
sg_textbitmap_ctline_pixbounds(CTLineRef line, CGPoint off, CGContextRef cxt)
{
    CGRect r = CTLineGetImageBounds(line, cxt);
    r.origin.x += off.x;
    r.origin.y += off.y;
    return r;
}

static CGRect
sg_textbitmap_ctline_logbounds(CTLineRef line, CGPoint off)
{
    double lwidth;
    CGFloat ascent, descent;
    CGRect r;
    lwidth = CTLineGetTypographicBounds(line, &ascent, &descent, NULL);
    r.origin.x = off.x;
    r.origin.y = off.y - descent;
    r.size.width = lwidth;
    r.size.height = ascent + descent;
    return r;
}

int
sg_textbitmap_getmetrics(struct sg_textbitmap *bitmap,
                         struct sg_textlayout_metrics *metrics,
                         struct sg_error **err)
{
    CGContextRef cxt = sg_textbitmap_dummycontext();
    CGRect pixbounds, logbounds, r;
    CFArrayRef array;
    CGPoint *origins, origin;
    CFIndex n, i;
    CTLineRef line;
    if (bitmap->frame) {
        array = CTFrameGetLines(bitmap->frame);
        if (!array) goto nomem;
        n = CFArrayGetCount(array);
        if (n > 0) {
            origins = malloc(sizeof(*origins) * n);
            if (!origins) goto nomem;
            CTFrameGetLineOrigins(bitmap->frame, CFRangeMake(0, n), origins);
            line = CFArrayGetValueAtIndex(array, 0);
            origin = origins[0];
            pixbounds = sg_textbitmap_ctline_pixbounds(line, origin, cxt);
            logbounds = sg_textbitmap_ctline_logbounds(line, origin);
            for (i = 1; i < n; ++i) {
                line = CFArrayGetValueAtIndex(array, i);
                origin = origins[i];
                r = sg_textbitmap_ctline_pixbounds(line, origin, cxt);
                pixbounds = CGRectUnion(pixbounds, r);
                r = sg_textbitmap_ctline_logbounds(line, origin);
                logbounds = CGRectUnion(logbounds, r);
            }
            metrics->baseline = (int) floorf(origins[0].y + 0.5f);
            free(origins);
        } else {
            pixbounds = CGRectZero;
            logbounds = CGRectZero;
            metrics->baseline = 0;
        }
    } else {
        line = bitmap->line;
        pixbounds = CTLineGetImageBounds(line, cxt);
        logbounds = sg_textbitmap_ctline_logbounds(line, CGPointZero);
        metrics->baseline = 0;
    }
    sg_textbitmap_copyrect(&metrics->pixel, &pixbounds);
    sg_textbitmap_copyrect(&metrics->logical, &logbounds);
    return 0;

nomem:
    sg_error_nomem(err);
    return -1;
}

int
sg_textbitmap_render(struct sg_textbitmap *bitmap,
                     struct sg_pixbuf *pixbuf,
                     struct sg_textpoint offset,
                     struct sg_error **err)
{
    CGColorSpaceRef color_space = NULL;
    CGContextRef context = NULL;

    color_space = CGColorSpaceCreateWithName(kCGColorSpaceGenericGray);
    if (!color_space) goto nomem;
    context = CGBitmapContextCreate(
        pixbuf->data, pixbuf->width, pixbuf->height, 8, pixbuf->rowbytes,
        color_space, 0);
    if (!context) goto nomem;

    if (bitmap->frame) {
        CGContextTranslateCTM(context, offset.x, offset.y);
        CTFrameDraw(bitmap->frame, context);
    } else {
        CGContextSetTextPosition(context, offset.x, offset.y);
        CTLineDraw(bitmap->line, context);
    }

    CFRelease(color_space);
    CFRelease(context);

    return 0;

nomem:
    sg_error_nomem(err);
    if (color_space) CFRelease(color_space);
    if (context) CFRelease(context);
    return -1;
}
