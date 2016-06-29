/*  Boolector: Satisfiablity Modulo Theories (SMT) solver.
 *
 *  Copyright (C) 2013-2015 Aina Niemetz.
 *  Copyright (C) 2014-2015 Mathias Preiner.
 *  Copyright (C) 2014 Armin Biere.
 *
 *  All rights reserved.
 *
 *  This file is part of Boolector.
 *  See COPYING for more information on using this software.
 */

#include "btoraig.h"
#include "btoraigvec.h"
#include "btorcore.h"
#include "utils/btormap.h"
#include "utils/btorhash.h"
#include "utils/btorstack.h"
#include "btorsat.h"
#include "btorlog.h"
#include "utils/btorutil.h"
#include "utils/btoriter.h"
#include "btorbeta.h"
#include "btorbitvec.h"
#include "utils/btoriter.h"
#include "btorsort.h"
#include "btormsg.h"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

BTOR_DECLARE_STACK (BtorNodePtrStackPtr, BtorNodePtrStack *);
BTOR_DECLARE_STACK (BtorPtrHashTablePtrPtr, BtorPtrHashTable **);

/*------------------------------------------------------------------------*/

void *
btor_clone_key_as_node (BtorMemMgr * mm, const void * map, const void * key)
{
  assert (map);
  assert (key);

  BtorNode *exp, *cloned_exp;
  BtorNodeMap *exp_map;

  (void) mm;
  exp = (BtorNode *) key;
  exp_map = (BtorNodeMap *) map;
  cloned_exp = btor_mapped_node (exp_map, exp);
  assert (cloned_exp);
  return cloned_exp;
}

void *
btor_clone_key_as_str (BtorMemMgr * mm, const void * map, const void * key)
{
  assert (mm);
  assert (key);

  (void) map;

  return btor_strdup (mm, (char *) key);
}

void *
btor_clone_key_as_bv_tuple (BtorMemMgr * mm, const void * map, const void * t)
{
  assert (mm);
  assert (t);
  (void) map;
  return btor_copy_bv_tuple (mm, (BtorBitVectorTuple *) t);
}

void
btor_clone_data_as_node_ptr (BtorMemMgr *mm,
                             const void * map,
                             BtorPtrHashData * data,
                             BtorPtrHashData * cloned_data)
{
  assert (mm);
  assert (map);
  assert (data);
  assert (cloned_data);

  BtorNode *exp, *cloned_exp;
  BtorNodeMap *exp_map;

  (void) mm;
  exp = (BtorNode *) data->asPtr;
  exp_map = (BtorNodeMap *) map;
  cloned_exp = btor_mapped_node (exp_map, exp);
  assert (cloned_exp);
  cloned_data->asPtr = cloned_exp;
}

void
btor_clone_data_as_str_ptr (BtorMemMgr *mm,
                            const void * str_table,
                            BtorPtrHashData * data,
                            BtorPtrHashData * cloned_data)
{
  assert (str_table);
  assert (data);
  assert (cloned_data);

  char *str;

  (void) mm;
  str = data->asStr;
  assert (btor_find_in_ptr_hash_table ((BtorPtrHashTable *) str_table, str));
  
  cloned_data->asStr = (char *) btor_find_in_ptr_hash_table (
                             (BtorPtrHashTable *) str_table, str)->key;
}

void
btor_clone_data_as_int (BtorMemMgr *mm,
                        const void * map,
                        BtorPtrHashData * data, 
                        BtorPtrHashData * cloned_data)
{
  assert (data);
  assert (cloned_data);

  (void) mm;
  (void) map;
  cloned_data->asInt = data->asInt;
}

void
btor_clone_data_as_bv_ptr (BtorMemMgr *mm,
                           const void * map,
                           BtorPtrHashData * data,
                           BtorPtrHashData * cloned_data)
{
  assert (mm);
  assert (data);
  assert (cloned_data);

  (void) map;
  cloned_data->asPtr = btor_copy_bv (mm, (BtorBitVector *) data->asPtr); 
}

void
btor_clone_data_as_bv_htable_ptr (BtorMemMgr *mm,
                                  const void * map,
                                  BtorPtrHashData * data, 
                                  BtorPtrHashData * cloned_data)
{
  assert (mm);
  assert (map);
  assert (data);
  assert (cloned_data);

  BtorPtrHashTable *table;
  table = (BtorPtrHashTable *) data->asPtr;
  cloned_data->asPtr = btor_clone_ptr_hash_table (mm, table,
      btor_clone_key_as_bv_tuple, btor_clone_data_as_bv_ptr, map, map);
}

void
btor_clone_data_as_htable_ptr (BtorMemMgr *mm,
                               const void * map,
                               BtorPtrHashData * data, 
                               BtorPtrHashData * cloned_data)
{
  assert (mm);
  assert (map);
  assert (data);
  assert (cloned_data);

  BtorPtrHashTable *table;
  BtorNodeMap *exp_map;

  table = (BtorPtrHashTable *) data->asPtr;
  exp_map = (BtorNodeMap *) map;

  cloned_data->asPtr = btor_clone_ptr_hash_table (
      mm, table, btor_clone_key_as_node, 0, exp_map, 0);
}

/*------------------------------------------------------------------------*/

static void
clone_sorts_unique_table (BtorMemMgr * mm,
                          BtorSortUniqueTable * table,
                          BtorSortUniqueTable * res)
{
  assert (mm);
  assert (table);
  assert (res);

  unsigned i, j;
  BtorSort *sort, *csort;
  BtorSortId cid;
  BtorSortIdStack elements;
  
  BTOR_INIT_STACK (elements);

  BTOR_CNEWN (mm, res->chains, table->size);
  res->size = table->size;
  res->num_elements = 0;
  res->mm = mm;
  BTOR_INIT_STACK (res->id2sort);

  for (i = 0; i < BTOR_COUNT_STACK (table->id2sort); i++)
    {
      sort = BTOR_PEEK_STACK (table->id2sort, i);

      if (!sort)
        {
          BTOR_PUSH_STACK (res->mm, res->id2sort, 0);
          continue;
        }

      switch (sort->kind)
        {
          case BTOR_BOOL_SORT:
            cid = btor_bool_sort (res);
            break;

          case BTOR_BITVEC_SORT:
            cid = btor_bitvec_sort (res, sort->bitvec.width);
            break;

          case BTOR_LST_SORT:
            cid = btor_lst_sort (res, sort->lst.head->id, sort->lst.tail->id);
            break;

          case BTOR_ARRAY_SORT:
            cid = btor_array_sort (res, sort->array.index->id,
                                   sort->array.element->id);
            break;

          case BTOR_FUN_SORT:
            assert (BTOR_PEEK_STACK (res->id2sort, sort->fun.domain->id));
            cid = btor_fun_sort (res, sort->fun.domain->id,
                                 sort->fun.codomain->id);
            break;

          case BTOR_TUPLE_SORT:
            BTOR_RESET_STACK (elements);
            for (j = 0; j < sort->tuple.num_elements; j++)
              BTOR_PUSH_STACK (mm, elements, sort->tuple.elements[j]->id);
            cid = btor_tuple_sort (res, elements.start,
                                   BTOR_COUNT_STACK (elements));
            break;

          default:
            cid = 0;
            break;
        }
      assert (cid);
      csort = BTOR_PEEK_STACK (res->id2sort, cid);
      assert (csort->refs == 1);
      assert (csort->id == sort->id);
      assert (csort->kind == sort->kind);
      assert (csort->table != sort->table);
    }

  /* update sort references (must be the same as in table) */
  assert (BTOR_COUNT_STACK (table->id2sort) == BTOR_COUNT_STACK (res->id2sort));
  for (i = 0; i < BTOR_COUNT_STACK (res->id2sort); i++)
    {
      sort = BTOR_PEEK_STACK (table->id2sort, i);
      csort = BTOR_PEEK_STACK (res->id2sort, i);
      if (!sort)
        {
          assert (!csort);
          continue;
        }
      assert (csort->id == sort->id);
      assert (csort->parents == sort->parents);
      assert (csort->ext_refs == 0);
      assert (sort->refs == csort->refs - 1 + sort->refs - sort->parents);
      csort->refs = sort->refs;
      csort->ext_refs = sort->ext_refs;
    }
  assert (res->num_elements == table->num_elements);
  BTOR_RELEASE_STACK (mm, elements);
}

#if 0
static void
clone_sorts_unique_table (BtorMemMgr * mm,
                          BtorSortUniqueTable * table,
                          BtorSortUniqueTable * res)
{
  assert (mm);
  assert (table);
  assert (res);

  int i, j;
  BtorSort *sort, *csort, *tmp;
  BtorSortPtrStack elements;
  
  BTOR_INIT_STACK (elements);

  BTOR_CNEWN (mm, res->chains, table->size);
  res->size = table->size;
  res->num_elements = 0;
  res->mm = mm;
  BTOR_INIT_STACK (res->id2sort);

  for (i = 0; i < BTOR_COUNT_STACK (table->id2sort); i++)
    {
      sort = BTOR_PEEK_STACK (table->id2sort, i);

      if (!sort)
        {
          BTOR_PUSH_STACK (res->mm, res->id2sort, 0);
          continue;
        }

      switch (sort->kind)
        {
          case BTOR_BOOL_SORT:
            csort = btor_bool_sort (res);
            break;

          case BTOR_BITVEC_SORT:
            csort = btor_bitvec_sort (res, sort->bitvec.len);
            break;

          case BTOR_LST_SORT:
            csort = btor_lst_sort (res,
                        BTOR_PEEK_STACK (res->id2sort, sort->lst.head->id),
                        BTOR_PEEK_STACK (res->id2sort, sort->lst.tail->id));
            break;

          case BTOR_ARRAY_SORT:
            csort = btor_array_sort (res,
                        BTOR_PEEK_STACK (res->id2sort, sort->array.index->id),
                        BTOR_PEEK_STACK (res->id2sort,
                                         sort->array.element->id));
            break;

          case BTOR_FUN_SORT:
            tmp = BTOR_PEEK_STACK (res->id2sort, sort->fun.domain->id);
            assert (tmp);
            if (sort->fun.arity > 1)
              {
                assert (sort->fun.domain->kind == BTOR_TUPLE_SORT);
                assert (tmp->kind == BTOR_TUPLE_SORT);
                assert (sort->fun.arity == tmp->tuple.num_elements);
                csort = btor_fun_sort (res, tmp->tuple.elements,
                            sort->fun.arity,
                            BTOR_PEEK_STACK (res->id2sort,
                                             sort->fun.codomain->id));
              }
            else
              {
                assert (sort->fun.domain->kind != BTOR_TUPLE_SORT
                        && sort->fun.domain->kind != BTOR_LST_SORT);
                csort = btor_fun_sort (res, &tmp, 1,
                            BTOR_PEEK_STACK (res->id2sort,
                                             sort->fun.codomain->id));
              }
            break;

          case BTOR_TUPLE_SORT:
            BTOR_RESET_STACK (elements);
            for (j = 0; j < sort->tuple.num_elements; j++)
              BTOR_PUSH_STACK (mm, elements,
                               BTOR_PEEK_STACK (res->id2sort,
                                                sort->tuple.elements[j]->id));
            csort = btor_tuple_sort (res, elements.start,
                                     BTOR_COUNT_STACK (elements));
            break;

          default:
            csort = 0;
            break;
        }
      assert (csort);
      assert (csort->refs == 1);
      assert (csort->id == sort->id);
      assert (csort->kind == sort->kind);
      assert (csort->table != sort->table);
    }

  /* update sort references (must be the same as in table) */
  assert (BTOR_COUNT_STACK (table->id2sort) == BTOR_COUNT_STACK (res->id2sort));
  for (i = 0; i < BTOR_COUNT_STACK (res->id2sort); i++)
    {
      sort = BTOR_PEEK_STACK (table->id2sort, i);
      csort = BTOR_PEEK_STACK (res->id2sort, i);
      if (!sort)
        {
          assert (!csort);
          continue;
        }
      assert (csort->id == sort->id);
      assert (csort->parents == sort->parents);
      assert (csort->ext_refs == 0);
      assert (sort->refs == csort->refs - 1 + sort->refs - sort->parents);
      csort->refs = sort->refs;
      csort->ext_refs = sort->ext_refs;
    }
  assert (res->num_elements == table->num_elements);
  BTOR_RELEASE_STACK (mm, elements);
}
#endif


static BtorNode *
clone_exp (Btor * btor, Btor * clone, BtorNode * exp, 
           BtorNodePtrPtrStack * parents, BtorNodePtrPtrStack * nodes,
           BtorNodePtrStack * rhos,
           BtorNodePtrStack * static_rhos,
           BtorNodeMap * exp_map, bool exp_layer_only)
{
  assert (clone);
  assert (exp);
  assert (BTOR_IS_REGULAR_NODE (exp));
  assert (parents);
  assert (nodes);
  assert (exp_map);

  int i;
  BtorBitVector *bits;
  BtorNode *res;
  BtorParamNode *param;
  BtorMemMgr *mm;

  mm = clone->mm;

  res = btor_malloc (mm, exp->bytes);
  memcpy (res, exp, exp->bytes);

  /* ----------------- BTOR_BV_VAR_NODE_STRUCT (all nodes) ----------------> */
  if (BTOR_IS_BV_CONST_NODE (exp))
    {
      bits = btor_copy_bv (mm, btor_const_get_bits (exp));
      btor_const_set_bits (res, bits);

      if (btor_const_get_invbits (exp))
        {
          bits = btor_copy_bv (mm, btor_const_get_invbits (exp));
          btor_const_set_invbits (res, bits);
        }
    }

  /* Note: no need to cache aig vectors here (exp->av is unique to exp). */
  if (BTOR_IS_FUN_NODE (exp))
    {
      if (exp_layer_only)
        res->rho = 0;
      else if (exp->rho)
        {
          BTOR_PUSH_STACK (btor->mm, *rhos, res);
          BTOR_PUSH_STACK (btor->mm, *rhos, exp);
        }
    }
  else if (exp->av)
    res->av = exp_layer_only ? 0 : btor_clone_aigvec (exp->av, clone->avmgr);

  assert (!exp->next || !BTOR_IS_INVALID_NODE (exp->next));
  BTOR_PUSH_STACK_IF (exp->next, mm, *nodes, &res->next);

  assert (!exp->simplified 
          || !BTOR_IS_INVALID_NODE (BTOR_REAL_ADDR_NODE (exp->simplified)));
  BTOR_PUSH_STACK_IF (exp->simplified, mm, *nodes, &res->simplified);

  res->btor = clone;
  
  assert (!exp->first_parent
          || !BTOR_IS_INVALID_NODE (BTOR_REAL_ADDR_NODE (exp->first_parent)));
  assert (!exp->last_parent
          || !BTOR_IS_INVALID_NODE (BTOR_REAL_ADDR_NODE (exp->last_parent)));

  BTOR_PUSH_STACK_IF (exp->first_parent, mm, *parents, &res->first_parent);
  BTOR_PUSH_STACK_IF (exp->last_parent, mm, *parents, &res->last_parent);
  /* <---------------------------------------------------------------------- */

  /* ------------ BTOR_BV_ADDITIONAL_VAR_NODE_STRUCT (all nodes) ----------> */
  if (!BTOR_IS_BV_CONST_NODE (exp))
    {
      if (!BTOR_IS_BV_VAR_NODE (exp) && !BTOR_IS_PARAM_NODE (exp))
        {
          if (exp->arity)
            { 
              for (i = 0; i < exp->arity; i++)
                {
                  res->e[i] = btor_mapped_node (exp_map, exp->e[i]);
                  assert (exp->e[i] != res->e[i]);
                  assert (res->e[i]);
                }
            }

          for (i = 0; i < exp->arity; i++)
            {
              assert (!exp->prev_parent[i]
                      || !BTOR_IS_INVALID_NODE (
                              BTOR_REAL_ADDR_NODE (exp->prev_parent[i])));
              assert (!exp->next_parent[i]
                      || !BTOR_IS_INVALID_NODE (
                              BTOR_REAL_ADDR_NODE (exp->next_parent[i])));

              BTOR_PUSH_STACK_IF (
                  exp->prev_parent[i], mm, *parents, &res->prev_parent[i]);
              BTOR_PUSH_STACK_IF (
                  exp->next_parent[i], mm, *parents, &res->next_parent[i]);
            }
        }
    }
  /* <---------------------------------------------------------------------- */

  if (BTOR_IS_PARAM_NODE (exp))
    {
      param = (BtorParamNode *) exp;
      assert (!param->lambda_exp
              || !BTOR_IS_INVALID_NODE (
                      BTOR_REAL_ADDR_NODE (param->lambda_exp)));
      assert (!param->assigned_exp
              || !BTOR_IS_INVALID_NODE (
                      BTOR_REAL_ADDR_NODE (param->assigned_exp)));

      BTOR_PUSH_STACK_IF (param->lambda_exp, mm, *nodes, 
                          (BtorNode **) &((BtorParamNode *) res)->lambda_exp);
      BTOR_PUSH_STACK_IF (param->assigned_exp, mm, *nodes,
                          (BtorNode **) &((BtorParamNode *) res)->assigned_exp);
    }

  if (BTOR_IS_LAMBDA_NODE (exp))
    {
      if (btor_lambda_get_static_rho (exp))
        {
          BTOR_PUSH_STACK (mm, *static_rhos, res);
          BTOR_PUSH_STACK (mm, *static_rhos, exp);
        }
      assert (!btor_lambda_get_body (exp)
              || !BTOR_IS_INVALID_NODE (
                      BTOR_REAL_ADDR_NODE (btor_lambda_get_body (exp))));
      BTOR_PUSH_STACK_IF (btor_lambda_get_body (exp), mm, *nodes,
                          &((BtorLambdaNode *) res)->body);
    }

  btor_map_node (exp_map, exp, res);

  return res;
}

void
btor_clone_node_ptr_stack (BtorMemMgr * mm,
                      BtorNodePtrStack * stack, BtorNodePtrStack * res,
                      BtorNodeMap * exp_map)
{
  assert (stack);
  assert (res);
  assert (exp_map);

  int i;
  BtorNode *cloned_exp;

  BTOR_INIT_STACK (*res);
  assert (BTOR_SIZE_STACK (*stack) || !BTOR_COUNT_STACK (*stack));
  if (BTOR_SIZE_STACK (*stack))
    {
      BTOR_NEWN (mm, res->start, BTOR_SIZE_STACK (*stack));
      res->top = res->start;
      res->end = res->start + BTOR_SIZE_STACK (*stack);

      for (i = 0; i < BTOR_COUNT_STACK (*stack); i++)
        {
          assert ((*stack).start[i]);
          cloned_exp = btor_mapped_node (exp_map, (*stack).start[i]);
          assert (cloned_exp);
          BTOR_PUSH_STACK (mm, *res, cloned_exp);
        }
    }
  assert (BTOR_COUNT_STACK (*stack) == BTOR_COUNT_STACK (*res));
  assert (BTOR_SIZE_STACK (*stack) == BTOR_SIZE_STACK (*res));
}

static void
clone_nodes_id_table (Btor * btor,
                      Btor * clone,
                      BtorNodePtrStack * id_table, 
                      BtorNodePtrStack * res,
                      BtorNodeMap * exp_map, 
                      bool exp_layer_only,
                      BtorNodePtrStack * rhos)
{
  assert (id_table);
  assert (res);
  assert (exp_map);

  int i, tag;
  BtorNode **tmp, *exp, *cloned_exp;
  BtorMemMgr *mm;
  BtorNodePtrPtrStack parents, nodes;
  BtorPtrHashTable *t;
  BtorNodePtrStack static_rhos;

  mm = clone->mm;

  BTOR_INIT_STACK (parents);
  BTOR_INIT_STACK (nodes);
  BTOR_INIT_STACK (static_rhos);

  BTOR_INIT_STACK (*res);
  assert (BTOR_SIZE_STACK (*id_table) || !BTOR_COUNT_STACK (*id_table));

  if (BTOR_SIZE_STACK (*id_table))
    {
      BTOR_NEWN (mm, res->start, BTOR_SIZE_STACK (*id_table));
      res->top = res->start + BTOR_COUNT_STACK (*id_table);
      res->end = res->start + BTOR_SIZE_STACK (*id_table);
      res->start[0] = 0;

      /* first element (id = 0) is empty */
      for (i = 1; i < BTOR_COUNT_STACK (*id_table); i++)
        {
          exp = id_table->start[i];
          res->start[i] = exp 
                          ? clone_exp (btor, clone, exp, &parents, &nodes,
                                       rhos, &static_rhos, exp_map,
                                       exp_layer_only)
                          : 0;
          assert (!exp || res->start[i]->id == i);
        }
    }
  assert (BTOR_COUNT_STACK (*res) == BTOR_COUNT_STACK (*id_table));
  assert (BTOR_SIZE_STACK (*res) == BTOR_SIZE_STACK (*id_table));
  
  /* update children, parent, lambda and next pointers of expressions */
  while (!BTOR_EMPTY_STACK (nodes))
    {
      tmp = BTOR_POP_STACK (nodes);
      assert (*tmp);
      *tmp = btor_mapped_node (exp_map, *tmp);
      assert (*tmp);
    }
  
  while (!BTOR_EMPTY_STACK (parents))
    {
      tmp = BTOR_POP_STACK (parents);
      assert (*tmp);
      tag = BTOR_GET_TAG_NODE (*tmp);
      *tmp = btor_mapped_node (exp_map, BTOR_REAL_ADDR_NODE (*tmp));
      assert (*tmp);
      *tmp = BTOR_TAG_NODE (*tmp, tag);
    }

  /* clone static_rho tables */
  while (!BTOR_EMPTY_STACK (static_rhos))
    {
      exp = BTOR_POP_STACK (static_rhos);
      cloned_exp = BTOR_POP_STACK (static_rhos);
      assert (BTOR_IS_LAMBDA_NODE (exp));
      assert (BTOR_IS_LAMBDA_NODE (cloned_exp));
      t = btor_lambda_get_static_rho (exp);
      assert (t);
      btor_lambda_set_static_rho (cloned_exp,
          btor_clone_ptr_hash_table (mm, t, btor_clone_key_as_node,
                                     btor_clone_data_as_node_ptr,
                                     exp_map, exp_map));
    }

  BTOR_RELEASE_STACK (mm, parents);
  BTOR_RELEASE_STACK (mm, nodes);
  BTOR_RELEASE_STACK (mm, static_rhos);
}

static void
clone_nodes_unique_table (BtorMemMgr * mm,
                          BtorNodeUniqueTable * table, 
                          BtorNodeUniqueTable * res,
                          BtorNodeMap * exp_map)
{
  assert (mm);
  assert (table);
  assert (res);
  assert (exp_map);

  int i;

  BTOR_CNEWN (mm, res->chains, table->size);
  res->size = table->size;
  res->num_elements = table->num_elements;

  for (i = 0; i < table->size; i++)
    {
      if (!table->chains[i]) continue;
      res->chains[i] = btor_mapped_node (exp_map, table->chains[i]);
      assert (res->chains[i]);
    }
}

#define MEM_PTR_HASH_TABLE(table) \
  ((table) ? sizeof (*(table)) \
             + (table)->size * sizeof (BtorPtrHashBucket *) \
             + (table)->count * sizeof (BtorPtrHashBucket) \
           : 0)

#define CHKCLONE_MEM_PTR_HASH_TABLE(table, clone) \
  do { \
    assert (MEM_PTR_HASH_TABLE (table) \
            == MEM_PTR_HASH_TABLE (clone)); \
  } while (0)

#define CLONE_PTR_HASH_TABLE(table) \
  do { \
    clone->table = btor_clone_ptr_hash_table (\
        mm, btor->table, btor_clone_key_as_node, 0, emap, 0); \
    CHKCLONE_MEM_PTR_HASH_TABLE(btor->table, clone->table); \
  } while (0)

#define CLONE_PTR_HASH_TABLE_DATA(table, data_func) \
  do { \
    BTORLOG_TIMESTAMP (delta); \
    clone->table = btor_clone_ptr_hash_table (\
        mm, btor->table, btor_clone_key_as_node, data_func, emap, emap); \
    BTORLOG (1, "  clone "#table" table: %.3f s", \
             (btor_time_stamp () - delta)); \
    CHKCLONE_MEM_PTR_HASH_TABLE(btor->table, clone->table); \
  } while (0)

#define MEM_BITVEC(bv) \
  ((bv) ? sizeof (*(bv)) + bv->len * sizeof (BTOR_BV_TYPE) : 0)

static Btor *
clone_aux_btor (Btor * btor, 
                BtorNodeMap ** exp_map, 
                bool exp_layer_only)
{
  assert (btor);
  assert (exp_layer_only
          || btor_has_clone_support_sat_mgr (btor_get_sat_mgr_btor (btor)));

  Btor *clone;
  BtorNodeMap *emap = 0;
  BtorMemMgr *mm;
  double start, delta;
  int i, len;
  char *prefix, *clone_prefix;
  BtorNode *exp, *cloned_exp;
  BtorHashTableIterator it;
  BtorNodePtrStack rhos;
#ifndef NDEBUG
  int h;
  size_t allocated;
  BtorNode *cur;
  BtorAIGMgr *amgr;
  BtorBVAssignment *bvass;
  BtorArrayAssignment *arrass;
  BtorHashTableIterator cit, ncit;
  BtorSort *sort;
  char **ind, **val;
  amgr = exp_layer_only ? 0 : btor_get_aig_mgr_aigvec_mgr (btor->avmgr);
  BtorPtrHashData *data, *cdata;
#endif

  BTORLOG (1, "start cloning btor %p ...", btor);
  start = btor_time_stamp ();

  mm = btor_new_mem_mgr ();
  BTOR_CNEW (mm, clone);
  memcpy (clone, btor, sizeof (Btor));
  clone->mm = mm;

  /* always auto cleanup external references (dangling, not held from extern) */
  clone->options.auto_cleanup.val = 1;
  
  if (exp_layer_only)
    {
      /* reset */
      clone->btor_sat_btor_called = 0;
      btor_reset_time_btor (clone);
      btor_reset_stats_btor (clone);
    }

  assert ((allocated = sizeof (Btor)) == clone->mm->allocated);

  clone->msg = btor_new_btor_msg (clone->mm, &clone->options.verbosity.val);
  assert ((allocated += sizeof (BtorMsg)) == clone->mm->allocated);

  /* set msg prefix for clone */
  clone_prefix = "clone";
  len = btor->msg->prefix ? strlen (btor->msg->prefix) : 0;
  len += strlen (clone_prefix) + 1;
#ifndef NDEBUG
  allocated += len + 1;
#endif
  BTOR_NEWN (clone->mm, prefix, len + 1);
  sprintf (prefix, "%s>%s", btor->msg->prefix, clone_prefix);
  btor_set_msg_prefix_btor (clone, prefix);
  BTOR_DELETEN (clone->mm, prefix, len + 1);

  if (exp_layer_only)
    {
      clone->bv_assignments = btor_new_bv_assignment_list (mm);
      assert ((allocated += sizeof (BtorBVAssignmentList)) 
              == clone->mm->allocated);
    }
  else
    {
      BTORLOG_TIMESTAMP (delta);
      clone->bv_assignments = btor_clone_bv_assignment_list (
                                 clone->mm, btor->bv_assignments);
      BTORLOG (1, "  clone BV assignments: %.3f s",
               (btor_time_stamp () - delta));
#ifndef NDEBUG
      for (bvass = btor->bv_assignments->first; bvass; bvass = bvass->next)
        allocated += sizeof (BtorBVAssignment) 
          + strlen (btor_get_bv_assignment_str (bvass));
      assert ((allocated += sizeof (BtorBVAssignmentList)) 
              == clone->mm->allocated);
#endif
    }

  if (exp_layer_only)
    {
      clone->array_assignments = btor_new_array_assignment_list (mm);
      assert ((allocated += sizeof (BtorArrayAssignmentList))
              == clone->mm->allocated);
    }
  else
    {
      BTORLOG_TIMESTAMP (delta);
      clone->array_assignments = btor_clone_array_assignment_list (
                                    clone->mm, btor->array_assignments);
      BTORLOG (1, "  clone array assignments: %.3f s", 
               (btor_time_stamp () - delta));
#ifndef NDEBUG
      for (arrass = btor->array_assignments->first; 
           arrass; 
           arrass = arrass->next)
        {
          allocated += sizeof (BtorArrayAssignment) 
                       + 2 * arrass->size * sizeof (char *);
          btor_get_array_assignment_indices_values (
              arrass, &ind, &val, arrass->size);
          for (i = 0; i < arrass->size; i++)
            allocated += strlen (ind[i]) + strlen (val[i]);
        }
      assert ((allocated += sizeof (BtorArrayAssignmentList))
              == clone->mm->allocated);
#endif
    }

  if (exp_layer_only)
    {
      clone->avmgr = btor_new_aigvec_mgr (mm, clone->msg, &clone->options);
      assert ((allocated += sizeof (BtorAIGVecMgr) 
                         + sizeof (BtorAIGMgr) 
                         + sizeof (BtorSATMgr)
                         /* true and false AIGs */
                         + 2 * sizeof (BtorAIG *)
                         + sizeof (int32_t)) /* unique table chains */
              == clone->mm->allocated);
    }
  else
    {
      BTORLOG_TIMESTAMP (delta);
      clone->avmgr = btor_clone_aigvec_mgr (mm, clone->msg, &clone->options, btor->avmgr);
      BTORLOG (1, "  clone AIG mgr: %.3f s", (btor_time_stamp () - delta));
      assert ((allocated += sizeof (BtorAIGVecMgr) 
                         + sizeof (BtorAIGMgr) 
                         + sizeof (BtorSATMgr)
#ifdef BTOR_USE_LINGELING
                         + (amgr->smgr->solver ? sizeof (BtorLGL) : 0)
#endif
                         + (amgr->smgr->optstr 
                              ? strlen (amgr->smgr->optstr) + 1 : 0 )
                         /* memory of AIG nodes */
                         + (amgr->cur_num_aigs +
                            amgr->cur_num_aig_vars) * sizeof (BtorAIG)
                         /* children for AND AIGs */
                         + amgr->cur_num_aigs * sizeof (int32_t) * 2
                         /* unique table chain */
                         + amgr->table.size * sizeof (int32_t)
                         + BTOR_SIZE_STACK (amgr->id2aig) * sizeof (BtorAIG *)
                         + BTOR_SIZE_STACK (amgr->cnfid2aig) * sizeof (int32_t))
              == clone->mm->allocated);
    }

  BTORLOG_TIMESTAMP (delta);
  clone_sorts_unique_table (mm,
                            &btor->sorts_unique_table,
                            &clone->sorts_unique_table);
  BTORLOG (1, "  clone sorts unique table: %.3f s",
           (btor_time_stamp () - delta));
#ifndef NDEBUG
  allocated += btor->sorts_unique_table.size * sizeof (BtorSort *)
            + btor->sorts_unique_table.num_elements * sizeof (BtorSort)
            + BTOR_SIZE_STACK (btor->sorts_unique_table.id2sort) *
              sizeof (BtorSort *);
  for (i = 0; i < BTOR_COUNT_STACK (btor->sorts_unique_table.id2sort); i++)
    {
      sort = BTOR_PEEK_STACK (btor->sorts_unique_table.id2sort, i);
      if (!sort || sort->kind != BTOR_TUPLE_SORT) continue;
      allocated += sort->tuple.num_elements * sizeof (BtorSort *);
    }
  assert (allocated == clone->mm->allocated);
#endif

  emap = btor_new_node_map (clone);
  assert ((allocated += sizeof (*emap) + MEM_PTR_HASH_TABLE (emap->table))
          == clone->mm->allocated);

  BTOR_INIT_STACK (rhos);
  BTORLOG_TIMESTAMP (delta);
  clone_nodes_id_table (btor, clone, &btor->nodes_id_table,
                        &clone->nodes_id_table, emap, exp_layer_only, &rhos);
  BTORLOG (1, "  clone nodes id table: %.3f s", (btor_time_stamp () - delta));
#ifndef NDEBUG
  for (i = 1; i < BTOR_COUNT_STACK (btor->nodes_id_table); i++)
    {
      if (!(cur = BTOR_PEEK_STACK (btor->nodes_id_table, i))) continue;
      allocated += cur->bytes;
      if (BTOR_IS_BV_CONST_NODE (cur))
        {
          allocated += MEM_BITVEC (btor_const_get_bits (cur));
          if (btor_const_get_invbits (cur))
            allocated += MEM_BITVEC (btor_const_get_invbits (cur));
        }
      if (!exp_layer_only)
        {
          if (!BTOR_IS_FUN_NODE (cur) && cur->av)
            allocated += sizeof (*(cur->av))
                         + cur->av->len * sizeof (BtorAIG *);
        }
      if (BTOR_IS_LAMBDA_NODE (cur) && btor_lambda_get_static_rho (cur)) 
        allocated += MEM_PTR_HASH_TABLE (btor_lambda_get_static_rho (cur));
    }
  /* Note: hash table is initialized with size 1 */
  allocated += (emap->table->size - 1) * sizeof (BtorPtrHashBucket *)
            + emap->table->count * sizeof (BtorPtrHashBucket)
            + BTOR_SIZE_STACK (btor->nodes_id_table) * sizeof (BtorNode *);
  assert (allocated == clone->mm->allocated);
#endif

  clone->true_exp = btor_mapped_node (emap, btor->true_exp);
  assert (clone->true_exp);

  BTORLOG_TIMESTAMP (delta);
  clone_nodes_unique_table (
      mm, &btor->nodes_unique_table, &clone->nodes_unique_table, emap);
  BTORLOG (1, "  clone nodes unique table: %.3f s",
           (btor_time_stamp () - delta));
  assert ((allocated += btor->nodes_unique_table.size * sizeof (BtorNode *)) 
          == clone->mm->allocated);

  clone->symbols = btor_clone_ptr_hash_table (
                       mm, btor->symbols, btor_clone_key_as_str, 0, 0, 0);
#ifndef NDEBUG
  int str_bytes = 0;
  btor_init_hash_table_iterator (&it, btor->symbols);
  while (btor_has_next_hash_table_iterator (&it))
    str_bytes += (strlen ((char *) btor_next_hash_table_iterator (&it)) + 1)
                 * sizeof (char);
  assert ((allocated += MEM_PTR_HASH_TABLE (btor->symbols) + str_bytes)
          == clone->mm->allocated);
#endif
  clone->node2symbol = btor_clone_ptr_hash_table (mm, btor->node2symbol,
      btor_clone_key_as_node, btor_clone_data_as_str_ptr, emap, clone->symbols);
#ifndef NDEBUG
  assert ((allocated += MEM_PTR_HASH_TABLE (btor->node2symbol)) 
          == clone->mm->allocated);
#endif

  CLONE_PTR_HASH_TABLE (inputs);
  assert ((allocated += MEM_PTR_HASH_TABLE (btor->inputs)) 
          == clone->mm->allocated);
  CLONE_PTR_HASH_TABLE (bv_vars);
  assert ((allocated += MEM_PTR_HASH_TABLE (btor->bv_vars)) 
          == clone->mm->allocated);
  CLONE_PTR_HASH_TABLE (ufs);
  assert ((allocated += MEM_PTR_HASH_TABLE (btor->ufs)) 
          == clone->mm->allocated);
  CLONE_PTR_HASH_TABLE_DATA (lambdas, btor_clone_data_as_int);
  assert ((allocated += MEM_PTR_HASH_TABLE (btor->lambdas)) 
          == clone->mm->allocated);
  CLONE_PTR_HASH_TABLE_DATA (feqs, btor_clone_data_as_int);
  assert ((allocated += MEM_PTR_HASH_TABLE (btor->feqs)) 
          == clone->mm->allocated);
  CLONE_PTR_HASH_TABLE_DATA (substitutions, btor_clone_data_as_node_ptr);
  assert ((allocated += MEM_PTR_HASH_TABLE (btor->substitutions)) 
          == clone->mm->allocated);
  CLONE_PTR_HASH_TABLE_DATA (varsubst_constraints, btor_clone_data_as_node_ptr);
  assert ((allocated += MEM_PTR_HASH_TABLE (btor->varsubst_constraints)) 
          == clone->mm->allocated);
  CLONE_PTR_HASH_TABLE (embedded_constraints);
  assert ((allocated += MEM_PTR_HASH_TABLE (btor->embedded_constraints)) 
          == clone->mm->allocated);
  CLONE_PTR_HASH_TABLE (unsynthesized_constraints);
  assert ((allocated += MEM_PTR_HASH_TABLE (btor->unsynthesized_constraints)) 
          == clone->mm->allocated);
  CLONE_PTR_HASH_TABLE (synthesized_constraints);
  assert ((allocated += MEM_PTR_HASH_TABLE (btor->synthesized_constraints)) 
          == clone->mm->allocated);
  CLONE_PTR_HASH_TABLE (assumptions);
  assert ((allocated += MEM_PTR_HASH_TABLE (btor->assumptions))
          == clone->mm->allocated);
  CLONE_PTR_HASH_TABLE (var_rhs);
  assert ((allocated += MEM_PTR_HASH_TABLE (btor->var_rhs))
          == clone->mm->allocated);
  CLONE_PTR_HASH_TABLE (fun_rhs);
  assert ((allocated += MEM_PTR_HASH_TABLE (btor->fun_rhs))
          == clone->mm->allocated);
  CLONE_PTR_HASH_TABLE_DATA (bv_model, btor_clone_data_as_bv_ptr);
#ifndef NDEBUG
  if (btor->bv_model)
    {
      btor_init_hash_table_iterator (&it, btor->bv_model);
      btor_init_hash_table_iterator (&cit, clone->bv_model);
      while (btor_has_next_node_hash_table_iterator (&it))
        {
          data = btor_next_data_hash_table_iterator (&it);
          cdata = btor_next_data_hash_table_iterator (&cit);
          assert (btor_size_bv ((BtorBitVector *) data->asPtr)
                  == btor_size_bv ((BtorBitVector *) cdata->asPtr));
          allocated += btor_size_bv ((BtorBitVector *) cdata->asPtr);
        }
    }
#endif
  assert ((allocated += MEM_PTR_HASH_TABLE (btor->bv_model))
              == clone->mm->allocated);
  CLONE_PTR_HASH_TABLE_DATA (fun_model, btor_clone_data_as_bv_htable_ptr);
#ifndef NDEBUG
  if (btor->fun_model)
    {
      btor_init_hash_table_iterator (&it, btor->fun_model);
      btor_init_hash_table_iterator (&cit, clone->fun_model);
      while (btor_has_next_hash_table_iterator (&it))
        {
          data = btor_next_data_hash_table_iterator (&it);
          cdata = btor_next_data_hash_table_iterator (&cit);
          assert (MEM_PTR_HASH_TABLE ((BtorPtrHashTable *) data->asPtr) ==
                  MEM_PTR_HASH_TABLE ((BtorPtrHashTable *) cdata->asPtr));
          allocated += MEM_PTR_HASH_TABLE ((BtorPtrHashTable *) data->asPtr);

          btor_init_hash_table_iterator (&ncit,
                                         ((BtorPtrHashTable *) data->asPtr));
          while (btor_has_next_hash_table_iterator (&ncit))
            {
              allocated += btor_size_bv (
                  (BtorBitVector *) ncit.bucket->data.asPtr);
              allocated += btor_size_bv_tuple (
                  (BtorBitVectorTuple *) btor_next_hash_table_iterator (&ncit));
            }
        }
    }
#endif
  assert ((allocated += MEM_PTR_HASH_TABLE (btor->fun_model))
              == clone->mm->allocated);

  /* NOTE: we need bv_model for cloning rhos */
  while (!BTOR_EMPTY_STACK (rhos))
    {
      exp = BTOR_POP_STACK (rhos);
      cloned_exp = BTOR_POP_STACK (rhos);
      assert (BTOR_IS_FUN_NODE (exp));
      assert (BTOR_IS_FUN_NODE (cloned_exp));
      assert (exp->rho);
      cloned_exp->rho = 
          btor_clone_ptr_hash_table (mm, exp->rho, btor_clone_key_as_node,
                                     btor_clone_data_as_node_ptr, emap, emap);
#ifndef NDEBUG
      allocated += MEM_PTR_HASH_TABLE (cloned_exp->rho);
#endif
    }
  BTOR_RELEASE_STACK (btor->mm, rhos);
  assert (allocated == clone->mm->allocated);

  if (exp_layer_only)
    {
      BTOR_INIT_STACK (clone->functions_with_model);
      /* we need to decrement the reference count of the cloned expressions
       * that were pushed onto the functions_with_model stack. */
      for (i = 0; i < BTOR_COUNT_STACK (btor->functions_with_model); i++)
        btor_release_exp (clone,
            btor_mapped_node (emap,
                BTOR_PEEK_STACK (btor->functions_with_model, i)));
    }
  else
    {
      BTORLOG_TIMESTAMP (delta);
      btor_clone_node_ptr_stack (
          mm, &btor->functions_with_model, &clone->functions_with_model, emap);
      BTORLOG (1, "  clone functions_with_model: %.3f s",
               btor_time_stamp () - delta);
      assert ((allocated += BTOR_SIZE_STACK (btor->functions_with_model) 
                            * sizeof (BtorNode *))
              == clone->mm->allocated);
    }

  CLONE_PTR_HASH_TABLE_DATA (cache, btor_clone_data_as_node_ptr);
  assert ((allocated += MEM_PTR_HASH_TABLE (btor->cache))
          == clone->mm->allocated);

  BTORLOG_TIMESTAMP (delta);
  clone->parameterized = btor_clone_ptr_hash_table (mm, btor->parameterized, 
      btor_clone_key_as_node, btor_clone_data_as_htable_ptr, emap, emap); 
  BTORLOG (1, "  clone parameterized table: %.3f s",
           (btor_time_stamp () - delta));
#ifndef NDEBUG
  CHKCLONE_MEM_PTR_HASH_TABLE (btor->parameterized, clone->parameterized);
  allocated += MEM_PTR_HASH_TABLE (btor->parameterized);
  btor_init_node_hash_table_iterator (&it, btor->parameterized);
  btor_init_node_hash_table_iterator (&cit, clone->parameterized);
  while (btor_has_next_node_hash_table_iterator (&it))
    {
      assert (MEM_PTR_HASH_TABLE ((BtorPtrHashTable *) it.bucket->data.asPtr)
          == MEM_PTR_HASH_TABLE ((BtorPtrHashTable *) cit.bucket->data.asPtr));
      allocated += 
        MEM_PTR_HASH_TABLE ((BtorPtrHashTable *) cit.bucket->data.asPtr);
      (void) btor_next_node_hash_table_iterator (&it);
      (void) btor_next_node_hash_table_iterator (&cit);
    }
  assert (allocated == clone->mm->allocated);
#endif

  /* move synthesized constraints to unsynthesized if we only clone the exp
   * layer */
  if (exp_layer_only)
    {
#ifndef NDEBUG
      allocated -= MEM_PTR_HASH_TABLE (clone->synthesized_constraints);
      allocated -= MEM_PTR_HASH_TABLE (clone->unsynthesized_constraints);
#endif
      btor_init_node_hash_table_iterator (&it, clone->synthesized_constraints);
      while (btor_has_next_node_hash_table_iterator (&it))
        {
          exp = btor_next_node_hash_table_iterator (&it);
          btor_insert_in_ptr_hash_table (clone->unsynthesized_constraints, exp);
        }
      btor_delete_ptr_hash_table (clone->synthesized_constraints);
      clone->synthesized_constraints = 
        btor_new_ptr_hash_table (mm, (BtorHashPtr) btor_hash_exp_by_id,
                                     (BtorCmpPtr) btor_compare_exp_by_id);
#ifndef NDEBUG
      allocated += MEM_PTR_HASH_TABLE (clone->synthesized_constraints);
      allocated += MEM_PTR_HASH_TABLE (clone->unsynthesized_constraints);
      assert (allocated == clone->mm->allocated);
#endif
    }

  if (btor->slv) clone->slv = btor->slv->api.clone (clone, btor, emap);
  assert ((btor->slv && clone->slv) || (!btor->slv && !clone->slv));
#ifndef NDEBUG
  if (clone->slv)
    {
      if (clone->slv->kind == BTOR_CORE_SOLVER_KIND)
        {
          BtorCoreSolver *slv = BTOR_CORE_SOLVER (btor);
          BtorCoreSolver *cslv = BTOR_CORE_SOLVER (clone);

          allocated += sizeof (BtorCoreSolver);

          allocated += MEM_PTR_HASH_TABLE (slv->lemmas);
          allocated += BTOR_SIZE_STACK (slv->cur_lemmas) * sizeof (BtorNode *);

          if (slv->score)
            {
              h = btor->options.just_heuristic.val;
              if (h == BTOR_JUST_HEUR_BRANCH_MIN_APP)
                {
                  CHKCLONE_MEM_PTR_HASH_TABLE (slv->score, cslv->score);
                  allocated += MEM_PTR_HASH_TABLE (slv->score);

                  btor_init_node_hash_table_iterator (&it, slv->score);
                  btor_init_node_hash_table_iterator (&cit, cslv->score);
                  while (btor_has_next_node_hash_table_iterator (&it))
                    {
                      assert (
        MEM_PTR_HASH_TABLE ((BtorPtrHashTable *) it.bucket->data.asPtr)
        == MEM_PTR_HASH_TABLE ((BtorPtrHashTable *) cit.bucket->data.asPtr));
                      allocated += MEM_PTR_HASH_TABLE (
                          (BtorPtrHashTable *) it.bucket->data.asPtr);
                      (void) btor_next_node_hash_table_iterator (&it);
                      (void) btor_next_node_hash_table_iterator (&cit);
                    }
                }
              else
                {
                  assert (h == BTOR_JUST_HEUR_BRANCH_MIN_DEP);
                  allocated += MEM_PTR_HASH_TABLE (slv->score);
                }
            }

          assert (BTOR_SIZE_STACK (slv->stats.lemmas_size) ==
                  BTOR_SIZE_STACK (cslv->stats.lemmas_size));
          assert (BTOR_COUNT_STACK (slv->stats.lemmas_size) ==
                  BTOR_COUNT_STACK (cslv->stats.lemmas_size));
          allocated += BTOR_SIZE_STACK (slv->stats.lemmas_size) * sizeof (int); 

          assert (allocated == clone->mm->allocated);
        }
    }
#endif

  clone->parse_error_msg = NULL;
#ifndef NDEBUG
  clone->clone = NULL;
#endif
  clone->close_apitrace = 0;

  if (exp_map)
    *exp_map = emap;
  else
    btor_delete_node_map (emap);

#ifndef NDEBUG
  /* flag sanity checks */
  btor_init_node_hash_table_iterator (&it, btor->synthesized_constraints);
  btor_queue_node_hash_table_iterator (&it, btor->unsynthesized_constraints);
  btor_queue_node_hash_table_iterator (&it, btor->embedded_constraints);
  while (btor_has_next_node_hash_table_iterator (&it))
    {
      exp = btor_next_node_hash_table_iterator (&it);
      assert (BTOR_REAL_ADDR_NODE (exp)->constraint);
    }
#endif

  btor->time.cloning += btor_time_stamp () - start;
  btor->stats.clone_calls += 1;
  BTORLOG (1, "cloning total: %.3f s", btor->time.cloning);
  return clone;
}

Btor * 
btor_clone_btor (Btor * btor)
{
  assert (btor);
  return clone_aux_btor (btor, 0,
              !btor_has_clone_support_sat_mgr (btor_get_sat_mgr_btor (btor)));
}

Btor *
btor_clone_exp_layer (Btor * btor, BtorNodeMap ** exp_map)
{
  assert (btor);
  return clone_aux_btor (btor, exp_map, true);
}

BtorNode *
btor_recursively_rebuild_exp_clone (Btor * btor, 
                                    Btor * clone, 
                                    BtorNode * exp,
                                    BtorNodeMap * exp_map,
                                    int rewrite_level)
{
  assert (btor);
  assert (exp);
  assert (BTOR_REAL_ADDR_NODE (exp)->btor == btor);
  assert (exp_map);

  int i, rwl;
  char *symbol;
  BtorNode *real_exp, *cur, *cur_clone, *e[3];
  BtorNodePtrStack work_stack, unmark_stack;
#ifndef NDEBUG
  BtorNodeMap * key_map = btor_new_node_map (btor);
#endif

  /* in some cases we may want to rebuild the expressions with a certain
   * rewrite level */
  rwl = clone->options.rewrite_level.val;
  if (rwl > 0)
    clone->options.rewrite_level.val = rewrite_level;

  BTOR_INIT_STACK (work_stack);
  BTOR_INIT_STACK (unmark_stack);

  real_exp = BTOR_REAL_ADDR_NODE (exp);
  BTOR_PUSH_STACK (btor->mm, work_stack, real_exp);
  while (!BTOR_EMPTY_STACK (work_stack))
    {
      cur = BTOR_REAL_ADDR_NODE (BTOR_POP_STACK (work_stack));
      
      if (btor_mapped_node (exp_map, cur))
        continue;

      if (cur->clone_mark == 2) continue;
      
      if (cur->clone_mark == 0)
        {
          cur->clone_mark = 1;
          BTOR_PUSH_STACK (btor->mm, unmark_stack, cur);
          BTOR_PUSH_STACK (btor->mm, work_stack, cur);
          for (i = 0; i < cur->arity; i++)
            BTOR_PUSH_STACK (btor->mm, work_stack, cur->e[i]);
        }
      else
        {
          assert (cur->clone_mark == 1);
          assert (!btor_mapped_node (exp_map, cur));
          assert (!BTOR_IS_PROXY_NODE (cur));
          for (i = 0; i < cur->arity; i++)
            {
              e[i] = btor_mapped_node (exp_map, cur->e[i]);
              assert (e[i]);
            }
          switch (cur->kind)
            {
              case BTOR_BV_CONST_NODE:
                cur_clone = btor_const_exp (clone, btor_const_get_bits (cur));
                break;
              case BTOR_BV_VAR_NODE:
                symbol = btor_find_in_ptr_hash_table (
                             btor->node2symbol, cur)->data.asStr;
                cur_clone = btor_var_exp (clone, btor_get_exp_width (btor, cur),
                                          symbol);
                break;
              case BTOR_PARAM_NODE:
                symbol = btor_find_in_ptr_hash_table (
                             btor->node2symbol, cur)->data.asStr;
                cur_clone = btor_param_exp (clone,
                                            btor_get_exp_width (btor, cur),
                                            symbol);
                break;
              case BTOR_UF_NODE:
                symbol = btor_find_in_ptr_hash_table (
                             btor->node2symbol, cur)->data.asStr;
                cur_clone = btor_uf_exp (clone, cur->sort_id, symbol);
                break;
              case BTOR_SLICE_NODE:
                cur_clone = btor_slice_exp (
                    clone, e[0], btor_slice_get_upper (cur),
                    btor_slice_get_lower (cur));
                break;
              case BTOR_AND_NODE:
                cur_clone = btor_and_exp (clone, e[0], e[1]);
                break;
              case BTOR_BEQ_NODE:
              case BTOR_FEQ_NODE:
                cur_clone = btor_eq_exp (clone, e[0], e[1]);
                break;
              case BTOR_ADD_NODE:
                cur_clone = btor_add_exp (clone, e[0], e[1]);
                break;
              case BTOR_MUL_NODE:
                cur_clone = btor_mul_exp (clone, e[0], e[1]);
                break;
              case BTOR_ULT_NODE:
                cur_clone = btor_ult_exp (clone, e[0], e[1]);
                break;
              case BTOR_SLL_NODE:
                cur_clone = btor_sll_exp (clone, e[0], e[1]);
                break;
              case BTOR_SRL_NODE:
                cur_clone = btor_srl_exp (clone, e[0], e[1]);
                break;
              case BTOR_UDIV_NODE:
                cur_clone = btor_udiv_exp (clone, e[0], e[1]);
                break;
              case BTOR_UREM_NODE:
                cur_clone = btor_urem_exp (clone, e[0], e[1]);
                break;
              case BTOR_CONCAT_NODE:
                cur_clone = btor_concat_exp (clone, e[0], e[1]);
                break;
              case BTOR_LAMBDA_NODE:
                assert (!btor_param_cur_assignment (e[0]));
                btor_param_set_binding_lambda (e[0], 0);
                cur_clone = btor_lambda_exp (clone, e[0], e[1]);
                break;
              case BTOR_APPLY_NODE:
                // FIXME use btor_apply_exp as soon as applies are
                // generated with rewriting (currently without)
                //cur_clone = btor_apply_exp (clone, e[0], e[1]);
                cur_clone = btor_apply_exp_node (clone, e[0], e[1]);
                break;
              case BTOR_ARGS_NODE:
                cur_clone = btor_args_exp (clone, cur->arity, e);
                break;
              default:
                assert (BTOR_IS_BV_COND_NODE (cur));
                cur_clone = btor_cond_exp (clone, e[0], e[1], e[2]);
            }
          btor_map_node (exp_map, cur, cur_clone);
#ifndef NDEBUG
          assert (!btor_mapped_node (key_map, cur_clone));
          assert (cur->kind == cur_clone->kind);
          btor_map_node (key_map, cur_clone, cur);
#endif
          btor_release_exp (clone, cur_clone);
        }
    }

  while (!BTOR_EMPTY_STACK (unmark_stack))
    BTOR_POP_STACK (unmark_stack)->clone_mark = 0;

  BTOR_RELEASE_STACK (btor->mm, work_stack);
  BTOR_RELEASE_STACK (btor->mm, unmark_stack);

  /* reset rewrite_level to original value */
  clone->options.rewrite_level.val = rwl;
#ifndef NDEBUG
  btor_delete_node_map (key_map);
#endif
  return btor_copy_exp (clone, btor_mapped_node (exp_map, exp));
}
