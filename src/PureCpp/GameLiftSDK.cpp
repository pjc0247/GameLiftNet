#include <stdio.h>

#include "GameLiftSDK.h"

#include <aws/gamelift/server/GameLiftServerAPI.h>

#pragma comment (lib, "aws-cpp-sdk-gamelift-server.lib")
#pragma comment (lib, "libprotobuf.lib")
#pragma comment (lib, "libprotobuf-lite.lib")
#pragma comment (lib, "libprotoc.lib")
#pragma comment (lib, "sioclient.lib")
#pragma comment (lib, "libboost_system-vc140-mt-1_62.lib")
#pragma comment (lib, "libboost_date_time-vc140-mt-1_62.lib")
#pragma comment (lib, "libboost_random-vc140-mt-1_62.lib")

static void(*onStartGameSession)(GameSession *);
static void(*onProcesTerminate)();
static bool(*onHealthCheck)();

void OnStartGameSession(Aws::GameLift::Server::Model::GameSession gameSession);
void OnProcessTerminate();
bool OnHealthCheck();

bool GL_Initialize(
	int listenPort,
	const char *stdoutPath, const char *stderrPath,
	void (*onStartGameSession)(GameSession *),
	void(*onProcesTerminate)(),
	bool(*onHealthCheck)()) {

	Aws::GameLift::Server::InitSDK();

	::onStartGameSession = onStartGameSession;
	::onProcesTerminate = onProcesTerminate;
	::onHealthCheck = onHealthCheck;
	
	std::vector<std::string> logPaths;
	logPaths.push_back(stdoutPath);
	logPaths.push_back(stderrPath);

	auto params = Aws::GameLift::Server::ProcessParameters(
		OnStartGameSession,
		OnProcessTerminate,
		OnHealthCheck,
		listenPort, Aws::GameLift::Server::LogParameters(logPaths));
	auto ret = Aws::GameLift::Server::ProcessReady(params);

	return ret.IsSuccess();
}
void GL_ProcessEnding() {
	Aws::GameLift::Server::ProcessEnding();
}
void GL_Destroy() {
	Aws::GameLift::Server::Destroy();
}

bool GL_ActivateGameSession() {
	return Aws::GameLift::Server::ActivateGameSession().IsSuccess();
}
bool GL_TerminateGameSession() {
	return Aws::GameLift::Server::TerminateGameSession().IsSuccess();
}

bool GL_AcceptPlayerSession(const char *playerSessionId) {
	return Aws::GameLift::Server::AcceptPlayerSession(playerSessionId).IsSuccess();
}
bool GL_RemovePlayerSession(const char *playerSessionId) {
	return Aws::GameLift::Server::RemovePlayerSession(playerSessionId).IsSuccess();
}

void OnStartGameSession(Aws::GameLift::Server::Model::GameSession gameSession) {
	if (onStartGameSession != nullptr)
	{
		GameSession cppGameSession;

		strcpy_s(cppGameSession.name, gameSession.GetName().c_str());

		onStartGameSession(&cppGameSession);
	}
}
void OnProcessTerminate() {
	if (onProcesTerminate != nullptr)
		onProcesTerminate();
}
bool OnHealthCheck() {
	if (onHealthCheck != nullptr)
		return onHealthCheck();

	return false;
}