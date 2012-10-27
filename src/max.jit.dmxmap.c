/* 
	Copyright 2012
	Tobias Ebsen - tobiasebsen@gmail.com
*/

#include "jit.common.h"
#include "max.jit.mop.h"

typedef struct _max_jit_dmxmap
{
	t_object 		ob;
	void			*obex;
} t_max_jit_dmxmap;

void *class_max_jit_dmxmap;

void max_jit_dmxmap_assist(t_max_jit_dmxmap *x, void *b, long m, long a, char *s);
void *max_jit_dmxmap_new(t_symbol *s, long argc, t_atom *argv);
void max_jit_dmxmap_free(t_max_jit_dmxmap *x);

//from jit.hello.c
t_jit_err jit_dmxmap_init(void); 

int main(void)
{	
	void *p,*q;
	
	jit_dmxmap_init();
	setup((t_messlist**)&class_max_jit_dmxmap, (method)max_jit_dmxmap_new, (method)max_jit_dmxmap_free, (short)sizeof(t_max_jit_dmxmap), NULL, A_GIMME, 0);

	p = max_jit_classex_setup(calcoffset(t_max_jit_dmxmap,obex));
	q = jit_class_findbyname(gensym("jit_dmxmap"));
	max_jit_classex_mop_wrap(p,q,0);
    max_jit_classex_standard_wrap(p,q,0); 	
    addmess((method)max_jit_mop_assist, "assist", A_CANT,0);

	return 0;
}

void max_jit_dmxmap_assist(t_max_jit_dmxmap *x, void *b, long m, long a, char *s)
{
	//nada for now	
}

void max_jit_dmxmap_free(t_max_jit_dmxmap *x)
{
	max_jit_mop_free(x);
	jit_object_free(max_jit_obex_jitob_get(x));
	max_jit_obex_free(x);
}

void *max_jit_dmxmap_new(t_symbol *s, long argc, t_atom *argv)
{
	t_max_jit_dmxmap *x;
	void *o,*m;
	t_jit_matrix_info info;
	long n;

	if (x = (t_max_jit_dmxmap *)max_jit_obex_new(class_max_jit_dmxmap,gensym("jit_dmxmap"))) {
		if (o=jit_object_new(gensym("jit_dmxmap"))) {
			
			attr_args_process(x, argc, argv);
			argc = attr_args_offset(argc, argv);

			max_jit_obex_jitob_set(x, o);
			max_jit_obex_dumpout_set(x, outlet_new(x,NULL));
			max_jit_mop_setup(x);
			max_jit_mop_inputs(x);
			max_jit_mop_outputs(x);
			
			if(argc == 1)
				n = jit_atom_getlong(&argv[0]);
			else
				n = 512;
			
			m = max_jit_mop_getoutput(x, 1);
			
			jit_object_method(m, _jit_sym_getinfo, &info);
			info.type			= _jit_sym_char;
			info.planecount		= 1;
			info.dimcount		= 1;
			info.dim[0]			= n;
			info.dimstride[0]	= 1;
			info.dimstride[1]	= n;
			jit_object_method(m, _jit_sym_setinfo, &info);

			//max_jit_obex_jitob_set(x,o);
			//max_jit_attr_args(x,argc,argv);
		} else {	
			jit_object_error((t_object *)x,"jit.dmxmap: out of memory");
			freeobject((void *)x);
			x = NULL;
			goto out;
		}
	}

out:	
	return (x);
}

