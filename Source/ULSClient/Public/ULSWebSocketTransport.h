// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WebSocketsModule.h" // Module definition
#include "IWebSocket.h"       // Socket definition
#include "ULSTransport.h"
#include "ULSWebSocketTransport.generated.h"

/**
 * 
 */
UCLASS(Blueprintable)
class ULSCLIENT_API UULSWebSocketTransport : public UULSTransport
{
	GENERATED_BODY()
	
public:
	~UULSWebSocketTransport();

protected:
	virtual void BeginDestroy() override;

	UFUNCTION(BlueprintCallable, Category = ULSWebSocketTransport)
		void SetConnectionData(FString ip, int32 port, FString resource, FString protocol);

	virtual bool IsConnected() const { return _webSocket != nullptr && _webSocket->IsConnected(); }

	virtual bool Connect();

	virtual void Disconnect();

	virtual void SendWirePacket(const UULSWirePacket* packet);

private:
	UPROPERTY()
		FString _ip;
	UPROPERTY()
		int32 _port;
	UPROPERTY()
		FString _resource;
	UPROPERTY()
		FString _protocol;

	TSharedPtr<IWebSocket> _webSocket;

	FDelegateHandle OnConnectedHandle;
	FDelegateHandle OnConnectionErrorHandle;
	FDelegateHandle OnClosedHandle;
	FDelegateHandle OnRawMessageHandle;
	FDelegateHandle OnMessageHandle;
	FDelegateHandle OnMessageSentHandle;
};
