// Fill out your copyright notice in the Description page of Project Settings.


#include "ULSClientNetworkOwner.h"
#include "ULSWirePacket.h"
#include "GameFramework/PlayerState.h"
#include "ULSTransport.h"
#include "Misc/OutputDeviceNull.h"

#define DEBUG_LOG 1
#define SERIALIZE_LOG 1 && DEBUG_LOG

void UULSClientNetworkOwner::HandleWirePacket(const UULSWirePacket* packet)
{
    if (packet != nullptr)
    {
        switch (packet->PacketType)
        {
			// Basic connection setup
			case EWirePacketType::ConnectionResponse:
				HandleConnectionResponseMessage(packet);
				break;

			case EWirePacketType::ConnectionEnd:
				HandleConnectionEndMessage(packet);
				break;

			// Runtime
			case EWirePacketType::Replication:
				HandleReplicationMessage(packet);
				break;

			case EWirePacketType::SpawnActor:
                HandleSpawnActorMessage(packet);
                break;

            case EWirePacketType::DespawnActor:
                HandleDespawnActorMessage(packet);
                break;

			case EWirePacketType::CreateObject:
				//HandleCreateObjectMessage(packet);
				break;

			case EWirePacketType::DestroyObject:
				//HandleDestroyObjectMessage(packet);
				break;

			case EWirePacketType::RpcCall:
				HandleRpcPacket(packet);
				break;

			case EWirePacketType::RpcCallResponse:
				HandleRpcResponsePacket(packet);
				break;

			// Custom packets
			case EWirePacketType::Custom:
				OnReceivePacket(packet);
				break;

            default:
                // Unhandled / undefined packet type
				// TODO: Add log output / error handling
                break;
        }
    }
}

void UULSClientNetworkOwner::OnConnected(bool success, const FString& errorMessage)
{
	UULSWirePacket* connectionRequestPacket = NewObject<UULSWirePacket>();
	BuildConnectionRequestPacket(connectionRequestPacket);
	Transport->SendWirePacket(connectionRequestPacket);
}

void UULSClientNetworkOwner::BuildConnectionRequestPacket(UULSWirePacket* packet)
{
	UWorld* world = GetWorld();
	if (IsValid(world))
	{
		APlayerController* playerController =
			(IsValid(world->GetFirstLocalPlayerFromController()) ? 
				world->GetFirstLocalPlayerFromController()->GetPlayerController(world) :
				nullptr);
		APlayerState* playerState = (IsValid(playerController) ? playerController->PlayerState : nullptr);
		if (IsValid(playerState))
		{
			auto netId = playerState->GetUniqueId().GetUniqueNetId();
			int position = 0;
			FString data = netId->ToString();
			packet->Payload.SetNumUninitialized(4 + data.Len());
			packet->PutString(data, position, position);
		}
	}
}

bool UULSClientNetworkOwner::ProcessConnectionResponsePacket(const UULSWirePacket* packet)
{
	int position = 0;
	int8 success = packet->ReadInt8(position, position);
	return success == 1;
}

void UULSClientNetworkOwner::OnDisconnected(int32 StatusCode, const FString& Reason, bool bWasClean)
{
	OnDisconnectionEvent.Broadcast(StatusCode, bWasClean);
}

void UULSClientNetworkOwner::HandleConnectionResponseMessage(const UULSWirePacket* packet)
{
	bool success = ProcessConnectionResponsePacket(packet);
	if (success)
	{
#if DEBUG_LOG
		UE_LOG(LogTemp, Display, TEXT("Login successful"));
#endif
	}
	else
	{
#if DEBUG_LOG
		UE_LOG(LogTemp, Display, TEXT("Login failed"));
#endif
	}

	OnConnectionEvent.Broadcast(success);
}

void UULSClientNetworkOwner::HandleConnectionEndMessage(const UULSWirePacket* packet)
{
	//
}

void UULSClientNetworkOwner::HandleRpcPacket(const UULSWirePacket* packet)
{
	int position = 0;
	int32 flags = packet->ReadInt32(position, position);
	int64 uniqueId = packet->ReadInt64(position, position);
	auto existingActor = FindActor(uniqueId);
	if (IsValid(existingActor) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("HandleRpcPacket failed: Actor with id %ld not found"), uniqueId);
		return;
	}
	FString methodName = packet->ReadString(position, position);
	FString returnType = packet->ReadString(position, position);
	int32 numberOfParameters = packet->ReadInt32(position, position);

#if SERIALIZE_LOG
	UE_LOG(LogTemp, Display, TEXT("*** HandleRpcPacket *** -- methodName: %s"), *methodName);
#endif
	if ((flags & (1 << 0)) == (1 << 0))
	{
		// FullReflection
		auto cls = existingActor->GetClass();
		UFunction* function = cls->FindFunctionByName(FName(methodName));
		if (IsValid(function) == false)
		{
			// TODO: Log properly
			UE_LOG(LogTemp, Error, TEXT("Failed to find function %s on actor of type %s with uniqueId: %ld"), *methodName, *cls->GetName(), FindUniqueId(existingActor));
			return;
		}
#if SERIALIZE_LOG
		UE_LOG(LogTemp, Display, TEXT("numberOfParameters: %i"), numberOfParameters);
#endif

		uint8* Parms = (uint8*)FMemory_Alloca_Aligned(function->ParmsSize, function->GetMinAlignment());
		FMemory::Memzero(Parms, function->ParmsSize);

		for (size_t i = 0; i < numberOfParameters; i++)
		{
			int8 type = packet->ReadInt8(position, position);
			FString fieldName = packet->ReadString(position, position);

			FProperty* prop = function->FindPropertyByName(FName(*fieldName));
			if (prop == nullptr)
			{
				// TODO: Log properly
				UE_LOG(LogTemp, Error, TEXT("Failed to find property %s on function %s::%s"), *fieldName, *cls->GetName(), *methodName);
				return;
			}

#if SERIALIZE_LOG
			UE_LOG(LogTemp, Display, TEXT("param #%i: type %i -- name: %s"), i, type, *fieldName);
#endif

			switch (type)
			{
			case EReplicatedFieldType::Reference:
			{
				// Ref
				auto actorRef = DeserializeRef(packet, position, position);
				if (IsValid(actorRef) == false)
				{
					// Set the reference to "null"
					FObjectProperty* objProp = (FObjectProperty*)prop;
					if (AActor** valuePtr = objProp->ContainerPtrToValuePtr<AActor*>(Parms))
					{
						if (valuePtr != nullptr)
						{
							*valuePtr = nullptr;
						}
					}
				}
				else
				{
					FObjectProperty* objProp = (FObjectProperty*)prop;
					if (AActor** valuePtr = objProp->ContainerPtrToValuePtr<AActor*>(Parms))
					{
						if (valuePtr != nullptr)
						{
							*valuePtr = actorRef;
						}
					}
					else
					{
						UE_LOG(LogTemp, Warning, TEXT("HandleRpcPacket: valuePtr failed"));
					}
				}
			}
			break;

			case EReplicatedFieldType::PrimitiveInt:
			{
				// Value
				int32 propSize = prop->GetSize();
				int32 size = packet->ReadInt32(position, position);

				int64 newVal = 0;
				if (size == 1)
				{
					newVal = DeserializeInt8(packet, position, position);
				}
				else if (size == 2)
				{
					newVal = DeserializeInt16(packet, position, position);
				}
				else if (size == 4)
				{
					newVal = DeserializeInt32(packet, position, position);
				}
				else if (size == 8)
				{
					newVal = DeserializeInt64(packet, position, position);
				}

				if (FIntProperty* intProp = CastField<FIntProperty>(prop))
				{
					if (int32* iVal = intProp->ContainerPtrToValuePtr<int32>(Parms))
					{
#if SERIALIZE_LOG
						UE_LOG(LogTemp, Display, TEXT("HandleRpcPacket: %s.%s = %i"), *methodName, *prop->GetName(), newVal);
#endif
						*iVal = (int32)newVal;
					}
				}
				else if (FInt16Property* int16Prop = CastField<FInt16Property>(prop))
				{
					if (int16* iVal = int16Prop->ContainerPtrToValuePtr<int16>(Parms))
					{
#if SERIALIZE_LOG
						UE_LOG(LogTemp, Display, TEXT("HandleRpcPacket: %s.%s = %ld"), *methodName, *prop->GetName(), newVal);
#endif
						*iVal = (int16)newVal;
					}
				}
				else if (FInt64Property* int64Prop = CastField<FInt64Property>(prop))
				{
					if (int64* iVal = int64Prop->ContainerPtrToValuePtr<int64>(Parms))
					{
#if SERIALIZE_LOG
						UE_LOG(LogTemp, Display, TEXT("HandleRpcPacket: %s.%s = %ld"), *methodName, *prop->GetName(), newVal);
#endif
						*iVal = (int64)newVal;
					}
				}
				else if (FBoolProperty* boolProp = CastField<FBoolProperty>(prop))
				{
					if (bool* bVal = boolProp->ContainerPtrToValuePtr<bool>(Parms))
					{
#if SERIALIZE_LOG
						UE_LOG(LogTemp, Display, TEXT("HandleRpcPacket: %s.%s = %s"), *methodName, *prop->GetName(), newVal ? TEXT("TRUE") : TEXT("FALSE"));
#endif
						*bVal = (bool)newVal;
					}
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("HandleRpcPacket: Unhandled property of type %s"), *prop->GetFullName());
				}
			}
			break;

			case EReplicatedFieldType::PrimitiveFloat:
			{
				// Value
				int32 propSize = prop->GetSize();
				int32 size = packet->ReadInt32(position, position);

				// Support upcasting
				double newVal = 0;
				if (size == 4)
				{
					newVal = DeserializeFloat32(packet, position, position);
				}
				else
				{
					newVal = DeserializeFloat64(packet, position, position);
				}

				if (FFloatProperty* floatProp = CastField<FFloatProperty>(prop))
				{
					if (float_t* fVal = floatProp->ContainerPtrToValuePtr<float_t>(Parms))
					{
#if SERIALIZE_LOG
						UE_LOG(LogTemp, Display, TEXT("HandleRpcPacket: %s.%s = %f"), *methodName, *prop->GetName(), newVal);
#endif
						*fVal = (float)newVal;
					}
				}
				else if (FDoubleProperty* doubleProp = CastField<FDoubleProperty>(prop))
				{
					if (double* dVal = doubleProp->ContainerPtrToValuePtr<double>(Parms))
					{
#if SERIALIZE_LOG
						UE_LOG(LogTemp, Display, TEXT("HandleRpcPacket: %s.%s = %f"), *methodName, *prop->GetName(), newVal);
#endif
						*dVal = newVal;
					}
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("HandleRpcPacket: Unhandled property of type %s"), *prop->GetFullName());
				}
			}
			break;

			case EReplicatedFieldType::String:
			{
				// String
				FString fieldValue = DeserializeString(packet, position, position);
				FStrProperty* strProp = (FStrProperty*)prop;
				if (FString* valuePtr = strProp->ContainerPtrToValuePtr<FString>(Parms))
				{
#if SERIALIZE_LOG
					UE_LOG(LogTemp, Display, TEXT("HandleRpcPacket: %s.%s = %s"), *methodName, *prop->GetName(), *fieldValue);
#endif
					*valuePtr = fieldValue;
				}
				else
				{
					UE_LOG(LogTemp, Error, TEXT("HandleRpcPacket: valuePtr failed"));
				}
			}
			break;

			case EReplicatedFieldType::Vector3:
			{
				// String
				FVector vec = DeserializeVector(packet, position, position);

				FProperty* vecProp = (FProperty*)prop;
				if (FVector* valuePtr = vecProp->ContainerPtrToValuePtr<FVector>(Parms))
				{
					if (valuePtr != nullptr)
					{
						*valuePtr = vec;
					}
				}
				else
				{
					UE_LOG(LogTemp, Error, TEXT("HandleRpcPacket: valuePtr failed"));
				}
			}
			break;
			}
		}

		//UE_LOG(LogTemp, Display, TEXT("Command: %s"), *command);
		//bool res = existingActor->CallFunctionByNameWithArguments(*command, outputDevice, nullptr, true);
		existingActor->ProcessEvent(function, Parms);
		//UE_LOG(LogTemp, Display, TEXT("Command: %s -> %s"), *command, (res ? TEXT("TRUE") : TEXT("FALSE")));
	}
	else
	{
		// Generated and partial reflection
		ProcessHandleRpcPacket(packet, position, existingActor, methodName, returnType, numberOfParameters);
	}
}

void UULSClientNetworkOwner::ProcessHandleRpcPacket(const UULSWirePacket* packet, int packetReadPosition, AActor* existingActor, const FString& methodName,
	const FString& returnType, const int32 numberOfParameters)
{
	// Base implementation does nothing
}

void UULSClientNetworkOwner::HandleRpcResponsePacket(const UULSWirePacket* packet)
{
    //UE_LOG(LogTemp, Display, TEXT("UWebSocketConnection::HandleRpHandleRpcResponsePacket"));
}

void UULSClientNetworkOwner::HandleSpawnActorMessage(const UULSWirePacket* packet)
{
	int position = 0;

	int32 flags = packet->ReadInt32(position, position);
	FString className = packet->ReadString(position, position);
	int64 uniqueId = packet->ReadInt64(position, position);

	// If there is no dot, add ".<object_name>_C"
	int32 PackageDelimPos = INDEX_NONE;
	className.FindChar(TCHAR('.'), PackageDelimPos);
	if (PackageDelimPos == INDEX_NONE)
	{
		int32 ObjectNameStart = INDEX_NONE;
		className.FindLastChar(TCHAR('/'), ObjectNameStart);
		if (ObjectNameStart != INDEX_NONE)
		{
			const FString ObjectName = className.Mid(ObjectNameStart + 1);
			className += TCHAR('.');
			className += ObjectName;
			className += TCHAR('_');
			className += TCHAR('C');
		}
	}

	//UE_LOG(LogTemp, Display, TEXT("HandleSpawnActorMessage: flags: %i"), flags);
	//UE_LOG(LogTemp, Display, TEXT("HandleSpawnActorMessage: className: %s"), *className);
	//UE_LOG(LogTemp, Display, TEXT("HandleSpawnActorMessage: uniqueId: %ld"), uniqueId);

	UClass* cls = FindObject<UClass>(nullptr, *className);
	if (IsValid(cls) == false)
	{
		cls = LoadObject<UClass>(nullptr, *className);
	}

	UE_LOG(LogTemp, Display, TEXT("HandleSpawnActorMessage: Spawn %s with network id: %ld"), *className, uniqueId);

	if (IsValid(cls))
	{
		//UE_LOG(LogTemp, Display, TEXT("HandleSpawnActorMessage: Class found: %s"), *cls->GetDescription());
		SpawnNetworkActor(uniqueId, cls);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("HandleSpawnActorMessage failed: Class '%s' not found"), *className);
	}
}

void UULSClientNetworkOwner::HandleDespawnActorMessage(const UULSWirePacket* packet)
{
	int position = 0;
	int32 flags = packet->ReadInt32(position, position);
	int64 uniqueId = packet->ReadInt64(position, position);

	auto actor = FindActor(uniqueId);
	if (IsValid(actor))
	{
		actor->Destroy();

		uniqueIdLookup.Remove(actor);
	}
	actorMap.Remove(uniqueId);
}

void UULSClientNetworkOwner::HandleReplicationMessage(const UULSWirePacket* packet)
{
	int position = 0;
	int32 flags = packet->ReadInt32(position, position);
	int64 uniqueId = packet->ReadInt64(position, position);
	auto existingActor = FindActor(uniqueId);
	if (IsValid(existingActor) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("HandleReplicationMessage failed: Actor with id %ld not found"), uniqueId);
		return;
	}

	int32 fieldCount = packet->ReadInt32(position, position);
	if (fieldCount == 0)
	{
		// Should not happen (server should not send empty packets)
		// Not an error regardless, just return
		return;
	}

	auto cls = existingActor->GetClass();
	for (size_t i = 0; i < fieldCount; i++)
	{
		int8 type = packet->ReadInt8(position, position);
		FString fieldName = packet->ReadString(position, position);

		auto prop = cls->FindPropertyByName(FName(*fieldName));

		if (prop == nullptr)
		{
			UE_LOG(LogTemp, Warning, TEXT("HandleReplicationMessage: prop not found: %s"), *fieldName);
			continue;
		}

		bool valueDidChange = false;

		switch (type)
		{
		case EReplicatedFieldType::Reference:
		{
			// Ref
			auto actorRef = DeserializeRef(packet, position, position);
			if (IsValid(actorRef) == false)
			{
				// Set the reference to "null"
				FObjectProperty* objProp = (FObjectProperty*)prop;
				if (AActor** valuePtr = objProp->ContainerPtrToValuePtr<AActor*>(existingActor))
				{
					if (*valuePtr != nullptr)
					{
#if SERIALIZE_LOG
						UE_LOG(LogTemp, Display, TEXT("HandleReplicationMessage: %s.%s = nullptr -- (%li)=(%li)"), *existingActor->GetName(), *prop->GetName(),
							uniqueId, -1);
#endif
						valueDidChange = true;
						*valuePtr = nullptr;
					}
				}
			}
			else
			{
				FObjectProperty* objProp = (FObjectProperty*)prop;
				if (AActor** valuePtr = objProp->ContainerPtrToValuePtr<AActor*>(existingActor))
				{
					if (*valuePtr != actorRef)
					{
#if SERIALIZE_LOG
						UE_LOG(LogTemp, Display, TEXT("HandleReplicationMessage: %s.%s = %s -- (%li)=(%li)"), *existingActor->GetName(), *prop->GetName(),
							*actorRef->GetName(), uniqueId, FindUniqueId(actorRef));
#endif
						valueDidChange = true;
						*valuePtr = actorRef;
					}
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("HandleReplicationMessage: valuePtr failed"));
				}
			}
		}
		break;

		case EReplicatedFieldType::PrimitiveInt:
		{
			// Value
			int32 propSize = prop->GetSize();
			int32 size = packet->ReadInt32(position, position);
			
			if (FIntProperty* intProp = CastField<FIntProperty>(prop))
			{
				if (int32* iVal = intProp->ContainerPtrToValuePtr<int32>(existingActor))
				{
					int32 newVal = DeserializeInt32(packet, position, position);
					if (*iVal != newVal)
					{
#if SERIALIZE_LOG
						UE_LOG(LogTemp, Display, TEXT("HandleReplicationMessage: %s.%s = %i"), *existingActor->GetName(), *prop->GetName(), newVal);
#endif
						valueDidChange = true;
						*iVal = newVal;
					}
				}
			}
			else if (FInt16Property* int16Prop = CastField<FInt16Property>(prop))
			{
				if (int16* iVal = int16Prop->ContainerPtrToValuePtr<int16>(existingActor))
				{
					int16 newVal = DeserializeInt16(packet, position, position);
					if (*iVal != newVal)
					{
#if SERIALIZE_LOG
						UE_LOG(LogTemp, Display, TEXT("HandleReplicationMessage: %s.%s = %ld"), *existingActor->GetName(), *prop->GetName(), newVal);
#endif
						valueDidChange = true;
						*iVal = newVal;
					}
				}
			}
			else if (FInt64Property* int64Prop = CastField<FInt64Property>(prop))
			{
				if (int64* iVal = int64Prop->ContainerPtrToValuePtr<int64>(existingActor))
				{
					int64 newVal = DeserializeInt64(packet, position, position);
					if (*iVal != newVal)
					{
#if SERIALIZE_LOG
						UE_LOG(LogTemp, Display, TEXT("HandleReplicationMessage: %s.%s = %ld"), *existingActor->GetName(), *prop->GetName(), newVal);
#endif
						valueDidChange = true;
						*iVal = newVal;
					}
				}
			}
			else if (FBoolProperty* boolProp = CastField<FBoolProperty>(prop))
			{
				if (bool* bVal = boolProp->ContainerPtrToValuePtr<bool>(existingActor))
				{
					bool newVal = DeserializeBool(packet, position, position, size);
					if (*bVal != newVal)
					{
#if SERIALIZE_LOG
						UE_LOG(LogTemp, Display, TEXT("HandleReplicationMessage: %s.%s = %s"), *existingActor->GetName(), *prop->GetName(), newVal ? TEXT("TRUE") : TEXT("FALSE"));
#endif
						valueDidChange = true;
						*bVal = newVal;
					}
				}
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("HandleReplicationMessage: Unhandled property of type %s"), *prop->GetFullName());
			}
		}
		break;

		case EReplicatedFieldType::PrimitiveFloat:
		{
			// Value
			int32 propSize = prop->GetSize();
			int32 size = packet->ReadInt32(position, position);

			if (FFloatProperty* floatProp = CastField<FFloatProperty>(prop))
			{
				if (float_t* fVal = floatProp->ContainerPtrToValuePtr<float_t>(existingActor))
				{
					float_t newVal = DeserializeFloat32(packet, position, position);
					if (*fVal != newVal)
					{
#if SERIALIZE_LOG
						UE_LOG(LogTemp, Display, TEXT("HandleReplicationMessage: %s.%s = %f"), *existingActor->GetName(), *prop->GetName(), newVal);
#endif
						valueDidChange = true;
						*fVal = newVal;
					}
				}
			}
			else if (FDoubleProperty* doubleProp = CastField<FDoubleProperty>(prop))
			{
				if (double* dVal = doubleProp->ContainerPtrToValuePtr<double>(existingActor))
				{
					double newVal = DeserializeFloat64(packet, position, position);
					if (*dVal != newVal)
					{
#if SERIALIZE_LOG
						UE_LOG(LogTemp, Display, TEXT("HandleReplicationMessage: %s.%s = %f"), *existingActor->GetName(), *prop->GetName(), newVal);
#endif
						valueDidChange = true;
						*dVal = newVal;
					}
				}
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("HandleReplicationMessage: Unhandled property of type %s"), *prop->GetFullName());
			}
		}
		break;

		case EReplicatedFieldType::String:
		{
			// String
			FString fieldValue = DeserializeString(packet, position, position);
			FStrProperty* strProp = (FStrProperty*)prop;
			if (FString* valuePtr = strProp->ContainerPtrToValuePtr<FString>(existingActor))
			{
				if (*valuePtr != fieldValue)
				{
#if SERIALIZE_LOG
					UE_LOG(LogTemp, Display, TEXT("HandleReplicationMessage: %s.%s = %s"), *existingActor->GetName(), *prop->GetName(), *fieldValue);
#endif
					valueDidChange = true;
					*valuePtr = fieldValue;
				}
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("HandleReplicationMessage: valuePtr failed"));
			}
		}
		break;

		case EReplicatedFieldType::Vector3:
		{
			// String
			FVector vec = DeserializeVector(packet, position, position);

			FProperty* vecProp = (FProperty*)prop;
			if (FVector* valuePtr = vecProp->ContainerPtrToValuePtr<FVector>(existingActor))
			{
				if (valuePtr != nullptr)
				{
					FVector val = *valuePtr;
					if (FMath::IsNearlyEqual(val.X, vec.X) == false ||
						FMath::IsNearlyEqual(val.Y, vec.Y) == false ||
						FMath::IsNearlyEqual(val.Z, vec.Z) == false)
					{
#if SERIALIZE_LOG
						UE_LOG(LogTemp, Display, TEXT("HandleReplicationMessage: %s.%s = %s"), *existingActor->GetName(), *prop->GetName(), *vec.ToString());
#endif
						valueDidChange = true;
						*valuePtr = vec;
					}
				}
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("HandleReplicationMessage: valuePtr failed"));
			}
		}
		break;
		}

		if (valueDidChange)
		{
			FString repFunctionName = TEXT("OnRep_") + fieldName;
			auto repFunction = cls->FindFunctionByName(FName(repFunctionName));
			if (IsValid(repFunction))
			{
				existingActor->ProcessEvent(repFunction, NULL);
			}
		}
	}
}

// Blueprint accessible function for finding actors by unique network ID
AActor* UULSClientNetworkOwner::FindActorByUniqueId(int64 uniqueId) const
{
	return FindActor(uniqueId);
}

AActor* UULSClientNetworkOwner::FindActor(int64 uniqueId) const
{
	if (uniqueId == -1)
	{
		return nullptr;
	}

	auto val = actorMap.Find(uniqueId);
	if (val == nullptr)
	{
		return nullptr;
	}
	return *val;
}

int64 UULSClientNetworkOwner::FindUniqueId(const AActor* actor) const
{
	if (IsValid(actor) == false)
	{
		return -1;
	}

	auto val = uniqueIdLookup.Find(actor);
	if (val == nullptr)
	{
		return -1;
	}
	return *val;
}

AActor* UULSClientNetworkOwner::SpawnNetworkActor(int64 uniqueId, UClass* cls)
{
	auto existingActor = FindActor(uniqueId);
	if (existingActor != nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("SpawnNetworkActor failed: Actor with id %ld already exists"), uniqueId);
		return nullptr;
	}

	UWorld* world = GetWorld();
	FTransform actorTransform = FTransform::Identity;
	auto actor = world->SpawnActor(cls, &actorTransform);
	auto networkActor = Cast<AActor>(actor);
	if (networkActor == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("SpawnNetworkActor failed: Class %s is not a subclass of AActor"), *cls->GetName());
		actor->Destroy();
		return nullptr;
	}
	actorMap.Add(uniqueId, networkActor);
	uniqueIdLookup.Add(networkActor, uniqueId);
	return networkActor;
}


AActor* UULSClientNetworkOwner::DeserializeRef(const UULSWirePacket* packet, int index, int& advancedPosition) const
{
	int64 uniqueId = packet->ReadInt64(index, advancedPosition);	
	if (uniqueId == -1)
	{
		return nullptr;
	}

	auto res = FindActor(uniqueId);
	if (IsValid(res) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("HandleReplicationMessage: Could not find actor with id %ld"), uniqueId);
	}
	return res;
}

int8 UULSClientNetworkOwner::DeserializeInt8(const UULSWirePacket* packet, int index, int& advancedPosition) const
{
	return packet->ReadInt8(index, advancedPosition);
}

int16 UULSClientNetworkOwner::DeserializeInt16(const UULSWirePacket* packet, int index, int& advancedPosition) const
{
	return packet->ReadInt16(index, advancedPosition);
}

int32 UULSClientNetworkOwner::DeserializeInt32(const UULSWirePacket* packet, int index, int& advancedPosition) const
{
	return packet->ReadInt32(index, advancedPosition);
}

int64 UULSClientNetworkOwner::DeserializeInt64(const UULSWirePacket* packet, int index, int& advancedPosition) const
{
	return packet->ReadInt64(index, advancedPosition);
}

float UULSClientNetworkOwner::DeserializeFloat32(const UULSWirePacket* packet, int index, int& advancedPosition) const
{
	return packet->ReadFloat32(index, advancedPosition);
}

double UULSClientNetworkOwner::DeserializeFloat64(const UULSWirePacket* packet, int index, int& advancedPosition) const
{
	return packet->ReadFloat64(index, advancedPosition);
}

bool UULSClientNetworkOwner::DeserializeBool(const UULSWirePacket* packet, int index, int& advancedPosition, int boolSize) const
{
	bool newVal = false;
	if (boolSize == 4)
	{
		newVal = packet->ReadInt32(index, advancedPosition) == 1;
	}
	else if (boolSize == 1)
	{
		newVal = packet->ReadInt8(index, advancedPosition) == 1;
	}
	else
	{
		// TODO: Handle
	}
	return newVal;
}

FString UULSClientNetworkOwner::DeserializeString(const UULSWirePacket* packet, int index, int& advancedPosition) const
{
	return packet->ReadString(index, advancedPosition);
}

FVector UULSClientNetworkOwner::DeserializeVector(const UULSWirePacket* packet, int index, int& advancedPosition) const
{
	return FVector(
		packet->ReadFloat32(index, advancedPosition),
		packet->ReadFloat32(advancedPosition, advancedPosition),
		packet->ReadFloat32(advancedPosition, advancedPosition)
	);
}

AActor* UULSClientNetworkOwner::DeserializeRefParameter(const UULSWirePacket* packet, int index, int& advancedPosition) const
{
	int8 type = packet->ReadInt8(index, advancedPosition);
	FString fieldName = packet->ReadString(advancedPosition, advancedPosition);
	// TODO: Validate type
	return DeserializeRef(packet, advancedPosition, advancedPosition);
}

// Deserialization
int16 UULSClientNetworkOwner::DeserializeInt16Parameter(const UULSWirePacket* packet, int index, int& advancedPosition) const
{
	int8 type = packet->ReadInt8(index, advancedPosition);
	FString fieldName = packet->ReadString(advancedPosition, advancedPosition);
	int32 size = packet->ReadInt32(advancedPosition, advancedPosition);
	// TODO: Validate type
	return DeserializeInt16(packet, advancedPosition, advancedPosition);
}

int32 UULSClientNetworkOwner::DeserializeInt32Parameter(const UULSWirePacket* packet, int index, int& advancedPosition) const
{
	int8 type = packet->ReadInt8(index, advancedPosition);
	FString fieldName = packet->ReadString(advancedPosition, advancedPosition);
	int32 size = packet->ReadInt32(advancedPosition, advancedPosition);
	// TODO: Validate type
	return DeserializeInt32(packet, advancedPosition, advancedPosition);
}

int64 UULSClientNetworkOwner::DeserializeInt64Parameter(const UULSWirePacket* packet, int index, int& advancedPosition) const
{
	int8 type = packet->ReadInt8(index, advancedPosition);
	FString fieldName = packet->ReadString(advancedPosition, advancedPosition);
	int32 size = packet->ReadInt32(advancedPosition, advancedPosition);
	// TODO: Validate type
	return DeserializeInt64(packet, advancedPosition, advancedPosition);
}

float UULSClientNetworkOwner::DeserializeFloat32Parameter(const UULSWirePacket* packet, int index, int& advancedPosition) const
{
	int8 type = packet->ReadInt8(index, advancedPosition);
	FString fieldName = packet->ReadString(advancedPosition, advancedPosition);
	int32 size = packet->ReadInt32(advancedPosition, advancedPosition);
	// TODO: Validate type
	return DeserializeFloat32(packet, advancedPosition, advancedPosition);
}

double UULSClientNetworkOwner::DeserializeFloat64Parameter(const UULSWirePacket* packet, int index, int& advancedPosition) const
{
	int8 type = packet->ReadInt8(index, advancedPosition);
	FString fieldName = packet->ReadString(advancedPosition, advancedPosition);
	int32 size = packet->ReadInt32(advancedPosition, advancedPosition);
	// TODO: Validate type
	return DeserializeFloat64(packet, advancedPosition, advancedPosition);
}

bool UULSClientNetworkOwner::DeserializeBoolParameter(const UULSWirePacket* packet, int index, int& advancedPosition) const
{
	int8 type = packet->ReadInt8(index, advancedPosition);
	FString fieldName = packet->ReadString(advancedPosition, advancedPosition);
	int32 size = packet->ReadInt32(advancedPosition, advancedPosition);
	// TODO: Validate type
	return DeserializeBool(packet, advancedPosition, advancedPosition, size);
}

FString UULSClientNetworkOwner::DeserializeStringParameter(const UULSWirePacket* packet, int index, int& advancedPosition) const
{
	int8 type = packet->ReadInt8(index, advancedPosition);
	FString fieldName = packet->ReadString(advancedPosition, advancedPosition);
	// TODO: Validate type
	return DeserializeString(packet, advancedPosition, advancedPosition);
}

FVector UULSClientNetworkOwner::DeserializeVectorParameter(const UULSWirePacket* packet, int index, int& advancedPosition) const
{
	int8 type = packet->ReadInt8(index, advancedPosition);
	FString fieldName = packet->ReadString(advancedPosition, advancedPosition);
	// TODO: Validate type
	return DeserializeVector(packet, advancedPosition, advancedPosition);
}

// Serialization
void UULSClientNetworkOwner::SerializeRefParameter(UULSWirePacket* packet, FString fieldname, const AActor* value, int index, int& advancedPosition) const
{
	packet->PutInt8(0, index, advancedPosition);
	packet->PutString(fieldname, advancedPosition, advancedPosition);
	packet->PutInt64(FindUniqueId(value), advancedPosition, advancedPosition);
}

void UULSClientNetworkOwner::SerializeInt16Parameter(UULSWirePacket* packet, FString fieldname, int16 value, int index, int& advancedPosition) const
{
	packet->PutInt8(1, index, advancedPosition);
	packet->PutString(fieldname, advancedPosition, advancedPosition);
	packet->PutInt32(sizeof(value), advancedPosition, advancedPosition);
	packet->PutInt16(value, advancedPosition, advancedPosition);
}

void UULSClientNetworkOwner::SerializeInt32Parameter(UULSWirePacket* packet, FString fieldname, int32 value, int index, int& advancedPosition) const
{
	packet->PutInt8(1, index, advancedPosition);
	packet->PutString(fieldname, advancedPosition, advancedPosition);
	packet->PutInt32(sizeof(value), advancedPosition, advancedPosition);
	packet->PutInt32(value, advancedPosition, advancedPosition);
}

void UULSClientNetworkOwner::SerializeInt64Parameter(UULSWirePacket* packet, FString fieldname, int64 value, int index, int& advancedPosition) const
{
	packet->PutInt8(1, index, advancedPosition);
	packet->PutString(fieldname, advancedPosition, advancedPosition);
	packet->PutInt32(sizeof(value), advancedPosition, advancedPosition);
	packet->PutInt64(value, advancedPosition, advancedPosition);
}

void UULSClientNetworkOwner::SerializeFloat32Parameter(UULSWirePacket* packet, FString fieldname, float value, int index, int& advancedPosition) const
{
	packet->PutInt8(1, index, advancedPosition);
	packet->PutString(fieldname, advancedPosition, advancedPosition);
	packet->PutInt32(sizeof(value), advancedPosition, advancedPosition);
	packet->PutFloat32(value, advancedPosition, advancedPosition);
}

void UULSClientNetworkOwner::SerializeFloat64Parameter(UULSWirePacket* packet, FString fieldname, double value, int index, int& advancedPosition) const
{
	packet->PutInt8(1, index, advancedPosition);
	packet->PutString(fieldname, advancedPosition, advancedPosition);
	packet->PutInt32(sizeof(value), advancedPosition, advancedPosition);
	packet->PutFloat64(value, advancedPosition, advancedPosition);
}

void UULSClientNetworkOwner::SerializeBoolParameter(UULSWirePacket* packet, FString fieldname, bool value, int index, int& advancedPosition) const
{
	packet->PutInt8(1, index, advancedPosition);
	packet->PutString(fieldname, advancedPosition, advancedPosition);
	packet->PutInt32(1, advancedPosition, advancedPosition);
	packet->PutInt8(value ? 1 : 0, advancedPosition, advancedPosition);
}

void UULSClientNetworkOwner::SerializeStringParameter(UULSWirePacket* packet, FString fieldname, FString value, int index, int& advancedPosition) const
{
	packet->PutInt8(2, index, advancedPosition);
	packet->PutString(fieldname, advancedPosition, advancedPosition);
	packet->PutString(value, advancedPosition, advancedPosition);
}

void UULSClientNetworkOwner::SerializeVectorParameter(UULSWirePacket* packet, FString fieldname, FVector value, int index, int& advancedPosition) const
{
	packet->PutInt8(3, index, advancedPosition);
	packet->PutString(fieldname, advancedPosition, advancedPosition);
	packet->PutFloat32(value.X, advancedPosition, advancedPosition);
	packet->PutFloat32(value.Y, advancedPosition, advancedPosition);
	packet->PutFloat32(value.Z, advancedPosition, advancedPosition);
}

// Get Sizes
int32 UULSClientNetworkOwner::GetSerializeRefParameterSize(FString fieldName) const
{
	return 1 + 4 + fieldName.Len() + 8;
}

int32 UULSClientNetworkOwner::GetSerializeInt16ParameterSize(FString fieldName) const
{
	return 1 + 4 + fieldName.Len() + 4 + sizeof(int16);
}

int32 UULSClientNetworkOwner::GetSerializeInt32ParameterSize(FString fieldName) const
{
	return 1 + 4 + fieldName.Len() + 4 + sizeof(int32);
}

int32 UULSClientNetworkOwner::GetSerializeInt64ParameterSize(FString fieldName) const
{
	return 1 + 4 + fieldName.Len() + 4 + sizeof(int64);
}

int32 UULSClientNetworkOwner::GetSerializeFloat32ParameterSize(FString fieldName) const
{
	return 1 + 4 + fieldName.Len() + 4 + sizeof(float_t);
}

int32 UULSClientNetworkOwner::GetSerializeFloat64ParameterSize(FString fieldName) const
{
	return 1 + 4 + fieldName.Len() + 4 + sizeof(double);
}

int32 UULSClientNetworkOwner::GetSerializeBoolParameterSize(FString fieldName) const
{
	return 1 + 4 + fieldName.Len() + 4 + 1;
}

int32 UULSClientNetworkOwner::GetSerializeStringParameterSize(FString fieldName, int stringLen) const
{
	return 1 + 4 + fieldName.Len() + 4 + stringLen;
}

int32 UULSClientNetworkOwner::GetSerializeVectorParameterSize(FString fieldName) const
{
	return 1 + 4 + fieldName.Len() + 4 + 4 + 4;
}
