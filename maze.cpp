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

	bool fullScaleCheck = true;

	if (CastRay(x, y, 0) - xVelocity * dt < 1 / 10.0 - 1 / 100.0 || CastRay(x, y, PI) + xVelocity * dt < 1 / 10.0 - 1 / 100.0) {
		xVelocity = 0;
		fullScaleCheck = false;
	}
	else x += xVelocity * dt;

	if (CastRay(x, y, PI / 2) - yVelocity * dt < 1 / 10.0 - 1 / 100.0 || CastRay(x, y, 3 * PI / 2) + yVelocity * dt < 1 / 10.0 - 1 / 100.0) {
		yVelocity = 0;
		fullScaleCheck = false;
	}
	else y += yVelocity * dt;

	if (fullScaleCheck) {
		for (double i = direction - PI; i < direction + PI; i += PI / 40) {
			double dist = CastRay(x, y, i);

			if (dist < 1 / 10.0 - 1 / 100.0) {
				x -= xVelocity * dt;
				y -= yVelocity * dt;
				xVelocity *= 0;
				yVelocity *= 0;
				break;
			}
		}
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

double Maze::CastRay(double x, double y, double direction) {
	double xComponent = cos(direction);
	double yComponent = sin(direction);

	double minimum = LONG_MAX;

	if (xComponent != 0 && yComponent != 0) {
		double slope = yComponent / xComponent;

		if (yComponent > 0) {
			for (int i = (int)y + 1; i <= height; i++) {
				double x_0 = (i - y) / slope + x;

				double dist = (x_0 - x) * (x_0 - x) + (y - i) * (y - i);

				if (!CellCheck((int)x_0, i, PathMask) && minimum > dist) minimum = dist;
			}
		}
		else {
			for (int i = (int)y; i >= 0; i--) {
				double x_0 = (i - y) / slope + x;

				double dist = (x_0 - x) * (x_0 - x) + (y - i) * (y - i);

				if ((!CellCheck((int)x_0, i - 1, PathMask) || i == 0) && minimum > dist) minimum = dist;
			}
		}

		if (xComponent > 0) {
			for (int i = (int)x + 1; i <= width; i++) {
				double y_0 = (i - x) * slope + y;

				double dist = (i - x) * (i - x) + (y - y_0) * (y - y_0);

				if (!CellCheck(i, (int)y_0, PathMask) && minimum > dist) minimum = dist;
			}
		}
		else {
			for (int i = (int)x; i >= 0; i--) {
				double y_0 = (i - x) * slope + y;

				double dist = (i - x) * (i - x) + (y - y_0) * (y - y_0);

				if ((!CellCheck(i - 1, (int)y_0, PathMask) || i == 0) && minimum > dist) minimum = dist;
			}
		}
	}
	else if (yComponent != 0) {
		if (yComponent > 0) {
			for (int i = (int)y + 1; i <= height; i++) {
				double dist = (y - i) * (y - i);

				if (!CellCheck(x, i, PathMask) && minimum > dist) minimum = dist;
			}
		}
		else {
			for (int i = (int)y; i >= 0; i--) {
				double dist = (y - i) * (y - i);

				if ((!CellCheck(x, i - 1, PathMask) || i == 0) && minimum > dist) minimum = dist;
			}
		}
	}
	else {
		if (xComponent > 0) {
			for (int i = (int)x + 1; i <= width; i++) {
				double dist = (i - x) * (i - x);

				if (!CellCheck(i, y, PathMask) && minimum > dist) minimum = dist;
			}
		}
		else {
			for (int i = (int)x; i >= 0; i--) {
				double dist = (i - x) * (i - x);

				if ((!CellCheck(i - 1, y, PathMask) || i == 0) && minimum > dist) minimum = dist;
			}
		}
	}

	return sqrt(minimum);
}

double Maze::GetPlayerDirection() {
	return direction;
}
