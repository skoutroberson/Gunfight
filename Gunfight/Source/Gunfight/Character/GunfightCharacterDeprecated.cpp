// Fill out your copyright notice in the Description page of Project Settings.


#include "GunfightCharacterDeprecated.h"
#include "Net/UnrealNetwork.h"
#include "Gunfight/GunfightComponents/VRComponent.h"
#include "Gunfight/GunfightComponents/CombatComponent.h"
#include "Gunfight/GunfightComponents/LagCompensationComponent.h"
#include "Camera/CameraComponent.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "MotionControllerComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Gunfight/Weapon/Weapon.h"
#include "Gunfight/Gunfight.h"

AGunfightCharacterDeprecated::AGunfightCharacterDeprecated()
{
	PrimaryActorTick.bCanEverTick = true;

	VRComponent = CreateDefaultSubobject<UVRComponent>(TEXT("VRComponent"));

	Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	Combat->SetIsReplicated(true);

	LagCompensation = CreateDefaultSubobject<ULagCompensationComponent>(TEXT("LagCompensation"));

	VRRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VRRoot"));
	VRRoot->SetupAttachment(GetRootComponent());

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(VRRoot);

	LeftHandController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("LeftHandController"));
	LeftHandController->SetupAttachment(VRRoot);
	RightHandController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("RightHandController"));
	RightHandController->SetupAttachment(VRRoot);

	LeftHandSphere = CreateDefaultSubobject<USphereComponent>(TEXT("LeftHandSphere"));
	LeftHandSphere->SetupAttachment(LeftHandController);
	LeftHandSphere->SetSphereRadius(10.f);
	LeftHandSphere->SetRelativeLocation(FVector(0.f, 5.f, 0.f));
	LeftHandSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	LeftHandSphere->SetCollisionObjectType(ECC_HandController);
	LeftHandSphere->SetCollisionResponseToChannel(ECC_Weapon, ECollisionResponse::ECR_Overlap);
	LeftHandSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	RightHandSphere = CreateDefaultSubobject<USphereComponent>(TEXT("RightHandSphere"));
	RightHandSphere->SetupAttachment(RightHandController);
	RightHandSphere->SetSphereRadius(10.f);
	RightHandSphere->SetRelativeLocation(FVector(0.f, -5.f, 0.f));
	RightHandSphere->SetCollisionObjectType(ECC_HandController);
	RightHandSphere->SetCollisionResponseToChannel(ECC_Weapon, ECollisionResponse::ECR_Overlap);
	RightHandSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	DefaultWeapon = CreateDefaultSubobject<UChildActorComponent>(TEXT("DefaultWeapon"));
	DefaultWeapon->SetupAttachment(GetRootComponent());

	NetUpdateFrequency = 66.f;
	MinNetUpdateFrequency = 33.f;
}

void AGunfightCharacterDeprecated::BeginPlay()
{
	Super::BeginPlay();

}

void AGunfightCharacterDeprecated::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UsingHMD = UHeadMountedDisplayFunctionLibrary::IsHeadMountedDisplayEnabled();
	if (UsingHMD)
	{
		Camera->SetRelativeLocation(FVector::ZeroVector);
		Camera->bUsePawnControlRotation = false;
		RightHandController->SetRelativeLocation(FVector::ZeroVector);
	}

	PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &AGunfightCharacterDeprecated::MoveForward);
	PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &AGunfightCharacterDeprecated::MoveRight);
	PlayerInputComponent->BindAxis(TEXT("TurnRight"), this, &AGunfightCharacterDeprecated::TurnRight);

	PlayerInputComponent->BindAction<FLeftRightButtonDelegate>("LeftGrip", IE_Pressed, this, &AGunfightCharacterDeprecated::GripPressed, true);
	PlayerInputComponent->BindAction<FLeftRightButtonDelegate>("LeftGrip", IE_Released, this, &AGunfightCharacterDeprecated::GripReleased, true);
	PlayerInputComponent->BindAction<FLeftRightButtonDelegate>("RightGrip", IE_Pressed, this, &AGunfightCharacterDeprecated::GripPressed, false);
	PlayerInputComponent->BindAction<FLeftRightButtonDelegate>("RightGrip", IE_Released, this, &AGunfightCharacterDeprecated::GripReleased, false);
	PlayerInputComponent->BindAction<FLeftRightButtonDelegate>("LeftTrigger", IE_Pressed, this, &AGunfightCharacterDeprecated::TriggerPressed, true);
	PlayerInputComponent->BindAction<FLeftRightButtonDelegate>("LeftTrigger", IE_Released, this, &AGunfightCharacterDeprecated::TriggerReleased, true);
	PlayerInputComponent->BindAction<FLeftRightButtonDelegate>("RightTrigger", IE_Pressed, this, &AGunfightCharacterDeprecated::TriggerPressed, false);
	PlayerInputComponent->BindAction<FLeftRightButtonDelegate>("RightTrigger", IE_Released, this, &AGunfightCharacterDeprecated::TriggerReleased, false);
	PlayerInputComponent->BindAction<FLeftRightButtonDelegate>("AButton", IE_Pressed, this, &AGunfightCharacterDeprecated::AButtonPressed, false);
	PlayerInputComponent->BindAction<FLeftRightButtonDelegate>("AButton", IE_Released, this, &AGunfightCharacterDeprecated::AButtonReleased, false);
	PlayerInputComponent->BindAction<FLeftRightButtonDelegate>("XButton", IE_Pressed, this, &AGunfightCharacterDeprecated::AButtonPressed, true);
	PlayerInputComponent->BindAction<FLeftRightButtonDelegate>("XButton", IE_Released, this, &AGunfightCharacterDeprecated::AButtonReleased, true);
	PlayerInputComponent->BindAction<FLeftRightButtonDelegate>("BButton", IE_Pressed, this, &AGunfightCharacterDeprecated::BButtonPressed, false);
	PlayerInputComponent->BindAction<FLeftRightButtonDelegate>("BButton", IE_Released, this, &AGunfightCharacterDeprecated::BButtonReleased, false);
	PlayerInputComponent->BindAction<FLeftRightButtonDelegate>("YButton", IE_Pressed, this, &AGunfightCharacterDeprecated::BButtonPressed, true);
	PlayerInputComponent->BindAction<FLeftRightButtonDelegate>("YButton", IE_Released, this, &AGunfightCharacterDeprecated::BButtonReleased, true);
}

void AGunfightCharacterDeprecated::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (VRComponent)
	{
		VRComponent->CharacterOwner = this;
	}
	if (Combat)
	{
		//Combat->Character = this;
	}
	if (DefaultWeapon && GetMesh())
	{
		FName HolsterToAttach = bRightHolsterPreferred ? FName("RightHolster") : FName("LeftHolster");
		DefaultWeapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, HolsterToAttach);
		if (DefaultWeapon->GetChildActor())
		{
			DefaultWeapon->GetChildActor()->SetOwner(this);
		}
	}
}

void AGunfightCharacterDeprecated::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGunfightCharacterDeprecated, bDisableGameplay);
}

void AGunfightCharacterDeprecated::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AGunfightCharacterDeprecated::MoveForward(float Throttle)
{
	if (Camera == nullptr) return;

	FVector CameraForwardVector = Camera->GetForwardVector();
	CameraForwardVector.Z = 0.f;
	CameraForwardVector = CameraForwardVector.GetSafeNormal();

	AddMovementInput(CameraForwardVector * Throttle);
}

void AGunfightCharacterDeprecated::MoveRight(float Throttle)
{
	if (Camera == nullptr) return;
	FVector CameraRightVector = Camera->GetRightVector();
	CameraRightVector.Z = 0.f;
	CameraRightVector = CameraRightVector.GetSafeNormal();
	AddMovementInput(CameraRightVector * Throttle);
}

void AGunfightCharacterDeprecated::TurnRight(float Throttle)
{
	if (GetWorld() == nullptr) return;
	AddControllerYawInput(Throttle * GetWorld()->DeltaTimeSeconds * 100.f);
}

void AGunfightCharacterDeprecated::GripPressed(bool bLeftController)
{
	if (bDisableGameplay) return;
	UMotionControllerComponent* CurrentController = bLeftController ? LeftHandController : RightHandController;
	if (Combat && CurrentController && OverlappingWeapon)
	{
		if (bLeftController)
		{
			if (HasAuthority())
			{
				Combat->EquipWeapon(OverlappingWeapon, true);
			}
			else
			{
				ServerEquipButtonPressed(true);
			}
		}
		else
		{
			if (HasAuthority())
			{
				Combat->EquipWeapon(OverlappingWeapon, false);
			}
			else
			{
				ServerEquipButtonPressed(false);
			}
		}
	}
}

void AGunfightCharacterDeprecated::ServerEquipButtonPressed_Implementation(bool bLeftController)
{
	if (Combat)
	{
		Combat->EquipWeapon(OverlappingWeapon, bLeftController);
	}
}

void AGunfightCharacterDeprecated::PollInit()
{
	if (!ChildWeapon && DefaultWeapon && DefaultWeapon->GetChildActor())
	{
		ChildWeapon = Cast<AWeapon>(DefaultWeapon->GetChildActor());
		ChildWeapon->SetOwner(this);
	}
}

void AGunfightCharacterDeprecated::GripReleased(bool bLeftController)
{
	UMotionControllerComponent* CurrentController = bLeftController ? LeftHandController : RightHandController;
}

void AGunfightCharacterDeprecated::TriggerPressed(bool bLeftController)
{
	UMotionControllerComponent* CurrentController = bLeftController ? LeftHandController : RightHandController;
}

void AGunfightCharacterDeprecated::TriggerReleased(bool bLeftController)
{
	UMotionControllerComponent* CurrentController = bLeftController ? LeftHandController : RightHandController;
}

void AGunfightCharacterDeprecated::AButtonPressed(bool bLeftController)
{
	UMotionControllerComponent* CurrentController = bLeftController ? LeftHandController : RightHandController;
}

void AGunfightCharacterDeprecated::AButtonReleased(bool bLeftController)
{
	UMotionControllerComponent* CurrentController = bLeftController ? LeftHandController : RightHandController;
}

void AGunfightCharacterDeprecated::BButtonPressed(bool bLeftController)
{
	UMotionControllerComponent* CurrentController = bLeftController ? LeftHandController : RightHandController;
}

void AGunfightCharacterDeprecated::BButtonReleased(bool bLeftController)
{
	UMotionControllerComponent* CurrentController = bLeftController ? LeftHandController : RightHandController;
}

void AGunfightCharacterDeprecated::ServerVRMove_Implementation(const FVector8 CameraLocation, const FRotator CameraRotation, const FVector8 LeftHandLocation, const FRotator LeftHandRotation, const FVector8 RightHandLocation, const FRotator RightHandRotation, float Delay)
{
	MulticastVRMove(CameraLocation, CameraRotation, LeftHandLocation, LeftHandRotation, RightHandLocation, RightHandRotation, Delay);
}

bool AGunfightCharacterDeprecated::ServerVRMove_Validate(const FVector8 CameraLocation, const FRotator CameraRotation, const FVector8 LeftHandLocation, const FRotator LeftHandRotation, const FVector8 RightHandLocation, const FRotator RightHandRotation, float Delay)
{
	return ValidateVRMovement(Delay);
}

void AGunfightCharacterDeprecated::MulticastVRMove_Implementation(const FVector8 CameraLocation, const FRotator CameraRotation, const FVector8 LeftHandLocation, const FRotator LeftHandRotation, const FVector8 RightHandLocation, const FRotator RightHandRotation, float Delay)
{
	if (VRComponent)
	{
		VRComponent->VRMove_ClientReceive(CameraLocation, CameraRotation, LeftHandLocation, LeftHandRotation, RightHandLocation, RightHandRotation, Delay);
	}
}

bool AGunfightCharacterDeprecated::MulticastVRMove_Validate(const FVector8 CameraLocation, const FRotator CameraRotation, const FVector8 LeftHandLocation, const FRotator LeftHandRotation, const FVector8 RightHandLocation, const FRotator RightHandRotation, float Delay)
{
	return ValidateVRMovement(Delay);
}

inline const bool AGunfightCharacterDeprecated::ValidateVRMovement(float Delay)
{
	if (VRComponent)
	{
		const bool bNearlyEqual = FMath::IsNearlyEqual(VRComponent->ReplicateVRDelay, Delay, 0.001f);
		return bNearlyEqual;
	}
	return true;
}

AWeapon* AGunfightCharacterDeprecated::GetWeapon()
{
	if (DefaultWeapon)
	{
		return Cast<AWeapon>(DefaultWeapon.Get()->GetChildActor());
	}
	return nullptr;
}
