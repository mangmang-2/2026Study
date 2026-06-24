# 멀티 이펙트 컴포지션 스킬 시스템 — 작업 기록

> 위치: `Source/StudyProject/Skills/`
> 설계 원본: `C:\Users\mang\Downloads\스킬_이펙트_조합_시스템_설계.md`
> 목표: 데미지/범위/시전/당기기/밀기/스턴/둔화 같은 효과 모듈을 데이터로 조합하는 스킬 시스템 + 실시간 UI + 멀티 대응.

---

## 아키텍처 요약

```
USkillDefinition (PrimaryDataAsset)
├── TargetingMode (NoTarget / Point / Direction / Actor)   ← 어디를 향하나
├── DeliveryType  (AOE / Cone / Projectile / Melee / Beam / Dash) ← 어떻게 닿나
├── CastTime / Range / Radius / Cooldown
├── VFX (ImpactVFX / ProjectileVFX / CastMontage / RangeDecal)
└── EffectModules[]  (EditInline, 순서/동시 실행)
       ├── UDamageEffectModule
       ├── UPullEffectModule
       ├── UPushEffectModule
       ├── UStunEffectModule  (기존 GE_StatusShocked 재사용)
       └── USlowEffectModule  (기존 GE_StatusChilled 재사용)

USkillManagerComponent (PlayerCharacter 부착)
├── EquippedSkills[3]  (복제) — Q/E/R 슬롯
├── 슬롯별 쿨다운 추적 (소유 클라=UI, 서버=권위)
└── ActivateSlot → 타겟팅 해석(클라) → Server_ActivateSlot RPC

UGA_SkillExecutor (UCombatGameplayAbility 상속, ServerOnly)
├── PrepareSkill(Def, Origin, Direction) ← 컴포넌트가 서버에서 세팅
├── CastTime 대기 → Detonate
└── DeliveryType별 분기 → 판정 → EffectModules[] 순회 Execute
```

### 멀티 역할 분담 (보스 돌진 패턴 재사용)
- 실행/판정/모듈 = **서버 권위** (GA ServerOnly)
- 착탄 VFX = `ACharacterBase::Multicast_SpawnSkillVFX` (NetMulticast)
- Pull/Push = 서버 `LaunchCharacter` → CharacterMovement 복제
- 쿨다운 = 소유 클라 로컬(예측·UI) + 서버 권위 게이팅

---

## 진행 상황

### Phase 1 — 코어 시스템 (코드 작성 완료, 빌드 대기)
- [x] 1. 태그 추가 (Input.Skill1~3, Ability.Skill, State.Casting) — StudyGameplayTags.h/.cpp
- [x] 2. SkillTypes.h (enum 2축 + FSkillExecutionContext)
- [x] 3. UEffectModule 베이스 + 모듈 5종 (Damage/Pull/Push/Stun/Slow)
- [x] 4. USkillDefinition (PrimaryDataAsset, 헤더 전용)
- [x] 5. UGA_SkillExecutor (AOE/Cone 완전구현, Beam=기하필터, Melee=전방반구, Projectile/Dash=지점AOE 임시)
- [x] 6. USkillManagerComponent (슬롯3/쿨다운/타겟팅해석/RPC/복제)
- [x] 7. ACharacterBase::Multicast_SpawnSkillVFX 추가
- [x] 8. Build.cs include 경로 추가 (Skills, Skills/Modules)
- [x] 9. PlayerCharacter에 SkillManagerComp 부착 + Exec `UseSkill 0/1/2` 테스트 커맨드
- [x] 10. 풀 리빌드 성공 (25/25 컴파일+링크, `Result: Succeeded`, 98초) → 로드 검증 진행 중

### 입력 바인딩
- **Z/X/C** → 슬롯 0/1/2. `PlayerCharacter::SetupPlayerInputComponent`서 `BindKey`(IMC/IA 미수정). HUD 라벨도 Z/X/C. 콘솔 `UseSkill N`도 유지.
- **함정**: 처음 Q/E/R로 했다가 충돌 발견 — IMC_Default에서 Q=IA_LockOn, R=IA_Finisher 이미 매핑됨(E만 빔). IMC 조회는 deprecated `mappings` 말고 `default_key_mappings.mappings`(InputMappingContextMappingData) 사용. 충돌 피해 빈 키 Z/X/C로 변경(사용자 선택).

### 발동 경로 (Phase 1)
- 발동 인자 전달: 컴포넌트(서버)가 `PendingSkill/Origin/Direction` stash → `TryActivateAbility` → GA가 `ConsumePendingActivation`으로 회수.
  (InstancedPerActor는 첫 활성화 전 인스턴스가 없어 PrepareSkill 직접 주입 불가 → stash 방식 채택)
- 테스트: PIE 콘솔 `UseSkill 0` (슬롯0=DefaultSkills[0]). IMC/IA 바인딩은 규칙상 미수정(Phase 2/사용자).

### 추가 요구 반영 (2차 빌드) — 코드 완료, 재빌드 대기
- [x] VFX 범위 비례 확대: `VFXReferenceRadius` 기준 `Scale = ImpactVFXScale × (Radius/RefRadius)` (GA `SpawnImpactVFX`)
- [x] 지속(필드)형 스킬: `Duration` > 0이면 `TickInterval`마다 재판정+모듈 재실행 (GA `FieldTick`/`FieldTimer`)
- [x] 효과 지속시간 조절: Stun/Slow 모듈 `StunDuration`/`SlowDuration` → `Spec.SetDuration` 덮어쓰기
- [x] 틱 주기 조절: `TickInterval`(1.0=1초당1번, 0.8=0.8초마다). Duration=3,Interval=1 → t=0,1,2,3 4타
- [x] 2차 풀 리빌드 + PIE 검증 완료

### ✅ PIE 검증 결과 (NoTarget 자기중심으로 모듈 격리 테스트)
- 단발 GravityWell: HP 1000→912.5 (데미지 적용), 로그 `overlaps=1 hits=1`, Pull→Stun→Damage 순차 실행 확인
- 지속 FlameField: Duration=3 TickInterval=1 → HP 912.5→762.5 (~4틱×40) 후 정지. 매 틱 RebuildContext+Damage+Slow 재실행 로그 확인
- 쿨다운: 발동 시 0→5 정상

### 🐞 핵심 버그 수정 (cooldown 자기차단)
- **증상**: GA가 활성화조차 안 됨(SkillDbg 로그 0개). `Cmd: UseSkill 0`은 찍힘.
- **원인**: 스탠드얼론/리슨서버는 클라=서버라 같은 컴포넌트 인스턴스 공유. `ActivateSlot`(클라)가 예측 쿨다운을 먼저 켜고 → `Server_ActivateSlot`이 "쿨다운 중"으로 보고 GA 활성화 전에 early-return.
- **수정**: 클라측 예측 StartCooldown 제거. 서버가 발동 성공 시에만 StartCooldown + `Client_StartCooldown`(원격 클라 UI 예측) RPC.
- 진단 로그 전부 제거함. 데모 스킬 targeting=PointTarget(마우스) 복원.

### Phase 2 — UI (진행 중, batch 1 작성 완료)
- [x] `USkillHUDWidget` (UI/HUD, 코드전용): 하단중앙 Q/E/R 3슬롯. NativeTick으로 SkillManagerComponent 폴링 → 아이콘/쿨다운 어둡게+카운트다운 실시간.
- [x] `USkillCastBarWidget` (UI/HUD, 코드전용): 중앙하단, 시전 중에만 표시. 진행률 폴링.
- [x] 컴포넌트에 캐스트 상태 노출: `IsCasting/GetCastProgress/GetCastingSkill` + ActivateSlot서 로컬 예측 세팅.
- [x] PlayerCharacter BeginPlay서 두 위젯 생성+AddToViewport(로컬만).
- [x] 빌드 + PIE: 사용자가 화면 중앙 하단 Q/E/R 슬롯 표시 확인.
- [x] 빈 UImage는 단색 렌더 안 됨 → 슬롯 배경/쿨다운 딤을 `UBorder`로 교체(텍스트만 보이던 문제 해결).
- 함정: `take_high_res_screenshot`는 UMG(Slate) 미캡처 → UI 시각검증은 사용자 눈/`take_automation_screenshot_of_ui`로. 보스바도 동일하게 스샷에 안 잡힘.
- 함정: PIE종료+save+quit 한 콜에 연달아 → TaskGraph.cpp:689 셧다운 Assertion. end_play 후 별도 콜에서 quit.
- [진행] batch 2 순차: ①타겟팅 데칼 프리뷰 ②머리위 WorldSpace 캐스트바 ③WBP_SkillTree ④ASkillProjectile
  - [x] ①타겟팅 데칼 프리뷰: PointTarget 스킬은 Z/X/C **홀드→커서 표시+바닥 범위 데칼(반경)→릴리스 발동**. 컴포넌트 TickComponent서 GetGroundAimPoint로 데칼 위치 갱신. BeginTargeting서 SetShowMouseCursor(true)+GameAndUI, EndTargeting서 복원. ActivateSlot=Pressed 분기, ReleaseSlot=Released. 즉발 스킬(NoTarget/Direction/Actor)은 Pressed 즉시 FireSlot.
  - 마우스 안 보이던 문제(사용자 지적): 평소 GameOnly라 커서 숨김 → 조준 중에만 커서 표시로 해결.
  - 데칼 머티리얼: skill.RangeDecalMaterial 우선, 없으면 DefaultRangeDecalMaterial. 데모 2종+컴포넌트 기본값=M_ChargeWarning 연결됨.
  - ✅ PIE 검증: activate_slot→커서 표시+데칼1개 스폰, release_slot→발동(cd5)+데칼제거+커서숨김.
  - [x] ②머리위 WorldSpace 캐스트바: `USkillCastBarWorldWidget`(코드전용) + 컴포넌트 BeginPlay서 UWidgetComponent(Screen space, 머리위 z110) 생성. 서버 `Multicast_CastStarted(Duration)`→각 클라 로컬 타이머로 채움(IsWorldCasting/GetWorldCastProgress). 클럭差 회피 위해 멀티캐스트 수신 시점부터 로컬 진행.
  - [x] ③스킬트리: `USkillTreeWidget`(중앙 팝업)+`USkillTreeEntryWidget`(행 [이름][Z][X][C]). T키/콘솔 ToggleSkillTree로 토글, SwitchToUIInput/GameInput. AssignSkill→서버 RPC→복제→OnSlotsChanged로 HUD/패널 갱신.
  - [x] ④투사체: `ASkillProjectile`(복제, ProjectileMovement+Sphere+Niagara FlightVFX). DeliveryType=Projectile이면 Detonate서 서버 스폰→충돌 시 `UGA_SkillExecutor::ExecuteSkillBurstAt`(정적, 착탄점 AOE판정+VFX+모듈). SkillDefinition에 ProjectileSpeed/Radius 추가.
  - 함정: UFUNCTION/델리게이트 파라미터명 `Slot` 금지(UWidget::Slot shadow)→InSlot. 정적 IsHostileValidTarget로 팀/사망/래그돌 필터 공유.

### ✅ batch 2 전부 완료 + PIE 검증
- ①타겟팅 프리뷰: 커서+데칼 스폰→릴리스 발동 ✓
- ②월드 캐스트바: 시전 시 is_world_casting=true(멀티캐스트) ✓
- ③스킬트리: T키/콘솔 토글 + AssignSkill로 슬롯 교체 확인(GravityWell→Fireball) ✓
- ④투사체: Fireball 발동→비행→명중, 적 HP 1000→882.5(~120 dmg) ✓
- 데모 스킬 3종: GravityWell(PointTarget+AOE), FlameField(지속 필드), Fireball(Direction+Projectile). 풀=이 3종, 슬롯 Z/X/C 기본 배정.
- 조작: **Z/X/C**=슬롯(PointTarget은 홀드 조준→릴리스), **T**=스킬트리. 콘솔 UseSkill/ToggleSkillTree도 유지.

### 멀티 검증 + 버그수정
- ✅ MP: 2인 리슨서버 PIE, 두 폰 EquippedSkills 복제, 네트워크 세션서 발동→데미지(882.5). 클라창 시각(머리위캐스트바/VFX/투사체)은 사용자 관찰 필요(MCP가 Slate/2nd창 구동 불가).
- 🐞 **이펙트가 캐릭터에 뜨던 버그**: `EndTargeting`이 커서를 먼저 숨기고 발동 → ResolveTargeting이 커서 대신 카메라 폴백 → 캐릭터 발밑에 VFX. 수정=커서 살아있는 동안 FireSlot 먼저, 그 다음 커서 복원.
- 타겟팅 조준: GetGroundAimPoint를 커서 트레이스→**캐스터 발밑 평면 교차**로 변경(폰/지오메트리 안 걸림) + 지면 down-trace 스냅.
- FlameField 지속 3→2초(사용자 요청).

### Phase 2 — 남은 폴리시(선택)

### 데모 스킬
- `/Game/Skills/DA_Skill_GravityWell` (생성됨): PointTarget+AOE, Sequential, Pull1400→Stun→Damage90, ImpactVFX=N_MagicalExplosion, cast0.8 cd5 range900 radius350

### Phase 2 — 예정
- [ ] ASkillProjectile (Projectile/Beam 전달)
- [ ] WBP_SkillHUD / WBP_SkillSlot (쿨다운 sweep + 단축키)
- [ ] WBP_CastBarLocal / WBP_CastBarWorld (머리 위)
- [ ] 타겟팅 데칼 프리뷰 (홀드→릴리스)
- [ ] WBP_SkillTree (슬롯 배정 UI)
- [ ] 데모 스킬 3종 DataAsset + 입력 바인딩 + PIE/멀티 검증

---

## 결정/함정 메모
- GA는 보스 돌진과 동일하게 **ServerOnly** — 마우스 타겟팅은 클라가 해석해 RPC로 서버에 전달(서버는 커서 모름).
- Executor는 `InstancedPerActor` — 첫 활성화 전엔 인스턴스가 없어 직접 주입 불가 → 컴포넌트가 `PendingSkill/Origin/Direction`을 stash하고 GA가 `ConsumePendingActivation`으로 회수. 슬롯 3개가 1인스턴스 공유 → 동시 1캐스트.
- **쿨다운 함정**: 스탠드얼론/리슨서버는 클라=서버 동일 인스턴스 → 클라가 예측 쿨다운 먼저 켜면 서버가 자기차단. 쿨다운은 서버 발동 성공 시에만 시작 + Client RPC로 원격클라 UI 예측.
- 마우스 타겟팅(PointTarget)은 실제 커서 필요 → MCP 헤드리스 PIE에선 검증 불가. 모듈 로직 검증은 NoTarget 자기중심으로 격리해서 함.
- enum은 Python에 `unreal.SkillTargetingMode.POINT_TARGET` 형태(앞 E 제거)로 노출. int/생성자 불가, 멤버 attr만.
- Stun/Slow 모듈은 기존 `GE_StatusShocked`/`GE_StatusChilled` 재사용(지속시간은 GE에 baked, 클래스 교체 가능).
- 모듈 베이스는 `UCLASS(Abstract, EditInlineNew, DefaultToInstanced)` + DataAsset에 `Instanced` 배열 → 에디터에서 +로 조합.
