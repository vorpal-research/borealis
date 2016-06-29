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

#ifndef BTORCORE_H_INCLUDED
#define BTORCORE_H_INCLUDED

#include "btortypes.h"
#include "btorass.h"
#include "btorexp.h"
#include "utils/btormem.h"
#include "btorsort.h"
#include "btoropt.h"
#include "btorsat.h"
#include "btormsg.h"
#include "btorslv.h"
#include "btorbitvec.h"

/*------------------------------------------------------------------------*/

#define BTOR_VERBOSITY_MAX 4

/*------------------------------------------------------------------------*/

//#define BTOR_DO_NOT_OPTIMIZE_UNCONSTRAINED

/*------------------------------------------------------------------------*/

// Currently, 'BoolectorNode' (external) vs. 'BtorNode' (internal) 
// syntactically hides internal nodes.  Hence, we assume that both structs
// 'BoolectorNode' and 'BtorNode' have/ the same structure and provide the
// following macros for type conversion (via typecasting).  We further assume
// that external 'boolector_xxx' functions provide the same functionality as
// their internal counter part 'btor_xxx' (except for API tracing and contract
// checks).
//
// If the assumption above does not hold, we have to provide
// real containers for 'BoolectorNode' (cf. 'BoolectorNodeMap').

#define BTOR_IMPORT_BOOLECTOR_NODE(node) (((BtorNode *) (node)))
#define BTOR_IMPORT_BOOLECTOR_NODE_ARRAY(array) (((BtorNode **) (array)))
#define BTOR_EXPORT_BOOLECTOR_NODE(node) (((BoolectorNode *) (node)))
#define BTOR_IMPORT_BOOLECTOR_SORT(sort) (((BtorSortId) (sort)))
#define BTOR_EXPORT_BOOLECTOR_SORT(sort) (((BoolectorSort) (sort)))

/*------------------------------------------------------------------------*/

#define BOOLECTOR_IS_REGULAR_NODE BTOR_IS_INVERTED_NODE
#define BOOLECTOR_IS_INVERTED_NODE BTOR_IS_INVERTED_NODE

#define BOOLECTOR_REAL_ADDR_NODE(node) \
  BTOR_EXPORT_BOOLECTOR_NODE ( \
    BTOR_REAL_ADDR_NODE (BTOR_IMPORT_BOOLECTOR_NODE (node)))

#define BOOLECTOR_INVERT_NODE(node) \
  BTOR_EXPORT_BOOLECTOR_NODE ( \
    BTOR_INVERT_NODE (BTOR_IMPORT_BOOLECTOR_NODE (node)))

/*------------------------------------------------------------------------*/

struct BtorNodeUniqueTable
{
  int size;
  int num_elements;
  BtorNode **chains;
};

typedef struct BtorNodeUniqueTable BtorNodeUniqueTable;

struct BtorCallbacks
{
  struct
    {
      /* the function to use for (checking) termination
       * (we need to distinguish between callbacks from C and Python) */
      int (*termfun)(void*);

      void *fun;               /* termination callback function */
      void *state;               /* termination callback function arguments */
      int done;
    } term;
};

typedef struct BtorCallbacks BtorCallbacks;

struct BtorConstraintStats
{
  int varsubst;
  int embedded;
  int unsynthesized;
  int synthesized;
};

typedef struct BtorConstraintStats BtorConstraintStats;

// TODO (ma): array_assignments -> fun_assignments
struct Btor
{
  BtorMemMgr *mm;
  BtorSolver *slv;
  BtorCallbacks cbs;

  BtorBVAssignmentList *bv_assignments;
  BtorArrayAssignmentList *array_assignments;

  BtorNodePtrStack nodes_id_table;
  BtorNodeUniqueTable nodes_unique_table;
  BtorSortUniqueTable sorts_unique_table;

  BtorAIGVecMgr *avmgr;

  BtorPtrHashTable *symbols;
  BtorPtrHashTable *node2symbol;

  BtorPtrHashTable *inputs;
  BtorPtrHashTable *bv_vars;
  BtorPtrHashTable *ufs;
  BtorPtrHashTable *lambdas;
  BtorPtrHashTable *feqs;
  BtorPtrHashTable *parameterized;

  BtorPtrHashTable *substitutions;

  BtorNode *true_exp;
  
  BtorPtrHashTable *bv_model;
  BtorPtrHashTable *fun_model;
  BtorNodePtrStack functions_with_model;

  int rec_rw_calls;             /* calls for recursive rewriting */
  int valid_assignments;
  int vis_idx;                  /* file index for visualizing expressions */
  int inconsistent;
  int found_constraint_false;
  int external_refs;            /* external references (library mode) */
  int btor_sat_btor_called;     /* how often is btor_sat_btor been called */
  int last_sat_result;                /* status of last SAT call (SAT/UNSAT) */

  BtorPtrHashTable *varsubst_constraints;
  BtorPtrHashTable *embedded_constraints;
  BtorPtrHashTable *unsynthesized_constraints;
  BtorPtrHashTable *synthesized_constraints;

  BtorPtrHashTable *assumptions;
  
  BtorPtrHashTable *var_rhs;
  BtorPtrHashTable *fun_rhs;
  
  BtorPtrHashTable *cache;      /* for btor_simplify_btor */

#ifndef NDEBUG
  Btor *clone;  /* shadow clone (debugging only) */
#endif

  char *parse_error_msg;

  FILE *apitrace;
  int close_apitrace;

  BtorOpts options;
  BtorMsg *msg;

  struct
  {
    int cur, max;
  } ops[BTOR_NUM_OPS_NODE];

  struct
  {
    int max_rec_rw_calls;       /* maximum number of recursive rewrite calls */
    int var_substitutions;      /* number substituted vars */
    int uf_substitutions;        /* num substituted uninterpreted functions */
    int ec_substitutions;       /* embedded constraint substitutions */
    int linear_equations;       /* number of linear equations */
    int gaussian_eliminations;  /* number of gaussian eliminations */
    int eliminated_slices;      /* number of eliminated slices */
    int skeleton_constraints;   /* number of extracted skeleton constraints */
    int adds_normalized;        /* number of add chains normalizations */
    int ands_normalized;        /* number of and chains normalizations */
    int muls_normalized;        /* number of mul chains normalizations */
    int ackermann_constraints;
    long long apply_props_construct;  /* number of static apply propagations */
#ifndef BTOR_DO_NOT_OPTIMIZE_UNCONSTRAINED
    int bv_uc_props;
    int fun_uc_props;
    int param_uc_props;
#endif
    long long lambdas_merged;
    BtorConstraintStats constraints;
    BtorConstraintStats oldconstraints;
    long long expressions;
    long long clone_calls;
    size_t node_bytes_alloc;
    long long beta_reduce_calls;
  } stats;

  struct
  {
    double rewrite;
    double subst;
    double subst_rebuild;
    double betareduce;
    double embedded;
    double slicing;
    double skel;
    double propagate;
    double beta;
    double reachable;
    double failed;
    double cloning;
    double synth_exp;
    double model_gen;
    double br_probing;
#ifndef BTOR_DO_NOT_OPTIMIZE_UNCONSTRAINED
    double ucopt;
#endif
  } time;
};

/* Creates new boolector instance. */
Btor *btor_new_btor (void);

/* Creates new boolector instance without initializing options. */
Btor *btor_new_btor_no_init (void);

/* Deletes boolector. */
void btor_delete_btor (Btor * btor);

/* Gets version. */
const char *btor_version (Btor * btor);

/* Set termination callback. */
void btor_set_term_btor (Btor * btor, int (*fun) (void*), void * state);

/* Determine if boolector has been terminated via termination callback. */
int btor_terminate_btor (Btor * btor);

/* Set verbosity message prefix. */
void btor_set_msg_prefix_btor (Btor * btor, const char * prefix);

/* Prints statistics. */
void btor_print_stats_btor (Btor * btor);

/* Reset time statistics. */
void btor_reset_time_btor (Btor * btor);

/* Reset other statistics. */
void btor_reset_stats_btor (Btor * btor);

/* Adds top level constraint. */
void btor_assert_exp (Btor * btor, BtorNode * exp);

/* Adds assumption. */
void btor_assume_exp (Btor * btor, BtorNode * exp);

/* Determines if expression has been previously assumed. */
int btor_is_assumption_exp (Btor * btor, BtorNode * exp);

/* Determines if assumption is a failed assumption. */
int btor_failed_exp (Btor * btor, BtorNode * exp);

/* Adds assumptions as assertions and resets the assumptions. */
void btor_fixate_assumptions (Btor * btor);

/* Resets assumptions */
void btor_reset_assumptions (Btor * btor);

/* Solves instance, but with lemmas on demand limit 'lod_limit' and conflict
 * limit for the underlying SAT solver 'sat_limit'. */
int btor_sat_btor (Btor * btor, int lod_limit, int sat_limit);

BtorSATMgr * btor_get_sat_mgr_btor (const Btor * btor);
BtorAIGMgr * btor_get_aig_mgr_btor (const Btor * btor);
BtorMemMgr * btor_get_mem_mgr_btor (const Btor * btor);

/* Run rewriting engine */
int btor_simplify (Btor * btor);

/*------------------------------------------------------------------------*/

#define BTOR_CORE_SOLVER(btor) ((BtorCoreSolver *) (btor)->slv)


struct BtorCoreSolver
{
  BTOR_SOLVER_STRUCT;

  BtorPtrHashTable *lemmas;
  BtorNodePtrStack cur_lemmas;

  BtorPtrHashTable *score;               /* dcr score */

  /* compare fun for sorting the inputs in search_inital_applies_dual_prop */
  int (* dp_cmp_inputs) (const void *, const void *);
  
  struct
  {
    int lod_refinements;        /* number of lemmas on demand refinements */
    int refinement_iterations;

    int function_congruence_conflicts;
    int beta_reduction_conflicts;
    int extensionality_lemmas;

    BtorIntStack lemmas_size;       /* distribution of n-size lemmas */
    long long int lemmas_size_sum;  /* sum of the size of all added lemmas */

    int dp_failed_vars;         /* number of vars in FA (dual prop) of last
                                   sat call (final bv skeleton) */
    int dp_assumed_vars;
    int dp_failed_applies;      /* number of applies in FA (dual prop) of last
                                   sat call (final bv skeleton) */
    int dp_assumed_applies;
    int dp_failed_eqs;
    int dp_assumed_eqs;

    long long eval_exp_calls;
    long long propagations;
    long long propagations_down;
    long long partial_beta_reduction_restarts;
  } stats;

  struct
  {
    double sat;
    double eval;
    double search_init_apps;
    double search_init_apps_compute_scores;
    double search_init_apps_compute_scores_merge_applies;
    double search_init_apps_cloning;
    double search_init_apps_sat;
    double search_init_apps_collect_var_apps;
    double search_init_apps_collect_fa;
    double search_init_apps_collect_fa_cone;
    double lemma_gen;
    double find_nenc_app;
    double find_prop_app;
    double find_cond_prop_app;
  } time;
};

typedef struct BtorCoreSolver BtorCoreSolver;

/*------------------------------------------------------------------------*/

/* Check whether the sorts of given arguments match the signature of the 
 * function. If sorts are correct -1 is returned, otherwise the position of
 * the invalid argument is returned. */
int btor_fun_sort_check (Btor * btor, uint32_t argc, BtorNode ** args,
                         BtorNode * fun);

/* Evaluates expression and returns its value. */
BtorBitVector * btor_eval_exp (Btor * btor, BtorNode * exp);

/* Synthesizes expression of arbitrary length to an AIG vector. Adds string
 * back annotation to the hash table, if the hash table is a non zero ptr.
 * The strings in 'data.asStr' are owned by the caller.  The hash table
 * is a map from AIG variables to strings.
 */
BtorAIGVec *btor_exp_to_aigvec (Btor * btor, BtorNode * exp,
                                BtorPtrHashTable * table);

/* Checks for existing substitutions, finds most simplified expression and 
 * shortens path to it */
BtorNode * btor_simplify_exp (Btor * btor, BtorNode * exp);

/* Finds most simplified expression and shortens path to it */
BtorNode * btor_pointer_chase_simplified_exp (Btor * btor, BtorNode * exp);

/* Frees BV assignment obtained by calling 'btor_assignment_exp' */
void btor_release_bv_assignment_str (Btor * btor, char * assignment);

void btor_release_all_ext_refs (Btor * btor);

void btor_init_substitutions (Btor *);
void btor_delete_substitutions (Btor *);
void btor_insert_substitution (Btor *, BtorNode *, BtorNode *, int);
BtorNode * btor_find_substitution (Btor *, BtorNode *);

void btor_substitute_and_rebuild (Btor *, BtorPtrHashTable *, int);
void btor_insert_varsubst_constraint (Btor *, BtorNode *, BtorNode *);

BtorNode * btor_aux_var_exp (Btor *, int);

#endif
