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
#include <aws/gamelift/internal/GameLiftServerState.h>
#include <aws/gamelift/server/ProcessParameters.h>
#include <sdk.pb.h>
#include <fstream>
#include <iostream>

using namespace Aws::GameLift;

#define HEALTHCHECK_TIMEOUT_SECONDS 60

Internal::GameLiftServerState::GameLiftServerState()
    : m_onStartGameSession(nullptr)
    , m_onProcessTerminate(nullptr)
    , m_onHealthCheck(nullptr)
    , m_processReady(false)
    , m_network(nullptr)
{
}


Internal::GameLiftServerState::~GameLiftServerState()
{
    Internal::GameLiftCommonState::SetInstance(nullptr);
    m_onStartGameSession = nullptr;
    m_onProcessTerminate = nullptr;
    m_onHealthCheck = nullptr;
    m_processReady = false;
    m_network = nullptr;
}

GenericOutcome Internal::GameLiftServerState::ProcessReady(const Aws::GameLift::Server::ProcessParameters &processParameters)
{
    m_processReady = true;

    m_onStartGameSession = processParameters.getOnStartGameSession();
    m_onProcessTerminate = processParameters.getOnProcessTerminate();

    if (processParameters.getOnHealthCheck() == nullptr)
    {
        m_onHealthCheck = std::bind(&Internal::GameLiftServerState::DefaultHealthCheck, this);
    }
    else
    {
        m_onHealthCheck = processParameters.getOnHealthCheck();
    }

    if (AssertNetworkInitialized())
    {
    return GenericOutcome(GameLiftError(GAMELIFT_ERROR_TYPE::GAMELIFT_SERVER_NOT_INITIALIZED));
    }

    GenericOutcome result = m_network->getAuxProxySender()->ProcessReady(processParameters.getPort(), processParameters.getLogParameters());

    std::thread healthCheck(std::bind(&Internal::GameLiftServerState::HealthCheck, this));
    healthCheck.detach();

    return result;
}

void Internal::GameLiftServerState::HealthCheck()
{
    while (m_processReady)
    {
        std::async(std::launch::async, &Internal::GameLiftServerState::ReportHealth, this);
        std::this_thread::sleep_for(std::chrono::seconds(HEALTHCHECK_TIMEOUT_SECONDS));
    }
}

void Internal::GameLiftServerState::ReportHealth()
{
    std::future<bool> future = std::async(std::launch::async, m_onHealthCheck);

    std::chrono::system_clock::time_point timeoutSeconds = std::chrono::system_clock::now() + std::chrono::seconds(HEALTHCHECK_TIMEOUT_SECONDS);

    // wait_until blocks until timeoutSeconds has been reached or the result becomes available
    if (std::future_status::ready == future.wait_until(timeoutSeconds))
    {
        m_network->getAuxProxySender()->ReportHealth(future.get());
    }
    else
    {
        m_network->getAuxProxySender()->ReportHealth(false);
    }
}

::GenericOutcome Internal::GameLiftServerState::ProcessEnding()
{
    m_processReady = false;

    if (AssertNetworkInitialized())
    {
        return GenericOutcome(GameLiftError(GAMELIFT_ERROR_TYPE::GAMELIFT_SERVER_NOT_INITIALIZED));
    }
    m_network->getAuxProxySender()->ProcessEnding();
    return GenericOutcome(nullptr);
}


std::string Internal::GameLiftServerState::GetGameSessionId()
{
    return m_gameSessionId;
}


GenericOutcome Internal::GameLiftServerState::InitializeNetworking()
{
    using namespace Aws::GameLift::Internal;

    //Sdk <-> AuxProxy communication
    sio::client* sioClient = new sio::client;
    Network::AuxProxyMessageSender* sender = new Network::AuxProxyMessageSender(sioClient);
    m_network = new Network::Network(sioClient, this, sender);

    return GenericOutcome(nullptr);
}

GenericOutcome Internal::GameLiftServerState::ActivateGameSession()
{
    if (!m_processReady)
    {
        return GenericOutcome(GameLiftError(GAMELIFT_ERROR_TYPE::PROCESS_NOT_READY));
    }

    if (AssertNetworkInitialized())
    {
        return GenericOutcome(GameLiftError(GAMELIFT_ERROR_TYPE::GAMELIFT_SERVER_NOT_INITIALIZED));
    }
    m_network->getAuxProxySender()->ActivateGameSession(m_gameSessionId);
    return GenericOutcome(nullptr);
}


GenericOutcome Internal::GameLiftServerState::TerminateGameSession()
{
    if (AssertNetworkInitialized())
    {
        return GenericOutcome(GameLiftError(GAMELIFT_ERROR_TYPE::GAMELIFT_SERVER_NOT_INITIALIZED));
    }
    m_network->getAuxProxySender()->TerminateGameSession(m_gameSessionId);
    return GenericOutcome(nullptr);
}

GenericOutcome Internal::GameLiftServerState::UpdatePlayerSessionCreationPolicy(Aws::GameLift::Server::Model::PlayerSessionCreationPolicy newPlayerSessionPolicy)
{
    std::string newPlayerSessionPolicyInString = Aws::GameLift::Server::Model::PlayerSessionCreationPolicyMapper::GetNameForPlayerSessionCreationPolicy(newPlayerSessionPolicy);

    if (AssertNetworkInitialized())
    {
        return GenericOutcome(GameLiftError(GAMELIFT_ERROR_TYPE::GAMELIFT_SERVER_NOT_INITIALIZED));
    }
    m_network->getAuxProxySender()->UpdatePlayerSessionCreationPolicy(m_gameSessionId.c_str(), newPlayerSessionPolicyInString.c_str());
    return GenericOutcome(nullptr);
}

Server::InitSDKOutcome
Internal::GameLiftServerState::CreateInstance()
{
    if (GameLiftCommonState::GetInstance().IsSuccess())
    {
        return Server::InitSDKOutcome(GameLiftError(GAMELIFT_ERROR_TYPE::ALREADY_INITIALIZED));
    }
    GameLiftServerState* newState = new GameLiftServerState;
    GenericOutcome setOutcome = GameLiftCommonState::SetInstance(newState);
    if (!setOutcome.IsSuccess())
    {
        delete newState;
        return Server::InitSDKOutcome(setOutcome.GetError());
    }
    return newState;
}


GenericOutcome Internal::GameLiftServerState::AcceptPlayerSession(const std::string& playerSessionId)
{
    if (AssertNetworkInitialized())
    {
        return GenericOutcome(GameLiftError(GAMELIFT_ERROR_TYPE::GAMELIFT_SERVER_NOT_INITIALIZED));
    }
    return m_network->getAuxProxySender()->AcceptPlayerSession(playerSessionId, m_gameSessionId);
}


GenericOutcome Internal::GameLiftServerState::RemovePlayerSession(const std::string& playerSessionId)
{
    if (AssertNetworkInitialized())
    {
        return GenericOutcome(GameLiftError(GAMELIFT_ERROR_TYPE::GAMELIFT_SERVER_NOT_INITIALIZED));
    }
    return m_network->getAuxProxySender()->RemovePlayerSession(playerSessionId, m_gameSessionId);
}


void Internal::GameLiftServerState::OnStartGameSession(Aws::GameLift::Server::Model::GameSession& gameSession, sio::message::list& ack_resp)
{
    std::string gameSessionId = gameSession.GetGameSessionId();
    if (!m_processReady)
    {
        ack_resp.insert(0, sio::bool_message::create(false));
        return;
    }

    m_gameSessionId = gameSessionId;

    //Invoking OnStartGameSession callback specified by the developer.
    std::thread activateGameSession(std::bind(m_onStartGameSession, gameSession));
    activateGameSession.detach();
    ack_resp.insert(0, sio::bool_message::create(true));
}


void Internal::GameLiftServerState::OnTerminateProcess()
{
    std::thread terminateProcess(std::bind(m_onProcessTerminate));
    terminateProcess.detach();
}


bool Internal::GameLiftServerState::AssertNetworkInitialized()
{
    return !m_network || !m_network->getAuxProxySender();
}
