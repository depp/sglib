/* Copyright 2012 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#ifndef SGPP_VIEWPORT_HPP
#define SGPP_VIEWPORT_HPP

/* An object for managing the OpenGL viewport.  The constructor and
   destructor both call glViewport, so the viewport changes when it
   comes into scope and is restored when it leaves scope.  */
class Viewport {
    int m_x, m_y, m_width, m_height;
    Viewport *m_parent;

    Viewport &operator=(const Viewport &);
    Viewport(const Viewport &);

public:
    // Create the initial viewport, this should only be called by
    // implementation of sg_game_draw.
    Viewport(int x, int y, int width, int height);

    // Create a sub-viewport, any window can do this.
    Viewport(Viewport &v, int x, int y, int width, int height);

    // Restore the old viewport.
    ~Viewport();

    int width() const
    {
        return m_width;
    }

    int height() const
    {
        return m_height;
    }

    // Create an orthographic projection matrix which maps (0, 0) to
    // the bottom left of the viewport and (width, height) to the top
    // right of the viewport.
    void ortho() const;
};

#endif
