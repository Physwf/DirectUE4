#pragma once

#include "UnrealMath.h"

/**
* Transform composed of Scale, Rotation (as a quaternion), and Translation.
*
* Transforms can be used to convert from one space to another, for example by transforming
* positions and directions from local space to world space.
*
* Transformation of position vectors is applied in the order:  Scale -> Rotate -> Translate.
* Transformation of direction vectors is applied in the order: Scale -> Rotate.
*
* Order matters when composing transforms: C = A * B will yield a transform C that logically
* first applies A then B to any subsequent transformation. Note that this is the opposite order of quaternion (FQuat) multiplication.
*
* Example: LocalToWorld = (DeltaRotation * LocalToWorld) will change rotation in local space by DeltaRotation.
* Example: LocalToWorld = (LocalToWorld * DeltaRotation) will change rotation in world space by DeltaRotation.
*/

struct FTransform
{
	friend struct Z_Construct_UScriptStruct_FTransform_Statics;

protected:
	/** Rotation of this transformation, as a quaternion. */
	FQuat	Rotation;
	/** Translation of this transformation, as a vector. */
	Vector	Translation;
	/** 3D scale (always applied in local space) as a vector. */
	Vector	Scale3D;

public:
	/**
	* The identity transformation (Rotation = FQuat::Identity, Translation = Vector::ZeroVector, Scale3D = (1,1,1)).
	*/
	static  const FTransform Identity;

	inline void DiagnosticCheckNaN_Translate() const {}
	inline void DiagnosticCheckNaN_Rotate() const {}
	inline void DiagnosticCheckNaN_Scale3D() const {}
	inline void DiagnosticCheckNaN_All() const {}
	inline void DiagnosticCheck_IsValid() const {}

	/** Default constructor. */
	inline FTransform()
		: Rotation(0.f, 0.f, 0.f, 1.f)
		, Translation(0.f)
		, Scale3D(Vector::OneVector)
	{
	}

	/**
	* Constructor with an initial translation
	*
	* @param InTranslation The value to use for the translation component
	*/
	inline explicit FTransform(const Vector& InTranslation)
		: Rotation(FQuat::Identity),
		Translation(InTranslation),
		Scale3D(Vector::OneVector)
	{
		DiagnosticCheckNaN_All();
	}
	/**
	* Constructor with an initial rotation
	*
	* @param InRotation The value to use for rotation component
	*/
	inline explicit FTransform(const FQuat& InRotation)
		: Rotation(InRotation),
		Translation(Vector::ZeroVector),
		Scale3D(Vector::OneVector)
	{
		DiagnosticCheckNaN_All();
	}

	/**
	* Constructor with an initial rotation
	*
	* @param InRotation The value to use for rotation component  (after being converted to a quaternion)
	*/
	inline explicit FTransform(const Rotator& InRotation)
		: Rotation(InRotation),
		Translation(Vector::ZeroVector),
		Scale3D(Vector::OneVector)
	{
		DiagnosticCheckNaN_All();
	}

	/**
	* Constructor with all components initialized
	*
	* @param InRotation The value to use for rotation component
	* @param InTranslation The value to use for the translation component
	* @param InScale3D The value to use for the scale component
	*/
	inline FTransform(const FQuat& InRotation, const Vector& InTranslation, const Vector& InScale3D = Vector::OneVector)
		: Rotation(InRotation),
		Translation(InTranslation),
		Scale3D(InScale3D)
	{
		DiagnosticCheckNaN_All();
	}
	/**
	* Constructor with all components initialized, taking a Rotator as the rotation component
	*
	* @param InRotation The value to use for rotation component (after being converted to a quaternion)
	* @param InTranslation The value to use for the translation component
	* @param InScale3D The value to use for the scale component
	*/
	inline FTransform(const Rotator& InRotation, const Vector& InTranslation, const Vector& InScale3D = Vector::OneVector)
		: Rotation(InRotation),
		Translation(InTranslation),
		Scale3D(InScale3D)
	{
		DiagnosticCheckNaN_All();
	}
	/**
	* Copy-constructor
	*
	* @param InTransform The source transform from which all components will be copied
	*/
	inline FTransform(const FTransform& InTransform) :
		Rotation(InTransform.Rotation),
		Translation(InTransform.Translation),
		Scale3D(InTransform.Scale3D)
	{
		DiagnosticCheckNaN_All();
	}
	/**
	* Constructor for converting a Matrix (including scale) into a FTransform.
	*/
	inline explicit FTransform(const Matrix& InMatrix)
	{
		SetFromMatrix(InMatrix);
		DiagnosticCheckNaN_All();
	}

	/** Constructor that takes basis axes and translation */
	inline FTransform(const Vector& InX, const Vector& InY, const Vector& InZ, const Vector& InTranslation)
	{
		SetFromMatrix(Matrix(InX, InY, InZ, InTranslation));
		DiagnosticCheckNaN_All();
	}

	/**
	* Does a debugf of the contents of this Transform.
	*/
	 void DebugPrint() const;

	/** Debug purpose only **/
	bool DebugEqualMatrix(const Matrix& M) const;

	 /**
	 * Copy another Transform into this one
	 */
	 inline FTransform& operator=(const FTransform& Other)
	 {
		 this->Rotation = Other.Rotation;
		 this->Translation = Other.Translation;
		 this->Scale3D = Other.Scale3D;

		 return *this;
	 }

	/**
	* Convert this Transform to a transformation matrix with scaling.
	*/
	inline Matrix ToMatrixWithScale() const
	{
		Matrix OutMatrix;

		OutMatrix.M[3][0] = Translation.X;
		OutMatrix.M[3][1] = Translation.Y;
		OutMatrix.M[3][2] = Translation.Z;

		const float x2 = Rotation.X + Rotation.X;
		const float y2 = Rotation.Y + Rotation.Y;
		const float z2 = Rotation.Z + Rotation.Z;
		{
			const float xx2 = Rotation.X * x2;
			const float yy2 = Rotation.Y * y2;
			const float zz2 = Rotation.Z * z2;

			OutMatrix.M[0][0] = (1.0f - (yy2 + zz2)) * Scale3D.X;
			OutMatrix.M[1][1] = (1.0f - (xx2 + zz2)) * Scale3D.Y;
			OutMatrix.M[2][2] = (1.0f - (xx2 + yy2)) * Scale3D.Z;
		}
		{
			const float yz2 = Rotation.Y * z2;
			const float wx2 = Rotation.W * x2;

			OutMatrix.M[2][1] = (yz2 - wx2) * Scale3D.Z;
			OutMatrix.M[1][2] = (yz2 + wx2) * Scale3D.Y;
		}
		{
			const float xy2 = Rotation.X * y2;
			const float wz2 = Rotation.W * z2;

			OutMatrix.M[1][0] = (xy2 - wz2) * Scale3D.Y;
			OutMatrix.M[0][1] = (xy2 + wz2) * Scale3D.X;
		}
		{
			const float xz2 = Rotation.X * z2;
			const float wy2 = Rotation.W * y2;

			OutMatrix.M[2][0] = (xz2 + wy2) * Scale3D.Z;
			OutMatrix.M[0][2] = (xz2 - wy2) * Scale3D.X;
		}

		OutMatrix.M[0][3] = 0.0f;
		OutMatrix.M[1][3] = 0.0f;
		OutMatrix.M[2][3] = 0.0f;
		OutMatrix.M[3][3] = 1.0f;

		return OutMatrix;
	}

	/**
	* Convert this Transform to matrix with scaling and compute the inverse of that.
	*/
	inline Matrix ToInverseMatrixWithScale() const
	{
		// todo: optimize
		return ToMatrixWithScale().Inverse();
	}

	/**
	* Convert this Transform to inverse.
	*/
	inline FTransform Inverse() const
	{
		FQuat   InvRotation = Rotation.Inverse();
		// this used to cause NaN if Scale contained 0 
		Vector InvScale3D = GetSafeScaleReciprocal(Scale3D);
		Vector InvTranslation = InvRotation * (InvScale3D * -Translation);

		return FTransform(InvRotation, InvTranslation, InvScale3D);
	}

	/**
	* Convert this Transform to a transformation matrix, ignoring its scaling
	*/
	inline Matrix ToMatrixNoScale() const
	{
		Matrix OutMatrix;

		OutMatrix.M[3][0] = Translation.X;
		OutMatrix.M[3][1] = Translation.Y;
		OutMatrix.M[3][2] = Translation.Z;

		const float x2 = Rotation.X + Rotation.X;
		const float y2 = Rotation.Y + Rotation.Y;
		const float z2 = Rotation.Z + Rotation.Z;
		{
			const float xx2 = Rotation.X * x2;
			const float yy2 = Rotation.Y * y2;
			const float zz2 = Rotation.Z * z2;

			OutMatrix.M[0][0] = (1.0f - (yy2 + zz2));
			OutMatrix.M[1][1] = (1.0f - (xx2 + zz2));
			OutMatrix.M[2][2] = (1.0f - (xx2 + yy2));
		}
		{
			const float yz2 = Rotation.Y * z2;
			const float wx2 = Rotation.W * x2;

			OutMatrix.M[2][1] = (yz2 - wx2);
			OutMatrix.M[1][2] = (yz2 + wx2);
		}
		{
			const float xy2 = Rotation.X * y2;
			const float wz2 = Rotation.W * z2;

			OutMatrix.M[1][0] = (xy2 - wz2);
			OutMatrix.M[0][1] = (xy2 + wz2);
		}
		{
			const float xz2 = Rotation.X * z2;
			const float wy2 = Rotation.W * y2;

			OutMatrix.M[2][0] = (xz2 + wy2);
			OutMatrix.M[0][2] = (xz2 - wy2);
		}

		OutMatrix.M[0][3] = 0.0f;
		OutMatrix.M[1][3] = 0.0f;
		OutMatrix.M[2][3] = 0.0f;
		OutMatrix.M[3][3] = 1.0f;

		return OutMatrix;
	}

	/** Set this transform to the weighted blend of the supplied two transforms. */
	inline void Blend(const FTransform& Atom1, const FTransform& Atom2, float Alpha)
	{

		if (Alpha <= ZERO_ANIMWEIGHT_THRESH)
		{
			// if blend is all the way for child1, then just copy its bone atoms
			(*this) = Atom1;
		}
		else if (Alpha >= 1.f - ZERO_ANIMWEIGHT_THRESH)
		{
			// if blend is all the way for child2, then just copy its bone atoms
			(*this) = Atom2;
		}
		else
		{
			// Simple linear interpolation for translation and scale.
			Translation = Math::Lerp(Atom1.Translation, Atom2.Translation, Alpha);
			Scale3D = Math::Lerp(Atom1.Scale3D, Atom2.Scale3D, Alpha);
			Rotation = FQuat::FastLerp(Atom1.Rotation, Atom2.Rotation, Alpha);

			// ..and renormalize
			Rotation.Normalize();
		}
	}

	/** Set this Transform to the weighted blend of it and the supplied Transform. */
	inline void BlendWith(const FTransform& OtherAtom, float Alpha)
	{
		if (Alpha > ZERO_ANIMWEIGHT_THRESH)
		{
			if (Alpha >= 1.f - ZERO_ANIMWEIGHT_THRESH)
			{
				// if blend is all the way for child2, then just copy its bone atoms
				(*this) = OtherAtom;
			}
			else
			{
				// Simple linear interpolation for translation and scale.
				Translation = Math::Lerp(Translation, OtherAtom.Translation, Alpha);
				Scale3D = Math::Lerp(Scale3D, OtherAtom.Scale3D, Alpha);
				Rotation = FQuat::FastLerp(Rotation, OtherAtom.Rotation, Alpha);

				// ..and renormalize
				Rotation.Normalize();
			}
		}
	}


	/**
	* Quaternion addition is wrong here. This is just a special case for linear interpolation.
	* Use only within blends!!
	* Rotation part is NOT normalized!!
	*/
	inline FTransform operator+(const FTransform& Atom) const
	{
		return FTransform(Rotation + Atom.Rotation, Translation + Atom.Translation, Scale3D + Atom.Scale3D);
	}
	inline FTransform& operator+=(const FTransform& Atom)
	{
		Translation += Atom.Translation;

		Rotation.X += Atom.Rotation.X;
		Rotation.Y += Atom.Rotation.Y;
		Rotation.Z += Atom.Rotation.Z;
		Rotation.W += Atom.Rotation.W;

		Scale3D += Atom.Scale3D;

		DiagnosticCheckNaN_All();
		return *this;
	}
	inline FTransform operator*(float Mult) const
	{
		return FTransform(Rotation * Mult, Translation * Mult, Scale3D * Mult);
	}
	inline FTransform& operator*=(float Mult)
	{
		Translation *= Mult;
		Rotation.X *= Mult;
		Rotation.Y *= Mult;
		Rotation.Z *= Mult;
		Rotation.W *= Mult;
		Scale3D *= Mult;
		DiagnosticCheckNaN_All();

		return *this;
	}
	/**
	* Return a transform that is the result of this multiplied by another transform.
	* Order matters when composing transforms : C = A * B will yield a transform C that logically first applies A then B to any subsequent transformation.
	*
	* @param  Other other transform by which to multiply.
	* @return new transform: this * Other
	*/
	inline FTransform operator*(const FTransform& Other) const;


	/**
	* Sets this transform to the result of this multiplied by another transform.
	* Order matters when composing transforms : C = A * B will yield a transform C that logically first applies A then B to any subsequent transformation.
	*
	* @param  Other other transform by which to multiply.
	*/
	inline void operator*=(const FTransform& Other);

	/**
	* Return a transform that is the result of this multiplied by another transform (made only from a rotation).
	* Order matters when composing transforms : C = A * B will yield a transform C that logically first applies A then B to any subsequent transformation.
	*
	* @param  Other other quaternion rotation by which to multiply.
	* @return new transform: this * FTransform(Other)
	*/
	inline FTransform operator*(const FQuat& Other) const;

	/**
	* Sets this transform to the result of this multiplied by another transform (made only from a rotation).
	* Order matters when composing transforms : C = A * B will yield a transform C that logically first applies A then B to any subsequent transformation.
	*
	* @param  Other other quaternion rotation by which to multiply.
	*/
	inline void operator*=(const FQuat& Other);

	inline static bool AnyHasNegativeScale(const Vector& InScale3D, const  Vector& InOtherScale3D);
	inline void ScaleTranslation(const Vector& InScale3D);
	inline void ScaleTranslation(const float& Scale);
	inline void RemoveScaling(float Tolerance = SMALL_NUMBER);
	inline float GetMaximumAxisScale() const;
	inline float GetMinimumAxisScale() const;

	// Inverse does not work well with VQS format(in particular non-uniform), so removing it, but made two below functions to be used instead. 

	/*******************************************************************************************
	* The below 2 functions are the ones to get delta transform and return FTransform format that can be concatenated
	* Inverse itself can't concatenate with VQS format(since VQS always transform from S->Q->T, where inverse happens from T(-1)->Q(-1)->S(-1))
	* So these 2 provides ways to fix this
	* GetRelativeTransform returns this*Other(-1) and parameter is Other(not Other(-1))
	* GetRelativeTransformReverse returns this(-1)*Other, and parameter is Other.
	*******************************************************************************************/
	 FTransform GetRelativeTransform(const FTransform& Other) const;
	 FTransform GetRelativeTransformReverse(const FTransform& Other) const;
	/**
	* Set current transform and the relative to ParentTransform.
	* Equates to This = This->GetRelativeTransform(Parent), but saves the intermediate FTransform storage and copy.
	*/
	 void SetToRelativeTransform(const FTransform& ParentTransform);

	inline Vector4 TransformVector4(const Vector4& V) const;
	inline Vector4 TransformVector4NoScale(const Vector4& V) const;
	inline Vector TransformPosition(const Vector& V) const;
	inline Vector TransformPositionNoScale(const Vector& V) const;

	/** Inverts the transform and then transforms V - correctly handles scaling in this transform. */
	inline Vector InverseTransformPosition(const Vector &V) const;
	inline Vector InverseTransformPositionNoScale(const Vector &V) const;
	inline Vector TransformVector(const Vector& V) const;
	inline Vector TransformVectorNoScale(const Vector& V) const;

	/**
	*	Transform a direction vector by the inverse of this transform - will not take into account translation part.
	*	If you want to transform a surface normal (or plane) and correctly account for non-uniform scaling you should use TransformByUsingAdjointT with adjoint of matrix inverse.
	*/
	inline Vector InverseTransformVector(const Vector &V) const;
	inline Vector InverseTransformVectorNoScale(const Vector &V) const;

	/**
	* Transform a rotation.
	* For example if this is a LocalToWorld transform, TransformRotation(Q) would transform Q from local to world space.
	*/
	inline FQuat TransformRotation(const FQuat& Q) const;

	/**
	* Inverse transform a rotation.
	* For example if this is a LocalToWorld transform, InverseTransformRotation(Q) would transform Q from world to local space.
	*/
	inline FQuat InverseTransformRotation(const FQuat& Q) const;

	inline FTransform GetScaled(float Scale) const;
	inline FTransform GetScaled(Vector Scale) const;
	inline Vector GetScaledAxis(EAxis::Type InAxis) const;
	inline Vector GetUnitAxis(EAxis::Type InAxis) const;
	inline void Mirror(EAxis::Type MirrorAxis, EAxis::Type FlipAxis);
	inline static Vector GetSafeScaleReciprocal(const Vector& InScale, float Tolerance = SMALL_NUMBER);

	// temp function for easy conversion
	inline Vector GetLocation() const
	{
		return GetTranslation();
	}

	inline Rotator FRotator() const
	{
		return Rotation.FRotator();
	}

	/** Calculate the  */
	inline float GetDeterminant() const
	{
		return Scale3D.X * Scale3D.Y * Scale3D.Z;
	}

	/** Set the translation of this transformation */
	inline void SetLocation(const Vector& Origin)
	{
		Translation = Origin;
		DiagnosticCheckNaN_Translate();
	}

	/**
	* Checks the components for non-finite values (NaN or Inf).
	* @return Returns true if any component (rotation, translation, or scale) is not finite.
	*/
	bool ContainsNaN() const
	{
		return (Translation.ContainsNaN() || Rotation.ContainsNaN() || Scale3D.ContainsNaN());
	}

	inline bool IsValid() const
	{
		if (ContainsNaN())
		{
			return false;
		}

		if (!Rotation.IsNormalized())
		{
			return false;
		}

		return true;
	}

private:

	inline bool Private_RotationEquals(const FQuat& InRotation, const float Tolerance = KINDA_SMALL_NUMBER) const
	{
		return Rotation.Equals(InRotation, Tolerance);
	}

	inline bool Private_TranslationEquals(const Vector& InTranslation, const float Tolerance = KINDA_SMALL_NUMBER) const
	{
		return Translation.Equals(InTranslation, Tolerance);
	}

	inline bool Private_Scale3DEquals(const Vector& InScale3D, const float Tolerance = KINDA_SMALL_NUMBER) const
	{
		return Scale3D.Equals(InScale3D, Tolerance);
	}

public:

	// Test if A's rotation equals B's rotation, within a tolerance. Preferred over "A.GetRotation().Equals(B.GetRotation())" because it is faster on some platforms.
	inline static bool AreRotationsEqual(const FTransform& A, const FTransform& B, float Tolerance = KINDA_SMALL_NUMBER)
	{
		return A.Private_RotationEquals(B.Rotation, Tolerance);
	}

	// Test if A's translation equals B's translation, within a tolerance. Preferred over "A.GetTranslation().Equals(B.GetTranslation())" because it is faster on some platforms.
	inline static bool AreTranslationsEqual(const FTransform& A, const FTransform& B, float Tolerance = KINDA_SMALL_NUMBER)
	{
		return A.Private_TranslationEquals(B.Translation, Tolerance);
	}

	// Test if A's scale equals B's scale, within a tolerance. Preferred over "A.GetScale3D().Equals(B.GetScale3D())" because it is faster on some platforms.
	inline static bool AreScale3DsEqual(const FTransform& A, const FTransform& B, float Tolerance = KINDA_SMALL_NUMBER)
	{
		return A.Private_Scale3DEquals(B.Scale3D, Tolerance);
	}



	// Test if this Transform's rotation equals another's rotation, within a tolerance. Preferred over "GetRotation().Equals(Other.GetRotation())" because it is faster on some platforms.
	inline bool RotationEquals(const FTransform& Other, float Tolerance = KINDA_SMALL_NUMBER) const
	{
		return AreRotationsEqual(*this, Other, Tolerance);
	}

	// Test if this Transform's translation equals another's translation, within a tolerance. Preferred over "GetTranslation().Equals(Other.GetTranslation())" because it is faster on some platforms.
	inline bool TranslationEquals(const FTransform& Other, float Tolerance = KINDA_SMALL_NUMBER) const
	{
		return AreTranslationsEqual(*this, Other, Tolerance);
	}

	// Test if this Transform's scale equals another's scale, within a tolerance. Preferred over "GetScale3D().Equals(Other.GetScale3D())" because it is faster on some platforms.
	inline bool Scale3DEquals(const FTransform& Other, float Tolerance = KINDA_SMALL_NUMBER) const
	{
		return AreScale3DsEqual(*this, Other, Tolerance);
	}


	// Test if all components of the transforms are equal, within a tolerance.
	inline bool Equals(const FTransform& Other, float Tolerance = KINDA_SMALL_NUMBER) const
	{
		return Private_TranslationEquals(Other.Translation, Tolerance) && Private_RotationEquals(Other.Rotation, Tolerance) && Private_Scale3DEquals(Other.Scale3D, Tolerance);
	}

	// Test if rotation and translation components of the transforms are equal, within a tolerance.
	inline bool EqualsNoScale(const FTransform& Other, float Tolerance = KINDA_SMALL_NUMBER) const
	{
		return Private_TranslationEquals(Other.Translation, Tolerance) && Private_RotationEquals(Other.Rotation, Tolerance);
	}

	/**
	* Create a new transform: OutTransform = A * B.
	*
	* Order matters when composing transforms : A * B will yield a transform that logically first applies A then B to any subsequent transformation.
	*
	* @param  OutTransform pointer to transform that will store the result of A * B.
	* @param  A Transform A.
	* @param  B Transform B.
	*/
	inline static void Multiply(FTransform* OutTransform, const FTransform* A, const FTransform* B);

	/**
	* Sets the components
	* @param InRotation The new value for the Rotation component
	* @param InTranslation The new value for the Translation component
	* @param InScale3D The new value for the Scale3D component
	*/
	inline void SetComponents(const FQuat& InRotation, const Vector& InTranslation, const Vector& InScale3D)
	{
		Rotation = InRotation;
		Translation = InTranslation;
		Scale3D = InScale3D;

		DiagnosticCheckNaN_All();
	}

	/**
	* Sets the components to the identity transform:
	*   Rotation = (0,0,0,1)
	*   Translation = (0,0,0)
	*   Scale3D = (1,1,1)
	*/
	inline void SetIdentity()
	{
		Rotation = FQuat::Identity;
		Translation = Vector::ZeroVector;
		Scale3D = Vector(1, 1, 1);
	}

	/**
	* Scales the Scale3D component by a new factor
	* @param Scale3DMultiplier The value to multiply Scale3D with
	*/
	inline void MultiplyScale3D(const Vector& Scale3DMultiplier)
	{
		Scale3D *= Scale3DMultiplier;
		DiagnosticCheckNaN_Scale3D();
	}

	/**
	* Sets the translation component
	* @param NewTranslation The new value for the translation component
	*/
	inline void SetTranslation(const Vector& NewTranslation)
	{
		Translation = NewTranslation;
		DiagnosticCheckNaN_Translate();
	}

	/** Copy translation from another FTransform. */
	inline void CopyTranslation(const FTransform& Other)
	{
		Translation = Other.Translation;
	}

	/**
	* Concatenates another rotation to this transformation
	* @param DeltaRotation The rotation to concatenate in the following fashion: Rotation = Rotation * DeltaRotation
	*/
	inline void ConcatenateRotation(const FQuat& DeltaRotation)
	{
		Rotation = Rotation * DeltaRotation;
		DiagnosticCheckNaN_Rotate();
	}

	/**
	* Adjusts the translation component of this transformation
	* @param DeltaTranslation The translation to add in the following fashion: Translation += DeltaTranslation
	*/
	inline void AddToTranslation(const Vector& DeltaTranslation)
	{
		Translation += DeltaTranslation;
		DiagnosticCheckNaN_Translate();
	}

	/**
	* Add the translations from two FTransforms and return the result.
	* @return A.Translation + B.Translation
	*/
	inline static Vector AddTranslations(const FTransform& A, const FTransform& B)
	{
		return A.Translation + B.Translation;
	}

	/**
	* Subtract translations from two FTransforms and return the difference.
	* @return A.Translation - B.Translation.
	*/
	inline static Vector SubtractTranslations(const FTransform& A, const FTransform& B)
	{
		return A.Translation - B.Translation;
	}

	/**
	* Sets the rotation component
	* @param NewRotation The new value for the rotation component
	*/
	inline void SetRotation(const FQuat& NewRotation)
	{
		Rotation = NewRotation;
		DiagnosticCheckNaN_Rotate();
	}

	/** Copy rotation from another FTransform. */
	inline void CopyRotation(const FTransform& Other)
	{
		Rotation = Other.Rotation;
	}

	/**
	* Sets the Scale3D component
	* @param NewScale3D The new value for the Scale3D component
	*/
	inline void SetScale3D(const Vector& NewScale3D)
	{
		Scale3D = NewScale3D;
		DiagnosticCheckNaN_Scale3D();
	}

	/** Copy scale from another FTransform. */
	inline void CopyScale3D(const FTransform& Other)
	{
		Scale3D = Other.Scale3D;
	}

	/**
	* Sets both the translation and Scale3D components at the same time
	* @param NewTranslation The new value for the translation component
	* @param NewScale3D The new value for the Scale3D component
	*/
	inline void SetTranslationAndScale3D(const Vector& NewTranslation, const Vector& NewScale3D)
	{
		Translation = NewTranslation;
		Scale3D = NewScale3D;

		DiagnosticCheckNaN_Translate();
		DiagnosticCheckNaN_Scale3D();
	}

	/** @note: Added template type function for Accumulate
	* The template type isn't much useful yet, but it is with the plan to move forward
	* to unify blending features with just type of additive or full pose
	* Eventually it would be nice to just call blend and it all works depending on full pose
	* or additive, but right now that is a lot more refactoring
	* For now this types only defines the different functionality of accumulate
	*/

	/**
	* Accumulates another transform with this one
	*
	* Rotation is accumulated multiplicatively (Rotation = SourceAtom.Rotation * Rotation)
	* Translation is accumulated additively (Translation += SourceAtom.Translation)
	* Scale3D is accumulated multiplicatively (Scale3D *= SourceAtom.Scale3D)
	*
	* @param SourceAtom The other transform to accumulate into this one
	*/
	inline void Accumulate(const FTransform& SourceAtom)
	{
		// Add ref pose relative animation to base animation, only if rotation is significant.
		if (Math::Square(SourceAtom.Rotation.W) < 1.f - DELTA * DELTA)
		{
			Rotation = SourceAtom.Rotation * Rotation;
		}

		Translation += SourceAtom.Translation;
		Scale3D *= SourceAtom.Scale3D;

		DiagnosticCheckNaN_All();

		//checkSlow(IsRotationNormalized());
	}

	/** Accumulates another transform with this one, with a blending weight
	*
	* Let SourceAtom = Atom * BlendWeight
	* Rotation is accumulated multiplicatively (Rotation = SourceAtom.Rotation * Rotation).
	* Translation is accumulated additively (Translation += SourceAtom.Translation)
	* Scale3D is accumulated multiplicatively (Scale3D *= SourceAtom.Scale3D)
	*
	* Note: Rotation will not be normalized! Will have to be done manually.
	*
	* @param Atom The other transform to accumulate into this one
	* @param BlendWeight The weight to multiply Atom by before it is accumulated.
	*/
	inline void Accumulate(const FTransform& Atom, float BlendWeight/* default param doesn't work since vectorized version takes ref param */)
	{
		FTransform SourceAtom(Atom * BlendWeight);

		// Add ref pose relative animation to base animation, only if rotation is significant.
		if (Math::Square(SourceAtom.Rotation.W) < 1.f - DELTA * DELTA)
		{
			Rotation = SourceAtom.Rotation * Rotation;
		}

		Translation += SourceAtom.Translation;
		Scale3D *= SourceAtom.Scale3D;

		DiagnosticCheckNaN_All();
	}

	/**
	* Accumulates another transform with this one, with an optional blending weight
	*
	* Rotation is accumulated additively, in the shortest direction (Rotation = Rotation +/- DeltaAtom.Rotation * Weight)
	* Translation is accumulated additively (Translation += DeltaAtom.Translation * Weight)
	* Scale3D is accumulated additively (Scale3D += DeltaAtom.Scale3D * Weight)
	*
	* @param DeltaAtom The other transform to accumulate into this one
	* @param Weight The weight to multiply DeltaAtom by before it is accumulated.
	*/
	inline void AccumulateWithShortestRotation(const FTransform& DeltaAtom, float BlendWeight/* default param doesn't work since vectorized version takes ref param */)
	{
		FTransform Atom(DeltaAtom * BlendWeight);

		// To ensure the 'shortest route', we make sure the dot product between the accumulator and the incoming child atom is positive.
		if ((Atom.Rotation | Rotation) < 0.f)
		{
			Rotation.X -= Atom.Rotation.X;
			Rotation.Y -= Atom.Rotation.Y;
			Rotation.Z -= Atom.Rotation.Z;
			Rotation.W -= Atom.Rotation.W;
		}
		else
		{
			Rotation.X += Atom.Rotation.X;
			Rotation.Y += Atom.Rotation.Y;
			Rotation.Z += Atom.Rotation.Z;
			Rotation.W += Atom.Rotation.W;
		}

		Translation += Atom.Translation;
		Scale3D += Atom.Scale3D;

		DiagnosticCheckNaN_All();
	}

	/** Accumulates another transform with this one, with a blending weight
	*
	* Let SourceAtom = Atom * BlendWeight
	* Rotation is accumulated multiplicatively (Rotation = SourceAtom.Rotation * Rotation).
	* Translation is accumulated additively (Translation += SourceAtom.Translation)
	* Scale3D is accumulated assuming incoming scale is additive scale (Scale3D *= (1 + SourceAtom.Scale3D))
	*
	* When we create additive, we create additive scale based on [TargetScale/SourceScale -1]
	* because that way when you apply weight of 0.3, you don't shrink. We only saves the % of grow/shrink
	* when we apply that back to it, we add back the 1, so that it goes back to it.
	* This solves issue where you blend two additives with 0.3, you don't come back to 0.6 scale, but 1 scale at the end
	* because [1 + [1-1]*0.3 + [1-1]*0.3] becomes 1, so you don't shrink by applying additive scale
	*
	* Note: Rotation will not be normalized! Will have to be done manually.
	*
	* @param Atom The other transform to accumulate into this one
	* @param BlendWeight The weight to multiply Atom by before it is accumulated.
	*/
	inline void AccumulateWithAdditiveScale(const FTransform& Atom, float BlendWeight/* default param doesn't work since vectorized version takes ref param */)
	{
		const Vector DefaultScale(Vector::OneVector);

		FTransform SourceAtom(Atom * BlendWeight);

		// Add ref pose relative animation to base animation, only if rotation is significant.
		if (Math::Square(SourceAtom.Rotation.W) < 1.f - DELTA * DELTA)
		{
			Rotation = SourceAtom.Rotation * Rotation;
		}

		Translation += SourceAtom.Translation;
		Scale3D *= (DefaultScale + SourceAtom.Scale3D);

		DiagnosticCheckNaN_All();
	}
	/**
	* Set the translation and Scale3D components of this transform to a linearly interpolated combination of two other transforms
	*
	* Translation = Math::Lerp(SourceAtom1.Translation, SourceAtom2.Translation, Alpha)
	* Scale3D = Math::Lerp(SourceAtom1.Scale3D, SourceAtom2.Scale3D, Alpha)
	*
	* @param SourceAtom1 The starting point source atom (used 100% if Alpha is 0)
	* @param SourceAtom2 The ending point source atom (used 100% if Alpha is 1)
	* @param Alpha The blending weight between SourceAtom1 and SourceAtom2
	*/
// 	inline void LerpTranslationScale3D(const FTransform& SourceAtom1, const FTransform& SourceAtom2, ScalarRegister Alpha)
// 	{
// 		Translation = Math::Lerp(SourceAtom1.Translation, SourceAtom2.Translation, Alpha);
// 		Scale3D = Math::Lerp(SourceAtom1.Scale3D, SourceAtom2.Scale3D, Alpha);
// 
// 		DiagnosticCheckNaN_Translate();
// 		DiagnosticCheckNaN_Scale3D();
// 	}

	/**
	* Normalize the rotation component of this transformation
	*/
	inline void NormalizeRotation()
	{
		Rotation.Normalize();
		DiagnosticCheckNaN_Rotate();
	}

	/**
	* Checks whether the rotation component is normalized or not
	*
	* @return true if the rotation component is normalized, and false otherwise.
	*/
	inline bool IsRotationNormalized() const
	{
		return Rotation.IsNormalized();
	}

	/**
	* Blends the Identity transform with a weighted source transform and accumulates that into a destination transform
	*
	* SourceAtom = Blend(Identity, SourceAtom, BlendWeight)
	* FinalAtom.Rotation = SourceAtom.Rotation * FinalAtom.Rotation
	* FinalAtom.Translation += SourceAtom.Translation
	* FinalAtom.Scale3D *= SourceAtom.Scale3D
	*
	* @param FinalAtom [in/out] The atom to accumulate the blended source atom into
	* @param SourceAtom The target transformation (used when BlendWeight = 1); this is modified during the process
	* @param BlendWeight The blend weight between Identity and SourceAtom
	*/
	inline static void BlendFromIdentityAndAccumulate(FTransform& FinalAtom, FTransform& SourceAtom, float BlendWeight)
	{
		const  FTransform AdditiveIdentity(FQuat::Identity, Vector::ZeroVector, Vector::ZeroVector);
		const Vector DefaultScale(Vector::OneVector);

		// Scale delta by weight
		if (BlendWeight < (1.f - ZERO_ANIMWEIGHT_THRESH))
		{
			SourceAtom.Blend(AdditiveIdentity, SourceAtom, BlendWeight);
		}

		// Add ref pose relative animation to base animation, only if rotation is significant.
		if (Math::Square(SourceAtom.Rotation.W) < 1.f - DELTA * DELTA)
		{
			FinalAtom.Rotation = SourceAtom.Rotation * FinalAtom.Rotation;
		}

		FinalAtom.Translation += SourceAtom.Translation;
		FinalAtom.Scale3D *= (DefaultScale + SourceAtom.Scale3D);

		FinalAtom.DiagnosticCheckNaN_All();

		//checkSlow(FinalAtom.IsRotationNormalized());
	}

	/**
	* Returns the rotation component
	*
	* @return The rotation component
	*/
	inline FQuat GetRotation() const
	{
		DiagnosticCheckNaN_Rotate();
		return Rotation;
	}

	/**
	* Returns the translation component
	*
	* @return The translation component
	*/
	inline Vector GetTranslation() const
	{
		DiagnosticCheckNaN_Translate();
		return Translation;
	}

	/**
	* Returns the Scale3D component
	*
	* @return The Scale3D component
	*/
	inline Vector GetScale3D() const
	{
		DiagnosticCheckNaN_Scale3D();
		return Scale3D;
	}

	/**
	* Sets the Rotation and Scale3D of this transformation from another transform
	*
	* @param SrcBA The transform to copy rotation and Scale3D from
	*/
	inline void CopyRotationPart(const FTransform& SrcBA)
	{
		Rotation = SrcBA.Rotation;
		Scale3D = SrcBA.Scale3D;

		DiagnosticCheckNaN_Rotate();
		DiagnosticCheckNaN_Scale3D();
	}

	/**
	* Sets the Translation and Scale3D of this transformation from another transform
	*
	* @param SrcBA The transform to copy translation and Scale3D from
	*/
	inline void CopyTranslationAndScale3D(const FTransform& SrcBA)
	{
		Translation = SrcBA.Translation;
		Scale3D = SrcBA.Scale3D;

		DiagnosticCheckNaN_Translate();
		DiagnosticCheckNaN_Scale3D();
	}

	void SetFromMatrix(const Matrix& InMatrix)
	{
		Matrix M = InMatrix;

		// Get the 3D scale from the matrix
		Scale3D = M.ExtractScaling();

		// If there is negative scaling going on, we handle that here
		if (InMatrix.Determinant() < 0.f)
		{
			// Assume it is along X and modify transform accordingly. 
			// It doesn't actually matter which axis we choose, the 'appearance' will be the same
			Scale3D.X *= -1.f;
			M.SetAxis(0, -M.GetScaledAxis(EAxis::X));
		}

		Rotation = FQuat(M);
		Translation = InMatrix.GetOrigin();

		// Normalize rotation
		Rotation.Normalize();
	}

private:
	/**
	* Create a new transform: OutTransform = A * B using the matrix while keeping the scale that's given by A and B
	* Please note that this operation is a lot more expensive than normal Multiply
	*
	* Order matters when composing transforms : A * B will yield a transform that logically first applies A then B to any subsequent transformation.
	*
	* @param  OutTransform pointer to transform that will store the result of A * B.
	* @param  A Transform A.
	* @param  B Transform B.
	*/
	inline static void MultiplyUsingMatrixWithScale(FTransform* OutTransform, const FTransform* A, const FTransform* B);
	/**
	* Create a new transform from multiplications of given to matrices (AMatrix*BMatrix) using desired scale
	* This is used by MultiplyUsingMatrixWithScale and GetRelativeTransformUsingMatrixWithScale
	* This is only used to handle negative scale
	*
	* @param	AMatrix first Matrix of operation
	* @param	BMatrix second Matrix of operation
	* @param	DesiredScale - there is no check on if the magnitude is correct here. It assumes that is correct.
	* @param	OutTransform the constructed transform
	*/
	inline static void ConstructTransformFromMatrixWithDesiredScale(const Matrix& AMatrix, const Matrix& BMatrix, const Vector& DesiredScale, FTransform& OutTransform);
	/**
	* Create a new transform: OutTransform = Base * Relative(-1) using the matrix while keeping the scale that's given by Base and Relative
	* Please note that this operation is a lot more expensive than normal GetRelativeTrnasform
	*
	* @param  OutTransform pointer to transform that will store the result of Base * Relative(-1).
	* @param  BAse Transform Base.
	* @param  Relative Transform Relative.
	*/
	static void GetRelativeTransformUsingMatrixWithScale(FTransform* OutTransform, const FTransform* Base, const FTransform* Relative);
};

inline bool FTransform::AnyHasNegativeScale(const Vector& InScale3D, const  Vector& InOtherScale3D)
{
	return  (InScale3D.X < 0.f || InScale3D.Y < 0.f || InScale3D.Z < 0.f
		|| InOtherScale3D.X < 0.f || InOtherScale3D.Y < 0.f || InOtherScale3D.Z < 0.f);
}

/** Scale the translation part of the Transform by the supplied vector. */
inline void FTransform::ScaleTranslation(const Vector& InScale3D)
{
	Translation *= InScale3D;

	DiagnosticCheckNaN_Translate();
}


inline void FTransform::ScaleTranslation(const float& Scale)
{
	Translation *= Scale;

	DiagnosticCheckNaN_Translate();
}


// this function is from matrix, and all it does is to normalize rotation portion
inline void FTransform::RemoveScaling(float Tolerance/*=SMALL_NUMBER*/)
{
	Scale3D = Vector(1, 1, 1);
	Rotation.Normalize();

	DiagnosticCheckNaN_Rotate();
	DiagnosticCheckNaN_Scale3D();
}

inline void FTransform::MultiplyUsingMatrixWithScale(FTransform* OutTransform, const FTransform* A, const FTransform* B)
{
	// the goal of using M is to get the correct orientation
	// but for translation, we still need scale
	ConstructTransformFromMatrixWithDesiredScale(A->ToMatrixWithScale(), B->ToMatrixWithScale(), A->Scale3D*B->Scale3D, *OutTransform);
}

inline void FTransform::ConstructTransformFromMatrixWithDesiredScale(const Matrix& AMatrix, const Matrix& BMatrix, const Vector& DesiredScale, FTransform& OutTransform)
{
	// the goal of using M is to get the correct orientation
	// but for translation, we still need scale
	Matrix M = AMatrix * BMatrix;
	M.RemoveScaling();

	// apply negative scale back to axes
	Vector SignedScale = DesiredScale.GetSignVector();

	M.SetAxis(0, SignedScale.X * M.GetScaledAxis(EAxis::X));
	M.SetAxis(1, SignedScale.Y * M.GetScaledAxis(EAxis::Y));
	M.SetAxis(2, SignedScale.Z * M.GetScaledAxis(EAxis::Z));

	// @note: if you have negative with 0 scale, this will return rotation that is identity
	// since matrix loses that axes
	FQuat Rotation = FQuat(M);
	Rotation.Normalize();

	// set values back to output
	OutTransform.Scale3D = DesiredScale;
	OutTransform.Rotation = Rotation;

	// technically I could calculate this using FTransform but then it does more quat multiplication 
	// instead of using Scale in matrix multiplication
	// it's a question of between RemoveScaling vs using FTransform to move translation
	OutTransform.Translation = M.GetOrigin();
}

/** Returns Multiplied Transform of 2 FTransforms **/
inline void FTransform::Multiply(FTransform* OutTransform, const FTransform* A, const FTransform* B)
{
	A->DiagnosticCheckNaN_All();
	B->DiagnosticCheckNaN_All();

	//checkSlow(A->IsRotationNormalized());
	//checkSlow(B->IsRotationNormalized());

	//	When Q = quaternion, S = single scalar scale, and T = translation
	//	QST(A) = Q(A), S(A), T(A), and QST(B) = Q(B), S(B), T(B)

	//	QST (AxB) 

	// QST(A) = Q(A)*S(A)*P*-Q(A) + T(A)
	// QST(AxB) = Q(B)*S(B)*QST(A)*-Q(B) + T(B)
	// QST(AxB) = Q(B)*S(B)*[Q(A)*S(A)*P*-Q(A) + T(A)]*-Q(B) + T(B)
	// QST(AxB) = Q(B)*S(B)*Q(A)*S(A)*P*-Q(A)*-Q(B) + Q(B)*S(B)*T(A)*-Q(B) + T(B)
	// QST(AxB) = [Q(B)*Q(A)]*[S(B)*S(A)]*P*-[Q(B)*Q(A)] + Q(B)*S(B)*T(A)*-Q(B) + T(B)

	//	Q(AxB) = Q(B)*Q(A)
	//	S(AxB) = S(A)*S(B)
	//	T(AxB) = Q(B)*S(B)*T(A)*-Q(B) + T(B)

	if (AnyHasNegativeScale(A->Scale3D, B->Scale3D))
	{
		// @note, if you have 0 scale with negative, you're going to lose rotation as it can't convert back to quat
		MultiplyUsingMatrixWithScale(OutTransform, A, B);
	}
	else
	{
		OutTransform->Rotation = B->Rotation*A->Rotation;
		OutTransform->Scale3D = A->Scale3D*B->Scale3D;
		OutTransform->Translation = B->Rotation*(B->Scale3D*A->Translation) + B->Translation;
	}

	// we do not support matrix transform when non-uniform
	// that was removed at rev 21 with UE4
	OutTransform->DiagnosticCheckNaN_All();
}
/**
* Apply Scale to this transform
*/
inline FTransform FTransform::GetScaled(float InScale) const
{
	FTransform A(*this);
	A.Scale3D *= InScale;

	A.DiagnosticCheckNaN_Scale3D();

	return A;
}


/**
* Apply Scale to this transform
*/
inline FTransform FTransform::GetScaled(Vector InScale) const
{
	FTransform A(*this);
	A.Scale3D *= InScale;

	A.DiagnosticCheckNaN_Scale3D();

	return A;
}


/** Transform homogenous Vector4, ignoring the scaling part of this transform **/
inline Vector4 FTransform::TransformVector4NoScale(const Vector4& V) const
{
	DiagnosticCheckNaN_All();

	// if not, this won't work
	//checkSlow(V.W == 0.f || V.W == 1.f);

	//Transform using QST is following
	//QST(P) = Q*S*P*-Q + T where Q = quaternion, S = scale, T = translation
	Vector4 Transform = Vector4(Rotation.RotateVector(Vector(V)), 0.f);
	if (V.W == 1.f)
	{
		Transform += Vector4(Translation, 1.f);
	}

	return Transform;
}


/** Transform Vector4 **/
inline Vector4 FTransform::TransformVector4(const Vector4& V) const
{
	DiagnosticCheckNaN_All();

	// if not, this won't work
	//checkSlow(V.W == 0.f || V.W == 1.f);

	//Transform using QST is following
	//QST(P) = Q*S*P*-Q + T where Q = quaternion, S = scale, T = translation

	Vector4 Transform = Vector4(Rotation.RotateVector(Scale3D*Vector(V)), 0.f);
	if (V.W == 1.f)
	{
		Transform += Vector4(Translation, 1.f);
	}

	return Transform;
}


inline Vector FTransform::TransformPosition(const Vector& V) const
{
	DiagnosticCheckNaN_All();
	return Rotation.RotateVector(Scale3D*V) + Translation;
}


inline Vector FTransform::TransformPositionNoScale(const Vector& V) const
{
	DiagnosticCheckNaN_All();
	return Rotation.RotateVector(V) + Translation;
}


inline Vector FTransform::TransformVector(const Vector& V) const
{
	DiagnosticCheckNaN_All();
	return Rotation.RotateVector(Scale3D*V);
}


inline Vector FTransform::TransformVectorNoScale(const Vector& V) const
{
	DiagnosticCheckNaN_All();
	return Rotation.RotateVector(V);
}


// do backward operation when inverse, translation -> rotation -> scale
inline Vector FTransform::InverseTransformPosition(const Vector &V) const
{
	DiagnosticCheckNaN_All();
	return (Rotation.UnrotateVector(V - Translation)) * GetSafeScaleReciprocal(Scale3D);
}


// do backward operation when inverse, translation -> rotation
inline Vector FTransform::InverseTransformPositionNoScale(const Vector &V) const
{
	DiagnosticCheckNaN_All();
	return (Rotation.UnrotateVector(V - Translation));
}


// do backward operation when inverse, translation -> rotation -> scale
inline Vector FTransform::InverseTransformVector(const Vector &V) const
{
	DiagnosticCheckNaN_All();
	return (Rotation.UnrotateVector(V)) * GetSafeScaleReciprocal(Scale3D);
}


// do backward operation when inverse, translation -> rotation
inline Vector FTransform::InverseTransformVectorNoScale(const Vector &V) const
{
	DiagnosticCheckNaN_All();
	return (Rotation.UnrotateVector(V));
}

inline FQuat FTransform::TransformRotation(const FQuat& Q) const
{
	return GetRotation() * Q;
}

inline FQuat FTransform::InverseTransformRotation(const FQuat& Q) const
{
	return GetRotation().Inverse() * Q;
}

inline FTransform FTransform::operator*(const FTransform& Other) const
{
	FTransform Output;
	Multiply(&Output, this, &Other);
	return Output;
}


inline void FTransform::operator*=(const FTransform& Other)
{
	Multiply(this, this, &Other);
}


inline FTransform FTransform::operator*(const FQuat& Other) const
{
	FTransform Output, OtherTransform(Other, Vector::ZeroVector, Vector::OneVector);
	Multiply(&Output, this, &OtherTransform);
	return Output;
}


inline void FTransform::operator*=(const FQuat& Other)
{
	FTransform OtherTransform(Other, Vector::ZeroVector, Vector::OneVector);
	Multiply(this, this, &OtherTransform);
}


// x = 0, y = 1, z = 2
inline Vector FTransform::GetScaledAxis(EAxis::Type InAxis) const
{
	if (InAxis == EAxis::X)
	{
		return TransformVector(Vector(1.f, 0.f, 0.f));
	}
	else if (InAxis == EAxis::Y)
	{
		return TransformVector(Vector(0.f, 1.f, 0.f));
	}

	return TransformVector(Vector(0.f, 0.f, 1.f));
}


// x = 0, y = 1, z = 2
inline Vector FTransform::GetUnitAxis(EAxis::Type InAxis) const
{
	if (InAxis == EAxis::X)
	{
		return TransformVectorNoScale(Vector(1.f, 0.f, 0.f));
	}
	else if (InAxis == EAxis::Y)
	{
		return TransformVectorNoScale(Vector(0.f, 1.f, 0.f));
	}

	return TransformVectorNoScale(Vector(0.f, 0.f, 1.f));
}


inline void FTransform::Mirror(EAxis::Type MirrorAxis, EAxis::Type FlipAxis)
{
	// We do convert to Matrix for mirroring. 
	Matrix M = ToMatrixWithScale();
	M.Mirror(MirrorAxis, FlipAxis);
	SetFromMatrix(M);
}


/** same version of Matrix::GetMaximumAxisScale function **/
/** @return the maximum magnitude of all components of the 3D scale. */
inline float FTransform::GetMaximumAxisScale() const
{
	DiagnosticCheckNaN_Scale3D();
	return Scale3D.GetAbsMax();
}


/** @return the minimum magnitude of all components of the 3D scale. */
inline float FTransform::GetMinimumAxisScale() const
{
	DiagnosticCheckNaN_Scale3D();
	return Scale3D.GetAbsMin();
}


// mathematically if you have 0 scale, it should be infinite, 
// however, in practice if you have 0 scale, and relative transform doesn't make much sense 
// anymore because you should be instead of showing gigantic infinite mesh
// also returning BIG_NUMBER causes sequential NaN issues by multiplying 
// so we hardcode as 0
inline Vector FTransform::GetSafeScaleReciprocal(const Vector& InScale, float Tolerance)
{
	Vector SafeReciprocalScale;
	if (Math::Abs(InScale.X) <= Tolerance)
	{
		SafeReciprocalScale.X = 0.f;
	}
	else
	{
		SafeReciprocalScale.X = 1 / InScale.X;
	}

	if (Math::Abs(InScale.Y) <= Tolerance)
	{
		SafeReciprocalScale.Y = 0.f;
	}
	else
	{
		SafeReciprocalScale.Y = 1 / InScale.Y;
	}

	if (Math::Abs(InScale.Z) <= Tolerance)
	{
		SafeReciprocalScale.Z = 0.f;
	}
	else
	{
		SafeReciprocalScale.Z = 1 / InScale.Z;
	}

	return SafeReciprocalScale;
}