/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#include <aws/gamelift/server/GameLiftServerAPI.h>
#include <aws/gamelift/internal/network/Network.h>
#include <sdk.pb.h>
#include <google/protobuf/text_format.h>
#include <functional>

#define LOCALHOST "http://127.0.0.1:5757"

#ifdef WIN32
#define BIND_EVENT(IO,EV,FN) \
    do{ \
	    sio::socket::event_listener_aux l = FN; \
	    IO->on(EV, l); \
        } while (0)

#else
#define BIND_EVENT(IO,EV,FN) \
	IO->on(EV, FN)
#include <unistd.h>
#endif

using namespace Aws::GameLift::Server::Model;
using namespace Aws::GameLift::Internal::Network;

static const char* pid = "pID";
static const char* sdkVersion = "sdkVersion";

Network::Network(sio::client* sioClient, AuxProxyMessageHandler* handler, AuxProxyMessageSender* sender)
: m_sio_client(sioClient)
, m_handler(handler)
, m_sender(sender)
{
    m_sio_client->set_open_listener(std::bind(&Network::OnConnected, this));
    m_sio_client->set_fail_listener(std::bind(&Network::OnFail, this));
    m_sio_client->set_close_listener(std::bind(&Network::OnClose, this, std::placeholders::_1));
    m_sio_client->set_reconnect_attempts(3);
    
    std::map<std::string, std::string> map;
    map[sdkVersion] = Aws::GameLift::Server::GetSdkVersion().GetResult().c_str();
#ifdef WIN32
    map[pid] = std::to_string(::GetCurrentProcessId());
#else
    map[pid] = std::to_string(::getpid());
#endif
    m_sio_client->connect(LOCALHOST, map);

    //Connecting synchronously.
    m_lock.lock();
    if (!m_connect_finish)
    {
        m_cond.wait(m_lock);
    }
    m_lock.unlock();
}

Network::~Network()
{
    m_sio_client->close();
    m_sio_client = nullptr;
    m_handler = nullptr;
    m_sender = nullptr;
}

void Network::OnConnected()
{
    m_lock.lock();
    m_cond.notify_all();
    m_connect_finish = true;
    // AWS_LOG_INFO("[GameLift]", "Socket connected.");
    using std::placeholders::_1;
    using std::placeholders::_2;
    using std::placeholders::_3;
    using std::placeholders::_4;
    BIND_EVENT(m_sio_client->socket(), "StartGameSession", std::bind(&Network::OnStartGameSession, this, _1, _2, _3, _4));
    BIND_EVENT(m_sio_client->socket(), "TerminateProcess", std::bind(&Network::OnTerminateProcess, this, _1, _2, _3, _4));
    m_lock.unlock();
}

void Network::OnFail()
{
	// AWS_LOG_INFO("[GameLift]", "Socket connection failed");
	m_sio_client->socket()->off_all();
}

void Network::OnClose(sio::client::close_reason const& reason)
{
	// AWS_LOG_INFO("[GameLift]", "Socket connection closed with reason {}", reason);
	m_sio_client->socket()->off_all();
}

void Network::OnStartGameSession(std::string const& name, sio::message::ptr const& data, bool hasAck, sio::message::list &ack_resp)
{
	// AWS_LOG_INFO("[GameLift]", "Network::OnStartGameSession invoked.");
	//Expecting a buffered GameSession
	com::amazon::whitewater::auxproxy::pbuffer::ActivateGameSession activateGameSessionEvent;
	if (!google::protobuf::TextFormat::ParseFromString(data->get_string(), &activateGameSessionEvent))
	{
		// AWS_LOG_ERROR("[GameLift]", "Network::OnStartGameSession: Could not parse buffered game session.");
		return;
	}
	GameSession gameSession = ParseFromBufferedGameSession(activateGameSessionEvent.gamesession());
	//Call the handler
	m_handler->OnStartGameSession(gameSession, ack_resp);
}

void Network::OnTerminateProcess(std::string const& name, sio::message::ptr const& data, bool hasAck, sio::message::list &ack_resp)
{
	// AWS_LOG_INFO("[GameLift]", "Network::OnTerminateProcess invoked.");
	m_handler->OnTerminateProcess();
}

Aws::GameLift::Server::Model::GameSession Network::ParseFromBufferedGameSession(com::amazon::whitewater::auxproxy::pbuffer::GameSession bGameSession)
{
    GameSession gameSession;
	if (bGameSession.has_fleetid()){
		gameSession.SetFleetId(bGameSession.fleetid().c_str());
	}
	if (bGameSession.has_gamesessionid()){
        gameSession.SetGameSessionId(bGameSession.gamesessionid().c_str());
	}
	if (bGameSession.has_maxplayers()){
        gameSession.SetMaximumPlayerSessionCount(bGameSession.maxplayers());
	}
	if (bGameSession.has_name()){
        gameSession.SetName(bGameSession.name().c_str());
	}

	std::vector<GameProperty> gameProperties;
	for (int i = 0; i < bGameSession.gameproperties_size(); i++)
	{
		GameProperty gameProperty;
		auto property = bGameSession.gameproperties(i);
		if (property.has_key()){
			gameProperty.SetKey(property.key().c_str());
		}
		if (property.has_value()){
			gameProperty.SetValue(property.value().c_str());
		}

		gameProperties.push_back(gameProperty);
	}

    gameSession.SetGameProperties(gameProperties);

	return gameSession;
}

