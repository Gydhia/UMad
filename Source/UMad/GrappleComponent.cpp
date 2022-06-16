// Fill out your copyright notice in the Description page of Project Settings.


#include "GrappleComponent.h"

#include "GameFramework/Character.h"
#include "Particles/ParticleSystemComponent.h"
#include "PhysicsEngine/PhysicsCollisionHandler.h"

// Sets default values for this component's properties
UGrappleComponent::UGrappleComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UGrappleComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UGrappleComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UGrappleComponent::BeginGrapple(AActor* NearestAttach)
{
	FVector Start = Owner->GetActorLocation();
	
	UWorld* World = GetWorld();
	
	_grapple = World->SpawnActor<AKahnaxGrapple>(Grapple, Start, Owner->GetActorRotation());
	if(_grapple != nullptr)
	{
		TargetAttach = NearestAttach->GetActorLocation();

		_grapple->StartGrapple(Start, NearestAttach->GetActorLocation(), Owner);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Couldn't create Kahnax Grapple"));
	}
	
}

