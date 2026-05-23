# -*- encoding: utf-8 -*-
"""
Step 1.2 + Step 1.3: 火球弹道技能完整链路（含冷却与消耗）
- GA_Fireball: 火球技能类，生成弹道、命中伤害
- BP_Projectile: 弹道 Actor，飞行 + 碰撞检测 + 命中伤害

Step 1.3 新增：
- CooldownGameplayEffectClass → GE_Cooldown_Fireball
- CostGameplayEffectClass → GE_Cost_Fireball
- CommitAbility() 流程：检查消耗 + 启动冷却
- ActivationBlockedTags → 冷却期间阻止激活

⚠️ GE 配置说明：
- GE_Cost_Fireball 的 Modifiers（扣减 Mana）需要在编辑器中创建蓝图子类配置，
  原因同 Step 1.2 的 GE_Damage_Fireball（NePy ArrayWrapper 无法赋值 Modifiers）
- GE_Cooldown_Fireball 的 Tag 由 TargetTagsGameplayEffectComponent 配置，可能导致同样问题
- 如果 Python 中 Tag 配置失败，也需要蓝图资产。见 ge_cooldown.py 和 ge_cost.py 中的降级说明
"""
import ue


# ==================== 1. 火球 GameplayAbility ====================

@ue.uclass()
class GA_Fireball(ue.GameplayAbility):
    """火球弹道技能：CommitAbility 检查消耗/冷却 → 生成弹道 → 命中伤害"""

    def __init_default__(self):
        self.InstancingPolicy = (
            ue.EGameplayAbilityInstancingPolicy.InstancedPerExecution
        )
        self.NetExecutionPolicy = (
            ue.EGameplayAbilityNetExecutionPolicy.LocalPredicted
        )

        # Step 1.3: 冷却 Tag 配置（无论 GE 是否通过蓝图配置，这些 Tag 容器都需要）
        fireball_tag = ue.GameplayTag()
        fireball_tag.TagName = "Ability.Fireball"
        self.AbilityTags.GameplayTags.Append(fireball_tag)

        cooldown_tag = ue.GameplayTag()
        cooldown_tag.TagName = "Cooldown.Fireball"
        self.ActivationBlockedTags.GameplayTags.Append(cooldown_tag)

    def configure_ge_classes(self):
        """运行时配置 Cooldown/Cost GE 类。
        优先用蓝图资产（配置了 Tag/Modifiers），回退 Python 类。
        """
        # 冷却 GE: 优先蓝图
        self.CooldownGameplayEffectClass = self._load_ge(
            "BP_GE_Cooldown_Fireball", "gas.effects.ge_cooldown", "GE_Cooldown_Fireball")
        # 消耗 GE: 优先蓝图
        self.CostGameplayEffectClass = self._load_ge(
            "BP_GE_Cost_Fireball", "gas.effects.ge_cost", "GE_Cost_Fireball")

    def _load_ge(self, bp_name, module_name, class_name):
        """加载 GE 类：优先 FindObject（PIE 可用），回退 Python 类"""
        import importlib
        path = f"/Game/Blueprint/GAS/{bp_name}.{bp_name}_C"
        # 方式 1: FindObject
        ge = ue.FindObject(path)
        if ge:
            print(f"[GA_Fireball] {bp_name} = FindObject")
            return ge
        # 方式 2: FindClass（不带 _C 后缀）
        path2 = f"/Game/Blueprint/GAS/{bp_name}"
        ge = ue.FindClass(path2)
        if ge:
            print(f"[GA_Fireball] {bp_name} = FindClass")
            return ge
        # 方式 3: Python 类
        try:
            mod = importlib.import_module(module_name)
            cls = getattr(mod, class_name)
            ge = cls.Class()
            if ge:
                print(f"[GA_Fireball] {bp_name} = Python 类")
                return ge
        except Exception:
            pass
        print(f"[GA_Fireball] 警告: 所有方式加载 {bp_name} 均失败")
        return None

    def try_commit_and_fire(self, asc, avatar):
        """Step 1.3: 手动模拟 Commit 流程（CDO 无法调用真正的 CommitAbility）
        1. 检查冷却 Tag
        2. 检查 Mana 消耗
        3. 手动 Apply Cost/Cooldown GE 到自身
        4. 发射弹道
        """
        from gas.setup_character import AttrSet_Base

        self.configure_ge_classes()

        attr_set = asc.GetAttributeSet(AttrSet_Base.Class())
        if not attr_set:
            print("[GA_Fireball] 错误: 获取不到 AttributeSet")
            return False

        # 1. 检查冷却
        cooldown_tag = ue.GameplayTag()
        cooldown_tag.TagName = "Cooldown.Fireball"
        if asc.GetGameplayTagCount(cooldown_tag) > 0:
            print("[GA_Fireball] 冷却中，无法激活")
            return False

        # 2. 检查 Mana
        cost_amount = 20.0
        if attr_set.Mana < cost_amount:
            print(f"[GA_Fireball] Mana 不足 ({attr_set.Mana}/{attr_set.MaxMana})")
            return False

        # 3. 手动 Apply Cost GE
        if self.CostGameplayEffectClass:
            ctx = asc.MakeEffectContext()
            spec = asc.MakeOutgoingSpec(self.CostGameplayEffectClass, 1.0, ctx)
            asc.ApplyGameplayEffectSpecToSelf(spec)
            print("[GA_Fireball] Cost GE 已应用")

        # 4. 手动 Apply Cooldown GE
        if self.CooldownGameplayEffectClass:
            ctx = asc.MakeEffectContext()
            spec = asc.MakeOutgoingSpec(self.CooldownGameplayEffectClass, 1.0, ctx)
            asc.ApplyGameplayEffectSpecToSelf(spec)
            print("[GA_Fireball] Cooldown GE 已应用")

        print(f"[GA_Fireball] 消耗成功 — Mana 剩余 {attr_set.Mana}")
        self.do_fireball(asc, avatar)
        return True

    def do_fireball(self, asc, avatar):
        """技能主逻辑：生成弹道"""
        # 生成位置：角色头顶偏上
        spawn_location = avatar.GetActorLocation()
        spawn_location.z += 100.0

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

        # 加载 GE_Damage 蓝图资产
        ge_class = (ue.FindObject(
            "/Game/Blueprint/GAS/GE_Damage_Fireball.GE_Damage_Fireball_C")
            or ue.FindClass("/Game/Blueprint/GAS/GE_Damage_Fireball"))
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
