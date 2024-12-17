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

AGunfightHUD::AGunfightHUD()
{
	HUDRoot = CreateDefaultSubobject<USceneComponent>(TEXT("HUDRoot"));
	SetRootComponent(HUDRoot);
	/*
	VRStereoLayer = CreateDefaultSubobject<UStereoLayerComponent>(TEXT("StereoLayer"));
	VRStereoLayer->SetupAttachment(HUDRoot);
	
	CharacterOverlayWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("CharacterOverlayWidget"));
	CharacterOverlayWidget->SetupAttachment(HUDRoot);
	*/
}

void AGunfightHUD::BeginPlay()
{
	Super::BeginPlay();

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