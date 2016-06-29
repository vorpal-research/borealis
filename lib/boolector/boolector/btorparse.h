/*  Boolector: Satisfiablity Modulo Theories (SMT) solver.
 *
 *  Copyright (C) 2007-2009 Robert Daniel Brummayer.
 *  Copyright (C) 2007-2012 Armin Biere.
 *  Copyright (C) 2013-2015 Mathias Preiner.
 *  Copyright (C) 2015 Aina Niemetz.
 *
 *  All rights reserved.
 *
 *  This file is part of Boolector.
 *  See COPYING for more information on using this software.
 */

#ifndef BTORPARSE_H_INCLUDED
#define BTORPARSE_H_INCLUDED

#include "boolector.h"
#include "utils/btorstack.h"
#include "btorlogic.h"
#include "btormsg.h"

#include <stdio.h>

/*------------------------------------------------------------------------*/

typedef struct BtorParseOpt BtorParseOpt;
typedef struct BtorParser BtorParser;
typedef struct BtorParseResult BtorParseResult;
typedef struct BtorParserAPI BtorParserAPI;

typedef BtorParser * (*BtorInitParser)(Btor *, BtorParseOpt *);

typedef void (*BtorResetParser)(void*);

typedef char * (*BtorParse) (BtorParser *, BtorCharStack * prefix,
                             FILE *, const char *, FILE *, BtorParseResult *);


enum BtorParseMode
{
  BTOR_PARSE_MODE_BASIC_INCREMENTAL = 1,
  BTOR_PARSE_MODE_INCREMENTAL_BUT_CONTINUE = 2,
  BTOR_PARSE_MODE_INCREMENTAL_IN_DEPTH = 8,
  BTOR_PARSE_MODE_INCREMENTAL_LOOK_AHEAD = 16,
  BTOR_PARSE_MODE_INCREMENTAL_INTERVAL = 32,
  BTOR_PARSE_MODE_INCREMENTAL_WINDOW = (8 | 16 | 32),
};

typedef enum BtorParseMode BtorParseMode;

struct BtorParseOpt
{
  BtorParseMode incremental;
  int verbosity;
  int need_model;
  int window;
  int interactive;
};

struct BtorParseResult
{
  BtorLogic logic;
  int status;
  int result;

  int ninputs;
  BoolectorNode **inputs;

  int noutputs;
  BoolectorNode **outputs;
};

struct BtorParserAPI
{
  BtorInitParser init;
  BtorResetParser reset;
  BtorParse parse;
};

int btor_parse (Btor * btor, 
                FILE * infile, 
                const char * infile_name, 
                FILE * outfile,
                char ** error_msg,
                int * status);

int btor_parse_btor (Btor * btor, 
                     FILE * infile, 
                     const char * infile_name, 
                     FILE * outfile,
                     char ** error_msg, 
                     int * status);

int btor_parse_smt1 (Btor * btor, 
                     FILE * infile, 
                     const char * infile_name, 
                     FILE * outfile,
                     char ** error_msg,
                     int * status);

int btor_parse_smt2 (Btor * btor, 
                     FILE * infile, 
                     const char * infile_name, 
                     FILE * outfile,
                     char ** error_msg,
                     int * status);

BtorMsg * boolector_get_btor_msg (Btor * btor);
#endif
