/*  Boolector: Satisfiablity Modulo Theories (SMT) solver.
 *
 *  Copyright (C) 2007-2014 Armin Biere.
 *  Copyright (C) 2015 Mathias Preiner.
 *
 *  All rights reserved.
 *
 *  This file is part of Boolector.
 *  See COPYING for more information on using this software.
 */
#ifndef BTORDUMPAIG_H_INCLUDED
#define BTORDUMPAIG_H_INCLUDED

#include "btortypes.h"
#include "btoraig.h"
#include <stdio.h>
#include <stdbool.h>

/* Dumps AIG in AIGER format to file. */
void btor_dump_aig (BtorAIGMgr * amgr,
                    int binary, FILE * output, BtorAIG * aig);

/* Dumps sequential AIGER model to file. */
void btor_dump_seq_aiger (BtorAIGMgr * amgr,
                          int binary, FILE * output,
                          int naigs, BtorAIG ** aigs,
                          int nregs, BtorAIG ** regs, BtorAIG ** nexts,
                          BtorPtrHashTable * back_annotation);

/* Dumps AIGs in AIGER format to file. */
void btor_dump_aiger (Btor * btor, FILE * output, bool is_binary,
                      bool merge_roots);
#endif
