// 門 Actor 實作：Timeline 曲線開關、上鎖提示、1013 房浮現。
#include "HVDoorActor.h"

#include "HotelVictoria.h"
#include "HVGameMode.h"

#include "Components/StaticMeshComponent.h"
#include "Components/TimelineComponent.h"
#include "Curves/CurveFloat.h"
#include "Engine/World.h"

AHVDoorActor::AHVDoorActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	FrameMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Frame"));
	FrameMesh->SetupAttachment(Root);

	DoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Door"));
	DoorMesh->SetupAttachment(Root);
	DoorMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	OpenTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("OpenTimeline"));

	LockedHint = NSLOCTEXT("HV", "DoorLocked", "鎖住了");
	OpenHint   = NSLOCTEXT("HV", "DoorOpen",   "開門");
	CloseHint  = NSLOCTEXT("HV", "DoorClose",  "關門");
}

void AHVDoorActor::BeginPlay()
{
	Super::BeginPlay();

	ClosedYaw = DoorMesh->GetRelativeRotation().Yaw;

	if (OpenCurve)
	{
		FOnTimelineFloat ProgressDelegate;
		ProgressDelegate.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(AHVDoorActor, HandleOpenProgress));
		OpenTimeline->AddInterpFloat(OpenCurve, ProgressDelegate, TEXT("Alpha"));

		FOnTimelineEvent FinishedDelegate;
		FinishedDelegate.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(AHVDoorActor, HandleOpenFinished));
		OpenTimeline->SetTimelineFinishedFunc(FinishedDelegate);

		OpenTimeline->SetTimelineLengthMode(TL_LastKeyFrame);
	}

	if (bIsRoom1013Door)
	{
		// 本不存在的房間：先隱形上鎖，等 GameMode 廣播。
		bLocked = true;
		SetActorHiddenInGame(true);
		SetActorEnableCollision(false);

		if (AHVGameMode* GameMode = GetWorld()->GetAuthGameMode<AHVGameMode>())
		{
			GameMode->OnRoom1013Unlocked.AddUniqueDynamic(this, &AHVDoorActor::HandleRoom1013Unlocked);
		}
	}
}

void AHVDoorActor::Interact_Implementation(AHVPlayerCharacter* /*Interactor*/)
{
	if (bAnimating)
	{
		return;
	}

	if (bLocked)
	{
		OnLockedInteract();
		return;
	}

	SetDoorOpen(!bIsOpen);
}

FText AHVDoorActor::GetInteractHint_Implementation()
{
	if (bLocked)
	{
		return LockedHint;
	}
	return bIsOpen ? CloseHint : OpenHint;
}

bool AHVDoorActor::CanInteract_Implementation()
{
	return !bAnimating;
}

void AHVDoorActor::SetDoorOpen(bool bOpen)
{
	if (bOpen == bIsOpen)
	{
		return;
	}
	bIsOpen = bOpen;

	if (OpenCurve)
	{
		bAnimating = true;
		if (bOpen)
		{
			OpenTimeline->Play();
		}
		else
		{
			OpenTimeline->Reverse();
		}
	}
	else
	{
		// 沒有曲線資產時的保底：瞬間切換，流程不中斷。
		HandleOpenProgress(bOpen ? 1.f : 0.f);
		UE_LOG(LogHotelVictoria, Warning, TEXT("%s 未指定 OpenCurve，門瞬間開關。"), *GetName());
	}
}

void AHVDoorActor::HandleOpenProgress(float Alpha)
{
	FRotator Rot = DoorMesh->GetRelativeRotation();
	Rot.Yaw = ClosedYaw + Alpha * OpenAngle;
	DoorMesh->SetRelativeRotation(Rot);
}

void AHVDoorActor::HandleOpenFinished()
{
	bAnimating = false;
}

void AHVDoorActor::HandleRoom1013Unlocked()
{
	// 1013 浮現：顯形 + 解鎖。材質淡入 / 霧氣演出建議在 BP 覆寫此流程後追加。
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	bLocked = false;
	UE_LOG(LogHotelVictoria, Log, TEXT("1013 房門浮現：%s"), *GetName());
}
