// Fill out your copyright notice in the Description page of Project Settings.


#include "VRComponent.h"
#include "Gunfight/Character/GunfightCharacter.h"
#include "MotionControllerComponent.h"
#include "Camera/CameraComponent.h"

UVRComponent::UVRComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

}

void UVRComponent::BeginPlay()
{
	Super::BeginPlay();
	
}

void UVRComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	ReplicateVRMovement(DeltaTime);
	InterpVRMovement(DeltaTime);
}

void UVRComponent::ReplicateVRMovement(float DeltaTime)
{
	CurrentDelay += DeltaTime;

	if (CurrentDelay > ReplicateVRDelay)
	{
		if (CharacterOwner && CharacterOwner->GetCamera() && CharacterOwner->GetLeftHandController() && CharacterOwner->GetRightHandController())
		{
			ServerVRMove_ClientSend(
				CharacterOwner->GetCamera()->GetComponentLocation(),
				CharacterOwner->GetCamera()->GetComponentRotation(),
				CharacterOwner->GetLeftHandController()->GetComponentLocation(),
				CharacterOwner->GetLeftHandController()->GetComponentRotation(),
				CharacterOwner->GetRightHandController()->GetComponentLocation(),
				CharacterOwner->GetRightHandController()->GetComponentRotation(),
				ReplicateVRDelay
			);
		}
	}
}

void UVRComponent::InterpVRMovement(float DeltaTime)
{
	if (CharacterOwner && !CharacterOwner->IsLocallyControlled())
	{
		USceneComponent* Camera = CharacterOwner->GetCamera();
		USceneComponent* LeftHandController = CharacterOwner->GetLeftHandController();
		USceneComponent* RightHandController = CharacterOwner->GetRightHandController();
		if (Camera && LeftHandController && RightHandController)
		{
			const FVector CameraInterpLocation = FMath::VInterpTo(Camera->GetComponentLocation(), GoalCameraLocation, DeltaTime, 20.f);
			const FRotator CameraInterpRotation = FMath::RInterpTo(Camera->GetComponentRotation(), GoalCameraRotation, DeltaTime, 10.f);
			const FVector LeftHandInterpLocation = FMath::VInterpTo(LeftHandController->GetComponentLocation(), GoalLeftHandLocation, DeltaTime, 20.f);
			const FRotator LeftHandInterpRotation = FMath::RInterpTo(LeftHandController->GetComponentRotation(), GoalLeftHandRotation, DeltaTime, 10.f);
			const FVector RightHandInterpLocation = FMath::VInterpTo(RightHandController->GetComponentLocation(), GoalRightHandLocation, DeltaTime, 20.f);
			const FRotator RightHandInterpRotation = FMath::RInterpTo(RightHandController->GetComponentRotation(), GoalRightHandRotation, DeltaTime, 10.f);
			Camera->SetWorldLocationAndRotation(CameraInterpLocation, CameraInterpRotation);
			LeftHandController->SetWorldLocationAndRotation(LeftHandInterpLocation, LeftHandInterpRotation);
			RightHandController->SetWorldLocationAndRotation(RightHandInterpLocation, RightHandInterpRotation);
		}
	}
}

void UVRComponent::ServerVRMove_ClientSend(const FVector_NetQuantize CameraLocation, const FRotator CameraRotation, const FVector_NetQuantize LeftHandLocation, const FRotator LeftHandRotation, const FVector_NetQuantize RightHandLocation, const FRotator RightHandRotation, float Delay)
{
	if (CharacterOwner)
	{
		CharacterOwner->ServerVRMove(CameraLocation, CameraRotation, LeftHandLocation, LeftHandRotation, RightHandLocation, RightHandRotation, Delay);
	}
}

void UVRComponent::VRMove_ClientReceive(const FVector_NetQuantize CameraLocation, const FRotator CameraRotation, const FVector_NetQuantize LeftHandLocation, const FRotator LeftHandRotation, const FVector_NetQuantize RightHandLocation, const FRotator RightHandRotation, float Delay)
{
	GoalCameraLocation = CameraLocation;
	GoalCameraRotation = CameraRotation;
	GoalLeftHandLocation = LeftHandLocation;
	GoalLeftHandRotation = LeftHandRotation;
	GoalRightHandLocation = RightHandLocation;
	GoalRightHandRotation = RightHandRotation;
}

