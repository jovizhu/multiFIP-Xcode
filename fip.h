

#ifndef FIP_H
#define FIP_H

#include "ff.h"


#define MAX_NUM_D_ACTION 10
#define MAX_PI_PARENTS 20
#define MAX_INVALID_ACTIONS 100000


/*
 *  maximum number of actions in a group
 */
typedef struct _PlanNode {
	int action;
	
	int num_sons;
	struct _PlanNode *sons[MAX_NUM_D_ACTION];
	
	int	id;
	Bool	isReferenced;
} PlanNode;



/*
 *  A group of D actions
 */
typedef struct _ActionGroup {
	
	/* identity of group */
	char *name;
	int instances;
	
	int actions[MAX_NUM_D_ACTION];
	int num_actions;

} ActionGroup, *ActionGroupPtr;


/*
 * for each state appeared in an weak plan
 * record it as well as the desired action
 *
 *	this structure is designed to be the basic
 *	structure in hashtable as well as a list
 */
typedef struct _StateActionPair {
	State	state;			/* key */
	PlanNode	*action;	/* a pointer to a node in plan tree */
	struct	_StateActionPair	*next;
    
	int	id;				/* state id, useful for plan printing */
	State	parent[MAX_PI_PARENTS];	/*Jicheng: Pi Parent. Used for backtracking*/
	int	numP;			/*Number of PI parents so far*/
	int	p_actions[MAX_PI_PARENTS];	/*parent state-action pair*/

	/*Jicheng: add intended state*/
	State	intended_state;
	int	act; /*JC: just for disable*/
} StateActionPair;

/*
 * build fip tree-shape plan from linear ff plan
 */
void convert_ff_plan_to_fip_plan( PlanNode *pPlanNode );

void build_action_group( void );

/* print fip plan in human readable psudo code
	including while and if struture
	*/
void print_fip_plan_1( StateActionPair *pCurrentState,  
							PlanNode	*pPlan,
							int		indent );
/* print fip plan in a big while loop with a single
 * switch inside.
 */					
void print_fip_plan_2(void);

/* print fip plan as a tree
 */
void print_fip_plan_3( PlanNode * pNode, int level );

void print_all_states( void );

StateActionPair* is_solved_state( State *pState );
StateActionPair* is_unsolved_state( State *pState );

void add_solved_state( StateActionPair *pState );

void add_unsolved_state( StateActionPair *pState );

StateActionPair* get_next_unsolved_state( void );

void solve_unsolved_states(void);

void reset_ff_states( void );

Bool is_D_action(const char *name);

/*Jicheng: label unsolved states*/
void label_unsolved_state(State *dest, State *source);
Bool is_strong_less_than(State s1, State s2);
Bool is_weak_less_than(State s1, State s2);
void increment_state_level_id(State *dest, State source);
void backtrack_single_nodes(State* s, int act);

void rm_unsolved_states(State *s);
#endif
