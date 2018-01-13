// ======================================================================
// FILE:        MyAI.cpp
//
// AUTHOR:     Lei Pan
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

#include "MyAI.hpp"

// #define DEBUG


/*
 * Constructor
 */
MyAI::MyAI() : Agent()
{
	// initialize the board to its biggest size, all grids have status of 0(unknown)
	for (int r = 0; r < MAXSIZE; ++r) {
		grids.push_back(vector<int>(MAXSIZE, 0));
	}
	// initialize actual size to MAXSIZE 
	sizeR = MAXSIZE;
	sizeC = MAXSIZE;
	// initial status of agent, at (0,0), facing EAST
	dir = 1001; // to avoid becoming negative
	curR = 0;
	curC = 0;
	prevR = 0;
	prevC = 0;
	goldFetched = false;
	wumpusLocked = false;
	canShoot = true;
	wumpusSensed = false;
	wumpusFirstSeen = make_pair(0,0);
	prevAction = CLIMB; // initial action cannot be FORWARD, TURN, or GRAB
}

/*
 * Return the next action
 */
Agent::Action MyAI::getAction
(
	bool stench,
	bool breeze,
	bool glitter,
	bool bump,
	bool scream
)
{
	updateAgentStatus(prevAction);

 	#ifdef DEBUG
	cout << "\nAgent at: (" << curR << ", " << curC << "), D:" << dir%4 << endl; 
	cout << "Prev: " << prevR << prevC << endl;
	#endif


	// case1: take action from a predetermined series of actions
	if (!qActions.empty()) {
		
		#ifdef DEBUG
		cout << "case1: predetermined, QOA sz: " << qActions.size() << endl;	
		printActionQueue(qActions);		
		#endif

		Agent::Action nextAction = qActions.front();
		qActions.pop();
		prevAction = nextAction;
		return nextAction;
	}
	else {
		updateBoard(stench, breeze, glitter, bump, scream);
		// case2: gold found!
		if (goldFetched)
			return GRAB;
		vector<pair<int, int>> explorable = getExplorable();
		// case3: no more moves, 
		// activate SHOOT if agent has ammo and wumpus has been sensed
		// otherwise return to entrance and escape
		if (explorable.empty()) {
			
			#ifdef DEBUG
			cout << "case3: No more moves" << endl;
			#endif
			

			// construct and execute SHOOT action queue
			if (canShoot && wumpusSensed) {
				int r = wumpusFirstSeen.first;
				int c = wumpusFirstSeen.second;
				queue<pair<int, int>> route = generateRoute(r, c);
				int finalDir = 0;
				qActions = routeToActions(route, finalDir, false);

				// determine where to shoot
				// TO-DO: MAKE FUNCTION
				pair<int, int> wumpus(0, 0);
				vector<pair<int, int>> nbs = getNeighbors(r, c);
				for (pair<int, int> nb: nbs) {
					if (grids[nb.first][nb.second] == 4) {
						wumpus.first = nb.first;
						wumpus.second = nb.second;
					}
				}
				// turn to face the wumpus
				// TO-DO: MAKE FUNCTION
				int tarDir = 0;
				// determine the direction of next movement
				if (wumpus.first == r - 1) // face South
					tarDir = 2;
				else if (wumpus.first == r + 1) // face North
					tarDir = 0;
				else if (wumpus.second == c - 1) // face West
					tarDir = 3;
				else // face East
					tarDir = 1;
				// make turn as necessary
				vector<Agent::Action> turns = makeTurns(finalDir, tarDir);
				for (auto turn: turns)
					qActions.push(turn);
				qActions.push(SHOOT);
				Action nextAction = qActions.front();
				qActions.pop();
				prevAction = nextAction;
				canShoot = false;
				grids[wumpus.first][wumpus.second] = 2;
				return nextAction;
			}

			// generate a route to entrance
			queue<pair<int, int>> route = generateRoute(0, 0);
			// transform the route to a queue of actions, attach CLIMB
			int ph = 0;
			qActions = routeToActions(route, ph, true);
			Action nextAction = qActions.front();
			qActions.pop();
			prevAction = nextAction;
			return nextAction;
		}
		// case4: move to the closest explorable grid through a safe route
		else {
			pair<int, int> dest = getNearestGrid(explorable);
			
			#ifdef DEBUG
			cout << "case4: Move to: " << dest.first << "," << dest.second << endl;
			#endif
			
			queue<pair<int, int>> route = generateRoute(dest.first, dest.second);
			
			#ifdef DEBUG
			cout << "route sz: " << route.size() << endl;
			#endif
			int ph = 0;
			qActions = routeToActions(route, ph, false);
			Action nextAction = qActions.front();


			#ifdef DEBUG
			printActionQueue(qActions);
			#endif

			qActions.pop();
			prevAction = nextAction;
			return nextAction;
		}
	}
}

void MyAI::updateAgentStatus(Agent::Action prev) {
	// store previous position
	prevR = curR;
	prevC = curC;
	// update agent's status according to the last action taken
	if (prev == FORWARD) {
		if (dir%4 == 0) // North
			++curR;
		else if (dir%4 == 1) // East
			++curC;
		else if (dir%4 == 2) // South
			--curR;
		else // West
			--curC;
	}
	else if (prev == TURN_LEFT)
		--dir;
	else if (prev == TURN_RIGHT)
		++dir;
}


vector<pair<int, int>> MyAI::getNeighbors(int r, int c) {
	vector<pair<int, int>> retVal;
	if (r != 0)
		retVal.push_back(make_pair(r-1, c));
	if (r != sizeR-1)
		retVal.push_back(make_pair(r+1, c));
	if (c != 0)
		retVal.push_back(make_pair(r, c-1));
	if (c != sizeC-1)
		retVal.push_back(make_pair(r, c+1));
	return retVal;
}

void MyAI::updateBoard(bool st, bool br, bool gl, bool bp, bool sc) {
	// set current grid to visited
	grids[curR][curC] = 1;
	
	if (gl) {
		goldFetched = true;
		// generate a route to the entrance
		queue<pair<int, int>> route = generateRoute(0, 0);
		// transform the route to a queue of actions
		int ph = 0;
		qActions = routeToActions(route, ph, true);
		return; // game win
	}
	if (bp) {
		// restore current position to the previous position
		curR = prevR;
		curC = prevC;
		// update board status
		// if agent is facing EAST, 
		// update sizeC and set the grids beyond the wall unmovable
		if (dir%4 == 1) { // 1 is EAST
			sizeC = curC + 1;
			for (int r = 0; r < MAXSIZE; ++r) {
				for (int c = curC+1; c < MAXSIZE; ++c)
					grids[r][c] = 3; // 3 is unmovable
			}		
		}
		// do the same if agent is facing NORTH
		else if (dir%4 == 0) { // 0 is NORTH
			sizeR = curR + 1;
			for (int r = curR+1; r < MAXSIZE; ++r) {
				for (int c = 0; c < MAXSIZE; ++c)
					grids[r][c] = 3; // 3 is unmovable
			}		
		}
		// Do nothing if agent bumps into WEST or SOUTH wall (shouldn't happen)

		// SHOULD NOT happen, just buy an insurance
		// if BUMP happens during action queue, abondon the queue
		if (!qActions.empty()) {
			queue<Action>().swap(qActions);
			return;
		}
	}
	if (sc) {
		// set all grids labeled "wumpus" to "explorable"
		for (int r = 0; r < sizeR; ++r) {
			for (int c = 0; c < sizeC; ++c) {
				if (grids[r][c] == 4) // 4 is wumpus
					grids[r][c] == 2; // 2 is explorable
			}
		}
	}
	// update status of neighbor grids according to breeze and stench
	// breeze will overwrite stench
	vector<pair<int, int>> neighbors = getNeighbors(curR, curC);
	// if breeze, all unvisited neighbors are set to unmovable
	// stench is ignored in the existence of breeze
	if (br) {
		for (pair<int, int> nb: neighbors) {
			int r = nb.first;
			int c = nb.second;
			// handling logic with great care
			if (grids[r][c] != 1 && grids[r][c] != 2) // 1 is visited, 2 is explorable
				grids[r][c] = 3;
		}
	}
	// if only stench is present
	// two possibilities: wumpus locked or not 
	else if (st) {
		// update status at the first time sensing stench
		if (!wumpusSensed) {
			wumpusSensed = true;
			wumpusFirstSeen.first = curR;
			wumpusFirstSeen.second = curC;
		}
		// if wumpus is not locked yet, try to lock wumpus first
		if (!wumpusLocked) {
			for (pair<int, int> nb: neighbors) {
				int r = nb.first;
				int c = nb.second;
				if (grids[r][c] == 4)
					lockWumpus(r, c);
			}
		}
		// failed to lock wumpus, update wumpus's possible positions
		if (!wumpusLocked) {
			for (pair<int, int> nb: neighbors) {
				int r = nb.first;
				int c = nb.second;
				// handling logic with great care
				if (grids[r][c] != 1 && grids[r][c] != 2 && grids[r][c] != 3)
					grids[r][c] = 4;
			}
		}
		// sensing stench and wumpus is locked
		else {	
			for (pair<int, int> nb: neighbors) {
				int r = nb.first;
				int c = nb.second;
				// handling logic with great care
				if (grids[r][c] != 1 && grids[r][c] != 4)
					grids[r][c] = 2;
			}
		}
	}
	// neighbors are safe, set unvisited nbs to explorable
	else {
		for (pair<int, int> nb: neighbors) {
			int r = nb.first;
			int c = nb.second;
			if (grids[r][c] != 1)
				grids[r][c] = 2; // 2 is explorable
		}
	}
}


queue<Agent::Action> MyAI::routeToActions(queue<pair<int, int>> route, 
											int& finalDir, bool escape) {
	// record the status while moving along the route, 
	// starting from current position, facing current direction
	int r = curR;
	int c = curC;
	int d = dir;
	queue<Agent::Action> retVal;
	while (!route.empty()) {
		pair<int, int> next = route.front(); // next grid in the route
		int tarDir = 0; // direction required to move to next grid
		route.pop();

		// determine the direction of next movement
		if (next.first == r - 1) // move South
			tarDir = 2;
		else if (next.first == r + 1) // move North
			tarDir = 0;
		else if (next.second == c - 1) // move West
			tarDir = 3;
		else // move East
			tarDir = 1;
		
		// make turn as necessary
		vector<Agent::Action> turns = makeTurns(d, tarDir);
		for (auto turn: turns)
			retVal.push(turn);
		// move forward
		retVal.push(FORWARD);
		// update status
		r = next.first;
		c = next.second;
		d = tarDir;
	}
	finalDir = d;
	if (escape)
		retVal.push(CLIMB);
	return retVal;
}

vector<Agent::Action> MyAI::makeTurns(int curDir, int tarDir) {
	vector<Agent::Action> retVal;
	// no need to change direction
	if (curDir%4 == tarDir)
		return retVal;
	// turn around: 1<->3, or 0<->2
	else if ((curDir+tarDir)%2 == 0) {
		retVal.push_back(TURN_LEFT);
		retVal.push_back(TURN_LEFT);
	}
	// turn left: 0->3, 1->0, 2->1, or 3->2
	else if ( (curDir%4 == 0 && tarDir == 3) || curDir%4 == tarDir+1 )
		retVal.push_back(TURN_LEFT);
	// turn right
	else
		retVal.push_back(TURN_RIGHT);
	return retVal;
}

// generate a safe route to the destination
queue<pair<int, int>> MyAI::generateRoute(int destR, int destC) {
	queue<pair<int, int>> retVal;

	// must make the destination "reachable" by setting its status to 1(visited)
	// this does not affect the board status 
	// b/c the dest will be set to visited anyway on the agent's arrival
	grids[destR][destC] = 1;

	pair<int, int> src(curR, curC);
	pair<int, int> dest(destR, destC);

	// keep two board status, board is a local copy attached to each generateRoute()
	// board: determine if a grid is already on the route(status 9)
	// grids: determine if the route can pass through the grid(status 1)
	vector<vector<int>> board = grids; // local copy's initial status doesn't matter
	
	vector<pair<int, int>> route;
	vector<pair<int, int>> routeSoFar;
	bool found = false;
	
	
	#ifdef DEBUG
	cout << "~DEBUG~src: " << src.first << ", " << src.second << ", ";
	cout << "dest: " << dest.first << ", " << dest.second << endl;
	cout << "grids visited: ";
	for (int r = 0; r < sizeR; ++r)
		for (int c = 0; c < sizeC; ++c)
			if (grids[r][c] == 1)
				cout << "(" << r << ", " << c << "), ";
	cout << endl;
	#endif
	
	board[src.first][src.second] = 9; // put src onto route before BTS
	generateRouteHelper(src, dest, board, route, routeSoFar, found);
	for (int i = 0; i < route.size(); ++i)
		retVal.push(route[i]);
	return retVal;
}

void MyAI::generateRouteHelper(
					pair<int, int> src,
					pair<int, int> dest,
					vector<vector<int>>& board,
					vector<pair<int, int>>& routeToDest,
					vector<pair<int, int>>& routeSoFar,
					bool& found) {

	/* #ifdef DEBUG
	cout << "BT|src: (" << src.first << ", " << src.second
		<< "), sz of routeSoFar: " << routeSoFar.size() << endl;
	#endif */
	


	// base case: when to stop
	if (src.first == dest.first && src.second == dest.second) {
		found = true;
		routeToDest = routeSoFar;
	}
	// makes BTS ends early (while not finding every possible solution)
	if (found)
		return;
	// retrive the frontier of src, sort by distance to dest
	vector<pair<pair<int, int>, int>> frontier = getFrontier(src, dest);
	if (frontier.empty())
		return;
	sort(frontier.begin(), frontier.end(), compare);
	// run BTS on the frontier
	for (pair<pair<int, int>, int> fr: frontier) {
		// a status is stored in board to avoid passing through a grid multiple times
		// board[r][c] == 9 -- already on the route
		// all other values -- not on the route yet
		int r = fr.first.first; // row
		int c = fr.first.second; // column
		if (board[r][c] != 9) {
			board[r][c] = 9;
			routeSoFar.push_back(fr.first);
			generateRouteHelper(fr.first, dest, board, routeToDest, routeSoFar, found);
			routeSoFar.pop_back();
			board[r][c] = 1; // any value not 9 suffices
		}
	}
}

vector<pair<pair<int, int>, int>> MyAI::getFrontier(
				pair<int, int> src,
				pair<int, int> dest) {
	vector<pair<pair<int, int>, int>> retVal;
	vector<pair<int, int>> neighbors;
	neighbors = getNeighbors(src.first, src.second);
	// frontier is defined as all nbs that are visited and not on the route
	for (pair<int, int> nb : neighbors) {
		if (grids[nb.first][nb.second] == 1) {
			int distance = (nb.first - dest.first) * (nb.first - dest.first)
						+ (nb.second - dest.second) * (nb.second - dest.second);
			retVal.push_back(make_pair(nb, distance));
		}
	}
	return retVal;
}

bool MyAI::compare(pair<pair<int, int>, int> a, pair<pair<int, int>, int> b) {
	return a.second <= b.second;
}

vector<pair<int, int>> MyAI::getExplorable() {
	vector<pair<int, int>> retVal;
	for (int r = 0; r < sizeR; ++r) {
		for (int c = 0; c < sizeC; ++c) {
			if (grids[r][c] == 2)
				retVal.push_back(make_pair(r, c));
		}
	}
	#ifdef DEBUG
	cout << "Explorable: ";
	for (auto it = retVal.begin(); it != retVal.end(); ++it)
		cout << "(" << (*it).first << ", " << (*it).second << "), ";
	cout << endl;
	#endif
	
	return retVal;
}

pair<int, int> MyAI::getNearestGrid(vector<pair<int, int>> cand) {
	int minDist = MAXSIZE * MAXSIZE;
	pair<int, int> retVal;
	for (pair<int, int> grid: cand) {
		int dist = getDistance(grid.first, grid.second);
		if (dist < minDist) {
			minDist = dist;
			retVal = make_pair(grid.first, grid.second);
		}
	}
	return retVal;
}

int MyAI::getDistance(int r, int c) {
	return (r - curR) * (r - curR) + (c - curC) * (c - curC);
}

void MyAI::lockWumpus(int r, int c) {
	wumpusLocked = true;	
	for (int i = 0; i < sizeR; ++i) {
		for (int j = 0; j < sizeC; ++j) {
			if (grids[i][j] == 4)
				grids[i][j] = 2;
		}
	}
	grids[r][c] = 4;
}

void MyAI::printActionQueue(queue<Agent::Action> q) {
	while (!q.empty()) {
		cout << q.front() << ", ";
		q.pop();
	}
	cout << endl;
}
// ======================================================================
// YOUR CODE ENDS
// ======================================================================
