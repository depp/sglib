/* Copyright 2012 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#ifndef SGPP_LETTERBOX_HPP
#define SGPP_LETTERBOX_HPP

class Letterbox {
    int m_ow, m_oh;
    int m_iw, m_ih;
    int m_x, m_y, m_w, m_h;

public:
    // Set size of outer window, real pixels
    void setOSize(int w, int h);

    // Set size of inner window, virtual pixels
    void setISize(int w, int h);

    // Call glViewport and glScissor on the given slice
    void enable();

    // Reset changes
    void disable();

    // Convert coordinates to inner window
    void convert(int &x, int &y);

private:
    void recalc();
};

#endif
