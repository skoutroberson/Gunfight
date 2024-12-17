// Fill out your copyright notice in the Description page of Project Settings.


#include "GunfightGameMode.h"
#include "Gunfight/PlayerController/GunfightPlayerController.h"
#include "Gunfight/GameState/GunfightGameState.h"
#include "Gunfight/PlayerState/GunfightPlayerState.h"
#include "Gunfight/Character/GunfightCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"

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

void AGunfightGameMode::PlayerEliminated(AGunfightCharacter* ElimmedCharacter, AGunfightPlayerController* VictimController, AGunfightPlayerController* AttackerController)
{
	if (AttackerController == nullptr || AttackerController->PlayerState == nullptr) return;
	if (VictimController == nullptr || VictimController->PlayerState == nullptr) return;
	AGunfightPlayerState* AttackerPlayerState = AttackerController ? Cast<AGunfightPlayerState>(AttackerController->PlayerState) : nullptr;
	AGunfightPlayerState* VictimPlayerState = VictimController ? Cast<AGunfightPlayerState>(VictimController->PlayerState) : nullptr;

	GunfightGameState = GetGameState<AGunfightGameState>();

	if (AttackerPlayerState && AttackerPlayerState != VictimPlayerState && GunfightGameState)
	{
		AttackerPlayerState->AddToScore(1.f);
		GunfightGameState->UpdateTopScore(AttackerPlayerState);
	}
	if (VictimPlayerState)
	{
		VictimPlayerState->AddToDefeats(1);
	}

	if (ElimmedCharacter)
	{
		ElimmedCharacter->Elim(false);
	}


}

void AGunfightGameMode::RequestRespawn(ACharacter* ElimmedCharacter, AController* ElimmedController)
{
	if (ElimmedCharacter)
	{
		ElimmedCharacter->Reset();
		ElimmedCharacter->Destroy();
	}
	if (ElimmedController)
	{
		TArray<AActor*> PlayerStarts;
		UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), PlayerStarts);
		int32 Selection = FMath::RandRange(0, PlayerStarts.Num() - 1);
		RestartPlayerAtPlayerStart(ElimmedController, PlayerStarts[Selection]);
	}
}

void AGunfightGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	GunfightGameState = GunfightGameState == nullptr ? GetGameState<AGunfightGameState>() : GunfightGameState;

	AGunfightPlayerState* NewPlayerState = NewPlayer->GetPlayerState<AGunfightPlayerState>();

	if (GunfightGameState && NewPlayerState)
	{
		GunfightGameState->SortedPlayers.AddUnique(NewPlayerState);
		GunfightGameState->OnRep_SortedPlayers();
	}
}

void AGunfightGameMode::Logout(AController* Exiting)
{
	GunfightGameState = GunfightGameState == nullptr ? GetGameState<AGunfightGameState>() : GunfightGameState;

	AGunfightPlayerState* ExitingPlayerState = Exiting->GetPlayerState<AGunfightPlayerState>();

	if (GunfightGameState && ExitingPlayerState)
	{
		GunfightGameState->SortedPlayers.RemoveSingle(ExitingPlayerState);
		GunfightGameState->OnRep_SortedPlayers();
	}

	Super::Logout(Exiting);
}

void AGunfightGameMode::PlayerLeftGame(AGunfightPlayerState* PlayerLeaving)
{
}
