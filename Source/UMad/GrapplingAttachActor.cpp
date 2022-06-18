// Fill out your copyright notice in the Description page of Project Settings.


#include "GrapplingAttachActor.h"

#include "HookWidget.h"
#include "UMadCharacter.h"
#include "Blueprint/UserWidget.h"
#include "Components/BoxComponent.h"
#include "Misc/OutputDeviceNull.h"
#include "Particles/ParticleSystemComponent.h"

// Sets default values
AGrapplingAttachActor::AGrapplingAttachActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	AttachCollider = CreateDefaultSubobject<UBoxComponent>(TEXT("AttachCollider"));
	HookIcon = CreateDefaultSubobject<UHookWidget>(TEXT("HookIconWidget"));
}

// Called when the game starts or when spawned
void AGrapplingAttachActor::BeginPlay()
{
	Super::BeginPlay();

	UnselectAttach();
	
}

// Called every frame
void AGrapplingAttachActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AGrapplingAttachActor::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);
	if(OtherActor->IsA(AUMadCharacter::StaticClass()))
	{
		AUMadCharacter* Character = Cast<AUMadCharacter>(OtherActor);
		Character->AddPossibleGrapplingAttach(this);
	}
}

void AGrapplingAttachActor::NotifyActorEndOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);
	if(OtherActor->IsA(AUMadCharacter::StaticClass()))
	{
		AUMadCharacter* Character = Cast<AUMadCharacter>(OtherActor);
		Character->RemovePossibleGrapplingAttach(this);
	}
}

void AGrapplingAttachActor::CallHideSelf(AActor* actor, FString funcName)
{
	FOutputDeviceNull ar;
	actor->CallFunctionByNameWithArguments(*funcName, ar, NULL, true);
}

void AGrapplingAttachActor::CallShowSelf(AActor* actor, FString funcName)
{
	FOutputDeviceNull ar;
	actor->CallFunctionByNameWithArguments(*funcName, ar, NULL, true);
}

void AGrapplingAttachActor::SelectAttach()
{
	FString func = FString("ShowSelf");
	CallShowSelf(this, func);
}

void AGrapplingAttachActor::UnselectAttach()
{
	FString func = FString("HideSelf");
	CallHideSelf(this, func);
}

