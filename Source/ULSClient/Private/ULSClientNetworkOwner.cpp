// Fill out your copyright notice in the Description page of Project Settings.


#include "ULSClientNetworkOwner.h"
#include "VaRestJsonObject.h"
#include "ULSWirePacket.h"
#include "ULSTransport.h"

void UULSClientNetworkOwner::HandleWirePacket(const UULSWirePacket* packet)
{
    if (packet != nullptr)
    {
        switch (packet->PacketType)
        {
            case EWirePacketType::RpcCall:
                HandleRpcPacket(packet);
                break;

            case EWirePacketType::RpcCallResponse:
                HandleRpcResponsePacket(packet);
                break;

            case EWirePacketType::SpawnActor:
                HandleSpawnActorMessage(packet);
                break;

            case EWirePacketType::DespawnActor:
                HandleDespawnActorMessage(packet);
                break;

            case EWirePacketType::Replication:
                HandleReplicationMessage(packet);
                break;

            default:
                OnReceivePacket(packet);
                break;
        }
    }
}

void UULSClientNetworkOwner::HandleRpcPacket(const UULSWirePacket* packet)
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
	FString methodName = packet->ReadString(position, position);
	FString returnType = packet->ReadString(position, position);
	int32 numberOfParameters = packet->ReadInt32(position, position);

    BEGIN_RPC_BP_EVENTS_FROM_SERVER_CALL
	END_RPC_BP_EVENTS_FROM_SERVER_CALL
}

BEGIN_RPC_BP_EVENTS_TO_SERVER_CALL
END_RPC_BP_EVENTS_TO_SERVER_CALL

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
				UE_LOG(LogTemp, Display, TEXT("HandleReplicationMessage: %s.%s = nullptr"), *existingActor->GetName(), *prop->GetName());
				// Set the reference to "null"
				FObjectProperty* objProp = (FObjectProperty*)prop;
				if (AActor** valuePtr = objProp->ContainerPtrToValuePtr<AActor*>(existingActor))
				{
					if (*valuePtr != nullptr)
					{
						valueDidChange = true;
						*valuePtr = nullptr;
					}
				}
			}
			else
			{
				UE_LOG(LogTemp, Display, TEXT("HandleReplicationMessage: %s.%s = %s"), *existingActor->GetName(), *prop->GetName(), *actorRef->GetName());
				FObjectProperty* objProp = (FObjectProperty*)prop;
				if (AActor** valuePtr = objProp->ContainerPtrToValuePtr<AActor*>(existingActor))
				{
					if (*valuePtr != actorRef)
					{
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

		case EReplicatedFieldType::Primitive:
		{
			// Value
			//UE_LOG(LogTemp, Display, TEXT("HandleReplicationMessage: Set %s of %ld to value: "), *fieldName, uniqueId);
			int32 propSize = prop->GetSize();
			int32 size = packet->ReadInt32(position, position);
			//UE_LOG(LogTemp, Display, TEXT("HandleReplicationMessage: propSize: %i --- size: %i"), propSize, size);
			//const uint8* rawData = packet->GetRawDataPtr(position);
			//prop->CopyCompleteValue_InContainer()
			if (FIntProperty* intProp = CastField<FIntProperty>(prop))
			{
				if (int32* iVal = intProp->ContainerPtrToValuePtr<int32>(existingActor))
				{
					int32 newVal = DeserializeInt32(packet, position, position);
					UE_LOG(LogTemp, Display, TEXT("HandleReplicationMessage: %s.%s = %i"), *existingActor->GetName(), *prop->GetName(), newVal);
					if (*iVal != newVal)
					{
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
					UE_LOG(LogTemp, Display, TEXT("HandleReplicationMessage: %s.%s = %ld"), *existingActor->GetName(), *prop->GetName(), newVal);
					if (*iVal != newVal)
					{
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
					UE_LOG(LogTemp, Display, TEXT("HandleReplicationMessage: %s.%s = %ld"), *existingActor->GetName(), *prop->GetName(), newVal);
					if (*iVal != newVal)
					{
						valueDidChange = true;
						*iVal = newVal;
					}
				}
			}
			else if (FFloatProperty* floatProp = CastField<FFloatProperty>(prop))
			{
				if (float_t* fVal = floatProp->ContainerPtrToValuePtr<float_t>(existingActor))
				{
					float_t newVal = DeserializeFloat32(packet, position, position);
					UE_LOG(LogTemp, Display, TEXT("HandleReplicationMessage: %s.%s = %f"), *existingActor->GetName(), *prop->GetName(), newVal);
					if (*fVal != newVal)
					{
						valueDidChange = true;
						*fVal = newVal;
					}
				}
			}
			else if (FBoolProperty* boolProp = CastField<FBoolProperty>(prop))
			{
				if (bool* bVal = boolProp->ContainerPtrToValuePtr<bool>(existingActor))
				{
					bool newVal = DeserializeBool(packet, position, position, size);
					UE_LOG(LogTemp, Display, TEXT("HandleReplicationMessage: %s.%s = %s"), *existingActor->GetName(), *prop->GetName(), newVal ? TEXT("TRUE") : TEXT("FALSE"));
					if (*bVal != newVal)
					{
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

		case EReplicatedFieldType::String:
		{
			// String
			FString fieldValue = DeserializeString(packet, position, position);
			UE_LOG(LogTemp, Display, TEXT("HandleReplicationMessage: %s.%s = %s"), *existingActor->GetName(), *prop->GetName(), *fieldValue);
			FStrProperty* strProp = (FStrProperty*)prop;
			if (FString* valuePtr = strProp->ContainerPtrToValuePtr<FString>(existingActor))
			{
				if (*valuePtr != fieldValue)
				{
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

			UE_LOG(LogTemp, Display, TEXT("HandleReplicationMessage: %s.%s = %s"), *existingActor->GetName(), *prop->GetName(), *vec.ToString());

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
		if (uniqueId == 1631806313)
		{
			UE_LOG(LogTemp, Warning, TEXT("HandleReplicationMessage: Could not find actor with id %ld"), uniqueId);
		}

		UE_LOG(LogTemp, Warning, TEXT("HandleReplicationMessage: Could not find actor with id %ld"), uniqueId);
	}
	return res;
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
	return 1 + 4 + fieldName.Len() + 4 + 2;
}

int32 UULSClientNetworkOwner::GetSerializeInt32ParameterSize(FString fieldName) const
{
	return 1 + 4 + fieldName.Len() + 4 + 4;
}

int32 UULSClientNetworkOwner::GetSerializeInt64ParameterSize(FString fieldName) const
{
	return 1 + 4 + fieldName.Len() + 4 + 8;
}

int32 UULSClientNetworkOwner::GetSerializeFloat32ParameterSize(FString fieldName) const
{
	return 1 + 4 + fieldName.Len() + 4 + 4;
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
