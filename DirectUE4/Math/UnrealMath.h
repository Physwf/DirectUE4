#pragma once

#include <initializer_list>
#include <cassert>
#include <cmath>
#include <memory>

typedef	signed char			int8;	
typedef signed short int	int16;	
typedef signed int	 		int32;	
typedef signed long long	int64;		

typedef unsigned char		uint8;
typedef unsigned short		uint16;
typedef unsigned int		uint32;
typedef unsigned long long	uint64;

#define INDEX_NONE -1

#undef  PI
#define PI 					(3.1415926535897932f)
#define SMALL_NUMBER		(1.e-8f)
#define KINDA_SMALL_NUMBER	(1.e-4f)
#define BIG_NUMBER			(3.4e+38f)
#define EULERS_NUMBER       (2.71828182845904523536f)

// Copied from float.h
#define MAX_FLT 3.402823466e+38F

// Aux constants.
#define INV_PI			(0.31830988618f)
#define HALF_PI			(1.57079632679f)

#define DELTA			(0.00001f)
/**
* Lengths of normalized vectors (These are half their maximum values
* to assure that dot products with normalized vectors don't overflow).
*/
#define FLOAT_NORMAL_THRESH				(0.0001f)

//
// Magic numbers for numerical precision.
//
#define THRESH_POINT_ON_PLANE			(0.10f)		/* Thickness of plane for front/back/inside test */
#define THRESH_POINT_ON_SIDE			(0.20f)		/* Thickness of polygon side's side-plane for point-inside/outside/on side test */
#define THRESH_POINTS_ARE_SAME			(0.00002f)	/* Two points are same if within this distance */
#define THRESH_POINTS_ARE_NEAR			(0.015f)	/* Two points are near if within this distance and can be combined if imprecise math is ok */
#define THRESH_NORMALS_ARE_SAME			(0.00002f)	/* Two normal points are same if within this distance */
#define THRESH_UVS_ARE_SAME			    (0.0009765625f)/* Two UV are same if within this threshold (1.0f/1024f) */
/* Making this too large results in incorrect CSG classification and disaster */
#define THRESH_VECTORS_ARE_NEAR			(0.0004f)	/* Two vectors are near if within this distance and can be combined if imprecise math is ok */
/* Making this too large results in lighting problems due to inaccurate texture coordinates */
#define THRESH_SPLIT_POLY_WITH_PLANE	(0.25f)		/* A plane splits a polygon in half */
#define THRESH_SPLIT_POLY_PRECISELY		(0.01f)		/* A plane exactly splits a polygon */
#define THRESH_ZERO_NORM_SQUARED		(0.0001f)	/* Size of a unit normal that is considered "zero", squared */
#define THRESH_NORMALS_ARE_PARALLEL		(0.999845f)	/* Two unit vectors are parallel if abs(A dot B) is greater than or equal to this. This is roughly cosine(1.0 degrees). */
#define THRESH_NORMALS_ARE_ORTHOGONAL	(0.017455f)	/* Two unit vectors are orthogonal (perpendicular) if abs(A dot B) is less than or equal this. This is roughly cosine(89.0 degrees). */

#define THRESH_VECTOR_NORMALIZED		(0.01f)		/** Allowed error for a normalized vector (against squared magnitude) */
#define THRESH_QUAT_NORMALIZED			(0.01f)		/** Allowed error for a normalized quaternion (against squared magnitude) */
/**
*	float4 vector register type, where the first float (X) is stored in the lowest 32 bits, and so on.
*/

#define ZERO_ANIMWEIGHT_THRESH (0.00001f)

struct FRotator;
struct FQuat;
struct Vector4;
struct FVector;
struct Vector2;
struct FBox;
class FSphere;
struct FTransform;
struct FPlane;

namespace EAxis
{
	enum Type
	{
		None,
		X,
		Y,
		Z,
	};
}

class FMath
{
public:
	template<typename T>
	static void Swap(T&A, T& B)
	{
		T Temp = A;
		A = B;
		B = Temp;
	}

	template< class T >
	static constexpr inline T Max(const T A, const T B)
	{
		return (A >= B) ? A : B;
	}

	/** Returns lower value in a generic way */
	template< class T >
	static constexpr inline T Min(const T A, const T B)
	{
		return (A <= B) ? A : B;
	}
	/** Returns lowest of 3 values */
	template< class T >
	static inline T Min3(const T A, const T B, const T C)
	{
		return Min(Min(A, B), C);
	}
	/** Returns highest of 3 values */
	template< class T >
	static inline T Max3(const T A, const T B, const T C)
	{
		return Max(Max(A, B), C);
	}
	/** Clamps X to be between Min and Max, inclusive */
	template< class T >
	static inline T Clamp(const T X, const T Min, const T Max)
	{
		return X < Min ? Min : X < Max ? X : Max;
	}
	/** Multiples value by itself */
	template< class T >
	static inline T Square(const T A)
	{
		return A * A;
	}
	/** Performs a linear interpolation between two values, Alpha ranges from 0-1 */
	template< class T, class U >
	static inline T Lerp(const T& A, const T& B, const U& Alpha)
	{
		return (T)(A + Alpha * (B - A));
	}
	/** Performs a 2D linear interpolation between four values values, FracX, FracY ranges from 0-1 */
	template< class T, class U >
	static inline T BiLerp(const T& P00, const T& P10, const T& P01, const T& P11, const U& FracX, const U& FracY)
	{
		return Lerp(
			Lerp(P00, P10, FracX),
			Lerp(P01, P11, FracX),
			FracY
		);
	}
	template< class T, class U >
	static inline T CubicInterp(const T& P0, const T& T0, const T& P1, const T& T1, const U& A)
	{
		const float A2 = A * A;
		const float A3 = A2 * A;

		return (T)(((2 * A3) - (3 * A2) + 1) * P0) + ((A3 - (2 * A2) + A) * T0) + ((A3 - A2) * T1) + (((-2 * A3) + (3 * A2)) * P1);
	}
	template< class U > static FRotator Lerp(const FRotator& A, const FRotator& B, const U& Alpha);
	template< class U > static FRotator LerpRange(const FRotator& A, const FRotator& B, const U& Alpha);
	template< class U > static FQuat Lerp(const FQuat& A, const FQuat& B, const U& Alpha);
	template< class U > static FQuat BiLerp(const FQuat& P00, const FQuat& P10, const FQuat& P01, const FQuat& P11, float FracX, float FracY);
	template< class U > static FQuat CubicInterp(const FQuat& P0, const FQuat& T0, const FQuat& P1, const FQuat& T1, const U& A);
	static constexpr inline int32 TruncToInt(float F)
	{
		return (int32)F;
	}
	static inline int32 CeilToInt(float F)
	{
		return TruncToInt(ceilf(F));
	}
	static inline int32 FloorToInt(float F)
	{
		return TruncToInt(floorf(F));
	}
	static inline int32 RoundToInt(float F)
	{
		return FloorToInt(F + 0.5f);
	}
	static inline float CeilToFloat(float F)
	{
		// Note: the x2 is to workaround the rounding-to-nearest-even-number issue when the fraction is .5
		return (float)CeilToInt(F);
	}
	static constexpr inline float TruncToFloat(float F)
	{
		return (float)TruncToInt(F);
	}
	static inline float FloorToFloat(float F)
	{
		return (float)FloorToInt(F);
	}
	static inline float Fractional(float Value)
	{
		return Value - TruncToFloat(Value);
	}
	/** Computes absolute value in a generic way */
	template< class T >
	static constexpr inline T Abs(const T A)
	{
		return (A >= (T)0) ? A : -A;
	}
	/** Divides two integers and rounds up */
	template <class T>
	static inline T DivideAndRoundUp(T Dividend, T Divisor)
	{
		return (Dividend + Divisor - 1) / Divisor;
	}

	/** Divides two integers and rounds down */
	template <class T>
	static inline T DivideAndRoundDown(T Dividend, T Divisor)
	{
		return Dividend / Divisor;
	}
	/** Divides two integers and rounds to nearest */
	template <class T>
	static inline T DivideAndRoundNearest(T Dividend, T Divisor)
	{
		return (Dividend >= 0)
			? (Dividend + Divisor / 2) / Divisor
			: (Dividend - Divisor / 2 + 1) / Divisor;
	}
	static inline float Loge(float Value) { return logf(Value); }
	static inline float Log2(float Value)
	{
		// Cached value for fast conversions
		static const float LogToLog2 = 1.f / Loge(2.f);
		// Do the platform specific log and convert using the cached value
		return Loge(Value) * LogToLog2;
	}
	template<class T>
	static inline auto DegreesToRadians(T const& DegVal) -> decltype(DegVal * (PI / 180.f))
	{
		return DegVal * (PI / 180.f);
	}
	static inline void SinCos(float* ScalarSin, float* ScalarCos, float  Value)
	{
		// Map Value to y in [-pi,pi], x = 2*pi*quotient + remainder.
		float quotient = (INV_PI*0.5f)*Value;
		if (Value >= 0.0f)
		{
			quotient = (float)((int)(quotient + 0.5f));
		}
		else
		{
			quotient = (float)((int)(quotient - 0.5f));
		}
		float y = Value - (2.0f*PI)*quotient;

		// Map y to [-pi/2,pi/2] with sin(y) = sin(Value).
		float sign;
		if (y > HALF_PI)
		{
			y = PI - y;
			sign = -1.0f;
		}
		else if (y < -HALF_PI)
		{
			y = -PI - y;
			sign = -1.0f;
		}
		else
		{
			sign = +1.0f;
		}

		float y2 = y * y;

		// 11-degree minimax approximation
		*ScalarSin = (((((-2.3889859e-08f * y2 + 2.7525562e-06f) * y2 - 0.00019840874f) * y2 + 0.0083333310f) * y2 - 0.16666667f) * y2 + 1.0f) * y;

		// 10-degree minimax approximation
		float p = ((((-2.6051615e-07f * y2 + 2.4760495e-05f) * y2 - 0.0013888378f) * y2 + 0.041666638f) * y2 - 0.5f) * y2 + 1.0f;
		*ScalarCos = sign * p;
	}
	static inline float Sin(float Value) { return sinf(Value); }
	static inline float Asin(float Value) { return asinf((Value < -1.f) ? -1.f : ((Value < 1.f) ? Value : 1.f)); }
	static inline float Sinh(float Value) { return sinhf(Value); }
	static inline float Cos(float Value) { return cosf(Value); }
	static inline float Acos(float Value) { return acosf((Value < -1.f) ? -1.f : ((Value < 1.f) ? Value : 1.f)); }
	static inline float Tan(float Value) { return tanf(Value); }
	static inline float Atan(float Value) { return atanf(Value); }
	static float Atan2(float Y, float X);
	static inline float Sqrt(float Value) { return sqrtf(Value); }
	static inline float Pow(float A, float B) { return powf(A, B); }
	/** Computes a fully accurate inverse square root */
	static inline float InvSqrt(float F) { 	return 1.0f / sqrtf(F); }
	/** Computes a faster but less accurate inverse square root */
	static inline float InvSqrtEst(float F) { return InvSqrt(F); }
	/** Return true if value is NaN (not a number). */
	static inline bool IsNaN(float A) { return ((*(uint32*)&A) & 0x7FFFFFFF) > 0x7F800000; }
	/** Return true if value is finite (not NaN and not Infinity). */
	static inline bool IsFinite(float A) { return ((*(uint32*)&A) & 0x7F800000) != 0x7F800000; }
	static inline bool IsNegativeFloat(const float& A) { return ((*(uint32*)&A) >= (uint32)0x80000000);  /*Detects sign bit.*/ }
	static inline bool IsNegativeDouble(const double& A) { return ((*(uint64*)&A) >= (uint64)0x8000000000000000); /*Detects sign bit.*/ }
	/** Returns a random integer between 0 and RAND_MAX, inclusive */
	static inline int32 Rand() { return rand(); }
	/** Seeds global random number functions Rand() and FRand() */
	static inline void RandInit(int32 Seed) { srand(Seed); }
	static constexpr inline double FloatSelect(double Comparand, double ValueGEZero, double ValueLTZero)
	{
		return Comparand >= 0.f ? ValueGEZero : ValueLTZero;
	}
	static inline float Fmod(float X, float Y)
	{
		if (fabsf(Y) <= 1.e-8f)
		{
			//FmodReportError(X, Y);
			return 0.f;
		}
		const float Quotient = TruncToFloat(X / Y);
		float IntPortion = Y * Quotient;

		// Rounding and imprecision could cause IntPortion to exceed X and cause the result to be outside the expected range.
		// For example Fmod(55.8, 9.3) would result in a very small negative value!
		if (fabsf(IntPortion) > fabsf(X))
		{
			IntPortion = X;
		}

		const float Result = X - IntPortion;
		return Result;
	}
	static inline float GridSnap(float Location, float Grid)
	{
		if (Grid == 0.f)	return Location;
		else
		{
			return FloorToFloat((Location + 0.5f*Grid) / Grid)*Grid;
		}
	}
#define FASTASIN_HALF_PI (1.5707963050f)
	/**
	* Computes the ASin of a scalar float.
	*
	* @param Value  input angle
	* @return ASin of Value
	*/
	static inline float FastAsin(float Value)
	{
		// Clamp input to [-1,1].
		bool nonnegative = (Value >= 0.0f);
		float x = FMath::Abs(Value);
		float omx = 1.0f - x;
		if (omx < 0.0f)
		{
			omx = 0.0f;
		}
		float root = FMath::Sqrt(omx);
		// 7-degree minimax approximation
		float result = ((((((-0.0012624911f * x + 0.0066700901f) * x - 0.0170881256f) * x + 0.0308918810f) * x - 0.0501743046f) * x + 0.0889789874f) * x - 0.2145988016f) * x + FASTASIN_HALF_PI;
		result *= root;  // acos(|x|)
						 // acos(x) = pi - acos(-x) when x < 0, asin(x) = pi/2 - acos(x)
		return (nonnegative ? FASTASIN_HALF_PI - result : result - FASTASIN_HALF_PI);
	}
#undef FASTASIN_HALF_PI
	static bool SphereAABBIntersection(const FVector& SphereCenter, const float RadiusSquared, const FBox& AABB);
	static bool SphereAABBIntersection(const FSphere& Sphere, const FBox& AABB);
	static FSphere ComputeBoundingSphereForCone(FVector const& ConeOrigin, FVector const& ConeDirection, float ConeRadius, float CosConeAngle, float SinConeAngle);
	static bool PointBoxIntersection(const FVector& Point, const FBox& Box);
	static bool LineBoxIntersection(const FBox& Box, const FVector& Start, const FVector& End, const FVector& Direction);
	static bool LineBoxIntersection(const FBox& Box, const FVector& Start, const FVector& End, const FVector& Direction, const FVector& OneOverDirection);
	static inline uint32 FloorLog2(uint32 Value)
	{
		uint32 pos = 0;
		if (Value >= 1 << 16) { Value >>= 16; pos += 16; }
		if (Value >= 1 << 8) { Value >>= 8; pos += 8; }
		if (Value >= 1 << 4) { Value >>= 4; pos += 4; }
		if (Value >= 1 << 2) { Value >>= 2; pos += 2; }
		if (Value >= 1 << 1) { pos += 1; }
		return (Value == 0) ? 0 : pos;
	}
	static inline uint32 CountLeadingZeros(uint32 Value)
	{
		if (Value == 0) return 32;
		return 31 - FloorLog2(Value);
	}
	static inline uint32 CeilLogTwo(uint32 Arg)
	{
		int32 Bitmask = ((int32)(CountLeadingZeros(Arg) << 26)) >> 31;
		return (32 - CountLeadingZeros(Arg - 1)) & (~Bitmask);
	}
	static FVector LinePlaneIntersection(const FVector &Point1, const FVector &Point2, const FPlane  &Plane);
	static inline float Exp(float Value) { return expf(Value); }
	static inline float Exp2(float Value) { return powf(2.f, Value); /*exp2f(Value);*/ }
};


template <typename T>
inline void Exchange(T& A, T& B)
{
	FMath::Swap(A, B);
}

struct FVector;

struct alignas(16) Vector4
{
	float X, Y, Z, W;

	Vector4(): X(0.0f), Y(0.0f), Z(0.0f), W(0.0f) {}
	explicit Vector4(float V) : X(V), Y(V), Z(V), W(V) {}
	Vector4(const FVector& V,float InW = 1.f);
	Vector4(float InX, float InY, float InZ, float InW = 1.0f) : X(InX), Y(InY), Z(InZ), W(InW) {}
	Vector4(std::initializer_list<float> list);

	Vector4 operator+(const Vector4& rhs) const;
	Vector4 operator-(const Vector4& rhs) const;
	Vector4 operator*(const Vector4& rhs) const;
	Vector4 operator*(float Value) const;
	Vector4 operator/(float Value) const;
	Vector4 operator^(const Vector4& rhs) const;
	Vector4 operator+=(const Vector4& rhs);
	Vector4 operator-=(const Vector4& rhs);
	Vector4 operator*=(const Vector4& rhs);
	Vector4 operator*=(float Value);
	Vector4 operator/=(float Value);
	Vector4 operator-() const;

	bool operator==(const Vector4& rhs) const;
	bool operator!=(const Vector4& rhs) const;

	float& operator[](int32 Index);
	float operator[](int32 Index) const;

	friend inline float Dot3(const Vector4& lhs, const Vector4& rhs)
	{
		return lhs.X * rhs.X + lhs.Y * rhs.Y + lhs.Z * rhs.Z;
	}
	friend inline float Dot4(const Vector4& lhs, const Vector4& rhs)
	{
		return lhs.X * rhs.X + lhs.Y * rhs.Y + lhs.Z * rhs.Z + lhs.W * rhs.W;
	}
	friend inline Vector4 operator*(float Value, const Vector4& rhs)
	{
		return rhs.operator*(Value);
	}

	Vector4 GetSafeNormal(float Tolerance = SMALL_NUMBER) const;
	inline float Vector4::SizeSquared3() const
	{
		return X * X + Y * Y + Z * Z;
	}
	inline float Vector4::Size() const
	{
		return FMath::Sqrt(X*X + Y * Y + Z * Z + W * W);
	}
	inline float Vector4::SizeSquared() const
	{
		return X * X + Y * Y + Z * Z + W * W;
	}

};

inline Vector4::Vector4(std::initializer_list<float> list)
{
	int32 i = 0;
	for (auto&& v : list)
	{
		(*this)[i] = v;
		if(i++ > 3) break;
	}
}

inline Vector4 Vector4::operator+(const Vector4& rhs) const
{
	return { X + rhs.X, Y + rhs.Y, Z + rhs.Z, W + rhs.W };
}

inline Vector4 Vector4::operator-(const Vector4& rhs) const
{
	return { X - rhs.X, Y - rhs.Y, Z - rhs.Z, W - rhs.W };
}

inline Vector4 Vector4::operator-() const
{
	return { -X,-Y,-Z,-W };
}

inline float& Vector4::operator[](int32 Index)
{
	return (&X)[Index];
}

inline float Vector4::operator[](int32 Index) const
{
	return (&X)[Index];
}

inline Vector4 Vector4::operator*(const Vector4& rhs) const
{
	return { X * rhs.X, Y * rhs.Y, Z * rhs.Z, W * rhs.W };
}

inline Vector4 Vector4::operator*(float Value) const
{
	return { X * Value, Y * Value, Z * Value, W * Value };
}

inline Vector4 Vector4::operator/(float Value) const
{
	return { X / Value, Y / Value, Z / Value, W / Value };
}

inline Vector4 Vector4::operator^(const Vector4& rhs) const
{
	return 
	{
		Y * rhs.Z - Z * rhs.Y,
		Z * rhs.X - X * rhs.Z,
		X * rhs.Y - Y * rhs.X,
		0.0f
	};
}

inline Vector4 Vector4::operator+=(const Vector4& rhs)
{
	X += rhs.X; Y += rhs.Y; Z += rhs.Z; W += rhs.W;
	return *this;
}

inline Vector4 Vector4::operator-=(const Vector4& rhs)
{
	X -= rhs.X; Y -= rhs.Y; Z -= rhs.Z; W -= rhs.W;
	return *this;
}


inline Vector4 Vector4::operator*=(const Vector4& rhs)
{
	X *= rhs.X; Y *= rhs.Y; Z *= rhs.Z; W *= rhs.W;
	return *this;
}

inline Vector4 Vector4::operator*=(float Value)
{
	X *= Value; Y *= Value; Z *= Value; W *= Value;
	return *this;
}

inline Vector4 Vector4::operator/=(float Value)
{
	X /= Value; Y /= Value; Z /= Value; W /= Value;
	return *this;
}

inline bool Vector4::operator==(const Vector4& rhs) const
{
	return (X == rhs.X && Y == rhs.Y && Z == rhs.Z && W == rhs.W);
}

inline bool Vector4::operator!=(const Vector4& rhs) const
{
	return (X != rhs.X || Y != rhs.Y || Z != rhs.Z || W != rhs.W);
}


struct FVector
{
	float X, Y, Z;

	static const FVector ZeroVector;
	static const FVector OneVector;
	static const FVector UpVector;
	static const FVector ForwardVector;
	static const FVector RightVector;

	FVector() : X(0.0f), Y(0.0f), Z(0.0f) {}
	explicit FVector(float V) : X(V), Y(V), Z(V) {}
	FVector(float InX, float InY, float InZ) : X(InX), Y(InY), Z(InZ) {}
	FVector(std::initializer_list<float> list);
	FVector(const Vector4& V) : X(V.X), Y(V.Y), Z(V.Z) {};

	FVector operator+(const FVector& rhs) const;
	FVector operator-(const FVector& rhs) const;
	FVector operator*(const FVector& rhs) const;
	FVector operator+(float Bias) const;
	FVector operator*(float Value) const;
	FVector operator/(float Value) const;
	FVector operator^(const FVector& rhs) const;
	static FVector CrossProduct(const FVector& A, const FVector& B);
	float operator|(const FVector& V) const;
	FVector operator+=(const FVector& rhs);
	FVector operator-=(const FVector& rhs);
	FVector operator*=(const FVector& rhs);
	FVector operator*=(float Value);
	FVector operator/=(float Value);
	FVector operator-() const;

	bool operator==(const FVector& rhs) const;
	bool operator!=(const FVector& rhs) const;

	float& operator[](int32 Index);
	float operator[](int32 Index) const;

	float SizeSquared() const;
	inline void Normalize();
	static void CreateOrthonormalBasis(FVector& XAxis, FVector& YAxis, FVector& ZAxis);
	static float Triple(const FVector& X, const FVector& Y, const FVector& Z);
	friend inline FVector operator*(float Value, const FVector& rhs)
	{
		return rhs.operator*(Value);
	}

	bool IsNearlyZero(float Tolerance = KINDA_SMALL_NUMBER) const;
	FVector GetSafeNormal(float Tolerance = SMALL_NUMBER) const;
	bool Equals(const FVector& V, float Tolerance = KINDA_SMALL_NUMBER) const;
	struct Vector2 ToVector2() const;

	float Size() const;
	FVector GetSignVector() const;
	float GetMax() const;
	float GetAbsMax() const;
	float GetMin() const;
	float GetAbsMin() const;
	inline bool FVector::ContainsNaN() const;
	bool IsZero() const;
	FVector Reciprocal() const;
	FVector GetUnsafeNormal() const;
	FVector RotateAngleAxis(const float AngleDeg, const FVector& Axis) const;
	FRotator Rotation() const;
	FRotator ToOrientationRotator() const;
	void FindBestAxisVectors(FVector& Axis1, FVector& Axis2) const;
};

float ComputeSquaredDistanceFromBoxToPoint(const FVector& Mins, const FVector& Maxs, const FVector& Point);

inline FVector::FVector(std::initializer_list<float> list)
{
	int32 i = 0;
	for (auto&& v : list)
	{
		(*this)[i] = v;
		if (i++ > 2) break;
	}
}
inline bool FVector::ContainsNaN() const
{
	return (!FMath::IsFinite(X) || !FMath::IsFinite(Y) || !FMath::IsFinite(Z));
}
inline FVector FVector::operator+(const FVector& rhs) const
{
	return { X + rhs.X, Y + rhs.Y, Z + rhs.Z };
}

inline FVector FVector::operator-(const FVector& rhs) const
{
	return { X - rhs.X, Y - rhs.Y, Z - rhs.Z };
}

inline FVector FVector::operator*(const FVector& rhs) const
{
	return { X * rhs.X, Y * rhs.Y, Z * rhs.Z };
}

inline FVector FVector::operator*(float Value) const
{
	return { X * Value, Y * Value, Z * Value };
}

inline FVector FVector::operator/(float Value) const
{
	return { X / Value, Y / Value, Z / Value };
}

inline FVector FVector::operator+=(const FVector& rhs)
{
	X += rhs.X; Y += rhs.Y; Z += rhs.Z;;
	return *this;
}

inline FVector FVector::operator-=(const FVector& rhs)
{
	X -= rhs.X; Y -= rhs.Y; Z -= rhs.Z;
	return *this;
}

inline FVector FVector::operator*=(const FVector& rhs)
{
	X *= rhs.X; Y *= rhs.Y; Z *= rhs.Z;;
	return *this;
}
inline FVector FVector::operator+(float Bias) const
{
	return FVector(X + Bias, Y + Bias, Z + Bias);
}
inline FVector FVector::operator*=(float Value)
{
	X *= Value; Y *= Value; Z *= Value;
	return *this;
}

inline FVector FVector::operator/=(float Value)
{
	X /= Value; Y /= Value; Z /= Value;
	return *this;
}

inline FVector FVector::operator-() const
{
	return { -X,-Y,-Z };
}

inline bool FVector::operator!=(const FVector& rhs) const
{
	return (X != rhs.X || Y != rhs.Y || Z != rhs.Z);
}

inline bool FVector::operator==(const FVector& rhs) const
{
	return (X == rhs.X && Y == rhs.Y && Z == rhs.Z);
}


inline FVector FVector::operator^(const FVector& rhs) const
{
	return
	{
		Y * rhs.Z - Z * rhs.Y,
		Z * rhs.X - X * rhs.Z,
		X * rhs.Y - Y * rhs.X,
	};
}
inline FVector FVector::CrossProduct(const FVector& A, const FVector& B)
{
	return A ^ B;
}
inline float FVector::operator|(const FVector& V) const
{
	return X * V.X + Y * V.Y + Z * V.Z;
}

inline float& FVector::operator[](int32 Index)
{
	return (&X)[Index];
}

inline float FVector::operator[](int32 Index) const
{
	return (&X)[Index];
}

inline void FVector::Normalize()
{
	float InvSqrt = 1.0f / sqrtf(X * X + Y * Y + Z * Z);
	X *= InvSqrt;
	Y *= InvSqrt;
	Z *= InvSqrt;
}

inline bool FVector::IsNearlyZero(float Tolerance /*= KINDA_SMALL_NUMBER*/) const
{
	return std::abs(X) <= Tolerance && std::abs(Y) <= Tolerance && std::abs(Z) <= Tolerance;
}

inline FVector FVector::GetSafeNormal(float Tolerance /*= SMALL_NUMBER*/) const
{
	const float SquareSum = X * X + Y * Y + Z * Z;
	if (SquareSum == 1.f)
	{
		return *this;
	}
	else if (SquareSum < Tolerance)
	{
		return FVector(0.f);
	}
	const float Scale = 1.f / std::sqrt(SquareSum);
	return FVector(X*Scale, Y*Scale, Z*Scale);
}
inline FVector FVector::GetSignVector() const
{
	return FVector
	(
		(float)FMath::FloatSelect(X, 1.f, -1.f),
		(float)FMath::FloatSelect(Y, 1.f, -1.f),
		(float)FMath::FloatSelect(Z, 1.f, -1.f)
	);
}
inline float FVector::GetMax() const
{
	return FMath::Max(FMath::Max(X, Y), Z);
}
inline float FVector::GetAbsMax() const
{
	return FMath::Max(FMath::Max(FMath::Abs(X), FMath::Abs(Y)), FMath::Abs(Z));
}
inline float FVector::GetMin() const
{
	return FMath::Min(FMath::Min(X, Y), Z);
}
inline float FVector::GetAbsMin() const
{
	return FMath::Min(FMath::Min(FMath::Abs(X), FMath::Abs(Y)), FMath::Abs(Z));
}
struct FRotator
{
public:
	/** Rotation around the right axis (around Y axis), Looking up and down (0=Straight Ahead, +Up, -Down) */
	float Pitch;
	/** Rotation around the up axis (around Z axis), Running in circles 0=East, +North, -South. */
	float Yaw;
	/** Rotation around the forward axis (around X axis), Tilting your head, 0=Straight, +Clockwise, -CCW. */
	float Roll;

	static const FRotator ZeroRotator;
public:
	inline FRotator() { }
	explicit inline FRotator(float InF);
	inline FRotator(float InPitch, float InYaw, float InRoll);
	explicit  FRotator(const FQuat& Quat);
public:
	FRotator operator+(const FRotator& R) const;
	FRotator operator-(const FRotator& R) const;
	FRotator operator*(float Scale) const;
	FRotator operator*=(float Scale);
	bool operator==(const FRotator& R) const;
	bool operator!=(const FRotator& V) const;
	FRotator operator+=(const FRotator& R);
	FRotator operator-=(const FRotator& R);
public:
	bool IsNearlyZero(float Tolerance = KINDA_SMALL_NUMBER) const;
	bool IsZero() const;
	bool Equals(const FRotator& R, float Tolerance = KINDA_SMALL_NUMBER) const;
	FRotator Add(float DeltaPitch, float DeltaYaw, float DeltaRoll);
	FRotator GetInverse() const;
	FRotator GridSnap(const FRotator& RotGrid) const;
	FVector Vector() const;
	FQuat Quaternion() const;
	FVector Euler() const;
	FVector RotateVector(const FVector& V) const;
	FVector UnrotateVector(const FVector& V) const;
	FRotator Clamp() const;
	FRotator GetNormalized() const;
	FRotator GetDenormalized() const;
	float GetComponentForAxis(EAxis::Type Axis) const;
	void SetComponentForAxis(EAxis::Type Axis, float Component);
	void Normalize();
	void GetWindingAndRemainder(FRotator& Winding, FRotator& Remainder) const;
	float GetManhattanDistance(const FRotator & R) const;
	FRotator GetEquivalentRotator() const;
	void SetClosestToMe(FRotator& MakeClosest) const;
	bool ContainsNaN() const;
public:
	static float ClampAxis(float Angle);
	static float NormalizeAxis(float Angle);
	static uint8 CompressAxisToByte(float Angle);
	static float DecompressAxisFromByte(uint8 Angle);
	static uint16 CompressAxisToShort(float Angle);
	static float DecompressAxisFromShort(uint16 Angle);
	static FRotator MakeFromEuler(const FVector& Euler);
};
inline FRotator operator*(float Scale, const FRotator& R)
{
	return R.operator*(Scale);
}
inline FRotator::FRotator(float InF)
	: Pitch(InF), Yaw(InF), Roll(InF)
{
	//DiagnosticCheckNaN();
}
inline FRotator::FRotator(float InPitch, float InYaw, float InRoll)
	: Pitch(InPitch), Yaw(InYaw), Roll(InRoll)
{
	//DiagnosticCheckNaN();
}
inline FRotator FRotator::operator+(const FRotator& R) const
{
	return FRotator(Pitch + R.Pitch, Yaw + R.Yaw, Roll + R.Roll);
}
inline FRotator FRotator::operator-(const FRotator& R) const
{
	return FRotator(Pitch - R.Pitch, Yaw - R.Yaw, Roll - R.Roll);
}
inline FRotator FRotator::operator*(float Scale) const
{
	return FRotator(Pitch*Scale, Yaw*Scale, Roll*Scale);
}
inline FRotator FRotator::operator*= (float Scale)
{
	Pitch = Pitch * Scale; Yaw = Yaw * Scale; Roll = Roll * Scale;
	//DiagnosticCheckNaN();
	return *this;
}
inline bool FRotator::operator==(const FRotator& R) const
{
	return Pitch == R.Pitch && Yaw == R.Yaw && Roll == R.Roll;
}
inline bool FRotator::operator!=(const FRotator& V) const
{
	return Pitch != V.Pitch || Yaw != V.Yaw || Roll != V.Roll;
}
inline FRotator FRotator::operator+=(const FRotator& R)
{
	Pitch += R.Pitch; Yaw += R.Yaw; Roll += R.Roll;
	//DiagnosticCheckNaN();
	return *this;
}
inline FRotator FRotator::operator-=(const FRotator& R)
{
	Pitch -= R.Pitch; Yaw -= R.Yaw; Roll -= R.Roll;
	//DiagnosticCheckNaN();
	return *this;
}
inline bool FRotator::IsNearlyZero(float Tolerance) const
{
	return
		FMath::Abs(NormalizeAxis(Pitch)) <= Tolerance
		&& FMath::Abs(NormalizeAxis(Yaw)) <= Tolerance
		&& FMath::Abs(NormalizeAxis(Roll)) <= Tolerance;
}
inline bool FRotator::IsZero() const
{
	return (ClampAxis(Pitch) == 0.f) && (ClampAxis(Yaw) == 0.f) && (ClampAxis(Roll) == 0.f);
}
inline bool FRotator::Equals(const FRotator& R, float Tolerance) const
{
	return (FMath::Abs(NormalizeAxis(Pitch - R.Pitch)) <= Tolerance)
		&& (FMath::Abs(NormalizeAxis(Yaw - R.Yaw)) <= Tolerance)
		&& (FMath::Abs(NormalizeAxis(Roll - R.Roll)) <= Tolerance);
}
inline FRotator FRotator::Add(float DeltaPitch, float DeltaYaw, float DeltaRoll)
{
	Yaw += DeltaYaw;
	Pitch += DeltaPitch;
	Roll += DeltaRoll;
	//DiagnosticCheckNaN();
	return *this;
}
inline FRotator FRotator::GridSnap(const FRotator& RotGrid) const
{
	return FRotator
	(
		FMath::GridSnap(Pitch, RotGrid.Pitch),
		FMath::GridSnap(Yaw, RotGrid.Yaw),
		FMath::GridSnap(Roll, RotGrid.Roll)
	);
}
inline FRotator FRotator::Clamp() const
{
	return FRotator(ClampAxis(Pitch), ClampAxis(Yaw), ClampAxis(Roll));
}
inline float FRotator::ClampAxis(float Angle)
{
	// returns Angle in the range (-360,360)
	Angle = FMath::Fmod(Angle, 360.f);

	if (Angle < 0.f)
	{
		// shift to [0,360) range
		Angle += 360.f;
	}

	return Angle;
}
inline float FRotator::NormalizeAxis(float Angle)
{
	// returns Angle in the range [0,360)
	Angle = ClampAxis(Angle);

	if (Angle > 180.f)
	{
		// shift to (-180,180]
		Angle -= 360.f;
	}

	return Angle;
}
inline uint8 FRotator::CompressAxisToByte(float Angle)
{
	// map [0->360) to [0->256) and mask off any winding
	return FMath::RoundToInt(Angle * 256.f / 360.f) & 0xFF;
}
inline float FRotator::DecompressAxisFromByte(uint8 Angle)
{
	// map [0->256) to [0->360)
	return (Angle * 360.f / 256.f);
}
inline uint16 FRotator::CompressAxisToShort(float Angle)
{
	// map [0->360) to [0->65536) and mask off any winding
	return FMath::RoundToInt(Angle * 65536.f / 360.f) & 0xFFFF;
}
inline float FRotator::DecompressAxisFromShort(uint16 Angle)
{
	// map [0->65536) to [0->360)
	return (Angle * 360.f / 65536.f);
}
inline FRotator FRotator::GetNormalized() const
{
	FRotator Rot = *this;
	Rot.Normalize();
	return Rot;
}
inline FRotator FRotator::GetDenormalized() const
{
	FRotator Rot = *this;
	Rot.Pitch = ClampAxis(Rot.Pitch);
	Rot.Yaw = ClampAxis(Rot.Yaw);
	Rot.Roll = ClampAxis(Rot.Roll);
	return Rot;
}
inline void FRotator::Normalize()
{
	Pitch = NormalizeAxis(Pitch);
	Yaw = NormalizeAxis(Yaw);
	Roll = NormalizeAxis(Roll);
	//DiagnosticCheckNaN();
}
inline float FRotator::GetComponentForAxis(EAxis::Type Axis) const
{
	switch (Axis)
	{
	case EAxis::X:
		return Roll;
	case EAxis::Y:
		return Pitch;
	case EAxis::Z:
		return Yaw;
	default:
		return 0.f;
	}
}
inline void FRotator::SetComponentForAxis(EAxis::Type Axis, float Component)
{
	switch (Axis)
	{
	case EAxis::X:
		Roll = Component;
		break;
	case EAxis::Y:
		Pitch = Component;
		break;
	case EAxis::Z:
		Yaw = Component;
		break;
	}
}
inline bool FRotator::ContainsNaN() const
{
	return (!FMath::IsFinite(Pitch) ||
		!FMath::IsFinite(Yaw) ||
		!FMath::IsFinite(Roll));
}

inline float FRotator::GetManhattanDistance(const FRotator & Rotator) const
{
	return FMath::Abs<float>(Yaw - Rotator.Yaw) + FMath::Abs<float>(Pitch - Rotator.Pitch) + FMath::Abs<float>(Roll - Rotator.Roll);
}

inline FRotator FRotator::GetEquivalentRotator() const
{
	return FRotator(180.0f - Pitch, Yaw + 180.0f, Roll + 180.0f);
}
inline void FRotator::SetClosestToMe(FRotator& MakeClosest) const
{
	FRotator OtherChoice = MakeClosest.GetEquivalentRotator();
	float FirstDiff = GetManhattanDistance(MakeClosest);
	float SecondDiff = GetManhattanDistance(OtherChoice);
	if (SecondDiff < FirstDiff)
		MakeClosest = OtherChoice;
}

/* Math inline functions
*****************************************************************************/

template<class U>
inline FRotator FMath::Lerp(const FRotator& A, const FRotator& B, const U& Alpha)
{
	return A + (B - A).GetNormalized() * Alpha;
}

template<class U>
inline FRotator FMath::LerpRange(const FRotator& A, const FRotator& B, const U& Alpha)
{
	// Similar to Lerp, but does not take the shortest path. Allows interpolation over more than 180 degrees.
	return (A * (1 - Alpha) + B * Alpha).GetNormalized();
}


inline Vector4::Vector4(const FVector& V, float InW/* = 1.f*/) : X(V.X), Y(V.Y), Z(V.Z), W(InW)
{

}

struct Vector2
{
	float X, Y;

	static const Vector2 ZeroVector;
	static const Vector2 UnitVector;

	Vector2() : X(0.0f), Y(0.0f) {}
	explicit Vector2(float V) : X(V), Y(V) {}
	Vector2(float InX, float InY) : X(InX), Y(InY) {}
	Vector2(std::initializer_list<float> list);

	Vector2 operator+(const Vector2& rhs) const;
	Vector2 operator-(const Vector2& rhs) const;
	Vector2 operator*(const Vector2& rhs) const;
	Vector2 operator*(float Value) const;
	Vector2 operator/(float Value) const;
	float operator^(const Vector2& rhs) const;
	float operator|(const Vector2& V) const;
	Vector2 operator+=(const Vector2& rhs);
	Vector2 operator-=(const Vector2& rhs);
	Vector2 operator*=(const Vector2& rhs);
	Vector2 operator*=(float Value);
	Vector2 operator/=(float Value);
	Vector2 operator-() const;

	bool operator==(const Vector2& rhs) const;
	bool operator!=(const Vector2& rhs) const;

	float& operator[](int32 Index);
	float operator[](int32 Index) const;

	inline Vector2 operator+(float A) const
	{
		return Vector2(X + A, Y + A);
	}
	inline Vector2 operator-(float A) const
	{
		return Vector2(X - A, Y - A);
	}
	void Normalize();
	float Size();
	float SizeSquared();

	bool Equals(const Vector2& V, float Tolerance = KINDA_SMALL_NUMBER) const;

	friend inline Vector2 operator*(float Value, const Vector2& rhs)
	{
		return rhs.operator*(Value);
	}

	bool IsNearlyZero(float Tolerance = KINDA_SMALL_NUMBER) const;
};


inline Vector2::Vector2(std::initializer_list<float> list)
{
	int32 i = 0;
	for (auto&& v : list)
	{
		(*this)[i] = v;
		if (i++ > 1) break;
	}
}

inline Vector2 Vector2::operator+(const Vector2& rhs) const
{
	return { X + rhs.X, Y + rhs.Y };
}

inline Vector2 Vector2::operator-(const Vector2& rhs) const
{
	return { X - rhs.X, Y - rhs.Y };
}

inline Vector2 Vector2::operator*(const Vector2& rhs) const
{
	return { X * rhs.X, Y * rhs.Y };
}

inline Vector2 Vector2::operator*(float Value) const
{
	return { X * Value, Y * Value };
}

inline Vector2 Vector2::operator/(float Value) const
{
	return { X / Value, Y / Value};
}

inline Vector2 Vector2::operator+=(const Vector2& rhs)
{
	X += rhs.X; Y += rhs.Y;
	return *this;
}

inline Vector2 Vector2::operator-=(const Vector2& rhs)
{
	X -= rhs.X; Y -= rhs.Y;
	return *this;
}

inline Vector2 Vector2::operator*=(const Vector2& rhs)
{
	X *= rhs.X; Y *= rhs.Y;
	return *this;
}

inline Vector2 Vector2::operator*=(float Value)
{
	X *= Value; Y *= Value;
	return *this;
}

inline Vector2 Vector2::operator/=(float Value)
{
	X /= Value; Y /= Value;
	return *this;
}

inline Vector2 Vector2::operator-() const
{
	return { -X,-Y };
}

inline bool Vector2::operator!=(const Vector2& rhs) const
{
	return (X != rhs.X || Y != rhs.Y);
}

inline bool Vector2::operator==(const Vector2& rhs) const
{
	return (X == rhs.X && Y == rhs.Y);
}


inline float Vector2::operator^(const Vector2& rhs) const
{
	return X * rhs.Y - Y * rhs.X;
}

inline float Vector2::operator|(const Vector2& V) const
{
	return X * V.X + Y * V.Y;
}

inline float& Vector2::operator[](int32 Index)
{
	return (&X)[Index];
}

inline float Vector2::operator[](int32 Index) const
{
	return (&X)[Index];
}

inline void Vector2::Normalize()
{
	float InvSqrt = 1.0f / sqrtf(X * X + Y * Y);
	X *= InvSqrt;
	Y *= InvSqrt;
}

inline float Vector2::Size()
{
	return sqrtf(X * X + Y * Y);
}

inline float Vector2::SizeSquared()
{
	return X * X + Y * Y;
}


struct FLinearColor
{
	float R, G, B, A;

	static double sRGBToLinearTable[256];

	FLinearColor() {}
	FLinearColor(float InR, float InG, float InB, float InA = 1.0f) : R(InR), G(InG), B(InB), A(InA) {}
	FLinearColor(struct FColor);

	static const FLinearColor White;
	static const FLinearColor Gray;
	static const FLinearColor Black;
	static const FLinearColor Transparent;
	static const FLinearColor Red;
	static const FLinearColor Green;
	static const FLinearColor Blue;
	static const FLinearColor Yellow;
};

struct FColor
{
	union { struct { uint8 B, G, R, A; }; uint32 AlignmentDummy; };
	uint32& DWColor(void) { return *((uint32*)this); }
	const uint32& DWColor(void) const { return *((uint32*)this); }

	FColor() {}
	static const FColor White;
	FColor(uint8 InR, uint8 InG, uint8 InB, uint8 InA = 255)
	{
		// put these into the body for proper ordering with INTEL vs non-INTEL_BYTE_ORDER
		R = InR;
		G = InG;
		B = InB;
		A = InA;
	}

	explicit FColor(uint32 InColor)
	{
		DWColor() = InColor;
	}
	bool operator==(const FColor &C) const
	{
		return DWColor() == C.DWColor();
	}

	bool operator!=(const FColor& C) const
	{
		return DWColor() != C.DWColor();
	}


};

struct alignas(16) FPlane : public FVector
{
public:
	float W;

	FPlane() {};

	inline FPlane(float InX, float InY, float InZ, float InW) : FVector(InX, InY, InZ)
		, W(InW)
	{}
	inline FPlane(FVector InNormal, float InW) : FVector(InNormal), W(InW)
	{}
	FPlane(FVector InBase, const FVector &InNormal);
	FPlane operator-(const FPlane& V) const;
	FPlane operator*(float Scale) const;

	float PlaneDot(const FVector &P) const;

	FPlane Flip() const;
};


inline void VectorMatrixInverse(void* DstMatrix, const void* SrcMatrix);

struct FMatrix
{
	union
	{
		alignas(16) float M[4][4];
	};
	static alignas(16) const FMatrix Identity;
	FMatrix() {}
	FMatrix(const FPlane& InX, const FPlane& InY, const FPlane& InZ, const FPlane& InW);
	FMatrix(const FVector& InX, const FVector& InY, const FVector& InZ, const FVector& InW);
	void SetIndentity();
	void Transpose();
	FMatrix GetTransposed() const;
	inline FMatrix Inverse() const;
	inline FVector GetScaledAxis(EAxis::Type Axis) const;
	inline float Determinant() const;
	FMatrix operator* (const FMatrix& Other) const;
	void operator*=(const FMatrix& Other);
	FMatrix operator+ (const FMatrix& Other) const;
	void operator+=(const FMatrix& Other);
	FMatrix operator* (float Other) const;
	void operator*=(float Other);
	 bool operator==(const FMatrix& Other) const;
	inline bool operator!=(const FMatrix& Other) const;
	FVector Transform(const FVector& InVector) const;
	FVector GetUnitAxis(EAxis::Type Axis) const;
	inline FMatrix InverseFast() const
	{
		FMatrix Result;
		VectorMatrixInverse(&Result, this);
		return Result;
	}
	inline FVector GetOrigin() const { return FVector(M[3][0], M[3][1], M[3][2]); }
	inline Vector4 TransformFVector4(const Vector4& V) const;
	Vector4 TransformVector(const FVector& V) const;

	inline Vector4 TransformPosition(const FVector &V) const
	{
		return TransformFVector4(Vector4(V.X, V.Y, V.Z, 1.0f));
	}

	inline FVector InverseTransformPosition(const FVector &V) const
	{
		FMatrix InvSelf = this->InverseFast();
		return InvSelf.TransformPosition(V);
	}
	inline void Mirror(EAxis::Type MirrorAxis, EAxis::Type FlipAxis);
	inline FMatrix RemoveTranslation() const
	{
		FMatrix Result = *this;
		Result.M[3][0] = 0.0f;
		Result.M[3][1] = 0.0f;
		Result.M[3][2] = 0.0f;
		return Result;
	}
	void			RemoveScaling(float Tolerance = SMALL_NUMBER);
	inline FVector	ExtractScaling(float Tolerance = SMALL_NUMBER);
	void			SetAxis(int32 i, const FVector& Axis);

	bool GetFrustumNearPlane(FPlane& OutPlane) const;
	bool GetFrustumFarPlane(FPlane& OutPlane) const;
	bool GetFrustumLeftPlane(FPlane& OutPlane) const;
	bool GetFrustumRightPlane(FPlane& OutPlane) const;
	bool GetFrustumTopPlane(FPlane& OutPlane) const;
	bool GetFrustumBottomPlane(FPlane& OutPlane) const;
	FVector GetColumn(int32 i) const;

	static FMatrix	FromScale(float Scale);
	static FMatrix	DXFromPitch(float fPitch);
	static FMatrix	DXFromYaw(float fYaw);
	static FMatrix	DXFromRoll(float fRoll);
	static FMatrix	DXFormRotation(FRotator Rotation);
	static FMatrix	DXFromTranslation(FVector Translation);
	static FMatrix	DXLookAtLH(const FVector& Eye,const FVector& LookAtPosition, const FVector& Up);
	static FMatrix	DXLookToLH(const FVector& To);
	static FMatrix	DXFromPerspectiveFovLH(float fieldOfViewY, float aspectRatio, float znearPlane, float zfarPlane);
	static FMatrix	DXReversedZFromPerspectiveFovLH(float fieldOfViewY, float aspectRatio, float znearPlane, float zfarPlane);
	static FMatrix	DXFromPerspectiveLH(float w, float h, float zn, float zf);
	static FMatrix	DXFromOrthognalLH(float w, float h, float zn, float zf);
	static FMatrix	DXFromOrthognalLH(float right, float left, float top, float bottom,float zFar, float zNear);
};


class FTranslationMatrix : public FMatrix
{
public:
	FTranslationMatrix(const FVector& Delta);
};
class FPerspectiveMatrix : public FMatrix
{
public:
	// Note: the value of this must match the mirror in Common.usf!
#define Z_PRECISION	0.0f
	/**
	* Constructor
	*
	* @param HalfFOVX Half FOV in the X axis
	* @param HalfFOVY Half FOV in the Y axis
	* @param MultFOVX multiplier on the X axis
	* @param MultFOVY multiplier on the y axis
	* @param MinZ distance to the near Z plane
	* @param MaxZ distance to the far Z plane
	*/
	FPerspectiveMatrix(float HalfFOVX, float HalfFOVY, float MultFOVX, float MultFOVY, float MinZ, float MaxZ);

	/**
	* Constructor
	*
	* @param HalfFOV half Field of View in the Y direction
	* @param Width view space width
	* @param Height view space height
	* @param MinZ distance to the near Z plane
	* @param MaxZ distance to the far Z plane
	* @note that the FOV you pass in is actually half the FOV, unlike most perspective matrix functions (D3DXMatrixPerspectiveFovLH).
	*/
	FPerspectiveMatrix(float HalfFOV, float Width, float Height, float MinZ, float MaxZ);

	/**
	* Constructor
	*
	* @param HalfFOV half Field of View in the Y direction
	* @param Width view space width
	* @param Height view space height
	* @param MinZ distance to the near Z plane
	* @note that the FOV you pass in is actually half the FOV, unlike most perspective matrix functions (D3DXMatrixPerspectiveFovLH).
	*/
	FPerspectiveMatrix(float HalfFOV, float Width, float Height, float MinZ);
};

class FReversedZPerspectiveMatrix : public FMatrix
{
public:
	FReversedZPerspectiveMatrix(float HalfFOVX, float HalfFOVY, float MultFOVX, float MultFOVY, float MinZ, float MaxZ);
	FReversedZPerspectiveMatrix(float HalfFOV, float Width, float Height, float MinZ, float MaxZ);
	FReversedZPerspectiveMatrix(float HalfFOV, float Width, float Height, float MinZ);
};

struct FBasisVectorMatrix : FMatrix
{
	// Create Basis matrix from 3 axis vectors and the origin
	FBasisVectorMatrix(const FVector& XAxis, const FVector& YAxis, const FVector& ZAxis, const FVector& Origin);
};


struct FLookAtMatrix : FMatrix
{
	/**
	* Creates a view matrix given an eye position, a position to look at, and an up vector.
	* This does the same thing as D3DXMatrixLookAtLH.
	*/
	FLookAtMatrix(const FVector& EyePosition, const FVector& LookAtPosition, const FVector& UpVector);
};


class FScaleMatrix : public FMatrix
{
public:
	FScaleMatrix(float Scale);
	FScaleMatrix(const FVector& Scale);
	static FMatrix Make(float Scale)
	{
		return FScaleMatrix(Scale);
	}
	static FMatrix Make(const FVector& Scale)
	{
		return FScaleMatrix(Scale);
	}
};
inline FScaleMatrix::FScaleMatrix(float Scale)
	: FMatrix(
		FPlane(Scale, 0.0f, 0.0f, 0.0f),
		FPlane(0.0f, Scale, 0.0f, 0.0f),
		FPlane(0.0f, 0.0f, Scale, 0.0f),
		FPlane(0.0f, 0.0f, 0.0f, 1.0f)
	)
{ }

inline FScaleMatrix::FScaleMatrix(const FVector& Scale)
	: FMatrix(
		FPlane(Scale.X, 0.0f, 0.0f, 0.0f),
		FPlane(0.0f, Scale.Y, 0.0f, 0.0f),
		FPlane(0.0f, 0.0f, Scale.Z, 0.0f),
		FPlane(0.0f, 0.0f, 0.0f, 1.0f)
	)
{ }
class FRotationTranslationMatrix
	: public FMatrix
{
public:
	FRotationTranslationMatrix(const FRotator& Rot, const FVector& Origin);
	static FMatrix Make(const FRotator& Rot, const FVector& Origin)
	{
		return FRotationTranslationMatrix(Rot, Origin);
	}
};

class FRotationMatrix
	: public FRotationTranslationMatrix
{
public:
	FRotationMatrix(const FRotator& Rot)
		: FRotationTranslationMatrix(Rot, FVector::ZeroVector)
	{ }
	/** Matrix factory. Return an FMatrix so we don't have type conversion issues in expressions. */
	static FMatrix Make(FRotator const& Rot)
	{
		return FRotationMatrix(Rot);
	}
	/** Matrix factory. Return an FMatrix so we don't have type conversion issues in expressions. */
	static FMatrix Make(FQuat const& Rot);
	/** Builds a rotation matrix given only a XAxis. Y and Z are unspecified but will be orthonormal. XAxis need not be normalized. */
	static FMatrix MakeFromX(FVector const& XAxis);
	/** Builds a rotation matrix given only a YAxis. X and Z are unspecified but will be orthonormal. YAxis need not be normalized. */
	static FMatrix MakeFromY(FVector const& YAxis);
	/** Builds a rotation matrix given only a ZAxis. X and Y are unspecified but will be orthonormal. ZAxis need not be normalized. */
	static FMatrix MakeFromZ(FVector const& ZAxis);
	/** Builds a matrix with given X and Y axes. X will remain fixed, Y may be changed minimally to enforce orthogonality. Z will be computed. Inputs need not be normalized. */
	static FMatrix MakeFromXY(FVector const& XAxis, FVector const& YAxis);
	/** Builds a matrix with given X and Z axes. X will remain fixed, Z may be changed minimally to enforce orthogonality. Y will be computed. Inputs need not be normalized. */
	static FMatrix MakeFromXZ(FVector const& XAxis, FVector const& ZAxis);
	/** Builds a matrix with given Y and X axes. Y will remain fixed, X may be changed minimally to enforce orthogonality. Z will be computed. Inputs need not be normalized. */
	static FMatrix MakeFromYX(FVector const& YAxis, FVector const& XAxis);
	/** Builds a matrix with given Y and Z axes. Y will remain fixed, Z may be changed minimally to enforce orthogonality. X will be computed. Inputs need not be normalized. */
	static FMatrix MakeFromYZ(FVector const& YAxis, FVector const& ZAxis);
	/** Builds a matrix with given Z and X axes. Z will remain fixed, X may be changed minimally to enforce orthogonality. Y will be computed. Inputs need not be normalized. */
	static FMatrix MakeFromZX(FVector const& ZAxis, FVector const& XAxis);
	/** Builds a matrix with given Z and Y axes. Z will remain fixed, Y may be changed minimally to enforce orthogonality. X will be computed. Inputs need not be normalized. */
	static FMatrix MakeFromZY(FVector const& ZAxis, FVector const& YAxis);
};

inline FTranslationMatrix::FTranslationMatrix(const FVector& Delta):
	FMatrix(
		FPlane(1.0f, 0.0f, 0.0f, 0.0f),
		FPlane(0.0f, 1.0f, 0.0f, 0.0f),
		FPlane(0.0f, 0.0f, 1.0f, 0.0f),
		FPlane(Delta.X, Delta.Y, Delta.Z, 1.0f)
	)
{ }

inline void FMatrix::SetIndentity()
{
	M[0][0] = 1; M[0][1] = 0;  M[0][2] = 0;  M[0][3] = 0;
	M[1][0] = 0; M[1][1] = 1;  M[1][2] = 0;  M[1][3] = 0;
	M[2][0] = 0; M[2][1] = 0;  M[2][2] = 1;  M[2][3] = 0;
	M[3][0] = 0; M[3][1] = 0;  M[3][2] = 0;  M[3][3] = 1;
}


inline void FMatrix::Transpose()
{
	FMath::Swap(M[0][1], M[1][0]);
	FMath::Swap(M[0][2], M[2][0]);
	FMath::Swap(M[0][3], M[3][0]);
	FMath::Swap(M[2][1], M[1][2]);
	FMath::Swap(M[3][1], M[1][3]);
	FMath::Swap(M[3][2], M[2][3]);
}

FMatrix FMatrix::Inverse() const
{
	FMatrix Result;
	if (GetScaledAxis(EAxis::X).IsNearlyZero(SMALL_NUMBER) &&
		GetScaledAxis(EAxis::Y).IsNearlyZero(SMALL_NUMBER) &&
		GetScaledAxis(EAxis::Z).IsNearlyZero(SMALL_NUMBER))
	{
		// just set to zero - avoids unsafe inverse of zero and duplicates what QNANs were resulting in before (scaling away all children)
		Result = FMatrix::Identity;
	}
	else
	{
		const float	Det = Determinant();

		if (Det == 0.0f)
		{
			Result = FMatrix::Identity;
		}
		else
		{
			VectorMatrixInverse(&Result, this);
		}
	}
	return Result;
}

inline FVector FMatrix::GetScaledAxis(EAxis::Type InAxis) const
{
	switch (InAxis)
	{
	case EAxis::X:
		return FVector(M[0][0], M[0][1], M[0][2]);

	case EAxis::Y:
		return FVector(M[1][0], M[1][1], M[1][2]);

	case EAxis::Z:
		return FVector(M[2][0], M[2][1], M[2][2]);

	default:
		return FVector(0.f);
	}
}

inline float FMatrix::Determinant() const
{
	return	M[0][0] * (
			M[1][1] * (M[2][2] * M[3][3] - M[2][3] * M[3][2]) -
			M[2][1] * (M[1][2] * M[3][3] - M[1][3] * M[3][2]) +
			M[3][1] * (M[1][2] * M[2][3] - M[1][3] * M[2][2])
			) -
		M[1][0] * (
			M[0][1] * (M[2][2] * M[3][3] - M[2][3] * M[3][2]) -
			M[2][1] * (M[0][2] * M[3][3] - M[0][3] * M[3][2]) +
			M[3][1] * (M[0][2] * M[2][3] - M[0][3] * M[2][2])
			) +
		M[2][0] * (
			M[0][1] * (M[1][2] * M[3][3] - M[1][3] * M[3][2]) -
			M[1][1] * (M[0][2] * M[3][3] - M[0][3] * M[3][2]) +
			M[3][1] * (M[0][2] * M[1][3] - M[0][3] * M[1][2])
			) -
		M[3][0] * (
			M[0][1] * (M[1][2] * M[2][3] - M[1][3] * M[2][2]) -
			M[1][1] * (M[0][2] * M[2][3] - M[0][3] * M[2][2]) +
			M[2][1] * (M[0][2] * M[1][3] - M[0][3] * M[1][2])
			);
}

inline FMatrix FMatrix::operator+(const FMatrix& Other) const
{
	FMatrix Result;
	for (int32 X = 0; X < 4; ++X)
	{
		for (int32 Y = 0; Y < 4; ++Y)
		{
			Result.M[X][Y] = M[X][Y] + Other.M[X][Y];
		}
	}
	return Result;
}

inline void FMatrix::operator+=(const FMatrix& Other)
{
	for (int32 X = 0; X < 4; ++X)
	{
		for (int32 Y = 0; Y < 4; ++Y)
		{
			M[X][Y] += Other.M[X][Y];
		}
	}
}

inline FMatrix FMatrix::operator*(float Other) const
{
	FMatrix Result;
	for (int32 X = 0; X < 4; ++X)
	{
		for (int32 Y = 0; Y < 4; ++Y)
		{
			Result.M[X][Y] = M[X][Y] * Other;
		}
	}
	return Result;
}

inline void FMatrix::operator*=(float Other)
{
	for (int32 X = 0; X < 4; ++X)
	{
		for (int32 Y = 0; Y < 4; ++Y)
		{
			M[X][Y] *= Other;
		}
	}
}

inline FMatrix FMatrix::operator*(const FMatrix& Other) const
{
	FMatrix Result;
	typedef float Float4x4[4][4];
	const Float4x4& A = *((const Float4x4*)this);
	const Float4x4& B = *((const Float4x4*)&Other);
	Float4x4 Temp;
	Temp[0][0] = A[0][0] * B[0][0] + A[0][1] * B[1][0] + A[0][2] * B[2][0] + A[0][3] * B[3][0];
	Temp[0][1] = A[0][0] * B[0][1] + A[0][1] * B[1][1] + A[0][2] * B[2][1] + A[0][3] * B[3][1];
	Temp[0][2] = A[0][0] * B[0][2] + A[0][1] * B[1][2] + A[0][2] * B[2][2] + A[0][3] * B[3][2];
	Temp[0][3] = A[0][0] * B[0][3] + A[0][1] * B[1][3] + A[0][2] * B[2][3] + A[0][3] * B[3][3];

	Temp[1][0] = A[1][0] * B[0][0] + A[1][1] * B[1][0] + A[1][2] * B[2][0] + A[1][3] * B[3][0];
	Temp[1][1] = A[1][0] * B[0][1] + A[1][1] * B[1][1] + A[1][2] * B[2][1] + A[1][3] * B[3][1];
	Temp[1][2] = A[1][0] * B[0][2] + A[1][1] * B[1][2] + A[1][2] * B[2][2] + A[1][3] * B[3][2];
	Temp[1][3] = A[1][0] * B[0][3] + A[1][1] * B[1][3] + A[1][2] * B[2][3] + A[1][3] * B[3][3];

	Temp[2][0] = A[2][0] * B[0][0] + A[2][1] * B[1][0] + A[2][2] * B[2][0] + A[2][3] * B[3][0];
	Temp[2][1] = A[2][0] * B[0][1] + A[2][1] * B[1][1] + A[2][2] * B[2][1] + A[2][3] * B[3][1];
	Temp[2][2] = A[2][0] * B[0][2] + A[2][1] * B[1][2] + A[2][2] * B[2][2] + A[2][3] * B[3][2];
	Temp[2][3] = A[2][0] * B[0][3] + A[2][1] * B[1][3] + A[2][2] * B[2][3] + A[2][3] * B[3][3];

	Temp[3][0] = A[3][0] * B[0][0] + A[3][1] * B[1][0] + A[3][2] * B[2][0] + A[3][3] * B[3][0];
	Temp[3][1] = A[3][0] * B[0][1] + A[3][1] * B[1][1] + A[3][2] * B[2][1] + A[3][3] * B[3][1];
	Temp[3][2] = A[3][0] * B[0][2] + A[3][1] * B[1][2] + A[3][2] * B[2][2] + A[3][3] * B[3][2];
	Temp[3][3] = A[3][0] * B[0][3] + A[3][1] * B[1][3] + A[3][2] * B[2][3] + A[3][3] * B[3][3];
	memcpy(&Result, &Temp, 16 * sizeof(float));

	return Result;
}

inline void FMatrix::operator*=(const FMatrix& Other)
{
	*this = operator*(Other);
}

inline FVector FMatrix::Transform(const FVector& InVector) const
{
	return 
	{ 
		InVector.X * M[0][0] + InVector.Y * M[1][0] + InVector.Z * M[2][0],
		InVector.X * M[0][1] + InVector.Y * M[1][1] + InVector.Z * M[2][1],
		InVector.X * M[0][2] + InVector.Y * M[1][2] + InVector.Z * M[2][2],
	};
}

inline FMatrix FMatrix::FromScale(float Scale)
{
	FMatrix Result;
	Result.M[0][0] = Scale;				Result.M[0][1] = 0.0f;			Result.M[0][2] = 0.0f;						Result.M[0][3] = 0.0f;
	Result.M[1][0] = 0.0f;				Result.M[1][1] = Scale;			Result.M[1][2] = 0.0f;						Result.M[1][3] = 0.0f;
	Result.M[2][0] = 0.0f;				Result.M[2][1] = 0.0f;			Result.M[2][2] = Scale;						Result.M[2][3] = 0.0f;
	Result.M[3][0] = 0.0f;				Result.M[3][1] = 0.0f;			Result.M[3][2] = 0.0f;						Result.M[3][3] = 1.0f;
	return Result;
}

inline FMatrix FMatrix::DXFromPitch(float fPitch)
{
	FMatrix Result;
	float CosTheta = cosf(fPitch);
	float SinTheta = sinf(fPitch);
	Result.M[0][0] = 1.0f;	Result.M[0][1] = 0.0f;		Result.M[0][2] = 0.0f;		Result.M[0][3] = 0.0f;
	Result.M[1][0] = 0.0f;	Result.M[1][1] = CosTheta;	Result.M[1][2] = SinTheta;	Result.M[1][3] = 0.0f;
	Result.M[2][0] = 0.0f;	Result.M[2][1] = -SinTheta;	Result.M[2][2] = CosTheta;	Result.M[2][3] = 0.0f;
	Result.M[3][0] = 0.0f;	Result.M[3][1] = 0.0f;		Result.M[3][2] = 0.0f;		Result.M[3][3] = 1.0f;
	return Result;
}

inline FMatrix FMatrix::DXFromYaw(float fYaw)
{
	FMatrix Result;
	float CosTheta = cosf(fYaw);
	float SinTheta = sinf(fYaw);
	Result.M[0][0] = CosTheta;	Result.M[0][1] = 0.0f;		Result.M[0][2] = -SinTheta;		Result.M[0][3] = 0.0f;
	Result.M[1][0] = 0.0f;		Result.M[1][1] = 1.0f;		Result.M[1][2] = 0.0f;			Result.M[1][3] = 0.0f;
	Result.M[2][0] = SinTheta;	Result.M[2][1] = 0.0f;		Result.M[2][2] = CosTheta;		Result.M[2][3] = 0.0f;
	Result.M[3][0] = 0.0f;		Result.M[3][1] = 0.0f;		Result.M[3][2] = 0.0f;			Result.M[3][3] = 1.0f;
	return Result;
}

inline FMatrix FMatrix::DXFromRoll(float fRoll)
{
	FMatrix Result;
	float CosTheta = cosf(fRoll);
	float SinTheta = sinf(fRoll);
	Result.M[0][0] = CosTheta;	Result.M[0][1] = SinTheta;		Result.M[0][2] = 0.0f;		Result.M[0][3] = 0.0f;
	Result.M[1][0] = -SinTheta;	Result.M[1][1] = CosTheta;		Result.M[1][2] = 0.0f;		Result.M[1][3] = 0.0f;
	Result.M[2][0] = 0.0f;		Result.M[2][1] = 0.0f;			Result.M[2][2] = 1.0f;		Result.M[2][3] = 0.0f;
	Result.M[3][0] = 0.0f;		Result.M[3][1] = 0.0f;			Result.M[3][2] = 0.0f;		Result.M[3][3] = 1.0f;
	return Result;
}

inline FMatrix FMatrix::DXFormRotation(FRotator Rotation)
{
	FMatrix Pitch =	DXFromPitch(Rotation.Pitch);
	FMatrix Yaw =	DXFromYaw(Rotation.Yaw);
	FMatrix Roll =	DXFromRoll(Rotation.Roll);
	return Pitch * Yaw * Roll;
}

inline FMatrix FMatrix::DXFromTranslation(FVector Translation)
{
	FMatrix Result;
	Result.M[0][0] = 1;					Result.M[0][1] = 0.0f;			Result.M[0][2] = 0.0f;						Result.M[0][3] = 0.0f;
	Result.M[1][0] = 0.0f;				Result.M[1][1] = 1.0f;			Result.M[1][2] = 0.0f;						Result.M[1][3] = 0.0f;
	Result.M[2][0] = 0.0f;				Result.M[2][1] = 0.0f;			Result.M[2][2] = 1.0f;						Result.M[2][3] = 0.0f;
	Result.M[3][0] = Translation.X;		Result.M[3][1] = Translation.Y;	Result.M[3][2] = Translation.Z;				Result.M[3][3] = 1.0f;
	return Result;
}

inline FMatrix FMatrix::DXLookAtLH(const FVector& Eye, const FVector& LookAtPosition, const FVector& Up)
{
	FMatrix Result;
	FVector Z = LookAtPosition - Eye;
	Z.Normalize();
	FVector X = Up ^ Z;
	X.Normalize();
	FVector Y = Z ^ X;
	Y.Normalize();
	Result.M[0][0] = X.X;			Result.M[0][1] = Y.X;			Result.M[0][2] = Z.X;				Result.M[0][3] = 0.0f;
	Result.M[1][0] = X.Y;			Result.M[1][1] = Y.Y;			Result.M[1][2] = Z.Y;				Result.M[1][3] = 0.0f;
	Result.M[2][0] = X.Z;			Result.M[2][1] = Y.Z;			Result.M[2][2] = Z.Z;				Result.M[2][3] = 0.0f;
	Result.M[3][0] = X | (-Eye);	Result.M[3][1] = Y | (-Eye);	Result.M[3][2] = Z | (-Eye);		Result.M[3][3] = 1.0f;
	return Result;
}

inline FMatrix FMatrix::DXFromPerspectiveFovLH(float fieldOfViewY, float aspectRatio, float znearPlane, float zfarPlane)
{
	/*
		xScale     0          0               0
		0        yScale       0               0
		0          0       zf/(zf-zn)         1
		0          0       -zn*zf/(zf-zn)     0
		where:
		yScale = cot(fovY/2)

		xScale = yScale / aspect ratio
	*/
	FMatrix Result;
	float Cot = 1.0f / tanf(0.5f * fieldOfViewY);
	float InverNF = zfarPlane / (zfarPlane - znearPlane);

	Result.M[0][0] = Cot / aspectRatio;			Result.M[0][1] = 0.0f;		Result.M[0][2] = 0.0f;									Result.M[0][3] = 0.0f;
	Result.M[1][0] = 0.0f;						Result.M[1][1] = Cot;		Result.M[1][2] = 0.0f;									Result.M[1][3] = 0.0f;
	Result.M[2][0] = 0.0f;						Result.M[2][1] = 0.0f;		Result.M[2][2] =  InverNF;								Result.M[2][3] = 1.0f;
	Result.M[3][0] = 0.0f;						Result.M[3][1] = 0.0f;		Result.M[3][2] = -InverNF * znearPlane;					Result.M[3][3] = 0.0f;
	return Result;
}

inline FMatrix FMatrix::DXFromPerspectiveLH(float w, float h, float zn, float zf)
{
	/*
		2 * zn / w			0				0					0
		0					2 * zn / h		0					0
		0					0				zf / (zf - zn)		1
		0					0				zn*zf / (zn - zf)	0
	*/

	FMatrix Result;
	Result.M[0][0] = 2 * zn / w;		Result.M[0][1] = 0.0f;				Result.M[0][2] = 0.0f;									Result.M[0][3] = 0.0f;
	Result.M[1][0] = 0.0f;				Result.M[1][1] = 2.0f * zn / h;		Result.M[1][2] = 0.0f;									Result.M[1][3] = 0.0f;
	Result.M[2][0] = 0.0f;				Result.M[2][1] = 0.0f;				Result.M[2][2] = zf / (zf - zn);						Result.M[2][3] = 1.0f;
	Result.M[3][0] = 0.0f;				Result.M[3][1] = 0.0f;				Result.M[3][2] = zn * zf / (zn - zf);					Result.M[3][3] = 0.0f;
	return Result;
}

inline bool FMatrix::operator==(const FMatrix& Other) const
{
	for (int32 X = 0; X < 4; ++X)
	{
		for (int32 Y = 0; Y < 4; ++Y)
		{
			if (M[X][Y] != Other.M[X][Y])
			{
				return false;
			}
		}
	}
	return true;
}

inline bool FMatrix::operator!=(const FMatrix& Other) const
{
	for (int32 X = 0; X < 4; ++X)
	{
		for (int32 Y = 0; Y < 4; ++Y)
		{
			if (M[X][Y] != Other.M[X][Y])
			{
				return true;
			}
		}
	}
	return false;
}

struct FBox
{
public:
	FVector Min;
	FVector Max;
	uint8 IsValid;

public:

	FBox() { }

	FBox(const FVector& InMin, const FVector& InMax)
		: Min(InMin)
		, Max(InMax)
		, IsValid(1)
	{ }
	 FBox(const FVector* Points, int32 Count);
public:
	 bool operator==(const FBox& Other) const
	{
		return (Min == Other.Min) && (Max == Other.Max);
	}
	 FBox& operator+=(const FVector &Other);
	 FBox operator+(const FVector& Other) const
	{
		return FBox(*this) += Other;
	}
	 FBox& operator+=(const FBox& Other);
	 FBox operator+(const FBox& Other) const
	{
		return FBox(*this) += Other;
	}
	 FVector& operator[](int32 Index)
	{
		assert((Index >= 0) && (Index < 2));

		if (Index == 0)
		{
			return Min;
		}

		return Max;
	}

public:
	 float ComputeSquaredDistanceToPoint(const FVector& Point) const
	{
		return ComputeSquaredDistanceFromBoxToPoint(Min, Max, Point);
	}
	 FBox ExpandBy(float W) const
	{
		return FBox(Min - FVector(W, W, W), Max + FVector(W, W, W));
	}
	 FBox ExpandBy(const FVector& V) const
	{
		return FBox(Min - V, Max + V);
	}
	FBox ExpandBy(const FVector& Neg, const FVector& Pos) const
	{
		return FBox(Min - Neg, Max + Pos);
	}
	 FBox ShiftBy(const FVector& Offset) const
	{
		return FBox(Min + Offset, Max + Offset);
	}
	 FBox MoveTo(const FVector& Destination) const
	{
		const FVector Offset = Destination - GetCenter();
		return FBox(Min + Offset, Max + Offset);
	}
	 FVector GetCenter() const
	{
		return FVector((Min + Max) * 0.5f);
	}
	 void GetCenterAndExtents(FVector& center, FVector& Extents) const
	{
		Extents = GetExtent();
		center = Min + Extents;
	}
	 FVector GetClosestPointTo(const FVector& Point) const;
	 FVector GetExtent() const
	{
		return 0.5f * (Max - Min);
	}
	 FVector& GetExtrema(int PointIndex)
	{
		return (&Min)[PointIndex];
	}
	 const FVector& GetExtrema(int PointIndex) const
	{
		return (&Min)[PointIndex];
	}
	 FVector GetSize() const
	{
		return (Max - Min);
	}
	 float GetVolume() const
	{
		return ((Max.X - Min.X) * (Max.Y - Min.Y) * (Max.Z - Min.Z));
	}
	 void Init()
	{
		Min = Max = FVector::ZeroVector;
		IsValid = 0;
	}
	 bool Intersect(const FBox& other) const;
	 bool IntersectXY(const FBox& Other) const;
	 FBox Overlap(const FBox& Other) const;
	 FBox InverseTransformBy(const FTransform& M) const;
	 bool IsInside(const FVector& In) const
	{
		return ((In.X > Min.X) && (In.X < Max.X) && (In.Y > Min.Y) && (In.Y < Max.Y) && (In.Z > Min.Z) && (In.Z < Max.Z));
	}
	 bool IsInsideOrOn(const FVector& In) const
	{
		return ((In.X >= Min.X) && (In.X <= Max.X) && (In.Y >= Min.Y) && (In.Y <= Max.Y) && (In.Z >= Min.Z) && (In.Z <= Max.Z));
	}
	 bool IsInside(const FBox& Other) const
	{
		return (IsInside(Other.Min) && IsInside(Other.Max));
	}
	 bool IsInsideXY(const FVector& In) const
	{
		return ((In.X > Min.X) && (In.X < Max.X) && (In.Y > Min.Y) && (In.Y < Max.Y));
	}
	 bool IsInsideXY(const FBox& Other) const
	{
		return (IsInsideXY(Other.Min) && IsInsideXY(Other.Max));
	}
	 FBox TransformBy(const FMatrix& M) const;
	 FBox TransformBy(const FTransform& M) const;
	 FBox TransformProjectBy(const FMatrix& ProjM) const;
public:

	static FBox BuildAABB(const FVector& Origin, const FVector& Extent)
	{
		FBox NewBox(Origin - Extent, Origin + Extent);

		return NewBox;
	}
};
 inline FBox& FBox::operator+=(const FVector &Other)
{
	if (IsValid)
	{
		Min.X = FMath::Min(Min.X, Other.X);
		Min.Y = FMath::Min(Min.Y, Other.Y);
		Min.Z = FMath::Min(Min.Z, Other.Z);

		Max.X = FMath::Max(Max.X, Other.X);
		Max.Y = FMath::Max(Max.Y, Other.Y);
		Max.Z = FMath::Max(Max.Z, Other.Z);
	}
	else
	{
		Min = Max = Other;
		IsValid = 1;
	}

	return *this;
}


 inline FBox& FBox::operator+=(const FBox& Other)
{
	if (IsValid && Other.IsValid)
	{
		Min.X = FMath::Min(Min.X, Other.Min.X);
		Min.Y = FMath::Min(Min.Y, Other.Min.Y);
		Min.Z = FMath::Min(Min.Z, Other.Min.Z);

		Max.X = FMath::Max(Max.X, Other.Max.X);
		Max.Y = FMath::Max(Max.Y, Other.Max.Y);
		Max.Z = FMath::Max(Max.Z, Other.Max.Z);
	}
	else if (Other.IsValid)
	{
		*this = Other;
	}

	return *this;
}


 inline FVector FBox::GetClosestPointTo(const FVector& Point) const
{
	// start by considering the point inside the box
	FVector ClosestPoint = Point;

	// now clamp to inside box if it's outside
	if (Point.X < Min.X)
	{
		ClosestPoint.X = Min.X;
	}
	else if (Point.X > Max.X)
	{
		ClosestPoint.X = Max.X;
	}

	// now clamp to inside box if it's outside
	if (Point.Y < Min.Y)
	{
		ClosestPoint.Y = Min.Y;
	}
	else if (Point.Y > Max.Y)
	{
		ClosestPoint.Y = Max.Y;
	}

	// Now clamp to inside box if it's outside.
	if (Point.Z < Min.Z)
	{
		ClosestPoint.Z = Min.Z;
	}
	else if (Point.Z > Max.Z)
	{
		ClosestPoint.Z = Max.Z;
	}

	return ClosestPoint;
}


 inline bool FBox::Intersect(const FBox& Other) const
{
	if ((Min.X > Other.Max.X) || (Other.Min.X > Max.X))
	{
		return false;
	}

	if ((Min.Y > Other.Max.Y) || (Other.Min.Y > Max.Y))
	{
		return false;
	}

	if ((Min.Z > Other.Max.Z) || (Other.Min.Z > Max.Z))
	{
		return false;
	}

	return true;
}


 inline bool FBox::IntersectXY(const FBox& Other) const
{
	if ((Min.X > Other.Max.X) || (Other.Min.X > Max.X))
	{
		return false;
	}

	if ((Min.Y > Other.Max.Y) || (Other.Min.Y > Max.Y))
	{
		return false;
	}

	return true;
}

/* Math inline functions
*****************************************************************************/

inline bool FMath::PointBoxIntersection
(
	const FVector&	Point,
	const FBox&		Box
)
{
	if (Point.X >= Box.Min.X && Point.X <= Box.Max.X &&
		Point.Y >= Box.Min.Y && Point.Y <= Box.Max.Y &&
		Point.Z >= Box.Min.Z && Point.Z <= Box.Max.Z)
		return 1;
	else
		return 0;
}

inline bool FMath::LineBoxIntersection
(
	const FBox& Box,
	const FVector& Start,
	const FVector& End,
	const FVector& StartToEnd
)
{
	return LineBoxIntersection(Box, Start, End, StartToEnd, StartToEnd.Reciprocal());
}

inline bool FMath::LineBoxIntersection
(
	const FBox&		Box,
	const FVector&	Start,
	const FVector&	End,
	const FVector&	StartToEnd,
	const FVector&	OneOverStartToEnd
)
{
	FVector	Time;
	bool	bStartIsOutside = false;

	if (Start.X < Box.Min.X)
	{
		bStartIsOutside = true;
		if (End.X >= Box.Min.X)
		{
			Time.X = (Box.Min.X - Start.X) * OneOverStartToEnd.X;
		}
		else
		{
			return false;
		}
	}
	else if (Start.X > Box.Max.X)
	{
		bStartIsOutside = true;
		if (End.X <= Box.Max.X)
		{
			Time.X = (Box.Max.X - Start.X) * OneOverStartToEnd.X;
		}
		else
		{
			return false;
		}
	}
	else
	{
		Time.X = 0.0f;
	}

	if (Start.Y < Box.Min.Y)
	{
		bStartIsOutside = true;
		if (End.Y >= Box.Min.Y)
		{
			Time.Y = (Box.Min.Y - Start.Y) * OneOverStartToEnd.Y;
		}
		else
		{
			return false;
		}
	}
	else if (Start.Y > Box.Max.Y)
	{
		bStartIsOutside = true;
		if (End.Y <= Box.Max.Y)
		{
			Time.Y = (Box.Max.Y - Start.Y) * OneOverStartToEnd.Y;
		}
		else
		{
			return false;
		}
	}
	else
	{
		Time.Y = 0.0f;
	}

	if (Start.Z < Box.Min.Z)
	{
		bStartIsOutside = true;
		if (End.Z >= Box.Min.Z)
		{
			Time.Z = (Box.Min.Z - Start.Z) * OneOverStartToEnd.Z;
		}
		else
		{
			return false;
		}
	}
	else if (Start.Z > Box.Max.Z)
	{
		bStartIsOutside = true;
		if (End.Z <= Box.Max.Z)
		{
			Time.Z = (Box.Max.Z - Start.Z) * OneOverStartToEnd.Z;
		}
		else
		{
			return false;
		}
	}
	else
	{
		Time.Z = 0.0f;
	}

	if (bStartIsOutside)
	{
		const float	MaxTime = Max3(Time.X, Time.Y, Time.Z);

		if (MaxTime >= 0.0f && MaxTime <= 1.0f)
		{
			const FVector Hit = Start + StartToEnd * MaxTime;
			const float BOX_SIDE_THRESHOLD = 0.1f;
			if (Hit.X > Box.Min.X - BOX_SIDE_THRESHOLD && Hit.X < Box.Max.X + BOX_SIDE_THRESHOLD &&
				Hit.Y > Box.Min.Y - BOX_SIDE_THRESHOLD && Hit.Y < Box.Max.Y + BOX_SIDE_THRESHOLD &&
				Hit.Z > Box.Min.Z - BOX_SIDE_THRESHOLD && Hit.Z < Box.Max.Z + BOX_SIDE_THRESHOLD)
			{
				return true;
			}
		}

		return false;
	}
	else
	{
		return true;
	}
}

inline float GetBasisDeterminantSign(const FVector& XAxis, const FVector& YAxis, const FVector& ZAxis)
{
	FMatrix Basis(
		FPlane(XAxis, 0),
		FPlane(YAxis, 0),
		FPlane(ZAxis, 0),
		FPlane(0, 0, 0, 1)
	);
	return (Basis.Determinant() < 0) ? -1.0f : +1.0f;
}

struct Box2D
{
	Vector2 Min;
	Vector2 Max;
	uint8 IsValid;

	Box2D():IsValid(false) {}

	Box2D(const Vector2& InMin, const Vector2& InMax)
		:Min(InMin)
		, Max(InMax)
		, IsValid(1)
	{ }

	inline Box2D& operator+=(const Vector2& Other)
	{
		if (IsValid)
		{
			Min.X = fminf(Min.X, Other.X);
			Min.Y = fminf(Min.Y, Other.Y);

			Max.X = fmaxf(Min.X, Other.X);
			Max.Y = fmaxf(Min.Y, Other.Y);
		}
		else
		{
			Min = Max = Other;
			IsValid = 1;
		}
		return *this;
	}

	inline Box2D& operator+=(const Box2D& Other)
	{
		if (IsValid && Other.IsValid)
		{
			Min.X = fminf(Min.X, Other.Min.X);
			Min.Y = fminf(Min.Y, Other.Min.Y);

			Max.X = fmaxf(Min.X, Other.Max.X);
			Max.Y = fmaxf(Min.Y, Other.Max.Y);
		}
		else if (Other.IsValid)
		{
			*this = Other;
		}
		return *this;
	}

	inline Box2D operator+(const Vector2& Other)
	{
		return Box2D(*this) += Other;
	}

	inline Box2D operator+(const Box2D& Other)
	{
		return Box2D(*this) += Other;
	}
};

struct Frustum
{
	FVector Vertices[8];

	Frustum(float fov, float aspect, float znear, float zfar)
	{
		float yNear = std::tan(0.5f*fov)  * znear;
		float yFar = std::tan(0.5f*fov)  * zfar;
		float xNear = yNear * aspect;
		float xFar = yFar * aspect;

		Vertices[0] = FVector(xNear, yNear, znear);
		Vertices[1] = FVector(-xNear, yNear, znear);
		Vertices[2] = FVector(-xNear, -yNear, znear);
		Vertices[3] = FVector(xNear, -yNear, znear);
		Vertices[4] = FVector(xFar, yFar, zfar);
		Vertices[5] = FVector(-xFar, yFar, zfar);
		Vertices[6] = FVector(-xFar, -yFar, zfar);
		Vertices[7] = FVector(xFar, -yFar, zfar);
	}

	Box2D GetBounds2D(const FMatrix& ViewMatrix, const FVector& Axis1, const FVector& Axis2);
	FBox GetBounds(const FMatrix& TransformMatrix);
	FBox GetBounds();
};

struct FIntPoint
{
	int32 X;
	int32 Y;

	FIntPoint() {}

	FIntPoint(int32 InX, int32 InY);
	const int32& operator()(int32 PointIndex) const;
	int32& operator()(int32 PointIndex);
	bool operator==(const FIntPoint& Other) const;
	bool operator!=(const FIntPoint& Other) const;
	FIntPoint& operator*=(int32 Scale);
	FIntPoint& operator/=(int32 Divisor);
	FIntPoint& operator+=(const FIntPoint& Other);
	FIntPoint& operator-=(const FIntPoint& Other);
	FIntPoint& operator/=(const FIntPoint& Other);
	FIntPoint& operator=(const FIntPoint& Other);
	FIntPoint operator*(int32 Scale) const;
	FIntPoint operator/(int32 Divisor) const;
	FIntPoint operator+(const FIntPoint& Other) const;
	FIntPoint operator-(const FIntPoint& Other) const;
	FIntPoint operator/(const FIntPoint& Other) const;
	int32& operator[](int32 Index);
	int32 operator[](int32 Index) const;

	inline FIntPoint ComponentMin(const FIntPoint& Other) const;
	inline FIntPoint ComponentMax(const FIntPoint& Other) const;
	int32 GetMax() const;
	int32 GetMin() const;
	int32 Size() const;
	int32 SizeSquared() const;

public:
	static FIntPoint DivideAndRoundUp(FIntPoint lhs, int32 Divisor);
	static FIntPoint DivideAndRoundUp(FIntPoint lhs, FIntPoint Divisor);
	static FIntPoint DivideAndRoundDown(FIntPoint lhs, int32 Divisor);
	static int32 Num();
};

inline FIntPoint::FIntPoint(int32 InX, int32 InY)
	: X(InX)
	, Y(InY)
{ }

inline const int32& FIntPoint::operator()(int32 PointIndex) const
{
	return (&X)[PointIndex];
}

inline int32& FIntPoint::operator()(int32 PointIndex)
{
	return (&X)[PointIndex];
}

inline int32 FIntPoint::Num()
{
	return 2;
}

inline bool FIntPoint::operator==(const FIntPoint& Other) const
{
	return X == Other.X && Y == Other.Y;
}

inline bool FIntPoint::operator!=(const FIntPoint& Other) const
{
	return (X != Other.X) || (Y != Other.Y);
}

inline FIntPoint& FIntPoint::operator*=(int32 Scale)
{
	X *= Scale;
	Y *= Scale;

	return *this;
}

inline FIntPoint& FIntPoint::operator/=(int32 Divisor)
{
	X /= Divisor;
	Y /= Divisor;

	return *this;
}

inline FIntPoint& FIntPoint::operator+=(const FIntPoint& Other)
{
	X += Other.X;
	Y += Other.Y;

	return *this;
}

inline FIntPoint& FIntPoint::operator-=(const FIntPoint& Other)
{
	X -= Other.X;
	Y -= Other.Y;

	return *this;
}

inline FIntPoint& FIntPoint::operator/=(const FIntPoint& Other)
{
	X /= Other.X;
	Y /= Other.Y;

	return *this;
}

inline FIntPoint& FIntPoint::operator=(const FIntPoint& Other)
{
	X = Other.X;
	Y = Other.Y;

	return *this;
}

inline FIntPoint FIntPoint::operator*(int32 Scale) const
{
	return FIntPoint(*this) *= Scale;
}

inline FIntPoint FIntPoint::operator/(int32 Divisor) const
{
	return FIntPoint(*this) /= Divisor;
}

inline int32& FIntPoint::operator[](int32 Index)
{
	//check(Index >= 0 && Index < 2);
	return ((Index == 0) ? X : Y);
}

inline int32 FIntPoint::operator[](int32 Index) const
{
	//check(Index >= 0 && Index < 2);
	return ((Index == 0) ? X : Y);
}

inline FIntPoint FIntPoint::ComponentMin(const FIntPoint& Other) const
{
	return FIntPoint(FMath::Min(X, Other.X), FMath::Min(Y, Other.Y));
}

inline FIntPoint FIntPoint::ComponentMax(const FIntPoint& Other) const
{
	return FIntPoint(FMath::Max(X, Other.X), FMath::Max(Y, Other.Y));
}

inline FIntPoint FIntPoint::DivideAndRoundUp(FIntPoint lhs, int32 Divisor)
{
	return FIntPoint(FMath::DivideAndRoundUp(lhs.X, Divisor), FMath::DivideAndRoundUp(lhs.Y, Divisor));
}

inline FIntPoint FIntPoint::DivideAndRoundUp(FIntPoint lhs, FIntPoint Divisor)
{
	return FIntPoint(FMath::DivideAndRoundUp(lhs.X, Divisor.X), FMath::DivideAndRoundUp(lhs.Y, Divisor.Y));
}

inline FIntPoint FIntPoint::DivideAndRoundDown(FIntPoint lhs, int32 Divisor)
{
	return FIntPoint(FMath::DivideAndRoundDown(lhs.X, Divisor), FMath::DivideAndRoundDown(lhs.Y, Divisor));
}

inline FIntPoint FIntPoint::operator+(const FIntPoint& Other) const
{
	return FIntPoint(*this) += Other;
}

inline FIntPoint FIntPoint::operator-(const FIntPoint& Other) const
{
	return FIntPoint(*this) -= Other;
}

inline FIntPoint FIntPoint::operator/(const FIntPoint& Other) const
{
	return FIntPoint(*this) /= Other;
}

inline int32 FIntPoint::GetMax() const
{
	return FMath::Max(X, Y);
}

inline int32 FIntPoint::GetMin() const
{
	return FMath::Min(X, Y);
}

inline int32 FIntPoint::Size() const
{
	int64 X64 = (int64)X;
	int64 Y64 = (int64)Y;
	return int32(FMath::Sqrt(float(X64 * X64 + Y64 * Y64)));
}

inline int32 FIntPoint::SizeSquared() const
{
	return X * X + Y * Y;
}

inline void QuantizeSceneBufferSize(const FIntPoint& InBufferSize, FIntPoint& OutBufferSize)
{
	// Ensure sizes are dividable by DividableBy to get post processing effects with lower resolution working well
	const uint32 DividableBy = 4;

	const uint32 Mask = ~(DividableBy - 1);
	OutBufferSize.X = (InBufferSize.X + DividableBy - 1) & Mask;
	OutBufferSize.Y = (InBufferSize.Y + DividableBy - 1) & Mask;
}

struct FIntVector
{
	/** Holds the point's x-coordinate. */
	int32 X;

	/** Holds the point's y-coordinate. */
	int32 Y;

	/**  Holds the point's z-coordinate. */
	int32 Z;
	FIntVector() {}
	FIntVector(int32 InX, int32 InY, int32 InZ);
};

inline FIntVector::FIntVector(int32 InX, int32 InY, int32 InZ) : X(InX) , Y(InY) , Z(InZ) { }

struct FIntRect
{
	FIntPoint Min;
	FIntPoint Max;
public:
	FIntRect() {};
	FIntRect(int32 X0, int32 Y0, int32 X1, int32 Y1);
	FIntRect(FIntPoint InMin, FIntPoint InMax);
public:
	const FIntPoint& operator()(int32 PointIndex) const;
	FIntPoint& operator()(int32 PointIndex);
	bool operator==(const FIntRect& Other) const;
	bool operator!=(const FIntRect& Other) const;
	FIntRect& operator*=(int32 Scale);
	FIntRect& operator+=(const FIntPoint& Point);
	FIntRect& operator-=(const FIntPoint& Point);
	FIntRect operator*(int32 Scale) const;
	FIntRect operator/(int32 Div) const;
	FIntRect operator+(const FIntPoint& Point) const;
	FIntRect operator/(const FIntPoint& Point) const;
	FIntRect operator-(const FIntPoint& Point) const;
	FIntRect operator+(const FIntRect& Other) const;
	FIntRect operator-(const FIntRect& Other) const;
public:
	int32 Area() const;
	FIntRect Bottom(int32 InHeight) const;
	void Clip(const FIntRect& Other);
	void Union(const FIntRect& Other);
	bool Contains(FIntPoint Point) const;
	void GetCenterAndExtents(FIntPoint& OutCenter, FIntPoint& OutExtent) const;
	int32 Height() const;
	void InflateRect(int32 Amount);
	void Include(FIntPoint Point);
	FIntRect Inner(FIntPoint Shrink) const;
	FIntRect Right(int32 InWidth) const;
	FIntRect Scale(float Fraction) const;
	FIntPoint Size() const;
	int32 Width() const;
	bool IsEmpty() const;
public:
	static FIntRect DivideAndRoundUp(FIntRect lhs, int32 Div);
	static FIntRect DivideAndRoundUp(FIntRect lhs, FIntPoint Div);
	static int32 Num();
};


inline FIntRect FIntRect::Scale(float Fraction) const
{
	Vector2 Min2D = Vector2((float)Min.X, (float)Min.Y) * Fraction;
	Vector2 Max2D = Vector2((float)Max.X, (float)Max.Y) * Fraction;

	return FIntRect(FMath::FloorToInt(Min2D.X), FMath::FloorToInt(Min2D.Y), FMath::CeilToInt(Max2D.X), FMath::CeilToInt(Max2D.Y));
}
/* IntRect inline functions
*****************************************************************************/
inline FIntRect::FIntRect(int32 X0, int32 Y0, int32 X1, int32 Y1)
	: Min(X0, Y0)
	, Max(X1, Y1)
{ }


inline FIntRect::FIntRect(FIntPoint InMin, FIntPoint InMax)
	: Min(InMin)
	, Max(InMax)
{ }


inline const FIntPoint& FIntRect::operator()(int32 PointIndex) const
{
	return (&Min)[PointIndex];
}


inline FIntPoint& FIntRect::operator()(int32 PointIndex)
{
	return (&Min)[PointIndex];
}


inline bool FIntRect::operator==(const FIntRect& Other) const
{
	return Min == Other.Min && Max == Other.Max;
}


inline bool FIntRect::operator!=(const FIntRect& Other) const
{
	return Min != Other.Min || Max != Other.Max;
}


inline FIntRect& FIntRect::operator*=(int32 Scale)
{
	Min *= Scale;
	Max *= Scale;

	return *this;
}


inline FIntRect& FIntRect::operator+=(const FIntPoint& Point)
{
	Min += Point;
	Max += Point;

	return *this;
}


inline FIntRect& FIntRect::operator-=(const FIntPoint& Point)
{
	Min -= Point;
	Max -= Point;

	return *this;
}


inline FIntRect FIntRect::operator*(int32 Scale) const
{
	return FIntRect(Min * Scale, Max * Scale);
}


inline FIntRect FIntRect::operator/(int32 Div) const
{
	return FIntRect(Min / Div, Max / Div);
}


inline FIntRect FIntRect::operator+(const FIntPoint& Point) const
{
	return FIntRect(Min + Point, Max + Point);
}


inline FIntRect FIntRect::operator/(const FIntPoint& Point) const
{
	return FIntRect(Min / Point, Max / Point);
}


inline FIntRect FIntRect::operator-(const FIntPoint& Point) const
{
	return FIntRect(Min - Point, Max - Point);
}


inline FIntRect FIntRect::operator+(const FIntRect& Other) const
{
	return FIntRect(Min + Other.Min, Max + Other.Max);
}


inline FIntRect FIntRect::operator-(const FIntRect& Other) const
{
	return FIntRect(Min - Other.Min, Max - Other.Max);
}


inline int32 FIntRect::Area() const
{
	return (Max.X - Min.X) * (Max.Y - Min.Y);
}


inline FIntRect FIntRect::Bottom(int32 InHeight) const
{
	return FIntRect(Min.X, FMath::Max(Min.Y, Max.Y - InHeight), Max.X, Max.Y);
}


inline void FIntRect::Clip(const FIntRect& R)
{
	Min.X = FMath::Max<int32>(Min.X, R.Min.X);
	Min.Y = FMath::Max<int32>(Min.Y, R.Min.Y);
	Max.X = FMath::Min<int32>(Max.X, R.Max.X);
	Max.Y = FMath::Min<int32>(Max.Y, R.Max.Y);

	// return zero area if not overlapping
	Max.X = FMath::Max<int32>(Min.X, Max.X);
	Max.Y = FMath::Max<int32>(Min.Y, Max.Y);
}

inline void FIntRect::Union(const FIntRect& R)
{
	Min.X = FMath::Min<int32>(Min.X, R.Min.X);
	Min.Y = FMath::Min<int32>(Min.Y, R.Min.Y);
	Max.X = FMath::Max<int32>(Max.X, R.Max.X);
	Max.Y = FMath::Max<int32>(Max.Y, R.Max.Y);
}

inline bool FIntRect::Contains(FIntPoint P) const
{
	return P.X >= Min.X && P.X < Max.X && P.Y >= Min.Y && P.Y < Max.Y;
}


inline FIntRect FIntRect::DivideAndRoundUp(FIntRect lhs, int32 Div)
{
	return DivideAndRoundUp(lhs, FIntPoint(Div, Div));
}

inline FIntRect FIntRect::DivideAndRoundUp(FIntRect lhs, FIntPoint Div)
{
	return FIntRect(lhs.Min / Div, FIntPoint::DivideAndRoundUp(lhs.Max, Div));
}

inline void FIntRect::GetCenterAndExtents(FIntPoint& OutCenter, FIntPoint& OutExtent) const
{
	OutExtent.X = (Max.X - Min.X) / 2;
	OutExtent.Y = (Max.Y - Min.Y) / 2;

	OutCenter.X = Min.X + OutExtent.X;
	OutCenter.Y = Min.Y + OutExtent.Y;
}


inline int32 FIntRect::Height() const
{
	return (Max.Y - Min.Y);
}


inline void FIntRect::InflateRect(int32 Amount)
{
	Min.X -= Amount;
	Min.Y -= Amount;
	Max.X += Amount;
	Max.Y += Amount;
}


inline void FIntRect::Include(FIntPoint Point)
{
	Min.X = FMath::Min(Min.X, Point.X);
	Min.Y = FMath::Min(Min.Y, Point.Y);
	Max.X = FMath::Max(Max.X, Point.X);
	Max.Y = FMath::Max(Max.Y, Point.Y);
}

inline FIntRect FIntRect::Inner(FIntPoint Shrink) const
{
	return FIntRect(Min + Shrink, Max - Shrink);
}


inline int32 FIntRect::Num()
{
	return 2;
}


inline FIntRect FIntRect::Right(int32 InWidth) const
{
	return FIntRect(FMath::Max(Min.X, Max.X - InWidth), Min.Y, Max.X, Max.Y);
}


inline FIntPoint FIntRect::Size() const
{
	return FIntPoint(Max.X - Min.X, Max.Y - Min.Y);
}


inline int32 FIntRect::Width() const
{
	return Max.X - Min.X;
}

inline bool FIntRect::IsEmpty() const
{
	return Width() == 0 && Height() == 0;
}

/** Inverse Rotation matrix */
class FInverseRotationMatrix : public FMatrix
{
public:
	/**
	* Constructor.
	*
	* @param Rot rotation
	*/
	FInverseRotationMatrix(const FRotator& Rot);
};


inline FInverseRotationMatrix::FInverseRotationMatrix(const FRotator& Rot)
	: FMatrix(
		FMatrix( // Yaw
			FPlane(+FMath::Cos(Rot.Yaw * PI / 180.f), -FMath::Sin(Rot.Yaw * PI / 180.f), 0.0f, 0.0f),
			FPlane(+FMath::Sin(Rot.Yaw * PI / 180.f), +FMath::Cos(Rot.Yaw * PI / 180.f), 0.0f, 0.0f),
			FPlane(0.0f, 0.0f, 1.0f, 0.0f),
			FPlane(0.0f, 0.0f, 0.0f, 1.0f)) *
		FMatrix( // Pitch
			FPlane(+FMath::Cos(Rot.Pitch * PI / 180.f), 0.0f, -FMath::Sin(Rot.Pitch * PI / 180.f), 0.0f),
			FPlane(0.0f, 1.0f, 0.0f, 0.0f),
			FPlane(+FMath::Sin(Rot.Pitch * PI / 180.f), 0.0f, +FMath::Cos(Rot.Pitch * PI / 180.f), 0.0f),
			FPlane(0.0f, 0.0f, 0.0f, 1.0f)) *
		FMatrix( // Roll
			FPlane(1.0f, 0.0f, 0.0f, 0.0f),
			FPlane(0.0f, +FMath::Cos(Rot.Roll * PI / 180.f), +FMath::Sin(Rot.Roll * PI / 180.f), 0.0f),
			FPlane(0.0f, -FMath::Sin(Rot.Roll * PI / 180.f), +FMath::Cos(Rot.Roll * PI / 180.f), 0.0f),
			FPlane(0.0f, 0.0f, 0.0f, 1.0f))
	)
{ }

struct alignas(16) FQuat
{
public:

	/** The quaternion's X-component. */
	float X;

	/** The quaternion's Y-component. */
	float Y;

	/** The quaternion's Z-component. */
	float Z;

	/** The quaternion's W-component. */
	float W;

public:
	/** Identity quaternion. */
	static  const FQuat Identity;
public:
	/** Default constructor (no initialization). */
	inline FQuat() { }
	inline FQuat(float InX, float InY, float InZ, float InW);
	inline FQuat(const FQuat& Q);
	explicit FQuat(const FMatrix& M);
	explicit FQuat(const FRotator& R);
	FQuat(FVector Axis, float AngleRad);
public:
	inline FQuat& operator=(const FQuat& Other);
	inline FQuat operator+(const FQuat& Q) const;
	inline FQuat operator+=(const FQuat& Q);
	inline FQuat operator-(const FQuat& Q) const;
	inline bool Equals(const FQuat& Q, float Tolerance = KINDA_SMALL_NUMBER) const;
	inline bool IsIdentity(float Tolerance = SMALL_NUMBER) const;
	inline FQuat operator-=(const FQuat& Q);
	FQuat operator*(const FQuat& Q) const;
	inline FQuat operator*=(const FQuat& Q);
	FVector operator*(const FVector& V) const;
	FMatrix operator*(const FMatrix& M) const;
	inline FQuat operator*=(const float Scale);
	inline FQuat operator*(const float Scale) const;
	inline FQuat operator/=(const float Scale);
	inline FQuat operator/(const float Scale) const;
	bool operator==(const FQuat& Q) const;
	bool operator!=(const FQuat& Q) const;
	float operator|(const FQuat& Q) const;
public:
	static  FQuat MakeFromEuler(const FVector& Euler);
	FVector Euler() const;
	inline void Normalize(float Tolerance = SMALL_NUMBER);
	inline FQuat GetNormalized(float Tolerance = SMALL_NUMBER) const;
	bool IsNormalized() const;
	inline float Size() const;
	inline float SizeSquared() const;
	inline float GetAngle() const;
	void ToAxisAndAngle(FVector& Axis, float& Angle) const;
	void ToSwingTwist(const FVector& InTwistAxis, FQuat& OutSwing, FQuat& OutTwist) const;
	FVector RotateVector(FVector V) const;
	FVector UnrotateVector(FVector V) const;
	FQuat Log() const;
	FQuat Exp() const;
	inline FQuat Inverse() const;
	void EnforceShortestArcWith(const FQuat& OtherQuat);
	inline FVector GetAxisX() const;
	inline FVector GetAxisY() const;
	inline FVector GetAxisZ() const;
	inline FVector GetForwardVector() const;
	inline FVector GetRightVector() const;
	inline FVector GetUpVector() const;
	inline FVector Vector() const;
	FRotator Rotator() const;
	inline FVector GetRotationAxis() const;
	inline float AngularDistance(const FQuat& Q) const;
	bool ContainsNaN() const;
public:
	inline void DiagnosticCheckNaN() const
	{
		if (ContainsNaN())
		{
			//logOrEnsureNanError(TEXT("FQuat contains NaN: %s"), *ToString());
			*const_cast<FQuat*>(this) = FQuat::Identity;
		}
	}

	inline void DiagnosticCheckNaN(const wchar_t* Message) const
	{
		if (ContainsNaN())
		{
			//logOrEnsureNanError(TEXT("%s: FQuat contains NaN: %s"), Message, *ToString());
			*const_cast<FQuat*>(this) = FQuat::Identity;
		}
	}

public:
	static inline FQuat FindBetween(const FVector& V1, const FVector& V2)
	{
		return FindBetweenVectors(V1, V2);
	}
	static  FQuat FindBetweenNormals(const FVector& Normal1, const FVector& Normal2);
	static  FQuat FindBetweenVectors(const FVector& V1, const FVector& V2);
	static inline float Error(const FQuat& Q1, const FQuat& Q2);
	static inline float ErrorAutoNormalize(const FQuat& A, const FQuat& B);
	static inline FQuat FastLerp(const FQuat& A, const FQuat& B, const float Alpha);
	static inline FQuat FastBilerp(const FQuat& P00, const FQuat& P10, const FQuat& P01, const FQuat& P11, float FracX, float FracY);
	static  FQuat Slerp_NotNormalized(const FQuat &Quat1, const FQuat &Quat2, float Slerp);
	static inline FQuat Slerp(const FQuat &Quat1, const FQuat &Quat2, float Slerp)
	{
		return Slerp_NotNormalized(Quat1, Quat2, Slerp).GetNormalized();
	}
	static  FQuat SlerpFullPath_NotNormalized(const FQuat &quat1, const FQuat &quat2, float Alpha);
	static inline FQuat SlerpFullPath(const FQuat &quat1, const FQuat &quat2, float Alpha)
	{
		return SlerpFullPath_NotNormalized(quat1, quat2, Alpha).GetNormalized();
	}
	static  FQuat Squad(const FQuat& quat1, const FQuat& tang1, const FQuat& quat2, const FQuat& tang2, float Alpha);
	static  FQuat SquadFullPath(const FQuat& quat1, const FQuat& tang1, const FQuat& quat2, const FQuat& tang2, float Alpha);
	static  void CalcTangents(const FQuat& PrevP, const FQuat& P, const FQuat& NextP, float Tension, FQuat& OutTan);
public:
};
/* FQuat inline functions
*****************************************************************************/
inline FQuat::FQuat(const FMatrix& M)
{
	// If Matrix is NULL, return Identity quaternion. If any of them is 0, you won't be able to construct rotation
	// if you have two plane at least, we can reconstruct the frame using cross product, but that's a bit expensive op to do here
	// for now, if you convert to matrix from 0 scale and convert back, you'll lose rotation. Don't do that. 
	if (M.GetScaledAxis(EAxis::X).IsNearlyZero() || M.GetScaledAxis(EAxis::Y).IsNearlyZero() || M.GetScaledAxis(EAxis::Z).IsNearlyZero())
	{
		*this = FQuat::Identity;
		return;
	}

	//const MeReal *const t = (MeReal *) tm;
	float	s;

	// Check diagonal (trace)
	const float tr = M.M[0][0] + M.M[1][1] + M.M[2][2];

	if (tr > 0.0f)
	{
		float InvS = FMath::InvSqrt(tr + 1.f);
		this->W = 0.5f * (1.f / InvS);
		s = 0.5f * InvS;

		this->X = (M.M[1][2] - M.M[2][1]) * s;
		this->Y = (M.M[2][0] - M.M[0][2]) * s;
		this->Z = (M.M[0][1] - M.M[1][0]) * s;
	}
	else
	{
		// diagonal is negative
		int32 i = 0;

		if (M.M[1][1] > M.M[0][0])
			i = 1;

		if (M.M[2][2] > M.M[i][i])
			i = 2;

		static const int32 nxt[3] = { 1, 2, 0 };
		const int32 j = nxt[i];
		const int32 k = nxt[j];

		s = M.M[i][i] - M.M[j][j] - M.M[k][k] + 1.0f;

		float InvS = FMath::InvSqrt(s);

		float qt[4];
		qt[i] = 0.5f * (1.f / InvS);

		s = 0.5f * InvS;

		qt[3] = (M.M[j][k] - M.M[k][j]) * s;
		qt[j] = (M.M[i][j] + M.M[j][i]) * s;
		qt[k] = (M.M[i][k] + M.M[k][i]) * s;

		this->X = qt[0];
		this->Y = qt[1];
		this->Z = qt[2];
		this->W = qt[3];

		DiagnosticCheckNaN();
	}
}
inline FQuat::FQuat(const FRotator& R)
{
	*this = R.Quaternion();
	DiagnosticCheckNaN();
}
inline FVector FQuat::operator*(const FVector& V) const
{
	return RotateVector(V);
}
inline FQuat::FQuat(float InX, float InY, float InZ, float InW)
	: X(InX)
	, Y(InY)
	, Z(InZ)
	, W(InW)
{
	DiagnosticCheckNaN();
}
inline FQuat::FQuat(const FQuat& Q)
	: X(Q.X)
	, Y(Q.Y)
	, Z(Q.Z)
	, W(Q.W)
{ }
inline FQuat& FQuat::operator=(const FQuat& Other)
{
	this->X = Other.X;
	this->Y = Other.Y;
	this->Z = Other.Z;
	this->W = Other.W;

	return *this;
}

inline FQuat::FQuat(FVector Axis, float AngleRad)
{
	const float half_a = 0.5f * AngleRad;
	float s, c;
	FMath::SinCos(&s, &c, half_a);

	X = s * Axis.X;
	Y = s * Axis.Y;
	Z = s * Axis.Z;
	W = c;

	DiagnosticCheckNaN();
}
inline FQuat FQuat::operator+(const FQuat& Q) const
{
	return FQuat(X + Q.X, Y + Q.Y, Z + Q.Z, W + Q.W);
}
inline FQuat FQuat::operator+=(const FQuat& Q)
{
	this->X += Q.X;
	this->Y += Q.Y;
	this->Z += Q.Z;
	this->W += Q.W;

	DiagnosticCheckNaN();

	return *this;
}

inline FQuat FQuat::operator-(const FQuat& Q) const
{
	return FQuat(X - Q.X, Y - Q.Y, Z - Q.Z, W - Q.W);
}

inline bool FQuat::Equals(const FQuat& Q, float Tolerance) const
{
	return (FMath::Abs(X - Q.X) <= Tolerance && FMath::Abs(Y - Q.Y) <= Tolerance && FMath::Abs(Z - Q.Z) <= Tolerance && FMath::Abs(W - Q.W) <= Tolerance)
		|| (FMath::Abs(X + Q.X) <= Tolerance && FMath::Abs(Y + Q.Y) <= Tolerance && FMath::Abs(Z + Q.Z) <= Tolerance && FMath::Abs(W + Q.W) <= Tolerance);
}

inline bool FQuat::IsIdentity(float Tolerance) const
{
	return Equals(FQuat::Identity, Tolerance);
}

inline FQuat FQuat::operator-=(const FQuat& Q)
{
	this->X -= Q.X;
	this->Y -= Q.Y;
	this->Z -= Q.Z;
	this->W -= Q.W;

	DiagnosticCheckNaN();

	return *this;
}
inline FQuat FQuat::operator*=(const float Scale)
{
	X *= Scale;
	Y *= Scale;
	Z *= Scale;
	W *= Scale;

	DiagnosticCheckNaN();

	return *this;
}
inline FQuat FQuat::operator*(const float Scale) const
{
	return FQuat(Scale * X, Scale * Y, Scale * Z, Scale * W);
}
inline FQuat FQuat::operator/=(const float Scale)
{
	const float Recip = 1.0f / Scale;
	X *= Recip;
	Y *= Recip;
	Z *= Recip;
	W *= Recip;
	DiagnosticCheckNaN();
	return *this;
}
inline FQuat FQuat::operator/(const float Scale) const
{
	const float Recip = 1.0f / Scale;
	return FQuat(X * Recip, Y * Recip, Z * Recip, W * Recip);
}
inline bool FQuat::operator==(const FQuat& Q) const
{
	return X == Q.X && Y == Q.Y && Z == Q.Z && W == Q.W;
}
inline bool FQuat::operator!=(const FQuat& Q) const
{
	return X != Q.X || Y != Q.Y || Z != Q.Z || W != Q.W;
}
inline float FQuat::operator|(const FQuat& Q) const
{
	return X * Q.X + Y * Q.Y + Z * Q.Z + W * Q.W;
}
inline void FQuat::Normalize(float Tolerance)
{
	const float SquareSum = X * X + Y * Y + Z * Z + W * W;

	if (SquareSum >= Tolerance)
	{
		const float Scale = FMath::InvSqrt(SquareSum);

		X *= Scale;
		Y *= Scale;
		Z *= Scale;
		W *= Scale;
	}
	else
	{
		*this = FQuat::Identity;
	}
}
inline FQuat FQuat::GetNormalized(float Tolerance) const
{
	FQuat Result(*this);
	Result.Normalize(Tolerance);
	return Result;
}
inline bool FQuat::IsNormalized() const
{
	return (FMath::Abs(1.f - SizeSquared()) < THRESH_QUAT_NORMALIZED);
}
inline float FQuat::Size() const
{
	return FMath::Sqrt(X * X + Y * Y + Z * Z + W * W);
}
inline float FQuat::SizeSquared() const
{
	return (X * X + Y * Y + Z * Z + W * W);
}

inline float FQuat::GetAngle() const
{
	return 2.f * FMath::Acos(W);
}
inline void FQuat::ToAxisAndAngle(FVector& Axis, float& Angle) const
{
	Angle = GetAngle();
	Axis = GetRotationAxis();
}
inline FVector FQuat::GetRotationAxis() const
{
	// Ensure we never try to sqrt a neg number
	const float S = FMath::Sqrt(FMath::Max(1.f - (W * W), 0.f));

	if (S >= 0.0001f)
	{
		return FVector(X / S, Y / S, Z / S);
	}

	return FVector(1.f, 0.f, 0.f);
}
float FQuat::AngularDistance(const FQuat& Q) const
{
	float InnerProd = X * Q.X + Y * Q.Y + Z * Q.Z + W * Q.W;
	return FMath::Acos((2 * InnerProd * InnerProd) - 1.f);
}
inline FVector FQuat::RotateVector(FVector V) const
{
	// http://people.csail.mit.edu/bkph/articles/Quaternions.pdf
	// V' = V + 2w(Q x V) + (2Q x (Q x V))
	// refactor:
	// V' = V + w(2(Q x V)) + (Q x (2(Q x V)))
	// T = 2(Q x V);
	// V' = V + w*(T) + (Q x T)

	const FVector Q(X, Y, Z);
	const FVector T = 2.f * FVector::CrossProduct(Q, V);
	const FVector Result = V + (W * T) + FVector::CrossProduct(Q, T);
	return Result;
}

inline FVector FQuat::UnrotateVector(FVector V) const
{
	const FVector Q(-X, -Y, -Z); // Inverse
	const FVector T = 2.f * FVector::CrossProduct(Q, V);
	const FVector Result = V + (W * T) + FVector::CrossProduct(Q, T);
	return Result;
}


inline FQuat FQuat::Inverse() const
{
	assert(IsNormalized());

	return FQuat(-X, -Y, -Z, W);
}
inline void FQuat::EnforceShortestArcWith(const FQuat& OtherQuat)
{
	const float DotResult = (OtherQuat | *this);
	const float Bias = (float)FMath::FloatSelect(DotResult, 1.0f, -1.0f);

	X *= Bias;
	Y *= Bias;
	Z *= Bias;
	W *= Bias;
}
inline FVector FQuat::GetAxisX() const
{
	return RotateVector(FVector(1.f, 0.f, 0.f));
}


inline FVector FQuat::GetAxisY() const
{
	return RotateVector(FVector(0.f, 1.f, 0.f));
}
inline FVector FQuat::GetAxisZ() const
{
	return RotateVector(FVector(0.f, 0.f, 1.f));
}

inline FVector FQuat::GetForwardVector() const
{
	return GetAxisX();
}
inline FVector FQuat::GetRightVector() const
{
	return GetAxisY();
}
inline FVector FQuat::GetUpVector() const
{
	return GetAxisZ();
}
inline FVector FQuat::Vector() const
{
	return GetAxisX();
}
inline float FQuat::Error(const FQuat& Q1, const FQuat& Q2)
{
	const float cosom = FMath::Abs(Q1.X * Q2.X + Q1.Y * Q2.Y + Q1.Z * Q2.Z + Q1.W * Q2.W);
	return (FMath::Abs(cosom) < 0.9999999f) ? FMath::Acos(cosom)*(1.f / PI) : 0.0f;
}
inline float FQuat::ErrorAutoNormalize(const FQuat& A, const FQuat& B)
{
	FQuat Q1 = A;
	Q1.Normalize();

	FQuat Q2 = B;
	Q2.Normalize();

	return FQuat::Error(Q1, Q2);
}
inline FQuat FQuat::FastLerp(const FQuat& A, const FQuat& B, const float Alpha)
{
	// To ensure the 'shortest route', we make sure the dot product between the both rotations is positive.
	const float DotResult = (A | B);
	const float Bias = (float)FMath::FloatSelect((double)DotResult, 1.0, -1.0);
	return (B * Alpha) + (A * (Bias * (1.f - Alpha)));
}
inline FQuat FQuat::FastBilerp(const FQuat& P00, const FQuat& P10, const FQuat& P01, const FQuat& P11, float FracX, float FracY)
{
	return FQuat::FastLerp(
		FQuat::FastLerp(P00, P10, FracX),
		FQuat::FastLerp(P01, P11, FracX),
		FracY
	);
}
inline bool FQuat::ContainsNaN() const
{
	return (!FMath::IsFinite(X) ||
		!FMath::IsFinite(Y) ||
		!FMath::IsFinite(Z) ||
		!FMath::IsFinite(W)
		);
}
template<class U>
inline FQuat FMath::Lerp(const FQuat& A, const FQuat& B, const U& Alpha)
{
	return FQuat::Slerp(A, B, Alpha);
}

template<class U>
inline FQuat FMath::BiLerp(const FQuat& P00, const FQuat& P10, const FQuat& P01, const FQuat& P11, float FracX, float FracY)
{
	FQuat Result;

	Result = Lerp(
		FQuat::Slerp_NotNormalized(P00, P10, FracX),
		FQuat::Slerp_NotNormalized(P01, P11, FracX),
		FracY
	);

	return Result;
}
template<class U>
inline FQuat FMath::CubicInterp(const FQuat& P0, const FQuat& T0, const FQuat& P1, const FQuat& T1, const U& A)
{
	return FQuat::Squad(P0, T0, P1, T1, A);
}
/**
* Implements a basic sphere.
*/
class FSphere
{
public:
	FVector Center;
	float W;
public:
	FSphere() { }
	FSphere(int32)
		: Center(0.0f, 0.0f, 0.0f)
		, W(0)
	{ }
	FSphere(FVector InV, float InW)
		: Center(InV)
		, W(InW)
	{ }
	FSphere(const FVector* Pts, int32 Count);

public:
	bool Equals(const FSphere& Sphere, float Tolerance = KINDA_SMALL_NUMBER) const
	{
		return Center.Equals(Sphere.Center, Tolerance) && FMath::Abs(W - Sphere.W) <= Tolerance;
	}
	bool IsInside(const FSphere& Other, float Tolerance = KINDA_SMALL_NUMBER) const
	{
		if (W > Other.W + Tolerance)
		{
			return false;
		}

		return (Center - Other.Center).SizeSquared() <= FMath::Square(Other.W + Tolerance - W);
	}
	bool IsInside(const FVector& In, float Tolerance = KINDA_SMALL_NUMBER) const
	{
		return (Center - In).SizeSquared() <= FMath::Square(W + Tolerance);
	}
	inline bool Intersects(const FSphere& Other, float Tolerance = KINDA_SMALL_NUMBER) const
	{
		return (Center - Other.Center).SizeSquared() <= FMath::Square(FMath::Max(0.f, Other.W + W + Tolerance));
	}
	FSphere TransformBy(const FMatrix& M) const;
	FSphere TransformBy(const FTransform& M) const;
	float GetVolume() const;
	FSphere& operator+=(const FSphere& Other);
	FSphere operator+(const FSphere& Other) const
	{
		return FSphere(*this) += Other;
	}
};

/* Math inline functions
*****************************************************************************/

/**
* Converts a sphere into a point plus radius squared for the test above
*/
inline bool FMath::SphereAABBIntersection(const FSphere& Sphere, const FBox& AABB)
{
	float RadiusSquared = FMath::Square(Sphere.W);
	// If the distance is less than or equal to the radius, they intersect
	return SphereAABBIntersection(Sphere.Center, RadiusSquared, AABB);
}

/**
* Computes minimal bounding sphere encompassing given cone
*/
inline FSphere FMath::ComputeBoundingSphereForCone(FVector const& ConeOrigin, FVector const& ConeDirection, float ConeRadius, float CosConeAngle, float SinConeAngle)
{
	// Based on: https://bartwronski.com/2017/04/13/cull-that-cone/
	const float COS_PI_OVER_4 = 0.707107f; // Cos(Pi/4);
	if (CosConeAngle < COS_PI_OVER_4)
	{
		return FSphere(ConeOrigin + ConeDirection * ConeRadius * CosConeAngle, ConeRadius * SinConeAngle);
	}
	else
	{
		const float BoundingRadius = ConeRadius / (2.0f * CosConeAngle);
		return FSphere(ConeOrigin + ConeDirection * BoundingRadius, BoundingRadius);
	}
}
struct FBoxSphereBounds
{
	FVector	Origin;
	FVector BoxExtent;
	float SphereRadius;
public:
	FBoxSphereBounds() { }
	FBoxSphereBounds(const FVector& InOrigin, const FVector& InBoxExtent, float InSphereRadius)
		: Origin(InOrigin)
		, BoxExtent(InBoxExtent)
		, SphereRadius(InSphereRadius)
	{
		DiagnosticCheckNaN();
	}
	FBoxSphereBounds(const FBox& B, const FSphere& Sphere)
	{
		B.GetCenterAndExtents(Origin, BoxExtent);
		SphereRadius = FMath::Min(BoxExtent.Size(), (Sphere.Center - Origin).Size() + Sphere.W);

		DiagnosticCheckNaN();
	}
	FBoxSphereBounds(const FBox& B)
	{
		B.GetCenterAndExtents(Origin, BoxExtent);
		SphereRadius = BoxExtent.Size();

		DiagnosticCheckNaN();
	}
	FBoxSphereBounds(const FSphere& Sphere)
	{
		Origin = Sphere.Center;
		BoxExtent = FVector(Sphere.W);
		SphereRadius = Sphere.W;

		DiagnosticCheckNaN();
	}
	FBoxSphereBounds(const FVector* Points, uint32 NumPoints);
public:
	inline FBoxSphereBounds operator+(const FBoxSphereBounds& Other) const;
	inline bool operator==(const FBoxSphereBounds& Other) const;
	inline bool operator!=(const FBoxSphereBounds& Other) const;
public:
	inline float ComputeSquaredDistanceFromBoxToPoint(const FVector& Point) const
	{
		FVector Mins = Origin - BoxExtent;
		FVector Maxs = Origin + BoxExtent;

		return ::ComputeSquaredDistanceFromBoxToPoint(Mins, Maxs, Point);
	}
	inline static bool SpheresIntersect(const FBoxSphereBounds& A, const FBoxSphereBounds& B, float Tolerance = KINDA_SMALL_NUMBER)
	{
		return (A.Origin - B.Origin).SizeSquared() <= FMath::Square(FMath::Max(0.f, A.SphereRadius + B.SphereRadius + Tolerance));
	}
	inline static bool BoxesIntersect(const FBoxSphereBounds& A, const FBoxSphereBounds& B)
	{
		return A.GetBox().Intersect(B.GetBox());
	}
	inline FBox GetBox() const
	{
		return FBox(Origin - BoxExtent, Origin + BoxExtent);
	}
	FVector GetBoxExtrema(uint32 Extrema) const
	{
		if (Extrema)
		{
			return Origin + BoxExtent;
		}

		return Origin - BoxExtent;
	}
	inline FSphere GetSphere() const
	{
		return FSphere(Origin, SphereRadius);
	}
	inline FBoxSphereBounds ExpandBy(float ExpandAmount) const
	{
		return FBoxSphereBounds(Origin, BoxExtent + ExpandAmount, SphereRadius + ExpandAmount);
	}
	FBoxSphereBounds TransformBy(const FMatrix& M) const;
	FBoxSphereBounds TransformBy(const FTransform& M) const;
	friend FBoxSphereBounds Union(const FBoxSphereBounds& A, const FBoxSphereBounds& B)
	{
		FBox BoundingBox;

		BoundingBox += (A.Origin - A.BoxExtent);
		BoundingBox += (A.Origin + A.BoxExtent);
		BoundingBox += (B.Origin - B.BoxExtent);
		BoundingBox += (B.Origin + B.BoxExtent);

		// Build a bounding sphere from the bounding box's origin and the radii of A and B.
		FBoxSphereBounds Result(BoundingBox);

		Result.SphereRadius = FMath::Min(Result.SphereRadius, FMath::Max((A.Origin - Result.Origin).Size() + A.SphereRadius, (B.Origin - Result.Origin).Size() + B.SphereRadius));
		Result.DiagnosticCheckNaN();

		return Result;
	}
	inline void DiagnosticCheckNaN() const {}
	inline bool ContainsNaN() const
	{
		return Origin.ContainsNaN() || BoxExtent.ContainsNaN() || !FMath::IsFinite(SphereRadius);
	}
};
inline FBoxSphereBounds::FBoxSphereBounds(const FVector* Points, uint32 NumPoints)
{
	FBox BoundingBox;

	// find an axis aligned bounding box for the points.
	for (uint32 PointIndex = 0; PointIndex < NumPoints; PointIndex++)
	{
		BoundingBox += Points[PointIndex];
	}

	BoundingBox.GetCenterAndExtents(Origin, BoxExtent);

	// using the center of the bounding box as the origin of the sphere, find the radius of the bounding sphere.
	SphereRadius = 0.0f;

	for (uint32 PointIndex = 0; PointIndex < NumPoints; PointIndex++)
	{
		SphereRadius = FMath::Max(SphereRadius, (Points[PointIndex] - Origin).Size());
	}

	DiagnosticCheckNaN();
}
inline FBoxSphereBounds FBoxSphereBounds::operator+(const FBoxSphereBounds& Other) const
{
	FBox BoundingBox;

	BoundingBox += (this->Origin - this->BoxExtent);
	BoundingBox += (this->Origin + this->BoxExtent);
	BoundingBox += (Other.Origin - Other.BoxExtent);
	BoundingBox += (Other.Origin + Other.BoxExtent);

	// build a bounding sphere from the bounding box's origin and the radii of A and B.
	FBoxSphereBounds Result(BoundingBox);

	Result.SphereRadius = FMath::Min(Result.SphereRadius, FMath::Max((Origin - Result.Origin).Size() + SphereRadius, (Other.Origin - Result.Origin).Size() + Other.SphereRadius));
	Result.DiagnosticCheckNaN();

	return Result;
}
inline bool FBoxSphereBounds::operator==(const FBoxSphereBounds& Other) const
{
	return Origin == Other.Origin && BoxExtent == Other.BoxExtent &&  SphereRadius == Other.SphereRadius;
}
inline bool FBoxSphereBounds::operator!=(const FBoxSphereBounds& Other) const
{
	return !(*this == Other);
}
//https://stackoverflow.com/questions/27939882/fast-crc-algorithm
/* CRC-32C (iSCSI) polynomial in reversed bit order. */
#define POLY 0x82f63b78
/* CRC-32 (Ethernet, ZIP, etc.) polynomial in reversed bit order. */
/* #define POLY 0xedb88320 */
inline uint32 crc32c(const uint8 *buf, size_t len, uint32 crc = 0)
{
	int k;
	crc = ~crc;
	while (len--) {
		crc ^= *buf++;
		for (k = 0; k < 8; k++)
			crc = crc & 1 ? (crc >> 1) ^ POLY : crc >> 1;
	}
	return ~crc;
}

struct FRandomStream
{
	friend struct Z_Construct_UScriptStruct_FRandomStream_Statics;
public:
	FRandomStream()
		: InitialSeed(0)
		, Seed(0)
	{ }

	FRandomStream(int32 InSeed)
		: InitialSeed(InSeed)
		, Seed(InSeed)
	{ }

public:
	void Initialize(int32 InSeed)
	{
		InitialSeed = InSeed;
		Seed = InSeed;
	}

	void Reset() const
	{
		Seed = InitialSeed;
	}

	int32 GetInitialSeed() const
	{
		return InitialSeed;
	}

	void GenerateNewSeed()
	{
		Initialize(FMath::Rand());
	}

	float GetFraction() const
	{
		MutateSeed();

		const float SRandTemp = 1.0f;
		float Result;

		*(int32*)&Result = (*(int32*)&SRandTemp & 0xff800000) | (Seed & 0x007fffff);

		return FMath::Fractional(Result);
	}

	uint32 GetUnsignedInt() const
	{
		MutateSeed();

		return *(uint32*)&Seed;
	}

	FVector GetUnitVector() const
	{
		FVector Result;
		float L;

		do
		{
			// Check random vectors in the unit sphere so result is statistically uniform.
			Result.X = GetFraction() * 2.f - 1.f;
			Result.Y = GetFraction() * 2.f - 1.f;
			Result.Z = GetFraction() * 2.f - 1.f;
			L = Result.SizeSquared();
		} while (L > 1.f || L < KINDA_SMALL_NUMBER);

		return Result.GetUnsafeNormal();
	}

	int32 GetCurrentSeed() const
	{
		return Seed;
	}
	inline float FRand() const
	{
		return GetFraction();
	}
	inline int32 RandHelper(int32 A) const
	{
		// Can't just multiply GetFraction by A, as GetFraction could be == 1.0f
		return ((A > 0) ? FMath::TruncToInt(GetFraction() * ((float)A - DELTA)) : 0);
	}
	inline int32 RandRange(int32 Min, int32 Max) const
	{
		const int32 Range = (Max - Min) + 1;

		return Min + RandHelper(Range);
	}
	inline float FRandRange(float InMin, float InMax) const
	{
		return InMin + (InMax - InMin) * FRand();
	}
	inline FVector VRand() const
	{
		return GetUnitVector();
	}
	inline FVector VRandCone(FVector const& Dir, float ConeHalfAngleRad) const
	{
		if (ConeHalfAngleRad > 0.f)
		{
			float const RandU = FRand();
			float const RandV = FRand();

			// Get spherical coords that have an even distribution over the unit sphere
			// Method described at http://mathworld.wolfram.com/SpherePointPicking.html	
			float Theta = 2.f * PI * RandU;
			float Phi = FMath::Acos((2.f * RandV) - 1.f);

			// restrict phi to [0, ConeHalfAngleRad]
			// this gives an even distribution of points on the surface of the cone
			// centered at the origin, pointing upward (z), with the desired angle
			Phi = FMath::Fmod(Phi, ConeHalfAngleRad);

			// get axes we need to rotate around
			FMatrix const DirMat = FRotationMatrix(Dir.Rotation());
			// note the axis translation, since we want the variation to be around X
			FVector const DirZ = DirMat.GetUnitAxis(EAxis::X);
			FVector const DirY = DirMat.GetUnitAxis(EAxis::Y);

			FVector Result = Dir.RotateAngleAxis(Phi * 180.f / PI, DirY);
			Result = Result.RotateAngleAxis(Theta * 180.f / PI, DirZ);

			// ensure it's a unit vector (might not have been passed in that way)
			Result = Result.GetSafeNormal();

			return Result;
		}
		else
		{
			return Dir.GetSafeNormal();
		}
	}
	inline FVector VRandCone(FVector const& Dir, float HorizontalConeHalfAngleRad, float VerticalConeHalfAngleRad) const
	{
		if ((VerticalConeHalfAngleRad > 0.f) && (HorizontalConeHalfAngleRad > 0.f))
		{
			float const RandU = FRand();
			float const RandV = FRand();

			// Get spherical coords that have an even distribution over the unit sphere
			// Method described at http://mathworld.wolfram.com/SpherePointPicking.html	
			float Theta = 2.f * PI * RandU;
			float Phi = FMath::Acos((2.f * RandV) - 1.f);

			// restrict phi to [0, ConeHalfAngleRad]
			// where ConeHalfAngleRad is now a function of Theta
			// (specifically, radius of an ellipse as a function of angle)
			// function is ellipse function (x/a)^2 + (y/b)^2 = 1, converted to polar coords
			float ConeHalfAngleRad = FMath::Square(FMath::Cos(Theta) / VerticalConeHalfAngleRad) + FMath::Square(FMath::Sin(Theta) / HorizontalConeHalfAngleRad);
			ConeHalfAngleRad = FMath::Sqrt(1.f / ConeHalfAngleRad);

			// clamp to make a cone instead of a sphere
			Phi = FMath::Fmod(Phi, ConeHalfAngleRad);

			// get axes we need to rotate around
			FMatrix const DirMat = FRotationMatrix(Dir.Rotation());
			// note the axis translation, since we want the variation to be around X
			FVector const DirZ = DirMat.GetUnitAxis(EAxis::X);
			FVector const DirY = DirMat.GetUnitAxis(EAxis::Y);

			FVector Result = Dir.RotateAngleAxis(Phi * 180.f / PI, DirY);
			Result = Result.RotateAngleAxis(Theta * 180.f / PI, DirZ);

			// ensure it's a unit vector (might not have been passed in that way)
			Result = Result.GetSafeNormal();

			return Result;
		}
		else
		{
			return Dir.GetSafeNormal();
		}
	}
protected:
	void MutateSeed() const
	{
		Seed = (Seed * 196314165) + 907633515;
	}
private:
	int32 InitialSeed;
	mutable int32 Seed;
};

template <typename T>
inline constexpr T Align(T Val, uint64 Alignment)
{
	//static_assert(TIsIntegral<T>::Value || TIsPointer<T>::Value, "Align expects an integer or pointer type");

	return (T)(((uint64)Val + Alignment - 1) & ~(Alignment - 1));
}