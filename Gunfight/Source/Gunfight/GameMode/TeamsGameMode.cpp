// Fill out your copyright notice in the Description page of Project Settings.


#include "TeamsGameMode.h"
#include "Gunfight/GameState/GunfightGameState.h"
#include "Gunfight/PlayerState/GunfightPlayerState.h"
#include "Gunfight/PlayerController/GunfightPlayerController.h"
#include "Gunfight/Character/GunfightCharacter.h"
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
			RestartGunfightRound();
		}
	}
	else if (GunfightRoundMatchState == EGunfightRoundMatchState::EGRMS_MatchCooldown)
	{
		CountdownTime = GunfightCooldownTime + GunfightRoundEndTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
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

	SetGunfightRoundMatchState(EGunfightRoundMatchState::EGRMS_RoundInProgress);
}

void ATeamsGameMode::EndGunfightRound()
{
	// Update team score, end game if team score > WinningScore
	UWorld* World = GetWorld();
	if (World == nullptr) return;
	LevelStartingTime = World->TimeSeconds;

	SetGunfightRoundMatchState(EGunfightRoundMatchState::EGRMS_RoundCooldown);
}

void ATeamsGameMode::RestartGunfightRound()
{
	UWorld* World = GetWorld();
	if (World == nullptr) return;
	LevelStartingTime = World->TimeSeconds;

	// First check if the game should end
	// 
	// Respawn all players with their input disabled

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

	SetGunfightRoundMatchState(EGunfightRoundMatchState::EGRMS_RoundStart);
}

void ATeamsGameMode::RestartGunfightRoundMatch()
{
	SetGunfightRoundMatchState(EGunfightRoundMatchState::EGRMS_Warmup);
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
