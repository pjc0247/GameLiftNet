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

	class ManagedStringHolder {
	public:
		ManagedStringHolder(String ^managedString)
			: strPtr((Marshal::StringToHGlobalAnsi(managedString)).ToPointer())
		{
		}
		~ManagedStringHolder() {
			Marshal::FreeHGlobal((IntPtr)strPtr);
		}

		const char *getPtr() {
			return (char*)strPtr;
		}

	private:
		void *strPtr;
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
		String ^stdoutPath, String ^stderrPath,
		StartGameSessionCallback ^onStartGameSession,
		ProcessTerminateCallback ^onProcessTerminate,
		HealthCheckCallback ^onHealthCheck) {

		CallbackStorage::onStartGameSession = onStartGameSession;
		CallbackStorage::onProcessTerminate = onProcessTerminate;
		CallbackStorage::onHealthCheck = onHealthCheck;

		ManagedStringHolder cppStdoutPath(stdoutPath);
		ManagedStringHolder cppStderrPath(stderrPath);

		GL_Initialize(port,
			cppStdoutPath.getPtr(), cppStderrPath.getPtr(),
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
		ManagedStringHolder cppPlayerSessionId(playerSessionId);
		auto ret = GL_AcceptPlayerSession(cppPlayerSessionId.getPtr());
		return ret;
	}
	bool Server::RemovePlayerSession(String ^playerSessionId) {
		ManagedStringHolder cppPlayerSessionId(playerSessionId);
		auto ret = GL_RemovePlayerSession(cppPlayerSessionId.getPtr());
		return ret;
	}
}
