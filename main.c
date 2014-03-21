/*********************************************************************
 * (C) Copyright 2001 Albert Ludwigs University Freiburg
 *     Institute of Computer Science
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 *********************************************************************/

/*
 * THIS SOURCE CODE IS SUPPLIED  ``AS IS'' WITHOUT WARRANTY OF ANY KIND,
 * AND ITS AUTHOR AND THE JOURNAL OF ARTIFICIAL INTELLIGENCE RESEARCH
 * (JAIR) AND JAIR'S PUBLISHERS AND DISTRIBUTORS, DISCLAIM ANY AND ALL
 * WARRANTIES, INCLUDING BUT NOT LIMITED TO ANY IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, AND
 * ANY WARRANTIES OR NON INFRINGEMENT.  THE USER ASSUMES ALL LIABILITY AND
 * RESPONSIBILITY FOR USE OF THIS SOURCE CODE, AND NEITHER THE AUTHOR NOR
 * JAIR, NOR JAIR'S PUBLISHERS AND DISTRIBUTORS, WILL BE LIABLE FOR
 * DAMAGES OF ANY KIND RESULTING FROM ITS USE.  Without limiting the
 * generality of the foregoing, neither the author, nor JAIR, nor JAIR's
 * publishers and distributors, warrant that the Source Code will be
 * error-free, will operate without interruption, or will meet the needs
 * of the user.
 */

/*********************************************************************
 * File: main.c
 * Description: The main routine for the FastForward Planner.
 *
 * Author: Joerg Hoffmann 2000
 *
 * Modified: jovi zhu 2013
 *********************************************************************/

#include "ff.h"

#include "memory.h"
#include "output.h"

#include "parse.h"

#include "inst_pre.h"
#include "inst_easy.h"
#include "inst_hard.h"
#include "inst_final.h"

#include "orderings.h"

#include "relax.h"
#include "search.h"

#include "fip.h"


/*
 *  ----------------------------- GLOBAL VARIABLES ----------------------------
 */

/*******************
 * GENERAL HELPERS *
 *******************/


/* used to time the different stages of the planner
 */
double gtempl_time = 0, greach_time = 0, grelev_time = 0, gconn_time = 0;
double gsearch_time = 0;

double gadd_templ_time = 0, gadd_reach_time = 0, gadd_relev_time = 0, gadd_conn_time = 0;
double gadd_search_time = 0;


/* the command line inputs
 */
struct _command_line gcmd_line;

/* number of states that got heuristically evaluated
 */
int gevaluated_states = 0;

/* maximal depth of breadth first search
 */
int gmax_search_depth = 0;


/***********
 * PARSING *
 ***********/

/* used for pddl parsing, flex only allows global variables
 */
int gbracket_count;
char *gproblem_name;

/* problem name in additional goals
 */
char *gmul_problem_name;

/* The current input line number
 */
int lineno = 1;

/* The current input filename
 */
char *gact_filename;

/* The pddl domain name
 */
char *gdomain_name = NULL;

/* The pddl domain name for additional goals
 */
char *gmul_domain_name = NULL;

/* loaded, uninstantiated operators
 */
PlOperator *gloaded_ops = NULL;

/* loaded, uninstantiated additional operators
 */
PlOperator *gadd_loaded_ops = NULL;

/* stores initials as fact_list
 */
PlNode *gorig_initial_facts = NULL;

/* not yet preprocessed  for additional goal facts
 */
PlNode *gadd_orig_goal_facts = NULL;

/* not yet preprocessed goal facts
 */
PlNode *gorig_goal_facts = NULL;

/* axioms as in UCPOP before being changed to ops
 */
PlOperator *gloaded_axioms = NULL;

/* the types, as defined in the domain file
 */
TypedList *gparse_types = NULL;

/* the constants, as defined in domain file
 */
TypedList *gparse_constants = NULL;

/* the predicates and their arg types, as defined in the domain file
 */
TypedListList *gparse_predicates = NULL;

/* the objects, declared in the problem file
 */
TypedList *gparse_objects = NULL;


/* connection to instantiation ( except ops, goal, initial )
 */

/* all typed objects
 */
FactList *gorig_constant_list = NULL;

/* the predicates and their types
 */
FactList *gpredicates_and_types = NULL;


/*****************
 * INSTANTIATING *
 *****************/

/* global arrays of constant names,
 *               type names (with their constants),
 *               predicate names,
 *               predicate aritys,
 *               defined types of predicate args
 */
Token gconstants[MAX_CONSTANTS];
int gnum_constants = 0;
Token gtype_names[MAX_TYPES];
int gtype_consts[MAX_TYPES][MAX_TYPE];
Bool gis_member[MAX_CONSTANTS][MAX_TYPES];
int gmember_nr[MAX_CONSTANTS][MAX_TYPES];/* nr of object within a type */
int gtype_size[MAX_TYPES];
int gnum_types = 0;
Token gpredicates[MAX_PREDICATES];
int garity[MAX_PREDICATES];
int gpredicates_args_type[MAX_PREDICATES][MAX_ARITY];
int gnum_predicates = 0;

/* jovi: defined for multiple purpose */
Token gadd_predicates[MAX_PREDICATES];

/* the domain in integer (Fact) representation */
Operator_pointer goperators[MAX_OPERATORS];
int gnum_operators = 0;
Fact *gfull_initial;
int gnum_full_initial = 0;
WffNode *ggoal = NULL;

/* jovi: defined for multiple purposes */
Operator_pointer gadd_operators[MAX_OPERATORS];
int gadd_num_operators = 0;
WffNode *gadd_goal = NULL;

/* stores inertia - information: is any occurence of the predicate
 * added / deleted in the uninstantiated ops ? */
Bool gis_added[MAX_PREDICATES];
Bool gis_deleted[MAX_PREDICATES];

/* splitted initial state:
 * initial non static facts,
 * initial static facts, divided into predicates
 * (will be two dimensional array, allocated directly before need)
 */
Facts *ginitial = NULL;
int gnum_initial = 0;
Fact **ginitial_predicate;
int *gnum_initial_predicate;

/* the type numbers corresponding to any unary inertia
 */
int gtype_to_predicate[MAX_PREDICATES];
int gpredicate_to_type[MAX_TYPES];

/* (ordered) numbers of types that new type is intersection of
 */
TypeArray gintersected_types[MAX_TYPES];
int gnum_intersected_types[MAX_TYPES];

/* splitted domain: hard n easy ops */
Operator_pointer *ghard_operators;
int gnum_hard_operators;
NormOperator_pointer *geasy_operators;
int gnum_easy_operators;

/* so called Templates for easy ops: possible inertia constrained
 * instantiation constants */
EasyTemplate *geasy_templates;
int gnum_easy_templates;

/* first step for hard ops: create mixed operators, with conjunctive
 * precondition and arbitrary effects */
MixedOperator *ghard_mixed_operators;
int gnum_hard_mixed_operators;

/* hard ''templates'' : pseudo actions */
PseudoAction_pointer *ghard_templates;
int gnum_hard_templates;

/*************************** mutlipl purpose support ******************************************************************/
/* splitted additional domain: hard n easy ops */
Operator_pointer *gadd_hard_operators;
int gadd_num_hard_operators;

NormOperator_pointer *gadd_easy_operators;
int gadd_num_easy_operators;

/* so called Templates for easy ops: possible inertia constrained
 * instantiation constants */
EasyTemplate *gadd_easy_templates;
int gadd_num_easy_templates;

/* first step for hard ops: create mixed operators, with conjunctive
 * precondition and arbitrary effects */
MixedOperator *gadd_hard_mixed_operators;
int gadd_num_hard_mixed_operators;

/* hard ''templates'' : pseudo actions */
PseudoAction_pointer *gadd_hard_templates;
int gadd_num_hard_templates;
/************************************** multiple purpose (end) **********************************************************/


/* store the final "relevant facts" */
Fact grelevant_facts[MAX_RELEVANT_FACTS];
int gnum_relevant_facts = 0;
int gnum_pp_facts = 0;

/* the final actions and problem representation */
Action *gactions;
int gnum_actions;
State ginitial_state;
State ggoal_state;

/* jovi: add for multiple purpose */
Action *gadd_actions;
int gadd_num_actions;
State gadd_goal_state;

/* define for overl */
Action *gall_actions;
int gall_num_actions;
State gall_goal_state;

/*added by JC*/
StateActionPair *gini_pair = NULL;

/* tree shape plan
 */
PlanNode gfipPlan;
int gnum_fip_plan_node = 0;

StateActionPair *gsolved_states = NULL;
int gnum_solved_states = 0;

StateActionPair *gunsovled_states = NULL;
int gnum_unsolved_states = 0;

Bool g_is_strong = FALSE;

Bool to_print_state = FALSE;

StateActionPair *gTail = NULL;

/*
 *JC: invalid actions
 */
StateActionPair *gInvActs;
int gnum_IV = 0;

/**********************
 * CONNECTIVITY GRAPH *
 **********************/

/* one ops (actions) array ...
 */
OpConn *gop_conn;
int gnum_op_conn;

/* jovi: add for multiple purpose */
OpConn *gadd_op_conn;
int gadd_num_op_conn;

/* one effects array ...
 */
EfConn *gef_conn;
int gnum_ef_conn;

/* jovi: add for multiple purpose */
EfConn *gadd_ef_conn;
int gadd_num_ef_conn;

/* one facts array.
 */
FtConn *gft_conn;
int gnum_ft_conn;

/* jovi: add for multiple purpose */
FtConn *gadd_ft_conn;
int gadd_num_ft_conn;

/*******************
 * SEARCHING NEEDS *
 *******************/

/* the goal state, divided into subsets
 */
State *ggoal_agenda;
int gnum_goal_agenda;


State *gadd_goal_agenda;
int gadd_num_goal_agenda;

/* byproduct of fixpoint: applicable actions
 */
int *gA;
int gnum_A;

/* communication from extract 1.P. to search engines:
 * 1P action choice
 */
int *gH;
int gnum_H;

/* the effects that are considered true in relaxed plan
 */
int *gin_plan_E;
int gnum_in_plan_E;

/* always stores (current) serial plan
 */
int gplan_ops[MAX_PLAN_LENGTH];
int gnum_plan_ops = 0;

/* stores the states that the current plan goes through
 * ( for knowing where new agenda entry starts from )
 */
State gplan_states[MAX_PLAN_LENGTH + 1];

State gadd_plan_states[MAX_PLAN_LENGTH + 1];

/*
 *  ----------------------------- HEADERS FOR PARSING ----------------------------
 * ( fns defined in the scan-* files )
 */
void get_fct_file_name( char *filename );
void load_ops_file( char *filename );
void load_fct_file( char *filename );
void load_mul_file( char *filename );

/*
 *  ----------------------------- MAIN ROUTINE ----------------------------
 */
struct timeb lstart, lend;
struct timeb mystart, myend;

int main( int argc, char *argv[] ) {
    /* resulting name for ops file
     */
    char ops_file[MAX_LENGTH] = "";
    
    /* same for fct file
     */
    char fct_file[MAX_LENGTH] = "";
    
    /* name for additional goal file
     */
    char mul_file[MAX_LENGTH] = "";
    
    struct timeb start, end;
    
    State current_start, current_end;
    int i, j;
    Bool found_plan;
    
    Bool found_plan_for_multiple_purpose;
    
    /*times ( &lstart );*/
    ftime(&lstart);
    
    /* command line treatment*/
    if ( argc == 1 || ( argc == 2 && *++argv[0] == '?' ) ) {
        ff_usage();
        exit( 1 );
    }
    if ( !process_command_line( argc, argv ) ) {
        ff_usage();
        exit( 1 );
    }
    
    /* make file names
     */
    
    /* one input name missing
     */
    if ( !gcmd_line.ops_file_name ||
        !gcmd_line.fct_file_name ||
        !gcmd_line.mul_file_name ) {
        
        fprintf(stdout, "\nff: three input files needed\n\n");
        ff_usage();
        exit( 1 );
    }
    /* add path info, complete file names will be stored in
     * ops_file and fct_file
     */
    sprintf(ops_file, "%s%s", gcmd_line.path, gcmd_line.ops_file_name);
    sprintf(fct_file, "%s%s", gcmd_line.path, gcmd_line.fct_file_name);
    sprintf(mul_file, "%s%s", gcmd_line.path, gcmd_line.mul_file_name);
    
    /* parse the input files
     */
    /* start parse & instantiation timing*/
    /*times( &start );*/
    ftime(&start);
    /* domain file (ops)
     */
    if ( gcmd_line.display_info >= 1 ) {
        printf("\nff: parsing domain file");
    }
    /* it is important for the pddl language to define the domain before
     * reading the problem  */
    load_ops_file( ops_file );
    /* problem file (facts) */
    if ( gcmd_line.display_info >= 1 ) {
        printf(" ... done.\nff: parsing problem file.\n");
    }
    
    load_fct_file( fct_file );
    
    if ( gcmd_line.display_info >= 1 ) {
        printf(" ... done.\n");
    }
    
    load_mul_file( mul_file );
    
    if ( gcmd_line.display_info >= 1 ) {
        printf("... done.\n");
    }
    
    /* This is needed to get all types.*/
    /* modified by jovi: adding supprot for addtional constant */
    build_orig_constant_list();
    
    /* last step of parsing: see if it's an ADL domain!
     */
    if ( !make_adl_domain() ) {
        printf("\nff: this is not an ADL problem!");
        printf("\n    can't be handled by this version.\n\n");
        exit( 1 );
    }
    
    
    /* now instantiate operators;
     */
    /*JC: initialize the array*/
    gInvActs = (StateActionPair*)calloc(MAX_INVALID_ACTIONS, sizeof(StateActionPair));
    
    /**************************
     * first do PREPROCESSING *
     **************************/
    
    /* start by collecting all strings and thereby encoding
     * the domain in integers.
     */
    encode_domain_in_integers();
    
    /* inertia preprocessing, first step:
     *   - collect inertia information
     *   - split initial state into
     *        _ arrays for individual predicates
     *        - arrays for all static relations
     *        - array containing non - static relations
     */
    do_inertia_preprocessing_step_1();
    
    /* normalize all PL1 formulae in domain description:
     * (goal, preconds and effect conditions)
     *   - simplify formula
     *   - expand quantifiers
     *   - NOTs down
     */
    normalize_all_wffs();
    
    /* translate negative preconds: introduce symmetric new predicate
     * NOT-p(..) (e.g., not-in(?ob) in briefcaseworld)
     */
    translate_negative_preconds();
    
    /* split domain in easy (disjunction of conjunctive preconds)
     * and hard (non DNF preconds) part, to apply
     * different instantiation algorithms
     */
    split_domain();
    
    
    /***********************************************
     * PREPROCESSING FINISHED                      *
     *                                             *
     * NOW MULTIPLY PARAMETERS IN EFFECTIVE MANNER *
     ***********************************************/
    
    /*jovi: updated for multiple purpose */
    build_easy_action_templates();
    build_hard_action_templates();
    
    /*times( &end );*/
    ftime(&end);
    TIME( gtempl_time );
    
    /*times( &start );*/
    ftime(&start);
    
    /* perform reachability analysis in terms of relaxed  fixpoint */
    perform_reachability_analysis();
    
    /*times( &end );*/
    ftime(&end);
    TIME( greach_time );
    
    /*times( &start );*/
    ftime(&start);
    
    /* collect the relevant facts and build final domain
     * and problem representations.*/
    collect_relevant_facts();
    
    /*times( &end );*/
    ftime(&end);
    TIME( grelev_time );
    
    /*times( &start );*/
    ftime(&start);
    
    /* now build globally accessable connectivity graph */
    build_connectivity_graph();
    
    /*times( &end );*/
    ftime(&end);
    TIME( gconn_time );
    
    
    /***********************************************************
     * we are finally through with preprocessing and can worry *
     * bout finding a plan instead.                            *
     ***********************************************************/
    ftime(&mystart);
    /*times( &start );*/
    ftime(&start);
    
    /* another quick preprocess: approximate goal orderings and split
     * goal set into sequence of smaller sets, the goal agenda
     */
    compute_goal_agenda();
    
    /*debugit(&ginitial_state);*/
    /* make space in plan states info, and relax
     * make sapce is initialize the space for gplan_states and
     * initialzie the variable
     */
    for ( i = 0; i < MAX_PLAN_LENGTH + 1; i++ ) {
        make_state( &(gplan_states[i]), gnum_ft_conn );
        gplan_states[i].max_F = gnum_ft_conn;
    }
    
    make_state( &current_start, gnum_ft_conn );
    current_start.max_F = gnum_ft_conn;
    make_state( &current_end, gnum_ft_conn );
    current_end.max_F = gnum_ft_conn;
    initialize_relax();
    
    /* need to read the agenda paper */
    source_to_dest( &(gplan_states[0]), &ginitial_state );
    source_to_dest( &current_start, &ginitial_state );
    source_to_dest( &current_end, &(ggoal_agenda[0]) );
    
    for ( i = 0; i < gnum_goal_agenda; i++ ) {
        /* JC add a hashtable creating in do_enforced_hill_climbling*/
        if ( !do_enforced_hill_climbing( &current_start, &current_end ) ) {
            break;
        }
        source_to_dest( &current_start, &(gplan_states[gnum_plan_ops]) );
        if ( i < gnum_goal_agenda - 1 ) {
            for ( j = 0; j < ggoal_agenda[i+1].num_F; j++ ) {
                current_end.F[current_end.num_F++] = ggoal_agenda[i+1].F[j];
            }
        }
    }
    
    found_plan = ( i == gnum_goal_agenda ) ? TRUE : FALSE;
    /////////

    if ( !found_plan ) {
        printf("\n\nEnforced Hill-climbing failed !");
        printf("\nswitching to Best-first Search now.\n");
        reset_ff_states();
        found_plan = do_best_first_search();
    }
    
    if ( found_plan ) {
        /*print_plan();*/
        /* D action add to group */
        build_action_group();
        
        gfipPlan.num_sons = 0;
        gfipPlan.action = -1;
        
        print_plan();
        
        /*put the ultimate goal to the solved set*/
        StateActionPair *g = new_StateActionPair();
        make_state(&g->state, gnum_ft_conn);
        g->state.max_F = gnum_ft_conn;
        source_to_dest(&g->state, &ggoal_state);
        g->state.num_L = 1;
        g->state.L[0] = 10000000; /*make it the biggest*/
        add_solved_state(g);
        /*ugly, but work*/
        
        convert_ff_plan_to_fip_plan( &gfipPlan );
        
        solve_unsolved_states();
        
        if(!gsolved_states){
            printf("No solutions are found! The problem is unsolvable.\n");
            exit(0);
        }else if(g_is_strong){
            StateActionPair *ptr = gsolved_states;
            Bool valid = FALSE;
            while(ptr){
                if(ptr->state.num_L == 1 && ptr->state.L[0] == 1){
                    valid = TRUE;
                    break;
                }
                ptr = ptr->next;
            }
            if(!valid){
                printf("The initial state is a dead-end! The problem is unsolvable.\n");
                exit(0);
            }
        }
        
        printf("##########################################\n");
        printf("#####   PROCEDURE-LIKE CODE   ############\n");
        printf("##########################################\n");
        /* print_fip_plan_1( is_solved_state(&ginitial_state) , &gfipPlan, 1); */
        
        /* times( &end ); */
        ftime(&end);
        TIME( gsearch_time );
        
        /* myend = clock(); */
        ftime(&myend);
        
        /* printf("my cac is %7.3f\n", 1.0*(myend.millitm - mystart.millitm)/1000.0); */
        print_fip_plan_2();
        
        if(to_print_state)
            print_all_states();
        /* print_fip_plan_3( &gfipPlan, 0 ); */
    }
    printf("The total searching time is %7.3f\n", (myend.time - mystart.time) + (myend.millitm - mystart.millitm)/1000.0);
    
    output_planner_info();
    
    
    
    
    /********************************************
     * Multiple Purpose Planning                *
     ********************************************/
    ftime(&start);
    update_reachability_analysis_for_multiple_purpose ();
    ftime(&end);
    TIME( gadd_reach_time );
    
    ftime(&start);
    update_relevant_facts_for_multiple_purpose ();
    ftime(&end);
    TIME( gadd_relev_time );
    
    ftime(&start);
    update_connectivity_graph_for_multiple_purpose();
    ftime(&end);
    TIME( gadd_conn_time );
    
    compute_goal_agenda_for_multiple_purpose ();
    
    for ( i = 0; i < MAX_PLAN_LENGTH + 1; i++ ) {
        make_state( &(gadd_plan_states[i]), gnum_ft_conn );
        gadd_plan_states[i].max_F = gnum_ft_conn;
    }
    
    source_to_dest( &current_end, &(gadd_goal_agenda[0]) );
    
    for ( i = 0; i < gadd_num_goal_agenda; i++ ) {
        /* JC add a hashtable creating in do_enforced_hill_climbling*/
        if ( !do_enforced_hill_climbing_for_multiple_purpose ( &current_start, &current_end ) ) {
            break;
        }
        source_to_dest( &current_start, &(gplan_states[gnum_plan_ops]) );
        if ( i < gnum_goal_agenda - 1 ) {
            for ( j = 0; j < ggoal_agenda[i+1].num_F; j++ ) {
                current_end.F[current_end.num_F++] = ggoal_agenda[i+1].F[j];
            }
        }
    }
    
    found_plan_for_multiple_purpose = ( i == gadd_num_goal_agenda ) ? TRUE : FALSE;
    
    if ( !found_plan_for_multiple_purpose ) {
        printf("\n\nEnforced Hill-climbing failed !");
        printf("\nswitching to Best-first Search now.\n");
        reset_ff_states();
        found_plan = do_best_first_search();
    }
    
    
    /*****************************************************************************************/
    printf("\n\n");
    exit( 0 );
    
}

/*
 *  ----------------------------- HELPING FUNCTIONS ----------------------------
 */
void output_planner_info( void ) {
    
    printf( "\n\ntime spent: %7.3f seconds instantiating %d easy, %d hard action templates",
           gtempl_time, gnum_easy_templates, gnum_hard_mixed_operators );
    printf( "\n            %7.3f seconds reachability analysis, yielding %d facts and %d actions",
           greach_time, gnum_pp_facts, gnum_actions );
    printf( "\n            %7.3f seconds creating final representation with %d relevant facts",
           grelev_time, gnum_relevant_facts );
    printf( "\n            %7.3f seconds building connectivity graph",
           gconn_time );
    printf( "\n            %7.3f seconds searching, evaluating %d states, to a max depth of %d",
           gsearch_time, gevaluated_states, gmax_search_depth );
    printf( "\n            %7.3f seconds total time",
           gtempl_time + greach_time + grelev_time + gconn_time + gsearch_time );
    
    printf("\n\n");
    
    exit( 0 );
    
    print_official_result();
    
}


FILE *out;

void print_official_result( void )

{
    
    int i;
    char name[MAX_LENGTH];
    
    sprintf( name, "%s.soln", gcmd_line.fct_file_name );
    
    if ( (out = fopen( name, "w")) == NULL ) {
        printf("\n\nCan't open official output file!\n\n");
        return;
    }
    
    /*times( &lend );*/
    ftime(&lend);
    fprintf(out, "Time %d\n",
            (int) ((lend.millitm - lstart.millitm) / 1000.0));
    
    for ( i = 0; i < gnum_plan_ops; i++ ) {
        print_official_op_name( gplan_ops[i] );
        fprintf(out, "\n");
    }
    
    fclose( out );
    
}



void print_official_op_name( int index )

{
    
    int i;
    Action *a = gop_conn[index].action;
    
    if ( a->norm_operator ||
        a->pseudo_action ) {
        fprintf(out, "(%s", a->name );
        for ( i = 0; i < a->num_name_vars; i++ ) {
            fprintf(out, " %s", gconstants[a->name_inst_table[i]]);
        }
        fprintf(out, ")");
    }
    
}



void ff_usage( void ) {
    
    printf("\nusage of FIP_ff:\n");
    
    printf("\nOPTIONS   DESCRIPTIONS\n");
    printf("-p <str>    path for operator and fact file\n");
    printf("-o <str>    operator file name\n");
    printf("-f <str>    fact file name\n");
    printf("-a <str>    multiple purpose file name\n");
    printf("-t <str>    type of planning. Either strong or strong cyclic\n");
    printf("-i <num>    run-time information level( preset: 1 )\n");
    printf("      0     only times\n");
    printf("      1     problem name, planning process infos\n");
    printf("    101     parsed problem data\n");
    printf("    102     cleaned up ADL problem\n");
    printf("    103     collected string tables\n");
    printf("    104     encoded domain\n");
    printf("    105     predicates inertia info\n");
    printf("    106     splitted initial state\n");
    printf("    107     domain with Wff s normalized\n");
    printf("    108     domain with NOT conds translated\n");
    printf("    109     splitted domain\n");
    printf("    110     cleaned up easy domain\n");
    printf("    111     unaries encoded easy domain\n");
    printf("    112     effects multiplied easy domain\n");
    printf("    113     inertia removed easy domain\n");
    printf("    114     easy action templates\n");
    printf("    115     cleaned up hard domain representation\n");
    printf("    116     mixed hard domain representation\n");
    printf("    117     final hard domain representation\n");
    printf("    118     reachability analysis results\n");
    printf("    119     facts selected as relevant\n");
    printf("    120     final domain and problem representations\n");
    printf("    121     connectivity graph\n");
    printf("    122     fixpoint result on each evaluated state\n");
    printf("    123     1P extracted on each evaluated state\n");
    printf("    124     H set collected for each evaluated state\n");
    printf("    125     False sets of goals <GAM>\n");
    printf("    126     detected ordering constraints leq_h <GAM>\n");
    printf("    127     the Goal Agenda <GAM>\n");
    
    
    
    /*   printf("    109     reachability analysis results\n"); */
    /*   printf("    110     final domain representation\n"); */
    /*   printf("    111     connectivity graph\n"); */
    /*   printf("    112     False sets of goals <GAM>\n"); */
    /*   printf("    113     detected ordering constraints leq_h <GAM>\n"); */
    /*   printf("    114     the Goal Agenda <GAM>\n"); */
    /*   printf("    115     fixpoint result on each evaluated state <1Ph>\n"); */
    /*   printf("    116     1P extracted on each evaluated state <1Ph>\n"); */
    /*   printf("    117     H set collected for each evaluated state <1Ph>\n"); */
    printf("\n-d <num>    switch on debugging\n\n");
}


/* process the commond line */
Bool process_command_line( int argc, char *argv[] ) {
    
    char option;
    
    gcmd_line.display_info = 1;
    gcmd_line.debug = 0;
    
    memset(gcmd_line.ops_file_name, 0, MAX_LENGTH);
    memset(gcmd_line.fct_file_name, 0, MAX_LENGTH);
    memset(gcmd_line.mul_file_name, 0, MAX_LENGTH);
    memset(gcmd_line.path, 0, MAX_LENGTH);
    
    while ( --argc && ++argv ) {
        if ( *argv[0] != '-' || strlen(*argv) != 2 ) {
            return FALSE;
        }
        option = *++argv[0];
        switch ( option ) {
            default:
                if ( --argc && ++argv ) {
                    switch ( option ) {
                        case 'p':
                            strncpy( gcmd_line.path, *argv, MAX_LENGTH );
                            break;
                        case 'o':
                            strncpy( gcmd_line.ops_file_name, *argv, MAX_LENGTH );
                            break;
                        case 'f':
                            strncpy( gcmd_line.fct_file_name, *argv, MAX_LENGTH );
                            break;
                        case 'm':
                            strncpy( gcmd_line.mul_file_name, *argv, MAX_LENGTH );
                            break;
                        case 'i':
                            sscanf( *argv, "%d", &gcmd_line.display_info );
                            break;
                        case 'd':
                            sscanf( *argv, "%d", &gcmd_line.debug );
                            break;
                        case 's':
                            to_print_state = TRUE;
                            break;
                        case 't':
                            if(strcmp(*argv, "strong")== 0){
                                g_is_strong = TRUE;
                            }else{
                                g_is_strong = FALSE;
                            }
                            break;
                        default:
                            printf( "\nff: unknown option: %c entered\n\n", option );
                            return FALSE;
                    }
                } else {
                    return FALSE;
                }
        }
    }
    return TRUE;
}

