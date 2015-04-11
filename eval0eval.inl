/******************************************************
SQL evaluator: evaluates simple data structures, like expressions, in
a query graph

(c) 1997 Innobase Oy

Created 12/29/1997 Heikki Tuuri
*******************************************************/

#include "que0que.h"
#include "rem0cmp.h"
#include "pars0grm.h"

/*********************************************************************
Evaluates a function node. */

void
eval_func(
/*======*/
	func_node_t*	func_node);	/* in: function node */
/*********************************************************************
Allocate a buffer from global dynamic memory for a value of a que_node.
NOTE that this memory must be explicitly freed when the query graph is
freed. If the node already has allocated buffer, that buffer is freed
here. NOTE that this is the only function where dynamic memory should be
allocated for a query node val field. */

byte*
eval_node_alloc_val_buf(
/*====================*/
				/* out: pointer to allocated buffer */
	que_node_t*	node,	/* in: query graph node; sets the val field
				data field to point to the new buffer, and
				len field equal to size */
	ulint		size);	/* in: buffer size */


/*********************************************************************
Allocates a new buffer if needed. */
UNIV_INLINE
byte*
eval_node_ensure_val_buf(
/*=====================*/
				/* out: pointer to buffer */
	que_node_t*	node,	/* in: query graph node; sets the val field
				data field to point to the new buffer, and
				len field equal to size */
	ulint		size)	/* in: buffer size */
{
	dfield_t*	dfield;
	byte*		data;

	dfield = que_node_get_val(node);
	dfield_set_len(dfield, size);

	data = dfield_get_data(dfield);
	
	if (!data || que_node_get_val_buf_size(node) < size) {

		data = eval_node_alloc_val_buf(node, size);
	}

	return(data);
}

/*********************************************************************
Evaluates a symbol table symbol. */
UNIV_INLINE
void
eval_sym(
/*=====*/
	sym_node_t*	sym_node)	/* in: symbol table node */
{

	ut_ad(que_node_get_type(sym_node) == QUE_NODE_SYMBOL);

	if (sym_node->indirection) {
		/* The symbol table node is an alias for a variable or a
		column */
		
		dfield_copy_data(que_node_get_val(sym_node),
				   que_node_get_val(sym_node->indirection));
	}
}

/*********************************************************************
Evaluates an expression. */
UNIV_INLINE
void
eval_exp(
/*=====*/
	que_node_t*	exp_node)	/* in: expression */
{
	if (que_node_get_type(exp_node) == QUE_NODE_SYMBOL) {

		eval_sym((sym_node_t*)exp_node);

		return;
	}
	
	eval_func(exp_node);
}

/*********************************************************************
Sets an integer value as the value of an expression node. */
UNIV_INLINE
void
eval_node_set_int_val(
/*==================*/
	que_node_t*	node,	/* in: expression node */
	lint		val)	/* in: value to set */
{
	dfield_t*	dfield;
	byte*		data;

	dfield = que_node_get_val(node);

	data = dfield_get_data(dfield);
	
	if (data == NULL) {
		data = eval_node_alloc_val_buf(node, 4);
	}

	ut_ad(dfield_get_len(dfield) == 4);
	
	mach_write_to_4(data, (ulint)val);
}

/*********************************************************************
Gets an integer non-SQL null value from an expression node. */
UNIV_INLINE
lint
eval_node_get_int_val(
/*==================*/
				/* out: integer value */
	que_node_t*	node)	/* in: expression node */
{
	dfield_t*	dfield;

	dfield = que_node_get_val(node);

	ut_ad(dfield_get_len(dfield) == 4);

	return((int)mach_read_from_4(dfield_get_data(dfield)));	
}

/*********************************************************************
Gets a iboolean value from a query node. */
UNIV_INLINE
ibool
eval_node_get_ibool_val(
/*===================*/
				/* out: iboolean value */
	que_node_t*	node)	/* in: query graph node */
{
	dfield_t*	dfield;
	byte*		data;

	dfield = que_node_get_val(node);

	data = dfield_get_data(dfield);
	
	ut_ad(data != NULL);

	return(mach_read_from_1(data));
}

/*********************************************************************
Sets a iboolean value as the value of a function node. */
UNIV_INLINE
void
eval_node_set_ibool_val(
/*===================*/
	func_node_t*	func_node,	/* in: function node */
	ibool		val)		/* in: value to set */
{
	dfield_t*	dfield;
	byte*		data;

	dfield = que_node_get_val(func_node);

	data = dfield_get_data(dfield);
	
	if (data == NULL) {
		/* Allocate 1 byte to hold the value */

		data = eval_node_alloc_val_buf(func_node, 1);
	}

	ut_ad(dfield_get_len(dfield) == 1);
	
	mach_write_to_1(data, val);
}

/*********************************************************************
Copies a binary string value as the value of a query graph node. Allocates a
new buffer if necessary. */
UNIV_INLINE
void
eval_node_copy_and_alloc_val(
/*=========================*/
	que_node_t*	node,	/* in: query graph node */
	byte*		str,	/* in: binary string */
	ulint		len)	/* in: string length or UNIV_SQL_NULL */
{
	byte*		data;
	
	ut_ad(UNIV_SQL_NULL > ULINT_MAX);

	if (len == UNIV_SQL_NULL) {
		dfield_set_len(que_node_get_val(node), len);

		return;
	}

	data = eval_node_ensure_val_buf(node, len);
	
	ut_memcpy(data, str, len);
}

/*********************************************************************
Copies a query node value to another node. */
UNIV_INLINE
void
eval_node_copy_val(
/*===============*/
	que_node_t*	node1,	/* in: node to copy to */
	que_node_t*	node2)	/* in: node to copy from */
{
	dfield_t*	dfield2;
	
	dfield2 = que_node_get_val(node2);

	eval_node_copy_and_alloc_val(node1, dfield_get_data(dfield2),
						dfield_get_len(dfield2));
}
