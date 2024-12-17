// Fill out your copyright notice in the Description page of Project Settings.


#include "GunfightGameState.h"
#include "Net/UnrealNetwork.h"
#include "Gunfight/PlayerState/GunfightPlayerState.h"
#include "Gunfight/PlayerController/GunfightPlayerController.h"

void AGunfightGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGunfightGameState, SortedPlayers);
}

void AGunfightGameState::UpdateTopScore(AGunfightPlayerState* ScoringPlayer)
{
	if (TopScoringPlayers.Num() == 0)
	{
		TopScoringPlayers.Add(ScoringPlayer);
		TopScore = ScoringPlayer->GetScore();
	}
	else if (ScoringPlayer->GetScore() == TopScore)
	{
		TopScoringPlayers.AddUnique(ScoringPlayer);
	}
	else if (ScoringPlayer->GetScore() > TopScore)
	{
		TopScoringPlayers.Empty();
		TopScoringPlayers.AddUnique(ScoringPlayer);
		TopScore = ScoringPlayer->GetScore();
	}
}

void AGunfightGameState::OnRep_SortedPlayers()
{
	if (GetWorld() == nullptr) return;
	
	LocalPlayerController = LocalPlayerController == nullptr ? GetWorld()->GetFirstPlayerController<AGunfightPlayerController>() : LocalPlayerController;

	if (SortedPlayers.Num() != CurrentPlayerCount && LocalPlayerController)
	{
		CurrentPlayerCount = SortedPlayers.Num();
		LocalPlayerController->UpdateScoreboard(LocalPlayerController->GetPlayerState<AGunfightPlayerState>(), EScoreboardUpdate::ESU_MAX);
	}
}

int32 AGunfightGameState::PlayerScoreUpdate(AGunfightPlayerState* ScoringPlayer)
{
	ScoringPlayerIndex = SortedPlayers.Find(ScoringPlayer);
	if (ScoringPlayerIndex == INDEX_NONE || ScoringPlayerIndex <= 0) return ScoringPlayerIndex;

	const float ScoringPlayerScore = ScoringPlayer->GetScore();

	int32 i = 1;
	while (ScoringPlayerIndex - i >= 0)
	{
		const float CurrentScore = SortedPlayers[ScoringPlayerIndex - i]->GetScore();
		if (ScoringPlayerScore <= CurrentScore)
		{
			break;
		}
		++i;
	}
	if (i == 1) return ScoringPlayerIndex;

	SortedPlayers.RemoveAt(ScoringPlayerIndex);
	SortedPlayers.Insert(ScoringPlayer, ScoringPlayerIndex - i + 1);

	return ScoringPlayerIndex - i + 1;
}