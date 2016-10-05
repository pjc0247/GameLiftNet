#pragma once

using namespace System;

namespace GameLiftNet {
	public ref class GameSession
	{
		/* TODO : 필드 채우기 */
	};

	public ref class Server
	{
	public:
		delegate void StartGameSessionCallback(GameSession ^);
		delegate void ProcessTerminateCallback();
		delegate bool HealthCheckCallback();

		static void Initialize(
			int port,
			String ^stdoutPath, String ^stderrPath,
			StartGameSessionCallback ^onStartGameSession,
			ProcessTerminateCallback ^onProcessTerminate,
			HealthCheckCallback ^onHealthCheck);
		static void ProcessEnding();
		static void Destroy();

		static void ActivateGameSession();
		static void TerminateGameSession();

		static bool AcceptPlayerSession(String ^playerSessionId);
		static bool RemovePlayerSession(String ^playerSessionId);

	public:
		static Action<GameSession^> ^onStartGameSession;
	};
}
