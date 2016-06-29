/*  Boolector: Satisfiablity Modulo Theories (SMT) solver.
 *
 *  Copyright (C) 2012-2013 Armin Biere.
 *
 *  All rights reserved.
 *
 *  This file is part of Boolector.
 *  See COPYING for more information on using this software.
 */

/*------------------------------------------------------------------------*/

#ifndef btormc_h_INCLUDED
#define btormc_h_INCLUDED

/*------------------------------------------------------------------------*/

#include "boolector.h"

/*------------------------------------------------------------------------*/

typedef struct BtorMC BtorMC;

/*------------------------------------------------------------------------*/

BtorMC * boolector_new_mc (void);
void boolector_delete_mc (BtorMC *);

void boolector_set_verbosity_mc (BtorMC *, int verbosity);

/* Default is to stop at the first reached bad state property.  Given a zero
 * argument 'stop == 0' to this function will result in the model checker to
 * run until all properties have been reached (or proven not to be
 * reachable) or the maximum bound is reached.
 */
void boolector_set_stop_at_first_reached_property_mc (BtorMC *, int stop);

/* In order to be able to obtain the trace after model checking you
 * need to request trace generation (before calling 'boolector_bmc').
 */
void boolector_enable_trace_gen (BtorMC *);

/*------------------------------------------------------------------------*/

Btor * boolector_btor_mc (BtorMC *);

/*------------------------------------------------------------------------*/

BoolectorNode * boolector_input (BtorMC *, int width, const char *);
BoolectorNode * boolector_latch (BtorMC *, int width, const char *);

void boolector_next (BtorMC *,
                     BoolectorNode * latch, BoolectorNode * next);

void boolector_init (BtorMC *,
                     BoolectorNode * latch, BoolectorNode * init);

int boolector_bad (BtorMC *, BoolectorNode * bad);

int boolector_constraint (BtorMC *, BoolectorNode * constraint);

/*------------------------------------------------------------------------*/

void boolector_dump_btormc (BtorMC *, FILE *);

/*------------------------------------------------------------------------*/

int boolector_bmc (BtorMC *, int mink, int maxk);

/*------------------------------------------------------------------------*/

/* Return the 'k' at which a previous model checking run proved that the bad
 * state property with index 'badidx' was reached or a negative number if
 * it was not reached.  This does not make much sense if the model checker
 * stops at the first reached property.  So if this function is used
 * it is an error if the user did not request to continue after the first
 * property was reached.
 */
int boolector_reached_bad_at_bound_mc (BtorMC *, int badidx);

/* Alternatively the user can provide a call back function which is called
 * the first time a bad state property is reached.  The same comment as in
 * the previous function applies, e.g. the user might want to call
 * 'boolector_set_stop_at_first_reached_property_mc (btormc, 0)' before
 * calling the model checker.
 */
typedef void (*BtorMCReachedAtBound)(void *, int badidx, int k);

void
boolector_set_reached_at_bound_call_back_mc (
  BtorMC *,
  void * state, 
  BtorMCReachedAtBound fun);

/*------------------------------------------------------------------------*/

typedef void (*BtorMCStartingBound)(void *, int k);

void
boolector_set_starting_bound_call_back_mc (
 BtorMC *,
 void * state,
 BtorMCStartingBound fun);

/*------------------------------------------------------------------------*/

/* Assumes that 'boolector_enable_trace_gen' was called and then
 * 'boolector_bmc' which returned a 'k' with '0 <= time <= k'.
 */
char * boolector_mc_assignment (BtorMC *, 
                                BoolectorNode * input_or_latch,
                                     int time);

/* The caller of 'boolector_mc_assignment' has to release the returned 
 * assignment with this 'boolector_free_mc_assignment' again.
 */
void boolector_free_mc_assignment (BtorMC *, char *);

/*------------------------------------------------------------------------*/
#endif
