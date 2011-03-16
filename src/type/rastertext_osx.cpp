#include "rastertext.hpp"
#include "sys/autocf_osx.hpp"
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

bool RasterText::loadTexture()
{
	AutoCF<CFStringRef> string(CFStringCreateWithBytes(
		kCFAllocatorDefault, (const UInt8 *)text_.data(), text_.size(),
		kCFStringEncodingUTF8, false));
	AutoCF<CGColorRef> white(CGColorCreateGenericGray(1.0f, 1.0f));
	AutoCF<CGColorRef> black(CGColorCreateGenericGray(0.0f, 1.0f));
	AutoCF<CTFontRef> font(CTFontCreateWithName(CFSTR("Courier"), 12.0f, NULL));
	CFStringRef keys[] = { kCTFontAttributeName,
						   kCTForegroundColorAttributeName };
	CFTypeRef values[] = { font, white };
	AutoCF<CFDictionaryRef> attributes(CFDictionaryCreate(
		kCFAllocatorDefault, (const void **)keys, (const void **)values, 2,
		&kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks));
	AutoCF<CFAttributedStringRef> attrString(
		CFAttributedStringCreate(kCFAllocatorDefault, string, attributes));
	AutoCF<CTLineRef> line(CTLineCreateWithAttributedString(attrString));
	CGRect bounds = CTLineGetImageBounds(line, getDummyContext());
	vx1_ = floorf(CGRectGetMinX(bounds));
	vx2_ = ceilf (CGRectGetMaxX(bounds));
	vy1_ = floorf(CGRectGetMinY(bounds));
	vy2_ = ceilf (CGRectGetMaxY(bounds));
	unsigned int iwidth  = vx2_ - vx1_, iheight = vy2_ - vy1_;
	alloc(iwidth, iheight, false, false);
	unsigned int twidth = this->twidth(), theight = this->theight();
	int xoff = (twidth - iwidth) / 2, yoff = (theight - iheight) / 2;
	float xscale = 1.0f / (float)twidth, yscale = 1.0f / (float)theight;
	tx1_ = xoff * xscale;
	tx2_ = (xoff + iwidth) * xscale;
	ty1_ = (theight - yoff) * yscale;
	ty2_ = (theight - yoff - iheight) * yscale;
	AutoCF<CGColorSpaceRef> colorSpace(
		CGColorSpaceCreateWithName(kCGColorSpaceGenericGray));
	AutoCF<CGContextRef> context(CGBitmapContextCreate(
		buf(), twidth, theight, 8, rowbytes(), colorSpace, 0));
	CGRect crect = { { 0, 0 }, { twidth, theight } };
	CGContextSetFillColorWithColor(context, black);
	CGContextFillRect(context, crect);
	CGContextSetTextPosition(context, xoff - vx1_, yoff - vy1_);
	CTLineDraw(line, context);
	return true;
}
