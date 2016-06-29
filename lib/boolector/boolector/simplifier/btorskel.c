/*  Boolector: Satisfiablity Modulo Theories (SMT) solver.
 *
 *  Copyright (C) 2007-2009 Robert Daniel Brummayer.
 *  Copyright (C) 2007-2014 Armin Biere.
 *  Copyright (C) 2012-2015 Mathias Preiner.
 *  Copyright (C) 2012-2015 Aina Niemetz.
 *
 *  All rights reserved.
 *
 *  This file is part of Boolector.
 *  See COPYING for more information on using this software.
 */

#ifdef BTOR_USE_LINGELING
#include "simplifier/btorskel.h"
#include "btorcore.h"
#include "utils/btoriter.h"
#include "utils/btorutil.h"
#include "btordbg.h"

#include "lglib.h"

static int
fixed_exp (Btor * btor, BtorNode * exp)
{
  BtorNode * real_exp;
  BtorSATMgr * smgr;
  BtorAIG * aig;
  int res, id;

  real_exp = BTOR_REAL_ADDR_NODE (exp);
  assert (btor_get_exp_width (btor, real_exp) == 1);
  if (! BTOR_IS_SYNTH_NODE (real_exp))
    return 0;
  assert (real_exp->av);
  assert (real_exp->av->len == 1);
  assert (real_exp->av->aigs);
  aig = real_exp->av->aigs[0];
  if (aig == BTOR_AIG_TRUE)
    res = 1;
  else if (aig == BTOR_AIG_FALSE)
    res = -1;
  else
    {
      id = BTOR_GET_CNF_ID_AIG (aig);
      if (!id)
        return 0;
      smgr = btor_get_sat_mgr_btor (btor);
      res = btor_fixed_sat (smgr, id);
    }
  if (BTOR_IS_INVERTED_NODE (exp))
    res = -res;
  return res;
}

static int
process_skeleton_tseitin_lit (BtorPtrHashTable * ids, BtorNode * exp)
{
  BtorPtrHashBucket * b;
  BtorNode * real_exp;
  int res;

  real_exp = BTOR_REAL_ADDR_NODE (exp);
  assert (btor_get_exp_width (real_exp->btor, real_exp) == 1);
  b = btor_find_in_ptr_hash_table (ids, real_exp);
  if (!b)
    {
      b = btor_insert_in_ptr_hash_table (ids, real_exp);
      b->data.asInt = (int) ids->count;
    }

  res = b->data.asInt;
  assert (res > 0);

  if (BTOR_IS_INVERTED_NODE (exp))
    res = -res;

  return res;
}

static void
process_skeleton_tseitin (Btor * btor, LGL * lgl,
                          BtorNodePtrStack * work_stack,
                          BtorNodePtrStack * unmark_stack,
                          BtorPtrHashTable * ids,
                          BtorNode * root)
{
  assert (btor);

  int i, lhs, rhs[3], fixed;
  BtorNode * exp;

  BTOR_PUSH_STACK (btor->mm, *work_stack, root);

  do {
    exp = BTOR_POP_STACK (*work_stack);
    assert (exp);
    exp = BTOR_REAL_ADDR_NODE (exp);
    if (!exp->mark)
      {
        exp->mark = 1;
        BTOR_PUSH_STACK (btor->mm, *unmark_stack, exp);

        BTOR_PUSH_STACK (btor->mm, *work_stack, exp);
        for (i = exp->arity-1; i >= 0; i--)
          BTOR_PUSH_STACK (btor->mm, *work_stack, exp->e[i]);
      }
    else if (exp->mark == 1)
      {
        exp->mark = 2;
        if (BTOR_IS_FUN_NODE (exp)
            || BTOR_IS_ARGS_NODE (exp)
            || exp->parameterized
            || btor_get_exp_width (btor, exp) != 1)
          continue;

#ifndef NDEBUG
        for (i = 0; i < exp->arity; i++)
          {
            BtorNode * child = exp->e[i];
            child = BTOR_REAL_ADDR_NODE (child);
            assert (child->mark == 2);
            if (!BTOR_IS_FUN_NODE (child)
                && !BTOR_IS_ARGS_NODE (child)
                && !child->parameterized
                && btor_get_exp_width (btor, child) == 1)
              assert (btor_find_in_ptr_hash_table (ids, child));
          }
#endif
        lhs = process_skeleton_tseitin_lit (ids, exp);
        fixed = fixed_exp (btor, exp);
        if (fixed)
          {
            lgladd (lgl, (fixed > 0) ? lhs : -lhs);
            lgladd (lgl, 0);
          }

        switch (exp->kind)
          {
            case BTOR_AND_NODE:
              rhs[0] = process_skeleton_tseitin_lit (ids, exp->e[0]);
              rhs[1] = process_skeleton_tseitin_lit (ids, exp->e[1]);

              lgladd (lgl, -lhs);
              lgladd (lgl, rhs[0]);
              lgladd (lgl, 0);

              lgladd (lgl, -lhs);
              lgladd (lgl, rhs[1]);
              lgladd (lgl, 0);

              lgladd (lgl, lhs);
              lgladd (lgl, -rhs[0]);
              lgladd (lgl, -rhs[1]);
              lgladd (lgl, 0);
              break;

            case BTOR_BEQ_NODE:
              if (btor_get_exp_width (btor, exp->e[0]) != 1)
                break;
              assert (btor_get_exp_width (btor, exp->e[1]) == 1);
              rhs[0] = process_skeleton_tseitin_lit (ids, exp->e[0]);
              rhs[1] = process_skeleton_tseitin_lit (ids, exp->e[1]);

              lgladd (lgl, -lhs);
              lgladd (lgl, -rhs[0]);
              lgladd (lgl, rhs[1]);
              lgladd (lgl, 0);

              lgladd (lgl, -lhs);
              lgladd (lgl, rhs[0]);
              lgladd (lgl, -rhs[1]);
              lgladd (lgl, 0);

              lgladd (lgl, lhs);
              lgladd (lgl, rhs[0]);
              lgladd (lgl, rhs[1]);
              lgladd (lgl, 0);

              lgladd (lgl, lhs);
              lgladd (lgl, -rhs[0]);
              lgladd (lgl, -rhs[1]);
              lgladd (lgl, 0);

              break;

            case BTOR_BCOND_NODE:
              assert (btor_get_exp_width (btor, exp->e[0]) == 1);
              if (btor_get_exp_width (btor, exp->e[1]) != 1)
                break;
              assert (btor_get_exp_width (btor, exp->e[2]) == 1);
              rhs[0] = process_skeleton_tseitin_lit (ids, exp->e[0]);
              rhs[1] = process_skeleton_tseitin_lit (ids, exp->e[1]);
              rhs[2] = process_skeleton_tseitin_lit (ids, exp->e[2]);

              lgladd (lgl, -lhs);
              lgladd (lgl, -rhs[0]);
              lgladd (lgl, rhs[1]);
              lgladd (lgl, 0);

              lgladd (lgl, -lhs);
              lgladd (lgl, rhs[0]);
              lgladd (lgl, rhs[2]);
              lgladd (lgl, 0);

              lgladd (lgl, lhs);
              lgladd (lgl, -rhs[0]);
              lgladd (lgl, -rhs[1]);
              lgladd (lgl, 0);

              lgladd (lgl, lhs);
              lgladd (lgl, rhs[0]);
              lgladd (lgl, -rhs[2]);
              lgladd (lgl, 0);
              break;

            default:
              assert (exp->kind != BTOR_PROXY_NODE);
              break;
          }
      }
  } while (!BTOR_EMPTY_STACK (*work_stack));
}

void
btor_process_skeleton (Btor * btor)
{
  BtorPtrHashTable * ids;
  BtorNodePtrStack unmark_stack;
  int count, fixed;
  BtorNodePtrStack work_stack;
  BtorMemMgr * mm = btor->mm;
  BtorHashTableIterator it;
  double start, delta;
  int res, lit, val;
  BtorNode * exp;
  LGL * lgl;

  start = btor_time_stamp ();

  ids = btor_new_ptr_hash_table (mm,
          (BtorHashPtr) btor_hash_exp_by_id,
          (BtorCmpPtr) btor_compare_exp_by_id);

  lgl = lglinit ();
  lglsetprefix (lgl, "[lglskel] ");

  if (btor->options.verbosity.val >= 2)
    {
      lglsetopt (lgl, "verbose", btor->options.verbosity.val - 1);
      lglbnr ("Lingeling", "[lglskel] ", stdout);
      fflush (stdout);
    }
  else
    lglsetopt (lgl, "verbose", -1);        

  count = 0;

  BTOR_INIT_STACK (work_stack);
  BTOR_INIT_STACK (unmark_stack);

  btor_init_node_hash_table_iterator (&it, btor->synthesized_constraints);
  btor_queue_node_hash_table_iterator (&it, btor->unsynthesized_constraints);
  while (btor_has_next_node_hash_table_iterator (&it))
    {
      count++;
      exp = btor_next_node_hash_table_iterator (&it);
      assert (btor_get_exp_width (btor, exp) == 1);
      process_skeleton_tseitin (btor, lgl,
        &work_stack, &unmark_stack, ids, exp);
      lgladd (lgl, process_skeleton_tseitin_lit (ids, exp));
      lgladd (lgl, 0);
    }

  BTOR_RELEASE_STACK (mm, work_stack);

  while (!BTOR_EMPTY_STACK (unmark_stack))
    {
      exp = BTOR_POP_STACK (unmark_stack);
      assert (!BTOR_IS_INVERTED_NODE (exp));
      assert (exp->mark);
      exp->mark = 0;
    }

  BTOR_RELEASE_STACK (mm, unmark_stack);

  BTOR_MSG (btor->msg, 1, "found %u skeleton literals in %d constraints",
                ids->count, count);

  res = lglsimp (lgl, 0);

  if (btor->options.verbosity.val)
    {
      BTOR_MSG (btor->msg, 1, "skeleton preprocessing result %d", res);
      lglstats (lgl);
    }

  fixed = 0;

  if (res == 20)
    {
      BTOR_MSG (btor->msg, 1, "skeleton inconsistent");
      btor->inconsistent = 1;
    }
  else
    {
      assert (res == 0 || res == 10);
      btor_init_node_hash_table_iterator (&it, ids);
      while (btor_has_next_node_hash_table_iterator (&it))
        {
          exp = btor_next_node_hash_table_iterator (&it);
          assert (!BTOR_IS_INVERTED_NODE (exp));
          lit = process_skeleton_tseitin_lit (ids, exp);
          val = lglfixed (lgl, lit);
          if (val)
            {
              if (!btor_find_in_ptr_hash_table (
                    btor->synthesized_constraints, exp) &&
                  !btor_find_in_ptr_hash_table (
                    btor->unsynthesized_constraints, exp))
                {
                  if (val < 0)
                    exp = BTOR_INVERT_NODE (exp);
                  btor_assert_exp (btor, exp);
                  btor->stats.skeleton_constraints++;
                  fixed++;
                }
            }
          else
            {
              assert (!btor_find_in_ptr_hash_table (
                         btor->synthesized_constraints, exp));
              assert (!btor_find_in_ptr_hash_table (
                         btor->unsynthesized_constraints, exp));
            }
        }
    }

  btor_delete_ptr_hash_table (ids);
  lglrelease (lgl);

  delta = btor_time_stamp () - start;
  btor->time.skel += delta;
  BTOR_MSG (btor->msg, 1,
      "skeleton preprocessing produced %d new constraints in %.1f seconds",
      fixed, delta);
  assert (btor_check_id_table_mark_unset_dbg (btor));
}
#endif
