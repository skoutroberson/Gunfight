#pragma once

UENUM(BlueprintType)
enum class EGunfightMatchState : uint8
{
	EGMS_Uninitialized		UMETA(DisplayName = "Uninitialized"),
	EGMS_Warmup				UMETA(DisplayName = "Warmup"),
	EGMS_MatchInProgress	UMETA(DisplayName = "MatchInProgress"),
	EGMS_MatchCooldown		UMETA(DisplayName = "MatchCooldown"),
	EGMS_WaitingForPlayers	UMETA(DisplayName = "WaitingForPlayers"),
};

UENUM(BlueprintType)
enum class EGunfightRoundMatchState : uint8
{
	EGRMS_Uninitialized			UMETA(DisplayName = "Uninitialized"),
	EGRMS_Warmup				UMETA(DisplayName = "Warmup"),
	EGRMS_RoundStart			UMETA(DisplayName = "RoundStart"),
	EGRMS_RoundInProgress		UMETA(DisplayName = "RoundInProgress"),
	EGRMS_RoundCooldown			UMETA(DisplayName = "RoundCooldown"),
	EGRMS_MatchCooldown			UMETA(DisplayName = "MatchCooldown"),
	EGRMS_WaitingForPlayers		UMETA(DisplayName = "WaitingForPlayers"),
};