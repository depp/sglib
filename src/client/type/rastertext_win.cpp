#define UNICODE

#include "rastertext.hpp"
#include "sys/error_win.hpp"
#include <Windows.h>
#include <usp10.h>

Font::Font(Font const &f)
{ }

Font::~Font()
{ }

Font &Font::operator=(Font const &f)
{
    return *this;
}

bool RasterText::loadTexture()
{
    return true;
}

#if 0

static HDC gDC;

// FIXME unicode-ize this
bool RasterText::loadTexture()
{
    if (text_.empty())
        return true;

    if (!gDC) {
        gDC = CreateCompatibleDC(NULL);
        if (!gDC)
            throw error_win(GetLastError());
    }

    bool result = false;
    HDC dc = gDC;
    SCRIPT_STRING_ANALYSIS ssa = NULL;
    SIZE *ssz;
    unsigned sz = text_.size(), g = sz + sz/2 + 16;
    HRESULT hr;
    HBITMAP hbit = NULL;
    HFONT hfont = NULL;
    BITMAPINFO bmi;
    BITMAPINFOHEADER *hdr;
    void *bptr;

    // Cleartype is a possible quality, so is antialiased
    hfont = CreateFont(12, 0, 0, 0, FW_REGULAR, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_DONTCARE, L"Arial");
    if (!hfont)
        goto failed;

    SelectObject(dc, hfont);
    SetTextAlign(dc, TA_LEFT);

    sz = text_.size();
    gsz = sz + sz/2 + 16;
    hr = ScriptStringAnalyse(
        dc, text_.data(), text_.size(), gsz, ANSI_CHARSET,
        SSA_GLYPHS, 0, NULL, NULL, NULL, NULL, NULL, &ssa);
    if (hr != S_OK)
        goto failure;

    ssz = ScriptString_pSize(ssa);
    if (!ssz)
        goto failure;

    hdr = &bmi.bmiHeader;
    hdr->biSize = sizeof(bmi.bmiHeader);
    hdr->biWidth = ssz->cx;
    hdr->biHeight = ssz->cy;
    hdr->biPlanes = 1;
    hdr->biBitCount = 32;
    hdr->biCompression = BI_RGB;
    hdr->biSizeImage = 0;
    hdr->biXPelsPerMeter = 0;
    hdr->biClrUsed = 0;
    hdr->biClrImportant = 0;

    hbit = CreateDIBSection(dc, &bmi, DIB_RGB_COLORS, &bptr, NULL, 0);
    if (!hbit)
        goto failure;

    hr = ScriptStringOut(ssa, xpos, ypos, 0, NULL, 0, 0, FALSE);
    if (hr != S_OK)
        goto failure;

failure:
    if (hfont)
        DeleteObject(hfont);
    if (ssa)
        ScriptStringFree(&ssa);
    if (hbit)
        DeleteObject(hbit);
    return result;
}

#endif
