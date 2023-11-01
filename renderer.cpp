#include "renderer.h"

Renderer::Renderer(HWND hWnd_, Maze* maze) :
	hWnd(hWnd_),
	maze(maze),

	factory(NULL),
	writeFactory(NULL),
	textFormat(NULL),
	renderTarget(NULL),
	cellBrush(NULL),
	gridBrush(NULL),
	playerBrush(NULL),
	pathBrush(NULL),
	infoBrush(NULL),
	whiteBrush(NULL),

	cameraRange(5),
	wallFrequency(10),

	cellSize(1),
	gridThickness(1),
	showPath(false),
	renderMode(0),
	infoStrip(true)
{
	CreateDeviceIndependentResources();
}

Renderer::~Renderer() {
	SafeRelease(&factory);
	SafeRelease(&writeFactory);
	SafeRelease(&textFormat);
	DiscardDeviceResources();
}

HRESULT Renderer::CreateDeviceIndependentResources() {
	HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &factory);

	return hr;
}

HRESULT Renderer::CreateDeviceResources() {
	HRESULT hr = S_OK;

	if (!renderTarget) {
		RECT rect;
		GetClientRect(hWnd, &rect);

		D2D1_SIZE_U size = D2D1::SizeU(rect.right - rect.left, rect.bottom - rect.top);

		hr = factory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(), D2D1::HwndRenderTargetProperties(hWnd, size), &renderTarget);

		if (SUCCEEDED(hr)) hr = renderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Red), &playerBrush);
		if (SUCCEEDED(hr)) hr = renderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &gridBrush);
		if (SUCCEEDED(hr)) hr = renderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::LimeGreen), &cellBrush);
		if (SUCCEEDED(hr)) hr = renderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Blue), &pathBrush);
		if (SUCCEEDED(hr)) hr = renderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::DarkSlateGray), &infoBrush);
		if (SUCCEEDED(hr)) hr = renderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &whiteBrush);

		if(SUCCEEDED(hr)) {
			hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(writeFactory), reinterpret_cast<IUnknown**>(&writeFactory));
		}

		if (SUCCEEDED(hr)) {
			hr = writeFactory->CreateTextFormat(
				L"Consolas",
				NULL,
				DWRITE_FONT_WEIGHT_NORMAL,
				DWRITE_FONT_STYLE_NORMAL,
				DWRITE_FONT_STRETCH_NORMAL,
				16,
				L"en-EN",
				&textFormat
			);
		}

		if (SUCCEEDED(hr)) {
			textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
			textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
		}
	}

	return hr;
}

void Renderer::DiscardDeviceResources() {
	SafeRelease(&renderTarget);
	SafeRelease(&cellBrush);
	SafeRelease(&gridBrush);
	SafeRelease(&playerBrush);
	SafeRelease(&pathBrush);
	SafeRelease(&infoBrush);
	SafeRelease(&whiteBrush);
}

void Renderer::Resize(UINT width, UINT height) {
	if (renderTarget) {
		renderTarget->Resize(D2D1::SizeU(width, height));
	}
}

double Renderer::CastRay(double x, double y, double direction) {
	double xComponent = cos(direction);
	double yComponent = sin(direction);


	if (xComponent != 0 && yComponent != 0) {
		double slope = yComponent / xComponent;

		double minimum = LONG_MAX;

		if (yComponent > 0) {
			for (int i = (int)y + 1; i <= maze->height; i++) {
				double x_0 = (i - y) / slope + x;

				D2D1_ELLIPSE wallPoint{};
				wallPoint.point = D2D1::Point2F(x_0 * (cellSize + gridThickness), i * (cellSize + gridThickness));
				wallPoint.radiusX = 5;
				wallPoint.radiusY = 5;

				double dist = (x_0 - x) * (x_0 - x) + (y - i) * (y - i);

				if (!maze->CellCheck((int)x_0, i, maze->PathMask) && minimum > dist) minimum = dist;
			}
		}
		else {
			for (int i = (int)y; i >= 0; i--) {
				double x_0 = (i - y) / slope + x;

				D2D1_ELLIPSE wallPoint{};
				wallPoint.point = D2D1::Point2F(x_0 * (cellSize + gridThickness), i * (cellSize + gridThickness));
				wallPoint.radiusX = 5;
				wallPoint.radiusY = 5;

				double dist = (x_0 - x) * (x_0 - x) + (y - i) * (y - i);

				if ((!maze->CellCheck((int)x_0, i - 1, maze->PathMask) || i == 0) && minimum > dist) minimum = dist;
			}
		}

		if (xComponent > 0) {
			for (int i = (int)x + 1; i <= maze->width; i++) {
				double y_0 = (i - x) * slope + y;

				D2D1_ELLIPSE wallPoint{};
				wallPoint.point = D2D1::Point2F(i * (cellSize + gridThickness), y_0 * (cellSize + gridThickness));
				wallPoint.radiusX = 5;
				wallPoint.radiusY = 5;

				double dist = (i - x) * (i - x) + (y - y_0) * (y - y_0);

				if (!maze->CellCheck(i, (int)y_0, maze->PathMask) && minimum > dist) minimum = (i - x) * (i - x) + (y - y_0) * (y - y_0);
			}
		}
		else {
			for (int i = (int)x; i >= 0; i--) {
				double y_0 = (i - x) * slope + y;

				D2D1_ELLIPSE wallPoint{};
				wallPoint.point = D2D1::Point2F(i * (cellSize + gridThickness), y_0 * (cellSize + gridThickness));
				wallPoint.radiusX = 5;
				wallPoint.radiusY = 5;

				double dist = (i - x) * (i - x) + (y - y_0) * (y - y_0);

				if ((!maze->CellCheck(i - 1, (int)y_0, maze->PathMask) || i == 0) && minimum > dist) minimum = (i - x) * (i - x) + (y - y_0) * (y - y_0);
			}
		}

		return sqrt(minimum);
	}
	else return 0;
}

HRESULT Renderer::Render() {
	HRESULT hr = S_OK;

	hr = CreateDeviceResources();

	if (SUCCEEDED(hr)) {
		renderTarget->BeginDraw();

		renderTarget->Clear(D2D1::ColorF(D2D1::ColorF::Black));

		const float width = renderTarget->GetSize().width;
		const float height = renderTarget->GetSize().height;

		if (renderMode == 0 || renderMode == 1) {
			if (renderMode == 0) {
				D2D1_RECT_F rect;
				rect.left = (cellSize + gridThickness) * (maze->width - 1);
				rect.right = (cellSize + gridThickness) * maze->width - gridThickness;
				rect.top = (cellSize + gridThickness) * (maze->height - 1);
				rect.bottom = (cellSize + gridThickness) * maze->height - gridThickness;
				renderTarget->DrawRectangle(rect, whiteBrush, gridThickness);
			}

			for (int i = 0; i < maze->height; i++) {
				for (int j = 0; j < maze->width; j++) {
					D2D1_RECT_F rect;
					rect.left = (cellSize + gridThickness) * j;
					rect.right = (cellSize + gridThickness) * (j + 1);
					rect.top = (cellSize + gridThickness) * i;
					rect.bottom = (cellSize + gridThickness) * (i + 1);
					if (j == (int)maze->x && i == (int)maze->y && renderMode == 0) renderTarget->FillRectangle(rect, playerBrush);
					else if (showPath && maze->CellCheck(j, i, Maze::TruePathMask)) renderTarget->FillRectangle(rect, pathBrush);
					if (!maze->CellCheck(j, i, Maze::PathMask) && renderMode == 0) renderTarget->FillRectangle(rect, cellBrush);
				}
			}

			if (renderMode == 0) {
				for (int i = 0; i < maze->width; i++) {
					D2D1_RECT_F rect;
					rect.left = (cellSize + gridThickness) * (i + 1) - gridThickness;
					rect.right = (cellSize + gridThickness) * (i + 1);
					rect.top = 0;
					rect.bottom = height - (infoStrip ? 98 : 0);
					renderTarget->FillRectangle(rect, gridBrush);
				}

				for (int i = 0; i < maze->height; i++) {
					D2D1_RECT_F rect;
					rect.left = 0;
					rect.right = width;
					rect.top = (cellSize + gridThickness) * (i + 1) - gridThickness;
					rect.bottom = (cellSize + gridThickness) * (i + 1);
					renderTarget->FillRectangle(rect, gridBrush);
				}
			}
			else {
				D2D1_ELLIPSE point{};
				point.point = D2D1::Point2F(maze->x * (cellSize + gridThickness), maze->y * (cellSize + gridThickness));
				point.radiusX = cellSize / 10.0f;
				point.radiusY = cellSize / 10.0f;
				renderTarget->FillEllipse(point, playerBrush);

				for (double i = maze->GetPlayerDirection() - PI / 4; i < maze->GetPlayerDirection() + PI / 4 - PI / (2 * width); i += PI / (4 * width)) {
					double dist = CastRay(maze->x, maze->y, i);

					D2D1_ELLIPSE wallPoint{};
					wallPoint.point = D2D1::Point2F(point.point.x + cos(i) * dist * (cellSize + gridThickness), point.point.y + sin(i) * dist * (cellSize + gridThickness));
					wallPoint.radiusX = 1;
					wallPoint.radiusY = 1;

					if (wallPoint.point.x >= width - 1) wallPoint.point.x--;

					D2D1_POINT_2F wallPointNotScaled = D2D1::Point2F(maze->x + cos(i) * dist, maze->y + sin(i) * dist);

					ID2D1SolidColorBrush* brush;
					hr = renderTarget->CreateSolidColorBrush(D2D1::ColorF(
						(wallPointNotScaled.x >= maze->width - 1) && (wallPointNotScaled.y >= maze->height - 1) ? 1 : 0,
						1, (wallPointNotScaled.x >= maze->width - 1) && (wallPointNotScaled.y >= maze->height - 1) ? 1 : 0,
						1 - min(dist * cos(i - maze->GetPlayerDirection()), cameraRange) / cameraRange), &brush);

					renderTarget->DrawLine(point.point, wallPoint.point, whiteBrush, 0.01f);
					if (brush) renderTarget->FillEllipse(wallPoint, brush);
				}
			}
		}
		else {
			int j = 0;

			for (double i = maze->GetPlayerDirection() - PI / 4; i < maze->GetPlayerDirection() + PI / 4 - PI / (2 * width); i += PI / (2 * width)) {
				double dist = CastRay(maze->x, maze->y, i);

				D2D1_POINT_2F point = D2D1::Point2F(maze->x + cos(i) * dist, maze->y + sin(i) * dist);

				ID2D1SolidColorBrush* brush;
				hr = renderTarget->CreateSolidColorBrush(D2D1::ColorF(
					(point.x >= maze->width - 1) && (point.y >= maze->height - 1) ? 1 : (((int)(wallFrequency * point.x) + (int)(wallFrequency * point.y)) % 2) / 1.5,
					1, (point.x >= maze->width - 1) && (point.y >= maze->height - 1) ? 1 : 0, 
					1 - min(dist * cos(i - maze->GetPlayerDirection()), cameraRange) / cameraRange), &brush);

				if(brush) renderTarget->DrawLine(D2D1::Point2F(j, 0), D2D1::Point2F(j, height), brush, 1.0f);

				j++;
			}
		}

		if (infoStrip) {
			D2D1_RECT_F rectangle;
			rectangle.left = 0;
			rectangle.right = width;
			rectangle.bottom = height;
			rectangle.top = rectangle.bottom - (renderMode != 2 ? 98 : 136);
			renderTarget->FillRectangle(rectangle, infoBrush);

			wstring out5 = L"C to highlight the path, H to switch the info strip";
			wstring out4 = L"W, A, S, D to move, Enter to generate a new maze";
			wstring out3 = L"Iteration count: " + to_wstring(maze->iterations) + L" [F, G to adjust]";
			wstring out2 = L"Maze size: " + to_wstring(maze->width) + L"x" + to_wstring(maze->height) + L" [+, -, =, _ to adjust]";
			wstring out1 = L"Maze mode: 2D view and gameplay [M to change]";
			if (renderMode == 1) out1 = L"Maze mode: 2D view, 3D gameplay [M to change]";
			else if (renderMode == 2) {
				out1 = L"Maze mode: 3D view and gameplay [M to change]";
				out5 = L"Camera range: " + to_wstring(cameraRange) + L" blocks [E, R to adjust]";
			}

			renderTarget->DrawText(out1.c_str(), out1.length(), textFormat, D2D1::RectF(2, rectangle.top, width, height), whiteBrush);
			renderTarget->DrawText(out2.c_str(), out2.length(), textFormat, D2D1::RectF(2, rectangle.top + 19, width, height), whiteBrush);
			renderTarget->DrawText(out3.c_str(), out3.length(), textFormat, D2D1::RectF(2, rectangle.top + 38, width, height), whiteBrush);
			renderTarget->DrawText(out4.c_str(), out4.length(), textFormat, D2D1::RectF(2, rectangle.top + 57, width, height), whiteBrush);
			renderTarget->DrawText(out5.c_str(), out5.length(), textFormat, D2D1::RectF(2, rectangle.top + 76, width, height), whiteBrush);

			if (renderMode == 2) {
				wstring out7 = L"H to switch the info strip";
				wstring out6 = L"Wall strip frequency: " + to_wstring(wallFrequency) + L" strips [T, Y to adjust]";
				renderTarget->DrawText(out6.c_str(), out6.length(), textFormat, D2D1::RectF(2, rectangle.top + 95, width, height), whiteBrush);
				renderTarget->DrawText(out7.c_str(), out7.length(), textFormat, D2D1::RectF(2, rectangle.top + 114, width, height), whiteBrush);
			}
		}
		
		renderTarget->EndDraw();

		if (hr == D2DERR_RECREATE_TARGET) {
			hr = S_OK;
			DiscardDeviceResources();
		}
	}

	return hr;
}