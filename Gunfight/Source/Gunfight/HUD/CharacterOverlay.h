// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CharacterOverlay.generated.h"

class UTextBlock;

USTRUCT(BlueprintType)
struct FScoreboardInfo
{
	GENERATED_BODY()

	UPROPERTY()
	UTextBlock* NameText;

	UPROPERTY()
	UTextBlock* ScoreText;

	UPROPERTY()
	UTextBlock* DeathsText;
};

/**
 * 
 */
UCLASS()
class GUNFIGHT_API UCharacterOverlay : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct();

public:

	void FillPlayers();

	bool bIsConstructed = false;
	
	UPROPERTY(meta = (BindWidget))
	class UProgressBar* HealthBar;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* HealthText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* ScoreAmount;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* DefeatsAmount;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* WeaponAmmoAmount;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* CarriedAmmoAmount;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* MatchCountdownText;

	UPROPERTY(meta = (BindWidget))
	class UImage* HighPingImage;

	UPROPERTY(meta = (BindWidgetAnim), Transient)
	class UWidgetAnimation* HighPingAnimation;

	// Announcement
	UPROPERTY(meta = (BindWidget))
	UTextBlock* AnnouncementText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* WarmupTime;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* InfoText;

	/**
	* Scoreboard
	*/

	// FScoreboardInfo at index corresponds with the scoreboard texts in that row
	TArray<FScoreboardInfo> ScoreboardInfos;

	// updates scoreboard info at the specified index
	void ScoreUpdate(int32 Index, FString Name, float Score, int32 Deaths);
	void ClearScore(int32 Index);
	void HideUnusedScores();
	
	UPROPERTY(meta = (BindWidget))
	class UGridPanel* Scoreboard;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* Player0Text;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* Player0ScoreText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* Player0DeathsText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* Player1Text;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* Player1ScoreText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* Player1DeathsText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* Player2Text;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* Player2ScoreText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* Player2DeathsText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* Player3Text;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* Player3ScoreText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* Player3DeathsText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* Player4Text;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* Player4ScoreText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* Player4DeathsText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* Player5Text;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* Player5ScoreText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* Player5DeathsText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* Player6Text;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* Player6ScoreText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* Player6DeathsText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* Player7Text;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* Player7ScoreText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* Player7DeathsText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* Player8Text;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* Player8ScoreText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* Player8DeathsText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* Player9Text;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* Player9ScoreText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* Player9DeathsText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* PingText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* RedScoreText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* BlueScoreText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* SwapText;

private:
	void InitializeScoreboardInfo();
};
