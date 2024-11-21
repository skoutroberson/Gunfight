// Fill out your copyright notice in the Description page of Project Settings.


#include "GunfightCharacter.h"
#include "Gunfight/Character/GunfightAnimInstance.h"
#include "GripMotionControllerComponent.h"
#include "VRExpansionFunctionLibrary.h"
#include "HeadMountedDisplayFunctionLibrary.h"

AGunfightCharacter::AGunfightCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AGunfightCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

void AGunfightCharacter::SetVREnabled(bool bEnabled)
{
	if (VRReplicatedCamera == nullptr) return;
	bIsInVR = bEnabled;
	if (!bEnabled)
	{
		VRReplicatedCamera->bUsePawnControlRotation = true;
		VRReplicatedCamera->AddLocalOffset(FVector(0.f, 0.f, 150.f));
	}
	else
	{
		VRReplicatedCamera->bUsePawnControlRotation = false;
		VRReplicatedCamera->AddLocalOffset(FVector::ZeroVector);
	}
	
}

void AGunfightCharacter::BeginPlay()
{
	Super::BeginPlay();
	SetActorTickEnabled(true);
	IKQueryParams.AddIgnoredActor(this);
}

void AGunfightCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	World = GetWorld();

	GunfightAnimInstance = Cast<UGunfightAnimInstance>(GetMesh()->GetAnimInstance());
}

void AGunfightCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &AGunfightCharacter::MoveForward);
	PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &AGunfightCharacter::MoveRight);
	PlayerInputComponent->BindAxis(TEXT("TurnRight"), this, &AGunfightCharacter::TurnRight);
	PlayerInputComponent->BindAxis(TEXT("LookUp"), this, &AGunfightCharacter::LookUp);
}

void AGunfightCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateAnimation();
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
	if (World == nullptr || bIsInVR) return;

	AddControllerPitchInput(Throttle * GetWorld()->DeltaTimeSeconds * 100.f);
}



void AGunfightCharacter::UpdateAnimInstanceIK()
{
	if (GunfightAnimInstance == nullptr) return;

	// Upper Body IK (UBIK) variables
	GunfightAnimInstance->HeadTransform = VRReplicatedCamera->GetComponentTransform();
	GunfightAnimInstance->LeftHandTransform = LeftMotionController->GetComponentTransform();
	GunfightAnimInstance->RightHandTransform = RightMotionController->GetComponentTransform();
	
	// Leg IK
	GunfightAnimInstance->LeftFootLocation = TraceFootLocationIK(true);
	GunfightAnimInstance->RightFootLocation = TraceFootLocationIK(true);
}

void AGunfightCharacter::UpdateAnimation()
{
	MoveMeshToCamera();
	UpdateAnimInstanceIK();
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
	TraceStart.Z = GetMesh()->GetComponentLocation().Z + 40.f;
	FVector TraceEnd = TraceStart;
	TraceEnd.Z -= 100.f;
	FCollisionShape SphereShape; SphereShape.SetSphere(1.f);
	FHitResult HitResult;
	bool bTraceHit = World->SweepSingleByChannel(HitResult, TraceStart, TraceEnd, FQuat(), ECollisionChannel::ECC_Visibility, SphereShape, IKQueryParams);
	if (bTraceHit)
	{
		return HitResult.ImpactPoint;
	}
	return FootLocation;
}