// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ULSFunctionLibrary.generated.h"

/**
 * 
 */
UCLASS()
class ULSCLIENT_API UULSFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category = "ULS|Utility")
		static int32 GetPacketTypeByName(const FString& str);

	UFUNCTION(BlueprintCallable, Category = "ULS|Utility")
		static FString GetPacketNameByType(int32 type);
};
