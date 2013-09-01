/* Copyright 2012 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#ifndef SGPP_RAND_HPP
#define SGPP_RAND_HPP
#include "sg/rand.h"

class Rand {
    sg_rand_state m_state;

public:
    void seed()
    {
        sg_rand_seed(&m_state, 1);
    }

    unsigned irand()
    {
        return sg_irand(&m_state);
    }

    static unsigned girand()
    {
        return sg_girand();
    }

    float frand()
    {
        return sg_frand(&m_state);
    }

    static float gfrand()
    {
        return sg_gfrand();
    }
};

#endif
