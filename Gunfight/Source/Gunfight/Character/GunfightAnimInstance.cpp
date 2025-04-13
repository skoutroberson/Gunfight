// Fill out your copyright notice in the Description page of Project Settings.


#include "GunfightAnimInstance.h"
#include "Gunfight/Character/GunfightCharacter.h"
#include "DrawDebugHelpers.h"
#include "Kismet/KismetMathLibrary.h"


void UGunfightAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	GunfightCharacter = Cast<AGunfightCharacter>(TryGetPawnOwner());
	World = GetWorld();
}

void UGunfightAnimInstance::NativeUpdateAnimation(float DeltaTime)
{
	Super::NativeUpdateAnimation(DeltaTime);

	if (GunfightCharacter == nullptr) GunfightCharacter = Cast<AGunfightCharacter>(TryGetPawnOwner());
	if (GunfightCharacter == nullptr) return;

	bLocallyControlled = GunfightCharacter->IsLocallyControlled(); // better to do this once on initialization

	bElimmed = GunfightCharacter->IsEliminated();

	if (bElimmed)
	{
		Speed = 0.f;
		bIsAccelerating = false;
		return;
	}

	FVector Velocity = GunfightCharacter->GetVelocity();
	Velocity.Z = 0.f;
	Speed = Velocity.Size();

	bIsAccelerating = GunfightCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.f ? true : false;
	
	FRotator AimRotation = GunfightCharacter->GetVRRotation();
	FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(GunfightCharacter->GetVelocity());
	FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation);
	YawOffset = DeltaRot.Yaw;

	// Leg IK
	UpdateFeetLocation();

	/*
	if (LeftHandComponent) LeftHandTransform = LeftHandComponent->GetComponentTransform();
	if (RightHandComponent) RightHandTransform = RightHandComponent->GetComponentTransform();
	*/
	LeftHandState = GunfightCharacter->GetHandState(true);
	RightHandState = GunfightCharacter->GetHandState(false);
}

void UGunfightAnimInstance::UpdateFeetLocation()
{
	if (GunfightCharacter == nullptr || GunfightCharacter->GetMesh() == nullptr) return;

	const FVector RootLocation = GunfightCharacter->GetMesh()->GetSocketLocation(FName("root"));
	const FVector LeftFootVB = GunfightCharacter->GetMesh()->GetSocketLocation(FName("VB pelvis_foot_l"));
	const FVector RightFootVB = GunfightCharacter->GetMesh()->GetSocketLocation(FName("VB pelvis_foot_r"));

	LeftFootLocation.X = LeftFootVB.X; 
	LeftFootLocation.Y = LeftFootVB.Y;
	LeftFootLocation.Z = (LeftFootVB - RootLocation).Z + TraceLeftFootLocation.Z;
	RightFootLocation.X = RightFootVB.X;
	RightFootLocation.Y = RightFootVB.Y;
	RightFootLocation.Z = (RightFootVB - RootLocation).Z + TraceRightFootLocation.Z;
}

FVector UGunfightAnimInstance::TraceFootLocation(bool bLeft)
{
	if (!World) return FVector();

	const FName SocketName = bLeft ? FName("ball_l") : FName("ball_r");
	const FVector FootLocation = GunfightCharacter->GetMesh()->GetSocketLocation(SocketName);
	FVector TraceStart = FootLocation;
	TraceStart.Z = GunfightCharacter->GetMesh()->GetComponentLocation().Z + 40.f;
	FVector TraceEnd = TraceStart;
	TraceEnd.Z -= 100.f;

	FCollisionShape SphereShape; SphereShape.SetSphere(1.f);
	FHitResult HitResult;
	bool bTraceHit = World->SweepSingleByChannel(HitResult,TraceStart,TraceEnd,FQuat(),ECollisionChannel::ECC_Camera,SphereShape);
	if (bTraceHit)
	{
		return HitResult.ImpactPoint;
	}
	return FootLocation;
}