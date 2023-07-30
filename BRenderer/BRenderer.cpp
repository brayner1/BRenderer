// VkRenderer.cpp : Defines the entry point for the application.
//
#include "BRenderer.h"

constexpr uint32_t SCREEN_WIDTH = 600;
constexpr uint32_t SCREEN_HEIGHT = 600;

int main(int argc, char* argv[])
{
	std::cout << "Hello CMake." << std::endl;

	brr::App application;
	application.Run();
	return 0;
}
/*
void App::Init()
{
	int rendererFlags, windowFlags;

	rendererFlags = SDL_RENDERER_ACCELERATED;

	windowFlags = 0;

	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		printf("Couldn't initialize SDL: %s\n", SDL_GetError());
		exit(1);
	}

	m_window = SDL_CreateWindow("Shooter 01", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, windowFlags);

	if (!m_window)
	{
		std::cout << "Failed to open " << SCREEN_WIDTH  << " x " <<  SCREEN_HEIGHT <<  " window: " << SDL_GetError() << std::endl;
		exit(1);
	}

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

	m_renderer = SDL_CreateRenderer(m_window, -1, rendererFlags);

	if (!m_renderer)
	{
		std::cout << "Failed to create renderer: " << SDL_GetError() << std::endl;
		exit(1);
	}
}
*/