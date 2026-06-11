// 電梯 Actor 實作：門滑動 Timeline、踏入轎廂的一次性倒掛驚嚇。
#include "HVElevatorActor.h"

#include "HotelVictoria.h"
#include "HVGhostDirector.h"
#include "HVPlayerCharacter.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TimelineComponent.h"
#include "Curves/CurveFloat.h"
#include "Engine/World.h"

AHVElevatorActor::AHVElevatorActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	CabinMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Cabin"));
	CabinMesh->SetupAttachment(Root);

	LeftDoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftDoor"));
	LeftDoorMesh->SetupAttachment(Root);
	LeftDoorMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	RightDoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightDoor"));
	RightDoorMesh->SetupAttachment(Root);
	RightDoorMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	HangPoint = CreateDefaultSubobject<USceneComponent>(TEXT("HangPoint"));
	HangPoint->SetupAttachment(Root);
	HangPoint->SetRelativeLocation(FVector(0.f, 0.f, 220.f)); // 門楣上方

	ScareTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("ScareTrigger"));
	ScareTrigger->SetupAttachment(Root);
	ScareTrigger->SetBoxExtent(FVector(80.f, 80.f, 110.f));
	ScareTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ScareTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	ScareTrigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	DoorTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("DoorTimeline"));

	OpenHint  = NSLOCTEXT("HV", "ElevatorOpen",  "按下按鈕");
	CloseHint = NSLOCTEXT("HV", "ElevatorClose", "關上電梯門");
}

void AHVElevatorActor::BeginPlay()
{
	Super::BeginPlay();

	LeftDoorClosedLoc = LeftDoorMesh->GetRelativeLocation();
	RightDoorClosedLoc = RightDoorMesh->GetRelativeLocation();

	if (DoorCurve)
	{
		FOnTimelineFloat ProgressDelegate;
		ProgressDelegate.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(AHVElevatorActor, HandleDoorProgress));
		DoorTimeline->AddInterpFloat(DoorCurve, ProgressDelegate, TEXT("Alpha"));

		FOnTimelineEvent FinishedDelegate;
		FinishedDelegate.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(AHVElevatorActor, HandleDoorFinished));
		DoorTimeline->SetTimelineFinishedFunc(FinishedDelegate);

		DoorTimeline->SetTimelineLengthMode(TL_LastKeyFrame);
	}

	ScareTrigger->OnComponentBeginOverlap.AddDynamic(this, &AHVElevatorActor::HandleScareTriggerOverlap);
}

void AHVElevatorActor::Interact_Implementation(AHVPlayerCharacter* /*Interactor*/)
{
	if (!bAnimating)
	{
		SetDoorsOpen(!bDoorsOpen);
	}
}

FText AHVElevatorActor::GetInteractHint_Implementation()
{
	return bDoorsOpen ? CloseHint : OpenHint;
}

bool AHVElevatorActor::CanInteract_Implementation()
{
	return !bAnimating;
}

void AHVElevatorActor::SetDoorsOpen(bool bOpen)
{
	if (bOpen == bDoorsOpen)
	{
		return;
	}
	bDoorsOpen = bOpen;

	if (DoorCurve)
	{
		bAnimating = true;
		if (bOpen)
		{
			DoorTimeline->Play();
		}
		else
		{
			DoorTimeline->Reverse();
		}
	}
	else
	{
		HandleDoorProgress(bOpen ? 1.f : 0.f);
		UE_LOG(LogHotelVictoria, Warning, TEXT("%s 未指定 DoorCurve，電梯門瞬間開關。"), *GetName());
	}
}

void AHVElevatorActor::HandleDoorProgress(float Alpha)
{
	// 左右門板沿本地 Y 軸對開。
	LeftDoorMesh->SetRelativeLocation(LeftDoorClosedLoc + FVector(0.f, -DoorSlideDistance * Alpha, 0.f));
	RightDoorMesh->SetRelativeLocation(RightDoorClosedLoc + FVector(0.f, DoorSlideDistance * Alpha, 0.f));
}

void AHVElevatorActor::HandleDoorFinished()
{
	bAnimating = false;
}

void AHVElevatorActor::HandleScareTriggerOverlap(UPrimitiveComponent* /*OverlappedComponent*/, AActor* OtherActor,
	UPrimitiveComponent* /*OtherComp*/, int32 /*OtherBodyIndex*/, bool /*bFromSweep*/, const FHitResult& /*SweepResult*/)
{
	if (bScareConsumed || !Cast<AHVPlayerCharacter>(OtherActor))
	{
		return;
	}
	bScareConsumed = true;

	if (UHVGhostDirector* Director = GetWorld()->GetSubsystem<UHVGhostDirector>())
	{
		// 點名觸發：女鬼自轎廂門楣倒掛垂下，凝視兩秒後縮回消散。
		FTransform HangTransform = HangPoint->GetComponentTransform();
		Director->RequestEventAtTransform(EHVGhostEventType::ElevatorHang, HangTransform);
	}

	OnHangScareTriggered();
	UE_LOG(LogHotelVictoria, Log, TEXT("%s 觸發一次性電梯倒掛驚嚇。"), *GetName());
}
