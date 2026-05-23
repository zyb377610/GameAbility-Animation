# -*- encoding: utf-8 -*-
"""
Step 1.2: 火球弹道技能完整链路
- GA_Fireball: 火球技能类，生成弹道、命中伤害
- BP_Projectile: 弹道 Actor，飞行 + 碰撞检测 + 命中伤害
"""
import ue


# ==================== 1. 火球 GameplayAbility ====================

@ue.uclass()
class GA_Fireball(ue.GameplayAbility):
    """火球弹道技能：生成弹道 → 命中伤害"""

    def __init_default__(self):
        self.InstancingPolicy = (
            ue.EGameplayAbilityInstancingPolicy.InstancedPerExecution
        )
        self.NetExecutionPolicy = (
            ue.EGameplayAbilityNetExecutionPolicy.LocalPredicted
        )

    def do_fireball(self, asc, avatar):
        """技能主逻辑：由外部传入 ASC 和 Avatar Actor"""
        # 生成位置：角色头顶偏上
        spawn_location = avatar.GetActorLocation()
        spawn_location.z += 100.0
        forward = avatar.GetActorForwardVector()

        transform = ue.Transform()
        transform.Translation = spawn_location
        transform.Rotation = ue.Quat(
            ue.Rotator(0.0, avatar.GetActorRotation().Yaw, 0.0)
        )
        transform.Scale3D = ue.Vector(1.0, 1.0, 1.0)

        spawn_class = BP_Projectile.Class()
        if not spawn_class:
            print("[GA_Fireball] 错误: BP_Projectile UClass 未找到")
            return

        world = avatar.GetWorld()
        projectile = ue.GameplayStatics.BeginDeferredActorSpawnFromClass(
            world, spawn_class, transform
        )
        if not projectile:
            print("[GA_Fireball] 错误: 弹道 Spawn 失败")
            return

        # 注入数据（instigator_avatar 必须在 FinishSpawning 前设置）
        projectile.instigator_avatar = avatar
        ue.GameplayStatics.FinishSpawningActor(projectile, transform)
        projectile.from_ability = self
        projectile._owner_asc = asc
        projectile.Speed = 2000.0
        projectile.MaxLifetime = 5.0
        projectile._elapsed = 0.0
        projectile._direction = projectile.GetActorForwardVector()

        # FinishSpawning 后绑定碰撞（避免立刻命中自己）
        def on_overlap(overlap_comp, other_actor, other_comp, other_body_idx,
                       b_from_sweep, sweep_result):
            projectile._on_hit(other_actor)
        projectile.CollisionSphere.OnComponentBeginOverlap.Add(on_overlap)

    def notify_projectile_hit(self, target_asc, owner_asc):
        """弹道命中后：Apply GE 到目标"""
        from gas.setup_character import init_gas_for_actor, AttrSet_Base

        target_actor = target_asc.OwnerActor
        if target_actor:
            init_gas_for_actor(target_actor)

        if not owner_asc:
            print("[GA_Fireball] 错误: owner_asc 无效")
            return

        # 加载蓝图 GE 资产
        ge_class = ue.LoadObject(ue.GameplayEffect,
            "/Game/Blueprints/GAS/GE_Damage_Fireball.GE_Damage_Fireball_C")
        if not ge_class:
            print("[GA_Fireball] 错误: 加载 GE_Damage_Fireball 失败")
            return

        context = owner_asc.MakeEffectContext()
        spec_handle = owner_asc.MakeOutgoingSpec(ge_class, 1.0, context)
        result = owner_asc.ApplyGameplayEffectSpecToTarget(
            spec_handle, target_asc)
        print(f"[GA_Fireball] GE 已应用, Handle={result}")

        attr_set = target_asc.GetAttributeSet(AttrSet_Base.Class())
        if attr_set:
            print(f"[GA_Fireball] 目标 Health={attr_set.Health}")


# ==================== 2. 弹道 Actor ====================

@ue.uclass()
class BP_Projectile(ue.Actor):
    """火球弹道：飞行 + 碰撞检测 + 命中伤害"""

    Damage = ue.uproperty(10.0)
    Speed = ue.uproperty(2000.0)
    MaxLifetime = ue.uproperty(5.0)

    CollisionSphere = ue.ucomponent(ue.SphereComponent)
    ProjectileMesh = ue.ucomponent(ue.StaticMeshComponent,
                                   attach='CollisionSphere')

    def __init_default__(self):
        """CDO 初始化：设置碰撞和网格"""
        self.CollisionSphere.SetSphereRadius(20.0, False)
        self.CollisionSphere.SetCollisionProfileName("OverlapAllDynamic")
        self.CollisionSphere.SetGenerateOverlapEvents(True)

        try:
            mesh = ue.LoadObject(ue.StaticMesh,
                "/Engine/BasicShapes/Sphere.Sphere")
            if mesh:
                self.ProjectileMesh.SetStaticMesh(mesh)
                self.ProjectileMesh.SetWorldScale3D(
                    ue.Vector(0.5, 0.5, 0.5))
        except Exception:
            pass

        self.ProjectileMesh.SetCollisionEnabled(
            ue.ECollisionEnabled.NoCollision)
        self.PrimaryActorTick.bCanEverTick = True

    @ue.ufunction(override=True)
    def ReceiveBeginPlay(self):
        pass

    @ue.ufunction(override=True)
    def ReceiveTick(self, DeltaSeconds: float):
        """每帧移动弹道"""
        if not hasattr(self, '_elapsed') or self._elapsed is None:
            self._elapsed = 0.0
        if not hasattr(self, '_direction') or self._direction is None:
            self._direction = self.GetActorForwardVector()

        self._elapsed += DeltaSeconds

        ml = self.MaxLifetime
        if ml == 0.0:
            ml = 5.0

        if self._elapsed > ml:
            self.DestroyActor()
            return

        if self._direction:
            spd = self.Speed or 2000.0
            delta = ue.Vector(
                self._direction.X * spd * DeltaSeconds,
                self._direction.Y * spd * DeltaSeconds,
                self._direction.Z * spd * DeltaSeconds,
            )
            self.AddActorWorldOffset(delta, False, False)

    def _on_hit(self, other_actor):
        """碰撞命中处理"""
        if not other_actor:
            return
        # 不命中自己
        instigator = getattr(self, 'instigator_avatar', None)
        if other_actor == instigator:
            return

        # 查找目标 ASC
        target_asc = None
        try:
            target_asc = other_actor.GetComponentByClass(
                ue.AbilitySystemComponent.Class())
        except Exception:
            pass

        if not target_asc:
            self.DestroyActor()
            return

        # 通过 GA 应用 GE 伤害
        from_ability = getattr(self, 'from_ability', None)
        if from_ability and hasattr(from_ability, 'notify_projectile_hit'):
            from_ability.notify_projectile_hit(target_asc, getattr(self, '_owner_asc', None))
        else:
            print("[BP_Projectile] 警告: from_ability 无效，无法应用 GE")

        self.DestroyActor()
