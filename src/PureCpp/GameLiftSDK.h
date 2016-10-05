#pragma once

#ifdef GAMELIFT_NET_EXPORTS
#define GLAPI __declspec( dllexport )
#else
#define GLAPI __declspec( dllimport )
#endif

extern "C" {
	struct GameSession {
		char name[256];
	};

	GLAPI bool GL_Initialize(
		int listenPort,
		const char *stdoutPath, const char *stderrPath, 
		void (*onStartGameSession)(GameSession *),
		void (*onProcesTerminate)(),
		bool (*onHealthCheck)());
	GLAPI void GL_ProcessEnding();
	GLAPI void GL_Destroy();

	GLAPI bool GL_ActivateGameSession();
	GLAPI bool GL_TerminateGameSession();

	GLAPI bool GL_AcceptPlayerSession(const char *playerSessionId);
	GLAPI bool GL_RemovePlayerSession(const char *playerSessionId);
}