// Fill out your copyright notice in the Description page of Project Settings.


#include "LagCompensationComponent.h"
#include "Net/UnrealNetwork.h"
#include "Gunfight/Weapon/Weapon.h"
#include "Kismet/GameplayStatics.h"
#include "Gunfight/Character/GunfightCharacter.h"
#include "PhysicsEngine\PhysicsAsset.h"

ULagCompensationComponent::ULagCompensationComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

}

void ULagCompensationComponent::BeginPlay()
{
	Super::BeginPlay();
	
}

void ULagCompensationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	SaveFramePackage();
}

void ULagCompensationComponent::ShowFramePackage(const FFramePackage& Package, const FColor& Color)
{
	for (auto& CapsuleInfo : Package.HitCapsulesInfo)
	{
		DrawDebugSphere(
			GetWorld(),
			CapsuleInfo.A,
			CapsuleInfo.Radius,
			12,
			Color,
			false,
			MaxRecordTime
		);
		DrawDebugSphere(
			GetWorld(),
			CapsuleInfo.B,
			CapsuleInfo.Radius,
			12,
			Color,
			false,
			MaxRecordTime
		);
	}
}

void ULagCompensationComponent::ServerScoreRequest_Implementation(AGunfightCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, float HitTime, AWeapon* DamageCauser)
{
	DamageCauserWeapon = DamageCauser;
	FServerSideRewindResult Confirm = ServerSideRewind(HitCharacter, TraceStart, HitLocation, HitTime);

	if (Character && HitCharacter && DamageCauser && Confirm.HitType != EHitbox::EH_None)
	{
		UGameplayStatics::ApplyDamage(
			HitCharacter,
			FHitbox::GetDamage(Confirm.HitType, DamageCauser->GetDamage()),
			Character->Controller,
			DamageCauser,
			UDamageType::StaticClass()
		);

		const FVector BloodSpawnLocation = ((TraceStart - Confirm.HitLocation).GetSafeNormal() * 10.f) + Confirm.HitLocation;
		HitCharacter->MulticastSpawnBlood(BloodSpawnLocation);
	}
}

FServerSideRewindResult ULagCompensationComponent::ServerSideRewind(AGunfightCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, float HitTime)
{
	FFramePackage FrameToCheck = GetFrameToCheck(HitCharacter, HitTime);
	return ConfirmHit(FrameToCheck, HitCharacter, TraceStart, HitLocation);
}

FFramePackage ULagCompensationComponent::GetFrameToCheck(AGunfightCharacter* HitCharacter, float HitTime)
{
	bool bReturn =
		HitCharacter == nullptr ||
		HitCharacter->GetLagCompensation() == nullptr ||
		HitCharacter->GetLagCompensation()->FrameHistory.GetHead() == nullptr ||
		HitCharacter->GetLagCompensation()->FrameHistory.GetTail() == nullptr;
	if (bReturn) return FFramePackage();

	// Frame package that we check to verify a hit
	FFramePackage FrameToCheck;
	bool bShouldInterpolate = true;

	// Frame history of the HitCharacter
	const TDoubleLinkedList<FFramePackage>& History = HitCharacter->GetLagCompensation()->FrameHistory;
	const float OldestHistoryTime = History.GetTail()->GetValue().Time;
	const float NewestHistoryTime = History.GetHead()->GetValue().Time;

	if (OldestHistoryTime > HitTime)
	{
		// too far back - too laggy to do SSR
		return FFramePackage();
	}
	if (OldestHistoryTime == HitTime)
	{
		FrameToCheck = History.GetTail()->GetValue();
		bShouldInterpolate = false;
	}
	if (NewestHistoryTime <= HitTime)
	{
		FrameToCheck = History.GetHead()->GetValue();
		bShouldInterpolate = false;
	}

	TDoubleLinkedList<FFramePackage>::TDoubleLinkedListNode* Younger = History.GetHead();
	TDoubleLinkedList<FFramePackage>::TDoubleLinkedListNode* Older = Younger;

	while (Older->GetValue().Time > HitTime) // is Older still younger than HitTime
	{
		// March back until: OlderTime < HitTime < YoungerTime
		if (Older->GetNextNode() == nullptr) break;
		Older = Older->GetNextNode();
		if (Older->GetValue().Time > HitTime)
		{
			Younger = Older;
		}
	}
	if (Older->GetValue().Time == HitTime) // highly unlikely, but we found our frame to check
	{
		FrameToCheck = Older->GetValue();
		bShouldInterpolate = false;
	}
	if (bShouldInterpolate)
	{
		// Interpolate between Younger and Older
		FrameToCheck = InterpBetweenFrames(Older->GetValue(), Younger->GetValue(), HitTime);
	}
	FrameToCheck.Character = HitCharacter;
	return FrameToCheck;
}

FFramePackage ULagCompensationComponent::InterpBetweenFrames(const FFramePackage& OlderFrame, const FFramePackage& YoungerFrame, float HitTime)
{
	const float Distance = YoungerFrame.Time - OlderFrame.Time;
	const float InterpFraction = FMath::Clamp((HitTime - OlderFrame.Time) / Distance, 0.f, 1.f);

	FFramePackage InterpFramePackage;
	InterpFramePackage.Time = HitTime;

	// assumes that physics asset holds SphylElems in the same order
	for (int i = 0; i < YoungerFrame.HitCapsulesInfo.Num(); ++i)
	{
		const FCapsuleInformation& OlderCapsule = OlderFrame.HitCapsulesInfo[i];
		const FCapsuleInformation& YoungerCapsule = YoungerFrame.HitCapsulesInfo[i];

		FCapsuleInformation InterpCapsuleInfo;
		InterpCapsuleInfo.A = FMath::VInterpTo(OlderCapsule.A, YoungerCapsule.A, 1.f, InterpFraction);
		InterpCapsuleInfo.B = FMath::VInterpTo(OlderCapsule.B, YoungerCapsule.B, 1.f, InterpFraction);
		InterpCapsuleInfo.Radius = YoungerCapsule.Radius;
		InterpCapsuleInfo.Length = YoungerCapsule.Length;
		InterpCapsuleInfo.HitboxType = YoungerCapsule.HitboxType;

		InterpFramePackage.HitCapsulesInfo.Add(InterpCapsuleInfo);
	}
	return InterpFramePackage;
}

void ULagCompensationComponent::SaveFramePackage()
{
	if (Character == nullptr || !Character->HasAuthority()) return;
	if (FrameHistory.Num() <= 1)
	{
		FFramePackage ThisFrame;
		SaveFramePackage(ThisFrame);
		FrameHistory.AddHead(ThisFrame);
	}
	else
	{
		float HistoryLength = FrameHistory.GetHead()->GetValue().Time - FrameHistory.GetTail()->GetValue().Time;
		while (HistoryLength > MaxRecordTime)
		{
			FrameHistory.RemoveNode(FrameHistory.GetTail());
			HistoryLength = FrameHistory.GetHead()->GetValue().Time - FrameHistory.GetTail()->GetValue().Time;
		}
		FFramePackage ThisFrame;
		SaveFramePackage(ThisFrame);
		FrameHistory.AddHead(ThisFrame);
		//ShowFramePackage(ThisFrame, FColor::Red);
	}
}

void ULagCompensationComponent::SaveFramePackage(FFramePackage& Package)
{
	Character = Character == nullptr ? Cast<AGunfightCharacter>(GetOwner()) : Character;
	if (Character == nullptr || Character->GetMesh() == nullptr || Character->GetMesh()->GetPhysicsAsset() == nullptr) return;

	Package.Time = GetWorld()->GetTimeSeconds();
	Package.Character = Character;

	USkeletalMeshComponent* Mesh = Character->GetMesh();

	for (auto& SkeletalBodySetup : Mesh->GetPhysicsAsset()->SkeletalBodySetups)
	{
		const FName& BName = SkeletalBodySetup->BoneName;
		const FTransform& BoneWorldTransform = Mesh->GetBoneTransform(Mesh->GetBoneIndex(BName));
		const EHitbox BoneHitboxType = HitboxTypes.Contains(BName) ? HitboxTypes[BName] : EHitbox::EH_None;
		for (auto& Sphyl : SkeletalBodySetup->AggGeom.SphylElems)
		{
			const FTransform& LocTransform = Sphyl.GetTransform();
			const FTransform WorldTransform = LocTransform * BoneWorldTransform;
			const float Radius = Sphyl.GetScaledRadius(WorldTransform.GetScale3D());
			const FVector CapsuleCenter = WorldTransform.GetLocation();
			const float CapsuleLength = Sphyl.GetScaledHalfLength(WorldTransform.GetScale3D()) - Radius;
			const FVector CapsuleAxis = WorldTransform.GetUnitAxis(EAxis::Z);
			const FVector A = CapsuleCenter + CapsuleAxis * CapsuleLength;
			const FVector B = CapsuleCenter - CapsuleAxis * CapsuleLength;

			FCapsuleInformation CapsuleInformation;
			CapsuleInformation.A = A;
			CapsuleInformation.B = B;
			CapsuleInformation.Radius = Radius;
			CapsuleInformation.Length = CapsuleLength;
			CapsuleInformation.HitboxType = BoneHitboxType;
			Package.HitCapsulesInfo.Add(CapsuleInformation);
		}
	}
}

void ULagCompensationComponent::EnableCharacterMeshCollision(AGunfightCharacter* HitCharacter, ECollisionEnabled::Type CollisionEnabled)
{
	if (HitCharacter && HitCharacter->GetMesh())
	{
		HitCharacter->GetMesh()->SetCollisionEnabled(CollisionEnabled);
	}
}

FHitInfo ULagCompensationComponent::TraceAgainstCapsules(const FFramePackage& Package, const AGunfightCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector& TraceEnd)
{
	if (HitCharacter == nullptr || HitCharacter->GetMesh() == nullptr || HitCharacter->GetMesh()->GetPhysicsAsset() == nullptr) return FHitInfo();

	FHitInfo HitInfo;
	uint8 HitSpheres = 0;
	const FVector Dir = (TraceEnd - TraceStart).GetSafeNormal();
	const float Length = (TraceEnd - TraceStart).Size();

	for (auto& Capsule : Package.HitCapsulesInfo)
	{
		// convert capsule to spheres for optimized collision check
		TArray<FSphereInfo> Spheres;
		CapsuleToSpheres(Capsule.A, Capsule.B, Capsule.Radius, Capsule.Length, Spheres);

		for (auto& Sphere : Spheres)
		{
			//const bool bTraceIntersectsSphere = FMath::LineSphereIntersection(FVector3f(TraceStart), FVector3f(Dir), Length, FVector3f(Sphere.Center), Sphere.Radius);
			const bool bTraceIntersectsSphere = LineSphereIntersection(TraceStart, Dir, Length, Sphere.Center, Sphere.Radius);
			// check if weapon trace intersects with each sphere of this capsule
			if (bTraceIntersectsSphere)
			{
				++HitSpheres;
				const FVector IntersectionPoint = FirstIntersectionPoint(TraceStart, TraceEnd, Sphere.Center, Sphere.Radius);
				if (HitInfo.HitType < Capsule.HitboxType)
				{
					HitInfo.Location = IntersectionPoint;
					HitInfo.HitType = Capsule.HitboxType;
					HitInfo.Normal = (HitInfo.Location - Sphere.Center) / Sphere.Radius;
				}
				else if (HitInfo.HitType == Capsule.HitboxType)
				{
					const float Dist1 = FVector::DistSquared(HitInfo.Location, TraceStart);
					const float Dist2 = FVector::DistSquared(IntersectionPoint, TraceStart);
					if (Dist1 > Dist2)
					{
						HitInfo.Location = IntersectionPoint;
						HitInfo.HitType = Capsule.HitboxType;
						HitInfo.Normal = (HitInfo.Location - Sphere.Center) / Sphere.Radius;
					}
				}
				if (HitSpheres >= MaxSpheresHit)
				{
					return HitInfo;
				}
			}
		}
	}
	return HitInfo;
}

void ULagCompensationComponent::CapsuleToSpheres(const FVector& A, const FVector& B, const float Radius, const float Length, TArray<FSphereInfo>& OutSpheres)
{
	OutSpheres.Add(FSphereInfo{ A, Radius });
	OutSpheres.Add(FSphereInfo{ (A + B) * 0.5f, Radius });
	OutSpheres.Add(FSphereInfo{ B, Radius });
}

inline bool ULagCompensationComponent::LineSphereIntersection(const FVector& Start, const FVector& Dir, float Length, const FVector& Origin, float Radius)
{
	const FVector	EO = Start - Origin;
	const float		v = (Dir | (Origin - Start));
	const float		disc = Radius * Radius - ((EO | EO) - v * v);

	if (disc >= 0)
	{
		const float	Time = (v - FMath::Sqrt(disc)) / Length;

		if (Time >= 0 && Time <= 1)
			return 1;
		else
			return 0;
	}
	else
		return 0;
}

inline FVector const ULagCompensationComponent::FirstIntersectionPoint(const FVector& LineStart, const FVector& LineEnd, const FVector& Center, const float Radius)
{
	const FVector Disp = LineEnd - LineStart;
	const FVector Dir = (LineEnd - LineStart).GetSafeNormal();
	const FVector ClosestPoint = FMath::ClosestPointOnLine(LineStart, LineEnd, Center);
	const float ClosestLength = (ClosestPoint - Center).Size();
	const float IntersectionLength = FMath::Sqrt(Radius * Radius - ClosestLength * ClosestLength);

	return ClosestPoint - Dir * IntersectionLength;
}

FServerSideRewindResult ULagCompensationComponent::ConfirmHit(const FFramePackage& Package, AGunfightCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation)
{
	if (HitCharacter == nullptr) return FServerSideRewindResult();

	EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::NoCollision);

	const FVector TraceEnd = TraceStart + (HitLocation - TraceStart) * 1.25f;
	FHitInfo HitInfo = TraceAgainstCapsules(Package, HitCharacter, TraceStart, TraceEnd);

	EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);
	return FServerSideRewindResult{ HitInfo.HitType, HitInfo.Location, HitInfo.Normal };
}