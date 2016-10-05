using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Net;
using System.Net.Sockets;

using GameLiftNet;

namespace Sample
{
    class Program
    {
        static readonly int Port = 9916;
        static TcpListener tcpListener;

        static void Main(string[] args)
        {
            new Thread(AcceptThread).Start();

            Server.Initialize(Port,
                "stdout.log", "stderr.log",
                OnStartGameSession, OnProcessTerminate, OnHealthCheck);

            while (true)
            {
                Console.Read();
            }
        }

        static void AcceptThread()
        {
            tcpListener = new TcpListener(Port);

            tcpListener.Start();

            while (true)
            {
                var client = tcpListener.AcceptTcpClient();

                new Thread(ClientThread).Start(client);
            }
        }
        static void ClientThread(object _client)
        {
            var client = _client as TcpClient;
            var playerSessionId = "";

            while(true)
            {
                try
                {
                    byte[] length = new byte[1] { 0x0 };

                    client.GetStream().Read(length, 0, 1);
                }
                catch(Exception e)
                {
                    break;
                }
            }

            if (string.IsNullOrEmpty(playerSessionId) == false)
                Server.RemovePlayerSession(playerSessionId);
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
