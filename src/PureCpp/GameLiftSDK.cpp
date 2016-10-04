#include <stdio.h>

#include "GameLiftSDK.h"

#include <aws/gamelift/server/GameLiftServerAPI.h>

#pragma comment (lib, "aws-cpp-sdk-gamelift-server.lib")

static void(*onStartGameSession)(GameSession *);
static void(*onProcesTerminate)();
static bool(*onHealthCheck)();

void OnStartGameSession(Aws::GameLift::Server::Model::GameSession gameSession);
void OnProcessTerminate();
bool OnHealthCheck();

bool GL_Initialize(
	int listenPort,
	void (*onStartGameSession)(GameSession *),
	void(*onProcesTerminate)(),
	bool(*onHealthCheck)()) {

	Aws::GameLift::Server::InitSDK();

	::onStartGameSession = onStartGameSession;
	::onProcesTerminate = onProcesTerminate;
	::onHealthCheck = onHealthCheck;

	// TODO : 경로 파라미터로 밧음
	std::string serverOut("logs\\serverOut.log");
	std::string serverErr("logs\\serverErr.log");
	std::vector<std::string> logPaths;
	logPaths.push_back(serverOut);
	logPaths.push_back(serverErr);

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

void GL_ActivateGameSession() {
	Aws::GameLift::Server::ActivateGameSession();
}
void GL_TerminateGameSession() {
	Aws::GameLift::Server::TerminateGameSession();
}

bool GL_AcceptPlayerSession(const char *playerSessionId) {
	return Aws::GameLift::Server::AcceptPlayerSession(playerSessionId).IsSuccess();
}
bool GL_RemovePlayerSession(const char *playerSessionId) {
	return Aws::GameLift::Server::RemovePlayerSession(playerSessionId).IsSuccess();
}

void OnStartGameSession(Aws::GameLift::Server::Model::GameSession gameSession) {
	if (onStartGameSession != nullptr)
		onStartGameSession(nullptr);
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