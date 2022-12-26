#include "WebServer.h"

int main()
{
	WebServer webServer("127.0.0.1", 8000);
	if (webServer.init() != 0)
		return 0;

	webServer.run();

	//system("pause");
	return 0;
}