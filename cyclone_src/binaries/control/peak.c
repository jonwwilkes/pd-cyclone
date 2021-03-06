/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "m_pd.h"

typedef struct _Peak
{
    t_object   x_ob;
    t_float    x_value;
    t_outlet  *x_out2;
    t_outlet  *x_out3;
} t_Peak;

static t_class *Peak_class;

static void Peak_bang(t_Peak *x)
{
    outlet_float(((t_object *)x)->ob_outlet, x->x_value);
}

static void Peak_ft1(t_Peak *x, t_floatarg f)
{
    /* CHECKME loud_checkint */
    outlet_float(x->x_out3, 0);  /* CHECKME */
    outlet_float(x->x_out2, 1);
    outlet_float(((t_object *)x)->ob_outlet, x->x_value = f);
}

static void Peak_float(t_Peak *x, t_float f)
{
    /* CHECKME loud_checkint */
    if (f > x->x_value) Peak_ft1(x, f);
    else
    {
	outlet_float(x->x_out3, 1);
	outlet_float(x->x_out2, 0);
    }
}

static void *Peak_new(t_symbol *s, int ac, t_atom *av)
{
    t_Peak *x = (t_Peak *)pd_new(Peak_class);
    t_float f1 = 0;
    switch(ac){
        default:
        case 1:
            f1=atom_getfloat(av);
            break;
        case 0:
            break;
    }
    x->x_value = f1;
    inlet_new((t_object *)x, (t_pd *)x, &s_float, gensym("ft1"));
    outlet_new((t_object *)x, &s_float);
    x->x_out2 = outlet_new((t_object *)x, &s_float);
    x->x_out3 = outlet_new((t_object *)x, &s_float);
    return (x);
}

void peak_setup(void)
{
    Peak_class = class_new(gensym("peak"),
			   (t_newmethod)Peak_new, 0,
			   sizeof(t_Peak), 0, A_GIMME, 0);
    class_addcreator((t_newmethod)Peak_new, gensym("Peak"), 0, A_GIMME, 0);
    class_addcreator((t_newmethod)Peak_new, gensym("cyclone/Peak"), 0, A_GIMME, 0);
    class_addbang(Peak_class, Peak_bang);
    class_addfloat(Peak_class, Peak_float);
    class_addmethod(Peak_class, (t_method)Peak_ft1,
		    gensym("ft1"), A_FLOAT, 0);
}

void Peak_setup(void)
{
    peak_setup();
}
