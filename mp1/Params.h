/**********************************
 * FILE NAME: Params.h
 *
 * DESCRIPTION: Header file of Parameter class
 **********************************/

#ifndef _PARAMS_H_
#define _PARAMS_H_

#include "stdincludes.h"
#include "Params.h"
#include "Member.h"

/*
We adopt
the terminology of the epidemiology literature and call a site with an update it is willing to share
infective with respect to that update. A site is susceptible if it has not yet received the update;
and a site is removed if it has received the update but is no longer willing to share the update.
Anti-entropy is an example of a simple epidemic: one in which sites are always either susceptible
or infective.
*/
enum testTYPE { CREATE_TEST, READ_TEST, UPDATE_TEST, DELETE_TEST };

/**
 * CLASS NAME: Params
 *
 * DESCRIPTION: Params class describing the test cases
 */
class Params{
public:
	int MAX_NNB;                // max number of neighbors
	int SINGLE_FAILURE;			// single/multi failure
	double MSG_DROP_PROB;		// message drop probability
	double STEP_RATE;		    // dictates the rate of insertion
	int EN_GPSZ;			    // actual number of peers
	int MAX_MSG_SIZE;
	int DROP_MSG;
	int dropmsg;
	int globaltime;
	int allNodesJoined;
	short PORTNUM;
	Params();
	void setparams(char *);
	int getcurrtime();
};

#endif /* _PARAMS_H_ */
