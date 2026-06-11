// 證據 Actor 實作：互動開啟閱讀、讀畢登記與隱藏。
#include "HVEvidenceActor.h"

#include "HotelVictoria.h"
#include "HVGameMode.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"

AHVEvidenceActor::AHVEvidenceActor()
{
	PrimaryActorTick.bCanEverTick = false;

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(MeshComp);
	// 必須擋住 Visibility 通道，玩家互動射線才打得到。
	MeshComp->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	InteractHint = NSLOCTEXT("HV", "EvidenceHintDefault", "閱讀");
}

void AHVEvidenceActor::Interact_Implementation(AHVPlayerCharacter* /*Interactor*/)
{
	if (bCollected)
	{
		return;
	}

	if (AHVGameMode* GameMode = GetWorld()->GetAuthGameMode<AHVGameMode>())
	{
		GameMode->BeginEvidenceReading(this);
	}
}

FText AHVEvidenceActor::GetInteractHint_Implementation()
{
	return InteractHint;
}

bool AHVEvidenceActor::CanInteract_Implementation()
{
	return !bCollected;
}

void AHVEvidenceActor::MarkCollected()
{
	if (bCollected)
	{
		return;
	}
	bCollected = true;

	// 收取後通知 GameMode 累計，並從場景中淡出（此處直接隱藏，演出交給 BP OnCollected）。
	if (AHVGameMode* GameMode = GetWorld()->GetAuthGameMode<AHVGameMode>())
	{
		GameMode->RegisterEvidence(EvidenceId);
	}
	else
	{
		UE_LOG(LogHotelVictoria, Warning, TEXT("MarkCollected 找不到 AHVGameMode。"));
	}

	OnCollected();

	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
}
