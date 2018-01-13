// ======================================================================
// FILE:        MyAI.hpp
//
// AUTHOR:      Abdullah Younis
//
// DESCRIPTION: This file contains your agent class, which you will
//              implement. You are responsible for implementing the
//              'getAction' function and any helper methods you feel you
//              need.
//
// NOTES:       - If you are having trouble understanding how the shell
//                works, look at the other parts of the code, as well as
//                the documentation.
//
//              - You are only allowed to make changes to this portion of
//                the code. Any changes to other portions of the code will
//                be lost when the tournament runs your code.
// ======================================================================

#ifndef MYAI_LOCK
#define MYAI_LOCK

#include <iostream>
#include <vector>
#include <queue>
#include <algorithm>
#include <set>
#include <string>
#include "Agent.hpp"

#define MAXSIZE 8

using namespace std;

class MyAI : public Agent
{
public:
	MyAI ( void );
	
	// if the predetermined action queue is not empty, pop and return
	// otherwise, 
	// update the board according to the sensors,
	// pick a grid from the explorable grids, (the closest one)
	// generate a route to the target grid, 
	// transform the route into a queue of action, pop and return
	Action getAction
	(
		bool stench,
		bool breeze,
		bool glitter,
		bool bump,
		bool scream
	);
	// ======================================================================
	// YOUR CODE BEGINS
	// ======================================================================
private:
	// represents the dungeon in the form of grids[r][c], starting from grids[0][0]
	// each grid has an int value representing its status
	// 0  unknown, the initial status
	// 1  visited, also implies a safe grid
	// 2  explorable, but not visited yet
	// 3  unmovable, could be a neighbor to "b", or a wall, will overwrite "wumpus"
	// 4  wumpus, where the worm could be, will be overwritten by #3 
	// *  wumpus is an inferior obstacle comparing to a pit or a wall
	// the logic is carefully constructed to avoid the following situation:
	// a pit overlaps the wumpus, which is removed later, 
	// so the agent considers the grid safe, which is not.
	vector<vector<int>> grids;
	// the actual size of the dungeon
	int sizeR; // number of rows
	int sizeC; // number of columes
	// agent's current zero-based position	
	int curR;
	int curC;
	// agent's previous position, before the latest action
	// used to resume status after bumping into a wall
	int prevR;
	int prevC;

	// dir stores the facing direction
	// E  dir mod 4 = 1
	// S  dir mod 4 = 2
	// W  dir mod 4 = 3
	// N  dir mod 4 = 0
	// starting with dir = 1001 to avoid negative values
	int dir;

	// the queue of a series of predetermined actions
	// if the queue is not empty, pop from the queue and return it as next action
	queue<Action> qActions;

	// agent's previous action
	// used to update status at the beginning of getAction()
	Action prevAction;	

	// indicates whether agent has fetched the gold
	bool goldFetched;

	// indicates whether the wumpus has been locked
	bool wumpusLocked;	

	// true if agent has not shoot yet
	bool canShoot;

	// true if agent has sensed stench somewhere
	bool wumpusSensed;
		
	// the location when agent first sensors stench
	// when shoot activated: agent moves to this location and shoot
	pair<int, int> wumpusFirstSeen;
	
	/*******************************
 	 * Subsection: helper functions 	
	 *******************************/

	// update agent's current position, previous position, 
	// and direction according to the last action taken
	// this function is called at the beginning of getActions()
	// prev: the previous action taken
	void updateAgentStatus(Action prev);

	// get legal neighbors of a grid (not limited to current grid)
	// (r, c) is the zero-based position of the grid
	// return: vector of neighbor positions, each in the form of (row, col) 
	vector<pair<int, int>> getNeighbors(int r, int c);

	// update dungeon info according to the sensors
	void updateBoard(bool st, bool br, bool gl, bool bp, bool sc);



	// transform a queue of route into a queue of actions
	// route:  a step-to-step route from current position to dest
	// finalDir: the direction agent will face after executing the series of actions
	// escape: if it is true, a CLIMB will be attached at the end of the queue
	// return: a queue of actions transformed from the route
	queue<Action> routeToActions(queue<pair<int, int>> route, 
									int& finalDir, bool escape);

	// helper function for routeToActions
	// add intermidiate TURN_LEFT/RIGHT between FORWARD
	// curDir:  curDir%4 represents direction
	// tarDir:  0(N),1(E),2(S),3(W)
	// return:  one of 4 cases -- empty, TURN_LEFT, TURN_RIGHT, (TURN_LEFT, TURN_LEFT)
	vector<Action> makeTurns(int curDir, int tarDir);


	// generate a route from current position to destination
	// backtracking heuristic path-finding algorithm:
	// the route only pass through visited grids
	// from valid frontier of each movement, 
	// it choose the one that is closest to the dest
	// backtrack when no more movement in frontier
	// (destR, destC): position of destination
	// return: a queue of grids towards to the destination
	queue<pair<int, int>> generateRoute(int destR, int destC);

	// BT route generator
	// src: starting grid
	// dest: destination
	// board: grids status, is a local copy of grids
	// routeToDest: the route from src to dest (solution)
	// routeSoFar: route found so far
	// found: stop BTS when a route to dest is found
	void generateRouteHelper (
					pair<int, int> src, 
					pair<int, int> dest,
					vector<vector<int>>& board,
					vector<pair<int, int>>& routeToDest,
					vector<pair<int, int>>& routeSoFar,
					bool& found);
						

	// helper function of route generator
	// frontier is defined as all neighbors of status 1(visited) and not on the route
	// board:  represents the route, status 9 means on the route
	// return: frontier of src, paired with each's distance to dest
	vector<pair<pair<int, int>, int>> getFrontier(
					pair<int, int> src, 
					pair<int, int> dest);

	// customer comparator for route generator
	// argument: a.first is the grid 1, a.second is its distance to destination
	// argument: b.first is the grid 2, b.second is its distance to destination
	// return:  a.second <= b.second, sort by distance to destination
	static bool compare(pair<pair<int, int>, int> a, pair<pair<int, int>, int> b);

 
	// retrive all grids labeled as explorable
	// return: a vector of grids in the form of (row, col)
	vector<pair<int, int>> getExplorable();	

	// from a set of grids, return the nearest one to the current grid
	// cand: candidate grids to pick
	// return: (row, col) of the nearest grid
	pair<int, int> getNearestGrid(vector<pair<int, int>> cand);

	// calculate the squared distance between current grid and the target grid
	// (r, c): position of target grid
	// return: distance b/w two grids
	int getDistance(int r, int c);

	// update board status when the position of wumpus is locked
	// (r, c): the position of wumpus
	// set all grids that are labeled "wumpus" to "explorable"
	void lockWumpus(int r, int c);


	// for debug use
	void printActionQueue(queue<Action> q);

};

#endif
