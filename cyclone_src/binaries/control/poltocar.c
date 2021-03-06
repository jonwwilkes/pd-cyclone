/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include <math.h>
#include "m_pd.h"

#if defined(_WIN32) || defined(__APPLE__)
/* cf pd/src/x_arithmetic.c */
#define sinf  sin
#define cosf  cos
#endif

typedef struct _poltocar
{
    t_object   x_ob;
    t_float    x_phase;
    t_float    x_re;
    t_float    x_im;
    t_outlet  *x_out2;
} t_poltocar;

static t_class *poltocar_class;

static void poltocar_float(t_poltocar *x, t_float f)
{
    outlet_float(x->x_out2, x->x_im = f * sinf(x->x_phase));
    outlet_float(((t_object *)x)->ob_outlet, x->x_re = f * cosf(x->x_phase));
}


static void poltocar_bang(t_poltocar *x)
{
    outlet_float(x->x_out2, x->x_im);
    outlet_float(((t_object *)x)->ob_outlet, x->x_re);
}

static void *poltocar_new(void)
{
    t_poltocar *x = (t_poltocar *)pd_new(poltocar_class);
    floatinlet_new((t_object *)x, &x->x_phase);
    outlet_new((t_object *)x, &s_float);
    x->x_out2 = outlet_new((t_object *)x, &s_float);
    return (x);
}

void poltocar_setup(void)
{
    poltocar_class = class_new(gensym("poltocar"),
			       (t_newmethod)poltocar_new, 0,
			       sizeof(t_poltocar), 0, 0);
    class_addfloat(poltocar_class, poltocar_float);
    class_addbang(poltocar_class, poltocar_bang);
}
