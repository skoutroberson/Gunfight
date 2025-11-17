// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CharacterOverlay.generated.h"

class UTextBlock;
class UImage;

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

	UPROPERTY()
	UImage* ImagePlate;
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
	class UGridPanel* GridPanelMain;
	
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
	UTextBlock* WeaponAmmoAmountLeft;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* CarriedAmmoAmountLeft;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* AmmoSlashR;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* AmmoSlashL;

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
	void ScoreUpdate(int32 Index, FString Name, float Score, int32 Deaths, FColor Color = FColor::White);
	void ClearScore(int32 Index);
	void HideUnusedScores();
	
	UPROPERTY(meta = (BindWidget))
	UGridPanel* Scoreboard;

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
	UImage* Player0Image;

	UPROPERTY(meta = (BindWidget))
	UImage* Player1Image;

	UPROPERTY(meta = (BindWidget))
	UImage* Player2Image;

	UPROPERTY(meta = (BindWidget))
	UImage* Player3Image;

	UPROPERTY(meta = (BindWidget))
	UImage* Player4Image;

	UPROPERTY(meta = (BindWidget))
	UImage* Player5Image;

	UPROPERTY(meta = (BindWidget))
	UImage* Player6Image;

	UPROPERTY(meta = (BindWidget))
	UImage* Player7Image;

	UPROPERTY(meta = (BindWidget))
	UImage* Player8Image;

	UPROPERTY(meta = (BindWidget))
	UImage* Player9Image;

	// misc

	UPROPERTY(meta = (BindWidget))
	UTextBlock* PingText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* RedScoreText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* BlueScoreText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* SwapText;

	UPROPERTY(meta = (BindWidget))
	UImage* HealthBackground;

	UPROPERTY(meta = (BindWidget))
	UImage* CountdownBackground;

	UPROPERTY(meta = (BindWidget))
	UImage* InfoBackground;

	UPROPERTY(meta = (BindWidget))
	UImage* AnnouncementBackground;

	UPROPERTY(meta = (BindWidget))
	UImage* BlueBackground;

	UPROPERTY(meta = (BindWidget))
	UImage* RedBackground;

private:
	void InitializeScoreboardInfo();
	bool IsScoreboardInfoValid(const FScoreboardInfo& InfoToCheck);
};
