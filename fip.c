
#include <string.h>

#include "fip.h"
#include "memory.h"
#include "search.h"

#define RUNTIME_ERROR(msg) fprintf(stderr, "%s:%d %s\n", __FILE__, __LINE__, msg)
#define ERROR_AND_EXIT(msg)	RUNTIME_ERROR(msg);exit(1)

#define MAX_ACTIONGROUP		8000

#define LABEL_STR "label"

/*#define GetOP(index) gop_conn[(index)].op
#define GetOPName(index) goperators[GetOP(index)]->name
#define GetNumVar(index) goperators[GetOP(index)]->num_vars
#define GetOPParamInst(index, i) gop_conn[index].inst_table[i]*/
#define GetOPName(index) gop_conn[index].action->name
#define GetNumVar(index) gop_conn[index].action->num_name_vars
#define GetOPParamInst(index, i) gop_conn[index].action->inst_table[i]


/**/


/* tree shape plan
*/
extern PlanNode gfipPlan;
extern int	gnum_fip_plan_node;

ActionGroup lActionGroup[MAX_ACTIONGROUP];
int			lnum_action_group = 0;

extern StateActionPair *gsolved_states;
extern int gnum_solved_states;
extern StateActionPair *gunsovled_states;
extern StateActionPair *gunsovled_tail;
extern int gnum_unsolved_states;
extern Bool g_is_strong;
extern StateActionPair* gInvActs;
extern int gnum_IV;
extern StateActionPair *gini_pair;
extern StateActionPair *gTail;
/********************************
* private functions
*/
/* D action: decomposed action
 * if a action has multiple effects
 * it will be decompsed into different actions
 * and marked as D actions
 */
Bool is_D_action(const char *name){

	return (4 < strlen(name))	&&
		(name[0] == 'D')	&&
		(name[1] == '_')	&&
		(name[3] == '_');
}

int getFactAddress(int *p, int n){

	int r = 0, b = 1, i;

	if( 0 == n ){
		return 0;
	}

	for ( i = n - 1; i > -1; i-- ) {
		r += b * p[i];
		b *= gnum_constants;
	}

	return r;
}


/*
* given a D_action, add it to action group
*/
void add_to_group(int index){

	const char * action_name = GetOPName(index)+4;
	int i;
	int fact_address = getFactAddress(
		&GetOPParamInst(index, 0), 
		GetNumVar(index));

	for(i = 0; i < lnum_action_group; i ++){
		if( strcmp(action_name, lActionGroup[i].name ) == SAME &&
			lActionGroup[i].instances == fact_address	){
				/* add to existing group */
				lActionGroup[i].actions[
					lActionGroup[i].num_actions++
				] = index;

				gop_conn[index].group = i;
				return;
		}
	}

	/* have to create a new group */
	if(MAX_ACTIONGROUP == lnum_action_group){
		fprintf(stderr, "%s %d: increase lnum_action_group. current:%d\n",
			__FILE__, __LINE__, MAX_ACTIONGROUP);
		exit(1);
	}

	/* i == lnum_action_group */
	lActionGroup[i].name = action_name;
	lActionGroup[i].instances = fact_address;
	lActionGroup[i].num_actions = 1;
	lActionGroup[i].actions[0] = index;
	gop_conn[index].group = i;
	lnum_action_group++;
}

/*
* add one single action to plan node
*/
void add_single_action(int index, PlanNode *pNode){
	int i;	
	/* check duplicate */
	for(i = 0; i < pNode->num_sons; i++){
		if(index == pNode->sons[i]->action){
			return;
		}
	}

	if( MAX_NUM_D_ACTION == pNode->num_sons ){
		fprintf(stderr, "%s line %d: increase MAX_NUM_D_ACTION(current:%d)\n",
			__FILE__, __LINE__, MAX_NUM_D_ACTION);
		exit(1);
	}	

	pNode->sons[ pNode->num_sons ] = new_PlanNode();
	(pNode->sons[ pNode->num_sons ])->action	=	index;
	(pNode->sons[ pNode->num_sons ])->id	=	++gnum_fip_plan_node;
	pNode->num_sons ++;
}


/* given a D_action, add it and it's brothers to plan node */
void add_D_action(int index, PlanNode *pNode){	
	ActionGroupPtr pGroup = &lActionGroup[ gop_conn[index].group ];

	int i;

	for(i = 0; i < pGroup->num_actions; i++){				
		add_single_action(pGroup->actions[i], pNode);
	}
}

/*Jicheng: label unsolved states*/
void label_unsolved_state(State *dest, State *source){
	dest->num_L = source->num_L + 2; /*dest state will be the branch*/
	dest->L[dest->num_L - 2] = ++source->num_B;
	dest->L[dest->num_L - 1] = 1; /*starting from 1*/
	int i;
	for(i = 0; i < source->num_L; i++){
		dest->L[i] = source->L[i];
	}
}

void copy_state_level(State *dest, State source){
	int i;
	/*Jicheng: deal with the state levels*/
	for(i = 0; i < source.num_L; i++){
		dest->L[i] = source.L[i];
	}
	dest->num_L = source.num_L;
	dest->num_B = source.num_B;
}

void increment_state_level_id(State *dest, State source){
	int i;
	dest->num_L = source.num_L;
	for(i = 0; i < source.num_L; i++){
		dest->L[i] = source.L[i];
	}
	dest->L[source.num_L - 1]++;	
}

Bool is_strong_less_than(State s1, State s2){
	int i, l = s1.num_L;
	if(s1.num_L > s2.num_L) /*len(s1) <= len(s2)?*/
		return FALSE;

	/* else if s1.num_L >= s2.num_L */
	for(i = 0; i < l - 1; i++){
		if(s1.L[i] != s2.L[i]){
			return FALSE;
		}
	}

	if(s1.L[l -1] <= s2.L[l - 1]){
		return TRUE;
	}else{
		return FALSE;
	}
}

Bool is_weak_less_than(State s1, State s2){
	int i, l;

	l = (s1.num_L < s2.num_L)?s1.num_L:s2.num_L;

	/*because we have checked strong less than, we have excluded the possibility of strong less than*/
	for(i = 0; i < l; i++){
		if(s1.L[i] < s2.L[i])
			return TRUE;
		else if(s1.L[i] > s2.L[i])
			return FALSE;
	}

	if(s1.num_L > s2.num_L)
		return FALSE;
	else
		return TRUE;
}
/*Jicheng: end of new functions*/

void print_state_level(State s){
	int i;
	for(i = 0; i < s.num_L; i++){
		printf("state level: %d.", s.L[i]);
	}
	printf("\n");
}


void debugit(State *s){
	printf("\nprinting level\n");
	print_state_level(*s);
	printf("\nprinting branch level%d\n", s->num_B);

	printf("\nprinting state numf = %d\n", s->num_F);
	print_state(*s);
}


void addToLeaf(State*s, int act){
	StateActionPair* prev = NULL, *ptr = gsolved_states;

	while(ptr){
		if(same_state(s, &ptr->state) && ptr->action && ptr->action->action == act){
			StateActionPair* tmp = ptr;
			tmp->state.num_B = 0;

			free(tmp->intended_state.F);	/*no intended effect any more!*/
			tmp->intended_state.num_F = 0;	/*no intended effect any more!*/

			if(ptr == gsolved_states){
				gsolved_states = gsolved_states->next;

				tmp->next = NULL;				
				add_unsolved_state(tmp);

				if(!gsolved_states){
					gTail = NULL;
					return;
				}

				ptr = gsolved_states; /*prev should be NULL still*/
			}else{
				prev->next = ptr->next;
				ptr = ptr->next;

				tmp->next = NULL;
				add_unsolved_state(tmp);

				if(!ptr)
					gTail = prev;
			}
		}else{
			prev = ptr;
			ptr = ptr->next;
		}
	}
}


void disable_actions(State*s, int index){
	char *name = gop_conn[index].action->name;
	if( is_D_action(name)){
		ActionGroupPtr pGroup = &lActionGroup[ gop_conn[index].group ];

		int i;
		for(i = 0; i < pGroup->num_actions; i++){					
			if(gnum_IV >= MAX_INVALID_ACTIONS){
				printf("\nmul-fip: Please increase the amount of MAX_INVALID_ACTIONS\n");
				exit(0);
			}

			printf("\nmul-fip:debug: action disabled\n\tgnum_IV is %d\n",gnum_IV);
			print_op_name(pGroup->actions[i]);
						

			gInvActs[gnum_IV++].act = pGroup->actions[i];			


			if(gInvActs[gnum_IV - 1].state.num_F == 0){
				make_state(&gInvActs[gnum_IV - 1].state, gnum_ft_conn);
				gInvActs[gnum_IV - 1].state.max_F = gnum_ft_conn;
			}
			
			source_to_dest(&gInvActs[gnum_IV - 1].state, s);

			addToLeaf(s, pGroup->actions[i]);
		}
	}else{
		printf("\nmul-fip: debug action disabled\n");
		print_op_name(index);

		gInvActs[gnum_IV++].act = index;

		if(gInvActs[gnum_IV - 1].state.num_F == 0){
				make_state(&gInvActs[gnum_IV - 1].state, gnum_ft_conn);
				gInvActs[gnum_IV - 1].state.max_F = gnum_ft_conn;
		}
		source_to_dest(&gInvActs[gnum_IV - 1].state, s);

		addToLeaf(s, index);
	}		
}

Bool check_parents(State *cS, StateActionPair *ptr){
	Bool res = TRUE;
	if(ptr->numP == 0) return res; /*good to be removed*/

	int i;
	State minState = ptr->parent[0];
	for(i = 0; i < ptr->numP; i++){
		if(!is_strong_less_than(*cS, ptr->parent[i])){
			res = FALSE;
			if(is_weak_less_than(ptr->parent[i], minState)){
				minState = ptr->parent[i];
			}
		}
	}

	if(!res){
		increment_state_level_id(&ptr->state, minState);
	}
	return res;
}

/* JC: The current state pointed by p is a deadend. Need to backtrack*/
void backtrack_solved_nodes(StateActionPair* p){
	if(!p->numP){				
		return; /*no PI parent, return.*/
	}


	int i;
	StateActionPair* prev = NULL, *ptr = gsolved_states;

	while(ptr){		
		StateActionPair* tmp;
		if(ptr == gsolved_states){			
			for(i = 0; i < p->numP; i++){		
				if(same_state(&p->parent[i], &ptr->state)){			
					gsolved_states = gsolved_states->next;	/*remove from solved list*/

					tmp = ptr;
					tmp->next = NULL;

					free(tmp->intended_state.F);
					tmp->intended_state.num_F = 0; /*no intended effect any more!*/


					add_unsolved_state(tmp);				/*put to the unsolved list*/			
					disable_actions(&tmp->state, tmp->action->action);	

					if(!gsolved_states){ 
						gTail = NULL;
						return;
					}

					ptr = gsolved_states;

				}else if(g_is_strong && is_strong_less_than(p->state, ptr->state)){/*remove PI-descendants*/					
					gsolved_states = gsolved_states->next;	/*remove from solved list*/
					tmp = ptr;
					tmp->next = NULL;

					if(check_parents(&p->state, tmp)){
						rm_unsolved_states(&tmp->state);
						free_StateActionPair(tmp);		
					}else{
						free(tmp->intended_state.F);
						tmp->intended_state.num_F = 0; /*no intended effect any more!*/
						add_unsolved_state(tmp);
					}

					if(!gsolved_states){ 
						gTail = NULL;
						return;
					}
					ptr = gsolved_states;
				}else{
					prev = ptr;
					ptr = ptr->next;
				}				
			}
		}else{			
			for(i = 0; i < p->numP; i++){			
				if(same_state(&p->parent[i], &ptr->state)){					
					tmp = ptr;
					prev ->next = ptr->next;		/*remove from solved list*/
					ptr = ptr->next;

					tmp->next = NULL;	


					free(tmp->intended_state.F);
					tmp->intended_state.num_F = 0; /*no intended effect any more!*/


					add_unsolved_state(tmp);			/*put back to unsolved list*/							

					if(tmp->action)
						disable_actions(&tmp->state, tmp->action->action);						

					if(!ptr) gTail = prev;

				}else if(g_is_strong && is_strong_less_than(p->state, ptr->state)){/*remove PI-descendants*/					
					tmp = ptr;
					prev->next = ptr->next;
					ptr = ptr->next;	

					tmp->next = NULL;

					if(check_parents(&p->state, tmp)){
						rm_unsolved_states(&tmp->state);
						free_StateActionPair(tmp);		
					}else{
						free(tmp->intended_state.F);
						tmp->intended_state.num_F = 0; /*no intended effect any more!*/
						add_unsolved_state(tmp);
					}					

					if(!ptr) gTail = prev;
				}else{					
					prev = ptr;
					ptr = ptr->next;
				}				
			}
		}
	}
}

/*Remove all the unsolved states that are descendent of s*/
void rm_unsolved_states_ex_self(State *s){
	StateActionPair *prev = NULL, *ptr = gunsovled_states;

	while(ptr){		
		if(!same_state(s, &ptr->state) && is_strong_less_than(*s, ptr->state)){
			StateActionPair *tmpPtr = ptr;
			if(ptr == gunsovled_states){				
				gunsovled_states = gunsovled_states->next;	
				free_StateActionPair(tmpPtr);
				ptr = gunsovled_states;				
			}else{
				prev->next = ptr->next;
				ptr = ptr->next;
				free_StateActionPair(tmpPtr);				
			}
		}else{
			prev = ptr;
			ptr = ptr->next;
		}
	}
}

/*Remove all the unsolved states that are descendent of s including s*/
void rm_unsolved_states(State *s){
	StateActionPair *prev = NULL, *ptr = gunsovled_states;

	while(ptr){		
		if(is_strong_less_than(*s, ptr->state)){
			StateActionPair *tmpPtr = ptr;
			if(ptr == gunsovled_states){				
				gunsovled_states = gunsovled_states->next;	
				free_StateActionPair(tmpPtr);
				ptr = gunsovled_states;				
			}else{
				prev->next = ptr->next;
				ptr = ptr->next;
				free_StateActionPair(tmpPtr);				
			}
		}else{
			prev = ptr;
			ptr = ptr->next;
		}
	}
}

void try_other_path(StateActionPair *s, int a){	
	disable_actions(&s->state, a);
	s->next = gunsovled_states;
	gunsovled_states = s;
	gnum_unsolved_states++;
}

void backtrack_single_nodes(State* s, int act){	
	StateActionPair* prev = NULL;
	StateActionPair* ptr = gsolved_states;

	while(ptr){				
		if(same_state(&ptr->state, s) || is_strong_less_than(*s, ptr->state)){	
			StateActionPair* tmp = ptr;
			tmp->state.num_B = 0;

			if(ptr == gsolved_states){
				gsolved_states = gsolved_states->next;				

				tmp->next = NULL;	
				if(same_state(&tmp->state, s)){
					if(tmp->numP > 0) tmp->numP--;	

						free(tmp->intended_state.F);
						tmp->intended_state.num_F = 0; /*no intended effect any more!*/

					add_unsolved_state(tmp);			/*put back to unsolved list*/			
				}else{				
					rm_unsolved_states(&tmp->state);
					free_StateActionPair(tmp);					
				}

				if(!gsolved_states){
					gTail = NULL;
					return;
				}

				ptr = gsolved_states;
			}else{
				prev->next = ptr->next;
				ptr = ptr->next;

				tmp->next = NULL;
				if(same_state(&tmp->state, s)){
					if(tmp->numP > 0) tmp->numP--;	

						free(tmp->intended_state.F);
						tmp->intended_state.num_F = 0;

					add_unsolved_state(tmp);			/*put back to unsolved list*/			
				}else{				
					rm_unsolved_states(&tmp->state);
					free_StateActionPair(tmp);					
				}

				if(!ptr)
					gTail = prev;
			}
		}else{
			prev = ptr;
			ptr = ptr->next;
		}
	}	

	rm_unsolved_states_ex_self(s);		
	disable_actions(s, act);
}


void increaseIndent( int indent ){

	int i;

	for(i = 0; i < indent; i++){
		printf("   ");
	}
}


StateActionPair* transferToNextState( StateActionPair *pCurrent, int op ){

	State next;
	StateActionPair *p;


	if( !pCurrent ){
		ERROR_AND_EXIT("pCurrent == NULL");
	}

	if( !pCurrent->action ){
		ERROR_AND_EXIT("pCurrent->action == NULL");
	}

	make_state(&next, gnum_ft_conn);
	next.max_F = gnum_ft_conn;

	result_to_dest( &next, &pCurrent->state, 
		op != -1 ? op : pCurrent->action->action );

	p = is_solved_state( &next );

	if( !p ){
		printf("\nmul-fip: State S%d has not been solved yet.\n", pCurrent->id );
		/*printf("is it a goal state?\n");*/
		printf("\tThere is no strong solution available\n");
		exit( 1 );
	}

	return p;
}

void print_desired_state(  StateActionPair *pCurrent, int indent){
	increaseIndent(indent);

	if(!pCurrent){
		printf("return; // check if this is goal state?\n");
		return;
	}

	printf( "if( state == S%d ){\n", pCurrent->id );

	increaseIndent(indent+1);
	printf( "break;\n" );

	increaseIndent(indent);
	printf( "}\n\n" );

}

void print_undesired_state(  StateActionPair *pCurrent, PlanNode *pPlanNode, int indent) {
	increaseIndent(indent);
	printf( "if( state == S%d ){\n", pCurrent->id );

	print_fip_plan_1(pCurrent, pPlanNode, indent+1);

	increaseIndent(indent);
	printf( "}\n\n" );
}

void print_end_state( StateActionPair *pCurrentState, int indent) {
	increaseIndent(indent);


	if( !pCurrentState ){
		printf("return;\n");
	} else if( !pCurrentState->action ){
		printf("return?\n");
	} else {
		printf("goto %s%d;\n", LABEL_STR, pCurrentState->action->id);
	}
}



/********************************
* public functions
*/

/*
* -1: is not the solved
* 0: solved, but don't need to return
* 1: to be returned
*/
short meet_solved_state(StateActionPair *pCurrentState, StateActionPair *pNextState, int act){
	/* check if the next state is solved already */
	StateActionPair *q;

	if( q = is_solved_state(&pNextState->state)){

		/*JC: just for strong planning*/
		if(g_is_strong){					
			if(same_state(&q->state, &pCurrentState->state)){
				printf("Same state. Not safe to jump... ...\n");		
				try_other_path(pCurrentState, act);
				return 1;
			}
			else if( is_strong_less_than(q->state, pCurrentState->state)){					
				printf("NOT safe to jump..\n");
				try_other_path(pCurrentState, act);
				return 1;
			}else if(is_weak_less_than(q->state, pCurrentState->state)){
				printf("Perform BFS to check whether it is a safe jump\n");
				try_other_path(pCurrentState, act);
				return 1;
				/*breath-first-search*/
				/*No real cases have been found that commit this situation!*/
			}else{
				printf("It is safe to jump\n");														
				return 0;
			}		
		}
		return 0;

		/*break;*/
	}else if(g_is_strong && same_state(&pNextState->state, &pCurrentState->state) ){
		printf("Same state. Not safe to jump.. num of parents %d\n", pCurrentState->numP);						
		try_other_path(pCurrentState, act);				
		return 1;
	}
	return -1;
}

/*
* build fip tree-shape plan from linear ff plan
* also, build a solved state hastable and a to-be-solved state list
*/
void convert_ff_plan_to_fip_plan( PlanNode *pPlanNode ){
	int i,j;
	PlanNode *current = pPlanNode;
	PlanNode *tmp;
	StateActionPair *pCurrentState, *pNextState, *pPrevState = NULL;

	pCurrentState =	new_StateActionPair();

	source_to_dest( &pCurrentState->state, &ginitial_state );

	if(gini_pair != NULL && gini_pair->numP > 0){
		for(i = 0; i < gini_pair->numP; i++){
			if(pCurrentState->parent[i].num_F == 0){
				make_state(&pCurrentState->parent[i], gnum_ft_conn);
				pCurrentState->parent[i].max_F = gnum_ft_conn;
			}

			source_to_dest(&pCurrentState->parent[i], &gini_pair->parent[i]);
			if(g_is_strong){
				copy_state_level(&pCurrentState->parent[i], gini_pair->parent[i]);
			}
		}
		pCurrentState->numP = gini_pair->numP;
	}

	pPrevState = pCurrentState;

	if(g_is_strong){
		copy_state_level(&pCurrentState->state, ginitial_state );/*JC: copy the state level*/		
		copy_state_level(&pPrevState->state, ginitial_state );/*JC: copy the state level*/	
	}

	/*JC: remember first time to avoid same parent*/
	Bool first_time = TRUE;
	/* for each ff operator */
	for ( i = 0; i < gnum_plan_ops; i++ ) {		
		/* add it to fip plan */			
		if( is_D_action( GetOPName( gplan_ops[i] ) ) ){			
			add_D_action(gplan_ops[i], current);			
			/* change desired action to first */
			for(j = 0; j < current->num_sons; j++){
				if(current->sons[j]->action == gplan_ops[i]){
					/*exchange the action at j and 0*/
					tmp = current->sons[0];
					current->sons[0] = current->sons[j];
					current->sons[j] = tmp;
					break;
				}
			}			
		} else { /*is not D action*/ 
			add_single_action(gplan_ops[i], current);			
		}
		

		/*Deal with pi-parent*/
		if(!first_time){	
			if(pCurrentState->parent[pCurrentState->numP].num_F == 0){
				make_state(&pCurrentState->parent[pCurrentState->numP], gnum_ft_conn); 
				pCurrentState->parent[pCurrentState->numP].max_F = gnum_ft_conn;
			}

			source_to_dest(&pCurrentState->parent[pCurrentState->numP++],
				&pPrevState->state);

			if(pPrevState->action)
				pCurrentState->p_actions[pCurrentState->numP - 1] = pPrevState->action->action;

			if(g_is_strong){
				copy_state_level(&pCurrentState->parent[pCurrentState->numP - 1],
					pPrevState->state);
			}
		}else{
			first_time = FALSE;
		}
		
		/*if current state has only one child-- for deterministic action*/
		if(g_is_strong && current->num_sons == 1){
			pNextState = new_StateActionPair();			

			result_to_dest( 
				& pNextState->state, 
				& pCurrentState->state, 
				current->sons[0]->action );

			if(meet_solved_state(pCurrentState, pNextState, current->sons[0]->action) == 1){
				free_StateActionPair(pNextState);
				return;
			}
		}

		/* if current state has more than one child, 
		add them to un-solved states */
		for(j = 1; j < current->num_sons; j++){			
			pNextState = new_StateActionPair();			

			result_to_dest( 
				& pNextState->state, 
				& pCurrentState->state, 
				current->sons[j]->action );
			pNextState->action = current->sons[j];
			
			short res = meet_solved_state(pCurrentState, pNextState, current->sons[0]->action);
			if(res == 1){
				free_StateActionPair(pNextState);
				return;
			}else if(res == 0){
				current->sons[j]->isReferenced = TRUE;		
				free_StateActionPair(pNextState);
				continue;

			}												

			if(!is_unsolved_state(&pNextState->state)){/*add only unique unsolved states*/			
				if(pNextState->parent[pNextState->numP].num_F == 0){
					make_state(&pNextState->parent[pNextState->numP], gnum_ft_conn);
					pNextState->parent[pNextState->numP].max_F = gnum_ft_conn;
				}

				source_to_dest(&pNextState->parent[pNextState->numP], &pCurrentState->state);  /*Jicheng: remember where this state comes from*/

				pNextState->p_actions[pNextState->numP++] = current->sons[0]->action; /*JC: remember parent state-action*/

				if(g_is_strong){
					label_unsolved_state(&pNextState->state, &pCurrentState->state);/*Jicheng: label non-goal leaf states*/
					copy_state_level(&pNextState->parent[pNextState->numP - 1], pCurrentState->state);
				}

				/*Jicheng: memorize the intended effect*/
				if(pNextState->intended_state.num_F == 0){
					make_state(&pNextState->intended_state, gnum_ft_conn);
					pNextState->intended_state.max_F = gnum_ft_conn;
				}

				result_to_dest(&pNextState->intended_state,
					&pCurrentState->state,
					current->sons[0]->action);

				if(g_is_strong){
					
					StateActionPair *sap = is_solved_state(&pNextState->intended_state);
					if(sap){
						copy_state_level(&pNextState->intended_state, sap->state);
					}else{
						increment_state_level_id(&pNextState->intended_state, pCurrentState->state);	
					}
				}

				add_unsolved_state( pNextState );	
			}
		}

		/* add current state as solved state */
		pCurrentState->action =	current->sons[0];		
		add_solved_state( pCurrentState );

		/* move current action and current state downward */
		current = current->sons[0];

		pNextState = new_StateActionPair();	

		result_to_dest( 
			& pNextState->state, 
			&pCurrentState->state, 
			current->action );

		/*Jicheng: increment the id of the next action in the same level*/
		if(g_is_strong){
			increment_state_level_id(&pNextState->state, pCurrentState->state);			
		}

		/*Jicheng: remember previous state*/
		pPrevState = pCurrentState;
		pCurrentState = pNextState;	

	}/* for each action in weak plan */


	if( i == gnum_plan_ops && !gini_pair){

		/*pCurrentState = new_StateActionPair();
		source_to_dest( &pCurrentState->state, ggoal_state );*/

		if( NULL == is_solved_state(& pCurrentState->state) ){
			/*Deal with pi-parent*/
			if(!first_time){
				if(pCurrentState->parent[pCurrentState->numP].num_F == 0){
					make_state(&pCurrentState->parent[pCurrentState->numP], gnum_ft_conn);
					pCurrentState->parent[pCurrentState->numP].max_F = gnum_ft_conn;
				}

				source_to_dest(&pCurrentState->parent[pCurrentState->numP++],
					&pPrevState->state);
				if(pPrevState->action)
					pCurrentState->p_actions[pCurrentState->numP - 1] = pPrevState->action->action;

				if(g_is_strong){
					copy_state_level(&pCurrentState->parent[pCurrentState->numP - 1],
						pPrevState->state);
				}
			}else{
				first_time = FALSE;
			}					
			
			add_solved_state( pCurrentState );
		}
	}

}

/*
* on the one hand: build action group
* on the other hand: for each action in gop_conn,
* build action group pointer
*/
void build_action_group( void ){
	int i;

	lnum_action_group = 0;

	/* transverse all actions. find out brother D_action */
	for(i = 0; i < gnum_op_conn; i ++){

		if( is_D_action( GetOPName(i) ) ){
			add_to_group(i);
		}
	}
}

StateActionPair* is_unsolved_state( State *pState ){
	StateActionPair *p = gunsovled_states;

	while( p ){
		if( same_state(&p->state, pState) ){
			break;
		}
		p = p->next;
	}

	return p;
}

StateActionPair* is_solved_state( State *pState ){
	StateActionPair *p = gsolved_states;

	while( p ){

		/*JC: big change -- need cautious!!!*/		
		if(g_is_strong){
			if( is_in_goal(pState, &p->state) || is_in_goal(&p->state, pState)){		
				break;
			}
		}else{
			if( same_state(pState, &p->state) ){
				break;
			}
		}
		
		p = p->next;
	}

	return p;
}

void debugAllUnSolvedState(){
	printf("Printing all UNsolved states...\n");
	StateActionPair *p = gunsovled_states;

	while(p){		
		debugit(&p->state);
		p = p->next;
	}
	printf("\n\nExiting debugAllUNSolvedState...\n");
}

void debugAllSolvedState(){
	printf("Printing all solved states...\n");
	StateActionPair *p = gsolved_states;

	while(p){		
		debugit(&p->state);
		p = p->next;
	}
	printf("\n\nExiting debugAllSolvedState...\n");
}

/* insert pState into gTail */
void add_solved_state( StateActionPair *pState ){	

	/*debug: duplicate check*/
	if(NULL != is_solved_state( &pState->state )){
		printf("state has been solved!\n");
		/*exit(1);*/
		return;
	}

	pState->id = gnum_solved_states++;

	if( gTail ){
		gTail->next = pState;
		gTail = pState;
	} else {
		gsolved_states = gTail = pState;
	}
	pState->next = NULL;
}

void update_intended_effect(StateActionPair *ptr, StateActionPair *pState){

	if(ptr->intended_state.num_F > 0){
		if(same_state(&ptr->intended_state, &pState->state)){
			free(ptr->intended_state.F);
			ptr->intended_state.num_F = 0;
		}
	}
}

void add_unsolved_state( StateActionPair *pState ){

	/*pState->next = gunsovled_states;
	gunsovled_states = pState;*/

	pState->next = NULL;
	if(gunsovled_states == NULL){
		gunsovled_states = pState;	
		pState->next = NULL;
	}else{
		StateActionPair *ptr = gunsovled_states;

		if(g_is_strong){
			if(same_state(&ptr->state, &pState->state)) return;

			update_intended_effect(ptr, pState);

			/*Check whether to insert to the first place*/
			if(is_weak_less_than(pState->state, ptr->state)){
				pState->next = gunsovled_states;
				gunsovled_states = pState;
				gnum_unsolved_states++;
				return;
			}			

			while(ptr->next != NULL){
				if(same_state(&ptr->next->state, &pState->state)) return;			
				if(is_weak_less_than(pState->state, ptr->next->state)){					
					StateActionPair *tmpPtr = ptr->next;
					ptr->next = pState;
					pState->next = tmpPtr;
					gnum_unsolved_states++;
					tmpPtr = NULL;					
					return;
				}
				
				update_intended_effect(ptr->next, pState);

				ptr = ptr->next;
			}		

		}else{
			while(ptr != NULL){
				update_intended_effect(ptr, pState);
				if(same_state(&ptr->state, &pState->state)){
					return;
				}			

				if(ptr->next == NULL)
					break;

				ptr = ptr->next;
			}			
		}
		ptr->next = pState;
	}		
	gnum_unsolved_states++;

	/*
	printf("adding unsolved state::\n");
	print_state(gunsovled_states->state);
	printf("\n");
	*/
}


StateActionPair* get_next_unsolved_state(){
	StateActionPair *p = gunsovled_states;

	if(p){
		gunsovled_states = gunsovled_states->next;
		gnum_unsolved_states--;
	}

	p->next = NULL;
	return p;
}

void solve_unsolved_states(void){
	StateActionPair *p, *q;
	Bool found_plan;
	State originState, originGoal;

	make_state(&originState, gnum_ft_conn);
	originState.max_F = gnum_ft_conn;
	make_state(&originGoal, gnum_ft_conn);
	originGoal.max_F = gnum_ft_conn;

	source_to_dest(&originState, &ginitial_state);
	source_to_dest(&originGoal, &ggoal_state);

	while( gunsovled_states ){
		
		/*printf("\nA new round starts\n");
		debugAllUnSolvedState();
		debugAllSolvedState();*/

		p = get_next_unsolved_state();		

		if( q = is_solved_state( & p->state ) ){
			/* state has been solved.
			* we need to record it in fipPlan tree
			*/	
			if(g_is_strong){						
				if(same_state(&q->state, &p->parent[p->numP - 1])){
					printf("solve_unsolved_states -- Same state. Not safe to jump..\n");
					/*debugit(&p->parent[p->numP - 1]);*/
					source_to_dest( &ginitial_state, &originState);
					source_to_dest(&ggoal_state, &originGoal);					
					backtrack_single_nodes(&p->parent[p->numP - 1], p->p_actions[p->numP - 1]);
					continue;
				}
				else if( is_strong_less_than(q->state, p->parent[p->numP - 1])){					
					printf("solve_unsolved_states -- NOT safe to jump..\n");
					/*debugit(&p->parent[p->numP - 1]);*/
					source_to_dest(&ginitial_state, &originState);
					source_to_dest(&ggoal_state, &originGoal);	
					backtrack_single_nodes(&p->parent[p->numP - 1], p->p_actions[p->numP - 1]);	

					continue;
				}else if(is_weak_less_than(q->state, p->parent[p->numP - 1])){
					printf("solve_unsolved_states -- Perform BFS to check whether it is a safe jump\n");
					source_to_dest(&ginitial_state, &originState);
					source_to_dest(&ggoal_state, &originGoal);	
					backtrack_single_nodes(&p->parent[p->numP - 1], p->p_actions[p->numP - 1]);	

					continue;
					/*breath-first-search*/
					/*No real cases have been found that commit this situation!*/
				}else{
					printf("solve_unsolved_states -- It is safe to jump\n");			
					/*debugit(&q->state);*/
				}
			}/* end q is strong*/ 

			free_StateActionPair( p );
			if(q->action)
				q->action->isReferenced = TRUE;
			printf("solved state reached\n");
			continue;
		} /* end q has next */

		/* need to call ff again */
		/* reset status */
		/* set initial condition */
		source_to_dest( &ginitial_state, &p->state);

		/*JC: Alternative goal*/
		if(p->intended_state.num_F > 0){
			source_to_dest( &ggoal_state, &p->intended_state);
		}else{
			source_to_dest( &ggoal_state, &originGoal);
		}

		gini_pair = p;

		if(g_is_strong){
			copy_state_level(&ginitial_state, p->state);
			if(p->intended_state.num_F > 0)
				copy_state_level(&ggoal_state, p->intended_state);
		}

		reset_ff_states();

		/* call ff */
		/*found_plan = do_enforced_hill_climbing(&ginitial_state, &ggoal_state);	*/
		found_plan = do_weak_planning(&ginitial_state, &ggoal_state);	

		if ( found_plan && gnum_plan_ops ) {	
			print_plan();
			PlanNode	subPlan; /*dump vairable for result*/
			subPlan.action = -1;
			subPlan.num_sons = 0;
			
			convert_ff_plan_to_fip_plan( &subPlan );				

			free_StateActionPair( p );				
		} else {
			if(same_state(&originState, &p->state)){
				printf("No solutions are found!\n");
				exit(0);
			}

			if(p->intended_state.num_F == 0){
				printf("try some other paths...\n");
				/*debugAllUnSolvedState();
				debugAllSolvedState();*/
				backtrack_solved_nodes(p);
				/*printf("after calling backtrack_solved_nodes ...\n");
				debugAllUnSolvedState();
				debugAllSolvedState();*/
			}else{
				printf("Change goal alternatively back to the oringinal goal\n");
				source_to_dest( &ggoal_state, &originGoal);

				reset_ff_states();

				/*found_plan = do_enforced_hill_climbing(&ginitial_state, &ggoal_state);	*/
				found_plan = do_weak_planning(&ginitial_state, &ggoal_state);	

				if ( found_plan && gnum_plan_ops ) {
					print_plan();
					PlanNode	subPlan; /*dump vairable for result*/
					subPlan.action = -1;
					subPlan.num_sons = 0;
					convert_ff_plan_to_fip_plan(&subPlan );				
					free_StateActionPair( p );			
				} else {
					printf( "Start to backtrack\n" );	/*the current state is a deadend. Need to backtrack*/
					backtrack_solved_nodes(p);			/*Jicheng: start to backtrack*/						
					/*exit(1);*/
				}
			}
		}				
	}

	source_to_dest( &ginitial_state, &originState);
	source_to_dest(&ggoal_state, &originGoal);
}


void reset_ff_states( void ){

	reset_search();
	reset_relax();
	gevaluated_states = 0;
	gnum_plan_ops = 0;
	source_to_dest( &(gplan_states[0]), &ginitial_state );
}


void reset_ff_states_for_multiple_purpose ( void ){
    
	reset_search();
	reset_relax();
	gevaluated_states = 0;
	gnum_plan_ops = 0;
	source_to_dest( &(gplan_states[0]), &ginitial_state );
}

void print_fip_plan_1( StateActionPair *pCurrentState,  
					  PlanNode	*pPlan,
					  int		indent){
						  int i;

						  CHECK_PTR(pPlan);
						  if( 0 == pPlan->num_sons ){
							  print_end_state(pCurrentState, indent);
							  return;
						  }

						  if( pPlan->sons[0]->isReferenced ){
							  /* print label */
							  printf("%s%d:\n", LABEL_STR, pPlan->sons[0]->id);
						  }

						  increaseIndent(indent);

						  print_op_name(pPlan->sons[0]->action);
						  printf("\n\n");

						  if( 1 <  pPlan->num_sons ){
							  increaseIndent(indent);
							  printf("while( true ){\n\n");


							  print_desired_state(
								  transferToNextState(pCurrentState, -1), indent+1);

							  for(i = 1; i < pPlan->num_sons; i++){
								  print_undesired_state(
									  transferToNextState(pCurrentState, pPlan->sons[i]->action), 
									  pPlan->sons[i], indent+1);
							  }	


							  increaseIndent(indent);
							  printf("}\n");		
						  }

						  CHECK_PTR(pPlan->sons[0]);
						  print_fip_plan_1(transferToNextState(pCurrentState, -1), pPlan->sons[0], indent);

}

/* print fip plan in a big while loop with a single
* switch inside.
*/					
void print_fip_plan_2(void){
	int indent = 1;
	StateActionPair *p = gsolved_states;

	increaseIndent( indent );
	printf("while(true){\n");

	increaseIndent( indent+1 );
	printf("switch(state){\n");

	while(p){
		increaseIndent( indent+2 );
		printf("case S%d:\t", p->id);

		if(p->action){
			print_op_name(p->action->action);
			printf(";\n");
		} else {
			printf("break;\n");
		}

		p = p->next;
	}	

	increaseIndent( indent+1 );
	printf("}\n");

	increaseIndent( indent );
	printf("}\n");
}

void print_fip_plan_3( PlanNode * pNode, int level ){
	int i;

	for(i = 0; i < pNode->num_sons; i++){
		increaseIndent(level);
		printf("%s%d: ", LABEL_STR, pNode->sons[i]->id);
		print_op_name( pNode->sons[i]->action );
		printf("\n");

		print_fip_plan_3( pNode->sons[i], level+1 );
	}
}


void print_all_states( void ){

	StateActionPair *p = gsolved_states;

	printf("All states involved in final fip plan:\n");

	while(p){
		printf("S%d:", p->id );
		print_state( p->state );
		printf("\n\n");
		p = p->next;
	}

}

