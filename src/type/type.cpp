#include "type.hpp"
#include <CoreGraphics/CoreGraphics.h>
#include <CoreText/CoreText.h>
#include "SDL_opengl.h"
#include <stdlib.h>

Type::Type()
    : text_(), textureLoaded_(false), texture_(0),
      vx1_(0), vx2_(0), vy1_(0), vy2_(0),
      tx1_(0), tx2_(0), ty1_(0), ty2_(0)
{ }

Type::~Type()
{ }

void Type::setText(std::string const &text)
{
    if (text == text_)
        return;
    text_ = text;
    textureLoaded_ = false;
}

void Type::draw()
{

    glPushAttrib(GL_ENABLE_BIT);
    glEnable(GL_TEXTURE_2D);
    load();
    if (texture_) {
        glBindTexture(GL_TEXTURE_2D, texture_);
        // glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
        glBegin(GL_TRIANGLE_STRIP);
        glTexCoord2f(tx1_, ty1_); glVertex2f(vx1_, vy1_);
        glTexCoord2f(tx2_, ty1_); glVertex2f(vx2_, vy1_);
        glTexCoord2f(tx1_, ty2_); glVertex2f(vx1_, vy2_);
        glTexCoord2f(tx2_, ty2_); glVertex2f(vx2_, vy2_);
        glEnd();
    }
    glPopAttrib();
}

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

void Type::load()
{
    if (textureLoaded_)
        return;
    if (text_.empty()) {
        if (texture_) {
            glDeleteTextures(1, &texture_);
            texture_ = 0;
        }
        textureLoaded_ = true;
        return;
    }

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
    unsigned int width  = vx2_ - vx1_, twidth  = round_up_pow2(width );
    unsigned int height = vy2_ - vy1_, theight = round_up_pow2(height);
    int xoff = (twidth - width) / 2, yoff = (theight - height) / 2;
    float xscale = 1.0f / (float)twidth, yscale = 1.0f / (float)theight;
    tx1_ = xoff * xscale;
    tx2_ = (xoff + width) * xscale;
    ty1_ = (theight - yoff) * yscale;
    ty2_ = (theight - yoff - height) * yscale;
    void *data = malloc(twidth * theight);
    if (!data)
        return;
    CGColorSpaceRef colorSpace =
        CGColorSpaceCreateWithName(kCGColorSpaceGenericGray);
    CGContextRef context = CGBitmapContextCreate(
        data, twidth, theight, 8, twidth, colorSpace, 0);
    CFRelease(colorSpace);
    CGRect crect = { { 0, 0 }, { twidth, theight } };
    CGContextSetFillColorWithColor(context, black);
    CGContextFillRect(context, crect);
    CGContextSetTextPosition(context, xoff - vx1_, yoff - vy1_);
    CTLineDraw(line, context);
    // CGContextFlush(context);
    CFRelease(line);
    CFRelease(context);
    CFRelease(white);
    CFRelease(black);
    if (!texture_) {
        glGenTextures(1, &texture_);
        glBindTexture(GL_TEXTURE_2D, texture_);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    } else
        glBindTexture(GL_TEXTURE_2D, texture_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, twidth, theight, 0,
                 GL_ALPHA, GL_UNSIGNED_BYTE, data);
    /*
    gluBuild2DMipmaps( GL_TEXTURE_2D, GL_LUMINANCE, twidth, theight,
                       GL_LUMINANCE, GL_UNSIGNED_BYTE, data );
    */
    free(data);
    textureLoaded_ = true;
}

void Type::unload()
{
    if (texture_)
        glDeleteTextures(1, &texture_);
}
