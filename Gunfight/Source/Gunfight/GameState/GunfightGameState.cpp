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

int32 AGunfightGameState::HandlePlayerScore(AGunfightPlayerState* ScoringPlayer)
{
	ScoringPlayerIndex = SortedPlayers.Find(ScoringPlayer);
	if (ScoringPlayerIndex == 0) return ScoringPlayerIndex;

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
	++RedTeamScore;
	OnRep_RedTeamScore();
}

void AGunfightGameState::BlueTeamScores()
{
	++BlueTeamScore;
	OnRep_BlueTeamScore();
}

void AGunfightGameState::OnRep_RedTeamScore()
{
	UpdateLocalHUDTeamScore(RedTeamScore, ETeam::ET_RedTeam);
}

void AGunfightGameState::OnRep_BlueTeamScore()
{
	UpdateLocalHUDTeamScore(BlueTeamScore, ETeam::ET_BlueTeam);
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

void AGunfightGameState::QueueOrSwapSwapper(AGunfightPlayerState* TeamSwapper)
{
	if (TeamSwapper == nullptr) return;

	if (TeamSwapper->GetTeam() == ETeam::ET_RedTeam)
	{
		if (!BlueTeamSwappers.IsEmpty() || BlueTeam.IsEmpty())
		{
			AGunfightPlayerState* BlueSwapper = nullptr;
			BlueTeamSwappers.Dequeue(BlueSwapper);
			SwapTeams(TeamSwapper, BlueSwapper);
		}
		else
		{
			RedTeamSwappers.Enqueue(TeamSwapper);
		}
	}
	else if (TeamSwapper->GetTeam() == ETeam::ET_BlueTeam)
	{
		if (!RedTeamSwappers.IsEmpty() || RedTeam.IsEmpty())
		{
			AGunfightPlayerState* RedSwapper = nullptr;
			RedTeamSwappers.Dequeue(RedSwapper);
			SwapTeams(RedSwapper, TeamSwapper);
		}
		else
		{
			BlueTeamSwappers.Enqueue(TeamSwapper);
		}
	}
}

void AGunfightGameState::SwapTeams(AGunfightPlayerState* RedSwapper, AGunfightPlayerState* BlueSwapper)
{
	//if (RedSwapper == nullptr || BlueSwapper == nullptr) return;

	if (GEngine)GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Black, FString("SwapTeams"));

	if (RedSwapper != nullptr)
	{
		if (RedTeam.Contains(RedSwapper))
		{
			RedTeam.Remove(RedSwapper);
		}
		BlueTeam.AddUnique(RedSwapper);
		RedSwapper->SetTeam(ETeam::ET_BlueTeam);
		//RedSwapper->SetTeamSwapRequested(false);
		RedSwapper->ClientTeamSwapped();
	}

	if (BlueSwapper != nullptr)
	{
		if (BlueTeam.Contains(BlueSwapper))
		{
			BlueTeam.Remove(BlueSwapper);
		}
		RedTeam.AddUnique(BlueSwapper);
		BlueSwapper->SetTeam(ETeam::ET_RedTeam);
		//BlueSwapper->SetTeamSwapRequested(false);
		BlueSwapper->ClientTeamSwapped();
	}
}

bool AGunfightGameState::IsMatchEnding()
{
	return (RedTeamScore > WinningScore || BlueTeamScore > WinningScore) && FMath::Abs(RedTeamScore - BlueTeamScore) > 1;
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

void AGunfightGameState::UpdateLocalHUDTeamScore(float ScoreAmount, ETeam TeamToUpdate)
{
	if (TeamToUpdate == ETeam::ET_NoTeam || TeamToUpdate == ETeam::ET_MAX) return;
	if (GetWorld() == nullptr) return;
	LocalPlayerController = LocalPlayerController == nullptr ? GetWorld()->GetFirstPlayerController<AGunfightPlayerController>() : LocalPlayerController;
	if (LocalPlayerController == nullptr) return;
	LocalPlayerController->SetHUDTeamScore(ScoreAmount, TeamToUpdate);
}

ETeam AGunfightGameState::GetWinningTeam()
{
	return RedTeamScore > BlueTeamScore ? ETeam::ET_RedTeam : ETeam::ET_BlueTeam;
}
