// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "ULSTransport.generated.h"

/**
 * 
 */
UCLASS(BlueprintType, Abstract)
class ULSCLIENT_API UULSTransport : public UObject
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category = ULSTransport)
		virtual bool IsConnected() const;

	UFUNCTION(BlueprintCallable, Category = ULSTransport)
		virtual bool Connect();

	UFUNCTION(BlueprintCallable, Category = ULSTransport)
		virtual void Disconnect();

	UFUNCTION(BlueprintCallable, Category = ULSTransport)
		virtual void SendWirePacket(const UULSWirePacket* packet);

	UPROPERTY(BlueprintReadWrite)
		class UULSClientNetworkOwner* ClientNetworkOwner;
};
