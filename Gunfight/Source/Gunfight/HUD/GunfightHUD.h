// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "GunfightHUD.generated.h"

/**
 * 
 */
UCLASS()
class GUNFIGHT_API AGunfightHUD : public AHUD
{
	GENERATED_BODY()
	
public:
	AGunfightHUD();
	virtual void Tick(float DeltaTime) override;

	void PollInit();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USceneComponent* HUDRoot;

	UPROPERTY(EditAnywhere, Category = "Player Stats")
	TSubclassOf<class UUserWidget> CharacterOverlayClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UCharacterOverlay* CharacterOverlay;

	UPROPERTY()
	class UStereoLayerComponent* StereoLayer;

	void AddCharacterOverlay();

	UPROPERTY(EditAnywhere, Category = "Announcements")
	TSubclassOf<UUserWidget> AnnouncementClass;

	UPROPERTY()
	class UAnnouncement* Announcement;

	void AddAnnouncement();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UStereoLayerComponent* VRStereoLayer;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UWidgetComponent* CharacterOverlayWidget;

	UPROPERTY(EditAnywhere)
	class UTextureRenderTarget2D* StereoLayerRenderTarget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UAudioComponent* Soundtrack;

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void UpdateSoundtrack(EGunfightMatchState NewMatchState, float StartTime = 0.f);

protected:
	virtual void BeginPlay() override;

private:

	UPROPERTY()
	class AGunfightCharacter* GunfightCharacter;

	bool bCharacterHUDInit = false;

	/**
	* Audio
	* 
	* Warmup
	* Cooldown Win/Loss
	* Game almost over?
	*/
	
	
};
