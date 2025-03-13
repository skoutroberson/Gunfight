// Fill out your copyright notice in the Description page of Project Settings.


#include "TeamsGameMode.h"
#include "Gunfight/GameState/GunfightGameState.h"
#include "Gunfight/PlayerState/GunfightPlayerState.h"
#include "Gunfight/PlayerController/GunfightPlayerController.h"
#include "Kismet/GameplayStatics.h"

ATeamsGameMode::ATeamsGameMode()
{
	bTeamsMatch = true;
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
