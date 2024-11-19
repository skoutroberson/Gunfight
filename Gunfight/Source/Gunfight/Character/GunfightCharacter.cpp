// Fill out your copyright notice in the Description page of Project Settings.


#include "GunfightCharacter.h"
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

AGunfightCharacter::AGunfightCharacter()
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

void AGunfightCharacter::BeginPlay()
{
	Super::BeginPlay();

}

void AGunfightCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UsingHMD = UHeadMountedDisplayFunctionLibrary::IsHeadMountedDisplayEnabled();
	if (UsingHMD)
	{
		Camera->SetRelativeLocation(FVector::ZeroVector);
		Camera->bUsePawnControlRotation = false;
		RightHandController->SetRelativeLocation(FVector::ZeroVector);
	}

	PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &AGunfightCharacter::MoveForward);
	PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &AGunfightCharacter::MoveRight);
	PlayerInputComponent->BindAxis(TEXT("TurnRight"), this, &AGunfightCharacter::TurnRight);

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

void AGunfightCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (VRComponent)
	{
		VRComponent->CharacterOwner = this;
	}
	if (Combat)
	{
		Combat->Character = this;
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

void AGunfightCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGunfightCharacter, bDisableGameplay);
}

void AGunfightCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AGunfightCharacter::MoveForward(float Throttle)
{
	if (Camera == nullptr) return;

	FVector CameraForwardVector = Camera->GetForwardVector();
	CameraForwardVector.Z = 0.f;
	CameraForwardVector = CameraForwardVector.GetSafeNormal();

	AddMovementInput(CameraForwardVector * Throttle);
}

void AGunfightCharacter::MoveRight(float Throttle)
{
	if (Camera == nullptr) return;
	FVector CameraRightVector = Camera->GetRightVector();
	CameraRightVector.Z = 0.f;
	CameraRightVector = CameraRightVector.GetSafeNormal();
	AddMovementInput(CameraRightVector * Throttle);
}

void AGunfightCharacter::TurnRight(float Throttle)
{
	if (GetWorld() == nullptr) return;
	AddControllerYawInput(Throttle * GetWorld()->DeltaTimeSeconds * 100.f);
}

void AGunfightCharacter::GripPressed(bool bLeftController)
{
	if (bDisableGameplay) return;
	UMotionControllerComponent* CurrentController = bLeftController ? LeftHandController : RightHandController;
	if (Combat && CurrentController && OverlappingWeapon)
	{
		if (bLeftController && OverlappingWeapon->bLeftControllerOverlap)
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
		else if (!bLeftController && OverlappingWeapon->bRightControllerOverlap)
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

void AGunfightCharacter::ServerEquipButtonPressed_Implementation(bool bLeftController)
{
	GetCharacterMovement()
	if (Combat)
	{
		Combat->EquipWeapon(OverlappingWeapon, bLeftController);
	}
}

void AGunfightCharacter::PollInit()
{
	if (!ChildWeapon && DefaultWeapon && DefaultWeapon->GetChildActor())
	{
		ChildWeapon = Cast<AWeapon>(DefaultWeapon->GetChildActor());
		ChildWeapon->SetOwner(this);
	}
}

void AGunfightCharacter::GripReleased(bool bLeftController)
{
	UMotionControllerComponent* CurrentController = bLeftController ? LeftHandController : RightHandController;
}

void AGunfightCharacter::TriggerPressed(bool bLeftController)
{
	UMotionControllerComponent* CurrentController = bLeftController ? LeftHandController : RightHandController;
}

void AGunfightCharacter::TriggerReleased(bool bLeftController)
{
	UMotionControllerComponent* CurrentController = bLeftController ? LeftHandController : RightHandController;
}

void AGunfightCharacter::AButtonPressed(bool bLeftController)
{
	UMotionControllerComponent* CurrentController = bLeftController ? LeftHandController : RightHandController;
}

void AGunfightCharacter::AButtonReleased(bool bLeftController)
{
	UMotionControllerComponent* CurrentController = bLeftController ? LeftHandController : RightHandController;
}

void AGunfightCharacter::BButtonPressed(bool bLeftController)
{
	UMotionControllerComponent* CurrentController = bLeftController ? LeftHandController : RightHandController;
}

void AGunfightCharacter::BButtonReleased(bool bLeftController)
{
	UMotionControllerComponent* CurrentController = bLeftController ? LeftHandController : RightHandController;
}

void AGunfightCharacter::ServerVRMove_Implementation(const FVector_NetQuantize CameraLocation, const FRotator CameraRotation, const FVector_NetQuantize LeftHandLocation, const FRotator LeftHandRotation, const FVector_NetQuantize RightHandLocation, const FRotator RightHandRotation, float Delay)
{
	MulticastVRMove(CameraLocation, CameraRotation, LeftHandLocation, LeftHandRotation, RightHandLocation, RightHandRotation, Delay);
}

bool AGunfightCharacter::ServerVRMove_Validate(const FVector_NetQuantize CameraLocation, const FRotator CameraRotation, const FVector_NetQuantize LeftHandLocation, const FRotator LeftHandRotation, const FVector_NetQuantize RightHandLocation, const FRotator RightHandRotation, float Delay)
{
	return ValidateVRMovement(Delay);
}

void AGunfightCharacter::MulticastVRMove_Implementation(const FVector_NetQuantize CameraLocation, const FRotator CameraRotation, const FVector_NetQuantize LeftHandLocation, const FRotator LeftHandRotation, const FVector_NetQuantize RightHandLocation, const FRotator RightHandRotation, float Delay)
{
	if (VRComponent)
	{
		VRComponent->VRMove_ClientReceive(CameraLocation, CameraRotation, LeftHandLocation, LeftHandRotation, RightHandLocation, RightHandRotation, Delay);
	}
}

bool AGunfightCharacter::MulticastVRMove_Validate(const FVector_NetQuantize CameraLocation, const FRotator CameraRotation, const FVector_NetQuantize LeftHandLocation, const FRotator LeftHandRotation, const FVector_NetQuantize RightHandLocation, const FRotator RightHandRotation, float Delay)
{
	return ValidateVRMovement(Delay);
}

inline const bool AGunfightCharacter::ValidateVRMovement(float Delay)
{
	if (VRComponent)
	{
		const bool bNearlyEqual = FMath::IsNearlyEqual(VRComponent->ReplicateVRDelay, Delay, 0.001f);
		return bNearlyEqual;
	}
	return true;
}

AWeapon* AGunfightCharacter::GetWeapon()
{
	if (DefaultWeapon)
	{
		return Cast<AWeapon>(DefaultWeapon.Get()->GetChildActor());
	}
	return nullptr;
}
