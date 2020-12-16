#pragma once

// Thresholds
#define TRANSLATION_ZEROING_THRESHOLD (0.0001f)
#define QUATERNION_ZEROING_THRESHOLD (0.0003f)
#define SCALE_ZEROING_THRESHOLD (0.000001f)

/** Size of Dummy bone used, when measuring error at an end effector which has a socket attached to it */
#define END_EFFECTOR_DUMMY_BONE_LENGTH_SOCKET	(50.f)
/** Dummy bone added to end effectors to make sure rotation doesn't get too aggressively compressed. */
#define END_EFFECTOR_DUMMY_BONE_LENGTH	(5.f)

#define Quant16BitDiv     (32767.f)
#define Quant16BitFactor  (32767.f)
#define Quant16BitOffs    (32767)

#define Quant10BitDiv     (511.f)
#define Quant10BitFactor  (511.f)
#define Quant10BitOffs    (511)

#define Quant11BitDiv     (1023.f)
#define Quant11BitFactor  (1023.f)
#define Quant11BitOffs    (1023)

class FQuatFixed48NoW
{
public:
	uint16 X;
	uint16 Y;
	uint16 Z;

	FQuatFixed48NoW()
	{}

	explicit FQuatFixed48NoW(const FQuat& Quat)
	{
		FromQuat(Quat);
	}

	void FromQuat(const FQuat& Quat)
	{
		FQuat Temp(Quat);
		if (Temp.W < 0.f)
		{
			Temp.X = -Temp.X;
			Temp.Y = -Temp.Y;
			Temp.Z = -Temp.Z;
			Temp.W = -Temp.W;
		}
		Temp.Normalize();

		X = (int32)(Temp.X * Quant16BitFactor) + Quant16BitOffs;
		Y = (int32)(Temp.Y * Quant16BitFactor) + Quant16BitOffs;
		Z = (int32)(Temp.Z * Quant16BitFactor) + Quant16BitOffs;
	}

	void ToQuat(FQuat& Out) const
	{
		const float FX = ((int32)X - (int32)Quant16BitOffs) / Quant16BitDiv;
		const float FY = ((int32)Y - (int32)Quant16BitOffs) / Quant16BitDiv;
		const float FZ = ((int32)Z - (int32)Quant16BitOffs) / Quant16BitDiv;
		const float WSquared = 1.f - FX * FX - FY * FY - FZ * FZ;

		Out.X = FX;
		Out.Y = FY;
		Out.Z = FZ;
		Out.W = WSquared > 0.f ? FMath::Sqrt(WSquared) : 0.f;
	}
};

class FQuatFixed32NoW
{
public:
	uint32 Packed;

	FQuatFixed32NoW()
	{}

	explicit FQuatFixed32NoW(const FQuat& Quat)
	{
		FromQuat(Quat);
	}

	void FromQuat(const FQuat& Quat)
	{
		FQuat Temp(Quat);
		if (Temp.W < 0.f)
		{
			Temp.X = -Temp.X;
			Temp.Y = -Temp.Y;
			Temp.Z = -Temp.Z;
			Temp.W = -Temp.W;
		}
		Temp.Normalize();

		const uint32 PackedX = (int32)(Temp.X * Quant11BitFactor) + Quant11BitOffs;
		const uint32 PackedY = (int32)(Temp.Y * Quant11BitFactor) + Quant11BitOffs;
		const uint32 PackedZ = (int32)(Temp.Z * Quant10BitFactor) + Quant10BitOffs;

		// 21-31 X, 10-20 Y, 0-9 Z.
		const uint32 XShift = 21;
		const uint32 YShift = 10;
		Packed = (PackedX << XShift) | (PackedY << YShift) | (PackedZ);
	}

	void ToQuat(FQuat& Out) const
	{
		const uint32 XShift = 21;
		const uint32 YShift = 10;
		const uint32 ZMask = 0x000003ff;
		const uint32 YMask = 0x001ffc00;
		const uint32 XMask = 0xffe00000;

		const uint32 UnpackedX = Packed >> XShift;
		const uint32 UnpackedY = (Packed & YMask) >> YShift;
		const uint32 UnpackedZ = (Packed & ZMask);

		const float X = ((int32)UnpackedX - (int32)Quant11BitOffs) / Quant11BitDiv;
		const float Y = ((int32)UnpackedY - (int32)Quant11BitOffs) / Quant11BitDiv;
		const float Z = ((int32)UnpackedZ - (int32)Quant10BitOffs) / Quant10BitDiv;
		const float WSquared = 1.f - X * X - Y * Y - Z * Z;

		Out.X = X;
		Out.Y = Y;
		Out.Z = Z;
		Out.W = WSquared > 0.f ? FMath::Sqrt(WSquared) : 0.f;
	}
};

class FQuatFloat96NoW
{
public:
	float X;
	float Y;
	float Z;

	FQuatFloat96NoW()
	{}

	explicit FQuatFloat96NoW(const FQuat& Quat)
	{
		FromQuat(Quat);
	}

	FQuatFloat96NoW(float InX, float InY, float InZ)
		: X(InX)
		, Y(InY)
		, Z(InZ)
	{}

	void FromQuat(const FQuat& Quat)
	{
		FQuat Temp(Quat);
		if (Temp.W < 0.f)
		{
			Temp.X = -Temp.X;
			Temp.Y = -Temp.Y;
			Temp.Z = -Temp.Z;
			Temp.W = -Temp.W;
		}
		Temp.Normalize();
		X = Temp.X;
		Y = Temp.Y;
		Z = Temp.Z;
	}

	void ToQuat(FQuat& Out) const
	{
		const float WSquared = 1.f - X * X - Y * Y - Z * Z;

		Out.X = X;
		Out.Y = Y;
		Out.Z = Z;
		Out.W = WSquared > 0.f ? FMath::Sqrt(WSquared) : 0.f;
	}
};



class FVectorFixed48
{
public:
	uint16 X;
	uint16 Y;
	uint16 Z;

	FVectorFixed48()
	{}

	explicit FVectorFixed48(const FVector& Vec)
	{
		FromVector(Vec);
	}

	void FromVector(const FVector& Vec)
	{
		FVector Temp(Vec / 128.0f);

		X = (int32)(Temp.X * Quant16BitFactor) + Quant16BitOffs;
		Y = (int32)(Temp.Y * Quant16BitFactor) + Quant16BitOffs;
		Z = (int32)(Temp.Z * Quant16BitFactor) + Quant16BitOffs;
	}

	void ToVector(FVector& Out) const
	{
		const float FX = ((int32)X - (int32)Quant16BitOffs) / Quant16BitDiv;
		const float FY = ((int32)Y - (int32)Quant16BitOffs) / Quant16BitDiv;
		const float FZ = ((int32)Z - (int32)Quant16BitOffs) / Quant16BitDiv;

		Out.X = FX * 128.0f;
		Out.Y = FY * 128.0f;
		Out.Z = FZ * 128.0f;
	}

};

class FVectorIntervalFixed32NoW
{
public:
	uint32 Packed;

	FVectorIntervalFixed32NoW()
	{}

	explicit FVectorIntervalFixed32NoW(const FVector& Value, const float* Mins, const float *Ranges)
	{
		FromVector(Value, Mins, Ranges);
	}

	void FromVector(const FVector& Value, const float* Mins, const float *Ranges)
	{
		FVector Temp(Value);

		Temp.X -= Mins[0];
		Temp.Y -= Mins[1];
		Temp.Z -= Mins[2];

		const uint32 PackedX = (int32)((Temp.X / Ranges[0]) * Quant10BitFactor) + Quant10BitOffs;
		const uint32 PackedY = (int32)((Temp.Y / Ranges[1]) * Quant11BitFactor) + Quant11BitOffs;
		const uint32 PackedZ = (int32)((Temp.Z / Ranges[2]) * Quant11BitFactor) + Quant11BitOffs;

		// 21-31 Z, 10-20 Y, 0-9 X.
		const uint32 ZShift = 21;
		const uint32 YShift = 10;
		Packed = (PackedZ << ZShift) | (PackedY << YShift) | (PackedX);
	}

	void ToVector(FVector& Out, const float* Mins, const float *Ranges) const
	{
		const uint32 ZShift = 21;
		const uint32 YShift = 10;
		const uint32 XMask = 0x000003ff;
		const uint32 YMask = 0x001ffc00;
		const uint32 ZMask = 0xffe00000;

		const uint32 UnpackedZ = Packed >> ZShift;
		const uint32 UnpackedY = (Packed & YMask) >> YShift;
		const uint32 UnpackedX = (Packed & XMask);

		const float X = ((((int32)UnpackedX - (int32)Quant10BitOffs) / Quant10BitDiv) * Ranges[0] + Mins[0]);
		const float Y = ((((int32)UnpackedY - (int32)Quant11BitOffs) / Quant11BitDiv) * Ranges[1] + Mins[1]);
		const float Z = ((((int32)UnpackedZ - (int32)Quant11BitOffs) / Quant11BitDiv) * Ranges[2] + Mins[2]);

		Out.X = X;
		Out.Y = Y;
		Out.Z = Z;
	}
};


class FQuatIntervalFixed32NoW
{
public:
	uint32 Packed;

	FQuatIntervalFixed32NoW()
	{}

	explicit FQuatIntervalFixed32NoW(const FQuat& Quat, const float* Mins, const float *Ranges)
	{
		FromQuat(Quat, Mins, Ranges);
	}

	void FromQuat(const FQuat& Quat, const float* Mins, const float *Ranges)
	{
		FQuat Temp(Quat);
		if (Temp.W < 0.f)
		{
			Temp.X = -Temp.X;
			Temp.Y = -Temp.Y;
			Temp.Z = -Temp.Z;
			Temp.W = -Temp.W;
		}
		Temp.Normalize();

		Temp.X -= Mins[0];
		Temp.Y -= Mins[1];
		Temp.Z -= Mins[2];

		const uint32 PackedX = (int32)((Temp.X / Ranges[0]) * Quant11BitFactor) + Quant11BitOffs;
		const uint32 PackedY = (int32)((Temp.Y / Ranges[1]) * Quant11BitFactor) + Quant11BitOffs;
		const uint32 PackedZ = (int32)((Temp.Z / Ranges[2]) * Quant10BitFactor) + Quant10BitOffs;

		// 21-31 X, 10-20 Y, 0-9 Z.
		const uint32 XShift = 21;
		const uint32 YShift = 10;
		Packed = (PackedX << XShift) | (PackedY << YShift) | (PackedZ);
	}

	void ToQuat(FQuat& Out, const float* Mins, const float *Ranges) const
	{
		const uint32 XShift = 21;
		const uint32 YShift = 10;
		const uint32 ZMask = 0x000003ff;
		const uint32 YMask = 0x001ffc00;
		const uint32 XMask = 0xffe00000;

		const uint32 UnpackedX = Packed >> XShift;
		const uint32 UnpackedY = (Packed & YMask) >> YShift;
		const uint32 UnpackedZ = (Packed & ZMask);

		const float X = ((((int32)UnpackedX - (int32)Quant11BitOffs) / Quant11BitDiv) * Ranges[0] + Mins[0]);
		const float Y = ((((int32)UnpackedY - (int32)Quant11BitOffs) / Quant11BitDiv) * Ranges[1] + Mins[1]);
		const float Z = ((((int32)UnpackedZ - (int32)Quant10BitOffs) / Quant10BitDiv) * Ranges[2] + Mins[2]);
		const float WSquared = 1.f - X * X - Y * Y - Z * Z;

		Out.X = X;
		Out.Y = Y;
		Out.Z = Z;
		Out.W = WSquared > 0.f ? FMath::Sqrt(WSquared) : 0.f;
	}

};

class FQuatFloat32NoW
{
public:
	uint32 Packed;

	FQuatFloat32NoW()
	{}

	explicit FQuatFloat32NoW(const FQuat& Quat)
	{
		FromQuat(Quat);
	}

	void FromQuat(const FQuat& Quat)
	{
		FQuat Temp(Quat);
		if (Temp.W < 0.f)
		{
			Temp.X = -Temp.X;
			Temp.Y = -Temp.Y;
			Temp.Z = -Temp.Z;
			Temp.W = -Temp.W;
		}
		Temp.Normalize();

		TFloatPacker<3, 7, true> Packer7e3;
		TFloatPacker<3, 6, true> Packer6e3;

		const uint32 PackedX = Packer7e3.Encode(Temp.X);
		const uint32 PackedY = Packer7e3.Encode(Temp.Y);
		const uint32 PackedZ = Packer6e3.Encode(Temp.Z);

		// 21-31 X, 10-20 Y, 0-9 Z.
		const uint32 XShift = 21;
		const uint32 YShift = 10;
		Packed = (PackedX << XShift) | (PackedY << YShift) | (PackedZ);
	}

	void ToQuat(FQuat& Out) const
	{
		const uint32 XShift = 21;
		const uint32 YShift = 10;
		const uint32 ZMask = 0x000003ff;
		const uint32 YMask = 0x001ffc00;
		const uint32 XMask = 0xffe00000;

		const uint32 UnpackedX = Packed >> XShift;
		const uint32 UnpackedY = (Packed & YMask) >> YShift;
		const uint32 UnpackedZ = (Packed & ZMask);

		TFloatPacker<3, 7, true> Packer7e3;
		TFloatPacker<3, 6, true> Packer6e3;

		const float X = Packer7e3.Decode(UnpackedX);
		const float Y = Packer7e3.Decode(UnpackedY);
		const float Z = Packer6e3.Decode(UnpackedZ);
		const float WSquared = 1.f - X * X - Y * Y - Z * Z;

		Out.X = X;
		Out.Y = Y;
		Out.Z = Z;
		Out.W = WSquared > 0.f ? FMath::Sqrt(WSquared) : 0.f;
	}

};

template <int32 FORMAT>
inline void DecompressRotation(FQuat& Out, const uint8* __restrict TopOfStream, const uint8* __restrict KeyData)
{
	// this if-else stack gets compiled away to a single result based on the template parameter
	if (FORMAT == ACF_None)
	{
		// due to alignment issue, this crahses accessing non aligned 
		// we don't think this is common case, so this is slower. 
		float* Keys = (float*)KeyData;
		Out = FQuat(Keys[0], Keys[1], Keys[2], Keys[3]);
	}
	else if (FORMAT == ACF_Float96NoW)
	{
		((FQuatFloat96NoW*)KeyData)->ToQuat(Out);
	}
	else if (FORMAT == ACF_Fixed32NoW)
	{
		((FQuatFixed32NoW*)KeyData)->ToQuat(Out);
	}
	else if (FORMAT == ACF_Fixed48NoW)
	{
		((FQuatFixed48NoW*)KeyData)->ToQuat(Out);
	}
	else if (FORMAT == ACF_IntervalFixed32NoW)
	{
		const float* __restrict Mins = (float*)TopOfStream;
		const float* __restrict Ranges = (float*)(TopOfStream + sizeof(float) * 3);
		((FQuatIntervalFixed32NoW*)KeyData)->ToQuat(Out, Mins, Ranges);
	}
	else if (FORMAT == ACF_Float32NoW)
	{
		((FQuatFloat32NoW*)KeyData)->ToQuat(Out);
	}
	else if (FORMAT == ACF_Identity)
	{
		Out = FQuat::Identity;
	}
	else
	{
		//UE_LOG(LogAnimation, Fatal, TEXT("%i: unknown or unsupported animation compression format"), (int32)FORMAT);
		Out = FQuat::Identity;
	}
}

template <int32 FORMAT>
inline void DecompressTranslation(FVector& Out, const uint8* __restrict TopOfStream, const uint8* __restrict KeyData)
{
	if ((FORMAT == ACF_None) || (FORMAT == ACF_Float96NoW))
	{
		Out = *((FVector*)KeyData);
	}
	else if (FORMAT == ACF_IntervalFixed32NoW)
	{
		const float* __restrict Mins = (float*)TopOfStream;
		const float* __restrict Ranges = (float*)(TopOfStream + sizeof(float) * 3);
		((FVectorIntervalFixed32NoW*)KeyData)->ToVector(Out, Mins, Ranges);
	}
	else if (FORMAT == ACF_Identity)
	{
		Out = FVector::ZeroVector;
	}
	else if (FORMAT == ACF_Fixed48NoW)
	{
		((FVectorFixed48*)KeyData)->ToVector(Out);
	}
	else
	{
		//UE_LOG(LogAnimation, Fatal, TEXT("%i: unknown or unsupported animation compression format"), (int32)FORMAT);
		// Silence compilers warning about a value potentially not being assigned.
		Out = FVector::ZeroVector;
	}
}

template <int32 FORMAT>
inline void DecompressScale(FVector& Out, const uint8* __restrict TopOfStream, const uint8* __restrict KeyData)
{
	if ((FORMAT == ACF_None) || (FORMAT == ACF_Float96NoW))
	{
		Out = *((FVector*)KeyData);
	}
	else if (FORMAT == ACF_IntervalFixed32NoW)
	{
		const float* __restrict Mins = (float*)TopOfStream;
		const float* __restrict Ranges = (float*)(TopOfStream + sizeof(float) * 3);
		((FVectorIntervalFixed32NoW*)KeyData)->ToVector(Out, Mins, Ranges);
	}
	else if (FORMAT == ACF_Identity)
	{
		Out = FVector::ZeroVector;
	}
	else if (FORMAT == ACF_Fixed48NoW)
	{
		((FVectorFixed48*)KeyData)->ToVector(Out);
	}
	else
	{
		//UE_LOG(LogAnimation, Fatal, TEXT("%i: unknown or unsupported animation compression format"), (int32)FORMAT);
		// Silence compilers warning about a value potentially not being assigned.
		Out = FVector::ZeroVector;
	}
}