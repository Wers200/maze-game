#pragma once

#include "framework.h"

class Maze {
public:
	int width;
	int height;
	int iterations;

	static const BYTE PathMask = 0b00000001;
	static const BYTE TruePathMask = 0b00000010;

	double x;
	double y;

	bool keyForward;
	bool keyBackward;
	bool keyLeft;
	bool keyRight;

	Maze(HWND hWnd_);
	~Maze();

	void Generate();
	void Reallocate();
	bool CellCheck(int x, int y, BYTE mask);
	void CellAssign(int x, int y, BYTE mask);
	void CellRemove(int x, int y, BYTE mask);
	int PathsAround(int x, int y);

	void PlayerUpdate(double delta, LARGE_INTEGER* timepoint);
	void PlayerReset();

	double GetPlayerDirection();
private:
	HWND hWnd;

	thread generation;
	bool dying;

	BYTE* board;

	vector<pair<int, int>> moves;
	vector<pair<int, int>> turns;
	vector<int> directions;
	unordered_set<long> closedMoves;

	double xVelocity;
	double yVelocity;
	double acceleration;
	double friction;

	double direction;
	double angularVelocity;
	double angularAcceleration;
	double angularFriction;

	void GenerateT();
};