#ifndef CLIENT_TYPE_FONT_HPP
#define CLIENT_TYPE_FONT_HPP
class RasterText;

class Font {
    friend class RasterText;
public:
    enum Style {
        StyleNormal,
        Italic,
        Oblique
    };
    enum Variant {
        VariantNormal,
        SmallCaps
    };
    static const int
        WeightThin = 100,
        WeightUltralight = 200,
        WeightLight = 300,
        WeightBook = 380,
        WeightNormal = 400,
        WeightMedium = 500,
        WeightSemibold = 600,
        WeightBold = 700,
        WeightUltrabold = 800,
        WeightHeavy = 900,
        WeightUltraheavy = 1000;

    Font() : info_(0) { }
    Font(Font const &f);
    ~Font();
    Font &operator=(Font const &f);

    void setFamily(char const *const names[]);
    void setSize(float size);
    void setStyle(Style style);
    void setVariant(Variant variant);
    void setWeight(int weight);

private:
    void mkinfo();
    struct Info;
    Info *info_;
};

#endif
