#pragma once

UENUM(BlueprintType)
enum class EGunfightMatchState : uint8
{
	EGMS_Warmup				UMETA(DisplayName = "Warmup"),
	EGMS_MatchInProgress	UMETA(DisplayName = "MatchInProgress"),
	EGMS_MatchCooldown		UMETA(DisplayName = "MatchCooldown"),
};