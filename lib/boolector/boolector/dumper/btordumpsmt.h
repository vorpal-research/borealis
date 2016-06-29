/*  Boolector: Satisfiablity Modulo Theories (SMT) solver.
 *
 *  Copyright (C) 2007-2009 Robert Daniel Brummayer.
 *  Copyright (C) 2007-2013 Armin Biere.
 *  Copyright (C) 2012-2013 Aina Niemetz.
 *  Copyright (C) 2012-2015 Mathias Preiner.
 *
 *  All rights reserved.
 *
 *  This file is part of Boolector.
 *  See COPYING for more information on using this software.
 */

#ifndef BTORDUMPSMT_H_INCLUDED
#define BTORDUMPSMT_H_INCLUDED

#include "btorcore.h"
#include <stdio.h>

void btor_dump_smt2_nodes (Btor * btor, FILE * file, BtorNode ** roots,
                           int nroots); 

void btor_dump_smt2_node (Btor * btor, FILE * file, BtorNode * node,
                          unsigned depth);

void btor_dump_smt2 (Btor * btor, FILE * file);

void btor_dump_const_value_smt (Btor * btor, const char * bits, int base,
                                FILE * file);

void btor_dump_sort_smt_node (BtorNode * exp, FILE * file);
void btor_dump_sort_smt (BtorSort * sort, FILE * file);
#endif
