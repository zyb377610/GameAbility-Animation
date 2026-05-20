# -*- encoding: utf-8 -*-
from __future__ import annotations
import typing
T = typing.TypeVar('T')

"""
Blueprint Type Hints
Auto-generated from blueprint_infos_full.json

Generated at: 2025-12-05 15:22:33
"""
from . import *  # noqa

# region Blueprint Classes

class ABP_FP_Copy_C(AnimInstance):
	""" ABP FP Copy """  # noqa

	AnimBlueprintExtension_PropertyAccess: StructProperty

	AnimBlueprintExtension_Base: StructProperty

	AnimGraphNode_Root: StructProperty

	AnimGraphNode_CopyPoseFromMesh: StructProperty

	AnimGraphNode_ControlRig: StructProperty

	def AnimGraph(self, AnimGraph: StructProperty) -> None:
		""" Anim Graph """  # noqa
		pass

	pass

class ABP_FP_Pistol_C(AnimInstance):
	""" ABP FP Pistol """  # noqa

	UberGraphFrame: StructProperty

	AnimBlueprintExtension_PropertyAccess: StructProperty

	AnimBlueprintExtension_Base: StructProperty

	AnimGraphNode_Root: StructProperty

	AnimGraphNode_Slot: StructProperty

	AnimGraphNode_ControlRig: StructProperty

	AnimGraphNode_CopyPoseFromMesh: StructProperty

	__CustomProperty_Ctrl_Head_DAEEF21A4B8AD684557275826D426F26: StructProperty

	__CustomProperty_Aim_DAEEF21A4B8AD684557275826D426F26: StructProperty

	IsMoving: BoolProperty

	bIsInAir: BoolProperty

	HasRifle: BoolProperty

	FirstPersonCharacter: ObjectProperty

	# First Person Camera: ObjectProperty  # [Unsupported] Invalid Python identifier, not supported in script

	Hand_R_Target: StructProperty

	# As BP FPShooter: ObjectProperty  # [Unsupported] Invalid Python identifier, not supported in script

	# First Person Mesh: ObjectProperty  # [Unsupported] Invalid Python identifier, not supported in script

	# Aim Target: StructProperty  # [Unsupported] Invalid Python identifier, not supported in script

	# Is Aiming: BoolProperty  # [Unsupported] Invalid Python identifier, not supported in script

	NewVar: StructProperty

	PitchN: DoubleProperty

	NewVar_0: StructProperty

	GripOffset: StructProperty

	def AnimGraph(self, AnimGraph: StructProperty) -> None:
		""" Anim Graph """  # noqa
		pass

	def EvaluateGraphExposedInputs_ExecuteUbergraph_ABP_FP_Pistol_AnimGraphNode_ControlRig_DAEEF21A4B8AD684557275826D426F26(self) -> None:
		""" Evaluate Graph Exposed Inputs Execute Ubergraph ABP FP Pistol Anim Graph Node Control Rig DAEEF21A4B8AD684557275826D426F26 """  # noqa
		pass

	def BlueprintUpdateAnimation(self, DeltaTimeX: FloatProperty) -> None:
		""" Executed when the Animation is updated """  # noqa
		pass

	def BlueprintBeginPlay(self) -> None:
		""" Executed when begin play is called on the owning component """  # noqa
		pass

	def ExecuteUbergraph_ABP_FP_Pistol(self, EntryPoint: IntProperty, Temp_object_Variable: ArrayProperty, CallFunc_Conv_DoubleToVector_ReturnValue: StructProperty, CallFunc_Conv_VectorToTransform_ReturnValue: StructProperty, CallFunc_MakeTransform_ReturnValue: StructProperty, K2Node_Event_DeltaTimeX: FloatProperty, CallFunc_TryGetPawnOwner_ReturnValue: ObjectProperty, CallFunc_TryGetPawnOwner_ReturnValue_1: ObjectProperty, CallFunc_IsValid_ReturnValue: BoolProperty, CallFunc_GetController_ReturnValue: ObjectProperty, CallFunc_GetControlRotation_ReturnValue: StructProperty, CallFunc_GetActorRightVector_ReturnValue: StructProperty, CallFunc_BreakRotator_Roll: FloatProperty, CallFunc_BreakRotator_Pitch: FloatProperty, CallFunc_BreakRotator_Yaw: FloatProperty, CallFunc_GetForwardVector_ReturnValue: StructProperty, CallFunc_Multiply_DoubleDouble_ReturnValue: DoubleProperty, CallFunc_Dot_VectorVector_ReturnValue: DoubleProperty, CallFunc_MakeRotator_ReturnValue: StructProperty, CallFunc_GetVelocity_ReturnValue: StructProperty, CallFunc_GetActorForwardVector_ReturnValue: StructProperty, CallFunc_Normal_ReturnValue: StructProperty, CallFunc_GetVelocity_ReturnValue_1: StructProperty, CallFunc_Dot_VectorVector_ReturnValue_1: DoubleProperty, CallFunc_VSize_ReturnValue: DoubleProperty, CallFunc_Dot_VectorVector_ReturnValue_2: DoubleProperty, CallFunc_Greater_DoubleDouble_ReturnValue: BoolProperty, CallFunc_MakeVector2D_ReturnValue: StructProperty, CallFunc_GetMovementComponent_ReturnValue: ObjectProperty, CallFunc_IsFalling_ReturnValue: BoolProperty, CallFunc_TryGetPawnOwner_ReturnValue_2: ObjectProperty, K2Node_DynamicCast_AsShooter_Character: ObjectProperty, K2Node_DynamicCast_bSuccess: BoolProperty, CallFunc_K2_GetComponentLocation_ReturnValue: StructProperty, CallFunc_GetForwardVector_ReturnValue_1: StructProperty, K2Node_MakeArray_Array: ArrayProperty, CallFunc_Multiply_VectorVector_ReturnValue: StructProperty, CallFunc_Add_VectorVector_ReturnValue: StructProperty, CallFunc_LineTraceSingleForObjects_OutHit: StructProperty, CallFunc_LineTraceSingleForObjects_ReturnValue: BoolProperty, CallFunc_BreakHitResult_bBlockingHit: BoolProperty, CallFunc_BreakHitResult_bInitialOverlap: BoolProperty, CallFunc_BreakHitResult_Time: FloatProperty, CallFunc_BreakHitResult_Distance: FloatProperty, CallFunc_BreakHitResult_Location: StructProperty, CallFunc_BreakHitResult_ImpactPoint: StructProperty, CallFunc_BreakHitResult_Normal: StructProperty, CallFunc_BreakHitResult_ImpactNormal: StructProperty, CallFunc_BreakHitResult_PhysMat: ObjectProperty, CallFunc_BreakHitResult_HitActor: ObjectProperty, CallFunc_BreakHitResult_HitComponent: ObjectProperty, CallFunc_BreakHitResult_HitBoneName: NameProperty, CallFunc_BreakHitResult_BoneName: NameProperty, CallFunc_BreakHitResult_HitItem: IntProperty, CallFunc_BreakHitResult_ElementIndex: IntProperty, CallFunc_BreakHitResult_FaceIndex: IntProperty, CallFunc_BreakHitResult_TraceStart: StructProperty, CallFunc_BreakHitResult_TraceEnd: StructProperty, CallFunc_SelectVector_ReturnValue: StructProperty, CallFunc_Multiply_DoubleDouble_A_ImplicitCast: DoubleProperty, CallFunc_MakeRotator_Yaw_ImplicitCast: FloatProperty) -> None:
		""" Execute Ubergraph ABP FP Pistol """  # noqa
		pass

	pass

class ABP_FP_Weapon_C(AnimInstance):
	""" ABP FP Weapon """  # noqa

	UberGraphFrame: StructProperty

	AnimBlueprintExtension_PropertyAccess: StructProperty

	AnimBlueprintExtension_Base: StructProperty

	AnimGraphNode_Root: StructProperty

	AnimGraphNode_Slot: StructProperty

	AnimGraphNode_ControlRig: StructProperty

	AnimGraphNode_CopyPoseFromMesh: StructProperty

	__CustomProperty_Ctrl_Head_3DC6268C49E5FBCF1CD84391DB96A114: StructProperty

	__CustomProperty_Aim_3DC6268C49E5FBCF1CD84391DB96A114: StructProperty

	IsMoving: BoolProperty

	bIsInAir: BoolProperty

	HasRifle: BoolProperty

	FirstPersonCharacter: ObjectProperty

	# First Person Camera: ObjectProperty  # [Unsupported] Invalid Python identifier, not supported in script

	Hand_R_Target: StructProperty

	# As BP FPShooter: ObjectProperty  # [Unsupported] Invalid Python identifier, not supported in script

	# First Person Mesh: ObjectProperty  # [Unsupported] Invalid Python identifier, not supported in script

	# Aim Target: StructProperty  # [Unsupported] Invalid Python identifier, not supported in script

	# Is Aiming: BoolProperty  # [Unsupported] Invalid Python identifier, not supported in script

	NewVar: StructProperty

	PitchN: DoubleProperty

	NewVar_0: StructProperty

	GripOffset: StructProperty

	def AnimGraph(self, AnimGraph: StructProperty) -> None:
		""" Anim Graph """  # noqa
		pass

	def EvaluateGraphExposedInputs_ExecuteUbergraph_ABP_FP_Weapon_AnimGraphNode_ControlRig_3DC6268C49E5FBCF1CD84391DB96A114(self) -> None:
		""" Evaluate Graph Exposed Inputs Execute Ubergraph ABP FP Weapon Anim Graph Node Control Rig 3DC6268C49E5FBCF1CD84391DB96A114 """  # noqa
		pass

	def BlueprintUpdateAnimation(self, DeltaTimeX: FloatProperty) -> None:
		""" Executed when the Animation is updated """  # noqa
		pass

	def BlueprintBeginPlay(self) -> None:
		""" Executed when begin play is called on the owning component """  # noqa
		pass

	def ExecuteUbergraph_ABP_FP_Weapon(self, EntryPoint: IntProperty, Temp_object_Variable: ArrayProperty, CallFunc_Conv_DoubleToVector_ReturnValue: StructProperty, CallFunc_Conv_VectorToTransform_ReturnValue: StructProperty, CallFunc_MakeTransform_ReturnValue: StructProperty, K2Node_Event_DeltaTimeX: FloatProperty, CallFunc_TryGetPawnOwner_ReturnValue: ObjectProperty, CallFunc_TryGetPawnOwner_ReturnValue_1: ObjectProperty, CallFunc_IsValid_ReturnValue: BoolProperty, CallFunc_GetController_ReturnValue: ObjectProperty, CallFunc_GetControlRotation_ReturnValue: StructProperty, CallFunc_GetActorRightVector_ReturnValue: StructProperty, CallFunc_BreakRotator_Roll: FloatProperty, CallFunc_BreakRotator_Pitch: FloatProperty, CallFunc_BreakRotator_Yaw: FloatProperty, CallFunc_GetForwardVector_ReturnValue: StructProperty, CallFunc_Multiply_DoubleDouble_ReturnValue: DoubleProperty, CallFunc_Dot_VectorVector_ReturnValue: DoubleProperty, CallFunc_MakeRotator_ReturnValue: StructProperty, CallFunc_GetVelocity_ReturnValue: StructProperty, CallFunc_GetActorForwardVector_ReturnValue: StructProperty, CallFunc_Normal_ReturnValue: StructProperty, CallFunc_GetVelocity_ReturnValue_1: StructProperty, CallFunc_Dot_VectorVector_ReturnValue_1: DoubleProperty, CallFunc_VSize_ReturnValue: DoubleProperty, CallFunc_Dot_VectorVector_ReturnValue_2: DoubleProperty, CallFunc_Greater_DoubleDouble_ReturnValue: BoolProperty, CallFunc_MakeVector2D_ReturnValue: StructProperty, CallFunc_GetMovementComponent_ReturnValue: ObjectProperty, CallFunc_IsFalling_ReturnValue: BoolProperty, CallFunc_TryGetPawnOwner_ReturnValue_2: ObjectProperty, K2Node_DynamicCast_AsShooter_Character: ObjectProperty, K2Node_DynamicCast_bSuccess: BoolProperty, CallFunc_K2_GetComponentLocation_ReturnValue: StructProperty, CallFunc_GetForwardVector_ReturnValue_1: StructProperty, K2Node_MakeArray_Array: ArrayProperty, CallFunc_Multiply_VectorVector_ReturnValue: StructProperty, CallFunc_Add_VectorVector_ReturnValue: StructProperty, CallFunc_LineTraceSingleForObjects_OutHit: StructProperty, CallFunc_LineTraceSingleForObjects_ReturnValue: BoolProperty, CallFunc_BreakHitResult_bBlockingHit: BoolProperty, CallFunc_BreakHitResult_bInitialOverlap: BoolProperty, CallFunc_BreakHitResult_Time: FloatProperty, CallFunc_BreakHitResult_Distance: FloatProperty, CallFunc_BreakHitResult_Location: StructProperty, CallFunc_BreakHitResult_ImpactPoint: StructProperty, CallFunc_BreakHitResult_Normal: StructProperty, CallFunc_BreakHitResult_ImpactNormal: StructProperty, CallFunc_BreakHitResult_PhysMat: ObjectProperty, CallFunc_BreakHitResult_HitActor: ObjectProperty, CallFunc_BreakHitResult_HitComponent: ObjectProperty, CallFunc_BreakHitResult_HitBoneName: NameProperty, CallFunc_BreakHitResult_BoneName: NameProperty, CallFunc_BreakHitResult_HitItem: IntProperty, CallFunc_BreakHitResult_ElementIndex: IntProperty, CallFunc_BreakHitResult_FaceIndex: IntProperty, CallFunc_BreakHitResult_TraceStart: StructProperty, CallFunc_BreakHitResult_TraceEnd: StructProperty, CallFunc_SelectVector_ReturnValue: StructProperty, CallFunc_Multiply_DoubleDouble_A_ImplicitCast: DoubleProperty, CallFunc_MakeRotator_Yaw_ImplicitCast: FloatProperty) -> None:
		""" Execute Ubergraph ABP FP Weapon """  # noqa
		pass

	pass

class ABP_TP_Pistol_C(AnimInstance):
	""" ABP TP Pistol """  # noqa

	UberGraphFrame: StructProperty

	__AnimBlueprintMutables: StructProperty

	AnimBlueprintExtension_PropertyAccess: StructProperty

	AnimBlueprintExtension_Base: StructProperty

	AnimGraphNode_Root: StructProperty

	AnimGraphNode_TransitionResult_6: StructProperty

	AnimGraphNode_TransitionResult_5: StructProperty

	AnimGraphNode_TransitionResult_4: StructProperty

	AnimGraphNode_TransitionResult_3: StructProperty

	AnimGraphNode_TransitionResult_2: StructProperty

	AnimGraphNode_TransitionResult_1: StructProperty

	AnimGraphNode_TransitionResult: StructProperty

	AnimGraphNode_BlendSpacePlayer: StructProperty

	AnimGraphNode_StateResult_4: StructProperty

	AnimGraphNode_SequencePlayer_4: StructProperty

	AnimGraphNode_StateResult_3: StructProperty

	AnimGraphNode_SequencePlayer_3: StructProperty

	AnimGraphNode_StateResult_2: StructProperty

	AnimGraphNode_SequencePlayer_2: StructProperty

	AnimGraphNode_StateResult_1: StructProperty

	AnimGraphNode_SequencePlayer_1: StructProperty

	AnimGraphNode_StateResult: StructProperty

	AnimGraphNode_StateMachine: StructProperty

	AnimGraphNode_LayeredBoneBlend: StructProperty

	AnimGraphNode_SequencePlayer: StructProperty

	AnimGraphNode_RotationOffsetBlendSpace: StructProperty

	bIsInAir: BoolProperty

	IsMoving: BoolProperty

	NewVar: StructProperty

	NewVar_0: StructProperty

	PitchN: DoubleProperty

	def AnimGraph(self, AnimGraph: StructProperty) -> None:
		""" Anim Graph """  # noqa
		pass

	def EvaluateGraphExposedInputs_ExecuteUbergraph_ABP_TP_Pistol_AnimGraphNode_TransitionResult_AAB302824D024A4D7F7106B1ADBA4817(self) -> None:
		""" Evaluate Graph Exposed Inputs Execute Ubergraph ABP TP Pistol Anim Graph Node Transition Result AAB302824D024A4D7F7106B1ADBA4817 """  # noqa
		pass

	def EvaluateGraphExposedInputs_ExecuteUbergraph_ABP_TP_Pistol_AnimGraphNode_TransitionResult_4D6CF25C4C11C0E0A25704B53C11A6F2(self) -> None:
		""" Evaluate Graph Exposed Inputs Execute Ubergraph ABP TP Pistol Anim Graph Node Transition Result 4D6CF25C4C11C0E0A25704B53C11A6F2 """  # noqa
		pass

	def BlueprintUpdateAnimation(self, DeltaTimeX: FloatProperty) -> None:
		""" Executed when the Animation is updated """  # noqa
		pass

	def ExecuteUbergraph_ABP_TP_Pistol(self, EntryPoint: IntProperty, CallFunc_GetRelevantAnimTimeRemaining_ReturnValue: FloatProperty, CallFunc_LessEqual_DoubleDouble_ReturnValue: BoolProperty, CallFunc_TryGetPawnOwner_ReturnValue: ObjectProperty, CallFunc_GetVelocity_ReturnValue: StructProperty, CallFunc_GetActorRightVector_ReturnValue: StructProperty, CallFunc_Normal_ReturnValue: StructProperty, CallFunc_GetActorForwardVector_ReturnValue: StructProperty, CallFunc_Dot_VectorVector_ReturnValue: DoubleProperty, CallFunc_Dot_VectorVector_ReturnValue_1: DoubleProperty, CallFunc_GetVelocity_ReturnValue_1: StructProperty, CallFunc_MakeVector2D_ReturnValue: StructProperty, CallFunc_VSize_ReturnValue: DoubleProperty, CallFunc_Greater_DoubleDouble_ReturnValue: BoolProperty, CallFunc_GetMovementComponent_ReturnValue: ObjectProperty, CallFunc_IsFalling_ReturnValue: BoolProperty, CallFunc_TryGetPawnOwner_ReturnValue_1: ObjectProperty, K2Node_Event_DeltaTimeX: FloatProperty, CallFunc_IsValid_ReturnValue: BoolProperty, CallFunc_TryGetPawnOwner_ReturnValue_2: ObjectProperty, CallFunc_GetController_ReturnValue: ObjectProperty, CallFunc_GetRelevantAnimTimeRemainingFraction_ReturnValue: FloatProperty, CallFunc_IsValid_ReturnValue_1: BoolProperty, CallFunc_LessEqual_DoubleDouble_ReturnValue_1: BoolProperty, CallFunc_GetControlRotation_ReturnValue: StructProperty, CallFunc_GetForwardVector_ReturnValue: StructProperty, CallFunc_BreakRotator_Roll: FloatProperty, CallFunc_BreakRotator_Pitch: FloatProperty, CallFunc_BreakRotator_Yaw: FloatProperty, CallFunc_Dot_VectorVector_ReturnValue_2: DoubleProperty, CallFunc_Multiply_DoubleDouble_ReturnValue: DoubleProperty, CallFunc_MakeRotator_ReturnValue: StructProperty, CallFunc_LessEqual_DoubleDouble_A_ImplicitCast: DoubleProperty, CallFunc_LessEqual_DoubleDouble_A_ImplicitCast_1: DoubleProperty, CallFunc_Multiply_DoubleDouble_A_ImplicitCast: DoubleProperty, CallFunc_MakeRotator_Yaw_ImplicitCast: FloatProperty) -> None:
		""" Execute Ubergraph ABP TP Pistol """  # noqa
		pass

	pass

class ABP_TP_Rifle_C(AnimInstance):
	""" ABP TP Rifle """  # noqa

	UberGraphFrame: StructProperty

	__AnimBlueprintMutables: StructProperty

	AnimBlueprintExtension_PropertyAccess: StructProperty

	AnimBlueprintExtension_Base: StructProperty

	AnimGraphNode_Root: StructProperty

	AnimGraphNode_TransitionResult_6: StructProperty

	AnimGraphNode_TransitionResult_5: StructProperty

	AnimGraphNode_TransitionResult_4: StructProperty

	AnimGraphNode_TransitionResult_3: StructProperty

	AnimGraphNode_TransitionResult_2: StructProperty

	AnimGraphNode_TransitionResult_1: StructProperty

	AnimGraphNode_TransitionResult: StructProperty

	AnimGraphNode_BlendSpacePlayer: StructProperty

	AnimGraphNode_StateResult_4: StructProperty

	AnimGraphNode_SequencePlayer_4: StructProperty

	AnimGraphNode_StateResult_3: StructProperty

	AnimGraphNode_SequencePlayer_3: StructProperty

	AnimGraphNode_StateResult_2: StructProperty

	AnimGraphNode_SequencePlayer_2: StructProperty

	AnimGraphNode_StateResult_1: StructProperty

	AnimGraphNode_SequencePlayer_1: StructProperty

	AnimGraphNode_StateResult: StructProperty

	AnimGraphNode_StateMachine: StructProperty

	AnimGraphNode_LayeredBoneBlend: StructProperty

	AnimGraphNode_SequencePlayer: StructProperty

	AnimGraphNode_RotationOffsetBlendSpace: StructProperty

	bIsInAir: BoolProperty

	IsMoving: BoolProperty

	NewVar: StructProperty

	NewVar_0: StructProperty

	PitchN: DoubleProperty

	def AnimGraph(self, AnimGraph: StructProperty) -> None:
		""" Anim Graph """  # noqa
		pass

	def EvaluateGraphExposedInputs_ExecuteUbergraph_ABP_TP_Rifle_AnimGraphNode_TransitionResult_09C60AD4402A5CBC796D8DB948D64B57(self) -> None:
		""" Evaluate Graph Exposed Inputs Execute Ubergraph ABP TP Rifle Anim Graph Node Transition Result 09C60AD4402A5CBC796D8DB948D64B57 """  # noqa
		pass

	def EvaluateGraphExposedInputs_ExecuteUbergraph_ABP_TP_Rifle_AnimGraphNode_TransitionResult_7DDA4253400E1E8BB40A90ADCD331C76(self) -> None:
		""" Evaluate Graph Exposed Inputs Execute Ubergraph ABP TP Rifle Anim Graph Node Transition Result 7DDA4253400E1E8BB40A90ADCD331C76 """  # noqa
		pass

	def BlueprintUpdateAnimation(self, DeltaTimeX: FloatProperty) -> None:
		""" Executed when the Animation is updated """  # noqa
		pass

	def ExecuteUbergraph_ABP_TP_Rifle(self, EntryPoint: IntProperty, CallFunc_GetRelevantAnimTimeRemaining_ReturnValue: FloatProperty, CallFunc_LessEqual_DoubleDouble_ReturnValue: BoolProperty, CallFunc_TryGetPawnOwner_ReturnValue: ObjectProperty, CallFunc_GetVelocity_ReturnValue: StructProperty, CallFunc_GetActorRightVector_ReturnValue: StructProperty, CallFunc_Normal_ReturnValue: StructProperty, CallFunc_GetActorForwardVector_ReturnValue: StructProperty, CallFunc_Dot_VectorVector_ReturnValue: DoubleProperty, CallFunc_Dot_VectorVector_ReturnValue_1: DoubleProperty, CallFunc_GetVelocity_ReturnValue_1: StructProperty, CallFunc_MakeVector2D_ReturnValue: StructProperty, CallFunc_VSize_ReturnValue: DoubleProperty, CallFunc_Greater_DoubleDouble_ReturnValue: BoolProperty, CallFunc_GetMovementComponent_ReturnValue: ObjectProperty, CallFunc_IsFalling_ReturnValue: BoolProperty, CallFunc_TryGetPawnOwner_ReturnValue_1: ObjectProperty, K2Node_Event_DeltaTimeX: FloatProperty, CallFunc_IsValid_ReturnValue: BoolProperty, CallFunc_TryGetPawnOwner_ReturnValue_2: ObjectProperty, CallFunc_GetController_ReturnValue: ObjectProperty, CallFunc_GetRelevantAnimTimeRemainingFraction_ReturnValue: FloatProperty, CallFunc_IsValid_ReturnValue_1: BoolProperty, CallFunc_LessEqual_DoubleDouble_ReturnValue_1: BoolProperty, CallFunc_GetControlRotation_ReturnValue: StructProperty, CallFunc_GetForwardVector_ReturnValue: StructProperty, CallFunc_BreakRotator_Roll: FloatProperty, CallFunc_BreakRotator_Pitch: FloatProperty, CallFunc_BreakRotator_Yaw: FloatProperty, CallFunc_Dot_VectorVector_ReturnValue_2: DoubleProperty, CallFunc_Multiply_DoubleDouble_ReturnValue: DoubleProperty, CallFunc_MakeRotator_ReturnValue: StructProperty, CallFunc_LessEqual_DoubleDouble_A_ImplicitCast: DoubleProperty, CallFunc_LessEqual_DoubleDouble_A_ImplicitCast_1: DoubleProperty, CallFunc_Multiply_DoubleDouble_A_ImplicitCast: DoubleProperty, CallFunc_MakeRotator_Yaw_ImplicitCast: FloatProperty) -> None:
		""" Execute Ubergraph ABP TP Rifle """  # noqa
		pass

	pass

class ABP_Unarmed_C(AnimInstance):
	""" ABP Unarmed """  # noqa

	UberGraphFrame: StructProperty

	__AnimBlueprintMutables: StructProperty

	AnimBlueprintExtension_PropertyAccess: StructProperty

	AnimBlueprintExtension_Base: StructProperty

	AnimGraphNode_Root: StructProperty

	AnimGraphNode_TransitionResult_7: StructProperty

	AnimGraphNode_TransitionResult_6: StructProperty

	AnimGraphNode_BlendSpacePlayer: StructProperty

	AnimGraphNode_StateResult_5: StructProperty

	AnimGraphNode_SequencePlayer_3: StructProperty

	AnimGraphNode_StateResult_4: StructProperty

	AnimGraphNode_StateMachine_1: StructProperty

	AnimGraphNode_SaveCachedPose: StructProperty

	AnimGraphNode_TransitionResult_5: StructProperty

	AnimGraphNode_TransitionResult_4: StructProperty

	AnimGraphNode_TransitionResult_3: StructProperty

	AnimGraphNode_TransitionResult_2: StructProperty

	AnimGraphNode_TransitionResult_1: StructProperty

	AnimGraphNode_TransitionResult: StructProperty

	AnimGraphNode_SequencePlayer_2: StructProperty

	AnimGraphNode_StateResult_3: StructProperty

	AnimGraphNode_SequencePlayer_1: StructProperty

	AnimGraphNode_StateResult_2: StructProperty

	AnimGraphNode_SequencePlayer: StructProperty

	AnimGraphNode_StateResult_1: StructProperty

	AnimGraphNode_UseCachedPose: StructProperty

	AnimGraphNode_StateResult: StructProperty

	AnimGraphNode_StateMachine: StructProperty

	AnimGraphNode_Slot: StructProperty

	AnimGraphNode_ControlRig: StructProperty

	__CustomProperty_ShouldDoIKTrace_BDE61FFC4E653CE138E8E8BB38AA415C: BoolProperty

	Character: ObjectProperty

	MovementComponent: ObjectProperty

	Velocity: StructProperty

	GroundSpeed: DoubleProperty

	ShouldMove: BoolProperty

	IsFalling: BoolProperty

	Strafe: DoubleProperty

	# Running Speed: DoubleProperty  # [Unsupported] Invalid Python identifier, not supported in script

	# Can Strafe: BoolProperty  # [Unsupported] Invalid Python identifier, not supported in script

	def AnimGraph(self, AnimGraph: StructProperty) -> None:
		""" Anim Graph """  # noqa
		pass

	def EvaluateGraphExposedInputs_ExecuteUbergraph_ABP_Unarmed_AnimGraphNode_TransitionResult_0E3DC2854F5C527DFEECC298E8C3D6FA(self) -> None:
		""" Evaluate Graph Exposed Inputs Execute Ubergraph ABP Unarmed Anim Graph Node Transition Result 0E3DC2854F5C527DFEECC298E8C3D6FA """  # noqa
		pass

	def BlueprintUpdateAnimation(self, DeltaTimeX: FloatProperty) -> None:
		""" Executed when the Animation is updated """  # noqa
		pass

	def BlueprintInitializeAnimation(self) -> None:
		""" Executed when the Animation is initialized """  # noqa
		pass

	def ExecuteUbergraph_ABP_Unarmed(self, EntryPoint: IntProperty, K2Node_Event_DeltaTimeX: FloatProperty, CallFunc_GetOwningActor_ReturnValue: ObjectProperty, K2Node_DynamicCast_AsCharacter: ObjectProperty, K2Node_DynamicCast_bSuccess: BoolProperty, CallFunc_IsValid_ReturnValue: BoolProperty, CallFunc_Greater_DoubleDouble_ReturnValue: BoolProperty, CallFunc_Not_PreBool_ReturnValue: BoolProperty, CallFunc_VSizeXY_ReturnValue: DoubleProperty, CallFunc_GetCurrentAcceleration_ReturnValue: StructProperty, CallFunc_NotEqual_VectorVector_ReturnValue: BoolProperty, CallFunc_IsFalling_ReturnValue: BoolProperty, CallFunc_BooleanAND_ReturnValue: BoolProperty, CallFunc_GetActorRightVector_ReturnValue: StructProperty, CallFunc_Normal_ReturnValue: StructProperty, CallFunc_Dot_VectorVector_ReturnValue: DoubleProperty, CallFunc_BreakVector_X: DoubleProperty, CallFunc_BreakVector_Y: DoubleProperty, CallFunc_BreakVector_Z: DoubleProperty, CallFunc_Greater_DoubleDouble_ReturnValue_1: BoolProperty, CallFunc_BooleanAND_ReturnValue_1: BoolProperty, CallFunc_Divide_DoubleDouble_ReturnValue: DoubleProperty) -> None:
		""" Execute Ubergraph ABP Unarmed """  # noqa
		pass

	pass

class BPI_TouchInterface_C(Interface):
	""" BPI Touch Interface """  # noqa

	# def Touch Jump End(...) -> None:  # [Unsupported] Invalid Python identifier, not supported in script
	#     """ Touch Jump End """
	#     pass

	# def Touch Jump Start(...) -> None:  # [Unsupported] Invalid Python identifier, not supported in script
	#     """ Touch Jump Start """
	#     pass

	# def Secondary Thumbstick(...) -> None:  # [Unsupported] Invalid Python identifier, not supported in script
	#     """ Secondary Thumbstick """
	#     pass

	# def Primary Thumbstick(...) -> None:  # [Unsupported] Invalid Python identifier, not supported in script
	#     """ Primary Thumbstick """
	#     pass

	pass

class BPI_Touch_Shooter_C(Interface):
	""" BPI Touch Shooter """  # noqa

	# def Touch Switch Weapon(...) -> None:  # [Unsupported] Invalid Python identifier, not supported in script
	#     """ Touch Switch Weapon """
	#     pass

	# def Touch Shoot End(...) -> None:  # [Unsupported] Invalid Python identifier, not supported in script
	#     """ Touch Shoot End """
	#     pass

	# def Touch Shoot Start(...) -> None:  # [Unsupported] Invalid Python identifier, not supported in script
	#     """ Touch Shoot Start """
	#     pass

	pass

class BP_DoorFrame_C(Actor):
	""" BP Door Frame """  # noqa

	UberGraphFrame: StructProperty

	Box: ObjectProperty

	DefaultSceneRoot: ObjectProperty

	Door_Control_NewTrack_0_9027BDA34FB8CF4858BE4191F8EB344A: FloatProperty

	Door_Control__Direction_9027BDA34FB8CF4858BE4191F8EB344A: ByteProperty

	# Door Control: ObjectProperty  # [Unsupported] Invalid Python identifier, not supported in script

	# Door Size: StructProperty  # [Unsupported] Invalid Python identifier, not supported in script

	# Door Frame: BoolProperty  # [Unsupported] Invalid Python identifier, not supported in script

	# Door Frame Scale: DoubleProperty  # [Unsupported] Invalid Python identifier, not supported in script

	Door: ObjectProperty

	# Closed Position: StructProperty  # [Unsupported] Invalid Python identifier, not supported in script

	# Door Detection Adjust: StructProperty  # [Unsupported] Invalid Python identifier, not supported in script

	# Door Thickness: DoubleProperty  # [Unsupported] Invalid Python identifier, not supported in script

	# Split Door: BoolProperty  # [Unsupported] Invalid Python identifier, not supported in script

	# Door Frame Overide Material: ObjectProperty  # [Unsupported] Invalid Python identifier, not supported in script

	# Add Door: BoolProperty  # [Unsupported] Invalid Python identifier, not supported in script

	Door2: ObjectProperty

	# Closed Position_0: StructProperty  # [Unsupported] Invalid Python identifier, not supported in script

	# def Set Mesh(...) -> None:  # [Unsupported] Invalid Python identifier, not supported in script
	#     """ Set Mesh """
	#     pass

	def UserConstructionScript(self, CallFunc_Vector_GetAbs_ReturnValue: StructProperty, CallFunc_BreakVector_X: DoubleProperty, CallFunc_BreakVector_Y: DoubleProperty, CallFunc_BreakVector_Z: DoubleProperty, CallFunc_Vector_GetAbs_ReturnValue_1: StructProperty, CallFunc_Divide_DoubleDouble_ReturnValue: DoubleProperty, CallFunc_BreakVector_X_1: DoubleProperty, CallFunc_BreakVector_Y_1: DoubleProperty, CallFunc_BreakVector_Z_1: DoubleProperty, CallFunc_Divide_DoubleDouble_ReturnValue_1: DoubleProperty, CallFunc_Divide_DoubleDouble_ReturnValue_2: DoubleProperty, CallFunc_Divide_DoubleDouble_ReturnValue_3: DoubleProperty, CallFunc_Divide_DoubleDouble_ReturnValue_4: DoubleProperty, CallFunc_MakeVector_ReturnValue: StructProperty, CallFunc_Divide_DoubleDouble_ReturnValue_5: DoubleProperty, CallFunc_Divide_DoubleDouble_ReturnValue_6: DoubleProperty, CallFunc_Multiply_DoubleDouble_ReturnValue: DoubleProperty, CallFunc_MakeVector_ReturnValue_1: StructProperty, CallFunc_MakeVector_ReturnValue_2: StructProperty, CallFunc_MakeVector_ReturnValue_3: StructProperty, CallFunc_MakeTransform_ReturnValue: StructProperty, CallFunc_MakeVector_ReturnValue_4: StructProperty, CallFunc_AddComponent_ReturnValue: ObjectProperty, CallFunc_MakeTransform_ReturnValue_1: StructProperty, CallFunc_AddComponent_ReturnValue_1: ObjectProperty, CallFunc_MakeVector_ReturnValue_5: StructProperty, CallFunc_MakeTransform_ReturnValue_2: StructProperty, CallFunc_AddComponent_ReturnValue_2: ObjectProperty, CallFunc_Vector_GetAbs_ReturnValue_2: StructProperty, CallFunc_BreakVector_X_2: DoubleProperty, CallFunc_BreakVector_Y_2: DoubleProperty, CallFunc_BreakVector_Z_2: DoubleProperty, CallFunc_BreakVector_X_3: DoubleProperty, CallFunc_BreakVector_Y_3: DoubleProperty, CallFunc_BreakVector_Z_3: DoubleProperty, CallFunc_Divide_DoubleDouble_ReturnValue_7: DoubleProperty, CallFunc_MakeVector_ReturnValue_6: StructProperty, CallFunc_MakeVector_ReturnValue_7: StructProperty, CallFunc_MakeVector_ReturnValue_8: StructProperty, CallFunc_K2_SetRelativeLocation_SweepHitResult: StructProperty, CallFunc_Add_VectorVector_ReturnValue: StructProperty, CallFunc_Divide_DoubleDouble_ReturnValue_8: DoubleProperty, CallFunc_Abs_ReturnValue: DoubleProperty, CallFunc_Multiply_DoubleDouble_ReturnValue_1: DoubleProperty, CallFunc_Multiply_DoubleDouble_ReturnValue_2: DoubleProperty, CallFunc_Subtract_DoubleDouble_ReturnValue: DoubleProperty, CallFunc_Divide_DoubleDouble_ReturnValue_9: DoubleProperty, CallFunc_Multiply_DoubleDouble_ReturnValue_3: DoubleProperty, CallFunc_MakeVector_ReturnValue_9: StructProperty, CallFunc_MakeTransform_ReturnValue_3: StructProperty, CallFunc_BreakVector_X_4: DoubleProperty, CallFunc_BreakVector_Y_4: DoubleProperty, CallFunc_BreakVector_Z_4: DoubleProperty, CallFunc_Abs_ReturnValue_1: DoubleProperty, CallFunc_Divide_DoubleDouble_ReturnValue_10: DoubleProperty, CallFunc_Subtract_DoubleDouble_ReturnValue_1: DoubleProperty, CallFunc_Multiply_DoubleDouble_ReturnValue_4: DoubleProperty, CallFunc_Divide_DoubleDouble_ReturnValue_11: DoubleProperty, CallFunc_Divide_DoubleDouble_ReturnValue_12: DoubleProperty, CallFunc_MakeVector_ReturnValue_10: StructProperty, CallFunc_MakeVector_ReturnValue_11: StructProperty, CallFunc_MakeTransform_ReturnValue_4: StructProperty, CallFunc_BreakVector_X_5: DoubleProperty, CallFunc_BreakVector_Y_5: DoubleProperty, CallFunc_BreakVector_Z_5: DoubleProperty, CallFunc_Abs_ReturnValue_2: DoubleProperty, CallFunc_Divide_DoubleDouble_ReturnValue_13: DoubleProperty, CallFunc_Subtract_DoubleDouble_ReturnValue_2: DoubleProperty, CallFunc_Divide_DoubleDouble_ReturnValue_14: DoubleProperty, CallFunc_BreakVector_X_6: DoubleProperty, CallFunc_BreakVector_Y_6: DoubleProperty, CallFunc_BreakVector_Z_6: DoubleProperty, CallFunc_MakeVector_ReturnValue_12: StructProperty, CallFunc_MakeVector_ReturnValue_13: StructProperty, CallFunc_Divide_DoubleDouble_ReturnValue_15: DoubleProperty, CallFunc_Multiply_DoubleDouble_ReturnValue_5: DoubleProperty, CallFunc_MakeVector_ReturnValue_14: StructProperty, CallFunc_MakeVector_ReturnValue_15: StructProperty, CallFunc_MakeTransform_ReturnValue_5: StructProperty, CallFunc_MakeTransform_ReturnValue_6: StructProperty, CallFunc_Divide_DoubleDouble_ReturnValue_16: DoubleProperty, CallFunc_MakeVector_ReturnValue_16: StructProperty, CallFunc_MakeTransform_ReturnValue_7: StructProperty) -> None:
		""" Construction script, the place to spawn components and do other setup.
		@note Name used in CreateBlueprint function
		"""  # noqa
		pass

	# def Door Control__FinishedFunc(...) -> None:  # [Unsupported] Invalid Python identifier, not supported in script
	#     """ Door Control  Finished Func """
	#     pass

	# def Door Control__UpdateFunc(...) -> None:  # [Unsupported] Invalid Python identifier, not supported in script
	#     """ Door Control  Update Func """
	#     pass

	def ReceiveActorEndOverlap(self, OtherActor: ObjectProperty) -> None:
		""" Event when an actor no longer overlaps another actor, and they have separated.
		@note Components on both this and the other Actor must have bGenerateOverlapEvents set to true to generate overlap events.
		"""  # noqa
		pass

	def ReceiveActorBeginOverlap(self, OtherActor: ObjectProperty) -> None:
		""" Event when this actor overlaps another actor, for example a player walking into a trigger.
		For events when objects have a blocking collision, for example a player hitting a wall, see 'Hit' events.
		@note Components on both this and the other Actor must have bGenerateOverlapEvents set to true to generate overlap events.
		"""  # noqa
		pass

	def ReceiveBeginPlay(self) -> None:
		""" Event when play begins for this actor. """  # noqa
		pass

	def ExecuteUbergraph_BP_DoorFrame(self, EntryPoint: IntProperty, CallFunc_GetOverlappingActors_OverlappingActors: ArrayProperty, CallFunc_Array_Length_ReturnValue: IntProperty, CallFunc_IsValid_ReturnValue: BoolProperty, CallFunc_EqualEqual_IntInt_ReturnValue: BoolProperty, CallFunc_IsValid_ReturnValue_1: BoolProperty, CallFunc_BreakVector_X: DoubleProperty, CallFunc_BreakVector_Y: DoubleProperty, CallFunc_BreakVector_Z: DoubleProperty, K2Node_Event_OtherActor_1: ObjectProperty, CallFunc_BreakVector_X_1: DoubleProperty, CallFunc_BreakVector_Y_1: DoubleProperty, CallFunc_BreakVector_Z_1: DoubleProperty, CallFunc_Multiply_DoubleDouble_ReturnValue: DoubleProperty, CallFunc_Multiply_DoubleDouble_ReturnValue_1: DoubleProperty, CallFunc_Add_DoubleDouble_ReturnValue: DoubleProperty, CallFunc_Add_DoubleDouble_ReturnValue_1: DoubleProperty, CallFunc_Lerp_ReturnValue: DoubleProperty, CallFunc_Multiply_DoubleDouble_ReturnValue_2: DoubleProperty, CallFunc_Lerp_ReturnValue_1: DoubleProperty, K2Node_Event_OtherActor: ObjectProperty, CallFunc_SelectFloat_ReturnValue: DoubleProperty, CallFunc_SelectFloat_ReturnValue_1: DoubleProperty, CallFunc_MakeVector_ReturnValue: StructProperty, CallFunc_BreakVector_X_2: DoubleProperty, CallFunc_BreakVector_Y_2: DoubleProperty, CallFunc_BreakVector_Z_2: DoubleProperty, CallFunc_K2_SetRelativeLocation_SweepHitResult: StructProperty, CallFunc_BreakVector_X_3: DoubleProperty, CallFunc_BreakVector_Y_3: DoubleProperty, CallFunc_BreakVector_Z_3: DoubleProperty, CallFunc_Lerp_ReturnValue_2: DoubleProperty, CallFunc_MakeVector_ReturnValue_1: StructProperty, CallFunc_K2_SetRelativeLocation_SweepHitResult_1: StructProperty, CallFunc_Lerp_Alpha_ImplicitCast: DoubleProperty, CallFunc_Lerp_Alpha_ImplicitCast_1: DoubleProperty, CallFunc_Lerp_Alpha_ImplicitCast_2: DoubleProperty) -> None:
		""" Execute Ubergraph BP Door Frame """  # noqa
		pass

	pass

class BP_FirstPersonCharacter_C(UE5_6_ProjectCharacter):
	""" BP First Person Character """  # noqa

	UberGraphFrame: StructProperty

	# Target Touch UI: ClassProperty  # [Unsupported] Invalid Python identifier, not supported in script

	def ReceivePossessed(self, NewController: ObjectProperty) -> None:
		""" Event called when the Pawn is possessed by a Controller. Only called on the server (or in standalone) """  # noqa
		pass

	# def Secondary Thumbstick(...) -> None:  # [Unsupported] Invalid Python identifier, not supported in script
	#     """ Secondary Thumbstick """
	#     pass

	# def Primary Thumbstick(...) -> None:  # [Unsupported] Invalid Python identifier, not supported in script
	#     """ Primary Thumbstick """
	#     pass

	# def Touch Jump Start(...) -> None:  # [Unsupported] Invalid Python identifier, not supported in script
	#     """ Touch Jump Start """
	#     pass

	# def Touch Jump End(...) -> None:  # [Unsupported] Invalid Python identifier, not supported in script
	#     """ Touch Jump End """
	#     pass

	def ExecuteUbergraph_BP_FirstPersonCharacter(self, EntryPoint: IntProperty, Temp_struct_Variable: StructProperty, CallFunc_GetPlatformName_ReturnValue: StrProperty, K2Node_SwitchString_CmpSuccess: BoolProperty, K2Node_Event_NewController: ObjectProperty, K2Node_DynamicCast_AsPlayer_Controller: ObjectProperty, K2Node_DynamicCast_bSuccess: BoolProperty, CallFunc_IsPlayerController_ReturnValue: BoolProperty, CallFunc_Create_ReturnValue: ObjectProperty, CallFunc_AddToPlayerScreen_ReturnValue: BoolProperty, CallFunc_GetLocalPlayerSubSystemFromPlayerController_ReturnValue: ObjectProperty, K2Node_Event_Axis_1: StructProperty, CallFunc_AddMappingContext_self_CastInput: InterfaceProperty, CallFunc_BreakVector2D_X: DoubleProperty, CallFunc_BreakVector2D_Y: DoubleProperty, K2Node_Event_Axis: StructProperty, CallFunc_BreakVector2D_X_1: DoubleProperty, CallFunc_BreakVector2D_Y_1: DoubleProperty, CallFunc_DoAim_Yaw_ImplicitCast: FloatProperty, CallFunc_DoAim_Pitch_ImplicitCast: FloatProperty, CallFunc_DoMove_Right_ImplicitCast: FloatProperty, CallFunc_DoMove_Forward_ImplicitCast: FloatProperty) -> None:
		""" Execute Ubergraph BP First Person Character """  # noqa
		pass

	pass

class BP_FirstPersonGameMode_C(UE5_6_ProjectGameMode):
	""" BP First Person Game Mode """  # noqa

	DefaultSceneRoot: ObjectProperty

	pass

class BP_FirstPersonPlayerController_C(UE5_6_ProjectPlayerController):
	""" BP First Person Player Controller """  # noqa

	pass

class BP_HorrorCharacter_C(HorrorCharacter):
	""" BP Horror Character """  # noqa

	UberGraphFrame: StructProperty

	# Target Touch UI: ClassProperty  # [Unsupported] Invalid Python identifier, not supported in script

	def ReceivePossessed(self, NewController: ObjectProperty) -> None:
		""" Event called when the Pawn is possessed by a Controller. Only called on the server (or in standalone) """  # noqa
		pass

	# def Secondary Thumbstick(...) -> None:  # [Unsupported] Invalid Python identifier, not supported in script
	#     """ Secondary Thumbstick """
	#     pass

	# def Primary Thumbstick(...) -> None:  # [Unsupported] Invalid Python identifier, not supported in script
	#     """ Primary Thumbstick """
	#     pass

	# def Touch Jump Start(...) -> None:  # [Unsupported] Invalid Python identifier, not supported in script
	#     """ Touch Jump Start """
	#     pass

	# def Touch Jump End(...) -> None:  # [Unsupported] Invalid Python identifier, not supported in script
	#     """ Touch Jump End """
	#     pass

	def ExecuteUbergraph_BP_HorrorCharacter(self, EntryPoint: IntProperty, Temp_struct_Variable: StructProperty, CallFunc_GetPlatformName_ReturnValue: StrProperty, K2Node_SwitchString_CmpSuccess: BoolProperty, K2Node_Event_NewController: ObjectProperty, K2Node_DynamicCast_AsPlayer_Controller: ObjectProperty, K2Node_DynamicCast_bSuccess: BoolProperty, CallFunc_IsPlayerController_ReturnValue: BoolProperty, CallFunc_Create_ReturnValue: ObjectProperty, CallFunc_AddToPlayerScreen_ReturnValue: BoolProperty, CallFunc_GetLocalPlayerSubSystemFromPlayerController_ReturnValue: ObjectProperty, K2Node_Event_Axis_1: StructProperty, CallFunc_AddMappingContext_self_CastInput: InterfaceProperty, CallFunc_BreakVector2D_X: DoubleProperty, CallFunc_BreakVector2D_Y: DoubleProperty, K2Node_Event_Axis: StructProperty, CallFunc_BreakVector2D_X_1: DoubleProperty, CallFunc_BreakVector2D_Y_1: DoubleProperty, CallFunc_DoAim_Yaw_ImplicitCast: FloatProperty, CallFunc_DoAim_Pitch_ImplicitCast: FloatProperty, CallFunc_DoMove_Right_ImplicitCast: FloatProperty, CallFunc_DoMove_Forward_ImplicitCast: FloatProperty) -> None:
		""" Execute Ubergraph BP Horror Character """  # noqa
		pass

	pass

class BP_HorrorGameMode_C(HorrorGameMode):
	""" BP Horror Game Mode """  # noqa

	DefaultSceneRoot: ObjectProperty

	pass

class BP_HorrorPlayerController_C(HorrorPlayerController):
	""" BP Horror Player Controller """  # noqa

	pass

class BP_JumpPad_C(Actor):
	""" BP Jump Pad """  # noqa

	UberGraphFrame: StructProperty

	Cylinder1: ObjectProperty

	PointLight: ObjectProperty

	NS_JumpDemo: ObjectProperty

	Sphere: ObjectProperty

	Cylinder: ObjectProperty

	DefaultSceneRoot: ObjectProperty

	Velocity: StructProperty

	# Color Target: StructProperty  # [Unsupported] Invalid Python identifier, not supported in script

	def UserConstructionScript(self, CallFunc_BreakColor_R: FloatProperty, CallFunc_BreakColor_G: FloatProperty, CallFunc_BreakColor_B: FloatProperty, CallFunc_BreakColor_A: FloatProperty, CallFunc_MakeVector4_ReturnValue: StructProperty, CallFunc_MakeVector4_X_ImplicitCast: DoubleProperty, CallFunc_MakeVector4_Y_ImplicitCast: DoubleProperty, CallFunc_MakeVector4_Z_ImplicitCast: DoubleProperty, CallFunc_MakeVector4_W_ImplicitCast: DoubleProperty) -> None:
		""" Construction script, the place to spawn components and do other setup.
		@note Name used in CreateBlueprint function
		"""  # noqa
		pass

	def ReceiveActorBeginOverlap(self, OtherActor: ObjectProperty) -> None:
		""" Event when this actor overlaps another actor, for example a player walking into a trigger.
		For events when objects have a blocking collision, for example a player hitting a wall, see 'Hit' events.
		@note Components on both this and the other Actor must have bGenerateOverlapEvents set to true to generate overlap events.
		"""  # noqa
		pass

	def ExecuteUbergraph_BP_JumpPad(self, EntryPoint: IntProperty, K2Node_Event_OtherActor: ObjectProperty, K2Node_DynamicCast_AsCharacter: ObjectProperty, K2Node_DynamicCast_bSuccess: BoolProperty, CallFunc_Multiply_VectorVector_ReturnValue: StructProperty, CallFunc_Add_VectorVector_ReturnValue: StructProperty) -> None:
		""" Execute Ubergraph BP Jump Pad """  # noqa
		pass

	pass

class BP_ShooterAIController_C(ShooterAIController):
	""" BP Shooter AIController """  # noqa

	pass

class BP_ShooterCharacter_C(ShooterCharacter):
	""" BP Shooter Character """  # noqa

	UberGraphFrame: StructProperty

	# Target Touch UI: ClassProperty  # [Unsupported] Invalid Python identifier, not supported in script

	def ReceivePossessed(self, NewController: ObjectProperty) -> None:
		""" Event called when the Pawn is possessed by a Controller. Only called on the server (or in standalone) """  # noqa
		pass

	# def Touch Switch Weapon(...) -> None:  # [Unsupported] Invalid Python identifier, not supported in script
	#     """ Touch Switch Weapon """
	#     pass

	# def Touch Shoot Start(...) -> None:  # [Unsupported] Invalid Python identifier, not supported in script
	#     """ Touch Shoot Start """
	#     pass

	# def Touch Shoot End(...) -> None:  # [Unsupported] Invalid Python identifier, not supported in script
	#     """ Touch Shoot End """
	#     pass

	# def Secondary Thumbstick(...) -> None:  # [Unsupported] Invalid Python identifier, not supported in script
	#     """ Secondary Thumbstick """
	#     pass

	# def Primary Thumbstick(...) -> None:  # [Unsupported] Invalid Python identifier, not supported in script
	#     """ Primary Thumbstick """
	#     pass

	# def Touch Jump Start(...) -> None:  # [Unsupported] Invalid Python identifier, not supported in script
	#     """ Touch Jump Start """
	#     pass

	# def Touch Jump End(...) -> None:  # [Unsupported] Invalid Python identifier, not supported in script
	#     """ Touch Jump End """
	#     pass

	def ExecuteUbergraph_BP_ShooterCharacter(self, EntryPoint: IntProperty, Temp_struct_Variable: StructProperty, CallFunc_GetPlatformName_ReturnValue: StrProperty, K2Node_SwitchString_CmpSuccess: BoolProperty, K2Node_Event_NewController: ObjectProperty, K2Node_DynamicCast_AsPlayer_Controller: ObjectProperty, K2Node_DynamicCast_bSuccess: BoolProperty, CallFunc_IsPlayerController_ReturnValue: BoolProperty, CallFunc_Create_ReturnValue: ObjectProperty, CallFunc_AddToPlayerScreen_ReturnValue: BoolProperty, CallFunc_GetLocalPlayerSubSystemFromPlayerController_ReturnValue: ObjectProperty, CallFunc_AddMappingContext_self_CastInput: InterfaceProperty, K2Node_Event_Axis_1: StructProperty, K2Node_Event_Axis: StructProperty, CallFunc_BreakVector2D_X: DoubleProperty, CallFunc_BreakVector2D_Y: DoubleProperty, CallFunc_BreakVector2D_X_1: DoubleProperty, CallFunc_BreakVector2D_Y_1: DoubleProperty, CallFunc_DoAim_Yaw_ImplicitCast: FloatProperty, CallFunc_DoAim_Pitch_ImplicitCast: FloatProperty, CallFunc_DoMove_Right_ImplicitCast: FloatProperty, CallFunc_DoMove_Forward_ImplicitCast: FloatProperty) -> None:
		""" Execute Ubergraph BP Shooter Character """  # noqa
		pass

	pass

class BP_ShooterGameMode_C(ShooterGameMode):
	""" BP Shooter Game Mode """  # noqa

	DefaultSceneRoot: ObjectProperty

	pass

class BP_ShooterNPC_C(ShooterNPC):
	""" BP Shooter NPC """  # noqa

	pass

class BP_ShooterPickup_C(ShooterPickup):
	""" BP Shooter Pickup """  # noqa

	UberGraphFrame: StructProperty

	BasePlate: ObjectProperty

	Respawn_Scale_94C209A34400322B4C1CF483ECFA627A: FloatProperty

	Respawn__Direction_94C209A34400322B4C1CF483ECFA627A: ByteProperty

	Respawn: ObjectProperty

	# def Spin(self, invalid_param_1, CallFunc_GetGameTimeInSeconds_ReturnValue, CallFunc_Multiply_DoubleDouble_ReturnValue, CallFunc_Multiply_DoubleDouble_ReturnValue_1, CallFunc_MakeRotator_ReturnValue, CallFunc_Sin_ReturnValue, CallFunc_Multiply_DoubleDouble_ReturnValue_2, CallFunc_MakeVector_ReturnValue, CallFunc_MakeTransform_ReturnValue, CallFunc_K2_SetRelativeTransform_SweepHitResult, CallFunc_MakeRotator_Yaw_ImplicitCast) -> None:  # [Unsupported] Contains invalid parameter names, not supported in script
	#     """ Spin """
	#     # Invalid parameters: New Transform Scale
	#     pass

	def Respawn__FinishedFunc(self) -> None:
		""" Respawn  Finished Func """  # noqa
		pass

	def Respawn__UpdateFunc(self) -> None:
		""" Respawn  Update Func """  # noqa
		pass

	def ReceiveTick(self, DeltaSeconds: FloatProperty) -> None:
		""" Event called every frame, if ticking is enabled """  # noqa
		pass

	def BP_OnRespawn(self) -> None:
		""" Passes control to Blueprint to animate the pickup respawn. Should end by calling FinishRespawn """  # noqa
		pass

	def ExecuteUbergraph_BP_ShooterPickup(self, EntryPoint: IntProperty, CallFunc_Conv_DoubleToVector_ReturnValue: StructProperty, K2Node_Event_DeltaSeconds: FloatProperty, CallFunc_Conv_DoubleToVector_InDouble_ImplicitCast: DoubleProperty) -> None:
		""" Execute Ubergraph BP Shooter Pickup """  # noqa
		pass

	pass

class BP_ShooterPlayerController_C(ShooterPlayerController):
	""" BP Shooter Player Controller """  # noqa

	pass

class BP_ShooterProjectile_Bullet_C(ShooterProjectile):
	""" BP Shooter Projectile Bullet """  # noqa

	UberGraphFrame: StructProperty

	Mesh: ObjectProperty

	Wiggle_Rotation_BD4B6CE5466630322F258AB06AC136AD: FloatProperty

	Wiggle__Direction_BD4B6CE5466630322F258AB06AC136AD: ByteProperty

	Wiggle: ObjectProperty

	def Wiggle__FinishedFunc(self) -> None:
		""" Wiggle  Finished Func """  # noqa
		pass

	def Wiggle__UpdateFunc(self) -> None:
		""" Wiggle  Update Func """  # noqa
		pass

	def BP_OnProjectileHit(self, Hit: StructProperty) -> None:
		""" Passes control to Blueprint to implement any effects on hit """  # noqa
		pass

	def ExecuteUbergraph_BP_ShooterProjectile_Bullet(self, EntryPoint: IntProperty, K2Node_Event_Hit: StructProperty, CallFunc_BreakHitResult_bBlockingHit: BoolProperty, CallFunc_BreakHitResult_bInitialOverlap: BoolProperty, CallFunc_BreakHitResult_Time: FloatProperty, CallFunc_BreakHitResult_Distance: FloatProperty, CallFunc_BreakHitResult_Location: StructProperty, CallFunc_BreakHitResult_ImpactPoint: StructProperty, CallFunc_BreakHitResult_Normal: StructProperty, CallFunc_BreakHitResult_ImpactNormal: StructProperty, CallFunc_BreakHitResult_PhysMat: ObjectProperty, CallFunc_BreakHitResult_HitActor: ObjectProperty, CallFunc_BreakHitResult_HitComponent: ObjectProperty, CallFunc_BreakHitResult_HitBoneName: NameProperty, CallFunc_BreakHitResult_BoneName: NameProperty, CallFunc_BreakHitResult_HitItem: IntProperty, CallFunc_BreakHitResult_ElementIndex: IntProperty, CallFunc_BreakHitResult_FaceIndex: IntProperty, CallFunc_BreakHitResult_TraceStart: StructProperty, CallFunc_BreakHitResult_TraceEnd: StructProperty, CallFunc_IsValid_ReturnValue: BoolProperty, K2Node_SwitchEnum_CmpSuccess: BoolProperty, CallFunc_MakeRotator_ReturnValue: StructProperty, CallFunc_K2_AddRelativeRotation_SweepHitResult: StructProperty, CallFunc_Conv_DoubleToVector_ReturnValue: StructProperty, CallFunc_Multiply_VectorVector_ReturnValue: StructProperty, CallFunc_MakeRotFromX_ReturnValue: StructProperty, CallFunc_K2_SetActorLocationAndRotation_SweepHitResult: StructProperty, CallFunc_K2_SetActorLocationAndRotation_ReturnValue: BoolProperty) -> None:
		""" Execute Ubergraph BP Shooter Projectile Bullet """  # noqa
		pass

	pass

class BP_ShooterProjectile_Grenade_C(ShooterProjectile):
	""" BP Shooter Projectile Grenade """  # noqa

	UberGraphFrame: StructProperty

	Sphere: ObjectProperty

	def BP_OnProjectileHit(self, Hit: StructProperty) -> None:
		""" Passes control to Blueprint to implement any effects on hit """  # noqa
		pass

	def ExecuteUbergraph_BP_ShooterProjectile_Grenade(self, EntryPoint: IntProperty, K2Node_Event_Hit: StructProperty) -> None:
		""" Execute Ubergraph BP Shooter Projectile Grenade """  # noqa
		pass

	pass

class BP_ShooterWeaponBase_C(ShooterWeapon):
	""" BP Shooter Weapon Base """  # noqa

	pass

class BP_ShooterWeapon_GrenadeLauncher_C(BP_ShooterWeaponBase_C):
	""" BP Shooter Weapon Grenade Launcher """  # noqa

	pass

class BP_ShooterWeapon_Pistol_C(BP_ShooterWeaponBase_C):
	""" BP Shooter Weapon Pistol """  # noqa

	pass

class BP_ShooterWeapon_Rifle_C(BP_ShooterWeaponBase_C):
	""" BP Shooter Weapon Rifle """  # noqa

	pass

class BP_WobbleTarget_C(Actor):
	""" BP Wobble Target """  # noqa

	BasePlate: ObjectProperty

	PhysicsConstraint: ObjectProperty

	Dummy: ObjectProperty

	DefaultSceneRoot: ObjectProperty

	def UserConstructionScript(self) -> None:
		""" Construction script, the place to spawn components and do other setup.
		@note Name used in CreateBlueprint function
		"""  # noqa
		pass

	pass

class CR_Mannequin_Body_C(ControlRig):
	""" CR Mannequin Body """  # noqa

	# L Arm IK Mode: BoolProperty  # [Unsupported] Invalid Python identifier, not supported in script

	# R Arm IK Mode: BoolProperty  # [Unsupported] Invalid Python identifier, not supported in script

	# L Leg IK Mode : BoolProperty  # [Unsupported] Invalid Python identifier, not supported in script

	# R Leg IK Mode: BoolProperty  # [Unsupported] Invalid Python identifier, not supported in script

	# Spine IK Mode: BoolProperty  # [Unsupported] Invalid Python identifier, not supported in script

	# Neck IK Mode: BoolProperty  # [Unsupported] Invalid Python identifier, not supported in script

	# Spine Offsets: ArrayProperty  # [Unsupported] Invalid Python identifier, not supported in script

	# Spine Length: DoubleProperty  # [Unsupported] Invalid Python identifier, not supported in script

	# Neck Length: DoubleProperty  # [Unsupported] Invalid Python identifier, not supported in script

	# L Arm IK Align: BoolProperty  # [Unsupported] Invalid Python identifier, not supported in script

	# R Arm IK Align: BoolProperty  # [Unsupported] Invalid Python identifier, not supported in script

	pass

class CR_Mannequin_FootIK_C(ControlRig):
	""" CR Mannequin Foot IK """  # noqa

	ZOffset_L_Target: DoubleProperty

	ZOffset_R_Target: DoubleProperty

	ZOffset_L: DoubleProperty

	ZOffset_R: DoubleProperty

	ZOffset_Pelvis: DoubleProperty

	ShouldDoIKTrace: BoolProperty

	pass

class CR_Mannequin_Procedural_C(ControlRig):
	""" CR Mannequin Procedural """  # noqa

	pass

class CtrlRig_FPWarp_C(ControlRig):
	""" Ctrl Rig FPWarp """  # noqa

	pass

class Ctrl_HandAdjusment_C(ControlRig):
	""" Ctrl Hand Adjusment """  # noqa

	# Aim Point: StructProperty  # [Unsupported] Invalid Python identifier, not supported in script

	# IronSight Adjust: FloatProperty  # [Unsupported] Invalid Python identifier, not supported in script

	# Control Arms: BoolProperty  # [Unsupported] Invalid Python identifier, not supported in script

	GunRestLOffset_1: StructProperty

	Interpolate_Result: StructProperty

	SpringInterpQuaternionV2_Result: StructProperty

	pass

class Ctrl_HandAdjusment_Pistol_C(ControlRig):
	""" Ctrl Hand Adjusment Pistol """  # noqa

	# Aim Point: StructProperty  # [Unsupported] Invalid Python identifier, not supported in script

	# IronSight Adjust: FloatProperty  # [Unsupported] Invalid Python identifier, not supported in script

	# Control Arms: BoolProperty  # [Unsupported] Invalid Python identifier, not supported in script

	GunRestLOffset_1: StructProperty

	Interpolate_Result: StructProperty

	SpringInterpQuaternionV2_Result: StructProperty

	pass

class Default__AnimBlueprintGeneratedClass(Object):
	""" Default  Anim Blueprint Generated Class """  # noqa

	pass

class Default__BlueprintGeneratedClass(Object):
	""" Default  Blueprint Generated Class """  # noqa

	pass

class Default__ControlRigBlueprintGeneratedClass(Object):
	""" Default  Control Rig Blueprint Generated Class """  # noqa

	pass

class Default__MVVMInstancedViewModelGeneratedClass(Object):
	""" Default  MVVMInstanced View Model Generated Class """  # noqa

	pass

class Default__MVVMViewModelBlueprintGeneratedClass(Object):
	""" Default  MVVMView Model Blueprint Generated Class """  # noqa

	pass

class Default__RigVMBlueprintGeneratedClass(Object):
	""" Default  Rig VMBlueprint Generated Class """  # noqa

	pass

class Default__WidgetBlueprintGeneratedClass(Object):
	""" Default  Widget Blueprint Generated Class """  # noqa

	pass

class DmgTypeBP_Environmental_C(DamageType):
	""" Dmg Type BP Environmental """  # noqa

	pass

class Light_C(Actor):
	""" Light """  # noqa

	NS_DustMote: ObjectProperty

	SpotLight: ObjectProperty

	Cylinder1: ObjectProperty

	Scene: ObjectProperty

	Cylinder: ObjectProperty

	Seed: StructProperty

	RandomIntensity: FloatProperty

	# Use Light Flicker: BoolProperty  # [Unsupported] Invalid Python identifier, not supported in script

	# New Light Color: StructProperty  # [Unsupported] Invalid Python identifier, not supported in script

	def UserConstructionScript(self, Temp_bool_Variable: BoolProperty, Temp_object_Variable: ObjectProperty, Temp_object_Variable_1: ObjectProperty, Temp_bool_Variable_1: BoolProperty, Temp_object_Variable_2: ObjectProperty, Temp_object_Variable_3: ObjectProperty, CallFunc_MakeVector4_ReturnValue: StructProperty, K2Node_Select_Default: ObjectProperty, K2Node_Select_Default_1: ObjectProperty, CallFunc_RandomFloatInRangeFromStream_ReturnValue: FloatProperty, CallFunc_MakeVector4_X_ImplicitCast: DoubleProperty, CallFunc_MakeVector4_Y_ImplicitCast: DoubleProperty, CallFunc_MakeVector4_Z_ImplicitCast: DoubleProperty, CallFunc_MakeVector4_W_ImplicitCast: DoubleProperty) -> None:
		""" Construction script, the place to spawn components and do other setup.
		@note Name used in CreateBlueprint function
		"""  # noqa
		pass

	pass

class NewBlueprint2_C(TestCharacter):
	""" New Blueprint 2 """  # noqa

	DefaultSceneRoot: ObjectProperty

	def BpOperatorArray(self, NewParam: ArrayProperty) -> None:
		""" Bp Operator Array """  # noqa
		pass

	def UserConstructionScript(self, NewArray: ArrayProperty) -> None:
		""" Construction script, the place to spawn components and do other setup.
		@note Name used in CreateBlueprint function
		"""  # noqa
		pass

	pass

class NewBlueprint_C(TestActor):
	""" New Blueprint """  # noqa

	UberGraphFrame: StructProperty

	BpArrayVar: ArrayProperty

	def BpTestFunc(self, NewParam: ArrayProperty) -> None:
		""" Bp Test Func """  # noqa
		pass

	def UserConstructionScript(self, NewInt64: Int64Property, NewBool: BoolProperty, ArrayVar: ArrayProperty) -> None:
		""" Construction script, the place to spawn components and do other setup.
		@note Name used in CreateBlueprint function
		"""  # noqa
		pass

	def ReceiveTick(self, DeltaSeconds: FloatProperty) -> None:
		""" Event called every frame, if ticking is enabled """  # noqa
		pass

	def ExecuteUbergraph_NewBlueprint(self, EntryPoint: IntProperty, K2Node_Event_DeltaSeconds: FloatProperty) -> None:
		""" Execute Ubergraph New Blueprint """  # noqa
		pass

	pass

class SKEL_ABP_FP_Copy_C(AnimInstance):
	""" ABP FP Copy """  # noqa

	UberGraphFrame: StructProperty

	def BlueprintUpdateAnimation(self, DeltaTimeX: FloatProperty) -> None:
		""" Executed when the Animation is updated """  # noqa
		pass

	def AnimGraph(self, AnimGraph: StructProperty) -> None:
		""" Anim Graph """  # noqa
		pass

	pass

class SKEL_ABP_FP_Pistol_C(AnimInstance):
	""" ABP FP Pistol """  # noqa

	UberGraphFrame: StructProperty

	__CustomProperty_Ctrl_Head_DAEEF21A4B8AD684557275826D426F26: StructProperty

	__CustomProperty_Aim_DAEEF21A4B8AD684557275826D426F26: StructProperty

	IsMoving: BoolProperty

	bIsInAir: BoolProperty

	HasRifle: BoolProperty

	FirstPersonCharacter: ObjectProperty

	# First Person Camera: ObjectProperty  # [Unsupported] Invalid Python identifier, not supported in script

	Hand_R_Target: StructProperty

	# As BP FPShooter: ObjectProperty  # [Unsupported] Invalid Python identifier, not supported in script

	# First Person Mesh: ObjectProperty  # [Unsupported] Invalid Python identifier, not supported in script

	# Aim Target: StructProperty  # [Unsupported] Invalid Python identifier, not supported in script

	# Is Aiming: BoolProperty  # [Unsupported] Invalid Python identifier, not supported in script

	NewVar: StructProperty

	PitchN: DoubleProperty

	NewVar_0: StructProperty

	GripOffset: StructProperty

	def BlueprintUpdateAnimation(self, DeltaTimeX: FloatProperty) -> None:
		""" Executed when the Animation is updated """  # noqa
		pass

	def BlueprintBeginPlay(self) -> None:
		""" Executed when begin play is called on the owning component """  # noqa
		pass

	def AnimGraph(self, AnimGraph: StructProperty) -> None:
		""" Anim Graph """  # noqa
		pass

	pass

class SKEL_ABP_FP_Weapon_C(AnimInstance):
	""" ABP FP Weapon """  # noqa

	UberGraphFrame: StructProperty

	__CustomProperty_Ctrl_Head_3DC6268C49E5FBCF1CD84391DB96A114: StructProperty

	__CustomProperty_Aim_3DC6268C49E5FBCF1CD84391DB96A114: StructProperty

	IsMoving: BoolProperty

	bIsInAir: BoolProperty

	HasRifle: BoolProperty

	FirstPersonCharacter: ObjectProperty

	# First Person Camera: ObjectProperty  # [Unsupported] Invalid Python identifier, not supported in script

	Hand_R_Target: StructProperty

	# As BP FPShooter: ObjectProperty  # [Unsupported] Invalid Python identifier, not supported in script

	# First Person Mesh: ObjectProperty  # [Unsupported] Invalid Python identifier, not supported in script

	# Aim Target: StructProperty  # [Unsupported] Invalid Python identifier, not supported in script

	# Is Aiming: BoolProperty  # [Unsupported] Invalid Python identifier, not supported in script

	NewVar: StructProperty

	PitchN: DoubleProperty

	NewVar_0: StructProperty

	GripOffset: StructProperty

	def BlueprintUpdateAnimation(self, DeltaTimeX: FloatProperty) -> None:
		""" Executed when the Animation is updated """  # noqa
		pass

	def BlueprintBeginPlay(self) -> None:
		""" Executed when begin play is called on the owning component """  # noqa
		pass

	def AnimGraph(self, AnimGraph: StructProperty) -> None:
		""" Anim Graph """  # noqa
		pass

	pass

class SKEL_ABP_TP_Pistol_C(AnimInstance):
	""" ABP TP Pistol """  # noqa

	UberGraphFrame: StructProperty

	bIsInAir: BoolProperty

	IsMoving: BoolProperty

	NewVar: StructProperty

	NewVar_0: StructProperty

	PitchN: DoubleProperty

	def BlueprintUpdateAnimation(self, DeltaTimeX: FloatProperty) -> None:
		""" Executed when the Animation is updated """  # noqa
		pass

	def AnimGraph(self, AnimGraph: StructProperty) -> None:
		""" Anim Graph """  # noqa
		pass

	pass

class SKEL_ABP_TP_Rifle_C(AnimInstance):
	""" ABP TP Rifle """  # noqa

	UberGraphFrame: StructProperty

	bIsInAir: BoolProperty

	IsMoving: BoolProperty

	NewVar: StructProperty

	NewVar_0: StructProperty

	PitchN: DoubleProperty

	def BlueprintUpdateAnimation(self, DeltaTimeX: FloatProperty) -> None:
		""" Executed when the Animation is updated """  # noqa
		pass

	def AnimGraph(self, AnimGraph: StructProperty) -> None:
		""" Anim Graph """  # noqa
		pass

	pass

class SKEL_ABP_Unarmed_C(AnimInstance):
	""" ABP Unarmed """  # noqa

	UberGraphFrame: StructProperty

	__CustomProperty_ShouldDoIKTrace_BDE61FFC4E653CE138E8E8BB38AA415C: BoolProperty

	Character: ObjectProperty

	MovementComponent: ObjectProperty

	Velocity: StructProperty

	GroundSpeed: DoubleProperty

	ShouldMove: BoolProperty

	IsFalling: BoolProperty

	Strafe: DoubleProperty

	# Running Speed: DoubleProperty  # [Unsupported] Invalid Python identifier, not supported in script

	# Can Strafe: BoolProperty  # [Unsupported] Invalid Python identifier, not supported in script

	def BlueprintUpdateAnimation(self, DeltaTimeX: FloatProperty) -> None:
		""" Executed when the Animation is updated """  # noqa
		pass

	def BlueprintInitializeAnimation(self) -> None:
		""" Executed when the Animation is initialized """  # noqa
		pass

	def AnimGraph(self, AnimGraph: StructProperty) -> None:
		""" Anim Graph """  # noqa
		pass

	pass

class SKEL_BPI_TouchInterface_C(Interface):
	""" BPI Touch Interface """  # noqa

	# def Primary Thumbstick(...) -> None:  # [Unsupported] Invalid Python identifier, not supported in script
	#     """ Primary Thumbstick """
	#     pass

	# def Secondary Thumbstick(...) -> None:  # [Unsupported] Invalid Python identifier, not supported in script
	#     """ Secondary Thumbstick """
	#     pass

	# def Touch Jump Start(...) -> None:  # [Unsupported] Invalid Python identifier, not supported in script
	#     """ Touch Jump Start """
	#     pass

	# def Touch Jump End(...) -> None:  # [Unsupported] Invalid Python identifier, not supported in script
	#     """ Touch Jump End """
	#     pass

	pass

class SKEL_BPI_Touch_Shooter_C(Interface):
	""" BPI Touch Shooter """  # noqa

	# def Touch Shoot Start(...) -> None:  # [Unsupported] Invalid Python identifier, not supported in script
	#     """ Touch Shoot Start """
	#     pass

	# def Touch Shoot End(...) -> None:  # [Unsupported] Invalid Python identifier, not supported in script
	#     """ Touch Shoot End """
	#     pass

	# def Touch Switch Weapon(...) -> None:  # [Unsupported] Invalid Python identifier, not supported in script
	#     """ Touch Switch Weapon """
	#     pass

	pass

class SKEL_BP_DoorFrame_C(Actor):
	""" BP Door Frame """  # noqa

	UberGraphFrame: StructProperty

	Box: ObjectProperty

	DefaultSceneRoot: ObjectProperty

	Door_Control_NewTrack_0_9027BDA34FB8CF4858BE4191F8EB344A: FloatProperty

	Door_Control__Direction_9027BDA34FB8CF4858BE4191F8EB344A: ByteProperty

	# Door Control: ObjectProperty  # [Unsupported] Invalid Python identifier, not supported in script

	# Door Size: StructProperty  # [Unsupported] Invalid Python identifier, not supported in script

	# Door Frame: BoolProperty  # [Unsupported] Invalid Python identifier, not supported in script

	# Door Frame Scale: DoubleProperty  # [Unsupported] Invalid Python identifier, not supported in script

	Door: ObjectProperty

	# Closed Position: StructProperty  # [Unsupported] Invalid Python identifier, not supported in script

	# Door Detection Adjust: StructProperty  # [Unsupported] Invalid Python identifier, not supported in script

	# Door Thickness: DoubleProperty  # [Unsupported] Invalid Python identifier, not supported in script

	# Split Door: BoolProperty  # [Unsupported] Invalid Python identifier, not supported in script

	# Door Frame Overide Material: ObjectProperty  # [Unsupported] Invalid Python identifier, not supported in script

	# Add Door: BoolProperty  # [Unsupported] Invalid Python identifier, not supported in script

	Door2: ObjectProperty

	# Closed Position_0: StructProperty  # [Unsupported] Invalid Python identifier, not supported in script

	def ReceiveBeginPlay(self) -> None:
		""" Event when play begins for this actor. """  # noqa
		pass

	def ReceiveActorBeginOverlap(self, OtherActor: ObjectProperty) -> None:
		""" Event when this actor overlaps another actor, for example a player walking into a trigger.
		For events when objects have a blocking collision, for example a player hitting a wall, see 'Hit' events.
		@note Components on both this and the other Actor must have bGenerateOverlapEvents set to true to generate overlap events.
		"""  # noqa
		pass

	def ReceiveActorEndOverlap(self, OtherActor: ObjectProperty) -> None:
		""" Event when an actor no longer overlaps another actor, and they have separated.
		@note Components on both this and the other Actor must have bGenerateOverlapEvents set to true to generate overlap events.
		"""  # noqa
		pass

	# def Door Control__UpdateFunc(...) -> None:  # [Unsupported] Invalid Python identifier, not supported in script
	#     """ Door Control  Update Func """
	#     pass

	# def Door Control__FinishedFunc(...) -> None:  # [Unsupported] Invalid Python identifier, not supported in script
	#     """ Door Control  Finished Func """
	#     pass

	def UserConstructionScript(self) -> None:
		""" Construction script, the place to spawn components and do other setup.
		@note Name used in CreateBlueprint function
		"""  # noqa
		pass

	# def Set Mesh(...) -> None:  # [Unsupported] Invalid Python identifier, not supported in script
	#     """ Set Mesh """
	#     pass

	pass

class SKEL_BP_FirstPersonCharacter_C(UE5_6_ProjectCharacter):
	""" BP First Person Character """  # noqa

	UberGraphFrame: StructProperty

	# Target Touch UI: ClassProperty  # [Unsupported] Invalid Python identifier, not supported in script

	def ReceivePossessed(self, NewController: ObjectProperty) -> None:
		""" Event called when the Pawn is possessed by a Controller. Only called on the server (or in standalone) """  # noqa
		pass

	# def Secondary Thumbstick(...) -> None:  # [Unsupported] Invalid Python identifier, not supported in script
	#     """ Secondary Thumbstick """
	#     pass

	# def Primary Thumbstick(...) -> None:  # [Unsupported] Invalid Python identifier, not supported in script
	#     """ Primary Thumbstick """
	#     pass

	# def Touch Jump Start(...) -> None:  # [Unsupported] Invalid Python identifier, not supported in script
	#     """ Touch Jump Start """
	#     pass

	# def Touch Jump End(...) -> None:  # [Unsupported] Invalid Python identifier, not supported in script
	#     """ Touch Jump End """
	#     pass

	def UserConstructionScript(self) -> None:
		""" Construction script, the place to spawn components and do other setup.
		@note Name used in CreateBlueprint function
		"""  # noqa
		pass

	pass

class SKEL_BP_FirstPersonGameMode_C(UE5_6_ProjectGameMode):
	""" BP First Person Game Mode """  # noqa

	UberGraphFrame: StructProperty

	DefaultSceneRoot: ObjectProperty

	def ReceiveBeginPlay(self) -> None:
		""" Event when play begins for this actor. """  # noqa
		pass

	def ReceiveTick(self, DeltaSeconds: FloatProperty) -> None:
		""" Event called every frame, if ticking is enabled """  # noqa
		pass

	def UserConstructionScript(self) -> None:
		""" Construction script, the place to spawn components and do other setup.
		@note Name used in CreateBlueprint function
		"""  # noqa
		pass

	pass

class SKEL_BP_FirstPersonPlayerController_C(UE5_6_ProjectPlayerController):
	""" BP First Person Player Controller """  # noqa

	UberGraphFrame: StructProperty

	def ReceiveBeginPlay(self) -> None:
		""" Event when play begins for this actor. """  # noqa
		pass

	def ReceiveTick(self, DeltaSeconds: FloatProperty) -> None:
		""" Event called every frame, if ticking is enabled """  # noqa
		pass

	def UserConstructionScript(self) -> None:
		""" Construction script, the place to spawn components and do other setup.
		@note Name used in CreateBlueprint function
		"""  # noqa
		pass

	pass

class SKEL_BP_HorrorCharacter_C(HorrorCharacter):
	""" BP Horror Character """  # noqa

	UberGraphFrame: StructProperty

	# Target Touch UI: ClassProperty  # [Unsupported] Invalid Python identifier, not supported in script

	def ReceivePossessed(self, NewController: ObjectProperty) -> None:
		""" Event called when the Pawn is possessed by a Controller. Only called on the server (or in standalone) """  # noqa
		pass

	# def Secondary Thumbstick(...) -> None:  # [Unsupported] Invalid Python identifier, not supported in script
	#     """ Secondary Thumbstick """
	#     pass

	# def Primary Thumbstick(...) -> None:  # [Unsupported] Invalid Python identifier, not supported in script
	#     """ Primary Thumbstick """
	#     pass

	# def Touch Jump Start(...) -> None:  # [Unsupported] Invalid Python identifier, not supported in script
	#     """ Touch Jump Start """
	#     pass

	# def Touch Jump End(...) -> None:  # [Unsupported] Invalid Python identifier, not supported in script
	#     """ Touch Jump End """
	#     pass

	def UserConstructionScript(self) -> None:
		""" Construction script, the place to spawn components and do other setup.
		@note Name used in CreateBlueprint function
		"""  # noqa
		pass

	pass

class SKEL_BP_HorrorGameMode_C(HorrorGameMode):
	""" BP Horror Game Mode """  # noqa

	UberGraphFrame: StructProperty

	DefaultSceneRoot: ObjectProperty

	def ReceiveBeginPlay(self) -> None:
		""" Event when play begins for this actor. """  # noqa
		pass

	def ReceiveTick(self, DeltaSeconds: FloatProperty) -> None:
		""" Event called every frame, if ticking is enabled """  # noqa
		pass

	def UserConstructionScript(self) -> None:
		""" Construction script, the place to spawn components and do other setup.
		@note Name used in CreateBlueprint function
		"""  # noqa
		pass

	pass

class SKEL_BP_HorrorPlayerController_C(HorrorPlayerController):
	""" BP Horror Player Controller """  # noqa

	UberGraphFrame: StructProperty

	def ReceiveBeginPlay(self) -> None:
		""" Event when play begins for this actor. """  # noqa
		pass

	def ReceiveTick(self, DeltaSeconds: FloatProperty) -> None:
		""" Event called every frame, if ticking is enabled """  # noqa
		pass

	def UserConstructionScript(self) -> None:
		""" Construction script, the place to spawn components and do other setup.
		@note Name used in CreateBlueprint function
		"""  # noqa
		pass

	pass

class SKEL_BP_JumpPad_C(Actor):
	""" BP Jump Pad """  # noqa

	UberGraphFrame: StructProperty

	Cylinder1: ObjectProperty

	PointLight: ObjectProperty

	NS_JumpDemo: ObjectProperty

	Sphere: ObjectProperty

	Cylinder: ObjectProperty

	DefaultSceneRoot: ObjectProperty

	Velocity: StructProperty

	# Color Target: StructProperty  # [Unsupported] Invalid Python identifier, not supported in script

	def ReceiveActorBeginOverlap(self, OtherActor: ObjectProperty) -> None:
		""" Event when this actor overlaps another actor, for example a player walking into a trigger.
		For events when objects have a blocking collision, for example a player hitting a wall, see 'Hit' events.
		@note Components on both this and the other Actor must have bGenerateOverlapEvents set to true to generate overlap events.
		"""  # noqa
		pass

	def UserConstructionScript(self) -> None:
		""" Construction script, the place to spawn components and do other setup.
		@note Name used in CreateBlueprint function
		"""  # noqa
		pass

	pass

class SKEL_BP_ShooterAIController_C(ShooterAIController):
	""" BP Shooter AIController """  # noqa

	UberGraphFrame: StructProperty

	def ReceiveBeginPlay(self) -> None:
		""" Event when play begins for this actor. """  # noqa
		pass

	def ReceiveTick(self, DeltaSeconds: FloatProperty) -> None:
		""" Event called every frame, if ticking is enabled """  # noqa
		pass

	def UserConstructionScript(self) -> None:
		""" Construction script, the place to spawn components and do other setup.
		@note Name used in CreateBlueprint function
		"""  # noqa
		pass

	pass

class SKEL_BP_ShooterCharacter_C(ShooterCharacter):
	""" BP Shooter Character """  # noqa

	UberGraphFrame: StructProperty

	# Target Touch UI: ClassProperty  # [Unsupported] Invalid Python identifier, not supported in script

	def ReceivePossessed(self, NewController: ObjectProperty) -> None:
		""" Event called when the Pawn is possessed by a Controller. Only called on the server (or in standalone) """  # noqa
		pass

	# def Touch Switch Weapon(...) -> None:  # [Unsupported] Invalid Python identifier, not supported in script
	#     """ Touch Switch Weapon """
	#     pass

	# def Touch Shoot Start(...) -> None:  # [Unsupported] Invalid Python identifier, not supported in script
	#     """ Touch Shoot Start """
	#     pass

	# def Touch Shoot End(...) -> None:  # [Unsupported] Invalid Python identifier, not supported in script
	#     """ Touch Shoot End """
	#     pass

	# def Secondary Thumbstick(...) -> None:  # [Unsupported] Invalid Python identifier, not supported in script
	#     """ Secondary Thumbstick """
	#     pass

	# def Primary Thumbstick(...) -> None:  # [Unsupported] Invalid Python identifier, not supported in script
	#     """ Primary Thumbstick """
	#     pass

	# def Touch Jump Start(...) -> None:  # [Unsupported] Invalid Python identifier, not supported in script
	#     """ Touch Jump Start """
	#     pass

	# def Touch Jump End(...) -> None:  # [Unsupported] Invalid Python identifier, not supported in script
	#     """ Touch Jump End """
	#     pass

	def UserConstructionScript(self) -> None:
		""" Construction script, the place to spawn components and do other setup.
		@note Name used in CreateBlueprint function
		"""  # noqa
		pass

	pass

class SKEL_BP_ShooterGameMode_C(ShooterGameMode):
	""" BP Shooter Game Mode """  # noqa

	UberGraphFrame: StructProperty

	DefaultSceneRoot: ObjectProperty

	def ReceiveBeginPlay(self) -> None:
		""" Event when play begins for this actor. """  # noqa
		pass

	def ReceiveTick(self, DeltaSeconds: FloatProperty) -> None:
		""" Event called every frame, if ticking is enabled """  # noqa
		pass

	def UserConstructionScript(self) -> None:
		""" Construction script, the place to spawn components and do other setup.
		@note Name used in CreateBlueprint function
		"""  # noqa
		pass

	pass

class SKEL_BP_ShooterNPC_C(ShooterNPC):
	""" BP Shooter NPC """  # noqa

	UberGraphFrame: StructProperty

	def ReceiveBeginPlay(self) -> None:
		""" Event when play begins for this actor. """  # noqa
		pass

	def ReceiveActorBeginOverlap(self, OtherActor: ObjectProperty) -> None:
		""" Event when this actor overlaps another actor, for example a player walking into a trigger.
		For events when objects have a blocking collision, for example a player hitting a wall, see 'Hit' events.
		@note Components on both this and the other Actor must have bGenerateOverlapEvents set to true to generate overlap events.
		"""  # noqa
		pass

	def ReceiveTick(self, DeltaSeconds: FloatProperty) -> None:
		""" Event called every frame, if ticking is enabled """  # noqa
		pass

	def UserConstructionScript(self) -> None:
		""" Construction script, the place to spawn components and do other setup.
		@note Name used in CreateBlueprint function
		"""  # noqa
		pass

	pass

class SKEL_BP_ShooterPickup_C(ShooterPickup):
	""" BP Shooter Pickup """  # noqa

	UberGraphFrame: StructProperty

	BasePlate: ObjectProperty

	Respawn_Scale_94C209A34400322B4C1CF483ECFA627A: FloatProperty

	Respawn__Direction_94C209A34400322B4C1CF483ECFA627A: ByteProperty

	Respawn: ObjectProperty

	def ReceiveTick(self, DeltaSeconds: FloatProperty) -> None:
		""" Event called every frame, if ticking is enabled """  # noqa
		pass

	def BP_OnRespawn(self) -> None:
		""" Passes control to Blueprint to animate the pickup respawn. Should end by calling FinishRespawn """  # noqa
		pass

	def Respawn__UpdateFunc(self) -> None:
		""" Respawn  Update Func """  # noqa
		pass

	def Respawn__FinishedFunc(self) -> None:
		""" Respawn  Finished Func """  # noqa
		pass

	def UserConstructionScript(self) -> None:
		""" Construction script, the place to spawn components and do other setup.
		@note Name used in CreateBlueprint function
		"""  # noqa
		pass

	# def Spin(self, invalid_param_1) -> None:  # [Unsupported] Contains invalid parameter names, not supported in script
	#     """ Spin """
	#     # Invalid parameters: New Transform Scale
	#     pass

	pass

class SKEL_BP_ShooterPlayerController_C(ShooterPlayerController):
	""" BP Shooter Player Controller """  # noqa

	UberGraphFrame: StructProperty

	def ReceiveBeginPlay(self) -> None:
		""" Event when play begins for this actor. """  # noqa
		pass

	def ReceiveTick(self, DeltaSeconds: FloatProperty) -> None:
		""" Event called every frame, if ticking is enabled """  # noqa
		pass

	def UserConstructionScript(self) -> None:
		""" Construction script, the place to spawn components and do other setup.
		@note Name used in CreateBlueprint function
		"""  # noqa
		pass

	pass

class SKEL_BP_ShooterProjectile_Bullet_C(ShooterProjectile):
	""" BP Shooter Projectile Bullet """  # noqa

	UberGraphFrame: StructProperty

	Mesh: ObjectProperty

	Wiggle_Rotation_BD4B6CE5466630322F258AB06AC136AD: FloatProperty

	Wiggle__Direction_BD4B6CE5466630322F258AB06AC136AD: ByteProperty

	Wiggle: ObjectProperty

	def BP_OnProjectileHit(self, Hit: StructProperty) -> None:
		""" Passes control to Blueprint to implement any effects on hit """  # noqa
		pass

	def Wiggle__UpdateFunc(self) -> None:
		""" Wiggle  Update Func """  # noqa
		pass

	def Wiggle__FinishedFunc(self) -> None:
		""" Wiggle  Finished Func """  # noqa
		pass

	def UserConstructionScript(self) -> None:
		""" Construction script, the place to spawn components and do other setup.
		@note Name used in CreateBlueprint function
		"""  # noqa
		pass

	pass

class SKEL_BP_ShooterProjectile_Grenade_C(ShooterProjectile):
	""" BP Shooter Projectile Grenade """  # noqa

	UberGraphFrame: StructProperty

	Sphere: ObjectProperty

	def BP_OnProjectileHit(self, Hit: StructProperty) -> None:
		""" Passes control to Blueprint to implement any effects on hit """  # noqa
		pass

	def UserConstructionScript(self) -> None:
		""" Construction script, the place to spawn components and do other setup.
		@note Name used in CreateBlueprint function
		"""  # noqa
		pass

	pass

class SKEL_BP_ShooterWeaponBase_C(ShooterWeapon):
	""" BP Shooter Weapon Base """  # noqa

	UberGraphFrame: StructProperty

	def ReceiveBeginPlay(self) -> None:
		""" Event when play begins for this actor. """  # noqa
		pass

	def ReceiveActorBeginOverlap(self, OtherActor: ObjectProperty) -> None:
		""" Event when this actor overlaps another actor, for example a player walking into a trigger.
		For events when objects have a blocking collision, for example a player hitting a wall, see 'Hit' events.
		@note Components on both this and the other Actor must have bGenerateOverlapEvents set to true to generate overlap events.
		"""  # noqa
		pass

	def ReceiveTick(self, DeltaSeconds: FloatProperty) -> None:
		""" Event called every frame, if ticking is enabled """  # noqa
		pass

	def UserConstructionScript(self) -> None:
		""" Construction script, the place to spawn components and do other setup.
		@note Name used in CreateBlueprint function
		"""  # noqa
		pass

	pass

class SKEL_BP_ShooterWeapon_GrenadeLauncher_C(SKEL_BP_ShooterWeaponBase_C):
	""" BP Shooter Weapon Grenade Launcher """  # noqa

	UberGraphFrame: StructProperty

	def ReceiveBeginPlay(self) -> None:
		""" Event when play begins for this actor. """  # noqa
		pass

	def ReceiveActorBeginOverlap(self, OtherActor: ObjectProperty) -> None:
		""" Event when this actor overlaps another actor, for example a player walking into a trigger.
		For events when objects have a blocking collision, for example a player hitting a wall, see 'Hit' events.
		@note Components on both this and the other Actor must have bGenerateOverlapEvents set to true to generate overlap events.
		"""  # noqa
		pass

	def ReceiveTick(self, DeltaSeconds: FloatProperty) -> None:
		""" Event called every frame, if ticking is enabled """  # noqa
		pass

	def UserConstructionScript(self) -> None:
		""" Construction script, the place to spawn components and do other setup.
		@note Name used in CreateBlueprint function
		"""  # noqa
		pass

	pass

class SKEL_BP_ShooterWeapon_Pistol_C(SKEL_BP_ShooterWeaponBase_C):
	""" BP Shooter Weapon Pistol """  # noqa

	UberGraphFrame: StructProperty

	def ReceiveBeginPlay(self) -> None:
		""" Event when play begins for this actor. """  # noqa
		pass

	def ReceiveActorBeginOverlap(self, OtherActor: ObjectProperty) -> None:
		""" Event when this actor overlaps another actor, for example a player walking into a trigger.
		For events when objects have a blocking collision, for example a player hitting a wall, see 'Hit' events.
		@note Components on both this and the other Actor must have bGenerateOverlapEvents set to true to generate overlap events.
		"""  # noqa
		pass

	def ReceiveTick(self, DeltaSeconds: FloatProperty) -> None:
		""" Event called every frame, if ticking is enabled """  # noqa
		pass

	def UserConstructionScript(self) -> None:
		""" Construction script, the place to spawn components and do other setup.
		@note Name used in CreateBlueprint function
		"""  # noqa
		pass

	pass

class SKEL_BP_ShooterWeapon_Rifle_C(SKEL_BP_ShooterWeaponBase_C):
	""" BP Shooter Weapon Rifle """  # noqa

	UberGraphFrame: StructProperty

	def ReceiveBeginPlay(self) -> None:
		""" Event when play begins for this actor. """  # noqa
		pass

	def ReceiveActorBeginOverlap(self, OtherActor: ObjectProperty) -> None:
		""" Event when this actor overlaps another actor, for example a player walking into a trigger.
		For events when objects have a blocking collision, for example a player hitting a wall, see 'Hit' events.
		@note Components on both this and the other Actor must have bGenerateOverlapEvents set to true to generate overlap events.
		"""  # noqa
		pass

	def ReceiveTick(self, DeltaSeconds: FloatProperty) -> None:
		""" Event called every frame, if ticking is enabled """  # noqa
		pass

	def UserConstructionScript(self) -> None:
		""" Construction script, the place to spawn components and do other setup.
		@note Name used in CreateBlueprint function
		"""  # noqa
		pass

	pass

class SKEL_BP_WobbleTarget_C(Actor):
	""" BP Wobble Target """  # noqa

	UberGraphFrame: StructProperty

	BasePlate: ObjectProperty

	PhysicsConstraint: ObjectProperty

	Dummy: ObjectProperty

	DefaultSceneRoot: ObjectProperty

	def ReceiveActorBeginOverlap(self, OtherActor: ObjectProperty) -> None:
		""" Event when this actor overlaps another actor, for example a player walking into a trigger.
		For events when objects have a blocking collision, for example a player hitting a wall, see 'Hit' events.
		@note Components on both this and the other Actor must have bGenerateOverlapEvents set to true to generate overlap events.
		"""  # noqa
		pass

	def ReceiveTick(self, DeltaSeconds: FloatProperty) -> None:
		""" Event called every frame, if ticking is enabled """  # noqa
		pass

	def UserConstructionScript(self) -> None:
		""" Construction script, the place to spawn components and do other setup.
		@note Name used in CreateBlueprint function
		"""  # noqa
		pass

	pass

class SKEL_CR_Mannequin_Body_C(ControlRig):
	""" CR Mannequin Body """  # noqa

	UberGraphFrame: StructProperty

	# L Arm IK Mode: BoolProperty  # [Unsupported] Invalid Python identifier, not supported in script

	# R Arm IK Mode: BoolProperty  # [Unsupported] Invalid Python identifier, not supported in script

	# L Leg IK Mode : BoolProperty  # [Unsupported] Invalid Python identifier, not supported in script

	# R Leg IK Mode: BoolProperty  # [Unsupported] Invalid Python identifier, not supported in script

	# Spine IK Mode: BoolProperty  # [Unsupported] Invalid Python identifier, not supported in script

	# Neck IK Mode: BoolProperty  # [Unsupported] Invalid Python identifier, not supported in script

	# Spine Offsets: ArrayProperty  # [Unsupported] Invalid Python identifier, not supported in script

	# Spine Length: DoubleProperty  # [Unsupported] Invalid Python identifier, not supported in script

	# Neck Length: DoubleProperty  # [Unsupported] Invalid Python identifier, not supported in script

	# L Arm IK Align: BoolProperty  # [Unsupported] Invalid Python identifier, not supported in script

	# R Arm IK Align: BoolProperty  # [Unsupported] Invalid Python identifier, not supported in script

	pass

class SKEL_CR_Mannequin_FootIK_C(ControlRig):
	""" CR Mannequin Foot IK """  # noqa

	UberGraphFrame: StructProperty

	ZOffset_L_Target: DoubleProperty

	ZOffset_R_Target: DoubleProperty

	ZOffset_L: DoubleProperty

	ZOffset_R: DoubleProperty

	ZOffset_Pelvis: DoubleProperty

	ShouldDoIKTrace: BoolProperty

	pass

class SKEL_CR_Mannequin_Procedural_C(ControlRig):
	""" CR Mannequin Procedural """  # noqa

	UberGraphFrame: StructProperty

	pass

class SKEL_CtrlRig_FPWarp_C(ControlRig):
	""" Ctrl Rig FPWarp """  # noqa

	UberGraphFrame: StructProperty

	pass

class SKEL_Ctrl_HandAdjusment_C(ControlRig):
	""" Ctrl Hand Adjusment """  # noqa

	UberGraphFrame: StructProperty

	# Aim Point: StructProperty  # [Unsupported] Invalid Python identifier, not supported in script

	# IronSight Adjust: FloatProperty  # [Unsupported] Invalid Python identifier, not supported in script

	# Control Arms: BoolProperty  # [Unsupported] Invalid Python identifier, not supported in script

	GunRestLOffset_1: StructProperty

	Interpolate_Result: StructProperty

	SpringInterpQuaternionV2_Result: StructProperty

	pass

class SKEL_Ctrl_HandAdjusment_Pistol_C(ControlRig):
	""" Ctrl Hand Adjusment Pistol """  # noqa

	UberGraphFrame: StructProperty

	# Aim Point: StructProperty  # [Unsupported] Invalid Python identifier, not supported in script

	# IronSight Adjust: FloatProperty  # [Unsupported] Invalid Python identifier, not supported in script

	# Control Arms: BoolProperty  # [Unsupported] Invalid Python identifier, not supported in script

	GunRestLOffset_1: StructProperty

	Interpolate_Result: StructProperty

	SpringInterpQuaternionV2_Result: StructProperty

	pass

class SKEL_DmgTypeBP_Environmental_C(DamageType):
	""" Dmg Type BP Environmental """  # noqa

	UberGraphFrame: StructProperty

	pass

class SKEL_Light_C(Actor):
	""" Light """  # noqa

	UberGraphFrame: StructProperty

	NS_DustMote: ObjectProperty

	SpotLight: ObjectProperty

	Cylinder1: ObjectProperty

	Scene: ObjectProperty

	Cylinder: ObjectProperty

	Seed: StructProperty

	RandomIntensity: FloatProperty

	# Use Light Flicker: BoolProperty  # [Unsupported] Invalid Python identifier, not supported in script

	# New Light Color: StructProperty  # [Unsupported] Invalid Python identifier, not supported in script

	def ReceiveBeginPlay(self) -> None:
		""" Event when play begins for this actor. """  # noqa
		pass

	def ReceiveActorBeginOverlap(self, OtherActor: ObjectProperty) -> None:
		""" Event when this actor overlaps another actor, for example a player walking into a trigger.
		For events when objects have a blocking collision, for example a player hitting a wall, see 'Hit' events.
		@note Components on both this and the other Actor must have bGenerateOverlapEvents set to true to generate overlap events.
		"""  # noqa
		pass

	def ReceiveTick(self, DeltaSeconds: FloatProperty) -> None:
		""" Event called every frame, if ticking is enabled """  # noqa
		pass

	def UserConstructionScript(self) -> None:
		""" Construction script, the place to spawn components and do other setup.
		@note Name used in CreateBlueprint function
		"""  # noqa
		pass

	pass

class SKEL_NewBlueprint2_C(TestCharacter):
	""" New Blueprint 2 """  # noqa

	UberGraphFrame: StructProperty

	DefaultSceneRoot: ObjectProperty

	def ReceiveBeginPlay(self) -> None:
		""" Event when play begins for this actor. """  # noqa
		pass

	def ReceiveActorBeginOverlap(self, OtherActor: ObjectProperty) -> None:
		""" Event when this actor overlaps another actor, for example a player walking into a trigger.
		For events when objects have a blocking collision, for example a player hitting a wall, see 'Hit' events.
		@note Components on both this and the other Actor must have bGenerateOverlapEvents set to true to generate overlap events.
		"""  # noqa
		pass

	def ReceiveTick(self, DeltaSeconds: FloatProperty) -> None:
		""" Event called every frame, if ticking is enabled """  # noqa
		pass

	def UserConstructionScript(self, NewArray: ArrayProperty) -> None:
		""" Construction script, the place to spawn components and do other setup.
		@note Name used in CreateBlueprint function
		"""  # noqa
		pass

	def BpOperatorArray(self, NewParam: ArrayProperty) -> None:
		""" Bp Operator Array """  # noqa
		pass

	pass

class SKEL_NewBlueprint_C(TestActor):
	""" New Blueprint """  # noqa

	UberGraphFrame: StructProperty

	BpArrayVar: ArrayProperty

	def ReceiveBeginPlay(self) -> None:
		""" Event when play begins for this actor. """  # noqa
		pass

	def ReceiveActorBeginOverlap(self, OtherActor: ObjectProperty) -> None:
		""" Event when this actor overlaps another actor, for example a player walking into a trigger.
		For events when objects have a blocking collision, for example a player hitting a wall, see 'Hit' events.
		@note Components on both this and the other Actor must have bGenerateOverlapEvents set to true to generate overlap events.
		"""  # noqa
		pass

	def ReceiveTick(self, DeltaSeconds: FloatProperty) -> None:
		""" Event called every frame, if ticking is enabled """  # noqa
		pass

	def UserConstructionScript(self, ArrayVar: ArrayProperty, NewBool: BoolProperty, NewInt64: Int64Property) -> None:
		""" Construction script, the place to spawn components and do other setup.
		@note Name used in CreateBlueprint function
		"""  # noqa
		pass

	def BpTestFunc(self, NewParam: ArrayProperty) -> None:
		""" Bp Test Func """  # noqa
		pass

	pass

class SKEL_StandardMacros_C(Object):
	""" Standard Macros """  # noqa

	pass

class SKEL_UI_Horror_C(HorrorUI):
	""" UI Horror """  # noqa

	UberGraphFrame: StructProperty

	SafeZone_0: ObjectProperty

	ProgressBar_21: ObjectProperty

	Drain: ObjectProperty

	def BP_SprintMeterUpdated(self, Percent: FloatProperty) -> None:
		""" Passes control to Blueprint to update the sprint meter widgets """  # noqa
		pass

	def BP_SprintStateChanged(self, bSprinting: BoolProperty) -> None:
		""" Passes control to Blueprint to update the sprint meter status """  # noqa
		pass

	pass

class SKEL_UI_ShooterBulletCounter_C(ShooterBulletCounterUI):
	""" UI Shooter Bullet Counter """  # noqa

	UberGraphFrame: StructProperty

	Image_33: ObjectProperty

	DM_BulletCount: ObjectProperty

	# Mag Size: IntProperty  # [Unsupported] Invalid Python identifier, not supported in script

	# Bullet Count: IntProperty  # [Unsupported] Invalid Python identifier, not supported in script

	# Bullet Thickness: DoubleProperty  # [Unsupported] Invalid Python identifier, not supported in script

	def PreConstruct(self, IsDesignTime: BoolProperty) -> None:
		""" Called by both the game and the editor.  Allows users to run initial setup for their widgets to better preview
		the setup in the designer and since generally that same setup code is required at runtime, it's called there
		as well.

		**WARNING**
		This is intended purely for cosmetic updates using locally owned data, you can not safely access any game related
		state, if you call something that doesn't expect to be run at editor time, you may crash the editor.

		In the event you save the asset with blueprint code that causes a crash on evaluation.  You can turn off
		PreConstruct evaluation in the Widget Designer settings in the Editor Preferences.
		"""  # noqa
		pass

	def BP_UpdateBulletCounter(self, MagazineSize: IntProperty, BulletCount: IntProperty) -> None:
		""" Allows Blueprint to update sub-widgets with the new bullet count """  # noqa
		pass

	pass

class SKEL_UI_Shooter_C(ShooterUI):
	""" UI Shooter """  # noqa

	UberGraphFrame: StructProperty

	Team2Score: ObjectProperty

	Team1Score: ObjectProperty

	Image_58: ObjectProperty

	Image_57: ObjectProperty

	def BP_UpdateScore(self, TeamByte: ByteProperty, Score: IntProperty) -> None:
		""" Allows Blueprint to update score sub-widgets """  # noqa
		pass

	pass

class SKEL_UI_Thumbstick_C(UserWidget):
	""" UI Thumbstick """  # noqa

	UberGraphFrame: StructProperty

	Stick: ObjectProperty

	Container: ObjectProperty

	TrackInput: BoolProperty

	# Initial Touch Location: StructProperty  # [Unsupported] Invalid Python identifier, not supported in script

	Position: StructProperty

	# Stick Input: MulticastInlineDelegateProperty  # [Unsupported] Invalid Python identifier, not supported in script

	# Thumbstick Size: StructProperty  # [Unsupported] Invalid Python identifier, not supported in script

	# Invert X: BoolProperty  # [Unsupported] Invalid Python identifier, not supported in script

	# Invert Y: BoolProperty  # [Unsupported] Invalid Python identifier, not supported in script

	# Scale Output: DoubleProperty  # [Unsupported] Invalid Python identifier, not supported in script

	# Touch Target: ByteProperty  # [Unsupported] Invalid Python identifier, not supported in script

	# def Stick Input__DelegateSignature(...) -> None:  # [Unsupported] Invalid Python identifier, not supported in script
	#     """ Stick Input  Delegate Signature """
	#     pass

	def Tick(self, MyGeometry: StructProperty, InDeltaTime: FloatProperty) -> None:
		""" Ticks this widget.  Override in derived classes, but always call the parent implementation.

		@param  MyGeometry The space allotted for this widget
		@param  InDeltaTime  Real time passed since last tick
		"""  # noqa
		pass

	def PreConstruct(self, IsDesignTime: BoolProperty) -> None:
		""" Called by both the game and the editor.  Allows users to run initial setup for their widgets to better preview
		the setup in the designer and since generally that same setup code is required at runtime, it's called there
		as well.

		**WARNING**
		This is intended purely for cosmetic updates using locally owned data, you can not safely access any game related
		state, if you call something that doesn't expect to be run at editor time, you may crash the editor.

		In the event you save the asset with blueprint code that causes a crash on evaluation.  You can turn off
		PreConstruct evaluation in the Widget Designer settings in the Editor Preferences.
		"""  # noqa
		pass

	# def Touch Input Check(...) -> None:  # [Unsupported] Invalid Python identifier, not supported in script
	#     """ Touch Input Check """
	#     pass

	pass

class SKEL_UI_TouchInterface_Shooter_C(UserWidget):
	""" UI Touch Interface Shooter """  # noqa

	UberGraphFrame: StructProperty

	Thumbstick_Move: ObjectProperty

	Thumbstick_Aim: ObjectProperty

	Btn_SwitchWeapon: ObjectProperty

	Btn_Jump: ObjectProperty

	Btn_FireWeapon: ObjectProperty

	# In String: StrProperty  # [Unsupported] Invalid Python identifier, not supported in script

	def Construct(self) -> None:
		""" Called after the underlying slate widget is constructed.  Depending on how the slate object is used
		this event may be called multiple times due to adding and removing from the hierarchy.
		If you need a true called-once-when-created event, use OnInitialized.
		"""  # noqa
		pass

	def BndEvt__UI_MobileOverlay_Button_1_K2Node_ComponentBoundEvent_1_OnButtonPressedEvent__DelegateSignature(self) -> None:
		""" Bnd Evt  UI Mobile Overlay Button 1 K2Node Component Bound Event 1 on Button Pressed Event  Delegate Signature """  # noqa
		pass

	def BndEvt__UI_MobileOverlay_Btn_Jump_K2Node_ComponentBoundEvent_3_OnButtonPressedEvent__DelegateSignature(self) -> None:
		""" Bnd Evt  UI Mobile Overlay Btn Jump K2Node Component Bound Event 3 on Button Pressed Event  Delegate Signature """  # noqa
		pass

	def BndEvt__UI_MobileOverlay_Btn_FireWeapon_K2Node_ComponentBoundEvent_4_OnButtonPressedEvent__DelegateSignature(self) -> None:
		""" Bnd Evt  UI Mobile Overlay Btn Fire Weapon K2Node Component Bound Event 4 on Button Pressed Event  Delegate Signature """  # noqa
		pass

	def BndEvt__UI_MobileOverlay_Btn_FireWeapon_K2Node_ComponentBoundEvent_5_OnButtonReleasedEvent__DelegateSignature(self) -> None:
		""" Bnd Evt  UI Mobile Overlay Btn Fire Weapon K2Node Component Bound Event 5 on Button Released Event  Delegate Signature """  # noqa
		pass

	# def BndEvt__UI_MobileOverlay_UI_Thumbstick_K2Node_ComponentBoundEvent_0_Stick Input__DelegateSignature(...) -> None:  # [Unsupported] Invalid Python identifier, not supported in script
	#     """ Bnd Evt  UI Mobile Overlay UI Thumbstick K2Node Component Bound Event 0 Stick Input  Delegate Signature """
	#     pass

	# def BndEvt__UI_MobileOverlay_Thumbstick_Aim_K2Node_ComponentBoundEvent_2_Stick Input__DelegateSignature(...) -> None:  # [Unsupported] Invalid Python identifier, not supported in script
	#     """ Bnd Evt  UI Mobile Overlay Thumbstick Aim K2Node Component Bound Event 2 Stick Input  Delegate Signature """
	#     pass

	def CustomEvent(self, DestroyedActor: ObjectProperty) -> None:
		""" Custom Event """  # noqa
		pass

	def BndEvt__UI_TouchInterface_Shooter_Btn_Jump_K2Node_ComponentBoundEvent_6_OnButtonReleasedEvent__DelegateSignature(self) -> None:
		""" Bnd Evt  UI Touch Interface Shooter Btn Jump K2Node Component Bound Event 6 on Button Released Event  Delegate Signature """  # noqa
		pass

	pass

class SKEL_UI_TouchSimple_C(UserWidget):
	""" UI Touch Simple """  # noqa

	UberGraphFrame: StructProperty

	Thumbstick_Move: ObjectProperty

	Thumbstick_Aim: ObjectProperty

	Btn_Jump: ObjectProperty

	# In String: StrProperty  # [Unsupported] Invalid Python identifier, not supported in script

	def Construct(self) -> None:
		""" Called after the underlying slate widget is constructed.  Depending on how the slate object is used
		this event may be called multiple times due to adding and removing from the hierarchy.
		If you need a true called-once-when-created event, use OnInitialized.
		"""  # noqa
		pass

	def CustomEvent_0(self, DestroyedActor: ObjectProperty) -> None:
		""" Custom Event 0 """  # noqa
		pass

	def BndEvt__UI_MobileOverlay_Btn_Jump_K2Node_ComponentBoundEvent_3_OnButtonPressedEvent__DelegateSignature(self) -> None:
		""" Bnd Evt  UI Mobile Overlay Btn Jump K2Node Component Bound Event 3 on Button Pressed Event  Delegate Signature """  # noqa
		pass

	# def BndEvt__UI_MobileOverlay_UI_Thumbstick_K2Node_ComponentBoundEvent_0_Stick Input__DelegateSignature(...) -> None:  # [Unsupported] Invalid Python identifier, not supported in script
	#     """ Bnd Evt  UI Mobile Overlay UI Thumbstick K2Node Component Bound Event 0 Stick Input  Delegate Signature """
	#     pass

	# def BndEvt__UI_MobileOverlay_Thumbstick_Aim_K2Node_ComponentBoundEvent_2_Stick Input__DelegateSignature(...) -> None:  # [Unsupported] Invalid Python identifier, not supported in script
	#     """ Bnd Evt  UI Mobile Overlay Thumbstick Aim K2Node Component Bound Event 2 Stick Input  Delegate Signature """
	#     pass

	def BndEvt__UI_TouchInterface_FirstPerson_Btn_Jump_K2Node_ComponentBoundEvent_1_OnButtonReleasedEvent__DelegateSignature(self) -> None:
		""" Bnd Evt  UI Touch Interface First Person Btn Jump K2Node Component Bound Event 1 on Button Released Event  Delegate Signature """  # noqa
		pass

	pass

class StandardMacros_C(Object):
	""" Standard Macros """  # noqa

	pass

class UI_Horror_C(HorrorUI):
	""" UI Horror """  # noqa

	UberGraphFrame: StructProperty

	SafeZone_0: ObjectProperty

	ProgressBar_21: ObjectProperty

	Drain: ObjectProperty

	def BP_SprintMeterUpdated(self, Percent: FloatProperty) -> None:
		""" Passes control to Blueprint to update the sprint meter widgets """  # noqa
		pass

	def BP_SprintStateChanged(self, bSprinting: BoolProperty) -> None:
		""" Passes control to Blueprint to update the sprint meter status """  # noqa
		pass

	def ExecuteUbergraph_UI_Horror(self, EntryPoint: IntProperty, CallFunc_PlayAnimation_ReturnValue: StructProperty, K2Node_Event_Percent: FloatProperty, K2Node_Event_bSprinting: BoolProperty) -> None:
		""" Execute Ubergraph UI Horror """  # noqa
		pass

	pass

class UI_ShooterBulletCounter_C(ShooterBulletCounterUI):
	""" UI Shooter Bullet Counter """  # noqa

	UberGraphFrame: StructProperty

	Image_33: ObjectProperty

	DM_BulletCount: ObjectProperty

	# Mag Size: IntProperty  # [Unsupported] Invalid Python identifier, not supported in script

	# Bullet Count: IntProperty  # [Unsupported] Invalid Python identifier, not supported in script

	# Bullet Thickness: DoubleProperty  # [Unsupported] Invalid Python identifier, not supported in script

	def PreConstruct(self, IsDesignTime: BoolProperty) -> None:
		""" Called by both the game and the editor.  Allows users to run initial setup for their widgets to better preview
		the setup in the designer and since generally that same setup code is required at runtime, it's called there
		as well.

		**WARNING**
		This is intended purely for cosmetic updates using locally owned data, you can not safely access any game related
		state, if you call something that doesn't expect to be run at editor time, you may crash the editor.

		In the event you save the asset with blueprint code that causes a crash on evaluation.  You can turn off
		PreConstruct evaluation in the Widget Designer settings in the Editor Preferences.
		"""  # noqa
		pass

	def BP_UpdateBulletCounter(self, MagazineSize: IntProperty, BulletCount: IntProperty) -> None:
		""" Allows Blueprint to update sub-widgets with the new bullet count """  # noqa
		pass

	def ExecuteUbergraph_UI_ShooterBulletCounter(self, EntryPoint: IntProperty, Temp_bool_Variable: BoolProperty, Temp_byte_Variable: EnumProperty, Temp_byte_Variable_1: EnumProperty, K2Node_Event_IsDesignTime: BoolProperty, CallFunc_CreateDynamicMaterialInstance_ReturnValue: ObjectProperty, K2Node_Event_MagazineSize: IntProperty, K2Node_Event_BulletCount: IntProperty, CallFunc_Conv_IntToDouble_ReturnValue: DoubleProperty, CallFunc_Conv_IntToDouble_ReturnValue_1: DoubleProperty, CallFunc_Conv_IntToDouble_ReturnValue_2: DoubleProperty, CallFunc_Multiply_DoubleDouble_ReturnValue: DoubleProperty, CallFunc_MakeVector2D_ReturnValue: StructProperty, CallFunc_Greater_IntInt_ReturnValue: BoolProperty, K2Node_Select_Default: EnumProperty, CallFunc_SetScalarParameterValue_Value_ImplicitCast: FloatProperty, CallFunc_SetScalarParameterValue_Value_ImplicitCast_1: FloatProperty) -> None:
		""" Execute Ubergraph UI Shooter Bullet Counter """  # noqa
		pass

	pass

class UI_Shooter_C(ShooterUI):
	""" UI Shooter """  # noqa

	UberGraphFrame: StructProperty

	Team2Score: ObjectProperty

	Team1Score: ObjectProperty

	Image_58: ObjectProperty

	Image_57: ObjectProperty

	def BP_UpdateScore(self, TeamByte: ByteProperty, Score: IntProperty) -> None:
		""" Allows Blueprint to update score sub-widgets """  # noqa
		pass

	def ExecuteUbergraph_UI_Shooter(self, EntryPoint: IntProperty, Temp_byte_Variable: ByteProperty, K2Node_Event_TeamByte: ByteProperty, K2Node_Event_Score: IntProperty, CallFunc_Conv_IntToText_ReturnValue: TextProperty, K2Node_Select_Default: ObjectProperty) -> None:
		""" Execute Ubergraph UI Shooter """  # noqa
		pass

	pass

class UI_Thumbstick_C(UserWidget):
	""" UI Thumbstick """  # noqa

	UberGraphFrame: StructProperty

	Stick: ObjectProperty

	Container: ObjectProperty

	TrackInput: BoolProperty

	# Initial Touch Location: StructProperty  # [Unsupported] Invalid Python identifier, not supported in script

	Position: StructProperty

	# Stick Input: MulticastInlineDelegateProperty  # [Unsupported] Invalid Python identifier, not supported in script

	# Thumbstick Size: StructProperty  # [Unsupported] Invalid Python identifier, not supported in script

	# Invert X: BoolProperty  # [Unsupported] Invalid Python identifier, not supported in script

	# Invert Y: BoolProperty  # [Unsupported] Invalid Python identifier, not supported in script

	# Scale Output: DoubleProperty  # [Unsupported] Invalid Python identifier, not supported in script

	# Touch Target: ByteProperty  # [Unsupported] Invalid Python identifier, not supported in script

	# def Touch Input Check(...) -> None:  # [Unsupported] Invalid Python identifier, not supported in script
	#     """ Touch Input Check """
	#     pass

	def PreConstruct(self, IsDesignTime: BoolProperty) -> None:
		""" Called by both the game and the editor.  Allows users to run initial setup for their widgets to better preview
		the setup in the designer and since generally that same setup code is required at runtime, it's called there
		as well.

		**WARNING**
		This is intended purely for cosmetic updates using locally owned data, you can not safely access any game related
		state, if you call something that doesn't expect to be run at editor time, you may crash the editor.

		In the event you save the asset with blueprint code that causes a crash on evaluation.  You can turn off
		PreConstruct evaluation in the Widget Designer settings in the Editor Preferences.
		"""  # noqa
		pass

	def Tick(self, MyGeometry: StructProperty, InDeltaTime: FloatProperty) -> None:
		""" Ticks this widget.  Override in derived classes, but always call the parent implementation.

		@param  MyGeometry The space allotted for this widget
		@param  InDeltaTime  Real time passed since last tick
		"""  # noqa
		pass

	def ExecuteUbergraph_UI_Thumbstick(self, EntryPoint: IntProperty, CallFunc_MakeVector2D_ReturnValue: StructProperty, CallFunc_BreakVector2D_X: DoubleProperty, CallFunc_BreakVector2D_Y: DoubleProperty, CallFunc_BreakVector2D_X_1: DoubleProperty, CallFunc_BreakVector2D_Y_1: DoubleProperty, CallFunc_Divide_DoubleDouble_ReturnValue: DoubleProperty, CallFunc_Divide_DoubleDouble_ReturnValue_1: DoubleProperty, CallFunc_Multiply_DoubleDouble_ReturnValue: DoubleProperty, CallFunc_Multiply_DoubleDouble_ReturnValue_1: DoubleProperty, CallFunc_Divide_Vector2DVector2D_ReturnValue: StructProperty, CallFunc_GetViewportScale_ReturnValue: FloatProperty, CallFunc_Conv_DoubleToVector2D_ReturnValue: StructProperty, CallFunc_GetPlayerController_ReturnValue: ObjectProperty, CallFunc_BreakVector2D_X_2: DoubleProperty, CallFunc_BreakVector2D_Y_2: DoubleProperty, CallFunc_GetInputTouchState_LocationX: FloatProperty, CallFunc_GetInputTouchState_LocationY: FloatProperty, CallFunc_GetInputTouchState_bIsCurrentlyPressed: BoolProperty, CallFunc_Subtract_DoubleDouble_ReturnValue: DoubleProperty, CallFunc_Subtract_DoubleDouble_ReturnValue_1: DoubleProperty, CallFunc_FClamp_ReturnValue: DoubleProperty, CallFunc_FClamp_ReturnValue_1: DoubleProperty, CallFunc_Divide_DoubleDouble_ReturnValue_2: DoubleProperty, CallFunc_MakeVector2D_ReturnValue_1: StructProperty, CallFunc_Multiply_DoubleDouble_ReturnValue_2: DoubleProperty, CallFunc_SelectFloat_ReturnValue: DoubleProperty, CallFunc_Divide_DoubleDouble_ReturnValue_3: DoubleProperty, CallFunc_Multiply_DoubleDouble_ReturnValue_3: DoubleProperty, CallFunc_Multiply_DoubleDouble_ReturnValue_4: DoubleProperty, CallFunc_SelectFloat_ReturnValue_1: DoubleProperty, CallFunc_SelectFloat_ReturnValue_2: DoubleProperty, K2Node_Event_IsDesignTime: BoolProperty, CallFunc_Multiply_DoubleDouble_ReturnValue_5: DoubleProperty, K2Node_Event_MyGeometry: StructProperty, K2Node_Event_InDeltaTime: FloatProperty, CallFunc_SelectFloat_ReturnValue_3: DoubleProperty, CallFunc_LocalToViewport_PixelPosition: StructProperty, CallFunc_LocalToViewport_ViewportPosition: StructProperty, CallFunc_MakeVector2D_ReturnValue_2: StructProperty, CallFunc_Multiply_Vector2DVector2D_ReturnValue: StructProperty, CallFunc_SetWidthOverride_InWidthOverride_ImplicitCast: FloatProperty, CallFunc_SetHeightOverride_InHeightOverride_ImplicitCast: FloatProperty, CallFunc_Conv_DoubleToVector2D_InDouble_ImplicitCast: DoubleProperty, CallFunc_Subtract_DoubleDouble_A_ImplicitCast: DoubleProperty, CallFunc_Subtract_DoubleDouble_A_ImplicitCast_1: DoubleProperty) -> None:
		""" Execute Ubergraph UI Thumbstick """  # noqa
		pass

	# def Stick Input__DelegateSignature(...) -> None:  # [Unsupported] Invalid Python identifier, not supported in script
	#     """ Stick Input  Delegate Signature """
	#     pass

	pass

class UI_TouchInterface_Shooter_C(UserWidget):
	""" UI Touch Interface Shooter """  # noqa

	UberGraphFrame: StructProperty

	Thumbstick_Move: ObjectProperty

	Thumbstick_Aim: ObjectProperty

	Btn_SwitchWeapon: ObjectProperty

	Btn_Jump: ObjectProperty

	Btn_FireWeapon: ObjectProperty

	# In String: StrProperty  # [Unsupported] Invalid Python identifier, not supported in script

	def BndEvt__UI_MobileOverlay_Button_1_K2Node_ComponentBoundEvent_1_OnButtonPressedEvent__DelegateSignature(self) -> None:
		""" Bnd Evt  UI Mobile Overlay Button 1 K2Node Component Bound Event 1 on Button Pressed Event  Delegate Signature """  # noqa
		pass

	def BndEvt__UI_MobileOverlay_Btn_Jump_K2Node_ComponentBoundEvent_3_OnButtonPressedEvent__DelegateSignature(self) -> None:
		""" Bnd Evt  UI Mobile Overlay Btn Jump K2Node Component Bound Event 3 on Button Pressed Event  Delegate Signature """  # noqa
		pass

	def BndEvt__UI_MobileOverlay_Btn_FireWeapon_K2Node_ComponentBoundEvent_4_OnButtonPressedEvent__DelegateSignature(self) -> None:
		""" Bnd Evt  UI Mobile Overlay Btn Fire Weapon K2Node Component Bound Event 4 on Button Pressed Event  Delegate Signature """  # noqa
		pass

	def BndEvt__UI_MobileOverlay_Btn_FireWeapon_K2Node_ComponentBoundEvent_5_OnButtonReleasedEvent__DelegateSignature(self) -> None:
		""" Bnd Evt  UI Mobile Overlay Btn Fire Weapon K2Node Component Bound Event 5 on Button Released Event  Delegate Signature """  # noqa
		pass

	# def BndEvt__UI_MobileOverlay_UI_Thumbstick_K2Node_ComponentBoundEvent_0_Stick Input__DelegateSignature(...) -> None:  # [Unsupported] Invalid Python identifier, not supported in script
	#     """ Bnd Evt  UI Mobile Overlay UI Thumbstick K2Node Component Bound Event 0 Stick Input  Delegate Signature """
	#     pass

	# def BndEvt__UI_MobileOverlay_Thumbstick_Aim_K2Node_ComponentBoundEvent_2_Stick Input__DelegateSignature(...) -> None:  # [Unsupported] Invalid Python identifier, not supported in script
	#     """ Bnd Evt  UI Mobile Overlay Thumbstick Aim K2Node Component Bound Event 2 Stick Input  Delegate Signature """
	#     pass

	def CustomEvent(self, DestroyedActor: ObjectProperty) -> None:
		""" Custom Event """  # noqa
		pass

	def Construct(self) -> None:
		""" Called after the underlying slate widget is constructed.  Depending on how the slate object is used
		this event may be called multiple times due to adding and removing from the hierarchy.
		If you need a true called-once-when-created event, use OnInitialized.
		"""  # noqa
		pass

	def BndEvt__UI_TouchInterface_Shooter_Btn_Jump_K2Node_ComponentBoundEvent_6_OnButtonReleasedEvent__DelegateSignature(self) -> None:
		""" Bnd Evt  UI Touch Interface Shooter Btn Jump K2Node Component Bound Event 6 on Button Released Event  Delegate Signature """  # noqa
		pass

	def ExecuteUbergraph_UI_TouchInterface_Shooter(self, EntryPoint: IntProperty, CallFunc_GetOwningPlayer_ReturnValue: ObjectProperty, CallFunc_K2_GetPawn_ReturnValue: ObjectProperty, K2Node_DynamicCast_AsBPI_Touch_Shooter: InterfaceProperty, K2Node_DynamicCast_bSuccess: BoolProperty, K2Node_ComponentBoundEvent_NewParam_1: StructProperty, K2Node_ComponentBoundEvent_NewParam: StructProperty, K2Node_CustomEvent_DestroyedActor: ObjectProperty, CallFunc_GetOwningPlayer_ReturnValue_1: ObjectProperty, CallFunc_GetOwningPlayer_ReturnValue_2: ObjectProperty, CallFunc_K2_GetPawn_ReturnValue_1: ObjectProperty, CallFunc_GetOwningPlayer_ReturnValue_3: ObjectProperty, CallFunc_GetOwningPlayer_ReturnValue_4: ObjectProperty, CallFunc_K2_GetPawn_ReturnValue_2: ObjectProperty, CallFunc_K2_GetPawn_ReturnValue_3: ObjectProperty, K2Node_DynamicCast_AsBPI_Touch_Interface: InterfaceProperty, K2Node_DynamicCast_bSuccess_1: BoolProperty, K2Node_DynamicCast_AsBPI_Touch_Interface_1: InterfaceProperty, K2Node_DynamicCast_bSuccess_2: BoolProperty, K2Node_DynamicCast_AsBPI_Touch_Interface_2: InterfaceProperty, K2Node_DynamicCast_bSuccess_3: BoolProperty, K2Node_DynamicCast_AsBPI_Touch_Interface_3: InterfaceProperty, K2Node_DynamicCast_bSuccess_4: BoolProperty, CallFunc_GetOwningPlayer_ReturnValue_5: ObjectProperty, CallFunc_K2_GetPawn_ReturnValue_4: ObjectProperty, K2Node_CreateDelegate_OutputDelegate: DelegateProperty, K2Node_DynamicCast_AsBPI_Touch_Shooter_1: InterfaceProperty, K2Node_DynamicCast_bSuccess_5: BoolProperty, K2Node_DynamicCast_AsBPI_Touch_Shooter_2: InterfaceProperty, K2Node_DynamicCast_bSuccess_6: BoolProperty) -> None:
		""" Execute Ubergraph UI Touch Interface Shooter """  # noqa
		pass

	pass

class UI_TouchSimple_C(UserWidget):
	""" UI Touch Simple """  # noqa

	UberGraphFrame: StructProperty

	Thumbstick_Move: ObjectProperty

	Thumbstick_Aim: ObjectProperty

	Btn_Jump: ObjectProperty

	# In String: StrProperty  # [Unsupported] Invalid Python identifier, not supported in script

	def CustomEvent_0(self, DestroyedActor: ObjectProperty) -> None:
		""" Custom Event 0 """  # noqa
		pass

	def BndEvt__UI_MobileOverlay_Btn_Jump_K2Node_ComponentBoundEvent_3_OnButtonPressedEvent__DelegateSignature(self) -> None:
		""" Bnd Evt  UI Mobile Overlay Btn Jump K2Node Component Bound Event 3 on Button Pressed Event  Delegate Signature """  # noqa
		pass

	# def BndEvt__UI_MobileOverlay_UI_Thumbstick_K2Node_ComponentBoundEvent_0_Stick Input__DelegateSignature(...) -> None:  # [Unsupported] Invalid Python identifier, not supported in script
	#     """ Bnd Evt  UI Mobile Overlay UI Thumbstick K2Node Component Bound Event 0 Stick Input  Delegate Signature """
	#     pass

	# def BndEvt__UI_MobileOverlay_Thumbstick_Aim_K2Node_ComponentBoundEvent_2_Stick Input__DelegateSignature(...) -> None:  # [Unsupported] Invalid Python identifier, not supported in script
	#     """ Bnd Evt  UI Mobile Overlay Thumbstick Aim K2Node Component Bound Event 2 Stick Input  Delegate Signature """
	#     pass

	def Construct(self) -> None:
		""" Called after the underlying slate widget is constructed.  Depending on how the slate object is used
		this event may be called multiple times due to adding and removing from the hierarchy.
		If you need a true called-once-when-created event, use OnInitialized.
		"""  # noqa
		pass

	def BndEvt__UI_TouchInterface_FirstPerson_Btn_Jump_K2Node_ComponentBoundEvent_1_OnButtonReleasedEvent__DelegateSignature(self) -> None:
		""" Bnd Evt  UI Touch Interface First Person Btn Jump K2Node Component Bound Event 1 on Button Released Event  Delegate Signature """  # noqa
		pass

	def ExecuteUbergraph_UI_TouchSimple(self, EntryPoint: IntProperty, CallFunc_GetOwningPlayer_ReturnValue: ObjectProperty, K2Node_CustomEvent_DestroyedActor: ObjectProperty, CallFunc_K2_GetPawn_ReturnValue: ObjectProperty, K2Node_DynamicCast_AsBPI_Touch_Interface: InterfaceProperty, K2Node_DynamicCast_bSuccess: BoolProperty, K2Node_DynamicCast_AsBPI_Touch_Interface_1: InterfaceProperty, K2Node_DynamicCast_bSuccess_1: BoolProperty, CallFunc_GetOwningPlayer_ReturnValue_1: ObjectProperty, K2Node_ComponentBoundEvent_NewParam_1: StructProperty, K2Node_ComponentBoundEvent_NewParam: StructProperty, CallFunc_GetOwningPlayer_ReturnValue_2: ObjectProperty, CallFunc_GetOwningPlayer_ReturnValue_3: ObjectProperty, CallFunc_K2_GetPawn_ReturnValue_1: ObjectProperty, CallFunc_K2_GetPawn_ReturnValue_2: ObjectProperty, K2Node_DynamicCast_AsBPI_Touch_Interface_2: InterfaceProperty, K2Node_DynamicCast_bSuccess_2: BoolProperty, K2Node_DynamicCast_AsBPI_Touch_Interface_3: InterfaceProperty, K2Node_DynamicCast_bSuccess_3: BoolProperty, K2Node_CreateDelegate_OutputDelegate: DelegateProperty) -> None:
		""" Execute Ubergraph UI Touch Simple """  # noqa
		pass

	pass

# endregion Blueprint Classes

# region Blueprint Structs

class Default__AISenseBlueprintListener(StructBase):
	""" Default  AISense Blueprint Listener """  # noqa

	pass

class Default__UserDefinedStruct(StructBase):
	""" Default  User Defined Struct """  # noqa

	pass

# endregion Blueprint Structs

# region Blueprint Enums

class Default__UserDefinedEnum(EnumBase):
	""" Default  User Defined Enum """  # noqa

	pass

class ENiagaraChannelCorrelation(EnumBase):
	""" ENiagara Channel Correlation """  # noqa

	pass

class ENiagaraEmitterLifeCycleMode(EnumBase):
	""" ENiagara Emitter Life Cycle Mode """  # noqa

	pass

class ENiagaraEmitterScalabilityMode_Limited(EnumBase):
	""" ENiagara Emitter Scalability Mode Limited """  # noqa

	pass

class ENiagaraExpansionMode(EnumBase):
	""" ENiagara Expansion Mode """  # noqa

	pass

class ENiagaraInactiveMode(EnumBase):
	""" ENiagara Inactive Mode """  # noqa

	pass

class ENiagaraRandomnessEvaluation(EnumBase):
	""" ENiagara Randomness Evaluation """  # noqa

	pass

class ENiagaraRandomnessMode(EnumBase):
	""" Toggles the behavior of the random number generator. """  # noqa

	pass

class ENiagaraRingDiscMode(EnumBase):
	""" ENiagara Ring Disc Mode """  # noqa

	pass

class ENiagaraScaleColorMode(EnumBase):
	""" ENiagara Scale Color Mode """  # noqa

	pass

class ENiagaraShapeTorusMode(EnumBase):
	""" ENiagara Shape Torus Mode """  # noqa

	pass

class ENiagaraSphereDistributionMode(EnumBase):
	""" ENiagara Sphere Distribution Mode """  # noqa

	pass

class ENiagaraSystemInactiveMode(EnumBase):
	""" ENiagara System Inactive Mode """  # noqa

	pass

class ENiagaraTorusDistributionMode(EnumBase):
	""" ENiagara Torus Distribution Mode """  # noqa

	pass

class ENiagara_AngleInput(EnumBase):
	""" ENiagara Angle Input """  # noqa

	pass

class ENiagara_AttributeSamplingApplyOutput(EnumBase):
	""" ENiagara Attribute Sampling Apply Output """  # noqa

	pass

class ENiagara_BoxPlaneMode(EnumBase):
	""" ENiagara Box Plane Mode """  # noqa

	pass

class ENiagara_ColorInitializationMode(EnumBase):
	""" ENiagara Color Initialization Mode """  # noqa

	pass

class ENiagara_ConeMode(EnumBase):
	""" ENiagara Cone Mode """  # noqa

	pass

class ENiagara_CylinderMode(EnumBase):
	""" ENiagara Cylinder Mode """  # noqa

	pass

class ENiagara_EmitterStateOptions(EnumBase):
	""" ENiagara Emitter State Options """  # noqa

	pass

class ENiagara_InfiniteLoopDuration(EnumBase):
	""" ENiagara Infinite Loop Duration """  # noqa

	pass

class ENiagara_LifetimeMode(EnumBase):
	""" ENiagara Lifetime Mode """  # noqa

	pass

class ENiagara_LocationShapes(EnumBase):
	""" ENiagara Location Shapes """  # noqa

	pass

class ENiagara_MassInitializationMode(EnumBase):
	""" ENiagara Mass Initialization Mode """  # noqa

	pass

class ENiagara_OffsetMode(EnumBase):
	""" ENiagara Offset Mode """  # noqa

	pass

class ENiagara_PositionInitializationMode(EnumBase):
	""" ENiagara Position Initialization Mode """  # noqa

	pass

class ENiagara_RotationMode(EnumBase):
	""" ENiagara Rotation Mode """  # noqa

	pass

class ENiagara_ScaleMode(EnumBase):
	""" ENiagara Scale Mode """  # noqa

	pass

class ENiagara_SizeScaleMode(EnumBase):
	""" ENiagara Size Scale Mode """  # noqa

	pass

class ENiagara_SpriteRotationMode(EnumBase):
	""" ENiagara Sprite Rotation Mode """  # noqa

	pass

class ENiagara_TransformOrder(EnumBase):
	""" ENiagara Transform Order """  # noqa

	pass

class ENiagara_TransformType(EnumBase):
	""" ENiagara Transform Type """  # noqa

	pass

class ENiagara_UVFlippingMode(EnumBase):
	""" ENiagara UVFlipping Mode """  # noqa

	pass

class ENiagara_UnsetDirectSet(EnumBase):
	""" ENiagara Unset Direct Set """  # noqa

	pass

class ENiagara_UnsetDirectSetRandom(EnumBase):
	""" ENiagara Unset Direct Set Random """  # noqa

	pass

# endregion Blueprint Enums
