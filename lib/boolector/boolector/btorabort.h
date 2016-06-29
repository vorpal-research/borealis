/*  Boolector: Satisfiablity Modulo Theories (SMT) solver.
 *
 *  Copyright (C) 2007-2009 Robert Daniel Brummayer.
 *  Copyright (C) 2007-2014 Armin Biere.
 *  Copyright (C) 2013-2014 Aina Niemetz
 *  Copyright (C) 2013-2015 Mathias Preiner.
 *
 *  All rights reserved.
 *
 *  This file is part of Boolector.
 *  See COPYING for more information on using this software.
 */
#ifndef BTORABORT_H_INCLUDED
#define BTORABORT_H_INCLUDED

/*------------------------------------------------------------------------*/

#include "btorexit.h"

#include <stdlib.h>

/*------------------------------------------------------------------------*/

#define BTOR_WARN_DEPRECATED(msg...) \
do { \
    fprintf (stderr, \
      "[%s] function %s is deprected, use %s instead \n", \
      __FILE__, __FUNCTION__, ##msg); \
    fflush (stderr); \
} while (0)

/*------------------------------------------------------------------------*/

#define BTOR_ABORT_BOOLECTOR(cond,fmt,msg...) \
do { \
  if (cond) \
    { \
      fprintf (stderr, \
        "[%s] %s: " fmt "\n", __FILE__, __FUNCTION__, ##msg); \
      fflush (stderr); \
      abort (); \
    } \
} while (0)

#define BTOR_ABORT_ARG_NULL_BOOLECTOR(arg) \
do { \
  if ((arg) == 0) \
    { \
      fprintf (stderr, "[%s] %s: ", __FILE__, __FUNCTION__); \
      fprintf (stderr, "'%s' must not be NULL\n", #arg); \
      fflush (stderr); \
      abort (); \
    } \
} while (0)

#define BTOR_ABORT_REFS_NOT_POS_BOOLECTOR(ARG) \
do { \
  if (BTOR_REAL_ADDR_NODE((ARG))->ext_refs < 1) \
    { \
      fprintf (stderr, "[%s] %s: ", __FILE__, __FUNCTION__); \
      fprintf (stderr, "reference counter of '%s' must not be < 1\n", #ARG); \
      fflush (stderr); \
      abort (); \
    } \
} while (0)

#define BTOR_ABORT_IF_BTOR_DOES_NOT_MATCH(BTOR,ARG) \
do { \
  if (BTOR_REAL_ADDR_NODE(ARG)->btor != (BTOR)) \
    { \
      fprintf (stderr, "[%s] %s: ", __FILE__, __FUNCTION__); \
      fprintf (stderr, \
         "argument '%s' belongs to different Boolector instance\n", #ARG); \
      fflush (stderr); \
      abort (); \
    } \
} while (0)

#define BTOR_ABORT_BV_BOOLECTOR(arg) \
do { \
  if (btor_is_bitvec_sort (&btor->sorts_unique_table, \
                           BTOR_REAL_ADDR_NODE (arg)->sort_id)) \
    { \
      fprintf (stderr, "[%s] %s: ", __FILE__, __FUNCTION__); \
      fprintf (stderr, "'%s' must not be a bit-vector\n", #arg); \
      fflush (stderr); \
      abort (); \
    } \
} while (0)

#define BTOR_ABORT_NOT_BV_BOOLECTOR(arg) \
do { \
  if (!btor_is_bitvec_sort (&btor->sorts_unique_table, \
                            BTOR_REAL_ADDR_NODE (arg)->sort_id)) \
    { \
      fprintf (stderr, "[%s] %s: ", __FILE__, __FUNCTION__); \
      fprintf (stderr, "'%s' must be a bit-vector\n", #arg); \
      fflush (stderr); \
      abort (); \
    } \
} while (0)

#define BTOR_ABORT_NE_BW(arg1,arg2) \
do { \
  if (BTOR_REAL_ADDR_NODE ((arg1))->sort_id != \
      BTOR_REAL_ADDR_NODE ((arg2))->sort_id) \
    { \
      fprintf (stderr, \
        "[%s] %s: ", __FILE__, __FUNCTION__); \
      fprintf (stderr, \
         "bit-width of '%s' and '%s' must not be unequal\n",#arg1,#arg2); \
      fflush (stderr); \
      abort (); \
    } \
} while (0)

/*------------------------------------------------------------------------*/

#define BTOR_COVER(COND) \
do { \
  if (!(COND)) break; \
  fprintf (stderr, \
    "%s:%d: in %s: Coverage target '" # COND "' hit\n", \
    __FILE__, __LINE__, __FUNCTION__); \
  fflush (stderr); \
  abort (); \
} while (0)

#endif
