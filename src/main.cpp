/*
MIT License

Copyright (c) 2022 Ruslan Popov <ruslanpopov1512@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <iostream>
#include <chrono>
#include <vector>

#define SDL_MAIN_HANDLED
#include <SDL/SDL.h>
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl.h"
#include "imgui/imgui_impl_sdlrenderer.h"

#define SIM_SPACE_COL IM_COL32(50, 50, 50, 255)
#define SIM_MINUS_DOT_COL IM_COL32(0, 0, 255, 255)
#define SIM_TRANSP_MINUS_DOT_COL IM_COL32(0, 0, 255, 127)
#define SIM_PLUS_DOT_COL IM_COL32(255, 0, 0, 255)
#define SIM_TRANSP_PLUS_DOT_COL IM_COL32(255, 0, 0, 127)
#define SIM_NEUTRAL_DOT_COL IM_COL32(200, 200, 200, 255)
#define SIM_DOT_RADIUS 10
#define SIM_DOT_BORDER_COL IM_COL32(255, 255, 255, 255)
#define SIM_CONSTANT_ADJUST 0.5
#define SIM_DEFAULT_CONSTANT 1

using namespace std;

typedef struct {
	float value;
	
	ImVec2 position;
	ImVec2 velocity;
	ImVec2 acceleration;
} dot_t;
vector<dot_t> dots;

float simulationConstant = SIM_DEFAULT_CONSTANT;
bool isAddingNewDot = false;
bool isPlacingNewDot = false;
float newDotValue = 0;

SDL_Window* window;
SDL_Renderer* renderer;

bool running = false;
bool p_open = true;
ImGuiWindowFlags mainWindowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove
				| ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;
ImFont* titleFont;
ImFont* regularFont;
SDL_Event event;
ImVec2 mousePos;

chrono::steady_clock::time_point frameBegin, frameEnd;
unsigned msOnFrame;
const unsigned APP_FPS = 60;
const constexpr unsigned APP_FPS_MS = 1000 / APP_FPS;

inline bool ButtonRighted(const char* text);
inline void TextCentered(const char* text);
ImVec2 operator+(const ImVec2& a, const ImVec2& b);
ImVec2 operator-(const ImVec2& a);
ImVec2 operator-(const ImVec2& a, const ImVec2& b);
void operator/=(ImVec2& a, const ImVec2& b);
void operator*=(ImVec2& a, const float c);
void operator+=(ImVec2& a, const ImVec2& b);
void operator+=(ImVec2& a, const float c);
void operator-=(ImVec2& a, const ImVec2& b);

int main()
{
	cout << "Creating ImGui context..." << endl;
	ImGui::CreateContext();

	cout << "Initializing SDL..." << endl;
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
	{
		cout << "SDL_Init error: " << SDL_GetError() << endl;
		return 1;
	}

	cout << "Creating window..." << endl;
	window = SDL_CreateWindow("Coulomb's law", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		800, 600, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
	if (window == nullptr)
	{
		cout << "SDL_CreateWindow error: " << SDL_GetError() << endl;
		return 2;
	}

	cout << "Creating renderer..." << endl;
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	if (renderer == nullptr)
	{
		cout << "SDL_CreateRenderer error: " << SDL_GetError() << endl;
		return 3;
	}

	cout << "Initializing ImGui..." << endl;
	ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
	ImGui_ImplSDLRenderer_Init(renderer);

	// Temporary dot
	dots.push_back({ 0, ImVec2(-50,-50), ImVec2(0,0), ImVec2(0,0) });

	running = true;
	while (running)
	{
		frameBegin = chrono::high_resolution_clock::now();

		while (SDL_PollEvent(&event))
		{
			ImGui_ImplSDL2_ProcessEvent(&event);
			if (event.type == SDL_QUIT)
				running = false;
			if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE)
				running = false;
			if (event.type == SDL_MOUSEMOTION)
			{
				mousePos.x = (float) event.motion.x;
				mousePos.y = (float) event.motion.y;
			}
			if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT)
			{
				if (isAddingNewDot)
				{
					isAddingNewDot = false;
					isPlacingNewDot = true;
				}
			}
			if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_RIGHT)
			{
				if (isAddingNewDot)
				{
					isAddingNewDot = false;
					isPlacingNewDot = false;
				}
				else
				{
					for (vector<dot_t>::reverse_iterator i = dots.rbegin(); i != dots.rend(); i++)
					{
						ImVec2 diffVector = i->position - mousePos;
						float r = sqrt(diffVector.x * diffVector.x + diffVector.y * diffVector.y);
						if (r <= SIM_DOT_RADIUS)
						{
							dots.erase(next(i).base());
							break;
						}
					}
				}
			}
		}

		ImGui_ImplSDLRenderer_NewFrame();
		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();

		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->WorkPos);
		ImGui::SetNextWindowSize(viewport->WorkSize);
		ImGui::Begin("MainWindow", &p_open, mainWindowFlags);

		TextCentered("Attempt to simulate Coulomb's law");
		ImGui::SameLine();
		if (ButtonRighted("About"))
		{

		}
		
		ImGui::Text("F1 = ma; F2 = k*|q1|*|q2| / r^2"); ImGui::SameLine(400); ImGui::Text("a = constant * (|q1|*|q2|/r^2)");
		ImGui::Text("Assuming: F1 = F2 => a = (k*|q1|*|q2|) / (r^2 * m)"); ImGui::SameLine(400); ImGui::Text("constant = k / m");
		
		ImGui::Dummy(ImVec2(0, 2));

		ImGui::Text("constant =");
		ImGui::SameLine();
		ImGui::PushItemWidth(100);
		ImGui::InputFloat("##ConstantValue", &simulationConstant);
		ImGui::SameLine();
		if (ImGui::Button("+"))
		{
			simulationConstant += SIM_CONSTANT_ADJUST;
		}
		ImGui::SameLine();
		if (ImGui::Button("-"))
		{
			simulationConstant -= SIM_CONSTANT_ADJUST;
		}

		ImGui::SameLine(400);
		ImGui::Text("New dot value:");
		ImGui::SameLine();
		ImGui::PushItemWidth(100);
		ImGui::InputFloat("##NewDotValue", &newDotValue);
		ImGui::SameLine();
		if (ImGui::Button("Add"))
		{
			isAddingNewDot = true;
		}
		ImGui::SameLine();
		ImGui::Text("Dot count: %d", dots.size()-1);
		if (isAddingNewDot)
		{
			ImGui::SameLine();
			ImGui::Text(" adding");
		}

		ImGui::Separator();

		const ImVec2 simulationSpaceStart = ImGui::GetCursorPos();
		const ImVec2 simulationSpaceEnd = ImGui::GetWindowContentRegionMax();
		ImDrawList* drawList = ImGui::GetWindowDrawList();

		drawList->AddRectFilled(simulationSpaceStart, simulationSpaceEnd, SIM_SPACE_COL);

		if (isPlacingNewDot)
		{
			if (mousePos.x < simulationSpaceEnd.x && mousePos.y < simulationSpaceEnd.y &&
				mousePos.x > simulationSpaceStart.x && mousePos.y > simulationSpaceStart.y)
			{
				dots.push_back({ newDotValue, mousePos, ImVec2(0, 0), ImVec2(0, 0) });
			}
			isPlacingNewDot = false;
		}

		// Main calculations
		// 1. Calculate acceleration + create links
		for (unsigned i = 0; i < dots.size(); i++)
		{
			for (unsigned j = i + 1; j < dots.size(); j++)
			{
				ImVec2 diffVector = dots[j].position - dots[i].position;
				float r = sqrt(diffVector.x * diffVector.x + diffVector.y * diffVector.y);
				// Checking collision
				if (r <= 2*SIM_DOT_RADIUS)
				{
					dots[i].velocity = ImVec2(0,0);
					dots[j].velocity = ImVec2(0,0);
					continue;
				}
				float coulombsForce = simulationConstant * ((abs(dots[i].value) * abs(dots[j].value)) / r);
				bool isVecOrientMinus = dots[i].value > 0 && dots[j].value > 0 || dots[i].value < 0 && dots[j].value < 0;
				drawList->AddLine(dots[j].position, dots[i].position, IM_COL32(255, 255, 255, abs(coulombsForce) * 10000), 2);
				diffVector *= coulombsForce;
				diffVector /= ImGui::GetWindowContentRegionMax();
				if (isVecOrientMinus)
				{
					diffVector = -diffVector;
				}
				dots[i].acceleration += diffVector;
				dots[j].acceleration -= diffVector;

				// 2. Calculate velocity
				dots[i].velocity += dots[i].acceleration;
				dots[j].velocity += dots[j].acceleration;

				// 3. Calculate position
				dots[i].position += dots[i].velocity;
				dots[j].position += dots[j].velocity;
			}
		}

		// Drawing dots
		for (unsigned i = 0; i < dots.size(); i++)
		{
			ImVec2 limitMax = ImGui::GetWindowContentRegionMax() + ImVec2(200, 200);
			ImVec2 limitMin = ImGui::GetWindowContentRegionMin() - ImVec2(200, 200);
			if (dots[i].position.x < limitMin.x || dots[i].position.y < limitMin.y || 
				dots[i].position.x > limitMax.x || dots[i].position.y > limitMax.y)
			{
				dots.erase(dots.begin() + i);
			}
			else
			{
				if (dots[i].position.x < simulationSpaceEnd.x && dots[i].position.y < simulationSpaceEnd.y && 
					dots[i].position.x > simulationSpaceStart.x && dots[i].position.y > simulationSpaceStart.y)
				{
					drawList->AddCircleFilled(dots[i].position, SIM_DOT_RADIUS,
						dots[i].value > 0 ? SIM_PLUS_DOT_COL : (dots[i].value == 0 ? SIM_NEUTRAL_DOT_COL : SIM_MINUS_DOT_COL));
					//drawList->AddCircle(dots[i].position, SIM_DOT_RADIUS, SIM_DOT_BORDER_COL, 0, 2.5);
				}
			}
		}

		if (isAddingNewDot)
		{
			if (mousePos.x < simulationSpaceEnd.x && mousePos.y < simulationSpaceEnd.y &&
				mousePos.x > simulationSpaceStart.x && mousePos.y > simulationSpaceStart.y)
			{
				drawList->AddCircleFilled(mousePos, SIM_DOT_RADIUS,
					newDotValue > 0 ? SIM_PLUS_DOT_COL : (newDotValue == 0 ? SIM_NEUTRAL_DOT_COL : SIM_MINUS_DOT_COL));
			}
		}

		drawList->AddText(ImVec2(simulationSpaceStart.x+2, simulationSpaceEnd.y-13-2), IM_COL32_WHITE, "Help: \"Add\" button - adding mode on, press LMB to place dot. Press RMB to delete dot or exit adding mode");

		ImGui::End();
		ImGui::Render();
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
		SDL_RenderClear(renderer);
		ImGui_ImplSDLRenderer_RenderDrawData(ImGui::GetDrawData());
		SDL_RenderPresent(renderer);

		frameEnd = chrono::high_resolution_clock::now();

		msOnFrame = (unsigned) chrono::duration_cast<chrono::milliseconds>(frameEnd - frameBegin).count();
		if (msOnFrame < APP_FPS_MS)
		{
			SDL_Delay(APP_FPS_MS - msOnFrame);
		}
	}

	cout << "Goodbye." << endl;
	return 0;
}

inline bool ButtonRighted(const char* text) {
	auto windowWidth = ImGui::GetWindowSize().x;
	auto textWidth = ImGui::CalcTextSize(text).x;

	ImGui::SetCursorPosX(windowWidth - textWidth - 16);
	return ImGui::Button(text);
}

inline void TextCentered(const char* text) {
	auto windowWidth = ImGui::GetWindowSize().x;
	auto textWidth = ImGui::CalcTextSize(text).x;

	ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
	ImGui::Text(text);
}

ImVec2 operator+(const ImVec2& a, const ImVec2& b)
{
	return ImVec2(a.x + b.x, a.y + b.y);
}

ImVec2 operator-(const ImVec2& a)
{
	return ImVec2(-a.x, -a.y);
}

ImVec2 operator-(const ImVec2& a, const ImVec2& b)
{
	return a + (-b);
}

void operator*=(ImVec2& a, const float c)
{
	a.x *= c;
	a.y *= c;
}

void operator/=(ImVec2& a, const ImVec2& b)
{
	a.x /= b.x;
	a.y /= b.y;
}

void operator+=(ImVec2& a, const ImVec2& b)
{
	a.x += b.x;
	a.y += b.y;
}

void operator-=(ImVec2& a, const ImVec2& b)
{
	a += (-b);
}

void operator+=(ImVec2& a, const float c)
{
	a.x += c;
	a.y += c;
}
