#include "maze.h"

long PairToLong(int x, int y) {
	return (x << 16) + y;
}

Maze::Maze(HWND hWnd_) :
	hWnd(hWnd_),
	board(nullptr),
	x(0.5),
	y(0.5),
	width(10),
	height(10),
	iterations(5),
	dying(false),
	moves(vector<pair<int, int>>()),
	turns(vector<pair<int, int>>()),
	closedMoves(unordered_set<long>()),

	keyForward(false),
	keyBackward(false),
	keyLeft(false),
	keyRight(false),

	xVelocity(0.0),
	yVelocity(0.0),
	direction(0.0),
	angularVelocity(0.0),

	acceleration(0.003),
	friction(0.03),
	angularAcceleration(0.005),
	angularFriction(0.05)
{}

Maze::~Maze() {
	dying = true;
	generation.join();

	if (board) free(board);
	board = nullptr;
}

void Maze::Reallocate() {
	if (board) free(board);
	board = (BYTE*)malloc(sizeof(BYTE) * width * height);
	if (board) memset(board, 0, sizeof(BYTE) * width * height);
	if (!board) Reallocate();
}

bool Maze::CellCheck(int x, int y, BYTE mask) {
	if (x < width && x >= 0 && y < height && y >= 0) return (board[y * width + x] & mask) != 0;
	else return false;
}

int Maze::PathsAround(int x, int y) {
	return CellCheck(x - 1, y, PathMask) + CellCheck(x + 1, y, PathMask) + CellCheck(x, y - 1, PathMask) + CellCheck(x, y + 1, PathMask) + CellCheck(x, y, PathMask);
}

void Maze::CellAssign(int x, int y, BYTE mask) {
	if (x < width && x >= 0 && y < height && y >= 0) board[y * width + x] |= mask;
	return;
}

void Maze::CellRemove(int x, int y, BYTE mask) {
	if (x < width && x >= 0 && y < height && y >= 0) board[y * width + x] ^= mask;
	return;
}

void Maze::GenerateT() {
	long long seed = chrono::steady_clock().now().time_since_epoch().count() % MAXINT32;

	srand(seed);

	moves.clear();
	turns.clear();

	int cX = 0;
	int cY = 0;
	CellAssign(0, 0, PathMask);
	CellAssign(0, 0, TruePathMask);

	int currentDirection = -1;

	while (!dying && (cX != width - 1 || cY != height - 1)) {
		directions.clear();
		if (cX > 0 && PathsAround(cX - 1, cY) == 1 && closedMoves.find(PairToLong(cX - 1, cY)) == closedMoves.end()) directions.push_back(0);
		if (cX < width - 1 && PathsAround(cX + 1, cY) == 1 && closedMoves.find(PairToLong(cX + 1, cY)) == closedMoves.end()) directions.push_back(1);
		if (cY > 0 && PathsAround(cX, cY - 1) == 1 && closedMoves.find(PairToLong(cX, cY - 1)) == closedMoves.end()) directions.push_back(2);
		if (cY < height - 1 && PathsAround(cX, cY + 1) == 1 && closedMoves.find(PairToLong(cX, cY + 1)) == closedMoves.end()) directions.push_back(3);
		if (directions.size() > 0) {
			int direction = directions[rand() % directions.size()];
			if (currentDirection != direction) {
				currentDirection = direction;
				turns.push_back({ cX, cY });
			}
			switch (direction) {
			case 0:
				cX--;
				break;
			case 1:
				cX++;
				break;
			case 2:
				cY--;
				break;
			case 3:
				cY++;
				break;
			}
			CellAssign(cX, cY, PathMask);
			CellAssign(cX, cY, TruePathMask);
			moves.push_back({ cX, cY });
		}
		else {
			CellRemove(cX, cY, PathMask);
			CellRemove(cX, cY, TruePathMask);
			closedMoves.insert(PairToLong(cX, cY));
			if (moves.size() > 0) moves.pop_back();
			if (moves.size() > 0) cX = moves[moves.size() - 1].first;
			if (moves.size() > 0) cY = moves[moves.size() - 1].second;
		}
	}
	closedMoves.clear();

	for (int j = 0; j < iterations; j++) {
		currentDirection = -1;
		int k = turns.size();
		for (int i = 0; i < k; i++) {
			cX = turns[i].first;
			cY = turns[i].second;
			while (!dying) {
				directions.clear();
				if (cX > 0 && PathsAround(cX - 1, cY) == 1) directions.push_back(0);
				if (cX < width - 1 && PathsAround(cX + 1, cY) == 1) directions.push_back(1);
				if (cY > 0 && PathsAround(cX, cY - 1) == 1) directions.push_back(2);
				if (cY < height - 1 && PathsAround(cX, cY + 1) == 1) directions.push_back(3);
				if (directions.size() > 0) {
					int direction = directions[rand() % directions.size()];
					if (currentDirection != direction) {
						currentDirection = direction;
						turns.push_back({ cX, cY });
					}
					switch (direction) {
					case 0:
						cX--;
						break;
					case 1:
						cX++;
						break;
					case 2:
						cY--;
						break;
					case 3:
						cY++;
						break;
					}
					CellAssign(cX, cY, PathMask);
				}
				else break;
			}
		}
	}

	return;
}

void Maze::Generate() {
	dying = true;
	if(generation.joinable()) generation.join();
	dying = false;

	Reallocate();
	generation = thread(&Maze::GenerateT, this);

	x = 0.5;
	y = 0.5;
}

void Maze::PlayerUpdate(double delta, LARGE_INTEGER* timepoint) {
	QueryPerformanceCounter(timepoint);

	double dt = delta * 30.0;

	if (keyLeft && !keyRight) angularVelocity += angularAcceleration * dt;
	else if (keyRight) angularVelocity -= angularAcceleration * dt;
	angularVelocity *= (1 - angularFriction * dt);
	direction += angularVelocity * dt;

	double xComponent = cos(direction);
	double yComponent = sin(direction);

	if (keyForward && !keyBackward) {
		xVelocity += xComponent * acceleration * dt;
		yVelocity += yComponent * acceleration * dt;
	}
	else if (keyBackward) {
		xVelocity -= xComponent * acceleration * dt;
		yVelocity -= yComponent * acceleration * dt;
	}
	xVelocity *= (1 - friction * dt);
	yVelocity *= (1 - friction * dt);

	if (!CellCheck((int)trunc(x + xVelocity * dt), (int)trunc(y), PathMask) || x + xVelocity * dt <= 0 || !CellCheck((int)trunc(x), (int)trunc(y + yVelocity * dt), PathMask) || y + yVelocity * dt <= 0) {
		xVelocity = 0;
		yVelocity = 0;
	}
	else {
		x += xVelocity * dt;
		y += yVelocity * dt;
	}
}

void Maze::PlayerReset() {
	xVelocity = 0;
	yVelocity = 0;
	angularVelocity = 0;
	keyForward = false;
	keyBackward = false;
	keyLeft = false;
	keyRight = false;
}

double Maze::GetPlayerDirection() {
	return direction;
}