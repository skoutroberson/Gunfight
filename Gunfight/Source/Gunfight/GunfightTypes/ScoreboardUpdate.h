#pragma once

UENUM(BlueprintType)
enum class EScoreboardUpdate : uint8
{
	ESU_Score		UMETA(DisplayName = "Score"),
	ESU_Death		UMETA(DisplayName = "Death"),
	ESU_Join		UMETA(DisplayName = "Join"),
	ESU_Leave		UMETA(DisplayName = "Leave"),

	ESU_MAX			UMETA(DisplayName = "DefaultMAX")
};