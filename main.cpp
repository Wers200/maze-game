#include "resource.h"
#include "renderer.h"

int width = 1;
int height = 1;
#pragma region gTmc stands for grid thickness multiplication constant, per se
const double gTmc = 0.075;
#pragma endregion

bool solved = false;

HCURSOR cursor = LoadCursor(NULL, IDC_ARROW);

HWND hWndG;

std::unique_ptr<Renderer> renderer;
std::shared_ptr<Maze> maze;

void UpdateWindowSize(HWND hWnd) {
	HMONITOR monitor = MonitorFromWindow(hWndG, MONITOR_DEFAULTTONEAREST);
	MONITORINFO info;
	info.cbSize = sizeof(MONITORINFO);
	GetMonitorInfo(monitor, &info);
	int monitorWidth = info.rcMonitor.right - info.rcMonitor.left;
	int monitorHeight = info.rcMonitor.bottom - info.rcMonitor.top;

	int strive = min(monitorWidth / 2, 2 * monitorHeight / 3);
	if (renderer->renderMode == 0 || renderer->renderMode == 1) {
		int div = max(maze->width, maze->height);
		renderer->gridThickness = strive / div * gTmc;
		width = maze->width * (strive / div) - renderer->gridThickness;
		height = maze->height * (strive / div) - renderer->gridThickness + (renderer->infoStrip ? 98 + renderer->gridThickness : 0);
		renderer->cellSize = strive / div - renderer->gridThickness;
		if (renderer->renderMode == 1) width += renderer->gridThickness;
	}
	else if(renderer->renderMode == 2) {
		width = strive;
		height = width + (renderer->infoStrip ? 136 : 0);
	}
	
	renderer->Resize(width, height);
	renderer->showPath = false;

	SetWindowPos(hWnd, NULL, 0, 0, width + 16, height + 39, SWP_SHOWWINDOW | SWP_NOMOVE);
	InvalidateRect(hWndG, NULL, NULL);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	hWndG = hWnd;
	switch (uMsg) {
		case WM_NCCREATE:
			EnableNonClientDpiScaling(hWnd);
			break;
		case WM_SETCURSOR:
			SetCursor(cursor);
			break;
		case WM_DISPLAYCHANGE:
			UpdateWindowSize(hWnd);
			break;
		case WM_PAINT:
		{
			renderer->Render();
			ValidateRect(hWnd, NULL);
			break;
		}
		case WM_CHAR:
		{
			switch (wParam) {
			case 'M':
			case 'm':
				if (renderer->renderMode != 2) renderer->renderMode++;
				else renderer->renderMode = 0;

				maze->PlayerReset();				

				UpdateWindowSize(hWnd);
				break;
			case 'H':
			case 'h':
				renderer->infoStrip = !renderer->infoStrip;
				UpdateWindowSize(hWnd);
				break;
			case 'F':
			case 'f':
				maze->iterations += 1;
				break;
			case 'G':
			case 'g':
				if (maze->iterations > 0) maze->iterations -= 1;
				break;
			case 'C':
			case 'c':
				renderer->showPath = !renderer->showPath;
				break;
			case 'E':
			case 'e':
				renderer->cameraRange++;
				break;
			case 'R':
			case 'r':
				if(renderer->cameraRange > 0) renderer->cameraRange--;
				break;
			case 'T':
			case 't':
				renderer->wallFrequency++;
				break;
			case 'Y':
			case 'y':
				if(renderer->wallFrequency > 0) renderer->wallFrequency--;
				break;
			case '+':
				maze->width += 1;
				maze->Generate();
				UpdateWindowSize(hWnd);
				break;
			case '-':
				if (maze->width > 3) {
					maze->width -= 1;
					maze->Generate();
					UpdateWindowSize(hWnd);
				}
				break;
			case '=':
				maze->height += 1;
				maze->Generate();
				UpdateWindowSize(hWnd);
				break;
			case '_':
				if (maze->height > 3) {
					maze->height -= 1;
					maze->Generate();
					UpdateWindowSize(hWnd);
				}
				break;
			}
			break;
		}
		case WM_KEYDOWN:
		{
			switch (wParam) {
			case VK_RETURN:
				renderer->showPath = false;
				maze->Generate();
				break;
			case 0x57:
			case VK_UP:
				if (renderer->renderMode == 0 && maze->y > 0 && maze->CellCheck((int)maze->x, (int)maze->y - 1, Maze::PathMask)) maze->y--;
				else if (renderer->renderMode > 0) maze->keyForward = true;
				break;
			case 0x53:
			case VK_DOWN:
				if (renderer->renderMode == 0 && maze->y < maze->height - 1 && maze->CellCheck((int)maze->x, (int)maze->y + 1, Maze::PathMask)) maze->y++;
				else if (renderer->renderMode > 0) maze->keyBackward = true;
				break;
			case 0x41:
			case VK_LEFT:
				if (renderer->renderMode == 0 && maze->x > 0 && maze->CellCheck((int)maze->x - 1, (int)maze->y, Maze::PathMask)) maze->x--;
				else if (renderer->renderMode > 0) maze->keyRight = true;
				break;
			case 0x44:
			case VK_RIGHT:
				if (renderer->renderMode == 0 && maze->x < maze->width - 1 && maze->CellCheck((int)maze->x + 1, (int)maze->y, Maze::PathMask)) maze->x++;
				else if (renderer->renderMode > 0) maze->keyLeft = true;
				break;
			}
			break;
		}
		case WM_KEYUP:
		{
			if (renderer->renderMode > 0) {
				switch (wParam) {
				case 0x57:
				case VK_UP:
					maze->keyForward = false;
					break;
				case 0x53:
				case VK_DOWN:
					maze->keyBackward = false;
					break;
				case 0x41:
				case VK_LEFT:
					maze->keyRight = false;
					break;
				case 0x44:
				case VK_RIGHT:
					maze->keyLeft = false;
					break;
				}
			}
			break;
		}
		case WM_DPICHANGED:
			UpdateWindowSize(hWnd);
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
	}
	if (maze && (int)trunc(maze->x) == maze->width - 1 && (int)trunc(maze->y) == maze->height - 1) renderer->showPath = true;
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
	WNDCLASSEX wc;
	ZeroMemory(&wc, sizeof(WNDCLASSEX));
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.lpszClassName = L"MazeWindow";
	wc.lpfnWndProc = WndProc;
	wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON));
	wc.hInstance = hInstance;
	RegisterClassEx(&wc);

	hWndG = CreateWindowEx(
		NULL,
		L"MazeWindow",
		L"Maze",
		WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME | WS_MAXIMIZEBOX),
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		width,
		height,
		NULL,
		NULL,
		hInstance,
		NULL
	);

	maze = std::shared_ptr<Maze>(new Maze(hWndG));
	maze->Generate();

	renderer = std::unique_ptr<Renderer>(new Renderer(hWndG, maze));

	UpdateWindowSize(hWndG);

	UpdateWindow(hWndG);
	ShowWindow(hWndG, SW_SHOW);

	MSG msg = {};
	bool quit = false;

	thread render([&quit] {
		LARGE_INTEGER frequency;
		LARGE_INTEGER t1, t2;

		QueryPerformanceFrequency(&frequency);
		QueryPerformanceCounter(&t1);

		while (!quit) {
			InvalidateRect(hWndG, NULL, FALSE);
			Sleep(1);
			QueryPerformanceCounter(&t2);

			if (renderer->renderMode != 0) {
				maze->PlayerUpdate((t2.QuadPart - t1.QuadPart) * 1.0 / frequency.QuadPart, &t1);
				if ((int)trunc(maze->x) == width - 1 && (int)trunc(maze->y) == height - 1) renderer->showPath = true;
			}
		}
	});

	while (!quit) {
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) {
				quit = true;	
				break;
			}
			else {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}
	
	render.join();

	return 0;
}