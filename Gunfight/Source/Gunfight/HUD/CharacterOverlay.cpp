// Fill out your copyright notice in the Description page of Project Settings.


#include "CharacterOverlay.h"
#include "Components/TextBlock.h"
#include "Gunfight/GameState/GunfightGameState.h"
#include "Kismet/GameplayStatics.h"
#include "Gunfight/PlayerController/GunfightPlayerController.h"
#include "Components/Image.h"

void UCharacterOverlay::NativeConstruct()
{
	Super::NativeConstruct();

	if (ScoreboardInfos.IsEmpty())
	{
		InitializeScoreboardInfo();
		FillPlayers();
	}
}

void UCharacterOverlay::FillPlayers()
{
	UWorld* World = GetWorld();
	if (World == nullptr) return;
	APawn* OwningPawn = GetOwningPlayerPawn<APawn>();
	if (OwningPawn == nullptr || !OwningPawn->IsLocallyControlled()) return;
	AGunfightGameState* GunfightGameState = GetWorld()->GetGameState<AGunfightGameState>();
	AGunfightPlayerController* GunfightPlayerController = GetOwningPlayer<AGunfightPlayerController>();
	if (GunfightGameState == nullptr || GunfightPlayerController == nullptr) return;
	GunfightPlayerController->SetHUDScoreboardScores(0, 9); // hardcoded for 10 players max
}

void UCharacterOverlay::InitializeScoreboardInfo()
{
	ScoreboardInfos.Init(FScoreboardInfo{}, 10);

	ScoreboardInfos[0].NameText = Player0Text;
	ScoreboardInfos[0].ScoreText = Player0ScoreText;
	ScoreboardInfos[0].DeathsText = Player0DeathsText;
	ScoreboardInfos[0].ImagePlate = Player0Image;

	ScoreboardInfos[1].NameText = Player1Text;
	ScoreboardInfos[1].ScoreText = Player1ScoreText;
	ScoreboardInfos[1].DeathsText = Player1DeathsText;
	ScoreboardInfos[1].ImagePlate = Player1Image;

	ScoreboardInfos[2].NameText = Player2Text;
	ScoreboardInfos[2].ScoreText = Player2ScoreText;
	ScoreboardInfos[2].DeathsText = Player2DeathsText;
	ScoreboardInfos[2].ImagePlate = Player2Image;

	ScoreboardInfos[3].NameText = Player3Text;
	ScoreboardInfos[3].ScoreText = Player3ScoreText;
	ScoreboardInfos[3].DeathsText = Player3DeathsText;
	ScoreboardInfos[3].ImagePlate = Player3Image;

	ScoreboardInfos[4].NameText = Player4Text;
	ScoreboardInfos[4].ScoreText = Player4ScoreText;
	ScoreboardInfos[4].DeathsText = Player4DeathsText;
	ScoreboardInfos[4].ImagePlate = Player4Image;

	ScoreboardInfos[5].NameText = Player5Text;
	ScoreboardInfos[5].ScoreText = Player5ScoreText;
	ScoreboardInfos[5].DeathsText = Player5DeathsText;
	ScoreboardInfos[5].ImagePlate = Player5Image;

	ScoreboardInfos[6].NameText = Player6Text;
	ScoreboardInfos[6].ScoreText = Player6ScoreText;
	ScoreboardInfos[6].DeathsText = Player6DeathsText;
	ScoreboardInfos[6].ImagePlate = Player6Image;

	ScoreboardInfos[7].NameText = Player7Text;
	ScoreboardInfos[7].ScoreText = Player7ScoreText;
	ScoreboardInfos[7].DeathsText = Player7DeathsText;
	ScoreboardInfos[7].ImagePlate = Player7Image;

	ScoreboardInfos[8].NameText = Player8Text;
	ScoreboardInfos[8].ScoreText = Player8ScoreText;
	ScoreboardInfos[8].DeathsText = Player8DeathsText;
	ScoreboardInfos[8].ImagePlate = Player8Image;

	ScoreboardInfos[9].NameText = Player9Text;
	ScoreboardInfos[9].ScoreText = Player9ScoreText;
	ScoreboardInfos[9].DeathsText = Player9DeathsText;
	ScoreboardInfos[9].ImagePlate = Player9Image;

	HideUnusedScores();

	bIsConstructed = true;
}

void UCharacterOverlay::ScoreUpdate(int32 Index, FString Name, float Score, int32 Deaths, FColor Color)
{
	if (Index >= ScoreboardInfos.Num()) return;
	FScoreboardInfo& ScoreInfo = ScoreboardInfos[Index];
	if (ScoreInfo.NameText && ScoreInfo.ScoreText && ScoreInfo.DeathsText)
	{
		ScoreInfo.NameText->SetText(FText::FromString(Name));
		ScoreInfo.ScoreText->SetText(FText::FromString(FString::Printf(TEXT("%d"), FMath::FloorToInt(Score))));
		ScoreInfo.DeathsText->SetText(FText::FromString(FString::Printf(TEXT("%d"), Deaths)));
		ScoreInfo.ImagePlate->SetRenderOpacity(1.f);
		ScoreInfo.NameText->SetColorAndOpacity(FSlateColor(Color));
	}
}

void UCharacterOverlay::ClearScore(int32 Index)
{
	if (Index >= ScoreboardInfos.Num()) return;

	FScoreboardInfo& ScoreboardInfo = ScoreboardInfos[Index];

	if (!IsScoreboardInfoValid(ScoreboardInfo)) return;

	ScoreboardInfo.NameText->SetText(FText::FromString(FString()));
	ScoreboardInfo.ScoreText->SetText(FText::FromString(FString()));
	ScoreboardInfo.DeathsText->SetText(FText::FromString(FString()));
	ScoreboardInfo.ImagePlate->SetRenderOpacity(0.f);
}

void UCharacterOverlay::HideUnusedScores()
{
	for (auto& ScoreboardInfo : ScoreboardInfos)
	{
		if (!IsScoreboardInfoValid(ScoreboardInfo)) continue;

		ScoreboardInfo.NameText->SetText(FText::FromString(FString()));
		ScoreboardInfo.ScoreText->SetText(FText::FromString(FString()));
		ScoreboardInfo.DeathsText->SetText(FText::FromString(FString()));
		ScoreboardInfo.ImagePlate->SetRenderOpacity(0.f);
	}
}

bool UCharacterOverlay::IsScoreboardInfoValid(const FScoreboardInfo& InfoToCheck)
{
	return InfoToCheck.NameText &&
		InfoToCheck.ScoreText &&
		InfoToCheck.DeathsText &&
		InfoToCheck.ImagePlate;
}