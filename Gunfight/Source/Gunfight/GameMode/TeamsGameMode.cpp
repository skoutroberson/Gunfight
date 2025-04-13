// Fill out your copyright notice in the Description page of Project Settings.


#include "TeamsGameMode.h"
#include "Gunfight/GameState/GunfightGameState.h"
#include "Gunfight/PlayerState/GunfightPlayerState.h"
#include "Gunfight/PlayerController/GunfightPlayerController.h"
#include "Gunfight/Character/GunfightCharacter.h"
#include "Gunfight/GunfightComponents/WeaponPoolComponent.h"
#include "Kismet/GameplayStatics.h"

ATeamsGameMode::ATeamsGameMode()
{
	bTeamsMatch = true;
}

void ATeamsGameMode::BeginPlay()
{
	Super::BeginPlay();

	ShuffleTeamSpawns();

	SetGunfightRoundMatchState(EGunfightRoundMatchState::EGRMS_WaitingForPlayers);
}

void ATeamsGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	AGunfightGameState* GGameState = GetGameState<AGunfightGameState>();
	if (GGameState)
	{
		AGunfightPlayerState* GPState = NewPlayer->GetPlayerState<AGunfightPlayerState>();
		if (GPState && GPState->GetTeam() == ETeam::ET_NoTeam)
		{
			if (GGameState->RedTeam.Num() >= GGameState->BlueTeam.Num())
			{
				GGameState->BlueTeam.AddUnique(GPState);
				GPState->SetTeam(ETeam::ET_BlueTeam);
			}
			else
			{
				GGameState->RedTeam.AddUnique(GPState);
				GPState->SetTeam(ETeam::ET_RedTeam);
			}
		}
	}

	int32 NumberOfPlayers = GameState.Get()->PlayerArray.Num();
	if (GunfightRoundMatchState == EGunfightRoundMatchState::EGRMS_WaitingForPlayers && NumberOfPlayers > 1)
	{
		RestartGunfightMatch();
	}
}

void ATeamsGameMode::Logout(AController* Exiting)
{
	AGunfightGameState* GGameState = Cast<AGunfightGameState>(UGameplayStatics::GetGameState(this));
	AGunfightPlayerState* GPState = Exiting->GetPlayerState<AGunfightPlayerState>();
	if (GGameState && GPState)
	{
		if (GGameState->RedTeam.Contains(GPState))
		{
			GGameState->RedTeam.Remove(GPState);
		}
		if (GGameState->BlueTeam.Contains(GPState))
		{
			GGameState->BlueTeam.Remove(GPState);
		}

		// remove team swapper
		AGunfightPlayerState* RedQueuePState = nullptr;
		AGunfightPlayerState* BlueQueuePState = nullptr;
		GGameState->RedTeamSwappers.Peek(RedQueuePState);
		GGameState->BlueTeamSwappers.Peek(BlueQueuePState);
		
		if (RedQueuePState == GPState)
		{
			GGameState->RedTeamSwappers.Pop();
		}
		if (BlueQueuePState == GPState)
		{
			GGameState->BlueTeamSwappers.Pop();
		}

		// End the round if the exiting player was the last alive on their team.
		if (ShouldEndRound(GPState->GetTeam()))
		{
			UpdateTeamScore(GPState->GetTeam());
			EndGunfightRound();
		}
	}

	Super::Logout(Exiting);
}

void ATeamsGameMode::TickGunfightMatchState(float DeltaTime)
{
	if (GunfightRoundMatchState == EGunfightRoundMatchState::EGRMS_Warmup)
	{
		CountdownTime = GunfightWarmupTime + WaitingToStartTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if (CountdownTime <= 0.f)
		{
			StartGunfightRoundMatch();
		}
	}
	else if (GunfightRoundMatchState == EGunfightRoundMatchState::EGRMS_RoundStart)
	{
		CountdownTime = GunfightRoundStartTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if (CountdownTime <= 0.f)
		{
			StartGunfightRound();
		}
	}
	else if (GunfightRoundMatchState == EGunfightRoundMatchState::EGRMS_RoundInProgress)
	{
		CountdownTime = GunfightRoundTime + GunfightRoundStartTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if (CountdownTime <= 0.f)
		{
			EndGunfightRound();
		}
	}
	else if (GunfightRoundMatchState == EGunfightRoundMatchState::EGRMS_RoundCooldown)
	{
		CountdownTime = GunfightRoundEndTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if (CountdownTime <= 0.f)
		{
			ShouldEndGame() ? EndGunfightRoundMatch() : RestartGunfightRound();
			//RestartGunfightRound();
		}
	}
	else if (GunfightRoundMatchState == EGunfightRoundMatchState::EGRMS_MatchCooldown)
	{
		CountdownTime = GunfightCooldownTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if (CountdownTime <= 0.f)
		{
			RestartGunfightRoundMatch();
		}
	}
}

void ATeamsGameMode::StartGunfightRoundMatch()
{
	UWorld* World = GetWorld();
	if (World == nullptr) return;
	LevelStartingTime = World->TimeSeconds;

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		AGunfightPlayerController* GunfightPlayer = Cast<AGunfightPlayerController>(*It);
		if (GunfightPlayer)
		{
			AGunfightCharacter* GunfightPlayerCharacter = Cast<AGunfightCharacter>(GunfightPlayer->GetPawn());
			AGunfightPlayerState* GunfightPlayerState = Cast<AGunfightPlayerState>(GunfightPlayer->PlayerState);
			if (GunfightPlayerState && GunfightPlayerCharacter)
			{
				GunfightPlayerState->SetDefeats(0);
				GunfightPlayerState->SetScore(0.f);
				RequestRespawn(GunfightPlayerCharacter, GunfightPlayer, true);
				//GunfightPlayer->ClientUpdateMatchState(EGunfightMatchState::EGMS_MatchInProgress);
			}
		}
	}

	SetGunfightRoundMatchState(EGunfightRoundMatchState::EGRMS_RoundStart);
}

void ATeamsGameMode::StartGunfightRound()
{
	// Enable input for all players

	/*for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		AGunfightPlayerController* GunfightPlayer = Cast<AGunfightPlayerController>(*It);
		if (GunfightPlayer)
		{
			AGunfightCharacter* GunfightPlayerCharacter = Cast<AGunfightCharacter>(GunfightPlayer->GetPawn());
			if (GunfightPlayerCharacter)
			{
				GunfightPlayerCharacter->MulticastEnableInput();
			}
		}
	}*/


	if (ShouldEndRound(ETeam::ET_RedTeam))
	{
		UpdateTeamScore(ETeam::ET_RedTeam);
		EndGunfightRound();
		return;
	}
	else if (ShouldEndRound(ETeam::ET_BlueTeam))
	{
		UpdateTeamScore(ETeam::ET_BlueTeam);
		EndGunfightRound();
		return;
	}

	SetGunfightRoundMatchState(EGunfightRoundMatchState::EGRMS_RoundInProgress);
}

void ATeamsGameMode::EndGunfightRound()
{
	// Update team score, end game if team score > WinningScore
	UWorld* World = GetWorld();
	if (World == nullptr) return;
	LevelStartingTime = World->TimeSeconds;

	ShuffleTeamSpawns();

	if (ShouldEndGame())
	{
		SetGunfightRoundMatchState(EGunfightRoundMatchState::EGRMS_MatchCooldown);
	}
	else
	{
		SetGunfightRoundMatchState(EGunfightRoundMatchState::EGRMS_RoundCooldown);
	}
}

void ATeamsGameMode::RestartGunfightRound()
{
	UWorld* World = GetWorld();
	if (World == nullptr || WeaponPool == nullptr) return;
	LevelStartingTime = World->TimeSeconds;

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		AGunfightPlayerController* GunfightPlayer = Cast<AGunfightPlayerController>(*It);
		if (GunfightPlayer)
		{
			AGunfightCharacter* GunfightPlayerCharacter = Cast<AGunfightCharacter>(GunfightPlayer->GetPawn());
			if (GunfightPlayerCharacter)
			{
				RequestRespawn(GunfightPlayerCharacter, GunfightPlayer, true);
				//GunfightPlayer->ClientUpdateMatchState(EGunfightMatchState::EGMS_MatchInProgress);
			}
		}
	}

	WeaponPool->ReclaimFieldWeapons();

	SetGunfightRoundMatchState(EGunfightRoundMatchState::EGRMS_RoundStart);
}

void ATeamsGameMode::RestartGunfightRoundMatch()
{
	GunfightGameState = GunfightGameState == nullptr ? GetGameState<AGunfightGameState>() : GunfightGameState;
	if (GunfightGameState == nullptr) return;

	UWorld* World = GetWorld();
	if (World == nullptr) return;
	LevelStartingTime = World->TimeSeconds;

	GunfightGameState->RedTeamScore = 0;
	GunfightGameState->BlueTeamScore = 0;
	// call OnRep to update HUD for server player
	GunfightGameState->OnRep_RedTeamScore();
	GunfightGameState->OnRep_BlueTeamScore();

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		AGunfightPlayerController* GunfightPlayer = Cast<AGunfightPlayerController>(*It);
		if (GunfightPlayer)
		{
			//GunfightPlayer->ClientRestartGame(EGunfightMatchState::EGMS_Warmup, GunfightWarmupTime, GunfightMatchTime, GunfightCooldownTime, 0.f);
			AGunfightCharacter* GunfightPlayerCharacter = Cast<AGunfightCharacter>(GunfightPlayer->GetPawn());
			AGunfightPlayerState* GunfightPlayerState = GunfightPlayer->GetPlayerState<AGunfightPlayerState>();
			if (GunfightPlayerState && GunfightPlayerCharacter)
			{
				GunfightPlayerState->SetScore(0.f);
				GunfightPlayerState->SetDefeats(0);
				RequestRespawn(GunfightPlayerCharacter, GunfightPlayer, false);
			}
		}
	}

	SetGunfightRoundMatchState(EGunfightRoundMatchState::EGRMS_Warmup);
}

void ATeamsGameMode::EndGunfightRoundMatch()
{
	SetGunfightRoundMatchState(EGunfightRoundMatchState::EGRMS_MatchCooldown);
}

bool ATeamsGameMode::ShouldEndRound(ETeam TeamToCheck)
{
	return GunfightRoundMatchState == EGunfightRoundMatchState::EGRMS_RoundInProgress && AreAllPlayersDead(TeamToCheck);
}

bool ATeamsGameMode::AreAllPlayersDead(ETeam TeamToCheck)
{
	AGunfightGameState* GGameState = GetGameState<AGunfightGameState>();
	if (GGameState == nullptr || TeamToCheck == ETeam::ET_NoTeam) return false;

	TArray<AGunfightPlayerState*> PlayersToCheck = TeamToCheck == ETeam::ET_RedTeam ? GGameState->RedTeam : GGameState->BlueTeam;

	bool bAllDead = true;

	for (AGunfightPlayerState* GPState : PlayersToCheck)
	{
		if (GPState && GPState->GetPawn())
		{
			AGunfightCharacter* GChar = Cast<AGunfightCharacter>(GPState->GetPawn());
			if (GChar && !GChar->IsEliminated())
			{
				bAllDead = false;
				break;
			}
		}
	}

	return bAllDead;
}

void ATeamsGameMode::UpdateTeamScore(ETeam LosingTeam)
{
	AGunfightGameState* GState = GetGameState<AGunfightGameState>();
	if (GState == nullptr) return;

	LosingTeam == ETeam::ET_RedTeam ? GState->BlueTeamScores() : GState->RedTeamScores();
}

bool ATeamsGameMode::ShouldEndGame()
{
	AGunfightGameState* GState = GetGameState<AGunfightGameState>();
	if (GState == nullptr) return false;

	int32 BlueScore = GState->BlueTeamScore;
	int32 RedScore = GState->RedTeamScore;

	if (FMath::Abs(BlueScore - RedScore) < 2) return false; // win by 2

	if (BlueScore >= WinningTeamScore || RedScore >= WinningTeamScore)
	{
		return true;
	}

	return false;
}

ETeam ATeamsGameMode::GetWinningTeam()
{
	GunfightGameState = GunfightGameState == nullptr ? GetGameState<AGunfightGameState>() : GunfightGameState;
	if (GunfightGameState == nullptr) return ETeam::ET_NoTeam;

	return GunfightGameState->RedTeamScore > GunfightGameState->BlueTeamScore ? ETeam::ET_RedTeam : ETeam::ET_BlueTeam;
}

void ATeamsGameMode::SetGunfightRoundMatchState(EGunfightRoundMatchState NewRoundMatchState)
{
	if (GunfightRoundMatchState == NewRoundMatchState)
	{
		return;
	}

	GunfightRoundMatchState = NewRoundMatchState;

	OnGunfightRoundMatchStateSet();
}

void ATeamsGameMode::OnGunfightRoundMatchStateSet()
{
	if (GunfightRoundMatchState == EGunfightRoundMatchState::EGRMS_Warmup)
	{
		
	}
	else if (GunfightRoundMatchState == EGunfightRoundMatchState::EGRMS_RoundStart)
	{

	}
	else if (GunfightRoundMatchState == EGunfightRoundMatchState::EGRMS_RoundInProgress)
	{

	}
	else if (GunfightRoundMatchState == EGunfightRoundMatchState::EGRMS_RoundCooldown)
	{
		bRedSpawnsA = !bRedSpawnsA; // swap team spawns every round.
	}
	else if (GunfightRoundMatchState == EGunfightRoundMatchState::EGRMS_MatchCooldown)
	{

	}

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		AGunfightPlayerController* GunfightPlayer = Cast<AGunfightPlayerController>(*It);
		if (GunfightPlayer)
		{
			GunfightPlayer->SetGunfightRoundMatchState(GunfightRoundMatchState);
		}
	}
}

float ATeamsGameMode::CalculateDamage(AController* Attacker, AController* Victim, float BaseDamage)
{
	AGunfightPlayerState* AttackerPlayerState = Attacker->GetPlayerState<AGunfightPlayerState>();
	AGunfightPlayerState* VictimPlayerState = Victim->GetPlayerState<AGunfightPlayerState>();
	if (AttackerPlayerState == nullptr || VictimPlayerState == nullptr) return BaseDamage;
	if (VictimPlayerState == AttackerPlayerState)
	{
		return BaseDamage;
	}
	if (AttackerPlayerState->GetTeam() == VictimPlayerState->GetTeam())
	{
		return 0.f;
	}
	return BaseDamage;
}

void ATeamsGameMode::PlayerEliminated(AGunfightCharacter* ElimmedCharacter, AGunfightPlayerController* VictimController, AGunfightPlayerController* AttackerController)
{
	Super::PlayerEliminated(ElimmedCharacter, VictimController, AttackerController);

	AGunfightPlayerState* GPState = VictimController->GetPlayerState<AGunfightPlayerState>();
	if (GPState == nullptr || GPState->GetTeam() == ETeam::ET_NoTeam) return;

	if (ShouldEndRound(GPState->GetTeam()))
	{
		UpdateTeamScore(GPState->GetTeam());
		EndGunfightRound();
	}
}

void ATeamsGameMode::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();

	AGunfightGameState* GGameState = Cast<AGunfightGameState>(UGameplayStatics::GetGameState(this));
	if (GGameState)
	{
		for (auto PState : GGameState->PlayerArray)
		{
			AGunfightPlayerState* GPState = Cast<AGunfightPlayerState>(PState.Get());
			if (GPState && GPState->GetTeam() == ETeam::ET_NoTeam)
			{
				if (GGameState->RedTeam.Num() >= GGameState->BlueTeam.Num())
				{
					GGameState->BlueTeam.AddUnique(GPState);
					GPState->SetTeam(ETeam::ET_BlueTeam);
				}
				else
				{
					GGameState->RedTeam.AddUnique(GPState);
					GPState->SetTeam(ETeam::ET_RedTeam);
				}
			}
		}
	}
}
