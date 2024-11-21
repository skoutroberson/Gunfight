// Fill out your copyright notice in the Description page of Project Settings.


#include "GunfightAnimInstance.h"
#include "Gunfight/Character/GunfightCharacter.h"


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

	// Leg IK
	UpdateFeetLocation();
}

void UGunfightAnimInstance::UpdateFeetLocation()
{
	if (GunfightCharacter == nullptr || GunfightCharacter->GetMesh() == nullptr) return;

	const FVector RootLocation = GunfightCharacter->GetMesh()->GetSocketLocation(FName("root"));
	const FVector LeftFootVB = GunfightCharacter->GetMesh()->GetSocketLocation(FName("VB pelvis_foot_l"));
	const FVector RightFootVB = GunfightCharacter->GetMesh()->GetSocketLocation(FName("VB pelvis_foot_r"));
	LeftFootLocation.X = LeftFootVB.X; 
	LeftFootLocation.Y = LeftFootVB.Y;
	LeftFootLocation.Z = (LeftFootVB - RootLocation).Z + LeftFootLocation.Z;
	RightFootLocation.X = RightFootVB.X;
	RightFootLocation.Y = RightFootVB.Y;
	RightFootLocation.Z = (RightFootVB - RootLocation).Z + RightFootLocation.Z;
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
