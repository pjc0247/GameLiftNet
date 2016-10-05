// 기본 DLL 파일입니다.

#include "stdafx.h"

#include "GameLiftNet.h"

#include <GameLiftSDK.h>

#pragma comment (lib, "PureCpp.lib")

using namespace System;
using namespace System::Runtime::InteropServices;

namespace GameLiftNet {

	ref class CallbackStorage {
	public:
		static Server::StartGameSessionCallback ^onStartGameSession;
		static Server::ProcessTerminateCallback ^onProcessTerminate;
		static Server::HealthCheckCallback ^onHealthCheck;
	};

	static void OnStartGameSession(::GameSession *gameSession) {
		auto clrGameSession = gcnew GameSession();

		CallbackStorage::onStartGameSession(clrGameSession);
	}
	static void OnProcessTerminate()
	{
		CallbackStorage::onProcessTerminate();
	}
	static bool OnHealthCheck() {
		return CallbackStorage::onHealthCheck();
	}

	void Server::Initialize(
		int port,
		StartGameSessionCallback ^onStartGameSession,
		ProcessTerminateCallback ^onProcessTerminate,
		HealthCheckCallback ^onHealthCheck) {

		CallbackStorage::onStartGameSession = onStartGameSession;
		CallbackStorage::onProcessTerminate = onProcessTerminate;
		CallbackStorage::onHealthCheck = onHealthCheck;

		GL_Initialize(port,
			OnStartGameSession,
			OnProcessTerminate,
			OnHealthCheck);
	}
	void Server::ProcessEnding() {
		GL_ProcessEnding();
	}
	void Server::Destroy() {
		GL_Destroy();
	}

	void Server::ActivateGameSession() {
		GL_ActivateGameSession();
	}
	void Server::TerminateGameSession() {
		GL_TerminateGameSession();
	}

	bool Server::AcceptPlayerSession(String ^playerSessionId) {
		auto strPtr = (IntPtr)(Marshal::StringToHGlobalAnsi(playerSessionId)).ToPointer();
		auto ret = GL_AcceptPlayerSession((const char*)(void*)strPtr);
		Marshal::FreeHGlobal(strPtr);

		return ret;
	}
	bool Server::RemovePlayerSession(String ^playerSessionId) {
		auto strPtr = (IntPtr)(Marshal::StringToHGlobalAnsi(playerSessionId)).ToPointer();
		auto ret = GL_RemovePlayerSession((const char*)(void*)strPtr);
		Marshal::FreeHGlobal(strPtr);

		return ret;
	}
}
