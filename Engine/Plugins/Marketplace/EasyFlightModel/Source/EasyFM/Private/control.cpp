// Copyright 2016 Mookie. All Rights Reserved.

#include "flightmodel.h"
#include "flightmodelcomponent.h"

void UeasyFM::SimulateControls(float DeltaTime, float alpha, float slip, float pitchrate, float yawrate, float rollrate, float speed, const FTransform &Transform, FVector location, FVector rawVelocity, float air) {
		speed = FMath::Sqrt(FMath::Pow(speed,2.0f)*air);

		FVector fwdvec, upvec, rightvec;
		GetAxisVectors(Transform, fwdvec, upvec, rightvec);

		float pitchTemp = FMath::Clamp(PitchInput, -1.0f, 1.0f);
		float yawTemp = FMath::Clamp(YawInput, -1.0f, 1.0f);
		float rollTemp = FMath::Clamp(RollInput, -1.0f, 1.0f);
		float flapsTemp = FMath::Clamp(FlapsInput, 0.0f, 1.0f);
		float spoilerTemp = FMath::Clamp(SpoilerInput, 0.0f, 1.0f);
		float throttleTemp = FMath::Clamp(ThrottleInput, -1.0f, 1.0f);
		float vtolTemp = FMath::Clamp(VTOLInput, -1.0f, 1.0f);
		float reverserTemp = FMath::Clamp(ReverserInput, -1.0f, 1.0f);

		float augScale;
		if (DampingScaleWithSpeed) {
			augScale = 1.0f - FMath::Clamp(FMath::Pow(speed,2.0f) / FMath::Pow(DampingShutoffSpeed, 2.0f), 0.0f, 1.0f);
		}
		else {
			augScale = 1.0f;
		}

		//guidance
		if (Guided) {

			FVector fwdvecComp;
			FVector upvecComp;
			FVector rightvecComp;
			FVector targetVec;

			if (GuidedToLocation) {
				targetVec = (GuidedVector - location).GetSafeNormal();
			}
			else {
				targetVec = GuidedVector.GetSafeNormal();
			}

			if (GuidedCompensateAlpha) {
				fwdvecComp = rawVelocity.GetSafeNormal();
				upvecComp = fwdvecComp.RotateAngleAxis(-90.0f, rightvec);
				rightvecComp = fwdvecComp.RotateAngleAxis(90.0f, upvec);
			}
			else {
				fwdvecComp = fwdvec;
				upvecComp = upvec;
				rightvecComp = rightvec;
			}

			float guideFwd = FVector::DotProduct(fwdvecComp, targetVec);
			float guideUp = -FVector::DotProduct(upvecComp, targetVec);
			float guideRight = FVector::DotProduct(rightvecComp, targetVec);

			if (GuidedPitchFullRange) {
				guideUp = -FMath::Atan2(-guideUp, guideFwd);
			}

			if (GuidedYawFullRange) {
				guideRight = FMath::Atan2(guideRight, guideFwd);
			}


			PitchTrimPos = FMath::Clamp(PitchTrimPos + (GuidedPID.PitchI*guideUp), -1.0f, 1.0f);
			YawTrimPos = FMath::Clamp(YawTrimPos + (GuidedPID.YawI*guideRight), -1.0f, 1.0f);
			RollTrimPos = FMath::Clamp(RollTrimPos + (GuidedPID.RollI*guideRight), -1.0f, 1.0f);

			float pitchGuide = (guideUp*GuidedPID.PitchP) + PitchTrimPos - (pitchrate*GuidedPID.PitchD*augScale);
			float yawGuide = (guideRight*GuidedPID.YawP) + YawTrimPos - (yawrate*GuidedPID.YawD*augScale);
			float rollGuide = (guideRight*GuidedPID.RollP) + RollTrimPos + (rollrate*GuidedPID.RollD*augScale);

			float autoLevel = FVector::DotProduct(rightvecComp, FVector(0, 0, 1))*(1.0f - FMath::Clamp(FMath::Abs(pitchGuide), 0.0f, 1.0f));

			pitchTemp = BlendInput(pitchGuide, pitchTemp);
			yawTemp = BlendInput(yawGuide, yawTemp);
			rollTemp = BlendInput(rollGuide + autoLevel*GuidedAutoLevel, rollTemp);

		}
		else {
			float dampedPitch = 0; float dampedYaw = 0; float dampedRoll = 0;

			if (StabilityAugmentation) {
				dampedPitch = -pitchrate*PitchAug*augScale;
				dampedYaw = -yawrate*YawAug*augScale;
				dampedRoll = rollrate*RollAug*augScale;
			}

			if (AutoTrim) {
				if (!InputPauseAutoTrim || FMath::Abs(pitchTemp) <= PauseAutoTrimThreshold) {
					if (TrimPitchTo1G) {
						FVector newAcceleration = acceleration - FVector(0.0f, 0.0f, GetWorld()->GetGravityZ());
						float AccelZ = FVector::DotProduct(newAcceleration, upvec);
						PitchTrimPos = FMath::Clamp(PitchTrimPos + ((AccelZ + GetWorld()->GetGravityZ())*DeltaTime*AutoTrimPitchSensitivity*augScale), -1.0f, 1.0f);
					}
					else {
						PitchTrimPos = FMath::Clamp(PitchTrimPos - (pitchrate*DeltaTime*AutoTrimPitchSensitivity*augScale), -1.0f, 1.0f);
					}
				}

				if (!InputPauseAutoTrim || FMath::Abs(yawTemp) <= PauseAutoTrimThreshold) {
					if (TrimYawToCenter) {
						YawTrimPos = FMath::Clamp(YawTrimPos - ((slip)*DeltaTime*AutoTrimYawSensitivity*augScale), -1.0f, 1.0f);
					}
					else {
						YawTrimPos = FMath::Clamp(YawTrimPos + (-yawrate*DeltaTime*AutoTrimYawSensitivity*augScale), -1.0f, 1.0f);
					}
				}

				if (!InputPauseAutoTrim || FMath::Abs(rollTemp) <= PauseAutoTrimThreshold) {
					RollTrimPos = FMath::Clamp(RollTrimPos + (rollrate*DeltaTime*AutoTrimRollSensitivity*augScale), -1.0f, 1.0f);
				}
			}

			float yawCenter = YawCentering*-slip;

			pitchTemp = BlendInput(PitchTrimPos + dampedPitch, pitchTemp);
			yawTemp = BlendInput(YawTrimPos + dampedYaw + yawCenter, yawTemp);
			rollTemp = BlendInput(RollTrimPos + dampedRoll, rollTemp);
		}


		if (AlphaLimiter) {
			pitchTemp = Limiter(pitchTemp, alpha - (pitchrate*PitchRateInfluence*augScale), MinAlpha, MaxAlpha, AlphaLimiterSensitivity);
		}

		if (SlipLimiter) {
			yawTemp = Limiter(yawTemp, -slip - (yawrate*YawRateInfluence*augScale), -MaxSlip, MaxSlip, SlipLimiterSensitivity);
		}

		if (AutoThrottle) {
			if (!InputPauseAutoThrottle || FMath::Abs(throttleTemp) <= PauseAutoThrottleThreshold) {
				if (AutoThrottleHover) {
					float vspeed = rawVelocity.Z;
					float acc = acceleration.Z;
					AutoThrottlePos = FMath::Clamp(AutoThrottlePos - (DeltaTime*(AutoThrottleSensitivity*vspeed + HoverAccelerationInfluence*acc)), -1.0f, 1.0f);
				}
				else {
					float diff = speed - previousAirspeed;
					previousAirspeed = speed;
					AutoThrottlePos = FMath::Clamp(AutoThrottlePos - (AutoThrottleSensitivity*diff), -1.0f, 1.0f);
				};
			};
			if (!AutoSpoiler) {
				AutoThrottlePos = FMath::Max(AutoThrottlePos, 0.0f);
			}
			else {
				spoilerTemp = BlendInput(0.0f - AutoThrottlePos, -throttleTemp);
			}
			throttleTemp = BlendInput(AutoThrottlePos, throttleTemp);
		}

		if (AutoFlaps) {
			float range = FullExtendSpeed - FullRetractSpeed;
			flapsTemp = (speed - FullRetractSpeed) / range;
		}


		if (ScaleSensitivityWithSpeed) {
			pitchTemp *= GetCurveValue(SpeedSensitivityCurve.Pitch, speed);
			yawTemp *= GetCurveValue(SpeedSensitivityCurve.Yaw, speed);
			rollTemp *= GetCurveValue(SpeedSensitivityCurve.Roll, speed);
		}

		//VTOL
		VTOLPos = FMath::Clamp(Transit(VTOLPos, vtolTemp, VTOLExtensionTime, VTOLRetractionTime, DeltaTime), 0.0f, 1.0f);
		PitchPos = FMath::Clamp(Transit(PitchPos, pitchTemp, PitchMoveTime, PitchMoveTime, DeltaTime), -1.0f, 1.0f);
		YawPos = FMath::Clamp(Transit(YawPos, yawTemp, YawMoveTime, YawMoveTime, DeltaTime), -1.0f, 1.0f);
		RollPos = FMath::Clamp(Transit(RollPos, rollTemp, RollMoveTime, RollMoveTime, DeltaTime), -1.0f, 1.0f);
		FlapsPos = FMath::Clamp(Transit(FlapsPos, flapsTemp, FlapsExtensionTime, FlapsRetractionTime, DeltaTime), 0.0f, 1.0f);
		SpoilerPos = FMath::Clamp(Transit(SpoilerPos, spoilerTemp, SpoilerExtensionTime, SpoilerRetractionTime, DeltaTime), 0.0f, 1.0f);
		ReverserPos = FMath::Clamp(Transit(ReverserPos, reverserTemp, ReverserExtensionTime, ReverserRetractionTime, DeltaTime), 0.0f, 1.0f);
		ThrottlePos = FMath::Clamp(Transit(ThrottlePos, throttleTemp, SpoolUpTime, SpoolDownTime, DeltaTime), 0.0f, 1.0f);
}