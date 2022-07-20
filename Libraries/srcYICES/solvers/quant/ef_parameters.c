/*
 * This file is part of the Yices SMT Solver.
 * Copyright (C) 2017 SRI International.
 *
 * Yices is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Yices is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Yices.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Parameters for the EF client.
 */

#include "solvers/quant/ef_parameters.h"
#include "solvers/quant/quant_parameters.h"

void init_ef_params(ef_param_t *p) {
  p->flatten_iff = false;
  p->flatten_ite = false;
  p->gen_mode = EF_GEN_AUTO_OPTION;
  p->max_samples = 5;

  p->max_iters = DEF_MBQI_MAX_ITERS;
  p->max_numlearnt_per_round = DEF_MBQI_MAX_LEMMAS_PER_ROUND;
  p->ematching = DEF_EMATCH_EN;

  p->ematch_inst_per_round = DEFAULT_MAX_INSTANCES_PER_ROUND;
  p->ematch_inst_per_search = DEFAULT_MAX_INSTANCES_PER_SEARCH;
  p->ematch_inst_total = DEFAULT_MAX_INSTANCES;
  p->ematch_rounds_per_search = DEFAULT_MAX_ROUNDS_PER_SEARCH;
  p->ematch_search_total = DEFAULT_MAX_SEARCH;

  p->ematch_exec_max_fdepth = DEF_MAX_FDEPTH;
  p->ematch_exec_max_vdepth = DEF_MAX_VDEPTH;
  p->ematch_exec_max_fapps = DEF_MAX_FAPPS;
  p->ematch_exec_max_matches = DEF_MAX_MATCHES;

  p->ematch_cnstr_epsilon = CNSTR_RL_EPSILON_MIN;
  p->ematch_cnstr_alpha = CNSTR_RL_ALPHA_DEFAULT;
  p->ematch_term_epsilon = TERM_RL_EPSILON_MIN;
  p->ematch_term_alpha = TERM_RL_ALPHA_DEFAULT;

  p->ematch_cnstr_mode = DEFAULT_EMATCH_MODE;
  p->ematch_term_mode = DEFAULT_EMATCH_MODE;
}

