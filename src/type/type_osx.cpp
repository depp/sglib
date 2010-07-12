#include "type.hpp"
#include <CoreGraphics/CoreGraphics.h>
#include <CoreText/CoreText.h>
#include <stdlib.h>

static CGContextRef getDummyContext()
{
    static CGContextRef dummyContext = NULL;
    static unsigned char dummyContextData = 0;
    if (!dummyContext) {
        CGColorSpaceRef colorSpace =
            CGColorSpaceCreateWithName(kCGColorSpaceGenericGray);
        dummyContext = CGBitmapContextCreate(
            &dummyContextData, 1, 1, 8, 1, colorSpace, 0);
        CGColorSpaceRelease(colorSpace);
    }
    return dummyContext;
}

static unsigned int round_up_pow2(unsigned int x)
{
    x -= 1;
    x |= x >> 16;
    x |= x >> 8;
    x |= x >> 4;
    x |= x >> 2;
    x |= x >> 1;
    return x + 1;
}

void Type::loadImage(void **data, unsigned int *width,
                     unsigned int *height)
{
    CFStringRef string = CFStringCreateWithBytes(
        kCFAllocatorDefault, (const UInt8 *)text_.data(), text_.size(),
        kCFStringEncodingUTF8, false);
    CGColorRef white = CGColorCreateGenericGray(1.0f, 1.0f);
    CGColorRef black = CGColorCreateGenericGray(0.0f, 1.0f);
    CTFontRef font = CTFontCreateWithName(CFSTR("Courier"), 12.0f, NULL);
    CFStringRef keys[] = { kCTFontAttributeName,
                           kCTForegroundColorAttributeName };
    CFTypeRef values[] = { font, white };
    CFDictionaryRef attributes = CFDictionaryCreate(
        kCFAllocatorDefault, (const void **)keys, (const void **)values, 2,
        &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    CFRelease(font);
    CFAttributedStringRef attrString =
        CFAttributedStringCreate(kCFAllocatorDefault, string, attributes);
    CFRelease(string);
    CFRelease(attributes);
    CTLineRef line = CTLineCreateWithAttributedString(attrString);
    CFRelease(attrString);
    CGRect bounds = CTLineGetImageBounds(line, getDummyContext());
    vx1_ = floorf(CGRectGetMinX(bounds));
    vx2_ = ceilf (CGRectGetMaxX(bounds));
    vy1_ = floorf(CGRectGetMinY(bounds));
    vy2_ = ceilf (CGRectGetMaxY(bounds));
    unsigned int iwidth  = vx2_ - vx1_, twidth  = round_up_pow2(iwidth );
    unsigned int iheight = vy2_ - vy1_, theight = round_up_pow2(iheight);
    int xoff = (twidth - iwidth) / 2, yoff = (theight - iheight) / 2;
    float xscale = 1.0f / (float)twidth, yscale = 1.0f / (float)theight;
    tx1_ = xoff * xscale;
    tx2_ = (xoff + iwidth) * xscale;
    ty1_ = (theight - yoff) * yscale;
    ty2_ = (theight - yoff - iheight) * yscale;
    void *ptr = malloc(twidth * theight);
    if (!ptr)
        return;
    CGColorSpaceRef colorSpace =
        CGColorSpaceCreateWithName(kCGColorSpaceGenericGray);
    CGContextRef context = CGBitmapContextCreate(
        ptr, twidth, theight, 8, twidth, colorSpace, 0);
    CFRelease(colorSpace);
    CGRect crect = { { 0, 0 }, { twidth, theight } };
    CGContextSetFillColorWithColor(context, black);
    CGContextFillRect(context, crect);
    CGContextSetTextPosition(context, xoff - vx1_, yoff - vy1_);
    CTLineDraw(line, context);
    CFRelease(line);
    CFRelease(context);
    CFRelease(white);
    CFRelease(black);
    *data = ptr;
    *width = twidth;
    *height = theight;
}
