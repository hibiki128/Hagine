#include"d3dx12.h"
#include "MyGame.h"

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	std::unique_ptr<Framework> game = std::make_unique<MyGame>();

	game->Run();

	return 0;
}
