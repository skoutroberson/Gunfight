// Fill out your copyright notice in the Description page of Project Settings.


#include "GunfightGameMode.h"

namespace MatchState
{
	const FName Cooldown = FName("Cooldown");
}

AGunfightGameMode::AGunfightGameMode()
{
	bDelayedStart = true;
}

void AGunfightGameMode::BeginPlay()
{
	Super::BeginPlay();

	LevelStartingTime = GetWorld()->GetTimeSeconds();
}

void AGunfightGameMode::Tick(float DeltaTime)
{
	if (MatchState == MatchState::WaitingToStart)
	{
		CountdownTime = WarmupTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if (CountdownTime <= 0.f)
		{
			StartMatch();
		}
	}
	else if (MatchState == MatchState::InProgress)
	{
		CountdownTime = WarmupTime + MatchTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if (CountdownTime <= 0.f)
		{
			SetMatchState(MatchState::Cooldown);
		}
	}
	else if (MatchState == MatchState::Cooldown)
	{
		CountdownTime = CooldownTime + WarmupTime + MatchTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if (CountdownTime <= 0.f)
		{
			RestartGame();
		}
	}
}

void AGunfightGameMode::OnMatchStateSet()
{
	Super::OnMatchStateSet();
}

void AGunfightGameMode::PlayerEliminated(ABlasterCharacter* ElimmedCharacter, ABlasterPlayerController* VictimController, ABlasterPlayerController* AttackerController)
{
}

void AGunfightGameMode::RequestRespawn(ACharacter* ElimmedCharacter, AController* ElimmedController)
{
}

void AGunfightGameMode::PlayerLeftGame(ABlasterPlayerState* PlayerLeaving)
{
}
