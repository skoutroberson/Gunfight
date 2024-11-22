#pragma once

UENUM(BlueprintType)
enum class EHandState : uint8
{
	EHS_Idle			UMETA(DisplayName = "Idle"),
	EHS_Grip			UMETA(DisplayName = "Grip"),
	EHS_HoldingPistol	UMETA(DisplayName = "HoldingPistol"),

	EHS_MAX				UMETA(DisplayName = "DefaultMAX"),
};