// Fill out your copyright notice in the Description page of Project Settings.


#include "VRComponent.h"
#include "Gunfight/Character/GunfightCharacterDeprecated.h"
#include "MotionControllerComponent.h"
#include "Camera/CameraComponent.h"
#include "Math/Vector.h"
#include "DrawDebugHelpers.h"
#include "Components/CapsuleComponent.h"

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

	CorrectCameraOffset();
	ReplicateVRMovement(DeltaTime);
	InterpVRMovement(DeltaTime);
	DrawReplicatedMovement();
}

void UVRComponent::ReplicateVRMovement(float DeltaTime)
{
	CurrentDelay += DeltaTime;

	if (CurrentDelay > ReplicateVRDelay)
	{
		if (ShouldSendRPC())
		{
			ServerVRMove_ClientSend(
				FVector8::PackVector(LastCameraLocation),
				LastCameraRotation,
				FVector8::PackVector(LastLeftHandControllerLocation),
				LastLeftHandControllerRotation,
				FVector8::PackVector(LastRightHandControllerLocation),
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
			const FVector CameraInterpLocation = FMath::VInterpTo(Camera->GetRelativeLocation(), GoalCameraLocation, DeltaTime, MoveSpeed);
			const FRotator CameraInterpRotation = FMath::RInterpTo(Camera->GetComponentRotation(), GoalCameraRotation, DeltaTime, RotateSpeed);
			const FVector LeftHandInterpLocation = FMath::VInterpTo(LeftHandController->GetRelativeLocation(), GoalLeftHandLocation, DeltaTime, MoveSpeed);
			const FRotator LeftHandInterpRotation = FMath::RInterpTo(LeftHandController->GetComponentRotation(), GoalLeftHandRotation, DeltaTime, RotateSpeed);
			const FVector RightHandInterpLocation = FMath::VInterpTo(RightHandController->GetRelativeLocation(), GoalRightHandLocation, DeltaTime, MoveSpeed);
			const FRotator RightHandInterpRotation = FMath::RInterpTo(RightHandController->GetComponentRotation(), GoalRightHandRotation, DeltaTime, RotateSpeed);
			Camera->SetRelativeLocationAndRotation(CameraInterpLocation, CameraInterpRotation);
			LeftHandController->SetRelativeLocationAndRotation(LeftHandInterpLocation, LeftHandInterpRotation);
			RightHandController->SetRelativeLocationAndRotation(RightHandInterpLocation, RightHandInterpRotation);

			Camera->AddLocalOffset(CameraInterpLocation);
			Camera->SetWorldRotation(CameraInterpRotation);
			LeftHandController->AddLocalOffset(LeftHandInterpLocation);
			LeftHandController->SetWorldRotation(LeftHandInterpRotation);
			RightHandController->AddLocalOffset(RightHandInterpLocation);
			RightHandController->SetWorldRotation(RightHandInterpRotation);
		}
	}
}

bool UVRComponent::ShouldSendRPCThisFrame(float DeltaTime)
{
	if (CharacterOwner && CharacterOwner->GetCamera() && CharacterOwner->GetLeftHandController() && CharacterOwner->GetRightHandController())
	{
		const FVector_NetQuantize CameraLocation = CharacterOwner->GetCamera()->GetRelativeLocation();
		const FRotator CameraRotation = CharacterOwner->GetCamera()->GetComponentRotation();
		const FVector_NetQuantize LeftHandLocation = CharacterOwner->GetLeftHandController()->GetRelativeLocation();
		const FRotator LeftHandRotation = CharacterOwner->GetLeftHandController()->GetComponentRotation();
		const FVector_NetQuantize RightHandLocation = CharacterOwner->GetRightHandController()->GetRelativeLocation();
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

		//UE_LOG(LogTemp, Warning, TEXT("CameraDot: %f"), CameraDot);
		//UE_LOG(LogTemp, Warning, TEXT("RightHandDot: %f"), RightHandDot);

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

bool UVRComponent::ShouldSendRPC()
{
	if (CharacterOwner && CharacterOwner->GetCamera() && CharacterOwner->GetLeftHandController() && CharacterOwner->GetRightHandController())
	{
		const FVector_NetQuantize CameraLocation = CharacterOwner->GetCamera()->GetRelativeLocation();
		const FRotator CameraRotation = CharacterOwner->GetCamera()->GetComponentRotation();
		const FVector_NetQuantize LeftHandLocation = CharacterOwner->GetLeftHandController()->GetRelativeLocation();
		const FRotator LeftHandRotation = CharacterOwner->GetLeftHandController()->GetComponentRotation();
		const FVector_NetQuantize RightHandLocation = CharacterOwner->GetRightHandController()->GetRelativeLocation();
		const FRotator RightHandRotation = CharacterOwner->GetRightHandController()->GetComponentRotation();

		LastCameraLocation = CameraLocation;
		LastCameraRotation = CameraRotation;
		LastLeftHandControllerLocation = LeftHandLocation;
		LastLeftHandControllerRotation = LeftHandRotation;
		LastRightHandControllerLocation = RightHandLocation;
		LastRightHandControllerRotation = RightHandRotation;

		return true;
	}

	return false;
}

void UVRComponent::DrawReplicatedMovement()
{
	DrawDebugSphere(GetWorld(), GoalLeftHandLocation, 4.f, 12, FColor::Red, false, ReplicateVRDelay);
	DrawDebugSphere(GetWorld(), GoalRightHandLocation, 4.f, 12, FColor::Blue, false, ReplicateVRDelay);
}

void UVRComponent::CorrectCameraOffset()
{
	if (!CharacterOwner || !CharacterOwner->IsLocallyControlled() || !CharacterOwner->GetCamera() || !CharacterOwner->GetLeftHandController() || !CharacterOwner->GetRightHandController()) return;
	
	FVector NewCameraOffset = CharacterOwner->GetCamera()->GetComponentLocation() - CharacterOwner->GetActorLocation();
	NewCameraOffset.Z = 0.f;

	CharacterOwner->AddActorWorldOffset(NewCameraOffset);
	CharacterOwner->GetVRRoot()->AddWorldOffset(-NewCameraOffset);
}

void UVRComponent::ServerVRMove_ClientSend(const FVector8 CameraLocation, const FRotator CameraRotation, const FVector8 LeftHandLocation, const FRotator LeftHandRotation, const FVector8 RightHandLocation, const FRotator RightHandRotation, float Delay)
{
	if (CharacterOwner)
	{
		CharacterOwner->ServerVRMove(CameraLocation, CameraRotation, LeftHandLocation, LeftHandRotation, RightHandLocation, RightHandRotation, Delay);
	}
}

void UVRComponent::VRMove_ClientReceive(const FVector8 CameraLocation, const FRotator CameraRotation, const FVector8 LeftHandLocation, const FRotator LeftHandRotation, const FVector8 RightHandLocation, const FRotator RightHandRotation, float Delay)
{
	if (!CharacterOwner || !CharacterOwner->GetCamera() || !CharacterOwner->GetLeftHandController() || !CharacterOwner->GetRightHandController() || !CharacterOwner->GetVRRoot()) return;
	if (CharacterOwner->GetCapsuleComponent() == nullptr) return;
	FVector UnpackedCamera = FVector8::UnpackVector(CameraLocation);
	FVector UnpackedLeftHand = FVector8::UnpackVector(LeftHandLocation);
	FVector UnpackedRightHand = FVector8::UnpackVector(RightHandLocation);

	FVector CharacterLocation = CharacterOwner->GetActorLocation();
	CharacterLocation.Z -= CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const FVector RootLocation = CharacterOwner->GetVRRoot()->GetComponentLocation();
	const FQuat CharacterRotation = CharacterOwner->GetActorQuat();

	UnpackedCamera = CharacterRotation.RotateVector(UnpackedCamera);
	UnpackedLeftHand = CharacterRotation.RotateVector(UnpackedLeftHand);
	UnpackedRightHand = CharacterRotation.RotateVector(UnpackedRightHand);
	
	GoalCameraLocation = RootLocation + UnpackedCamera;
	GoalCameraRotation = CameraRotation;
	GoalLeftHandLocation = RootLocation + UnpackedLeftHand;
	GoalLeftHandRotation = LeftHandRotation;
	GoalRightHandLocation = RootLocation + UnpackedRightHand;
	GoalRightHandRotation = RightHandRotation;
}

