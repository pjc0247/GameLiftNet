using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

using GameLiftNet;

namespace Sample
{
    class Program
    {
        static void Main(string[] args)
        {
            Server.Initialize(9916,
                OnStartGameSession, OnProcessTerminate, OnHealthCheck);
        }

        static void OnStartGameSession(GameSession gameSession)
        {
            Server.ActivateGameSession();
        }
        static void OnProcessTerminate()
        {
            Server.TerminateGameSession();
            Server.ProcessEnding();
        }
        static bool OnHealthCheck()
        {
            return true;
        }
    }
}
