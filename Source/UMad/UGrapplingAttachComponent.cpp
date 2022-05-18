// Fill out your copyright notice in the Description page of Project Settings.


#include "UGrapplingAttachComponent.h"
#include "Components/BoxComponent.h"

// Sets default values for this component's properties
UUGrapplingAttachComponent::UUGrapplingAttachComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	AttachCollider = CreateDefaultSubobject<UBoxComponent>(TEXT("AttachCollider"));
}


// Called when the game starts
void UUGrapplingAttachComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UUGrapplingAttachComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

