#include "UnrealMath.h"

alignas(16) const Matrix Matrix::Identity(Plane(1, 0, 0, 0), Plane(0, 1, 0, 0), Plane(0, 0, 1, 0), Plane(0, 0, 0, 1));

Matrix::Matrix(const Plane& InX, const Plane& InY, const Plane& InZ, const Plane& InW)
{
	M[0][0] = InX.X; M[0][1] = InX.Y;  M[0][2] = InX.Z;  M[0][3] = InX.W;
	M[1][0] = InY.X; M[1][1] = InY.Y;  M[1][2] = InY.Z;  M[1][3] = InY.W;
	M[2][0] = InZ.X; M[2][1] = InZ.Y;  M[2][2] = InZ.Z;  M[2][3] = InZ.W;
	M[3][0] = InW.X; M[3][1] = InW.Y;  M[3][2] = InW.Z;  M[3][3] = InW.W;
}
inline Matrix::Matrix(const Vector& InX, const Vector& InY, const Vector& InZ, const Vector& InW)
{
	M[0][0] = InX.X; M[0][1] = InX.Y;  M[0][2] = InX.Z;  M[0][3] = 0.0f;
	M[1][0] = InY.X; M[1][1] = InY.Y;  M[1][2] = InY.Z;  M[1][3] = 0.0f;
	M[2][0] = InZ.X; M[2][1] = InZ.Y;  M[2][2] = InZ.Z;  M[2][3] = 0.0f;
	M[3][0] = InW.X; M[3][1] = InW.Y;  M[3][2] = InW.Z;  M[3][3] = 1.0f;
}
Matrix Matrix::GetTransposed() const
{
	Matrix	Result;

	Result.M[0][0] = M[0][0];
	Result.M[0][1] = M[1][0];
	Result.M[0][2] = M[2][0];
	Result.M[0][3] = M[3][0];

	Result.M[1][0] = M[0][1];
	Result.M[1][1] = M[1][1];
	Result.M[1][2] = M[2][1];
	Result.M[1][3] = M[3][1];

	Result.M[2][0] = M[0][2];
	Result.M[2][1] = M[1][2];
	Result.M[2][2] = M[2][2];
	Result.M[2][3] = M[3][2];

	Result.M[3][0] = M[0][3];
	Result.M[3][1] = M[1][3];
	Result.M[3][2] = M[2][3];
	Result.M[3][3] = M[3][3];

	return Result;
}

inline Vector Matrix::GetUnitAxis(EAxis::Type Axis) const
{
	return GetScaledAxis(Axis).GetSafeNormal();
}

Vector4 Matrix::TransformFVector4(const Vector4& P) const
{
	Vector4 Result;
	VectorRegister VecP = VectorLoadAligned(&P);
	VectorRegister VecR = VectorTransformVector(VecP, this);
	VectorStoreAligned(VecR, &Result);
	return Result;
}

Vector4 Matrix::TransformVector(const Vector& V) const
{
	return TransformFVector4(Vector4(V.X, V.Y, V.Z, 0.0f));
}

void Matrix::Mirror(EAxis::Type MirrorAxis, EAxis::Type FlipAxis)
{
	if (MirrorAxis == EAxis::X)
	{
		M[0][0] *= -1.f;
		M[1][0] *= -1.f;
		M[2][0] *= -1.f;

		M[3][0] *= -1.f;
	}
	else if (MirrorAxis == EAxis::Y)
	{
		M[0][1] *= -1.f;
		M[1][1] *= -1.f;
		M[2][1] *= -1.f;

		M[3][1] *= -1.f;
	}
	else if (MirrorAxis == EAxis::Z)
	{
		M[0][2] *= -1.f;
		M[1][2] *= -1.f;
		M[2][2] *= -1.f;

		M[3][2] *= -1.f;
	}

	if (FlipAxis == EAxis::X)
	{
		M[0][0] *= -1.f;
		M[0][1] *= -1.f;
		M[0][2] *= -1.f;
	}
	else if (FlipAxis == EAxis::Y)
	{
		M[1][0] *= -1.f;
		M[1][1] *= -1.f;
		M[1][2] *= -1.f;
	}
	else if (FlipAxis == EAxis::Z)
	{
		M[2][0] *= -1.f;
		M[2][1] *= -1.f;
		M[2][2] *= -1.f;
	}
}

void Matrix::RemoveScaling(float Tolerance /*= SMALL_NUMBER*/)
{
	const float SquareSum0 = (M[0][0] * M[0][0]) + (M[0][1] * M[0][1]) + (M[0][2] * M[0][2]);
	const float SquareSum1 = (M[1][0] * M[1][0]) + (M[1][1] * M[1][1]) + (M[1][2] * M[1][2]);
	const float SquareSum2 = (M[2][0] * M[2][0]) + (M[2][1] * M[2][1]) + (M[2][2] * M[2][2]);
	const float Scale0 = (float)Math::FloatSelect(SquareSum0 - Tolerance, Math::InvSqrt(SquareSum0), 1.0f);
	const float Scale1 = (float)Math::FloatSelect(SquareSum1 - Tolerance, Math::InvSqrt(SquareSum1), 1.0f);
	const float Scale2 = (float)Math::FloatSelect(SquareSum2 - Tolerance, Math::InvSqrt(SquareSum2), 1.0f);
	M[0][0] *= Scale0;
	M[0][1] *= Scale0;
	M[0][2] *= Scale0;
	M[1][0] *= Scale1;
	M[1][1] *= Scale1;
	M[1][2] *= Scale1;
	M[2][0] *= Scale2;
	M[2][1] *= Scale2;
	M[2][2] *= Scale2;
}

Vector Matrix::ExtractScaling(float Tolerance /*= SMALL_NUMBER*/)
{
	Vector Scale3D(0, 0, 0);

	// For each row, find magnitude, and if its non-zero re-scale so its unit length.
	const float SquareSum0 = (M[0][0] * M[0][0]) + (M[0][1] * M[0][1]) + (M[0][2] * M[0][2]);
	const float SquareSum1 = (M[1][0] * M[1][0]) + (M[1][1] * M[1][1]) + (M[1][2] * M[1][2]);
	const float SquareSum2 = (M[2][0] * M[2][0]) + (M[2][1] * M[2][1]) + (M[2][2] * M[2][2]);

	if (SquareSum0 > Tolerance)
	{
		float Scale0 = Math::Sqrt(SquareSum0);
		Scale3D[0] = Scale0;
		float InvScale0 = 1.f / Scale0;
		M[0][0] *= InvScale0;
		M[0][1] *= InvScale0;
		M[0][2] *= InvScale0;
	}
	else
	{
		Scale3D[0] = 0;
	}

	if (SquareSum1 > Tolerance)
	{
		float Scale1 = Math::Sqrt(SquareSum1);
		Scale3D[1] = Scale1;
		float InvScale1 = 1.f / Scale1;
		M[1][0] *= InvScale1;
		M[1][1] *= InvScale1;
		M[1][2] *= InvScale1;
	}
	else
	{
		Scale3D[1] = 0;
	}

	if (SquareSum2 > Tolerance)
	{
		float Scale2 = Math::Sqrt(SquareSum2);
		Scale3D[2] = Scale2;
		float InvScale2 = 1.f / Scale2;
		M[2][0] *= InvScale2;
		M[2][1] *= InvScale2;
		M[2][2] *= InvScale2;
	}
	else
	{
		Scale3D[2] = 0;
	}

	return Scale3D;
}

void Matrix::SetAxis(int32 i, const Vector& Axis)
{
	//checkSlow(i >= 0 && i <= 2);
	M[i][0] = Axis.X;
	M[i][1] = Axis.Y;
	M[i][2] = Axis.Z;
}

Matrix Matrix::DXLookToLH(const Vector& To)
{
	srand(unsigned int(To.SizeSquared()));
	Vector XDir = Vector((float)rand(), (float)rand(), (float)rand());
	XDir.Normalize();
	Vector ZDir = To;
	ZDir.Normalize();
	Vector YDir = -ZDir ^ XDir;
	XDir = YDir ^ -ZDir;
	XDir.Normalize();
	YDir.Normalize();
	ZDir.Normalize();
	//Vector::CreateOrthonormalBasis(XDir, YDir, ZDir);
	return Matrix
	(
		Plane(YDir, 0.0f),
		Plane(ZDir, 0.0f),
		Plane(XDir, 0.0f),
		Plane(0.0f, 0.0f, 0.0f, 1.0f)
	);
}

Matrix Matrix::DXReversedZFromPerspectiveFovLH(float fieldOfViewY, float aspectRatio, float zNearPlane, float zFarPlane)
{
	/*
	xScale     0          0               0
	0        yScale       0               0
	0          0       zn/(zn-zf)         1
	0          0       -zn*zf/(zn-zf)     0
	where:
	yScale = cot(fovY/2)

	xScale = yScale / aspect ratio
	*/
	Matrix Result;
	float Cot = 1.0f / tanf(0.5f * fieldOfViewY);
	float InverNF = zNearPlane / (zNearPlane - zFarPlane);

	Result.M[0][0] = Cot / aspectRatio;			Result.M[0][1] = 0.0f;		Result.M[0][2] = 0.0f;									Result.M[0][3] = 0.0f;
	Result.M[1][0] = 0.0f;						Result.M[1][1] = Cot;		Result.M[1][2] = 0.0f;									Result.M[1][3] = 0.0f;
	Result.M[2][0] = 0.0f;						Result.M[2][1] = 0.0f;		Result.M[2][2] = InverNF; 								Result.M[2][3] = 1.0f;
	Result.M[3][0] = 0.0f;						Result.M[3][1] = 0.0f;		Result.M[3][2] = -InverNF * zFarPlane;					Result.M[3][3] = 0.0f;
	return Result;
}

Matrix Matrix::DXFromOrthognalLH(float w, float h, float zn, float zf)
{
	Matrix Result;
	Result.M[0][0] = 2 * zn / w;		Result.M[0][1] = 0.0f;				Result.M[0][2] = 0.0f;									Result.M[0][3] = -1.0f;
	Result.M[1][0] = 0.0f;				Result.M[1][1] = 2.0f * zn / h;		Result.M[1][2] = 0.0f;									Result.M[1][3] = -1.0f;
	Result.M[2][0] = 0.0f;				Result.M[2][1] = 0.0f;				Result.M[2][2] = 1 / (zf - zn);							Result.M[2][3] = 1.0f;
	Result.M[3][0] = 0.0f;				Result.M[3][1] = 0.0f;				Result.M[3][2] = -zn  / (zn - zf);						Result.M[3][3] = 0.0f;
	return Result;
}

Matrix Matrix::DXFromOrthognalLH(float r, float l, float t, float b, float zf, float zn)
{
	Matrix Result;
	Result.M[0][0] = 2.0f / (r - l);		Result.M[0][1] = 0.0f;					Result.M[0][2] = 0.0f;							Result.M[0][3] = 0.0f;
	Result.M[1][0] = 0.0f;					Result.M[1][1] = 2.0f / (t - b);		Result.M[1][2] = 0.0f;							Result.M[1][3] = 0.0f;
	Result.M[2][0] = 0.0f;					Result.M[2][1] = 0.0f;					Result.M[2][2] = 1.0f / (zf - zn);				Result.M[2][3] = 0.0f;
	Result.M[3][0] = -(r + l) / (r - l);	Result.M[3][1] = (t + b) / (t - b);		Result.M[3][2] = -zn / (zf - zn);				Result.M[3][3] = 1.0f;
	return Result;
}

float Vector::SizeSquared() const
{
	return X * X + Y * Y + Z * Z;
}

void Vector::CreateOrthonormalBasis(Vector& XAxis, Vector& YAxis, Vector& ZAxis)
{
	// Project the X and Y axes onto the plane perpendicular to the Z axis.
	XAxis -= (XAxis | ZAxis) / (ZAxis | ZAxis) * ZAxis;
	YAxis -= (YAxis | ZAxis) / (ZAxis | ZAxis) * ZAxis;

	// If the X axis was parallel to the Z axis, choose a vector which is orthogonal to the Y and Z axes.
	if (XAxis.SizeSquared() < DELTA*DELTA)
	{
		XAxis = YAxis ^ ZAxis;
	}

	// If the Y axis was parallel to the Z axis, choose a vector which is orthogonal to the X and Z axes.
	if (YAxis.SizeSquared() < DELTA*DELTA)
	{
		YAxis = XAxis ^ ZAxis;
	}

	// Normalize the basis vectors.
	XAxis.Normalize();
	YAxis.Normalize();
	ZAxis.Normalize();
}

float Vector::Triple(const Vector& X, const Vector& Y, const Vector& Z)
{
	return
		((X.X * (Y.Y * Z.Z - Y.Z * Z.Y))
			+ (X.Y * (Y.Z * Z.X - Y.X * Z.Z))
			+ (X.Z * (Y.X * Z.Y - Y.Y * Z.X)));
}

bool Vector::Equals(const Vector& V, float Tolerance /*= KINDA_SMALL_NUMBER*/) const
{
	return Math::Abs(X - V.X) <= Tolerance && Math::Abs(Y - V.Y) <= Tolerance && Math::Abs(Z - V.Z) <= Tolerance;
}

struct Vector2 Vector::ToVector2() const
{
	return Vector2(X,Y);
}

float Vector::Size() const
{
	return Math::Sqrt(X*X + Y * Y + Z * Z);
}

bool Vector::IsZero() const
{
	return X == 0.f && Y == 0.f && Z == 0.f;
}

Vector Vector::Reciprocal() const
{
	Vector RecVector;
	if (X != 0.f)
	{
		RecVector.X = 1.f / X;
	}
	else
	{
		RecVector.X = BIG_NUMBER;
	}
	if (Y != 0.f)
	{
		RecVector.Y = 1.f / Y;
	}
	else
	{
		RecVector.Y = BIG_NUMBER;
	}
	if (Z != 0.f)
	{
		RecVector.Z = 1.f / Z;
	}
	else
	{
		RecVector.Z = BIG_NUMBER;
	}
	return RecVector;
}

static const float OneOver255 = 1.0f / 255.0f;

LinearColor::LinearColor(struct FColor InColor)
{
	R = (float)sRGBToLinearTable[InColor.R];
	G = (float)sRGBToLinearTable[InColor.G];
	B = (float)sRGBToLinearTable[InColor.B];
	A = float(InColor.A) * OneOver255;
}

double LinearColor::sRGBToLinearTable[256] =
{
	0,
	0.000303526983548838, 0.000607053967097675, 0.000910580950646512, 0.00121410793419535, 0.00151763491774419,
	0.00182116190129302, 0.00212468888484186, 0.0024282158683907, 0.00273174285193954, 0.00303526983548838,
	0.00334653564113713, 0.00367650719436314, 0.00402471688178252, 0.00439144189356217, 0.00477695332960869,
	0.005181516543916, 0.00560539145834456, 0.00604883284946662, 0.00651209061157708, 0.00699540999852809,
	0.00749903184667767, 0.00802319278093555, 0.0085681254056307, 0.00913405848170623, 0.00972121709156193,
	0.0103298227927056, 0.0109600937612386, 0.0116122449260844, 0.012286488094766, 0.0129830320714536,
	0.0137020827679224, 0.0144438433080002, 0.0152085141260192, 0.0159962930597398, 0.0168073754381669,
	0.0176419541646397, 0.0185002197955389, 0.0193823606149269, 0.0202885627054049, 0.0212190100154473,
	0.0221738844234532, 0.02315336579873, 0.0241576320596103, 0.0251868592288862, 0.0262412214867272,
	0.0273208912212394, 0.0284260390768075, 0.0295568340003534, 0.0307134432856324, 0.0318960326156814,
	0.0331047661035236, 0.0343398063312275, 0.0356013143874111, 0.0368894499032755, 0.0382043710872463,
	0.0395462347582974, 0.0409151963780232, 0.0423114100815264, 0.0437350287071788, 0.0451862038253117,
	0.0466650857658898, 0.0481718236452158, 0.049706565391714, 0.0512694577708345, 0.0528606464091205,
	0.0544802758174765, 0.0561284894136735, 0.0578054295441256, 0.0595112375049707, 0.0612460535624849,
	0.0630100169728596, 0.0648032660013696, 0.0666259379409563, 0.0684781691302512, 0.070360094971063,
	0.0722718499453493, 0.0742135676316953, 0.0761853807213167, 0.0781874210336082, 0.0802198195312533,
	0.0822827063349132, 0.0843762107375113, 0.0865004612181274, 0.0886555854555171, 0.0908417103412699,
	0.0930589619926197, 0.0953074657649191, 0.0975873462637915, 0.0998987273569704, 0.102241732185838,
	0.104616483176675, 0.107023102051626, 0.109461709839399, 0.1119324268857, 0.114435372863418,
	0.116970666782559, 0.119538426999953, 0.122138771228724, 0.124771816547542, 0.127437679409664,
	0.130136475651761, 0.132868320502552, 0.135633328591233, 0.138431613955729, 0.141263290050755,
	0.144128469755705, 0.147027265382362, 0.149959788682454, 0.152926150855031, 0.155926462553701,
	0.158960833893705, 0.162029374458845, 0.16513219330827, 0.168269398983119, 0.171441099513036,
	0.174647402422543, 0.17788841473729, 0.181164242990184, 0.184474993227387, 0.187820771014205,
	0.191201681440861, 0.194617829128147, 0.198069318232982, 0.201556252453853, 0.205078735036156,
	0.208636868777438, 0.212230756032542, 0.215860498718652, 0.219526198320249, 0.223227955893977,
	0.226965872073417, 0.23074004707378, 0.23455058069651, 0.238397572333811, 0.242281120973093,
	0.246201325201334, 0.250158283209375, 0.254152092796134, 0.258182851372752, 0.262250655966664,
	0.266355603225604, 0.270497789421545, 0.274677310454565, 0.278894261856656, 0.283148738795466,
	0.287440836077983, 0.291770648154158, 0.296138269120463, 0.300543792723403, 0.304987312362961,
	0.309468921095997, 0.313988711639584, 0.3185467763743, 0.323143207347467, 0.32777809627633,
	0.332451534551205, 0.337163613238559, 0.341914423084057, 0.346704054515559, 0.351532597646068,
	0.356400142276637, 0.361306777899234, 0.36625259369956, 0.371237678559833, 0.376262121061519,
	0.381326009488037, 0.386429431827418, 0.39157247577492, 0.396755228735618, 0.401977777826949,
	0.407240209881218, 0.41254261144808, 0.417885068796976, 0.423267667919539, 0.428690494531971,
	0.434153634077377, 0.439657171728079, 0.445201192387887, 0.450785780694349, 0.456411021020965,
	0.462076997479369, 0.467783793921492, 0.473531493941681, 0.479320180878805, 0.485149937818323,
	0.491020847594331, 0.496932992791578, 0.502886455747457, 0.50888131855397, 0.514917663059676,
	0.520995570871595, 0.527115123357109, 0.533276401645826, 0.539479486631421, 0.545724458973463,
	0.552011399099209, 0.558340387205378, 0.56471150325991, 0.571124827003694, 0.577580437952282,
	0.584078415397575, 0.590618838409497, 0.597201785837643, 0.603827336312907, 0.610495568249093,
	0.617206559844509, 0.623960389083534, 0.630757133738175, 0.637596871369601, 0.644479679329661,
	0.651405634762384, 0.658374814605461, 0.665387295591707, 0.672443154250516, 0.679542466909286,
	0.686685309694841, 0.693871758534824, 0.701101889159085, 0.708375777101046, 0.71569349769906,
	0.723055126097739, 0.730460737249286, 0.737910405914797, 0.745404206665559, 0.752942213884326,
	0.760524501766589, 0.768151144321824, 0.775822215374732, 0.783537788566466, 0.791297937355839,
	0.799102735020525, 0.806952254658248, 0.81484656918795, 0.822785751350956, 0.830769873712124,
	0.838799008660978, 0.846873228412837, 0.854992605009927, 0.863157210322481, 0.871367116049835,
	0.879622393721502, 0.887923114698241, 0.896269350173118, 0.904661171172551, 0.913098648557343,
	0.921581853023715, 0.930110855104312, 0.938685725169219, 0.947306533426946, 0.955973349925421,
	0.964686244552961, 0.973445287039244, 0.982250546956257, 0.991102093719252, 1.0,
};

Box2D Frustum::GetBounds2D(const Matrix& ViewMatrix, const Vector& Axis1, const Vector& Axis2)
{
	Box2D Result;
	for (Vector V : Vertices)
	{
		V = ViewMatrix.Transform(V);
		Vector PrjectV1 = V * Axis1;
		Vector PrjectV2 = V * Axis2;
		Result += PrjectV1.ToVector2();
		Result += PrjectV2.ToVector2();
	}
	return Result;
}

FBox Frustum::GetBounds(const Matrix& TransformMatrix)
{
	FBox Result;
	for (const Vector& V : Vertices)
	{
		Result += TransformMatrix.Transform(V);
	}
	return Result;
}

FBox Frustum::GetBounds()
{
	FBox Result;
	for (const Vector& V : Vertices)
	{
		Result += V;
	}
	return Result;
}

bool Vector2::Equals(const Vector2& V, float Tolerance /*= KINDA_SMALL_NUMBER*/) const
{
	return Math::Abs(X - V.X) <= Tolerance && Math::Abs(Y - V.Y) <= Tolerance;
}

bool Vector2::IsNearlyZero(float Tolerance /*= KINDA_SMALL_NUMBER*/) const
{
	return	Math::Abs(X) <= Tolerance && Math::Abs(Y) <= Tolerance;
}

float Math::Atan2(float Y, float X)
{
	//return atan2f(Y,X);
	// atan2f occasionally returns NaN with perfectly valid input (possibly due to a compiler or library bug).
	// We are replacing it with a minimax approximation with a max relative error of 7.15255737e-007 compared to the C library function.
	// On PC this has been measured to be 2x faster than the std C version.

	const float absX = Math::Abs(X);
	const float absY = Math::Abs(Y);
	const bool yAbsBigger = (absY > absX);
	float t0 = yAbsBigger ? absY : absX; // Max(absY, absX)
	float t1 = yAbsBigger ? absX : absY; // Min(absX, absY)

	if (t0 == 0.f)
		return 0.f;

	float t3 = t1 / t0;
	float t4 = t3 * t3;

	static const float c[7] = {
		+7.2128853633444123e-03f,
		-3.5059680836411644e-02f,
		+8.1675882859940430e-02f,
		-1.3374657325451267e-01f,
		+1.9856563505717162e-01f,
		-3.3324998579202170e-01f,
		+1.0f
	};

	t0 = c[0];
	t0 = t0 * t4 + c[1];
	t0 = t0 * t4 + c[2];
	t0 = t0 * t4 + c[3];
	t0 = t0 * t4 + c[4];
	t0 = t0 * t4 + c[5];
	t0 = t0 * t4 + c[6];
	t3 = t0 * t3;

	t3 = yAbsBigger ? (0.5f * PI) - t3 : t3;
	t3 = (X < 0.0f) ? PI - t3 : t3;
	t3 = (Y < 0.0f) ? -t3 : t3;

	return t3;
}

bool Math::SphereAABBIntersection(const Vector& SphereCenter, const float RadiusSquared, const FBox& AABB)
{
	float DistSquared = 0.f;
	// Check each axis for min/max and add the distance accordingly
	// NOTE: Loop manually unrolled for > 2x speed up
	if (SphereCenter.X < AABB.Min.X)
	{
		DistSquared += Math::Square(SphereCenter.X - AABB.Min.X);
	}
	else if (SphereCenter.X > AABB.Max.X)
	{
		DistSquared += Math::Square(SphereCenter.X - AABB.Max.X);
	}
	if (SphereCenter.Y < AABB.Min.Y)
	{
		DistSquared += Math::Square(SphereCenter.Y - AABB.Min.Y);
	}
	else if (SphereCenter.Y > AABB.Max.Y)
	{
		DistSquared += Math::Square(SphereCenter.Y - AABB.Max.Y);
	}
	if (SphereCenter.Z < AABB.Min.Z)
	{
		DistSquared += Math::Square(SphereCenter.Z - AABB.Min.Z);
	}
	else if (SphereCenter.Z > AABB.Max.Z)
	{
		DistSquared += Math::Square(SphereCenter.Z - AABB.Max.Z);
	}
	// If the distance is less than or equal to the radius, they intersect
	return DistSquared <= RadiusSquared;
}

Vector4 Vector4::GetSafeNormal(float Tolerance /*= SMALL_NUMBER*/) const
{
	const float SquareSum = X * X + Y * Y + Z * Z;
	if (SquareSum > Tolerance)
	{
		const float Scale = Math::InvSqrt(SquareSum);
		return Vector4(X*Scale, Y*Scale, Z*Scale, 0.0f);
	}
	return Vector4(0.f);
}

FQuat Rotator::Quaternion() const
{
	const float DEG_TO_RAD = PI / (180.f);
	const float DIVIDE_BY_2 = DEG_TO_RAD / 2.f;
	float SP, SY, SR;
	float CP, CY, CR;

	Math::SinCos(&SP, &CP, Pitch*DIVIDE_BY_2);
	Math::SinCos(&SY, &CY, Yaw*DIVIDE_BY_2);
	Math::SinCos(&SR, &CR, Roll*DIVIDE_BY_2);

	FQuat RotationQuat;
	RotationQuat.X = CR * SP*SY - SR * CP*CY;
	RotationQuat.Y = -CR * SP*CY - SR * CP*SY;
	RotationQuat.Z = CR * CP*SY - SR * SP*CY;
	RotationQuat.W = CR * CP*CY + SR * SP*SY;
	return RotationQuat;
}

Rotator FQuat::FRotator() const
{
	//SCOPE_CYCLE_COUNTER(STAT_MathConvertQuatToRotator);

	DiagnosticCheckNaN();
	const float SingularityTest = Z * X - W * Y;
	const float YawY = 2.f*(W*Z + X * Y);
	const float YawX = (1.f - 2.f*(Math::Square(Y) + Math::Square(Z)));

	// reference 
	// http://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles
	// http://www.euclideanspace.com/maths/geometry/rotations/conversions/quaternionToEuler/

	// this value was found from experience, the above websites recommend different values
	// but that isn't the case for us, so I went through different testing, and finally found the case 
	// where both of world lives happily. 
	const float SINGULARITY_THRESHOLD = 0.4999995f;
	const float RAD_TO_DEG = (180.f) / PI;
	Rotator RotatorFromQuat;

	if (SingularityTest < -SINGULARITY_THRESHOLD)
	{
		RotatorFromQuat.Pitch = -90.f;
		RotatorFromQuat.Yaw = Math::Atan2(YawY, YawX) * RAD_TO_DEG;
		RotatorFromQuat.Roll = Rotator::NormalizeAxis(-RotatorFromQuat.Yaw - (2.f * Math::Atan2(X, W) * RAD_TO_DEG));
	}
	else if (SingularityTest > SINGULARITY_THRESHOLD)
	{
		RotatorFromQuat.Pitch = 90.f;
		RotatorFromQuat.Yaw = Math::Atan2(YawY, YawX) * RAD_TO_DEG;
		RotatorFromQuat.Roll = Rotator::NormalizeAxis(RotatorFromQuat.Yaw - (2.f * Math::Atan2(X, W) * RAD_TO_DEG));
	}
	else
	{
		RotatorFromQuat.Pitch = Math::FastAsin(2.f*(SingularityTest)) * RAD_TO_DEG;
		RotatorFromQuat.Yaw = Math::Atan2(YawY, YawX) * RAD_TO_DEG;
		RotatorFromQuat.Roll = Math::Atan2(-2.f*(W*X + Y * Z), (1.f - 2.f*(Math::Square(X) + Math::Square(Y)))) * RAD_TO_DEG;
	}

	if (RotatorFromQuat.ContainsNaN())
	{
		//logOrEnsureNanError(TEXT("FQuat::Rotator(): Rotator result %s contains NaN! Quat = %s, YawY = %.9f, YawX = %.9f"), *RotatorFromQuat.ToString(), *this->ToString(), YawY, YawX);
		RotatorFromQuat = Rotator::ZeroRotator;
	}

	return RotatorFromQuat;
}

const Vector Vector::ZeroVector(0.0f, 0.0f, 0.0f);
const Vector Vector::OneVector(1.0f, 1.0f, 1.0f);
const Vector Vector::UpVector(0.0f, 0.0f, 1.0f);
const Vector Vector::ForwardVector(1.0f, 0.0f, 0.0f);
const Vector Vector::RightVector(0.0f, 1.0f, 0.0f);
const Vector2 Vector2::ZeroVector(0.0f, 0.0f);
const Vector2 Vector2::UnitVector(1.0f, 1.0f);
const Rotator Rotator::ZeroRotator(0.f, 0.f, 0.f);
const FQuat FQuat::Identity(0, 0, 0, 1);
const FColor FColor::White(255, 255, 255);

float ComputeSquaredDistanceFromBoxToPoint(const Vector& Mins, const Vector& Maxs, const Vector& Point)
{
	// Accumulates the distance as we iterate axis
	float DistSquared = 0.f;

	// Check each axis for min/max and add the distance accordingly
	// NOTE: Loop manually unrolled for > 2x speed up
	if (Point.X < Mins.X)
	{
		DistSquared += Math::Square(Point.X - Mins.X);
	}
	else if (Point.X > Maxs.X)
	{
		DistSquared += Math::Square(Point.X - Maxs.X);
	}

	if (Point.Y < Mins.Y)
	{
		DistSquared += Math::Square(Point.Y - Mins.Y);
	}
	else if (Point.Y > Maxs.Y)
	{
		DistSquared += Math::Square(Point.Y - Maxs.Y);
	}

	if (Point.Z < Mins.Z)
	{
		DistSquared += Math::Square(Point.Z - Mins.Z);
	}
	else if (Point.Z > Maxs.Z)
	{
		DistSquared += Math::Square(Point.Z - Maxs.Z);
	}

	return DistSquared;
}
