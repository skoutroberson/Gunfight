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
		if (ShouldSendRPCThisFrame(DeltaTime))
		{
			ServerVRMove_ClientSend(
				LastCameraLocation,
				LastCameraRotation,
				LastLeftHandControllerLocation,
				LastLeftHandControllerRotation,
				LastRightHandControllerLocation,
				LastRightHandControllerRotation,
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
			const FVector CameraInterpLocation = FMath::VInterpTo(Camera->GetComponentLocation(), GoalCameraLocation, DeltaTime, MoveSpeed);
			const FRotator CameraInterpRotation = FMath::RInterpTo(Camera->GetComponentRotation(), GoalCameraRotation, DeltaTime, RotateSpeed);
			const FVector LeftHandInterpLocation = FMath::VInterpTo(LeftHandController->GetComponentLocation(), GoalLeftHandLocation, DeltaTime, MoveSpeed);
			const FRotator LeftHandInterpRotation = FMath::RInterpTo(LeftHandController->GetComponentRotation(), GoalLeftHandRotation, DeltaTime, RotateSpeed);
			const FVector RightHandInterpLocation = FMath::VInterpTo(RightHandController->GetComponentLocation(), GoalRightHandLocation, DeltaTime, MoveSpeed);
			const FRotator RightHandInterpRotation = FMath::RInterpTo(RightHandController->GetComponentRotation(), GoalRightHandRotation, DeltaTime, RotateSpeed);
			Camera->SetWorldLocationAndRotation(CameraInterpLocation, CameraInterpRotation);
			LeftHandController->SetWorldLocationAndRotation(LeftHandInterpLocation, LeftHandInterpRotation);
			RightHandController->SetWorldLocationAndRotation(RightHandInterpLocation, RightHandInterpRotation);
		}
	}
}

bool UVRComponent::ShouldSendRPCThisFrame(float DeltaTime)
{
	if (CharacterOwner && CharacterOwner->GetCamera() && CharacterOwner->GetLeftHandController() && CharacterOwner->GetRightHandController())
	{
		const FVector_NetQuantize CameraLocation = CharacterOwner->GetCamera()->GetComponentLocation();
		const FRotator CameraRotation = CharacterOwner->GetCamera()->GetComponentRotation();
		const FVector_NetQuantize LeftHandLocation = CharacterOwner->GetLeftHandController()->GetComponentLocation();
		const FRotator LeftHandRotation = CharacterOwner->GetLeftHandController()->GetComponentRotation();
		const FVector_NetQuantize RightHandLocation = CharacterOwner->GetRightHandController()->GetComponentLocation();
		const FRotator RightHandRotation = CharacterOwner->GetRightHandController()->GetComponentRotation();

		const FVector_NetQuantize CameraLocationDelta = CameraLocation - LastCameraLocation;
		const FVector_NetQuantize LeftHandLocationDelta = LeftHandLocation - LastLeftHandControllerLocation;
		const FVector_NetQuantize RightHandLocationDelta = RightHandLocation - LastRightHandControllerLocation;
		const float CameraVelocitySquared = CameraLocationDelta.SizeSquared();
		const float LeftHandVelocitySquared = LeftHandLocationDelta.SizeSquared();
		const float RightHandVelocitySquared = RightHandLocationDelta.SizeSquared();

		/* these dots would give better accuracy but cost more bandwidth */
		const float CameraDot = FVector::DotProduct(CameraLocationDelta, LastCameraLocationDelta);
		const float LeftHandDot = FVector::DotProduct(LeftHandLocationDelta, LastLeftHandLocationDelta);
		const float RightHandDot = FVector::DotProduct(RightHandLocationDelta, LastRightHandLocationDelta);

		UE_LOG(LogTemp, Warning, TEXT("CameraDot: %f"), CameraDot);
		UE_LOG(LogTemp, Warning, TEXT("RightHandDot: %f"), RightHandDot);

		const bool bDotsUnderThreshold =
			CameraDot > DotThreshold &&
			LeftHandDot > DotThreshold &&
			RightHandDot > DotThreshold;

		const bool bVelocitiesUnderThreshold = 
			CameraVelocitySquared < CameraVelocitySquaredThreshold &&
			LeftHandVelocitySquared < HandVelocitySquaredThreshold &&
			RightHandVelocitySquared < HandVelocitySquaredThreshold;

		const bool bRotationDeltasUnderThreshold =
			CameraRotation.EqualsOrientation(LastCameraRotation, CameraRotationDeltaThreshold) &&
			LeftHandRotation.EqualsOrientation(LastLeftHandControllerRotation, HandRotationDeltaThreshold) &&
			RightHandRotation.EqualsOrientation(LastRightHandControllerRotation, HandRotationDeltaThreshold);
		
		if (bDotsUnderThreshold && bVelocitiesUnderThreshold && bRotationDeltasUnderThreshold)
		{
			CurrentPredictionTime += ReplicateVRDelay;
			if (CurrentPredictionTime < MaxPredictionTime)
			{
				return false;
			}
		}

		// save variables for server-client prediction algorithm

		LastCameraLocation = CameraLocation;
		LastCameraRotation = CameraRotation;
		LastLeftHandControllerLocation = LeftHandLocation;
		LastLeftHandControllerRotation = LeftHandRotation;
		LastRightHandControllerLocation = RightHandLocation;
		LastRightHandControllerRotation = RightHandRotation;
		LastCameraLocationDelta = CameraLocationDelta;
		LastLeftHandLocationDelta = LeftHandLocationDelta;
		LastRightHandLocationDelta = RightHandLocationDelta;

		CurrentPredictionTime = 0.f;
		return true;
	}

	return false;
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

