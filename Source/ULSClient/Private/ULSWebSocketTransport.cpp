// Fill out your copyright notice in the Description page of Project Settings.


#include "ULSWebSocketTransport.h"
#include "ULSClientNetworkOwner.h"
#include "ULSWirePacket.h"

UULSWebSocketTransport::~UULSWebSocketTransport()
{
    this->Disconnect();
}

void UULSWebSocketTransport::BeginDestroy()
{
    Super::BeginDestroy();

    Disconnect();
}

void UULSWebSocketTransport::SetConnectionData(FString ip, int32 port, FString resource, FString protocol)
{
	_ip = ip;
	_port = port;
	_resource = resource;
	_protocol = protocol;
}

bool UULSWebSocketTransport::Connect()
{
    FString serverUrl = FString::Printf(TEXT("%s://%s:%d/%s"), *_protocol, *_ip, _port, *_resource);

    UE_LOG(LogTemp, Display, TEXT("UWebSocketConnection::Connect to %s"), *serverUrl);

    if (!FModuleManager::Get().IsModuleLoaded("WebSockets"))
    {
        FModuleManager::Get().LoadModule("WebSockets");
    }

    auto WebSocketModule = &FWebSocketsModule::Get();
    _webSocket = WebSocketModule->CreateWebSocket(serverUrl, *_protocol);

    // We bind all available events
    OnConnectedHandle = _webSocket->OnConnected().AddLambda([this]() -> void {
        AsyncTask(ENamedThreads::GameThread, [this]()
            {
                //UE_LOG(LogTemp, Display, TEXT("OnConnected"));
                FString empty = FString();
                this->ClientNetworkOwner->OnConnected(true, empty);
            });
        });

    OnConnectionErrorHandle = _webSocket->OnConnectionError().AddLambda([this](const FString& Error) -> void {
        // This code will run if the connection failed. Check Error to see what happened.
        AsyncTask(ENamedThreads::GameThread, [this, Error]()
            {
                //UE_LOG(LogTemp, Display, TEXT("OnConnectionError"));
                this->ClientNetworkOwner->OnConnected(false, Error);
            });
        });

    OnClosedHandle = _webSocket->OnClosed().AddLambda([this](int32 StatusCode, const FString& Reason, bool bWasClean) -> void {
        // This code will run when the connection to the server has been terminated.
        // Because of an error or a call to Socket->Close().
        //UE_LOG(LogTemp, Display, TEXT("OnClosed"));
        AsyncTask(ENamedThreads::GameThread, [this, StatusCode, Reason, bWasClean]()
            {
                //UE_LOG(LogTemp, Display, TEXT("Call OnDisconnected"));
                this->ClientNetworkOwner->OnDisconnected(StatusCode, Reason, bWasClean);
            });
        });

    OnMessageHandle = _webSocket->OnMessage().AddLambda([this](const FString& Message) -> void {
        // This code will run when we receive a string message from the server.
        AsyncTask(ENamedThreads::GameThread, [this, Message]()
            {
                //UE_LOG(LogTemp, Display, TEXT("OnMessage"));
            });
        });

    OnRawMessageHandle = _webSocket->OnRawMessage().AddLambda([this](const void* Data, SIZE_T Size, SIZE_T BytesRemaining) -> void {
        // This code will run when we receive a raw (binary) message from the server.
        //UE_LOG(LogTemp, Display, TEXT("OnRawMessage"));
        TArray<uint8> bytes = TArray<uint8>((uint8*)Data, Size);
        AsyncTask(ENamedThreads::GameThread, [this, bytes]()
            {
                //this->OnReceiveBytes(bytes);
                auto packet = NewObject<UULSWirePacket>();
                if (packet != nullptr)
                {
                    if (packet->ParseFromBytes(bytes) == false)
                    {
                        UE_LOG(LogTemp, Error, TEXT("Failed to parse WirePacket from bytes"));
                        return;
                    }

                    this->ClientNetworkOwner->HandleWirePacket(packet);
                }
            });
        });

    OnMessageSentHandle = _webSocket->OnMessageSent().AddLambda([this](const FString& MessageString) -> void {
        // This code is called after we sent a message to the server.
        AsyncTask(ENamedThreads::GameThread, [this, MessageString]()
            {
                //UE_LOG(LogTemp, Display, TEXT("OnMessageSent"));
            });
        });

    _webSocket->Connect();

    return true;
}

void UULSWebSocketTransport::Disconnect()
{
    UE_LOG(LogTemp, Display, TEXT("UWebSocketConnection::Disconnect"));

    if (_webSocket != nullptr)
    {
        _webSocket->OnConnected().Remove(OnConnectedHandle);
        _webSocket->OnConnectionError().Remove(OnConnectionErrorHandle);
        _webSocket->OnMessage().Remove(OnMessageHandle);
        _webSocket->OnRawMessage().Remove(OnRawMessageHandle);
        _webSocket->OnMessageSent().Remove(OnMessageSentHandle);
        _webSocket->OnClosed().Remove(OnClosedHandle);

        _webSocket->Close();
    }
    _webSocket = nullptr;
}

void UULSWebSocketTransport::SendWirePacket(const UULSWirePacket* packet)
{
    if (!IsConnected())
    {
        // Don't send if we're not connected.
        return;
    }

    if (IsValid(packet) == false)
    {
        UE_LOG(LogTemp, Error, TEXT("SendWirePacket: Failed to send packet. Packet is NULL."));
        return;
    }

    auto bytes = packet->SerializeToBytes();
    _webSocket->Send(bytes.GetData(), sizeof(uint8) * bytes.Num(), true);
}