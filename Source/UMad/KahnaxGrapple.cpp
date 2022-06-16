// Fill out your copyright notice in the Description page of Project Settings.


#include "KahnaxGrapple.h"

#include "GameFramework/Character.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Microsoft/AllowMicrosoftPlatformTypes.h"
#include "UObject/ReferenceChainSearch.h"

// Sets default values
AKahnaxGrapple::AKahnaxGrapple()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	GrappleLine = CreateDefaultSubobject<UNiagaraComponent>(TEXT("Grapple Line"));
	GrappleLine->SetupAttachment(RootComponent);
}

// Called when the game starts or when spawned
void AKahnaxGrapple::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AKahnaxGrapple::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if(_progress != -1)
	{
		if(_progress <= TimeToReach)
		{
			if(!_goBackToOwner)
			{
				_progress += DeltaTime;
				GrappleLine->SetVectorParameter(FName("GrappleEnd"), FMath::Lerp(_startingLocation, _endingLocation, _progress / TimeToReach));
			}
			else
			{
				_progress += DeltaTime;
				GrappleLine->SetVectorParameter(FName("GrappleEnd"), FMath::Lerp(_startingLocation, Owner->GetMesh()->GetBoneLocation(FName("RightHand")), _progress / TimeToReach));
			}
		}
		else
		{
			if(_goBackToOwner)
			{
				Destroy();
			}
		}
	}
	
}

void AKahnaxGrapple::StartGrapple(FVector Start, FVector Target, ACharacter* GrappleOwner)
{
	_startingLocation = Start;
	_endingLocation = Target;

	Owner = GrappleOwner;
	GrappleLine->SetVectorParameter(FName("GrappleEnd"), Target);
	GrappleLine->Activate();
	GrappleLine->AttachToComponent(GrappleOwner->GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, FName("RightHand"));

	_progress = 0;
}

void AKahnaxGrapple::EndGrapple()
{
	_startingLocation = _endingLocation;
	
	_progress = 0;
	_goBackToOwner = true;
}