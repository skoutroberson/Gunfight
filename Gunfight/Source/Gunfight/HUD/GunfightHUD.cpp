// Fill out your copyright notice in the Description page of Project Settings.


#include "GunfightHUD.h"
#include "GameFramework/PlayerController.h"
#include "CharacterOverlay.h"
#include "Announcement.h"
#include "Components/StereoLayerComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Slate/WidgetRenderer.h"
#include "Components/Widget.h"
#include "Components/TextBlock.h"
#include "Components/WidgetComponent.h"
#include "Components/AudioComponent.h"
#include "Gunfight/Character/GunfightCharacter.h"

AGunfightHUD::AGunfightHUD()
{
	PrimaryActorTick.bCanEverTick = true;

	HUDRoot = CreateDefaultSubobject<USceneComponent>(TEXT("HUDRoot"));
	SetRootComponent(HUDRoot);
	/*
	VRStereoLayer = CreateDefaultSubobject<UStereoLayerComponent>(TEXT("StereoLayer"));
	VRStereoLayer->SetupAttachment(HUDRoot);
	
	CharacterOverlayWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("CharacterOverlayWidget"));
	CharacterOverlayWidget->SetupAttachment(HUDRoot);
	*/

	Soundtrack = CreateDefaultSubobject<UAudioComponent>(TEXT("Soundtrack"));
}

void AGunfightHUD::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	PollInit();
}

void AGunfightHUD::PollInit()
{
	if (!bCharacterHUDInit)
	{
		GunfightCharacter = GunfightCharacter == nullptr ? Cast<AGunfightCharacter>(GetOwningPawn()) : GunfightCharacter;

		if (GunfightCharacter && GunfightCharacter->CharacterOverlayWidget && GunfightCharacter->VRStereoLayer)
		{
			bCharacterHUDInit = true;
			GunfightCharacter->CharacterOverlayWidget->SetVisibility(true);
			GunfightCharacter->VRStereoLayer->SetVisibility(true);
		}
	}
}

void AGunfightHUD::BeginPlay()
{
	Super::BeginPlay();
	SetActorTickEnabled(true);

	APlayerController* PlayerController = GetOwningPlayerController();
	if (PlayerController == nullptr) return;
	AGunfightCharacter* GChar = Cast<AGunfightCharacter>(PlayerController->GetPawn());
	if (GChar && GChar->CharacterOverlayWidget && GChar->VRStereoLayer)
	{
		GChar->CharacterOverlayWidget->SetVisibility(true);
		GChar->VRStereoLayer->SetVisibility(true);
	}
}

void AGunfightHUD::AddCharacterOverlay()
{
	APlayerController* PlayerController = GetOwningPlayerController();
	if (PlayerController && CharacterOverlayClass && !CharacterOverlay)
	{
		//CharacterOverlay = CreateWidget<UCharacterOverlay>(PlayerController, CharacterOverlayClass);
		//CharacterOverlay->AddToViewport();
		//CharacterOverlay = Cast<UCharacterOverlay>(CharacterOverlayWidget->GetUserWidgetObject());
	}
}

void AGunfightHUD::AddAnnouncement()
{
	APlayerController* PlayerController = GetOwningPlayerController();
	if (PlayerController && AnnouncementClass)
	{
		Announcement = CreateWidget<UAnnouncement>(PlayerController, AnnouncementClass);
		Announcement->AddToViewport();
	}
}