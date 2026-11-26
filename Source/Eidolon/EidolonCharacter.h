#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EidolonCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UInputAction;
class UInputMappingContext;
struct FInputActionValue;

UCLASS()
class EIDOLON_API AEidolonCharacter : public ACharacter
{
    GENERATED_BODY()
public:
    AEidolonCharacter();
    virtual void Tick(float DeltaSeconds) override;
    virtual float TakeDamage(float Amount, const FDamageEvent& Event, AController* Instigator, AActor* Causer) override;
    bool IsDefeated() const { return Health <= 0.f; }
    float GetHealthFraction() const { return Health / 100.f; }
    float GetStaminaFraction() const { return Stamina / 100.f; }
    virtual void PawnClientRestart() override;
    virtual void UnPossessed() override;
protected:
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera")
    TObjectPtr<USpringArmComponent> CameraBoom;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera")
    TObjectPtr<UCameraComponent> FollowCamera;
    UPROPERTY(EditDefaultsOnly, Category="Movement", meta=(ClampMin="0"))
    float WalkSpeed = 400.f;
    UPROPERTY(EditDefaultsOnly, Category="Movement", meta=(ClampMin="0"))
    float SprintSpeed = 650.f;
    UPROPERTY(EditDefaultsOnly, Category="Input", meta=(ClampMin="0"))
    float StickLookDegreesPerSecond = 120.f;
private:
    UPROPERTY(Transient) TObjectPtr<UInputMappingContext> Mapping;
    UPROPERTY(Transient) TObjectPtr<UInputAction> ForwardAction;
    UPROPERTY(Transient) TObjectPtr<UInputAction> RightAction;
    UPROPERTY(Transient) TObjectPtr<UInputAction> MouseYawAction;
    UPROPERTY(Transient) TObjectPtr<UInputAction> MousePitchAction;
    UPROPERTY(Transient) TObjectPtr<UInputAction> StickYawAction;
    UPROPERTY(Transient) TObjectPtr<UInputAction> StickPitchAction;
    UPROPERTY(Transient) TObjectPtr<UInputAction> JumpAction;
    UPROPERTY(Transient) TObjectPtr<UInputAction> SprintAction;
    UPROPERTY() TObjectPtr<class UStaticMeshComponent> PrototypeBody;
    UPROPERTY(Transient) TObjectPtr<UInputAction> LightAction;
    UPROPERTY(Transient) TObjectPtr<UInputAction> HeavyAction;
    UPROPERTY(Transient) TObjectPtr<UInputAction> DodgeAction;
    UPROPERTY(Transient) TObjectPtr<UInputAction> BlockAction;
    UPROPERTY(Transient) TObjectPtr<UInputAction> RestartAction;
    float Health = 100.f;
    float Stamina = 100.f;
    float AttackCooldown = 0.f;
    float InvulnerableTime = 0.f;
    bool bBlocking = false;
    void Attack(float Damage, float Cost, float Cooldown);
    void LightAttack();
    void HeavyAttack();
    void Dodge();
    void StartBlock();
    void EndBlock();
    void RestartArena();
    void BuildInputMapping();
    void RemoveInputMapping();
    void MoveForward(const FInputActionValue& Value);
    void MoveRight(const FInputActionValue& Value);
    void MouseYaw(const FInputActionValue& Value);
    void MousePitch(const FInputActionValue& Value);
    void StickYaw(const FInputActionValue& Value);
    void StickPitch(const FInputActionValue& Value);
    void StartJump();
    void EndJump();
    void StartSprint();
    void StopSprint();
};
