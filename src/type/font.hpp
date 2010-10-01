#ifndef TYPE_FONT_HPP
#define TYPE_FONT_HPP

class Font {
public:
    Font() : info_(0) { }
    Font(Font const &f);
    ~Font();
    Font &operator=(Font const &f);

    void setFamily(char const *const names[]);
    void setSize(float size);

private:
    void mkinfo();
    struct Info;
    Info *info_;
};

#endif
