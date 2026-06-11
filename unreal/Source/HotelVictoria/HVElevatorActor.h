// 電梯 Actor：左右門 Timeline 滑動開合、一次性「倒掛女鬼」驚嚇觸發區。
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HVInteractable.h"
#include "HVElevatorActor.generated.h"

class UBoxComponent;
class UCurveFloat;
class UStaticMeshComponent;
class UTimelineComponent;

/**
 * 飯店老電梯。
 * - 互動開/關門：DoorCurve（0→1）驅動左右門板沿 Y 軸對開滑動。
 * - ScareTrigger：玩家踏入轎廂時觸發一次性 ElevatorHang（女鬼自天花板倒掛垂下），之後永不再發。
 */
UCLASS()
class HOTELVICTORIA_API AHVElevatorActor : public AActor, public IHVInteractable
{
	GENERATED_BODY()

public:
	AHVElevatorActor();

	// ---- IHVInteractable ----
	virtual void Interact_Implementation(AHVPlayerCharacter* Interactor) override;
	virtual FText GetInteractHint_Implementation() override;
	virtual bool CanInteract_Implementation() override;

	UFUNCTION(BlueprintCallable, Category = "HV|Elevator")
	void SetDoorsOpen(bool bOpen);

	UFUNCTION(BlueprintPure, Category = "HV|Elevator")
	bool AreDoorsOpen() const { return bDoorsOpen; }

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleDoorProgress(float Alpha);

	UFUNCTION()
	void HandleDoorFinished();

	UFUNCTION()
	void HandleScareTriggerOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/** 驚嚇觸發瞬間的藍圖掛點（金屬呻吟音效、燈光驟暗）。 */
	UFUNCTION(BlueprintImplementableEvent, Category = "HV|Elevator")
	void OnHangScareTriggered();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HV|Elevator")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HV|Elevator")
	TObjectPtr<UStaticMeshComponent> CabinMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HV|Elevator")
	TObjectPtr<UStaticMeshComponent> LeftDoorMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HV|Elevator")
	TObjectPtr<UStaticMeshComponent> RightDoorMesh;

	/** 倒掛女鬼出現的定位點（轎廂門楣上方，朝向玩家）。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HV|Elevator")
	TObjectPtr<USceneComponent> HangPoint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HV|Elevator")
	TObjectPtr<UBoxComponent> ScareTrigger;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HV|Elevator")
	TObjectPtr<UTimelineComponent> DoorTimeline;

	/** 門滑動曲線（0→1，建議 1.6 秒、帶一點起步遲滯的老電梯感）。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HV|Elevator")
	TObjectPtr<UCurveFloat> DoorCurve;

	/** 單側門板滑動距離（cm）。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HV|Elevator", meta = (ClampMin = "10.0"))
	float DoorSlideDistance = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HV|Elevator")
	FText OpenHint;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HV|Elevator")
	FText CloseHint;

private:
	bool bDoorsOpen = false;
	bool bAnimating = false;

	/** 一次性驚嚇是否已消耗。 */
	bool bScareConsumed = false;

	FVector LeftDoorClosedLoc = FVector::ZeroVector;
	FVector RightDoorClosedLoc = FVector::ZeroVector;
};
