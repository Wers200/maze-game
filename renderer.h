#pragma once

#include "framework.h"
#include "maze.h"

class Renderer {
public:
	Renderer(HWND hWnd_, std::shared_ptr<Maze> maze);
	~Renderer();

	std::shared_ptr<Maze> maze;
	/*
	renderMode = 0: 2D rendering, 2D gameplay
	renderMode = 1: 2D rendering, 3D gameplay
	renderMode = 2: 3D rendering, 3D gameplay
	*/
	int renderMode;
	bool infoStrip;

	float cellSize;
	float gridThickness;
	bool showPath;

	int wallFrequency;
	int cameraRange;

	HRESULT Render();
	void Resize(UINT width, UINT height);
private:
	HRESULT CreateDeviceIndependentResources();
	HRESULT CreateDeviceResources();
	void DiscardDeviceResources();

	double CastRay(double x, double y, double direction);

	HWND hWnd;
	ID2D1Factory* factory;
	ID2D1HwndRenderTarget* renderTarget;

	IDWriteFactory* writeFactory;
	IDWriteTextFormat* textFormat;

	ID2D1SolidColorBrush* cellBrush;
	ID2D1SolidColorBrush* gridBrush;
	ID2D1SolidColorBrush* playerBrush;
	ID2D1SolidColorBrush* pathBrush;
	ID2D1SolidColorBrush* infoBrush;
	ID2D1SolidColorBrush* whiteBrush;
};