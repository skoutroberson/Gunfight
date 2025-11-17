// Fill out your copyright notice in the Description page of Project Settings.


#include "GunfightCharacter.h"
#include "Net/UnrealNetwork.h"
#include "Gunfight/Character/GunfightAnimInstance.h"
#include "GripMotionControllerComponent.h"
#include "VRExpansionFunctionLibrary.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Gunfight/GunfightComponents/CombatComponent.h"
#include "Gunfight/GunfightComponents/LagCompensationComponent.h"
#include "Gunfight/Weapon/Weapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Components/SphereComponent.h"
#include "Gunfight/Gunfight.h"
#include "Gunfight/GameMode/GunfightGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Gunfight/PlayerController/GunfightPlayerController.h"
#include "Gunfight/Weapon/FullMagazine.h"
#include "ParentRelativeAttachmentComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Components/StereoLayerComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/CapsuleComponent.h"
#include "Gunfight/PlayerState/GunfightPlayerState.h"
#include "Gunfight/HUD/CharacterOverlay.h"
#include "Sound/SoundCue.h"
#include "Gunfight/HUD/GunfightHUD.h"
#include "Gunfight/EOS/EOSSubsystem.h"
#include "Kismet/KismetMathLibrary.h"
#include "Gunfight/GameInstance/GunfightGameInstanceSubsystem.h"
#include "Gunfight/SaveGame/GunfightSaveGame.h"
#include "VRRootComponent.h"
#include "Engine/GameInstance.h"
#include "Gunfight/GunfightTypes/GunfightMatchState.h"
#include "Gunfight/Hands/HandAnimInstance.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Components/AudioComponent.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"


AGunfightCharacter::AGunfightCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	
	Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	Combat->SetIsReplicated(true);

	LagCompensation = CreateDefaultSubobject<ULagCompensationComponent>(TEXT("LagCompensation"));

	LeftHandSphere = CreateDefaultSubobject<USphereComponent>(TEXT("LeftHandSphere"));
	//LeftHandSphere->SetupAttachment(GetMesh(), FName("hand_l"));
	LeftHandSphere->SetCollisionObjectType(ECC_HandController);
	LeftHandSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	LeftHandSphere->SetCollisionResponseToChannel(ECC_Weapon, ECollisionResponse::ECR_Overlap);
	LeftHandSphere->SetUseCCD(true);
	LeftHandSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	RightHandSphere = CreateDefaultSubobject<USphereComponent>(TEXT("RightHandSphere"));
	//RightHandSphere->SetupAttachment(GetMesh(), FName("hand_r"));
	RightHandSphere->SetCollisionObjectType(ECC_HandController);
	RightHandSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	RightHandSphere->SetCollisionResponseToChannel(ECC_Weapon, ECollisionResponse::ECR_Overlap);
	RightHandSphere->SetUseCCD(true);
	RightHandSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	RightHandSphere->SetupAttachment(RightMotionController);
	LeftHandSphere->SetupAttachment(LeftMotionController);

	// Hand Offsets

	LeftHandMeshOffset = CreateDefaultSubobject<USceneComponent>(TEXT("LeftHandMeshOffset"));
	LeftHandMeshOffset->SetupAttachment(LeftMotionController);
	RightHandMeshOffset = CreateDefaultSubobject<USceneComponent>(TEXT("RightHandMeshOffset"));
	RightHandMeshOffset->SetupAttachment(RightMotionController);

	// Weapon Offsets

	LeftHandPistolOffset = CreateDefaultSubobject<USceneComponent>(TEXT("LeftHandPistolOffset"));
	LeftHandPistolOffset->SetupAttachment(LeftMotionController);
	RightHandPistolOffset = CreateDefaultSubobject<USceneComponent>(TEXT("RightHandPistolOffset"));
	RightHandPistolOffset->SetupAttachment(RightMotionController);

	VRStereoLayer = CreateDefaultSubobject<UStereoLayerComponent>(TEXT("VRStereoLayer"));
	VRStereoLayer->SetupAttachment(VRReplicatedCamera);
	VRStereoLayer->SetRelativeLocation(FVector(50.f, 0.f, -5.f));

	CharacterOverlayWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("CharacterOverlayWidget"));
	CharacterOverlayWidget->SetupAttachment(VRReplicatedCamera);
	CharacterOverlayWidget->SetRelativeLocation(FVector(50.f, 0.f, -5.f));
	CharacterOverlayWidget->SetRelativeRotation(FRotator(0.f, 180.f, 0.f));
	CharacterOverlayWidget->SetMaterial(0, WidgetMaterial);

	FootstepAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("FootstepAudio"));
	FootstepAudio->SetupAttachment(GetRootComponent());

	InitWeaponOffsets();
}

void AGunfightCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGunfightCharacter, DefaultWeapon);
	DOREPLIFETIME_CONDITION(AGunfightCharacter, OverlappingWeapon, COND_OwnerOnly);
	DOREPLIFETIME(AGunfightCharacter, bDisableGameplay);
	DOREPLIFETIME(AGunfightCharacter, Health);
	DOREPLIFETIME(AGunfightCharacter, bDisableShooting);
}

void AGunfightCharacter::Restart()
{
	Super::Restart();

	/*
	GunfightPlayerController = GunfightPlayerController == nullptr ? Cast<AGunfightPlayerController>(Controller) : GunfightPlayerController;
	if (GunfightPlayerController)
	{
		GunfightPlayerController->InitializeHUD();
	}
	*/
}

void AGunfightCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	World = GetWorld();

	GunfightAnimInstance = Cast<UGunfightAnimInstance>(GetMesh()->GetAnimInstance());
	if (GunfightAnimInstance)
	{
		GunfightAnimInstance->SetCharacter(this);
		if (IsLocallyControlled())
		{
			GunfightAnimInstance->SetLocallyControlled(true);
		}
	}

	if (Combat)
	{
		Combat->Character = this;
	}
	if (LagCompensation)
	{
		LagCompensation->Character = this;
		if (Controller)
		{
			LagCompensation->Controller = Cast<AGunfightPlayerController>(Controller);
		}
	}
}

void AGunfightCharacter::DestroyDefaultWeapon()
{
	if (GetWorld() == nullptr || Combat == nullptr) return;
	GunfightGameMode = GunfightGameMode == nullptr ? GetWorld()->GetAuthGameMode<AGunfightGameMode>() : GunfightGameMode;
	if (GunfightGameMode == nullptr) return;
	
	bool bWeaponDestroyed = false; // only destroy one weapon

	if (Combat->LeftEquippedWeapon)
	{
		GunfightGameMode->DestroyWeapon(Combat->LeftEquippedWeapon);
		bWeaponDestroyed = true;
	}
	if (!bWeaponDestroyed && Combat->RightEquippedWeapon)
	{
		GunfightGameMode->DestroyWeapon(Combat->RightEquippedWeapon);
		bWeaponDestroyed = true;
	}
	if (!bWeaponDestroyed && Combat->LeftHolsteredWeapon)
	{
		GunfightGameMode->DestroyWeapon(Combat->LeftHolsteredWeapon);
		bWeaponDestroyed = true;
	}
	if (!bWeaponDestroyed && Combat->RightHolsteredWeapon)
	{
		GunfightGameMode->DestroyWeapon(Combat->RightHolsteredWeapon);
	}
}

void AGunfightCharacter::DeathPP()
{
	DeathPostProcess();
}

void AGunfightCharacter::SpawnPP()
{
	SpawnPostProcess();
}

void AGunfightCharacter::ReInitializeSubsystem()
{
	UGameInstance* GI = UGameplayStatics::GetGameInstance(this);
	if (GI == nullptr) return;
	DebugLogMessage(FString("=======Init Game Instance======="));
	

}

void AGunfightCharacter::ShutDownGameInstance()
{
	UGameInstance* GI = UGameplayStatics::GetGameInstance(this);
	if (GI == nullptr) return;
	DebugLogMessage(FString("=======Shutdown Game Instance======="));
	GI->Shutdown();
}

void AGunfightCharacter::RestartGameInstance()
{
	UGameInstance* GI = UGameplayStatics::GetGameInstance(this);
	if (GI == nullptr) return;
	DebugLogMessage(FString("=======Restart Game Instance======="));
	GI->RestartSubsystems();
}

void AGunfightCharacter::SetTeamColor(ETeam Team)
{
	if (GetMesh() == nullptr || OriginalMaterial == nullptr || RedMaterial == nullptr || BlueMaterial == nullptr) return;

	switch (Team)
	{
	case ETeam::ET_NoTeam:
		GetMesh()->SetMaterial(0, OriginalMaterial);
		break;
	case ETeam::ET_RedTeam:
		GetMesh()->SetMaterial(0, RedMaterial);
		break;
	case ETeam::ET_BlueTeam:
		GetMesh()->SetMaterial(0, BlueMaterial);
		break;
	}
}

USceneComponent* AGunfightCharacter::GetWeaponOffset(EWeaponType WeaponType, bool bLeft)
{
	if (WeaponType == EWeaponType::EWT_Pistol)
	{
		return bLeft ? LeftHandPistolOffset : RightHandPistolOffset;
	}

	return nullptr;
}

void AGunfightCharacter::OnRep_AttachmentReplication()
{
	
}

void AGunfightCharacter::BeginPlay()
{
	Super::BeginPlay();
	SetActorTickEnabled(true);
	IKQueryParams.AddIgnoredActor(this);
	IKQueryParams.bReturnPhysicalMaterial = true;
	IKQueryParams.IgnoreMask = 1 << 3;

	VRRootReference.Get()->SetMaskFilterOnBodyInstance(1 << 3); // so IK ignores this component when walking through other characters

	if (GetMesh())
	{
		GetMesh()->SetMaskFilterOnBodyInstance(1 << 3);
	}

	if (HasAuthority())
	{
		OnTakeAnyDamage.AddDynamic(this, &AGunfightCharacter::ReceiveDamage);
	}

	// TODO make the widget/stereo layer not construct on non locally controlled clients

	if (Controller)
	{
		AGunfightPlayerController* GPlayer = Cast<AGunfightPlayerController>(Controller);
		if (GPlayer && GPlayer->GetGunfightHUD())
		{
			CharacterOverlayWidget->SetVisibility(true);
			VRStereoLayer->SetVisibility(true);
		}
		else
		{
			bStereoLayerInitialized = true;
		}
	}
	else
	{
		//bStereoLayerInitialized = true;
	}

	//GetWorldTimerManager().SetTimer(CheckSlot2Timer, this, &AGunfightCharacter::CheckSlot2TimerFinished, 0.2f, true);
}

void AGunfightCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &AGunfightCharacter::MoveForward);
	PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &AGunfightCharacter::MoveRight);
	PlayerInputComponent->BindAxis(TEXT("TurnRight"), this, &AGunfightCharacter::TurnRight);
	PlayerInputComponent->BindAxis(TEXT("LookUp"), this, &AGunfightCharacter::LookUp);

	PlayerInputComponent->BindAction<FLeftRightButtonDelegate>("LeftGrip", IE_Pressed, this, &AGunfightCharacter::GripPressed, true);
	PlayerInputComponent->BindAction<FLeftRightButtonDelegate>("LeftGrip", IE_Released, this, &AGunfightCharacter::GripReleased, true);
	PlayerInputComponent->BindAction<FLeftRightButtonDelegate>("RightGrip", IE_Pressed, this, &AGunfightCharacter::GripPressed, false);
	PlayerInputComponent->BindAction<FLeftRightButtonDelegate>("RightGrip", IE_Released, this, &AGunfightCharacter::GripReleased, false);
	PlayerInputComponent->BindAction<FLeftRightButtonDelegate>("LeftTrigger", IE_Pressed, this, &AGunfightCharacter::TriggerPressed, true);
	PlayerInputComponent->BindAction<FLeftRightButtonDelegate>("LeftTrigger", IE_Released, this, &AGunfightCharacter::TriggerReleased, true);
	PlayerInputComponent->BindAction<FLeftRightButtonDelegate>("RightTrigger", IE_Pressed, this, &AGunfightCharacter::TriggerPressed, false);
	PlayerInputComponent->BindAction<FLeftRightButtonDelegate>("RightTrigger", IE_Released, this, &AGunfightCharacter::TriggerReleased, false);
	PlayerInputComponent->BindAction<FLeftRightButtonDelegate>("AButton", IE_Pressed, this, &AGunfightCharacter::AButtonPressed, false);
	PlayerInputComponent->BindAction<FLeftRightButtonDelegate>("AButton", IE_Released, this, &AGunfightCharacter::AButtonReleased, false);
	PlayerInputComponent->BindAction<FLeftRightButtonDelegate>("XButton", IE_Pressed, this, &AGunfightCharacter::AButtonPressed, true);
	PlayerInputComponent->BindAction<FLeftRightButtonDelegate>("XButton", IE_Released, this, &AGunfightCharacter::AButtonReleased, true);
	PlayerInputComponent->BindAction<FLeftRightButtonDelegate>("BButton", IE_Pressed, this, &AGunfightCharacter::BButtonPressed, false);
	PlayerInputComponent->BindAction<FLeftRightButtonDelegate>("BButton", IE_Released, this, &AGunfightCharacter::BButtonReleased, false);
	PlayerInputComponent->BindAction<FLeftRightButtonDelegate>("YButton", IE_Pressed, this, &AGunfightCharacter::BButtonPressed, true);
	PlayerInputComponent->BindAction<FLeftRightButtonDelegate>("YButton", IE_Released, this, &AGunfightCharacter::BButtonReleased, true);
	PlayerInputComponent->BindAction("MenuButton", IE_Pressed, this, &AGunfightCharacter::MenuButtonPressed);

	PlayerInputComponent->BindAction("LeftStickPressed", IE_Pressed, this, &AGunfightCharacter::LeftStickPressed);
	PlayerInputComponent->BindAction("LeftStickPressed", IE_Released, this, &AGunfightCharacter::LeftStickReleased);

	PlayerInputComponent->BindAction("RightStickPressed", IE_Pressed, this, &AGunfightCharacter::RightStickPressed);
	PlayerInputComponent->BindAction("RightStickPressed", IE_Released, this, &AGunfightCharacter::RightStickReleased);
}

void AGunfightCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateAnimation(DeltaTime);
	PollInit();
	UpdateAverageMotionControllerVelocities();

	CheckSlot2TimerFinished();

	/*
	if (IsLocallyControlled() && Combat)
	{
		if (Combat->LeftEquippedWeapon && Combat->LeftEquippedWeapon->Slot1MotionController && Combat->LeftEquippedWeapon->Slot1MotionController == LeftMotionController)
		{
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, DeltaTime * 1.1f, FColor::Green, FString("Attach Left Hand Mesh to slot 1"));
		}
		else if (Combat->LeftEquippedWeapon && Combat->LeftEquippedWeapon->Slot2MotionController && Combat->LeftEquippedWeapon->Slot2MotionController == LeftMotionController)
		{
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, DeltaTime * 1.1f, FColor::Green, FString("Attach Left Hand Mesh to slot 2"));
		}
		else
		{
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, DeltaTime * 1.1f, FColor::Green, FString("Attach Left Hand Mesh to MOTION CONTROLLER"));
		}

		if (Combat->RightEquippedWeapon && Combat->RightEquippedWeapon->Slot1MotionController && Combat->RightEquippedWeapon->Slot1MotionController == RightMotionController)
		{
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, DeltaTime * 1.1f, FColor::Green, FString("Attach Right Hand Mesh to slot 1"));
		}
		else if (Combat->RightEquippedWeapon && Combat->RightEquippedWeapon->Slot2MotionController && Combat->RightEquippedWeapon->Slot2MotionController == RightMotionController)
		{
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, DeltaTime * 1.1f, FColor::Green, FString("Attach Right Hand Mesh to slot 2"));
		}
		else
		{
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, DeltaTime * 1.1f, FColor::Green, FString("Attach Right Hand Mesh to MOTION CONTROLLER"));
		}

		//bRightHandAttached = Combat->RightEquippedWeapon && Combat->RightEquippedWeapon->GetAttachmentReplication().AttachComponent.GetName()
		//if (Combat->RightEquippedWeapon)
		//{
		//	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, DeltaTime * 1.1f, FColor::Green, FString::Printf(TEXT("RightHandAttached: %s"), *Combat->RightEquippedWeapon->GetAttachmentReplication().AttachComponent.GetName()));
		//}
		//else
		//{
		//	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, DeltaTime * 1.1f, FColor::Green, FString::Printf(TEXT("RightHandAttached: NONE")));
		//}
		
		//if (GEngine) GEngine->AddOnScreenDebugMessage(-1, DeltaTime * 1.1f, FColor::Green, FString::Printf(TEXT("LeftHandAttached: %d"), Combat->LeftEquippedWeapon && Combat->LeftEquippedWeapon->GetAttachmentReplication().AttachComponent == LeftMotionController));
	}
	*/

	//if(GEngine) GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, FString::Printf(TEXT("Overlap: %d"), GetOverlappingWeapon()));
}

void AGunfightCharacter::MoveForward(float Throttle)
{
	if (VRReplicatedCamera == nullptr || bDisableMovement) return;

	if (FMath::Abs(Throttle) < 0.15f) return; // deadzone

	FVector CameraForwardVector = VRReplicatedCamera->GetForwardVector();
	CameraForwardVector.Z = 0.f;
	CameraForwardVector = CameraForwardVector.GetSafeNormal();

	AddMovementInput(CameraForwardVector * Throttle);
}

void AGunfightCharacter::MoveRight(float Throttle)
{
	if (VRReplicatedCamera == nullptr || bDisableMovement) return;
	if (FMath::Abs(Throttle) < 0.15f) return; // deadzone
	FVector CameraRightVector = VRReplicatedCamera->GetRightVector();
	CameraRightVector.Z = 0.f;
	CameraRightVector = CameraRightVector.GetSafeNormal();
	AddMovementInput(CameraRightVector * Throttle);
}

void AGunfightCharacter::TurnRight(float Throttle)
{
	if (World == nullptr || VRMovementReference == nullptr) return;
	if (FMath::Abs(Throttle) < 0.15f) return; // deadzone

	if (bSnapTurning)
	{
		float AbsThrottle = FMath::Abs(Throttle);
		if (bSnapped && AbsThrottle < 0.25f)
		{
			bSnapped = false;
		}
		else if(!bSnapped && AbsThrottle > 0.65f)
		{
			bSnapped = true;
			const float AngleSign = Throttle > 0 ? 1.f : -1.f;
			VRMovementReference->PerformMoveAction_SnapTurn(45.f * AngleSign);
		}
	}
	else
	{
		float ScaledSens = TurnSensitivity * 16.f + 20.f; // TurnSensitivity of 5 is ScaledSens = 100.f;
		AddControllerYawInput(Throttle * GetWorld()->DeltaTimeSeconds * ScaledSens);
	}
}

void AGunfightCharacter::LookUp(float Throttle)
{
	if (World == nullptr) return;
	if (FMath::Abs(Throttle) < 0.15f) return; // deadzone

	AddControllerPitchInput(Throttle * GetWorld()->DeltaTimeSeconds * 100.f);
}

void AGunfightCharacter::HandleWeaponGripped(bool bLeftController, AWeapon* WeaponGripped)
{
	if (WeaponGripped == nullptr || Combat == nullptr) return;

	AWeapon* EquippedWeaponOther = bLeftController ? Combat->RightEquippedWeapon : Combat->LeftEquippedWeapon;

	// other hand is already grabbing the weapon.
	if (EquippedWeaponOther && WeaponGripped == EquippedWeaponOther)
	{
		// Attach hand mesh to weapon and apply relevant offsets
		// Make both hands determine weapon rotation

		if (HasAuthority())
		{
			Combat->EquipWeaponSecondary(WeaponGripped, bLeftController);
		}
		else
		{
			bLeftController ? ServerEquipWeaponSecondaryLeft() : ServerEquipWeaponSecondaryRight();
		}

		return;
	}

	// first hand grabbing weapon
	if (HasAuthority())
	{
		Combat->EquipWeapon(WeaponGripped, bLeftController);
	}
	else
	{
		if (bLeftController) ServerEquipButtonPressedLeft();
		else ServerEquipButtonPressedRight();
	}
}

void AGunfightCharacter::HandleWeaponReleased(bool bLeftController, AWeapon* WeaponReleased)
{
	if (WeaponReleased == nullptr || Combat == nullptr) return;
	AWeapon* EquippedWeaponOther = bLeftController ? Combat->RightEquippedWeapon : Combat->LeftEquippedWeapon;
	// handle what happens when dropped and the other hand is still gripping the weapon.
	if (EquippedWeaponOther && EquippedWeaponOther == WeaponReleased)
	{
		bool bDropWeaponSecondary = false; // true if we dropped the weapon from slot2 while slot1 is still being grabbed.
		bool bDropWeaponPrimary = false; // true if we dropped the weapon while we were holding slot1 and slot2 is still being grabbed.
		if (bLeftController)
		{
			// secondary hand dropped the weapon
			if (EquippedWeaponOther->Slot2MotionController && EquippedWeaponOther->Slot2MotionController == LeftMotionController)
			{
				bDropWeaponSecondary = true;
			} // primary hand dropped the weapon with the secondary hand still gripping
			else if (EquippedWeaponOther->Slot1MotionController && EquippedWeaponOther->Slot1MotionController == LeftMotionController)
			{
				bDropWeaponPrimary = true;
			}
		}
		else
		{
			// secondary hand dropped the weapon
			if (EquippedWeaponOther->Slot2MotionController && EquippedWeaponOther->Slot2MotionController == RightMotionController)
			{
				bDropWeaponSecondary = true;
			} // primary hand dropped the weapon with the secondary hand still gripping
			else if (EquippedWeaponOther->Slot1MotionController && EquippedWeaponOther->Slot1MotionController == RightMotionController)
			{
				bDropWeaponPrimary = true;
			}
		}

		if (HasAuthority())
		{
			if (bDropWeaponSecondary)
			{
				Combat->DropWeaponSecondary(WeaponReleased, bLeftController);
			}
			else if (bDropWeaponPrimary)
			{
				Combat->DropWeaponPrimary(WeaponReleased, bLeftController);
			}
		}
		else
		{
			if (bDropWeaponSecondary)
			{
				bLeftController ? ServerDropWeaponSecondaryLeft() : ServerDropWeaponSecondaryRight();
			}
			else if(bDropWeaponPrimary)
			{
				bLeftController ? ServerDropWeaponPrimaryLeft() : ServerDropWeaponPrimaryRight();
			}
		}

		return;
	}

	if (!Combat->CheckEquippedWeapon(bLeftController)) return;

	if (HasAuthority())
	{
		Combat->DropWeapon(bLeftController);
	}
	else
	{
		if (bLeftController) ServerDropWeaponLeft();
		else ServerDropWeaponRight();
	}
}

AWeapon* AGunfightCharacter::GetClosestOverlappingWeapon(bool bLeft)
{
	const UMotionControllerComponent* CurrentMotionController = bLeft ? LeftMotionController : RightMotionController;
	if (CurrentMotionController == nullptr) return nullptr;

	const TArray<AWeapon*>& CurrentOverlappingWeapons = bLeft ? LeftOverlappingWeapons : RightOverlappingWeapons;
	if (CurrentOverlappingWeapons.IsEmpty()) return nullptr;

	const FVector MCL = CurrentMotionController->GetComponentLocation();

	AWeapon* ClosestWeapon = nullptr;
	float ClosestDistance = 400000000.f;
	
	for (auto i : CurrentOverlappingWeapons)
	{
		if (i == nullptr) continue;
		const float Distance = FVector::DistSquared(MCL, i->GetActorLocation());
		if (Distance < ClosestDistance)
		{
			ClosestWeapon = i;
			ClosestDistance = Distance;
		}
	}

	return ClosestWeapon;
}

void AGunfightCharacter::GripCooldownLeftFinished()
{
	bGripCooldownLeft = false;
	if (bPressingGripLeft)
	{
		if (!bGripPressedLastLeft)
		{
			GripPressed(true);
		}
	}
	else
	{
		if (bGripPressedLastLeft)
		{
			GripReleased(true);
		}
	}
}

void AGunfightCharacter::GripCooldownRightFinished()
{
	bGripCooldownRight = false;
	if (bPressingGripRight)
	{
		if (!bGripPressedLastRight)
		{
			GripPressed(false);
		}
	}
	else
	{
		if (bGripPressedLastRight)
		{
			GripReleased(false);
		}
	}
}

void AGunfightCharacter::GripPressed(bool bLeftController)
{
	// grip cooldown check
	if (bLeftController)
	{
		bPressingGripLeft = true;
		if (bGripCooldownLeft) return;
		bGripCooldownLeft = true;
		bGripPressedLastLeft = true;
		GetWorldTimerManager().SetTimer(GripLeftTimer, this, &AGunfightCharacter::GripCooldownLeftFinished, GripCooldownTime, false);
	}
	else
	{
		bPressingGripRight = true;
		if (bGripCooldownRight) return;
		bGripCooldownRight = true;
		bGripPressedLastRight = true;
		GetWorldTimerManager().SetTimer(GripRightTimer, this, &AGunfightCharacter::GripCooldownRightFinished, GripCooldownTime, false);
	}
	//

	if (bDisableGameplay) return;
	if (Combat)
	{
		AWeapon* CurrentWeapon = bLeftController ? Combat->LeftEquippedWeapon : Combat->RightEquippedWeapon;
		if (CurrentWeapon)
		{
			return;
		}

		bool bOverlappingWeapon = false;
		bool bOverlappingMagazine = false;

		AWeapon* WeaponToCheck = bLeftController ? LeftOverlappingWeapon : RightOverlappingWeapon;
		if (WeaponToCheck) // weapon check
		{
			bOverlappingWeapon = true;
		}
		if (OverlappingMagazine && !OverlappingMagazine->bEquipped && OverlappingMagazine->CheckHandOverlap(bLeftController)) // magazine check
		{
			bOverlappingMagazine = true;
		}

		if (bOverlappingWeapon && bOverlappingMagazine)
		{
			UMotionControllerComponent* CurrentMotionController = bLeftController ? LeftMotionController : RightMotionController;
			IsGunCloserThanMag(WeaponToCheck, OverlappingMagazine, CurrentMotionController) ? bOverlappingMagazine = false : bOverlappingWeapon = false;
		}

		if (bOverlappingWeapon)
		{
			if (HasAuthority())
			{
				Combat->GripWeapon(GetClosestOverlappingWeapon(bLeftController), bLeftController);
			}
			else
			{
				ServerGripWeapon(bLeftController);
			}

			CurrentWeapon = bLeftController ? Combat->LeftEquippedWeapon : Combat->RightEquippedWeapon;
			if (CurrentWeapon)
			{
				return; // so the server returns here if they grabbed the weapon.
			}
			//HandleWeaponGripped(bLeftController, GetClosestOverlappingWeapon(bLeftController));
			//return;
		}
		else if (bOverlappingMagazine)
		{
			Combat->EquipMagazine(OverlappingMagazine, bLeftController);
		}

		USkeletalMeshComponent* CurrentHandMesh = bLeftController ? LeftHandMesh : RightHandMesh;
		if (CurrentHandMesh && CurrentHandMesh->GetAttachParentActor()->IsA(AWeapon::StaticClass())) return;

		SetHandState(bLeftController, EHandState::EHS_Grip);
	}
}

void AGunfightCharacter::GripReleased(bool bLeftController)
{
	// release cooldown check
	if (bLeftController)
	{
		bPressingGripLeft = false;
		if (bGripCooldownLeft) return;
		bGripCooldownLeft = true;
		bGripPressedLastLeft = false;
		GetWorldTimerManager().SetTimer(GripLeftTimer, this, &AGunfightCharacter::GripCooldownLeftFinished, GripCooldownTime, false);
	}
	else
	{
		bPressingGripRight = false;
		if (bGripCooldownRight) return;
		bGripCooldownRight = true;
		bGripPressedLastRight = false;
		GetWorldTimerManager().SetTimer(GripRightTimer, this, &AGunfightCharacter::GripCooldownRightFinished, GripCooldownTime, false);
	}
	//

	if (bDisableGameplay) return;
	if (Combat)
	{
		AWeapon* CurrentWeapon = bLeftController ? Combat->LeftEquippedWeapon : Combat->RightEquippedWeapon;
		AWeapon* PreviousWeapon = bLeftController ? Combat->PreviousLeftEquippedWeapon : Combat->PreviousRightEquippedWeapon;
		UMotionControllerComponent* CurrentMotionController = bLeftController ? LeftMotionController : RightMotionController;
		if (CurrentWeapon)
		{	
			//HandleWeaponReleased(bLeftController, CurrentWeapon);
			if (HasAuthority())
			{
				Combat->ReleaseWeapon(bLeftController);
			}
			else
			{
				ServerReleaseWeapon(bLeftController);
			}

			if (CurrentWeapon->Slot1MotionController == CurrentMotionController || CurrentWeapon->Slot2MotionController == CurrentMotionController)
			{
				return;
			}
			else if (PreviousWeapon)
			{
				if (PreviousWeapon->Slot1MotionController == CurrentMotionController || PreviousWeapon->Slot2MotionController == CurrentMotionController)
				{
					return;
				}
			}
		}
		else if (Combat->CheckEquippedMagazine(bLeftController))
		{
			Combat->DropMagazine(bLeftController);
		}
		else if (PreviousWeapon)
		{
			if (PreviousWeapon->Slot1MotionController == CurrentMotionController || PreviousWeapon->Slot2MotionController == CurrentMotionController)
			{
				return;
			}
		}

		USkeletalMeshComponent* CurrentHandMesh = bLeftController ? LeftHandMesh : RightHandMesh;
		if (CurrentHandMesh && CurrentHandMesh->GetAttachParentActor()->IsA(AWeapon::StaticClass())) return;
	}
	SetHandState(bLeftController, EHandState::EHS_Idle);
}

void AGunfightCharacter::TriggerPressed(bool bLeftController)
{
	if (bLeftController) LeftTriggerPressedUI();
	else RightTriggerPressedUI();

	if (bDisableGameplay || bDisableShooting) return;

	if (Combat)
	{
		AWeapon* CurrentWeapon = bLeftController ? Combat->LeftEquippedWeapon : Combat->RightEquippedWeapon;
		if (CurrentWeapon)
		{
			Combat->FireButtonPressed(true, bLeftController);
		}
	}
}

void AGunfightCharacter::TriggerReleased(bool bLeftController)
{
	if (bLeftController)
	{
		LeftTriggerReleasedUI();
	}
	else
	{
		RightTriggerReleasedUI();
	}
}

void AGunfightCharacter::AButtonPressed(bool bLeftController)
{
	if (Combat)
	{
		AWeapon* CurrentWeapon = bLeftController ? Combat->LeftEquippedWeapon : Combat->RightEquippedWeapon;
		if (CurrentWeapon && CurrentWeapon->IsMagInserted() && CurrentWeapon->GetAmmo() != CurrentWeapon->GetMagCapacity() && CurrentWeapon->GetCarriedAmmo() > 0 && CurrentWeapon->GetWeaponMesh())
		{
			if (Combat->LeftEquippedWeapon && Combat->LeftEquippedWeapon->GetMagazine())
			{
				Combat->LeftEquippedWeapon->GetMagazine()->Destroy();
				Combat->LeftEquippedWeapon->SetMagazine(nullptr);
				Combat->LeftEquippedWeapon->SetMagInserted(true);
				Combat->LeftEquippedWeapon->UnhideMag();
				Combat->LeftEquippedWeapon->ClearWeaponMagTimer();
			}
			if (Combat->RightEquippedWeapon && Combat->RightEquippedWeapon->GetMagazine())
			{
				Combat->RightEquippedWeapon->GetMagazine()->Destroy();
				Combat->RightEquippedWeapon->SetMagazine(nullptr);
				Combat->RightEquippedWeapon->SetMagInserted(true);
				Combat->RightEquippedWeapon->UnhideMag();
				Combat->RightEquippedWeapon->ClearWeaponMagTimer();
			}

			CurrentWeapon->EjectMagazine();
			SpawnFullMagazine(CurrentWeapon->GetFullMagazineClass(), CurrentWeapon->GetCurrentSkinIndex(), bLeftController);
		}
	}
}

void AGunfightCharacter::AButtonReleased(bool bLeftController)
{
}

void AGunfightCharacter::BButtonPressed(bool bLeftController)
{

}

void AGunfightCharacter::BButtonReleased(bool bLeftController)
{
}

void AGunfightCharacter::LeftStickPressed()
{
	bPressingLeftStick = true;
	// show scoreboard
	GunfightPlayerController = GunfightPlayerController == nullptr ? Cast<AGunfightPlayerController>(Controller) : GunfightPlayerController;
	if (GunfightPlayerController == nullptr) return;
	GunfightPlayerController->SetScoreboardVisibility(true);
}

void AGunfightCharacter::LeftStickReleased()
{
	bPressingLeftStick = false;
	// hide scoreboard
	GunfightPlayerController = GunfightPlayerController == nullptr ? Cast<AGunfightPlayerController>(Controller) : GunfightPlayerController;
	if (GunfightPlayerController == nullptr) return;
	GunfightPlayerController->SetScoreboardVisibility(false);

	if (bPressingRightStick)
	{
		GetWorldTimerManager().ClearTimer(TeamSwapTimer);
	}
}

void AGunfightCharacter::RightStickPressed()
{
	bPressingRightStick = true;
	if (bPressingLeftStick)
	{
		GetWorldTimerManager().SetTimer(TeamSwapTimer, this, &AGunfightCharacter::TeamSwapTimerFinished, 1.f, false, 1.f);
		// start timer to send the TeamSwapRequest if match state is warmup
	}
}

void AGunfightCharacter::RightStickReleased()
{
	bPressingRightStick = false;
	if (bPressingLeftStick)
	{
		GetWorldTimerManager().ClearTimer(TeamSwapTimer);
	}
}

void AGunfightCharacter::TeamSwapTimerFinished()
{
	if (bPressingLeftStick && bPressingRightStick)
	{
		GunfightPlayerController = GunfightPlayerController == nullptr ? Cast<AGunfightPlayerController>(Controller) : GunfightPlayerController;
		if (GunfightPlayerController)
		{
			GunfightPlayerState = GunfightPlayerState == nullptr ? GetPlayerState<AGunfightPlayerState>() : GunfightPlayerState;
			if (GunfightPlayerState && !GunfightPlayerState->HasRequestedTeamSwap())
			{
				GunfightPlayerController->UpdateTeamSwapText(FString("Team swap requested."));
				GunfightPlayerState->RequestTeamSwap();
			}
		}
	}
}

void AGunfightCharacter::TryInitializeLocalHands()
{
	if (IsLocallyControlled() == false) return;
	GunfightPlayerController = GunfightPlayerController == nullptr ? Cast<AGunfightPlayerController>(Controller) : GunfightPlayerController;
	USkeletalMeshComponent* BodyMesh = GetMesh();
	if (BodyMesh == nullptr || HandMesh == nullptr) return;
	SetLocalArmsVisible(false);

	RightHandMesh = NewObject<USkeletalMeshComponent>(this);
	if (RightHandMesh == nullptr) return;

	RightHandMesh->RegisterComponent();
	//RightHandMesh->AttachToComponent(RightMotionController, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	RightHandMesh->AttachToComponent(RightHandMeshOffset, FAttachmentTransformRules::SnapToTargetNotIncludingScale);

	RightHandMesh->SetSkeletalMesh(HandMesh);
	RightHandMesh->SetCastShadow(false);
	RightHandMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	//RightHandMesh->SetRelativeLocationAndRotation(RightHandMeshLocationOffset, RightHandMeshRotationOffset);
	RightHandMesh->InitLODInfos();
	RightHandMesh->OverrideMinLOD(0);
	InitializeHandAnimation(RightHandMesh);
	RightHandAnim = Cast<UHandAnimInstance>(RightHandMesh->GetAnimInstance());

	LeftHandMesh = NewObject<USkeletalMeshComponent>(this);
	if (LeftHandMesh == nullptr) return;

	LeftHandMesh->RegisterComponent();
	//LeftHandMesh->AttachToComponent(LeftMotionController, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	LeftHandMesh->AttachToComponent(LeftHandMeshOffset, FAttachmentTransformRules::SnapToTargetNotIncludingScale);

	LeftHandMesh->SetSkeletalMesh(HandMesh);
	LeftHandMesh->SetCastShadow(false);
	LeftHandMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	//LeftHandMesh->SetRelativeLocationAndRotation(LeftHandMeshLocationOffset, LeftHandMeshRotationOffset);
	LeftHandMesh->SetWorldScale3D(FVector(1.f, -1.f, 1.f));
	LeftHandMesh->InitLODInfos();
	LeftHandMesh->OverrideMinLOD(0);
	InitializeHandAnimation(LeftHandMesh);
	LeftHandAnim = Cast<UHandAnimInstance>(RightHandMesh->GetAnimInstance());

	bLocalHandsInitialized = true;
}

void AGunfightCharacter::SetLocalArmsVisible(bool bVisible)
{
	if (IsLocallyControlled() == false) return;
	USkeletalMeshComponent* BodyMesh = GetMesh();
	if (BodyMesh == nullptr) return;

	if (bVisible)
	{
		BodyMesh->UnHideBoneByName(FName("upperarm_l"));
		BodyMesh->UnHideBoneByName(FName("upperarm_r"));
	}
	else
	{
		BodyMesh->HideBoneByName(FName("upperarm_l"), EPhysBodyOp::PBO_None);
		BodyMesh->HideBoneByName(FName("upperarm_r"), EPhysBodyOp::PBO_None);
	}
}

void AGunfightCharacter::MenuButtonPressed()
{
	ToggleMenu();
}

void AGunfightCharacter::UpdateAnimInstanceIK(float DeltaTime)
{
	if (GunfightAnimInstance == nullptr || LeftMotionController == nullptr || RightMotionController == nullptr) return;

	// Upper Body IK (UBIK) variables
	GunfightAnimInstance->HeadTransform = VRReplicatedCamera->GetComponentTransform();

	FTransform GoalLeftHandTransform;
	FTransform GoalRightHandTransform;

	if (LeftHandIKComponent && RightHandIKComponent)
	{
		//GunfightAnimInstance->LeftHandTransform = LeftHandIKComponent->GetComponentTransform();
		//GunfightAnimInstance->RightHandTransform = RightHandIKComponent->GetComponentTransform();
		GoalLeftHandTransform = LeftHandIKComponent->GetComponentTransform();
		GoalRightHandTransform = RightHandIKComponent->GetComponentTransform();
	}
	else
	{
		//GunfightAnimInstance->LeftHandTransform = LeftMotionController->GetComponentTransform();
		//GunfightAnimInstance->RightHandTransform = RightMotionController->GetComponentTransform();
		GoalLeftHandTransform = LeftMotionController->GetComponentTransform();
		GoalRightHandTransform = RightMotionController->GetComponentTransform();
	}
	//GunfightAnimInstance->LeftHandTransform = FMath::Lerp(GunfightAnimInstance->LeftHandTransform, GoalLeftHandTransform, DeltaTime * 20.f);
	//GunfightAnimInstance->RightHandTransform = FMath::Lerp(GunfightAnimInstance->RightHandTransform, GoalRightHandTransform, DeltaTime * 20.f);

	// lerp location
	//FMath::Lerp(GunfightAnimInstance->LeftHandTransform.GetLocation(), GoalLeftHandTransform.GetLocation(), DeltaTime * 30.f)
	// FMath::VInterpTo(GunfightAnimInstance->LeftHandTransform.GetLocation(), GoalLeftHandTransform.GetLocation(), DeltaTime, 30.f)
	GunfightAnimInstance->LeftHandTransform.SetLocation(FMath::Lerp(GunfightAnimInstance->LeftHandTransform.GetLocation(), GoalLeftHandTransform.GetLocation(), DeltaTime * 40.f));
	GunfightAnimInstance->RightHandTransform.SetLocation(FMath::Lerp(GunfightAnimInstance->RightHandTransform.GetLocation(), GoalRightHandTransform.GetLocation(), DeltaTime * 40.f));
	// slerp rotation
	GunfightAnimInstance->LeftHandTransform.SetRotation(FQuat::Slerp(GunfightAnimInstance->LeftHandTransform.GetRotation(), GoalLeftHandTransform.GetRotation(), DeltaTime * 30.f));
	GunfightAnimInstance->RightHandTransform.SetRotation(FQuat::Slerp(GunfightAnimInstance->RightHandTransform.GetRotation(), GoalRightHandTransform.GetRotation(), DeltaTime * 30.f));

	//GunfightAnimInstance->LeftHandTransform = LeftMotionController->GetComponentTransform();
	//GunfightAnimInstance->RightHandTransform = RightMotionController->GetComponentTransform();

	/*
	GunfightAnimInstance->LeftHandComponent = LeftMotionController;
	GunfightAnimInstance->RightHandComponent = RightMotionController;

	
	if (Combat && Combat->LeftEquippedWeapon && Combat->LeftEquippedWeapon->Slot2MotionController)
	{
		if (Combat->LeftEquippedWeapon->Slot2MotionController == LeftMotionController)
		{
			GunfightAnimInstance->LeftHandComponent = Combat->LeftEquippedWeapon->GrabSlot2IKL;
		}
		else
		{
			GunfightAnimInstance->RightHandComponent = Combat->RightEquippedWeapon->GrabSlot2IKR;
		}
	}
	*/

	// Leg IK
	GunfightAnimInstance->TraceLeftFootLocation = TraceFootLocationIK(true);
	GunfightAnimInstance->TraceRightFootLocation = TraceFootLocationIK(false);
}

void AGunfightCharacter::UpdateAnimation(float DeltaTime)
{
	if (bElimmed) return;
	MoveMeshToCamera();
	UpdateAnimInstanceIK(DeltaTime);
}

void AGunfightCharacter::ServerGripWeapon_Implementation(bool bLeft)
{
	if (Combat)
	{
		Combat->GripWeapon(GetClosestOverlappingWeapon(bLeft), bLeft);
	}
}

void AGunfightCharacter::ServerReleaseWeapon_Implementation(bool bLeft)
{
	if (Combat)
	{
		AWeapon* CurrentWeapon = bLeft ? Combat->LeftEquippedWeapon : Combat->RightEquippedWeapon;
		if (CurrentWeapon)
		{
			Combat->ReleaseWeapon(bLeft);
		}
	}
}

void AGunfightCharacter::ServerEquipButtonPressedLeft_Implementation()
{
	if (Combat)
	{
		Combat->EquipWeapon(GetClosestOverlappingWeapon(true), true);
	}
}

void AGunfightCharacter::ServerEquipButtonPressedRight_Implementation()
{
	if (Combat)
	{
		Combat->EquipWeapon(GetClosestOverlappingWeapon(false), false);
	}
}

void AGunfightCharacter::ServerEquipWeaponSecondaryLeft_Implementation()
{
	if (Combat)
	{
		Combat->EquipWeaponSecondary(GetClosestOverlappingWeapon(true), true);
	}
}

void AGunfightCharacter::ServerEquipWeaponSecondaryRight_Implementation()
{
	if (Combat)
	{
		Combat->EquipWeaponSecondary(GetClosestOverlappingWeapon(false), false);
	}
}

void AGunfightCharacter::ServerDropWeaponLeft_Implementation()
{
	if(Combat)
	{
		Combat->DropWeapon(true);
	}
}

void AGunfightCharacter::ServerDropWeaponRight_Implementation()
{
	if (Combat)
	{
		Combat->DropWeapon(false);
	}
}

void AGunfightCharacter::ServerDropWeaponPrimaryLeft_Implementation()
{
	if (Combat) Combat->DropWeaponPrimary(Combat->LeftEquippedWeapon, true);
}

void AGunfightCharacter::ServerDropWeaponPrimaryRight_Implementation()
{
	if (Combat) Combat->DropWeaponPrimary(Combat->RightEquippedWeapon, false);
}

void AGunfightCharacter::ServerDropWeaponSecondaryLeft_Implementation()
{
	if (Combat) Combat->DropWeaponSecondary(Combat->LeftEquippedWeapon, true);
}

void AGunfightCharacter::ServerDropWeaponSecondaryRight_Implementation()
{
	if (Combat) Combat->DropWeaponSecondary(Combat->RightEquippedWeapon, false);
}

// I would like to clean this mess up
void AGunfightCharacter::PollInit()
{
	if (GunfightPlayerState == nullptr)
	{
		GunfightPlayerState = GetPlayerState<AGunfightPlayerState>();
		if (GunfightPlayerState)
		{
			SetTeamColor(GunfightPlayerState->GetTeam());
		}
	}
	if (GunfightAnimInstance && !GunfightAnimInstance->GetCharacter() && GetMesh())
	{
		GunfightAnimInstance = Cast<UGunfightAnimInstance>(GetMesh()->GetAnimInstance());
		if (GunfightAnimInstance)
		{
			GunfightAnimInstance->SetCharacter(this);
			if (IsLocallyControlled())
			{
				GunfightAnimInstance->SetLocallyControlled(true);
			}
		}
	}
	else if (!GunfightAnimInstance)
	{
		if (GetMesh() && LeftMotionController && RightMotionController)
		{
			GunfightAnimInstance = Cast<UGunfightAnimInstance>(GetMesh()->GetAnimInstance());
			if (GunfightAnimInstance)
			{
				GunfightAnimInstance->LeftHandComponent = LeftMotionController;
				GunfightAnimInstance->RightHandComponent = RightMotionController;
			}
		}
	}


	if (GunfightPlayerController == nullptr || bPlayerInitialized == false)
	{
		GunfightPlayerController = GunfightPlayerController == nullptr ? Cast<AGunfightPlayerController>(Controller) : GunfightPlayerController;
		if (GunfightPlayerController)
		{
			SpawnDefaultWeapon();
			InitializeHUD();
			//UpdateHUDAmmo();
			UpdateHUDHealth();
			if (IsLocallyControlled())
			{
				InitializeSettings();
			}
			bPlayerInitialized = true;
		}
	}
	else if(!GunfightPlayerController->GetCharacterOverlay())
	{
		InitializeHUD();
		//UpdateHUDAmmo();
		UpdateHUDHealth();
	}

	if (!bLocalHandsInitialized)
	{
		TryInitializeLocalHands();
	}
}

void AGunfightCharacter::OnRep_DefaultWeapon()
{
	if (DefaultWeapon == nullptr || DefaultWeapon->GetWeaponMesh() == nullptr || Combat == nullptr) return;
	DefaultWeapon->CharacterOwner = this;
	DefaultWeapon->SetOwner(this);

	Combat->AttachWeaponToHolster(DefaultWeapon);
	DefaultWeapon->ResetWeapon();

	// Update the server with my weapon skin
	if (!IsLocallyControlled()) return;

	InitMyWeaponSkin(DefaultWeapon);

	/*UGameInstance* GI = UGameplayStatics::GetGameInstance(this);
	if (GI == nullptr) return;
	UGunfightGameInstanceSubsystem* GunfightSubsystem = GI->GetSubsystem<UGunfightGameInstanceSubsystem>();
	if (GunfightSubsystem == nullptr || GunfightSubsystem->GunfightSaveGame == nullptr) return;

	int32 SkinIndex = GunfightSubsystem->GunfightSaveGame->EquippedWeaponSkin;
	DefaultWeapon->ServerSetWeaponSkin(SkinIndex);*/
}

void AGunfightCharacter::ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatorController, AActor* DamageCauser)
{
	GunfightGameMode = GunfightGameMode == nullptr ? GetWorld()->GetAuthGameMode<AGunfightGameMode>() : GunfightGameMode;
	if (bElimmed || GunfightGameMode == nullptr) return;
	Damage = GunfightGameMode->CalculateDamage(InstigatorController, Controller, Damage);

	Health = FMath::Clamp(Health - Damage, 0.f, MaxHealth);
	UpdateHUDHealth();
	//PlayHitReactMontage();
	ReceiveDamageHaptic();

	if (Health == 0.f && !bElimmed)
	{
		GunfightGameMode = GunfightGameMode == nullptr ? GetWorld()->GetAuthGameMode<AGunfightGameMode>() : GunfightGameMode;
		if (GunfightGameMode)
		{
			GunfightPlayerController = GunfightPlayerController == nullptr ? Cast<AGunfightPlayerController>(Controller) : GunfightPlayerController;
			AGunfightPlayerController* AttackerController = Cast<AGunfightPlayerController>(InstigatorController);
			GunfightGameMode->PlayerEliminated(this, GunfightPlayerController, AttackerController);
		}
	}
}

void AGunfightCharacter::MulticastSpawnBlood_Implementation(const FVector_NetQuantize Location, const FVector_NetQuantize Normal, EHitbox HitType, int32 BoneIndex)
{
	if (GetMesh() == nullptr || BoneIndex == INDEX_NONE) return;
	FName BoneName = GetMesh()->GetBoneName(BoneIndex);
	EHitbox BoxType = FHitbox::GetHitboxType(BoneName);
	//FVector BloodLocation = GetMesh()->GetBoneLocation(BoneName) + Location;
	FTransform BoneTransform = GetMesh()->GetBoneTransform(BoneName, ERelativeTransformSpace::RTS_ParentBoneSpace);
	
	FVector FromBoneLocation;
	FRotator FromBoneRotation;
	GetMesh()->TransformFromBoneSpace(BoneName, Location, BoneTransform.Rotator(), FromBoneLocation, FromBoneRotation);
	FVector BloodScale = FVector::One();

	//DrawDebugSphere(GetWorld(), FromBoneLocation, 5.f, 12, FColor::Purple, true);

	USoundCue* ImpactSound = ImpactBodySound;

	if (BoxType == EHitbox::EH_Head)
	{
		BloodScale *= 1.5f;
		ImpactSound = ImpactHeadSound;
	}
	
	if (BloodParticles)
	{
		UGameplayStatics::SpawnEmitterAtLocation(this, BloodParticles, FromBoneLocation, Normal.Rotation(), BloodScale);
	}
	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, FromBoneLocation);
	}

	/*
	* 
	* if (BloodNiagaraSystem)
	{
		UNiagaraComponent* n = UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, BloodNiagaraSystem, FromBoneLocation, Normal.Rotation());
	}

	if (BoxType == EHitbox::EH_Head)
	{
		if (BloodParticles)
		{
			UGameplayStatics::SpawnEmitterAtLocation(this, BloodParticles, FromBoneLocation, Normal.Rotation(), ((FVector)((2.f))));
		}
		if (ImpactHeadSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, ImpactHeadSound, FromBoneLocation);
		}
		
		return;
	}

	if (BloodParticles)
	{
		UGameplayStatics::SpawnEmitterAtLocation(this, BloodParticles, FromBoneLocation, Normal.Rotation());
	}
	if (ImpactBodySound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactBodySound, FromBoneLocation);
	}*/
}

void AGunfightCharacter::Elim(bool bPlayerLeftGame)
{
	//DropOrDestroyWeapons();
	MultiCastElim(bPlayerLeftGame);
}

void AGunfightCharacter::MultiCastElim_Implementation(bool bPlayerLeftGame)
{
	bElimmed = true;
	// drop equipped weapon/magazine
	if (Combat)
	{
		if (IsLocallyControlled())
		{
			if (Combat->LeftEquippedWeapon || Combat->LeftEquippedMagazine)
			{
				GripReleased(true);
			}
			if (Combat->RightEquippedWeapon || Combat->RightEquippedMagazine)
			{
				GripReleased(false);
			}
			DeathPP();
		}

		// drop all weapons
		if (HasAuthority())
		{
			if (Combat->LeftHolsteredWeapon)
			{
				Combat->LeftHolsteredWeapon->Dropped(1);
				Combat->LeftHolsteredWeapon = nullptr;
			}
			if (Combat->RightHolsteredWeapon)
			{
				Combat->RightHolsteredWeapon->Dropped(1);
				Combat->RightHolsteredWeapon = nullptr;
			}
		}
	}

	if (GunfightPlayerController)
	{
		//GunfightPlayerController->SetHUDWeaponAmmo(0);
	}

	SetLocalArmsVisible(true);

	bLeftGame = false;
	bDisableGameplay = true;
	Ragdoll();

	/*
	//if killed while reloading
	if (DefaultMagazine && DefaultWeapon && Combat)
	{
		DefaultMagazine->Destroy();
		DefaultWeapon->UnhideMag();
		Combat->Reload();
	}
	*/

	//PlayElimMontage()

	/*
	// Disable character movement
	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->StopMovementImmediately();
	if (Combat)
	{
		Combat->FireButtonPressed(false, false);
	}

	// Disable collision
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCapsuleComponent()->SetSimulatePhysics(false);
	MeshCollisionResponses = GetMesh()->GetCollisionResponseToChannels();
	*/

	GetWorldTimerManager().SetTimer(
		ElimTimer,
		this,
		&AGunfightCharacter::ElimTimerFinished,
		5.f
	);
}

void AGunfightCharacter::MulticastRespawn_Implementation(FVector_NetQuantize SpawnLocation, FRotator SpawnRotation, bool bInputDisabled)
{
	Respawn(SpawnLocation, SpawnRotation, bInputDisabled);
}

void AGunfightCharacter::MulticastEnableInput_Implementation()
{
	GunfightPlayerController = GunfightPlayerController == nullptr ? Cast<AGunfightPlayerController>(Controller) : GunfightPlayerController;
	if (GunfightPlayerController)
	{
		EnableInput(GunfightPlayerController);
	}
}

void AGunfightCharacter::Respawn(FVector_NetQuantize SpawnLocation, FRotator SpawnRotation, bool bInputDisabled)
{
	bDisableGameplay = false;
	SetLocalArmsVisible(false);
	UnRagdoll();

	Health = 100.f;

	/*
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetCapsuleComponent()->SetSimulatePhysics(true);

	GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Walking);
	bDisableGameplay = false;
	*/

	SpawnLocation.Z += 5.f; // so we don't spawn in the floor

	SetActorLocationAndRotationVR(SpawnLocation, SpawnRotation, true, true, true);
	
	if (GetCapsuleComponent())
	{
		GetCapsuleComponent()->SetPhysicsLinearVelocity(FVector::ZeroVector);
		GetCapsuleComponent()->SetPhysicsAngularVelocityInRadians(FVector::ZeroVector);
	}
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_None);
		GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Walking);
	}

	if (Combat)
	{
		bool bHasAWeapon = Combat->ResetOwnedWeapons();

		GunfightGameMode = GunfightGameMode == nullptr ? GetWorld()->GetAuthGameMode<AGunfightGameMode>() : GunfightGameMode;
		if (GunfightGameMode && !bHasAWeapon)
		{
			GunfightGameMode->GiveMeWeapon(this, EWeaponType::EWT_Pistol);

			/*UGameInstance* GI = UGameplayStatics::GetGameInstance(this);
			if (GI == nullptr) return;
			UGunfightGameInstanceSubsystem* GunfightSubsystem = GI->GetSubsystem<UGunfightGameInstanceSubsystem>();
			if (GunfightSubsystem == nullptr || GunfightSubsystem->GunfightSaveGame == nullptr) return;

			int32 SkinIndex = GunfightSubsystem->GunfightSaveGame->EquippedWeaponSkin;
			DefaultWeapon->ServerSetWeaponSkin(SkinIndex);*/
		}
	}

	GunfightPlayerController = GunfightPlayerController == nullptr ? Cast<AGunfightPlayerController>(Controller) : GunfightPlayerController;
	if (GunfightPlayerController)
	{
		GunfightPlayerController->SetHUDHealth(MaxHealth, MaxHealth);
	}

	bElimmed = false;

	// turn off death post process
	if (IsLocallyControlled())
	{
		SpawnPP();
	}

	if (bInputDisabled)
	{
		SetDisableMovement(true);
	}
}

void AGunfightCharacter::ElimTimerFinished()
{
	GunfightGameMode = GunfightGameMode == nullptr ? GetWorld()->GetAuthGameMode<AGunfightGameMode>() : GunfightGameMode;

	/*
	if (DefaultWeapon)
	{
		DefaultWeapon->Destroy();
	}
	*/
	
	if (GunfightGameMode && !bLeftGame && bElimmed)
	{
		bool bShouldSpawnTeams = GunfightGameMode->GunfightRoundMatchState == EGunfightRoundMatchState::EGRMS_WaitingForPlayers || GunfightGameMode->GunfightRoundMatchState == EGunfightRoundMatchState::EGRMS_Warmup;
		if (!GunfightGameMode->bTeamsMatch || bShouldSpawnTeams)
		{
			GunfightGameMode->RequestRespawn(this, Controller);
		}
	}
}

void AGunfightCharacter::Ragdoll()
{
	//SetReplicateMovement(false);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECR_Block);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECR_Ignore);
	GetMesh()->SetAllBodiesSimulatePhysics(true);
	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->WakeAllRigidBodies();
	GetMesh()->bBlendPhysics = true;
	//GetCharacterMovement()->SetComponentTickEnabled(false);
}

void AGunfightCharacter::UnRagdoll()
{
	//SetReplicateMovement(true);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECR_Block);
	GetMesh()->SetAllBodiesSimulatePhysics(false);
	GetMesh()->SetSimulatePhysics(false);
	GetMesh()->PutAllRigidBodiesToSleep();
	GetMesh()->bBlendPhysics = false;
	//GetCharacterMovement()->SetComponentTickEnabled(true);
	GetMesh()->SetRelativeLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);
}

void AGunfightCharacter::UpdateHUDHealth()
{
	GunfightPlayerController = GunfightPlayerController == nullptr ? Cast<AGunfightPlayerController>(Controller) : GunfightPlayerController;
	if (GunfightPlayerController)
	{
		GunfightPlayerController->SetHUDHealth(Health, MaxHealth);
	}
}

void AGunfightCharacter::InitializeHUD()
{
	GunfightPlayerController = GunfightPlayerController == nullptr ? Cast<AGunfightPlayerController>(Controller) : GunfightPlayerController;
	if (GunfightPlayerController)
	{
		GunfightPlayerController->InitializeHUD();
	}
}

void AGunfightCharacter::OnRep_Health()
{
	UpdateHUDHealth();
	//PlayHitReactMontage();
	if (Health < 99.f)
	{
		ReceiveDamageHaptic();
	}
}

void AGunfightCharacter::SpawnDefaultWeapon()
{
	if (GEngine)GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Purple, FString("SpawnDefaultWeapon"));
	GunfightGameMode = GunfightGameMode == nullptr ? GetWorld()->GetAuthGameMode<AGunfightGameMode>() : GunfightGameMode;
	World = World == nullptr ? GetWorld() : World;
	if (GunfightGameMode && World && WeaponClass && Combat)
	{
		DefaultWeapon = World->SpawnActor<AWeapon>(WeaponClass);
		if (DefaultWeapon)
		{
			DefaultWeapon->SetOwner(this);
			DefaultWeapon->SetCharacterOwner(this);
			DefaultWeapon->SetHolsterSide(ESide::ES_Right);
			OnRep_DefaultWeapon();
			Combat->RightHolsteredWeapon = DefaultWeapon;
			GunfightGameMode->AddWeaponToPool(DefaultWeapon);
			//GunfightGameMode->GetWeapon
			if (GEngine)GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Purple, FString("SpawnDefaultWeapon 22222"));
		}
	}
}

void AGunfightCharacter::SpawnFullMagazine(TSubclassOf<AFullMagazine> FullMagClass, int32 SkinIndex, bool bLeft)
{
	World = World == nullptr ? GetWorld() : World;
	if (World && Combat)
	{
		AWeapon* CurrentWeapon = bLeft ? Combat->LeftEquippedWeapon : Combat->RightEquippedWeapon;
		if (CurrentWeapon == nullptr) return;

		AFullMagazine* CurrentMagazine = bLeft ? LeftMagazine : RightMagazine;

		CurrentMagazine = World->SpawnActor<AFullMagazine>(FullMagClass);
		if (CurrentMagazine)
		{
			CurrentMagazine->SetWeaponSkin(SkinIndex);
			CurrentMagazine->SetOwner(this);
			CurrentMagazine->SetCharacterOwner(this);
			CurrentMagazine->SetWeaponOwner(CurrentWeapon);
			CurrentWeapon->SetMagazine(CurrentMagazine);

			ESide Side = bLeft ? ESide::ES_Right : ESide::ES_Left;
			AttachMagazineToHolster(CurrentMagazine, Side);
		}
	}

}

void AGunfightCharacter::SetHUDAmmo(int32 Ammo, bool bLeft)
{
	GunfightPlayerController = GunfightPlayerController == nullptr ? Cast<AGunfightPlayerController>(Controller) : GunfightPlayerController;
	if (GunfightPlayerController && IsLocallyControlled())
	{
		GunfightPlayerController->SetHUDWeaponAmmo(Ammo, bLeft);
	}
}

void AGunfightCharacter::SetHUDCarriedAmmo(int32 Ammo, bool bLeft)
{
	GunfightPlayerController = GunfightPlayerController == nullptr ? Cast<AGunfightPlayerController>(Controller) : GunfightPlayerController;
	if (GunfightPlayerController && IsLocallyControlled())
	{
		GunfightPlayerController->SetHUDCarriedAmmo(Ammo, bLeft);
	}
}

UMotionControllerComponent* AGunfightCharacter::GetMotionControllerFromAttachment(FName AttachSocket, USceneComponent* AttachComponent)
{
	FString SocketString = AttachSocket.ToString();
	if (!SocketString.IsEmpty())
	{
		if (SocketString.Contains("Left"))
		{
			return LeftMotionController;
		}
		else if (SocketString.Contains("Right"))
		{
			return RightMotionController;
		}
	}

	if (AttachComponent == LeftMotionController)
	{
		return LeftMotionController;
	}
	else if (AttachComponent == RightMotionController)
	{
		return RightMotionController;
	}

	if (AttachComponent == LeftHandPistolOffset)
	{
		return LeftMotionController;
	}
	else if (AttachComponent == RightHandPistolOffset)
	{
		return RightMotionController;
	}

	return nullptr;
}

bool AGunfightCharacter::GetLeftFromAttachment(FName AttachSocket, USceneComponent* AttachComponent)
{
	FString SocketString = AttachSocket.ToString();
	if (!SocketString.IsEmpty())
	{
		if (SocketString.Contains("Left"))
		{
			return true;
		}
		else if (SocketString.Contains("Right"))
		{
			return false;
		}
	}

	if (AttachComponent == LeftHandPistolOffset)
	{
		return true;
	}
	else if (AttachComponent == RightHandPistolOffset)
	{
		return false;
	}

	return false;
}

void AGunfightCharacter::AttachHandMeshToMotionController(bool bLeft)
{
	if (!IsLocallyControlled()) return;

	USkeletalMeshComponent* HMesh = nullptr;
	USceneComponent* HOffset = nullptr;
	if (bLeft)
	{
		HMesh = LeftHandMesh;
		HOffset = LeftHandMeshOffset;
	}
	else
	{
		HMesh = RightHandMesh;
		HOffset = RightHandMeshOffset;
	}
	if (HMesh == nullptr || HOffset == nullptr) return;
	HMesh->AttachToComponent(HOffset, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
}

void AGunfightCharacter::DebugMagOverlap(bool bLeft)
{
	if (OverlappingMagazine)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, TEXT("OverlappingMagazine"));
		}
	}
	else
	{
		return;
	}
	if (!OverlappingMagazine->bEquipped)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, TEXT("!OverlappingMagazine->bEquipped"));
		}
	}
	if (OverlappingMagazine->CheckHandOverlap(bLeft))
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, TEXT("OverlappingMagazine->CheckHandOverlap(bLeftController)"));
		}
	}
}

void AGunfightCharacter::AttachMagazineToHolster(AFullMagazine* FullMag, ESide HolsterSide)
{
	if (FullMag && GetMesh())
	{
		FName HolsterSocketName = FName("MagHolster");
		if (HolsterSide == ESide::ES_Left) HolsterSocketName = FName("LeftMagHolster");
		else if (HolsterSide == ESide::ES_Right) HolsterSocketName = FName("RightMagHolster");

		const USkeletalMeshSocket* HolsterSocket = GetMesh()->GetSocketByName(HolsterSocketName);
		if (HolsterSocket)
		{
			HolsterSocket->AttachActor(FullMag, GetMesh());
		}
	}
}

void AGunfightCharacter::UpdateHUDAmmo()
{
	GunfightPlayerController = GunfightPlayerController == nullptr ? Cast<AGunfightPlayerController>(Controller) : GunfightPlayerController;
	if (GunfightPlayerController && DefaultWeapon)
	{
		GunfightPlayerController->SetHUDCarriedAmmo(DefaultWeapon->GetCarriedAmmo(), true);
		GunfightPlayerController->SetHUDWeaponAmmo(DefaultWeapon->GetAmmo(), true);
		GunfightPlayerController->SetHUDCarriedAmmo(DefaultWeapon->GetCarriedAmmo(), false);
		GunfightPlayerController->SetHUDWeaponAmmo(DefaultWeapon->GetAmmo(), false);
	}
}

void AGunfightCharacter::SetupUI()
{
	
}

void AGunfightCharacter::UpdateAverageMotionControllerVelocities()
{
	const FVector LeftHCLocation = LeftMotionController->GetComponentLocation();
	const FVector RightHCLocation = RightMotionController->GetComponentLocation();
	const FVector LeftVelocity = LeftHCLocation - LastLeftMotionControllerLocation;
	const FVector RightVelocity = RightHCLocation - LastRightMotionControllerLocation;

	const FVector LeftHCRotation = LeftMotionController->GetComponentQuat().Vector();
	const FVector RightHCRotation = RightMotionController->GetComponentQuat().Vector();
	LeftMotionControllerAverageAngularVelocity = LeftHCRotation - LastLeftMotionControllerRotation;
	RightMotionControllerAverageAngularVelocity = RightHCRotation - LastRightMotionControllerRotation;

	if (LeftMotionControllerVelocities.Num() < 4)
	{
		LeftMotionControllerVelocities.Push(LeftVelocity);
		RightMotionControllerVelocities.Push(RightVelocity);
	}
	else
	{
		LeftMotionControllerVelocities.Pop();
		RightMotionControllerVelocities.Pop();

		LeftMotionControllerVelocities.Insert(LeftVelocity, 0);
		RightMotionControllerVelocities.Insert(RightVelocity, 0);
	}

	//const FVector TraceStart = VRReplicatedCamera->GetComponentLocation() + VRReplicatedCamera->GetForwardVector() * 10.f;
	//DrawDebugLine(GetWorld(), TraceStart, TraceStart + RightMotionControllerAverageVelocity, FColor::Cyan, false, GetWorld()->DeltaTimeSeconds * 1.1f);
	
	LastLeftMotionControllerLocation = LeftHCLocation;
	LastRightMotionControllerLocation = RightHCLocation;
	LastLeftMotionControllerRotation = LeftHCRotation;
	LastRightMotionControllerRotation = RightHCRotation;
}

void AGunfightCharacter::InitializeSettings()
{
	GSaveGame = Cast<UGunfightSaveGame>(UGameplayStatics::LoadGameFromSlot(FString("GunfightSaveSlot"), 0));
	if (GSaveGame)
	{
		TurnSensitivity = GSaveGame->TurnSensitivity;
		bSnapTurning = GSaveGame->bSnapTurning;
	}
	else
	{
		DebugLogMessage(FString("Failed to load save game :("));
	}
}

void AGunfightCharacter::InitMyWeaponSkin(AWeapon* MyWeapon)
{
	if (MyWeapon == nullptr) return;

	UGameInstance* GI = UGameplayStatics::GetGameInstance(this);
	if (GI == nullptr) return;
	UGunfightGameInstanceSubsystem* GunfightSubsystem = GI->GetSubsystem<UGunfightGameInstanceSubsystem>();
	if (GunfightSubsystem == nullptr || GunfightSubsystem->GunfightSaveGame == nullptr) return;

	int32 SkinIndex = GunfightSubsystem->GunfightSaveGame->EquippedWeaponSkin;
	int32 CurrentSkinIndex = MyWeapon->GetCurrentSkinIndex();

	if (SkinIndex != CurrentSkinIndex)
	{
		MyWeapon->ServerSetWeaponSkin(SkinIndex);
	}
}

void AGunfightCharacter::UpdateFootstepSound(TWeakObjectPtr<UPhysicalMaterial> PhysMat)
{
	if (FootstepAudio == nullptr) return;
	if (!PhysMat.IsValid() || !PhysMat.Get()) return;
	
	TEnumAsByte<EPhysicalSurface> HitSurface = PhysMat.Get()->SurfaceType;
	int32 SurfaceInt; // for setting the sound cue switch variable
	switch (HitSurface)
	{
	case EST_Concrete:
		SurfaceInt = 0;
		break;
	case EST_Metal:
		SurfaceInt = 1;
		break;
	case EST_Tile:
		SurfaceInt = 2;
		break;
	case EST_Wood:
		SurfaceInt = 3;
		break;
	default:
		SurfaceInt = 0;
		break;
	}
	FootstepAudio->SetIntParameter(FName("Surface"), SurfaceInt);
}

AWeapon* AGunfightCharacter::ClosestValidOverlappingWeapon(bool bLeft)
{
	AWeapon* ValidWeapon = nullptr;

	TArray<AWeapon*>& WeaponArray = bLeft ? LeftOverlappingWeapons : RightOverlappingWeapons;

	for (auto w : WeaponArray)
	{
		if (!w) continue;


	}

	return ValidWeapon;
}

void AGunfightCharacter::InitWeaponOffsets()
{
	// left
	HandOffsetPistolLeft = CreateDefaultSubobject<USceneComponent>(TEXT("HandOffsetPistolLeft"));
	HandOffsetPistolLeft->SetupAttachment(LeftMotionController);

	// right
	HandOffsetPistolRight = CreateDefaultSubobject<USceneComponent>(TEXT("HandOffsetPistolRight"));
	HandOffsetPistolRight->SetupAttachment(RightMotionController);
}

void AGunfightCharacter::CheckSlot2TimerFinished()
{
	if (IsLocallyControlled())
	{
		//GetWorldTimerManager().ClearTimer(CheckSlot2Timer);
		return;
	}
	if (Combat == nullptr) return;

	// grabbing slot 2
	if (Combat->LeftEquippedWeapon && Combat->LeftEquippedWeapon->Slot2MotionController)
	{
		// In Anim Instance, change Slot2MotionController side IK to follow the gun GrabSlot2IK scene component
		bool bLeft = Combat->LeftEquippedWeapon->Slot2MotionController == LeftMotionController ? true : false;
		USceneComponent* WeaponSlot2IK = Combat->LeftEquippedWeapon->GetSlot2IK(bLeft);
		if (WeaponSlot2IK)
		{
			bLeft ? LeftHandIKComponent = WeaponSlot2IK : RightHandIKComponent = WeaponSlot2IK;
		}
		SetHandState(bLeft, EHandState::EHS_HoldingPistol2);
	}
	else // not grabbing slot 2
	{
		// In Anim Instance, change Slo2MotionController side IK to follow the motion controller
		LeftHandIKComponent = LeftMotionController;
		RightHandIKComponent = RightMotionController;

		if (Combat->PreviousLeftEquippedWeapon && Combat->PreviousLeftEquippedWeapon->PreviousSlot2MotionController) // there's gotta be a better way to do this
		{ 
			bool bLeft = Combat->PreviousLeftEquippedWeapon->PreviousSlot2MotionController == LeftMotionController ? true : false;
			if (DoesHandHaveWeapon(bLeft)) return;

			SetHandState(bLeft, EHandState::EHS_Idle);
		}
	}
}

void AGunfightCharacter::SetDefaultWeaponSkin(int32 SkinIndex)
{
	if (DefaultWeapon)
	{
		DefaultWeapon->SetWeaponSkin(SkinIndex);
	}
}

void AGunfightCharacter::SetOverlappingWeapon(AWeapon* Weapon)
{
	OverlappingWeapon = Weapon;
}

void AGunfightCharacter::OnRep_OverlappingWeapon(AWeapon* LastWeapon)
{

}

void AGunfightCharacter::SetHandState(bool bLeftHand, EHandState NewState)
{
	USkeletalMeshComponent* CurrentHand = bLeftHand ? LeftHandMesh : RightHandMesh;
	//if (CurrentHand == nullptr) return;
	//UHandAnimInstance* CurrentAnim = Cast<UHandAnimInstance>(CurrentHand->GetAnimInstance());
	//if (CurrentAnim == nullptr) return;

	if (CurrentHand != nullptr)
	{
		UHandAnimInstance* CurrentAnim = Cast<UHandAnimInstance>(CurrentHand->GetAnimInstance());
		if (CurrentAnim != nullptr)
		{
			CurrentAnim->HandState = NewState;
		}
	}

	if (bLeftHand)
	{
		LeftHandState = NewState;
	}
	else
	{
		RightHandState = NewState;
	}
}

void AGunfightCharacter::SetDefaultWeapon(AWeapon* NewDefaultWeapon)
{
	DefaultWeapon = NewDefaultWeapon;
	OnRep_DefaultWeapon();
}

EHandState AGunfightCharacter::GetGripState(bool bLeft)
{
	if (bLeft)
	{
		return bPressingGripLeft ? EHandState::EHS_Grip : EHandState::EHS_Idle;
	}
	else
	{
		return bPressingGripRight ? EHandState::EHS_Grip : EHandState::EHS_Idle;
	}
}

USceneComponent* AGunfightCharacter::GetHandWeaponOffset(EWeaponType WeaponType, bool bLeft)
{
	if (WeaponType == EWeaponType::EWT_Pistol)
	{
		return bLeft ? HandOffsetPistolLeft : HandOffsetPistolRight;
	}
	else if (WeaponType == EWeaponType::EWT_M4)
	{
		// return bLeft ? HandOffsetM4Left : HandOffsetM4Right;
	}
	return nullptr;
}

void AGunfightCharacter::UpdateAnimationHandComponent(bool bLeft, USceneComponent* NewHandComponent)
{
	if (GunfightAnimInstance == nullptr) return;
	USceneComponent* CurrentHandComponent = bLeft ? GunfightAnimInstance->LeftHandComponent : GunfightAnimInstance->RightHandComponent;
	CurrentHandComponent = NewHandComponent;
}

FVector AGunfightCharacter::GetLeftMotionControllerAverageVelocity()
{
	LeftMotionControllerAverageVelocity = FVector::ZeroVector;

	int Num = LeftMotionControllerVelocities.Num();
	for (int i = 0; i < Num; i++)
	{
		LeftMotionControllerAverageVelocity += LeftMotionControllerVelocities[i];
	}
	LeftMotionControllerAverageVelocity /= Num;

	return LeftMotionControllerAverageVelocity.GetClampedToMaxSize(3125.f);
}

FVector AGunfightCharacter::GetRightMotionControllerAverageVelocity()
{
	RightMotionControllerAverageVelocity = FVector::ZeroVector;

	int Num = RightMotionControllerVelocities.Num();
	for (int i = 0; i < Num; i++)
	{
		RightMotionControllerAverageVelocity += RightMotionControllerVelocities[i];
	}

	RightMotionControllerAverageVelocity /= Num;

	return RightMotionControllerAverageVelocity;
}

bool AGunfightCharacter::IsGunCloserThanMag(AActor* Gun, AActor* Mag, USceneComponent* Hand)
{
	if (!Gun || !Mag || !Hand) return false;

	return FVector::DistSquared(Gun->GetActorLocation(), Hand->GetComponentLocation()) < FVector::DistSquared(Mag->GetActorLocation(), Hand->GetComponentLocation());
}

void AGunfightCharacter::ReleaseGrip(bool bLeft)
{
	GripReleased(bLeft);
}

bool AGunfightCharacter::DoesHandHaveWeapon(bool bLeft)
{
	if (GetMesh() == nullptr) return false;

	TArray<AActor*> AttachedActors;
	GetAttachedActors(AttachedActors);
	if (AttachedActors.IsEmpty()) return false;

	for (auto i : AttachedActors)
	{
		if (i == nullptr) continue;

		if (i->IsA(AWeapon::StaticClass()))
		{
			if (GetLeftFromAttachment(i->GetRootComponent()->GetAttachSocketName(), i->GetRootComponent()->GetAttachParent()) == bLeft)
			{
				return true;
			}
		}
	}

	return false;
}

void AGunfightCharacter::InitializeHUDOnWidgetInit()
{
	GunfightPlayerController = GunfightPlayerController == nullptr ? Cast<AGunfightPlayerController>(Controller) : GunfightPlayerController;
	if (GunfightPlayerController)
	{
		GunfightPlayerController->InitializeHUD();
	}
}

void AGunfightCharacter::MoveMeshToCamera()
{
	const FVector Delta = GetVRHeadLocation() - GetMesh()->GetSocketLocation(FName("CameraSocket"));
	GetMesh()->AddWorldOffset(Delta);
}

FVector AGunfightCharacter::TraceFootLocationIK(bool bLeft)
{
	if (!World) return FVector();

	const FName SocketName = bLeft ? FName("ball_l") : FName("ball_r");
	const FVector FootLocation = GetMesh()->GetSocketLocation(SocketName);
	FVector TraceStart = FootLocation;
	TraceStart.Z = GetActorLocation().Z + 40.f;
	FVector TraceEnd = TraceStart;
	TraceEnd.Z = FootLocation.Z - 100.f;
	//DrawDebugLine(World, TraceStart, TraceEnd, FColor::Green, false, World->DeltaTimeSeconds * 1.1f);
	FCollisionShape SphereShape; SphereShape.SetSphere(1.f);
	FHitResult HitResult;
	//bool bTraceHit = World->SweepSingleByChannel(HitResult, TraceStart, TraceEnd, FQuat(), ECollisionChannel::ECC_Camera, SphereShape, IKQueryParams);
	bool bTraceHit = World->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECollisionChannel::ECC_WorldDynamic, IKQueryParams);

	if (bTraceHit)
	{
		if (!bLeft)
		{
			UpdateFootstepSound(HitResult.PhysMaterial);
		}
		return HitResult.ImpactPoint;
	}

	const FVector LeftFootVB = GetMesh()->GetSocketLocation(FName("VB pelvis_foot_l"));
	const FVector RightFootVB = GetMesh()->GetSocketLocation(FName("VB pelvis_foot_r"));

	return bLeft ? LeftFootVB : RightFootVB;
}

void AGunfightCharacter::SetVREnabled(bool bEnabled)
{
	if (VRReplicatedCamera == nullptr) return;
	bIsInVR = bEnabled;
	if (!bEnabled)
	{
		VRReplicatedCamera->bUsePawnControlRotation = true;
		VRReplicatedCamera->AddLocalOffset(FVector(0.f, 0.f, 150.f));
		RightMotionController->AddLocalOffset(FVector(FVector(0.f, 0.f, 140.f)));
		VRMovementReference->bUseClientControlRotation = true;
	}
	else
	{
		VRReplicatedCamera->bUsePawnControlRotation = false;
		VRReplicatedCamera->AddLocalOffset(FVector::ZeroVector);
		VRMovementReference->bUseClientControlRotation = false;
	}
}