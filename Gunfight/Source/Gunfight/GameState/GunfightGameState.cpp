// Fill out your copyright notice in the Description page of Project Settings.


#include "GunfightGameState.h"
#include "Net/UnrealNetwork.h"
#include "Gunfight/PlayerState/GunfightPlayerState.h"
#include "Gunfight/PlayerController/GunfightPlayerController.h"

void AGunfightGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGunfightGameState, TopScoringPlayers);
	DOREPLIFETIME(AGunfightGameState, SortedPlayers);
	DOREPLIFETIME(AGunfightGameState, GunfightMatchState);
	DOREPLIFETIME(AGunfightGameState, RedTeamScore);
	DOREPLIFETIME(AGunfightGameState, BlueTeamScore);
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

void AGunfightGameState::RedTeamScores()
{
}

void AGunfightGameState::BlueTeamScores()
{
}

void AGunfightGameState::OnRep_RedTeamScore()
{

}

void AGunfightGameState::OnRep_BlueTeamScore()
{

}

void AGunfightGameState::OnRep_GunfightMatchState()
{
	if (GunfightMatchState == EGunfightMatchState::EGMS_Warmup)
	{
		HandleGunfightWarmupStarted();
	}
	else if (GunfightMatchState == EGunfightMatchState::EGMS_MatchInProgress)
	{
		HandleGunfightMatchStarted();
	}
	else if (GunfightMatchState == EGunfightMatchState::EGMS_MatchCooldown)
	{
		HandleGunfightCooldownStarted();
	}
}

void AGunfightGameState::SetGunfightMatchState(EGunfightMatchState NewState)
{
	if (GetLocalRole() == ROLE_Authority)
	{
		GunfightMatchState = NewState;

		// Call the onrep to make sure the callbacks happen
		OnRep_GunfightMatchState();
	}
}

void AGunfightGameState::HandleGunfightWarmupStarted()
{
	
}

void AGunfightGameState::HandleGunfightMatchStarted()
{

}

void AGunfightGameState::HandleGunfightCooldownStarted()
{

}
