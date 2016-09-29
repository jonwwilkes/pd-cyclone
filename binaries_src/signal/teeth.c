#include <string.h>
#include "m_pd.h"

typedef struct _teeth
{
    t_object  x_obj;
    t_inlet  *x_del_inlet1;
    t_inlet  *x_del_inlet2;
    t_inlet  *x_a_inlet;
    t_inlet  *x_b_inlet;
    t_inlet  *x_c_inlet;
    float     x_sr;
    float     x_ksr;
    t_float  *x_buf;
    int       x_bufsize;   /* as allocated */
    int       x_maxsize;   /* as used */
    float     x_maxdelay;  /* same in ms */
    int       x_phase;     /* writing head */
} t_teeth;

static t_class *teeth_class;

#define teeth_DEFMAXDELAY  10.0

static void teeth_clear(t_teeth *x)
{
    memset(x->x_buf, 0, x->x_maxsize * sizeof(*x->x_buf));
    x->x_phase = 0;
}

static void teeth_resize(t_teeth *x, int newsize)
{
    if (newsize > 0 && newsize != x->x_maxsize)
    {
	if (newsize > x->x_bufsize)
	{
	    x->x_buf = resizebytes(x->x_buf,
				   x->x_bufsize * sizeof(*x->x_buf),
				   newsize * sizeof(*x->x_buf));
	    /* LATER test for failure */
	    x->x_bufsize = newsize;
	}
	x->x_maxsize = newsize;
    }
    teeth_clear(x);
}

static t_int *teeth_perform(t_int *w)
{
    t_teeth *x = (t_teeth *)(w[1]);
    int nblock = (int)(w[2]);
    t_float *xin = (t_float *)(w[3]);
    t_float *din1 = (t_float *)(w[4]);
    t_float *din2 = (t_float *)(w[5]);
    t_float *ain = (t_float *)(w[6]);
    t_float *bin = (t_float *)(w[7]);
    t_float *cin = (t_float *)(w[8]);
    t_float *out = (t_float *)(w[9]);
    t_float *buf = x->x_buf;
    int maxsize = x->x_maxsize;
    int guardpoint = maxsize - 1;
    float ksr = x->x_ksr;
    int wph = x->x_phase;
    while (nblock--)
    {  /* TDFII scheme is used.  Do not forget, that any signal value
	  read after writing to out has to be saved beforehand. */
	float xn = *xin++;
	float delsize = ksr * *din1++;
	float bgain = *bin++;
	float cgain = *cin++;
	float yn = *ain++ * xn;
	float rph;  /* reading head */
//	if (cgain < -teeth_MAXFEEDBACK) cgain = -teeth_MAXFEEDBACK;
//	else if (cgain > teeth_MAXFEEDBACK) cgain = teeth_MAXFEEDBACK;
	if (delsize >= 0)
	{
	    int ndx;
	    float val;
	    rph = wph - (delsize > guardpoint ? guardpoint : delsize);
	    if (rph < 0) rph += guardpoint;
	    ndx = (int)rph;
	    val = buf[ndx];
	    /* ``a cheezy linear interpolation'' ala msp,
	       (vd~ uses 4-point interpolation...) */
	    yn += val + (buf[ndx+1] - val) * (rph - ndx);
	}
	*out++ = yn;
	if (wph == guardpoint)
	{
	    buf[wph] = *buf = bgain * xn + cgain * yn;
	    wph = 1;
	}
	else buf[wph++] = bgain * xn + cgain * yn;
    }
    x->x_phase = wph;
    return (w + 10);
}

static void teeth_dsp(t_teeth *x, t_signal **sp)
{
    float sr = sp[0]->s_sr;
    if (sr != x->x_sr)
    {
	x->x_sr = sr;
	x->x_ksr = sr * 0.001;
	teeth_resize(x, x->x_ksr * x->x_maxdelay);
    }
    else teeth_clear(x);
    dsp_add(teeth_perform, 9, x, sp[0]->s_n,
	    sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec,
	    sp[3]->s_vec, sp[4]->s_vec, sp[5]->s_vec, sp[6]->s_vec);
}

static void *teeth_new(t_floatarg f1, t_floatarg f2,
		      t_floatarg f3, t_floatarg f4, t_floatarg f5, t_floatarg f6)
{
    t_teeth *x;
    float maxdelay = (f1 > 0 ? f1 : teeth_DEFMAXDELAY);
    float sr = sys_getsr();
    float ksr = sr * 0.001;
    int bufsize = ksr * maxdelay;
    t_float *buf = (t_float *)getbytes(bufsize * sizeof(*buf));
    if (!buf)
	return (0);
    x = (t_teeth *)pd_new(teeth_class);
    x->x_maxdelay = maxdelay;
    x->x_sr = sr;
    x->x_ksr = ksr;
    x->x_bufsize = x->x_maxsize = bufsize;
    x->x_buf = buf;
    if (f2 < 0) f2 = 0;
//  if (f5 < -teeth_MAXFEEDBACK) f5 = -teeth_MAXFEEDBACK;
//  else if (f5 > teeth_MAXFEEDBACK) f5 = teeth_MAXFEEDBACK;
    x->x_del_inlet1 = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_del_inlet1, f2);
    x->x_del_inlet2 = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_del_inlet2, f3);
    x->x_a_inlet = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_a_inlet, f4);
    x->x_b_inlet = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_b_inlet, f5);
    x->x_c_inlet = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_c_inlet, f6);
    outlet_new((t_object *)x, &s_signal);
    teeth_clear(x);
    return (x);
}

static void teeth_free(t_teeth *x)
{
    if (x->x_buf) freebytes(x->x_buf, x->x_bufsize * sizeof(*x->x_buf));
}

void teeth_tilde_setup(void)
{
    teeth_class = class_new(gensym("teeth~"),
			   (t_newmethod)teeth_new,
			   (t_method)teeth_free,
			   sizeof(t_teeth), 0,
			   A_DEFFLOAT, A_DEFFLOAT,
			   A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, 0);
    class_addmethod(teeth_class, nullfn, gensym("signal"), 0);
    class_addmethod(teeth_class, (t_method)teeth_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(teeth_class, (t_method)teeth_clear, gensym("clear"), 0);
}
