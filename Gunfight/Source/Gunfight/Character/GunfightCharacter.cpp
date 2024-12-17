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

AGunfightCharacter::AGunfightCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	
	Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	Combat->SetIsReplicated(true);

	LagCompensation = CreateDefaultSubobject<ULagCompensationComponent>(TEXT("LagCompensation"));

	LeftHandSphere = CreateDefaultSubobject<USphereComponent>(TEXT("LeftHandSphere"));
	LeftHandSphere->SetupAttachment(GetMesh(), FName("hand_l"));
	LeftHandSphere->SetCollisionObjectType(ECC_HandController);
	LeftHandSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	LeftHandSphere->SetCollisionResponseToChannel(ECC_Weapon, ECollisionResponse::ECR_Overlap);
	LeftHandSphere->SetUseCCD(true);
	LeftHandSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	RightHandSphere = CreateDefaultSubobject<USphereComponent>(TEXT("RightHandSphere"));
	RightHandSphere->SetupAttachment(GetMesh(), FName("hand_r"));
	RightHandSphere->SetCollisionObjectType(ECC_HandController);
	RightHandSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	RightHandSphere->SetCollisionResponseToChannel(ECC_Weapon, ECollisionResponse::ECR_Overlap);
	RightHandSphere->SetUseCCD(true);
	RightHandSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	VRStereoLayer = CreateDefaultSubobject<UStereoLayerComponent>(TEXT("VRStereoLayer"));
	VRStereoLayer->SetupAttachment(VRReplicatedCamera);
	VRStereoLayer->SetRelativeLocation(FVector(50.f, 0.f, -5.f));

	CharacterOverlayWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("CharacterOverlayWidget"));
	CharacterOverlayWidget->SetupAttachment(VRReplicatedCamera);
	CharacterOverlayWidget->SetRelativeLocation(FVector(50.f, 0.f, -5.f));
	CharacterOverlayWidget->SetRelativeRotation(FRotator(0.f, 180.f, 0.f));
	CharacterOverlayWidget->SetMaterial(0, WidgetMaterial);
}

void AGunfightCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGunfightCharacter, DefaultWeapon);
	DOREPLIFETIME_CONDITION(AGunfightCharacter, OverlappingWeapon, COND_OwnerOnly);
	DOREPLIFETIME(AGunfightCharacter, bDisableGameplay);
	DOREPLIFETIME(AGunfightCharacter, Health);
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

void AGunfightCharacter::BeginPlay()
{
	Super::BeginPlay();
	SetActorTickEnabled(true);
	IKQueryParams.AddIgnoredActor(this);

	if (HasAuthority())
	{
		OnTakeAnyDamage.AddDynamic(this, &AGunfightCharacter::ReceiveDamage);
	}

	// TODO make the widget/stereo layer not construct on non locally controlled clients

	// this causes the widget not be visible on PIE on the server after respawning for some reason, I don't think it will cause issues in a packaged game
	if (!IsLocallyControlled())
	{
		bStereoLayerInitialized = true;
		VRStereoLayer->DestroyComponent();
		CharacterOverlayWidget->DestroyComponent();
	}
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
}

void AGunfightCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateAnimation();
	PollInit();
}

void AGunfightCharacter::MoveForward(float Throttle)
{
	if (VRReplicatedCamera == nullptr) return;

	FVector CameraForwardVector = VRReplicatedCamera->GetForwardVector();
	CameraForwardVector.Z = 0.f;
	CameraForwardVector = CameraForwardVector.GetSafeNormal();

	AddMovementInput(CameraForwardVector * Throttle);
}

void AGunfightCharacter::MoveRight(float Throttle)
{
	if (VRReplicatedCamera == nullptr) return;
	FVector CameraRightVector = VRReplicatedCamera->GetRightVector();
	CameraRightVector.Z = 0.f;
	CameraRightVector = CameraRightVector.GetSafeNormal();
	AddMovementInput(CameraRightVector * Throttle);
}

void AGunfightCharacter::TurnRight(float Throttle)
{
	if (World == nullptr) return;
	AddControllerYawInput(Throttle * GetWorld()->DeltaTimeSeconds * 100.f);
}

void AGunfightCharacter::LookUp(float Throttle)
{
	if (World == nullptr) return;

	AddControllerPitchInput(Throttle * GetWorld()->DeltaTimeSeconds * 100.f);
}

void AGunfightCharacter::GripPressed(bool bLeftController)
{
	if (bDisableGameplay) return;
	if (Combat)
	{
		if (OverlappingWeapon && OverlappingWeapon->CheckHandOverlap(bLeftController) && OverlappingWeapon->GetWeaponState() != EWeaponState::EWS_Equipped) // weapon check
		{
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, TEXT("EquipWeapon"));
			}
			if (HasAuthority())
			{
				Combat->EquipWeapon(OverlappingWeapon, bLeftController);
			}
			else
			{
				if (bLeftController) ServerEquipButtonPressedLeft();
				else ServerEquipButtonPressedRight();
			}
			return;
		}
		else if (OverlappingMagazine && !OverlappingMagazine->bEquipped && OverlappingMagazine->CheckHandOverlap(bLeftController)) // magazine check
		{
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, TEXT("EquipMagazine"));
			}
			Combat->EquipMagazine(OverlappingMagazine, bLeftController);
		}
	}
	SetHandState(bLeftController, EHandState::EHS_Grip);
}

void AGunfightCharacter::GripReleased(bool bLeftController)
{
	if (bDisableGameplay) return;
	if (Combat && Combat->CheckEquippedWeapon(bLeftController))
	{
		AWeapon* CurrentWeapon = bLeftController ? Combat->LeftEquippedWeapon : Combat->RightEquippedWeapon;
		if (CurrentWeapon)
		{
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
	}
	else if (Combat && Combat->CheckEquippedMagazine(bLeftController))
	{
		Combat->DropMagazine(bLeftController);
	}
	SetHandState(bLeftController, EHandState::EHS_Idle);
}

void AGunfightCharacter::TriggerPressed(bool bLeftController)
{
	if (bDisableGameplay) return;
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
}

void AGunfightCharacter::AButtonPressed(bool bLeftController)
{
	if (Combat)
	{
		AWeapon* CurrentWeapon = bLeftController ? Combat->LeftEquippedWeapon : Combat->RightEquippedWeapon;
		if (CurrentWeapon && CurrentWeapon->IsMagInserted())
		{
			CurrentWeapon->EjectMagazine();
			SpawnFullMagazine(CurrentWeapon->GetFullMagazineClass());
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

void AGunfightCharacter::UpdateAnimInstanceIK()
{
	if (GunfightAnimInstance == nullptr) return;

	// Upper Body IK (UBIK) variables
	GunfightAnimInstance->HeadTransform = VRReplicatedCamera->GetComponentTransform();
	GunfightAnimInstance->LeftHandTransform = LeftMotionController->GetComponentTransform();
	GunfightAnimInstance->RightHandTransform = RightMotionController->GetComponentTransform();
	
	// Leg IK
	GunfightAnimInstance->TraceLeftFootLocation = TraceFootLocationIK(true);
	GunfightAnimInstance->TraceRightFootLocation = TraceFootLocationIK(false);
}

void AGunfightCharacter::UpdateAnimation()
{
	if (bElimmed) return;
	MoveMeshToCamera();
	UpdateAnimInstanceIK();
}

void AGunfightCharacter::ServerEquipButtonPressedLeft_Implementation()
{
	if (Combat)
	{
		Combat->EquipWeapon(OverlappingWeapon, true);
	}
}

void AGunfightCharacter::ServerEquipButtonPressedRight_Implementation()
{
	if (Combat)
	{
		Combat->EquipWeapon(OverlappingWeapon, false);
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

void AGunfightCharacter::PollInit()
{
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
		if (GetMesh())
		{
			GunfightAnimInstance = Cast<UGunfightAnimInstance>(GetMesh()->GetAnimInstance());
		}
	}
	if (GunfightPlayerController == nullptr)
	{
		GunfightPlayerController = GunfightPlayerController == nullptr ? Cast<AGunfightPlayerController>(Controller) : GunfightPlayerController;
		if (GunfightPlayerController)
		{
			SpawnDefaultWeapon();
			InitializeHUD();
			UpdateHUDAmmo();
			UpdateHUDHealth();
		}
	}
	else if(!GunfightPlayerController->GetCharacterOverlay())
	{
		InitializeHUD();
		UpdateHUDAmmo();
		UpdateHUDHealth();
	}
}

void AGunfightCharacter::OnRep_DefaultWeapon()
{
	if (DefaultWeapon)
	{
		DefaultWeapon->CharacterOwner = this;
	}
}

void AGunfightCharacter::ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatorController, AActor* DamageCauser)
{
	Health = FMath::Clamp(Health - Damage, 0.f, MaxHealth);
	UpdateHUDHealth();
	//PlayHitReactMontage();

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

void AGunfightCharacter::Elim(bool bPlayerLeftGame)
{
	//DropOrDestroyWeapons();
	MultiCastElim(bPlayerLeftGame);
}

void AGunfightCharacter::MultiCastElim_Implementation(bool bPlayerLeftGame)
{
	bLeftGame = false;
	if (GunfightPlayerController)
	{
		GunfightPlayerController->SetHUDWeaponAmmo(0);
	}
	bElimmed = true;

	//PlayElimMontage()

	Ragdoll();

	// Disable character movement
	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->StopMovementImmediately();
	bDisableGameplay = true;
	if (Combat)
	{
		Combat->FireButtonPressed(false, false);
	}

	// Disable collision
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCapsuleComponent()->SetSimulatePhysics(false);
	MeshCollisionResponses = GetMesh()->GetCollisionResponseToChannels();

	GetWorldTimerManager().SetTimer(
		ElimTimer,
		this,
		&AGunfightCharacter::ElimTimerFinished,
		2.f
	);
}

void AGunfightCharacter::ElimTimerFinished()
{
	GunfightGameMode = GunfightGameMode == nullptr ? GetWorld()->GetAuthGameMode<AGunfightGameMode>() : GunfightGameMode;
	if (GunfightGameMode && !bLeftGame)
	{
		GunfightGameMode->RequestRespawn(this, Controller);
	}
	// TODO leftgame stuff
}

void AGunfightCharacter::Ragdoll()
{
	SetReplicateMovement(false);

	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECR_Block);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECR_Ignore);
	GetMesh()->SetAllBodiesSimulatePhysics(true);
	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->WakeAllRigidBodies();
	GetMesh()->bBlendPhysics = true;
	GetCharacterMovement()->SetComponentTickEnabled(false);
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
}

void AGunfightCharacter::SpawnDefaultWeapon()
{
	GunfightGameMode = GunfightGameMode == nullptr ? GetWorld()->GetAuthGameMode<AGunfightGameMode>() : GunfightGameMode;
	World = World == nullptr ? GetWorld() : World;
	if (GunfightGameMode && World && WeaponClass && Combat)
	{
		DefaultWeapon = World->SpawnActor<AWeapon>(WeaponClass);
		Combat->AttachWeaponToHolster(DefaultWeapon);
		DefaultWeapon->SetOwner(this);
		DefaultWeapon->CharacterOwner = this;
	}
}

void AGunfightCharacter::SpawnFullMagazine(TSubclassOf<AFullMagazine> FullMagClass)
{
	World = World == nullptr ? GetWorld() : World;
	if (World)
	{
		AFullMagazine* FullMagazine = World->SpawnActor<AFullMagazine>(FullMagClass);
		if (FullMagazine)
		{
			DefaultMagazine = FullMagazine;
			FullMagazine->SetOwner(this);
			FullMagazine->SetCharacterOwner(this);
			AttachMagazineToHolster();
		}
	}
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

void AGunfightCharacter::AttachMagazineToHolster()
{
	if (DefaultMagazine && GetMesh())
	{
		const FName SocketName = !GetRightHolsterPreferred() ? FName("RightHolster") : FName("LeftHolster");
		const USkeletalMeshSocket* HolsterSocket = GetMesh()->GetSocketByName(SocketName);
		if (HolsterSocket)
		{
			HolsterSocket->AttachActor(DefaultMagazine, GetMesh());
		}
	}
}

void AGunfightCharacter::UpdateHUDAmmo()
{
	GunfightPlayerController = GunfightPlayerController == nullptr ? Cast<AGunfightPlayerController>(Controller) : GunfightPlayerController;
	if (GunfightPlayerController && DefaultWeapon)
	{
		GunfightPlayerController->SetHUDCarriedAmmo(DefaultWeapon->GetCarriedAmmo());
		GunfightPlayerController->SetHUDWeaponAmmo(DefaultWeapon->GetAmmo());
	}
}

void AGunfightCharacter::SetupUI()
{
	
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
	if (bLeftHand)
	{
		LeftHandState = NewState;
	}
	else
	{
		RightHandState = NewState;
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
	bool bTraceHit = World->SweepSingleByChannel(HitResult, TraceStart, TraceEnd, FQuat(), ECollisionChannel::ECC_Camera, SphereShape, IKQueryParams);
	if (bTraceHit)
	{
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