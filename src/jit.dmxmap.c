/* 
 Copyright 2012
 Tobias Ebsen - tobiasebsen@gmail.com
*/

#include "jit.common.h"

typedef struct _jit_dmxmap
{
	t_jit_object	ob;
	t_symbol		*text;
} t_jit_dmxmap;

void *_jit_dmxmap_class;

BEGIN_USING_C_LINKAGE
t_jit_err jit_dmxmap_init(void);
void jit_dmxmap_free(t_jit_dmxmap *x);
t_jit_err jit_dmxmap_matrix_calc(t_jit_dmxmap *x, void *inputs, void *outputs);
void jit_dmxmap_calculate_ndim(void *vecdata, long dimcount, long *dim, long planecount, t_jit_matrix_info *in1_minfo, char *bip1, 
							   t_jit_matrix_info *in2_minfo, char *bip2, t_jit_matrix_info *out_minfo, char *bop);

void jit_dmxmap_vector_char(long n, void *vecdata, t_jit_op_info *in1, t_jit_op_info *in2, t_jit_op_info *out);
void jit_dmxmap_vector_long(long n, void *vecdata, t_jit_op_info *in1, t_jit_op_info *in2, t_jit_op_info *out);
END_USING_C_LINKAGE

t_jit_dmxmap * jit_dmxmap_new(t_symbol *s);

t_jit_err jit_dmxmap_init(void) 
{
	long attrflags=0;
	t_jit_object *attr,*mop;
	void *o;
	t_atom a, a2[2];
	
	_jit_dmxmap_class = jit_class_new("jit_dmxmap",(method)jit_dmxmap_new,(method)jit_dmxmap_free,
		sizeof(t_jit_dmxmap),0L);

	//add mop
	mop = (t_jit_object*)jit_object_new(_jit_sym_jit_mop, 2, 1);
	
	//input1
	o = jit_object_method(mop, _jit_sym_getinput, 1);
	jit_atom_setsym(&a, _jit_sym_char);
	jit_object_method(o, _jit_sym_types, 1, &a);			//type		char
	jit_atom_setlong(&a, 1);
	jit_object_method(o, _jit_sym_minplanecount, 4, &a);	//minplane	4
	jit_object_method(o, _jit_sym_maxplanecount, 4, &a);	//maxplane	4
	
	//input2
	o = jit_object_method(mop, _jit_sym_getinput, 2);
	jit_atom_setsym(&a2[0], _jit_sym_long);
	jit_atom_setsym(&a2[1], _jit_sym_char);
	jit_object_method(o, _jit_sym_types, 2, &a2);			//type		char, long
	jit_attr_setlong(o, _jit_sym_planelink, 0);				//planelink	0
	jit_atom_setlong(&a, 1);
	jit_object_method(o, _jit_sym_minplanecount, 1, &a);	//minplane	1
	jit_object_method(o, _jit_sym_maxplanecount, 1, &a);	//maxplane	1
	
	//output
	o = jit_object_method(mop, _jit_sym_getoutput, 1);
	jit_atom_setsym(&a, _jit_sym_char);
	jit_object_method(o, _jit_sym_types, 1, &a);			//type		char
	jit_attr_setlong(o, _jit_sym_dimlink, 0);				//dimlink	0
	jit_atom_setlong(&a, 512);
	jit_object_method(o, _jit_sym_dim, 1, &a);				//dim		512
	jit_attr_setlong(o, _jit_sym_planelink, 0);				//planelink	0
	jit_atom_setlong(&a, 1);
	jit_object_method(o, _jit_sym_minplanecount, 1, &a);	//minplane	1
	jit_object_method(o, _jit_sym_maxplanecount, 1, &a);	//maxplane	1
	
	jit_class_addadornment(_jit_dmxmap_class, mop);
	
	//add methods
	jit_class_addmethod(_jit_dmxmap_class, (method)jit_dmxmap_matrix_calc, (char*)"matrix_calc", A_CANT, 0);
	
	//add attributes	
	attrflags = JIT_ATTR_GET_DEFER_LOW | JIT_ATTR_SET_USURP_LOW;
	attr = (t_jit_object*)jit_object_new(_jit_sym_jit_attr_offset,"text",_jit_sym_symbol,attrflags,
		(method)0L,(method)0L,calcoffset(t_jit_dmxmap,text));
	jit_class_addattr(_jit_dmxmap_class,attr);
	
	jit_class_register(_jit_dmxmap_class);

	return JIT_ERR_NONE;
}

t_jit_err jit_dmxmap_matrix_calc(t_jit_dmxmap *x, void *inputs, void *outputs)
{
	t_jit_err err = JIT_ERR_NONE;
	void *in_matrix,*map_matrix,*out_matrix;
	long in_savelock,map_savelock,out_savelock;
	t_jit_matrix_info in_minfo,map_minfo,out_minfo;
	char *in_bp,*out_bp;
	char *map_bp;
	
	in_matrix = jit_object_method(inputs, _jit_sym_getindex, 0);
	map_matrix = jit_object_method(inputs, _jit_sym_getindex, 1);
	out_matrix = jit_object_method(outputs, _jit_sym_getindex, 0);
	
	if (x && in_matrix && map_matrix && out_matrix) {
		
		in_savelock = (long)jit_object_method(in_matrix, _jit_sym_lock, 1);
		map_savelock = (long)jit_object_method(map_matrix, _jit_sym_lock, 1);
		out_savelock = (long)jit_object_method(out_matrix, _jit_sym_lock, 1);
		
		jit_object_method(in_matrix, _jit_sym_getinfo, &in_minfo);
		jit_object_method(map_matrix, _jit_sym_getinfo, &map_minfo);
		jit_object_method(out_matrix, _jit_sym_getinfo, &out_minfo);
		
		if ((in_minfo.type!=_jit_sym_char) || (map_minfo.type!=_jit_sym_long)) {
			err=JIT_ERR_MISMATCH_TYPE;
			goto out;
		}
		
		if ((in_minfo.planecount!=4) || (map_minfo.planecount!=1)) {
			err=JIT_ERR_MISMATCH_PLANE;
			goto out;
		}
		
		if ((in_minfo.dimcount!=2) || (map_minfo.dimcount!=2) || (in_minfo.dim[0]!=map_minfo.dim[0])  || (in_minfo.dim[1]!=map_minfo.dim[1])) {
			err=JIT_ERR_MISMATCH_DIM;
			goto out;
		}
		
		jit_object_method(in_matrix, _jit_sym_getdata, &in_bp);
		jit_object_method(map_matrix, _jit_sym_getdata, &map_bp);
		jit_object_method(out_matrix, _jit_sym_getdata, &out_bp);
		
		if (!in_bp) { err=JIT_ERR_INVALID_INPUT; goto out;}
		if (!map_bp) { err=JIT_ERR_INVALID_INPUT; goto out;}
		if (!out_bp) { err=JIT_ERR_INVALID_OUTPUT; goto out;}
		
		jit_parallel_ndim_simplecalc3((method)jit_dmxmap_calculate_ndim,
									  NULL, in_minfo.dimcount, in_minfo.dim, in_minfo.planecount, &in_minfo, in_bp, &map_minfo, map_bp, &out_minfo, out_bp,
									  0 /* flags1 */, JIT_PARALLEL_NDIM_FLAGS_FULL_MATRIX /* flags2 */, 0 /*flags3*/);
	}
	else {
		return JIT_ERR_INVALID_PTR;
	}

out:
	jit_object_method(out_matrix, _jit_sym_lock, out_savelock);
	jit_object_method(map_matrix, _jit_sym_lock, map_savelock);
	jit_object_method(in_matrix, _jit_sym_lock, in_savelock);
	return err;
}

void jit_dmxmap_calculate_ndim(void *vecdata, long dimcount, long *dim, long planecount, t_jit_matrix_info *in1_minfo, char *bip1, 
							   t_jit_matrix_info *in2_minfo, char *bip2, t_jit_matrix_info *out_minfo, char *bop)
{
	long i,n;
	char *ip1=bip1,*op=bop;
	t_jit_op_info in1_opinfo,in2_opinfo,out_opinfo;
	
	if (dimcount<1) return; //safety
	
	switch(dimcount) {
		case 1:
			dim[1] = 1;
		case 2:
			//if planecount the same, flatten planes - treat as single plane data for speed
			n = dim[0];
			in1_opinfo.stride = in1_minfo->dim[0]>1?in1_minfo->planecount:0;
			in2_opinfo.stride = in2_minfo->dim[0]>1?in2_minfo->planecount:0;
			out_opinfo.stride = out_minfo->dim[0]>1?out_minfo->planecount:0;
			if (in2_minfo->type==_jit_sym_long) {
				if ((in1_opinfo.stride==4)&&(in2_opinfo.stride==1)&&(in2_opinfo.stride==1)) {
					for (i=0;i<dim[1];i++){
						in1_opinfo.p = bip1 + i*in1_minfo->dimstride[1];
						in2_opinfo.p = bip2 + i*in2_minfo->dimstride[1];
						out_opinfo.p = bop;//  + i*out_minfo->dimstride[1];
						jit_dmxmap_vector_long(n,vecdata,&in1_opinfo,&in2_opinfo,&out_opinfo);
					}
				}
			}
			else if (in2_minfo->type==_jit_sym_char) {
				if ((in1_opinfo.stride==4)&&(in2_opinfo.stride==1)&&(in2_opinfo.stride==1)) {
					for (i=0;i<dim[1];i++){
						in1_opinfo.p = bip1 + i*in1_minfo->dimstride[1];
						in2_opinfo.p = bip2 + i*in2_minfo->dimstride[1];
						out_opinfo.p = bop;//  + i*out_minfo->dimstride[1];
						jit_dmxmap_vector_char(n,vecdata,&in1_opinfo,&in2_opinfo,&out_opinfo);
					}
				}				
			}

			break;
		default:
			for	(i=0;i<dim[dimcount-1];i++) {
				ip1 = bip1 + i*in1_minfo->dimstride[dimcount-1];
				op  = bop  + i*out_minfo->dimstride[dimcount-1];
				jit_dmxmap_calculate_ndim(vecdata,dimcount-1,dim,planecount,in1_minfo,ip1,in2_minfo,bip2,out_minfo,op);
			}
	}
}

//template<typename T>
void jit_dmxmap_vector_char(long n, void *vecdata, t_jit_op_info *in1, t_jit_op_info *in2, t_jit_op_info *out)
{
	uchar *ip1,*op,*p0;
	uchar *ip2;
	long is1,is2,os;
	
	ip1 = ((uchar *)in1->p);
	ip2 = ((uchar *)in2->p);
	op  = ((uchar *)out->p);	
	is1 = in1->stride;
	is2 = in2->stride;
	os = out->stride;
	
	++n;
	while (--n) {
		p0 = op + (*ip2);
		p0[0] = ip1[0];
		p0[1] = ip1[1];
		p0[2] = ip1[2];
		ip1 += is1;
		ip2 += is2;
	}
}

void jit_dmxmap_vector_long(long n, void *vecdata, t_jit_op_info *in1, t_jit_op_info *in2, t_jit_op_info *out)
{
	uchar *ip1,*op,*p0;
	ulong *ip2;
	long is1,is2,os;
	
	ip1 = ((uchar *)in1->p);
	ip2 = ((ulong *)in2->p);
	op  = ((uchar *)out->p);	
	is1 = in1->stride;
	is2 = in2->stride;
	os = out->stride;
	
	++n;
	while (--n) {
		p0 = op + (*ip2);
		p0[0] = ip1[0];
		p0[1] = ip1[1];
		p0[2] = ip1[2];
		ip1 += is1;
		ip2 += is2;
	}
}

void jit_dmxmap_free(t_jit_dmxmap *x)
{
	//nada
}

t_jit_dmxmap * jit_dmxmap_new(t_symbol *s)
{
	t_jit_dmxmap *x;

	if (x = (t_jit_dmxmap *)jit_object_alloc(_jit_dmxmap_class)) {
		x->text = s;
	} 
		
	return x;
}
