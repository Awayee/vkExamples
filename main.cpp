#include <iostream>
#include "vkapp.h"


void TestVk() {
	vkapp::VkApp app;
	app.Init();
	app.Loop();
	app.Clear();
}


int main() {
	TestVk();
	system("pause");
	return 0;
}