/*

       AAAA    CCCC   OOOO   TTTTTT   SSSSS  PPPPP
      AA  AA  CC     OO  OO    TT    SS      PP  PP
      AAAAAA  CC     OO  OO    TT     SSSS   PPPPP
      AA  AA  CC     OO  OO    TT        SS  PP
      AA  AA   CCCC   OOOO     TT    SSSSS   PP

######################################################
##########    ACO algorithms for the TSP    ##########
######################################################

      Version: 1.0
      File:    ants.c
      Author:  Thomas Stuetzle
      Purpose: implementation of procedures for ants' behaviour
      Check:   README.txt and legal.txt
      Copyright (C) 2002  Thomas Stuetzle
*/

/***************************************************************************

    Program's name: acotsp

    Ant Colony Optimization algorithms (AS, ACS, EAS, RAS, MMAS, BWAS) for the 
    symmetric TSP 

    Copyright (C) 2004  Thomas Stuetzle

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    email: stuetzle no@spam ulb.ac.be
    mail address: Universite libre de Bruxelles
                  IRIDIA, CP 194/6
                  Av. F. Roosevelt 50
                  B-1050 Brussels
		  Belgium

***************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <limits.h>
#include <time.h>

#include "InOut.h"
#include "TSP.h"
#include "ants.h"
#include "ls.h"
#include "utilities.h"
#include "timer.h"

extern int iLevyFlag;				// 0 or 1
extern double dLevyThreshold;		//0--1
extern double dLevyRatio;			//0.1--5

extern double dContribution;  		//0--10

extern int iGreedyLevyFlag;			// 0 or 1
extern double dGreedyEpsilon;		//0--1
extern double dGreedyLevyThreshold;	//0--1
extern double dGreedyLevyRatio;		//0.1--5

ant_struct *ant;
ant_struct *best_so_far_ant;
ant_struct *restart_best_ant;

double   **pheromone;
double   **total;

double   *prob_of_selection;

long int n_ants;      /* number of ants */
long int nn_ants;     /* length of nearest neighbor lists for the ants'
			 solution construction */

double rho;           /* parameter for evaporation */
double alpha;         /* importance of trail */
double beta;          /* importance of heuristic evaluate */
double q_0;           /* probability of best choice in tour construction */


long int as_flag;     /* ant system */
long int eas_flag;    /* elitist ant system */
long int ras_flag;    /* rank-based version of ant system */
long int mmas_flag;   /* MAX-MIN ant system */
long int bwas_flag;   /* best-worst ant system */
long int acs_flag;    /* ant colony system */

long int elitist_ants;    /* additional parameter for elitist
			     ant system, no. elitist ants */

long int ras_ranks;       /* additional parameter for rank-based version 
			     of ant system */

double   trail_max;       /* maximum pheromone trail in MMAS */
double   trail_min;       /* minimum pheromone trail in MMAS */
long int u_gb;            /* every u_gb iterations update with best-so-far ant */

double   trail_0;         /* initial pheromone level in ACS and BWAS */



void allocate_ants ( void )
/*    
      FUNCTION:       allocate the memory for the ant colony, the best-so-far and 
                      the iteration best ant
      INPUT:          none
      OUTPUT:         none
      (SIDE)EFFECTS:  allocation of memory for the ant colony and two ants that 
                      store intermediate tours

*/
{
    long int i;
  
    if((ant = malloc(sizeof( ant_struct ) * n_ants +
		     sizeof(ant_struct *) * n_ants	 )) == NULL){
	printf("Out of memory, exit.");
	exit(1);
    }
    for ( i = 0 ; i < n_ants ; i++ ) {
	ant[i].tour        = calloc(n+1, sizeof(long int));
	ant[i].visited     = calloc(n, sizeof(char));
    }

    if((best_so_far_ant = malloc(sizeof( ant_struct ) )) == NULL){
	printf("Out of memory, exit.");
	exit(1);
    }
    best_so_far_ant->tour        = calloc(n+1, sizeof(long int));
    best_so_far_ant->visited     = calloc(n, sizeof(char));

    if((restart_best_ant = malloc(sizeof( ant_struct ) )) == NULL){
	printf("Out of memory, exit.");
	exit(1);
    }
    restart_best_ant->tour        = calloc(n+1, sizeof(long int));
    restart_best_ant->visited     = calloc(n, sizeof(char));

    if ((prob_of_selection = malloc(sizeof(double) * (nn_ants + 1))) == NULL) {
	printf("Out of memory, exit.");
	exit(1);
    }
    /* Ensures that we do not run over the last element in the random wheel.  */
    prob_of_selection[nn_ants] = HUGE_VAL;
}



long int find_best( void ) 
/*    
      FUNCTION:       find the best ant of the current iteration
      INPUT:          none
      OUTPUT:         index of struct containing the iteration best ant
      (SIDE)EFFECTS:  none
*/
{
    long int   min;
    long int   k, k_min;

    min = ant[0].tour_length;
    k_min = 0;
    for( k = 1 ; k < n_ants ; k++ ) {
	if( ant[k].tour_length < min ) {
	    min = ant[k].tour_length;
	    k_min = k;
	}
    }
    return k_min;
}



long int find_worst( void ) 
/*    
      FUNCTION:       find the worst ant of the current iteration
      INPUT:          none
      OUTPUT:         pointer to struct containing iteration best ant
      (SIDE)EFFECTS:  none
*/
{
    long int   max;
    long int   k, k_max;

    max = ant[0].tour_length;
    k_max = 0;
    for( k = 1 ; k < n_ants ; k++ ) {
	if( ant[k].tour_length > max ) {
	    max = ant[k].tour_length;
	    k_max = k;
	}
    }
    return k_max;
}



/************************************************************
 ************************************************************
Procedures for pheromone manipulation 
 ************************************************************
 ************************************************************/



void init_pheromone_trails( double initial_trail )
/*    
      FUNCTION:      initialize pheromone trails
      INPUT:         initial value of pheromone trails "initial_trail"
      OUTPUT:        none
      (SIDE)EFFECTS: pheromone matrix is reinitialized
*/
{
    long int i, j;

    TRACE ( printf(" init trails with %.15f\n",initial_trail); );

    /* Initialize pheromone trails */
    for ( i = 0 ; i < n ; i++ ) {
	for ( j =0 ; j <= i ; j++ ) {
	    pheromone[i][j] = initial_trail;
	    pheromone[j][i] = initial_trail;
	    total[i][j] = initial_trail;
	    total[j][i] = initial_trail;
	}
    }
}



void evaporation( void )
/*    
      FUNCTION:      implements the pheromone trail evaporation
      INPUT:         none
      OUTPUT:        none
      (SIDE)EFFECTS: pheromones are reduced by factor rho
*/
{ 
    long int    i, j;

    TRACE ( printf("pheromone evaporation\n"); );

    for ( i = 0 ; i < n ; i++ ) {
	for ( j = 0 ; j <= i ; j++ ) {
	    pheromone[i][j] = (1 - rho) * pheromone[i][j];
	    pheromone[j][i] = pheromone[i][j];
	}
    }
}



void evaporation_nn_list( void )
/*    
      FUNCTION:      simulation of the pheromone trail evaporation
      INPUT:         none
      OUTPUT:        none
      (SIDE)EFFECTS: pheromones are reduced by factor rho
      REMARKS:       if local search is used, this evaporation procedure 
                     only considers links between a city and those cities
		     of its candidate list
*/
{ 
    long int    i, j, help_city;

    TRACE ( printf("pheromone evaporation nn_list\n"); );

    for ( i = 0 ; i < n ; i++ ) {
	for ( j = 0 ; j < nn_ants ; j++ ) {
	    help_city = instance.nn_list[i][j];
	    pheromone[i][help_city] = (1 - rho) * pheromone[i][help_city];
	}
    }
}



void global_update_pheromone( ant_struct *a )
/*    
      FUNCTION:      reinforces edges used in ant k's solution
      INPUT:         pointer to ant that updates the pheromone trail 
      OUTPUT:        none
      (SIDE)EFFECTS: pheromones of arcs in ant k's tour are increased
*/
{  
    long int i, j, h;
    //double   d_tau;

    TRACE ( printf("global pheromone update\n"); );

    //d_tau = 1.0 / (double) a->tour_length;
    for ( i = 0 ; i < n ; i++ ) {
	j = a->tour[i];
	h = a->tour[i+1];
	//pheromone[j][h] += d_tau;
	//pheromone[j][h] += ( 1 + dContribution ) / ( 1 + dContribution * instance.n * instance.distance[j][h] / a->tour_length );
	//pheromone[j][h] += ( 1 - dContribution * instance.distance[j][h] / a->tour_length ) / ( 1 - dContribution / instance.n ) / a->tour_length;
	pheromone[j][h] += 2 / ( a->tour_length + dContribution * instance.n * instance.distance[j][h] );
	pheromone[h][j] = pheromone[j][h];
    }
}



void global_update_pheromone_weighted( ant_struct *a, long int weight )
/*    
      FUNCTION:      reinforces edges of the ant's tour with weight "weight"
      INPUT:         pointer to ant that updates pheromones and its weight  
      OUTPUT:        none
      (SIDE)EFFECTS: pheromones of arcs in the ant's tour are increased
*/
{  
    long int      i, j, h;
    double        d_tau;

    TRACE ( printf("global pheromone update weighted\n"); );

    //d_tau = (double) weight / (double) a->tour_length;
    for ( i = 0 ; i < n ; i++ ) {
	j = a->tour[i];
	h = a->tour[i+1];
	//pheromone[j][h] += d_tau;
	pheromone[j][h] += 2 * weight / ( a->tour_length + dContribution * instance.n * instance.distance[j][h] );
	pheromone[h][j] = pheromone[j][h];
    }       
}



void compute_total_information( void )
/*    
      FUNCTION: calculates heuristic info times pheromone for each arc
      INPUT:    none  
      OUTPUT:   none
*/
{
    long int     i, j;

    TRACE ( printf("compute total information\n"); );

    for ( i = 0 ; i < n ; i++ ) {
	for ( j = 0 ; j < i ; j++ ) {
	    total[i][j] = pow(pheromone[i][j],alpha) * pow(HEURISTIC(i,j),beta);
	    total[j][i] = total[i][j];
	}
    }
}



void compute_nn_list_total_information( void )
/*    
      FUNCTION: calculates heuristic info times pheromone for arcs in nn_list
      INPUT:    none  
      OUTPUT:   none
*/
{ 
    long int    i, j, h;

    TRACE ( printf("compute total information nn_list\n"); );

    for ( i = 0 ; i < n ; i++ ) {
	for ( j = 0 ; j < nn_ants ; j++ ) {
	    h = instance.nn_list[i][j];
	    if ( pheromone[i][h] < pheromone[h][i] )
		/* force pheromone trails to be symmetric as much as possible */
		pheromone[h][i] = pheromone[i][h];
	    total[i][h] = pow(pheromone[i][h], alpha) * pow(HEURISTIC(i,h),beta);
	    total[h][i] = total[i][h];
	}
    }
}



/****************************************************************
 ****************************************************************
Procedures implementing solution construction and related things
 ****************************************************************
 ****************************************************************/



void ant_empty_memory( ant_struct *a ) 
/*    
      FUNCTION:       empty the ants's memory regarding visited cities
      INPUT:          ant identifier
      OUTPUT:         none
      (SIDE)EFFECTS:  vector of visited cities is reinitialized to FALSE
*/
{
    long int   i;

    for( i = 0 ; i < n ; i++ ) {
	a->visited[i]=FALSE;
    }
}



void place_ant( ant_struct *a , long int step )
/*    
      FUNCTION:      place an ant on a randomly chosen initial city
      INPUT:         pointer to ant and the number of construction steps 
      OUTPUT:        none
      (SIDE)EFFECT:  ant is put on the chosen city
*/
{
    long int     rnd;

    rnd = (long int) (ran01( &seed ) * (double) n); /* random number between 0 .. n-1 */
    a->tour[step] = rnd; 
    a->visited[rnd] = TRUE;
}



void choose_best_next( ant_struct *a, long int phase )
/*    
      FUNCTION:      chooses for an ant as the next city the one with
                     maximal value of heuristic information times pheromone 
      INPUT:         pointer to ant and the construction step
      OUTPUT:        none 
      (SIDE)EFFECT:  ant moves to the chosen city
*/
{ 
    long int city, current_city, next_city;
    double   value_best;

    next_city = n;
    DEBUG( assert ( phase > 0 && phase < n ); );
    current_city = a->tour[phase-1];
    value_best = -1.;             /* values in total matrix are always >= 0.0 */    
    for ( city = 0 ; city < n ; city++ ) {
	if ( a->visited[city] ) 
	    ; /* city already visited, do nothing */
	else {
	    if ( total[current_city][city] > value_best ) {
		next_city = city;
		value_best = total[current_city][city];
	    }
	} 
    }
    DEBUG( assert ( 0 <= next_city && next_city < n); );
    DEBUG( assert ( value_best > 0.0 ); )
    DEBUG( assert ( a->visited[next_city] == FALSE ); )
    a->tour[phase] = next_city;
    a->visited[next_city] = TRUE;
}



void neighbour_choose_best_next( ant_struct *a, long int phase )
/*    
      FUNCTION:      chooses for an ant as the next city the one with
                     maximal value of heuristic information times pheromone 
      INPUT:         pointer to ant and the construction step "phase" 
      OUTPUT:        none 
      (SIDE)EFFECT:  ant moves to the chosen city
*/
{ 
    long int i, current_city, next_city, help_city;
    double   value_best, help;
  
    next_city = n;
    DEBUG( assert ( phase > 0 && phase < n ); );
    current_city = a->tour[phase-1];
    DEBUG ( assert ( 0 <= current_city && current_city < n ); )
    value_best = -1.;             /* values in total matix are always >= 0.0 */    
    for ( i = 0 ; i < nn_ants ; i++ ) {
	help_city = instance.nn_list[current_city][i];
	if ( a->visited[help_city] ) 
	    ;   /* city already visited, do nothing */
	else {
	    help = total[current_city][help_city];
	    if ( help > value_best ) {
		value_best = help;
		next_city = help_city;
	    }
	}
    }
    if ( next_city == n )
	/* all cities in nearest neighbor list were already visited */
	choose_best_next( a, phase );
    else {
	DEBUG( assert ( 0 <= next_city && next_city < n); )
	DEBUG( assert ( value_best > 0.0 ); )
	DEBUG( assert ( a->visited[next_city] == FALSE ); )
	a->tour[phase] = next_city;
	a->visited[next_city] = TRUE;
    }
}



void choose_closest_next( ant_struct *a, long int phase )
/*    
      FUNCTION:      Chooses for an ant the closest city as the next one 
      INPUT:         pointer to ant and the construction step "phase" 
      OUTPUT:        none 
      (SIDE)EFFECT:  ant moves to the chosen city
*/
{ 
    long int city, current_city, next_city, min_distance;
  
    next_city = n;
    DEBUG( assert ( phase > 0 && phase < n ); );
    current_city = a->tour[phase-1];
    min_distance = INFTY;             /* Search shortest edge */    
    for ( city = 0 ; city < n ; city++ ) {
	if ( a->visited[city] ) 
	    ; /* city already visited */
	else {
	    if ( instance.distance[current_city][city] < min_distance) {
		next_city = city;
		min_distance = instance.distance[current_city][city];
	    }
	} 
    }
    DEBUG( assert ( 0 <= next_city && next_city < n); );
    a->tour[phase] = next_city;
    a->visited[next_city] = TRUE;
}



void neighbour_choose_and_move_to_next( ant_struct *a , long int phase )
/*    
      FUNCTION:      Choose for an ant probabilistically a next city among all 
                     unvisited cities in the current city's candidate list. 
		     If this is not possible, choose the closest next
      INPUT:         pointer to ant the construction step "phase" 
      OUTPUT:        none 
      (SIDE)EFFECT:  ant moves to the chosen city
*/
{ 
    long int i, help; 
    long int current_city;
    double   rnd, partial_sum = 0., sum_prob = 0.0;
    /*  double   *prob_of_selection; */ /* stores the selection probabilities 
	of the nearest neighbor cities */
    double   *prob_ptr;

	long int ordered_city[nn_ants];

	// Both q_0 and dGreedyEpsilon will run these codes.
	rnd = ran01( &seed );
    if ( ( (q_0 > 0.0) && (rnd < q_0) ) || ( iGreedyLevyFlag && (dGreedyEpsilon > 0.0) && (rnd < dGreedyEpsilon) ) ) {
	/* with a probability q_0 make the best possible choice
	   according to pheromone trails and heuristic information */
	/* we first check whether q_0 > 0.0, to avoid the very common case
	   of q_0 = 0.0 to have to compute a random number, which is
	   expensive computationally */
	neighbour_choose_best_next(a, phase);
	return;
    }

    prob_ptr = prob_of_selection;

    current_city = a->tour[phase-1]; /* current_city city of ant k */
    DEBUG( assert ( current_city >= 0 && current_city < n ); )
    for ( i = 0 ; i < nn_ants ; i++ ) {
	if ( a->visited[instance.nn_list[current_city][i]] ) 
	    prob_ptr[i] = 0.0;   /* city already visited */
	else {
	    DEBUG( assert ( instance.nn_list[current_city][i] >= 0 && instance.nn_list[current_city][i] < n ); )	    
		prob_ptr[i] = total[current_city][instance.nn_list[current_city][i]];
		sum_prob += prob_ptr[i];
	}
	ordered_city[i] = i;	//define original value;
    }

	/*
	printf("Pheromone before sort!\n");
	for( i=0; i<nn_ants; i++ )
	{
		printf("Pheromone [%ld] is [%ld] [%lf].\n", i, ordered_city[i], prob_ptr[ ordered_city[i] ] ); 
	}
	*/

	if( iLevyFlag || iGreedyLevyFlag )
	{
		//Sort City by total value.
		int iStopFlag = 0;
		while( !iStopFlag )
		{
			iStopFlag = 1;
			for( i = 0; i < nn_ants-1; i++ )
			{
				if( prob_ptr[ ordered_city[i] ] < prob_ptr[ ordered_city[i+1] ] )
				{
					long int j = ordered_city[i];
					ordered_city[i] = ordered_city[i+1];
					ordered_city[i+1] = j;
					iStopFlag = 0;
				}
			}
		}
		/*
		printf("Pheromone after sort!\n");
		for( i=0; i<nn_ants; i++ )
		{
			printf("Pheromone [%ld] is [%ld] [%lf].\n", i, ordered_city[i], prob_ptr[ ordered_city[i] ] ); 
		}
		*/
	}
	
    if (sum_prob <= 0.0) {
	/* All cities from the candidate set are tabu */
	choose_best_next( a, phase );
    }     
    else {  
	/* at least one neighbor is eligible, chose one according to the
	   selection probabilities */
	
	//amplify the random number from dGreedyEpsilon--1 to 0--1 for next Levy flight process.
	if( iGreedyLevyFlag && rnd >= dGreedyEpsilon )
	{
		rnd = (rnd - dGreedyEpsilon) / (1-dGreedyEpsilon);
	}
	else
	{
		rnd = ran01( &seed );
	}
	
	if( iLevyFlag )
	{
		double rndLevy = ran01( &seed );
		if( rndLevy > dLevyThreshold )
		{
			rnd = 1 - 1/dLevyRatio * (1-rnd) * (1-rndLevy) / (1-dLevyThreshold);
		}
	}

	if( iGreedyLevyFlag )
	{
		double rndLevy = ran01( &seed );
		if( rndLevy > dGreedyLevyThreshold )
		{
			rnd = 1 - 1/dGreedyLevyRatio * (1-rnd) * (1-rndLevy) / (1-dGreedyLevyThreshold);
		}
	}
	
	rnd *= sum_prob;
	i = 0;

	if( iLevyFlag || iGreedyLevyFlag )
		partial_sum = prob_ptr[ ordered_city[i] ];
	else
		partial_sum = prob_ptr[i];
        /* This loop always stops because prob_ptr[nn_ants] == HUGE_VAL  */
        while (partial_sum <= rnd) {
            i++;
			if( iLevyFlag || iGreedyLevyFlag )
			{
				partial_sum += prob_ptr[ ordered_city[i] ];
			}
			else
			{
				partial_sum += prob_ptr[i];
			}
        }
        /* This may very rarely happen because of rounding if rnd is
           close to 1.  */
        if (i == nn_ants) {
            neighbour_choose_best_next(a, phase);
            return;
        }
	DEBUG( assert ( 0 <= i && i < nn_ants); );
	if( iLevyFlag || iGreedyLevyFlag )
	{
	DEBUG( assert ( prob_ptr[ ordered_city[i] ] >= 0.0); );
	help = instance.nn_list[current_city][ ordered_city[i] ];
	}
	else
	{
	DEBUG( assert ( prob_ptr[i] >= 0.0); );
	help = instance.nn_list[current_city][i];
	}
	DEBUG( assert ( help >= 0 && help < n ); )
	DEBUG( assert ( a->visited[help] == FALSE ); )
	a->tour[phase] = help; /* instance.nn_list[current_city][i]; */
	a->visited[help] = TRUE;
    }
}




/****************************************************************
 ****************************************************************
Procedures specific to MAX-MIN Ant System
 ****************************************************************
****************************************************************/



void mmas_evaporation_nn_list( void )
/*    
      FUNCTION:      simulation of the pheromone trail evaporation for MMAS
      INPUT:         none
      OUTPUT:        none
      (SIDE)EFFECTS: pheromones are reduced by factor rho
      REMARKS:       if local search is used, this evaporation procedure 
                     only considers links between a city and those cities
		     of its candidate list
*/
{ 
    long int    i, j, help_city;

    TRACE ( printf("mmas specific evaporation on nn_lists\n"); );

    for ( i = 0 ; i < n ; i++ ) {
	for ( j = 0 ; j < nn_ants ; j++ ) {
	    help_city = instance.nn_list[i][j];
	    pheromone[i][help_city] = (1 - rho) * pheromone[i][help_city];
	    if ( pheromone[i][help_city] < trail_min )
		pheromone[i][help_city] = trail_min;
	}
    }
}



void check_pheromone_trail_limits( void )
/*    
      FUNCTION:      only for MMAS without local search: 
                     keeps pheromone trails inside trail limits
      INPUT:         none
      OUTPUT:        none
      (SIDE)EFFECTS: pheromones are forced to interval [trail_min,trail_max]
*/
{ 
    long int    i, j;

    TRACE ( printf("mmas specific: check pheromone trail limits\n"); );

    for ( i = 0 ; i < n ; i++ ) {
	for ( j = 0 ; j < i ; j++ ) {
	    if ( pheromone[i][j] < trail_min ) {
		pheromone[i][j] = trail_min;
		pheromone[j][i] = trail_min;
	    } else if ( pheromone[i][j] > trail_max ) {
		pheromone[i][j] = trail_max;
		pheromone[j][i] = trail_max;
	    }
	}
    }
}



void check_nn_list_pheromone_trail_limits( void )
/*    
      FUNCTION:      only for MMAS with local search: keeps pheromone trails 
                     inside trail limits
      INPUT:         none
      OUTPUT:        none
      (SIDE)EFFECTS: pheromones are forced to interval [trail_min,trail_max]
      COMMENTS:      currently not used since check for trail_min is integrated
                     mmas_evaporation_nn_list and typically check for trail_max 
		     is not done (see FGCS paper or ACO book for explanation 
*/
{ 
    long int    i, j, help_city;

    TRACE ( printf("mmas specific: check pheromone trail limits nn_list\n"); );

    for ( i = 0 ; i < n ; i++ ) {
	for ( j = 0 ; j < nn_ants ; j++ ) {
	    help_city = instance.nn_list[i][j];
	    if ( pheromone[i][help_city] < trail_min )
		pheromone[i][help_city] = trail_min;
	    if ( pheromone[i][help_city] > trail_max )
		pheromone[i][help_city] = trail_max;
	}
    }
}



/****************************************************************
 ****************************************************************
Procedures specific to Ant Colony System
 ****************************************************************
****************************************************************/



void global_acs_pheromone_update( ant_struct *a )
/*    
      FUNCTION:      reinforces the edges used in ant's solution as in ACS
      INPUT:         pointer to ant that updates the pheromone trail 
      OUTPUT:        none
      (SIDE)EFFECTS: pheromones of arcs in ant k's tour are increased
*/
{  
    long int i, j, h;
    double   d_tau;

    TRACE ( printf("acs specific: global pheromone update\n"); );

    d_tau = 1.0 / (double) a->tour_length;

    for ( i = 0 ; i < n ; i++ ) {
	j = a->tour[i];
	h = a->tour[i+1];

	pheromone[j][h] = (1. - rho) * pheromone[j][h] + rho * d_tau;
	pheromone[h][j] = pheromone[j][h];

	total[h][j] = pow(pheromone[h][j], alpha) * pow(HEURISTIC(h,j),beta);
	total[j][h] = total[h][j];
    }
}



void local_acs_pheromone_update( ant_struct *a, long int phase )
/*    
      FUNCTION:      removes some pheromone on edge just passed by the ant
      INPUT:         pointer to ant and number of constr. phase
      OUTPUT:        none
      (SIDE)EFFECTS: pheromones of arcs in ant k's tour are increased
      COMMENTS:      I did not do experiments with with different values of the parameter 
                     xi for the local pheromone update; therefore, here xi is fixed to 0.1 
		     as suggested by Gambardella and Dorigo for the TSP. If you wish to run 
		     experiments with that parameter it may be reasonable to use it as a 
		     commandline parameter
*/
{  
    long int  h, j;
    
    DEBUG ( assert ( phase > 0 && phase <= n ); )
	j = a->tour[phase];

    h = a->tour[phase-1];
    DEBUG ( assert ( 0 <= j && j < n ); )
	DEBUG ( assert ( 0 <= h && h < n ); )
	/* still additional parameter has to be introduced */
	pheromone[h][j] = (1. - 0.1) * pheromone[h][j] + 0.1 * trail_0;
    pheromone[j][h] = pheromone[h][j];
    total[h][j] = pow(pheromone[h][j], alpha) * pow(HEURISTIC(h,j),beta);
    total[j][h] = total[h][j];
}



/****************************************************************
 ****************************************************************
Procedures specific to Best-Worst Ant System
 ****************************************************************
****************************************************************/



void bwas_worst_ant_update( ant_struct *a1, ant_struct *a2)
/*    
      FUNCTION:      uses additional evaporation on the arcs of iteration worst
                     ant that are not shared with the global best ant
      INPUT:         pointer to the worst (a1) and the best (a2) ant
      OUTPUT:        none
      (SIDE)EFFECTS: pheromones on some arcs undergo additional evaporation
*/
{  
    long int    i, j, h, pos, pred;
    long int    distance;
    long int    *pos2;        /* positions of cities in tour of ant a2 */ 

    TRACE ( printf("bwas specific: best-worst pheromone update\n"); );

    pos2 = malloc(n * sizeof(long int));
    for ( i = 0 ; i < n ; i++ ) {
	pos2[a2->tour[i]] = i;
    }
 
    distance = 0;
    for ( i = 0 ; i < n ; i++ ) {
	j = a1->tour[i];
	h = a1->tour[i+1];
	pos = pos2[j];
	if (pos - 1 < 0)
	    pred = n - 1;
	else 
	    pred = pos - 1;
	if (a2->tour[pos+1] == h)
	    ; /* do nothing, edge is common with a2 (best solution found so far) */
	else if (a2->tour[pred] == h)
	    ; /* do nothing, edge is common with a2 (best solution found so far) */
	else {   /* edge (j,h) does not occur in ant a2 */       
	    pheromone[j][h] = (1 - rho) * pheromone[j][h];
	    pheromone[h][j] = (1 - rho) * pheromone[h][j];
	}
    }
    free ( pos2 );
}



void bwas_pheromone_mutation( void )
/*    
      FUNCTION: implements the pheromone mutation in Best-Worst Ant System
      INPUT:    none  
      OUTPUT:   none
*/
{
    long int     i, j, k;
    long int     num_mutations;
    double       avg_trail = 0.0, mutation_strength = 0.0, mutation_rate = 0.3;

    TRACE ( printf("bwas specific: pheromone mutation\n"); );

    /* compute average pheromone trail on edges of global best solution */
    for ( i = 0 ; i < n ; i++ ) {
	avg_trail +=  pheromone[best_so_far_ant->tour[i]][best_so_far_ant->tour[i+1]];
    }
    avg_trail /= (double) n;
  
    /* determine mutation strength of pheromone matrix */ 
    /* FIXME: we add a small value to the denominator to avoid any
       potential division by zero. This may not be fully correct
       according to the original BWAS. */
    if ( max_time > 0.1 )
	mutation_strength = 4. * avg_trail * (elapsed_time(VIRTUAL) - restart_time) / (max_time - restart_time + 0.0001);
    else if ( max_tours > 100 )
	mutation_strength = 4. * avg_trail * (iteration - restart_iteration) 
            / (max_tours - restart_iteration + 1);
    else
	printf("apparently no termination condition applied!!\n");

    /* finally use fast version of matrix mutation */
    mutation_rate = mutation_rate / n * nn_ants;
    num_mutations = n * mutation_rate / 2;   
    /* / 2 because of adjustment for symmetry of pheromone trails */
 
    if ( restart_iteration < 2 )
	num_mutations = 0; 

    for ( i = 0 ; i < num_mutations ; i++ ) {
	j =   (long int) (ran01( &seed ) * (double) n);
	k =   (long int) (ran01( &seed ) * (double) n);
	if ( ran01( &seed ) < 0.5 ) {
	    pheromone[j][k] += mutation_strength;
	    pheromone[k][j] = pheromone[j][k];
	}
	else {
	    pheromone[j][k] -= mutation_strength;
	    if ( pheromone[j][k] <= 0.0 ) {
		pheromone[j][k] = EPSILON;
	    }
	    pheromone[k][j] = pheromone[j][k]; 
	}
    }
}



/**************************************************************************
 **************************************************************************
Procedures specific to the ant's tour manipulation other than construction
***************************************************************************
 **************************************************************************/



void copy_from_to(ant_struct *a1, ant_struct *a2) 
{
/*    
      FUNCTION:       copy solution from ant a1 into ant a2
      INPUT:          pointers to the two ants a1 and a2 
      OUTPUT:         none
      (SIDE)EFFECTS:  a2 is copy of a1
*/
    int   i;
  
    a2->tour_length = a1->tour_length;
    for ( i = 0 ; i < n ; i++ ) {
	a2->tour[i] = a1->tour[i];
    }
    a2->tour[n] = a2->tour[0];
}



long int nn_tour( void )
/*    
      FUNCTION:       generate some nearest neighbor tour and compute tour length
      INPUT:          none
      OUTPUT:         none
      (SIDE)EFFECTS:  needs ant colony and one statistic ants
*/
{
    long int phase, help;

    ant_empty_memory( &ant[0] );

    phase = 0; /* counter of the construction steps */
    place_ant( &ant[0], phase);

    while ( phase < n-1 ) {
	phase++;
	choose_closest_next( &ant[0],phase);
    }
    phase = n;
    ant[0].tour[n] = ant[0].tour[0];
    if ( ls_flag ) {
	two_opt_first( ant[0].tour );
    }
    n_tours += 1;
/*   copy_from_to( &ant[0], best_so_far_ant ); */
    ant[0].tour_length = compute_tour_length( ant[0].tour );

    help = ant[0].tour_length;
    ant_empty_memory( &ant[0] );
    return help;
}



long int distance_between_ants( ant_struct *a1, ant_struct *a2)
/*    
      FUNCTION: compute the distance between the tours of ant a1 and a2
      INPUT:    pointers to the two ants a1 and a2
      OUTPUT:   distance between ant a1 and a2
*/
{  
    long int    i, j, h, pos, pred;
    long int    distance;
    long int    *pos2;        /* positions of cities in tour of ant a2 */ 

    pos2 = malloc(n * sizeof(long int));
    for ( i = 0 ; i < n ; i++ ) {
	pos2[a2->tour[i]] = i;
    }

    distance = 0;
    for ( i = 0 ; i < n ; i++ ) {
	j = a1->tour[i];
	h = a1->tour[i+1];
	pos = pos2[j];
	if (pos - 1 < 0)
	    pred = n - 1;
	else 
	    pred = pos - 1;
	if (a2->tour[pos+1] == h)
	    ; /* do nothing, edge is common with best solution found so far */
	else if (a2->tour[pred] == h)
	    ; /* do nothing, edge is common with best solution found so far */
	else {   /* edge (j,h) does not occur in ant a2 */       
	    distance++;
	}
    }
    free ( pos2 );
    return distance;
}


