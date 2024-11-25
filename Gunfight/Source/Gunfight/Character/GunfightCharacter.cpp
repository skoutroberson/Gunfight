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
}

void AGunfightCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGunfightCharacter, DefaultWeapon);
	DOREPLIFETIME_CONDITION(AGunfightCharacter, OverlappingWeapon, COND_OwnerOnly);
	DOREPLIFETIME(AGunfightCharacter, bDisableGameplay);
}

void AGunfightCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	World = GetWorld();

	GunfightAnimInstance = Cast<UGunfightAnimInstance>(GetMesh()->GetAnimInstance());
	if (GunfightAnimInstance)
	{
		GunfightAnimInstance->SetCharacter(this);
	}

	if (Combat)
	{
		Combat->Character = this;
	}
	/*
	if (DefaultWeapon && GetMesh())
	{
		FName HolsterToAttach = bRightHolsterPreferred ? FName("RightHolster") : FName("LeftHolster");
		DefaultWeapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, HolsterToAttach);
		if (DefaultWeapon->GetChildActor())
		{
			WeaponActor = Cast<AWeapon>(DefaultWeapon->GetChildActor());
			WeaponActor->SetOwner(this);
			WeaponActor->CharacterOwner = this;
			IKQueryParams.AddIgnoredActor(WeaponActor);
			WeaponActor->SetReplicates(true);
		}
	}
	*/
}

void AGunfightCharacter::BeginPlay()
{
	Super::BeginPlay();
	SetActorTickEnabled(true);
	IKQueryParams.AddIgnoredActor(this);

	/*
	if (HasAuthority())
	{
		SpawnWeaponInHolster();
	}
	*/
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
		if (OverlappingWeapon)
		{
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
		else if (OverlappingMagazine)
		{
			Combat->AttachActorToHand(OverlappingMagazine, bLeftController);
		}
	}
	SetHandState(bLeftController, EHandState::EHS_Grip);
}

void AGunfightCharacter::GripReleased(bool bLeftController)
{
	if (bDisableGameplay) return;
	if (Combat)
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
	SetHandState(bLeftController, EHandState::EHS_Idle);
}

void AGunfightCharacter::TriggerPressed(bool bLeftController)
{
}

void AGunfightCharacter::TriggerReleased(bool bLeftController)
{
}

void AGunfightCharacter::AButtonPressed(bool bLeftController)
{
	if (Combat)
	{
		AWeapon* CurrentWeapon = bLeftController ? Combat->LeftEquippedWeapon : Combat->RightEquippedWeapon;
		if (CurrentWeapon)
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

void AGunfightCharacter::SpawnWeaponInHolster()
{
	World = World == nullptr ? Cast<UWorld>(GetWorld()) : World;
	if (World && GetMesh())
	{
		WeaponActor = World->SpawnActor<AWeapon>(WeaponClass, FVector::ZeroVector, FRotator::ZeroRotator);
		if (WeaponActor && Combat && HasAuthority())
		{
			bWeaponInitialized = true;
			WeaponActor->SetReplicates(true);
			Combat->AttachWeaponToHolster(WeaponActor);
		}
	}
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
			//TO DO
			//UpdateHUDAmmo();
			//UpdateHUDHealth();
		}
	}
}

void AGunfightCharacter::SpawnDefaultWeapon()
{
	AGunfightGameMode* GunfightGameMode = Cast<AGunfightGameMode>(UGameplayStatics::GetGameMode(this));
	World = World == nullptr ? GetWorld() : World;
	if (GunfightGameMode && World && WeaponClass && Combat)
	{
		AWeapon* StartingWeapon = World->SpawnActor<AWeapon>(WeaponClass);
		DefaultWeapon = StartingWeapon;
		Combat->AttachWeaponToHolster(StartingWeapon);
		StartingWeapon->SetOwner(this);
		StartingWeapon->CharacterOwner = this;
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
	DrawDebugLine(World, TraceStart, TraceEnd, FColor::Green, false, World->DeltaTimeSeconds * 1.1f);
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