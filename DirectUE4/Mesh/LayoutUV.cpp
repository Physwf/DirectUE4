#include "LayoutUV.h"
#include "DisjoinSet.h"
#include <algorithm>
#include <unordered_map>

#define CHART_JOINING	1
namespace MeshDescriptionOp
{
	FLayoutUV::FLayoutUV(MeshDescription& InMesh, uint32 InSrcChannel, uint32 InDstChannel, uint32 InTextureResolution)
		: MD(InMesh)
		, SrcChannel(InSrcChannel)
		, DstChannel(InDstChannel)
		, TextureResolution(InTextureResolution)
		, TotalUVArea(0.0f)
		, LayoutRaster(TextureResolution, TextureResolution)
		, ChartRaster(TextureResolution, TextureResolution)
		, BestChartRaster(TextureResolution, TextureResolution)
		, ChartShader(&ChartRaster)
		, LayoutVersion(MeshDescriptionOperations::ELightmapUVVersion::Latest)
	{}

	void FLayoutUV::FindCharts(const std::multimap<int32, int32>& OverlappingCorners)
	{
		const float ThreshUVsAreSame = GetUVEqualityThreshold();
		//double Begin = FPlatformTime::Seconds();

		uint32 NumTris = 0;
		for (const int PolygonID : MD.Polygons().GetElementIDs())
		{
			NumTris += MD.GetPolygonTriangles(PolygonID).size();
		}
		uint32 NumIndexes = NumTris * 3;

		std::vector< int32 > TranslatedMatches;
		TranslatedMatches.resize(NumIndexes);
		TexCoords.resize(NumIndexes);
		int32 WedgeIndex = 0;
		RemapVerts.resize(NumIndexes);

		const std::vector<Vector2>& VertexUVs = MD.VertexInstanceAttributes().GetAttributes<Vector2>(MeshAttribute::VertexInstance::TextureCoordinate, SrcChannel);

		for (const int PolygonID : MD.Polygons().GetElementIDs())
		{
			const std::vector<MeshTriangle>& Triangles = MD.GetPolygonTriangles(PolygonID);
			for (const MeshTriangle MTri : Triangles)
			{
				for (int32 Corner = 0; Corner < 3; ++Corner)
				{
					const int VertexInstanceID = MTri.GetVertexInstanceID(Corner);

					TranslatedMatches[WedgeIndex] = -1;
					TexCoords[WedgeIndex] = VertexUVs[VertexInstanceID];
					RemapVerts[WedgeIndex] = VertexInstanceID;
					++WedgeIndex;
				}
			}
		}

		// Build disjoint set
		FDisjointSet DisjointSet(NumTris);
		for (uint32 i = 0; i < NumIndexes; i++)
		{
			//for (auto It = OverlappingCorners.CreateConstKeyIterator(i); It; ++It)
			auto range = OverlappingCorners.equal_range(i);
			for (auto It = range.first; It != range.second; ++It)
			{
				uint32 j = It->second;

				if (j > i)
				{
					const uint32 TriI = i / 3;
					const uint32 TriJ = j / 3;

					bool bUnion = false;

#if CHART_JOINING
					bool bPositionMatch = PositionsMatch(i, j);
					if (bPositionMatch)
					{
						uint32 i1 = 3 * TriI + (i + 1) % 3;
						uint32 i2 = 3 * TriI + (i + 2) % 3;
						uint32 j1 = 3 * TriJ + (j + 1) % 3;
						uint32 j2 = 3 * TriJ + (j + 2) % 3;

						bool bEdgeMatch21 = PositionsMatch(i2, j1);
						bool bEdgeMatch12 = PositionsMatch(i1, j2);
						if (bEdgeMatch21 || bEdgeMatch12)
						{
							uint32 ie = bEdgeMatch21 ? i2 : i1;
							uint32 je = bEdgeMatch21 ? j1 : j2;

							bool bUVMatch = UVsMatch(i, j) && UVsMatch(ie, je);
							bool bUVWindingMatch = TriangleUVArea(TriI) * TriangleUVArea(TriJ) >= 0.0f;
							if (bUVMatch && bUVWindingMatch)
							{
								bUnion = true;
							}
							else if (NormalsMatch(i, j) && NormalsMatch(ie, je))
							{
								// Chart edge
								Vector2 EdgeUVi = TexCoords[ie] - TexCoords[i];
								Vector2 EdgeUVj = TexCoords[je] - TexCoords[j];

								// Would these edges match if the charts were translated
								bool bTranslatedUVMatch = (EdgeUVi - EdgeUVj).IsNearlyZero(ThreshUVsAreSame);
								if (bTranslatedUVMatch)
								{
									// Note: may be mirrored

									// TODO should these be restricted to axis aligned edges?
									uint32 EdgeI = bEdgeMatch21 ? i2 : i;
									uint32 EdgeJ = bEdgeMatch21 ? j : j2;

									// Only allow one match per edge
									if (TranslatedMatches[EdgeI] < 0 &&
										TranslatedMatches[EdgeJ] < 0)
									{
										TranslatedMatches[EdgeI] = EdgeJ;
										TranslatedMatches[EdgeJ] = EdgeI;
									}
								}
							}
						}
					}
#else
					if (VertsMatch(i, j))
					{
						// Edge must match as well (same winding)
						if (VertsMatch(3 * TriI + (i - 1) % 3, 3 * TriJ + (j + 1) % 3) ||
							VertsMatch(3 * TriI + (i + 1) % 3, 3 * TriJ + (j - 1) % 3))
						{
							// Check for UV winding match too
							if (TriangleUVArea(TriI) * TriangleUVArea(TriJ) >= 0.0f)
							{
								bUnion = true;
							}
						}
					}
#endif

					if (bUnion)
					{
						// TODO solve spiral case by checking sets for UV overlap
						DisjointSet.Union(TriI, TriJ);
					}
				}
			}
		}

		// Sort tris by chart
		//SortedTris.SetNumUninitialized(NumTris);
		SortedTris.resize(NumTris);
		for (uint32 i = 0; i < NumTris; i++)
		{
			// Flatten disjoint set path
			DisjointSet.Find(i);
			SortedTris[i] = i;
		}

		struct FCompareTris
		{
			FDisjointSet* DisjointSet;

			FCompareTris(FDisjointSet* InDisjointSet)
				: DisjointSet(InDisjointSet)
			{}

			inline bool operator()(uint32 A, uint32 B) const
			{
				return (*DisjointSet)[A] < (*DisjointSet)[B];
			}
		};

		std::sort(SortedTris.begin(), SortedTris.end(), FCompareTris(&DisjointSet));
		//Algo::IntroSort(SortedTris, FCompareTris(&DisjointSet));

		std::map< uint32, int32 > DisjointSetToChartMap;

		const std::vector<Vector>& VertexPositions = MD.VertexAttributes().GetAttributes<Vector>(MeshAttribute::Vertex::Position);

		// Build Charts
		for (uint32 Tri = 0; Tri < NumTris; )
		{
			Charts.push_back(FMeshChart());
			size_t i = Charts.size() - 1;
			FMeshChart& Chart = Charts[i];

			Chart.MinUV = Vector2(FLT_MAX, FLT_MAX);
			Chart.MaxUV = Vector2(-FLT_MAX, -FLT_MAX);
			Chart.UVArea = 0.0f;
			Chart.WorldScale = Vector2(0,0);
			memset(Chart.Join, 0xff,sizeof(Chart.Join));
			//FMemory::Memset(Chart.Join, 0xff);

			Chart.FirstTri = Tri;

			uint32 ChartID = DisjointSet[SortedTris[Tri]];
			DisjointSetToChartMap.insert(std::make_pair(ChartID, i));

			for (; Tri < NumTris && DisjointSet[SortedTris[Tri]] == ChartID; Tri++)
			{
				// Calculate chart bounds
				Vector		Positions[3];
				Vector2		UVs[3];
				for (int k = 0; k < 3; k++)
				{
					uint32 Index = 3 * SortedTris[Tri] + k;

					int VertexInstanceID(RemapVerts[Index]);
					Positions[k] = VertexPositions[MD.GetVertexInstanceVertex(VertexInstanceID)];
					UVs[k] = TexCoords[Index];

					Chart.MinUV.X = Math::Min(Chart.MinUV.X, UVs[k].X);
					Chart.MinUV.Y = Math::Min(Chart.MinUV.Y, UVs[k].Y);
					Chart.MaxUV.X = Math::Max(Chart.MaxUV.X, UVs[k].X);
					Chart.MaxUV.Y = Math::Max(Chart.MaxUV.Y, UVs[k].Y);
				}

				Vector Edge1 = Positions[1] - Positions[0];
				Vector Edge2 = Positions[2] - Positions[0];
				float Area = 0.5f * (Edge1 ^ Edge2).Size();

				Vector2 EdgeUV1 = UVs[1] - UVs[0];
				Vector2 EdgeUV2 = UVs[2] - UVs[0];
				float UVArea = 0.5f * Math::Abs(EdgeUV1.X * EdgeUV2.Y - EdgeUV1.Y * EdgeUV2.X);

				Vector2 UVLength;
				UVLength.X = (EdgeUV2.Y * Edge1 - EdgeUV1.Y * Edge2).Size();
				UVLength.Y = (-EdgeUV2.X * Edge1 + EdgeUV1.X * Edge2).Size();

				Chart.WorldScale += UVLength;
				Chart.UVArea += UVArea;
			}

			Chart.LastTri = Tri;

#if !CHART_JOINING
			if (LayoutVersion >= MeshDescriptionOperations::ELightmapUVVersion::SmallChartPacking)
			{
				Chart.WorldScale /= Math::Max(Chart.UVArea, 1e-8f);
			}
			else
			{
				if (Chart.UVArea > 1e-4f)
				{
					Chart.WorldScale /= Chart.UVArea;
				}
				else
				{
					Chart.WorldScale = Vector2(0,0);
				}
			}

			TotalUVArea += Chart.UVArea * Chart.WorldScale.X * Chart.WorldScale.Y;
#endif
		}

#if CHART_JOINING
		for (size_t i = 0; i < Charts.size(); i++)
		{
			FMeshChart& Chart = Charts[i];

			for (uint32 Tri = Chart.FirstTri; Tri < Chart.LastTri; Tri++)
			{
				for (int k = 0; k < 3; k++)
				{
					uint32 Index = 3 * SortedTris[Tri] + k;

					if (TranslatedMatches[Index] >= 0)
					{
						//checkSlow(TranslatedMatches[TranslatedMatches[Index]] == Index);

						uint32 V0i = Index;
						uint32 V0j = TranslatedMatches[Index];

						uint32 TriI = V0i / 3;
						uint32 TriJ = V0j / 3;

						if (TriJ <= TriI)
						{
							// Only need to consider one direction
							continue;
						}

						uint32 V1i = 3 * TriI + (V0i + 1) % 3;
						uint32 V1j = 3 * TriJ + (V0j + 1) % 3;

						int32 ChartI = i;
						int32 ChartJ = DisjointSetToChartMap[DisjointSet[TriJ]];

						Vector2 UV0i = TexCoords[V0i];
						Vector2 UV1i = TexCoords[V1i];
						Vector2 UV0j = TexCoords[V0j];
						Vector2 UV1j = TexCoords[V1j];

						Vector2 EdgeUVi = UV1i - UV0i;
						Vector2 EdgeUVj = UV1j - UV0j;

						bool bMirrored = TriangleUVArea(TriI) * TriangleUVArea(TriJ) < 0.0f;

						Vector2 EdgeOffset0 = UV0i - UV1j;
						Vector2 EdgeOffset1 = UV1i - UV0j;
						//checkSlow((EdgeOffset0 - EdgeOffset1).IsNearlyZero(ThreshUVsAreSame));

						Vector2 Translation = EdgeOffset0;

						FMeshChart& ChartA = Charts[ChartI];
						FMeshChart& ChartB = Charts[ChartJ];

						for (uint32 Side = 0; Side < 4; Side++)
						{
							// Join[] = { left, right, bottom, top }

							// FIXME
							if (bMirrored)
								continue;

							if (ChartA.Join[Side ^ 0] != -1 ||
								ChartB.Join[Side ^ 1] != -1)
							{
								// Already joined with something else
								continue;
							}

							uint32 Sign = Side & 1;
							uint32 Axis = Side >> 1;

							bool bAxisAligned = Math::Abs(EdgeUVi[Axis]) < ThreshUVsAreSame;
							bool bBorderA = Math::Abs(UV0i[Axis] - (Sign ^ 0 ? Chart.MaxUV[Axis] : Chart.MinUV[Axis])) < ThreshUVsAreSame;
							bool bBorderB = Math::Abs(UV0j[Axis] - (Sign ^ 1 ? Chart.MaxUV[Axis] : Chart.MinUV[Axis])) < ThreshUVsAreSame;

							// FIXME mirrored
							if (!bAxisAligned || !bBorderA || !bBorderB)
							{
								// Edges weren't on matching rectangle borders
								continue;
							}

							Vector2 CenterA = 0.5f * (ChartA.MinUV + ChartA.MaxUV);
							Vector2 CenterB = 0.5f * (ChartB.MinUV + ChartB.MaxUV);

							Vector2 ExtentA = 0.5f * (ChartA.MaxUV - ChartA.MinUV);
							Vector2 ExtentB = 0.5f * (ChartB.MaxUV - ChartB.MinUV);

							// FIXME mirrored
							CenterB += Translation;

							Vector2 CenterDiff = CenterA - CenterB;
							Vector2 ExtentDiff = ExtentA - ExtentB;
							Vector2 Separation = ExtentA + ExtentB + CenterDiff * (Sign ? 1.0f : -1.0f);

							bool bCenterMatch = Math::Abs(CenterDiff[Axis ^ 1]) < ThreshUVsAreSame;
							bool bExtentMatch = Math::Abs(ExtentDiff[Axis ^ 1]) < ThreshUVsAreSame;
							bool bSeparate = Math::Abs(Separation[Axis ^ 0]) < ThreshUVsAreSame;

							if (!bCenterMatch || !bExtentMatch || !bSeparate)
							{
								// Rectangles don't match up after translation
								continue;
							}

							// Found a valid edge join
							ChartA.Join[Side ^ 0] = ChartJ;
							ChartB.Join[Side ^ 1] = ChartI;
							break;
						}
					}
				}
			}
		}

		std::vector< uint32 > JoinedSortedTris;
		JoinedSortedTris.reserve(NumTris);

		// Detect loops
		for (uint32 Axis = 0; Axis < 2; Axis++)
		{
			uint32 Side = Axis << 1;

			for (size_t i = 0; i < Charts.size(); i++)
			{
				int32 j = Charts[i].Join[Side ^ 1];
				while (j != -1)
				{
					int32 Next = Charts[j].Join[Side ^ 1];
					if (Next == i)
					{
						// Break loop
						Charts[i].Join[Side ^ 0] = -1;
						Charts[j].Join[Side ^ 1] = -1;
						break;
					}
					j = Next;
				}
			}
		}

		// Join rows first, then columns
		for (uint32 Axis = 0; Axis < 2; Axis++)
		{
			for (size_t i = 0; i < Charts.size(); i++)
			{
				FMeshChart& ChartA = Charts[i];

				if (ChartA.FirstTri == ChartA.LastTri)
				{
					// Empty chart
					continue;
				}

				for (uint32 Side = 0; Side < 4; Side++)
				{
					if (ChartA.Join[Side] != -1)
					{
						FMeshChart& ChartB = Charts[ChartA.Join[Side]];

						//check(ChartB.Join[Side ^ 1] == i);
						//check(ChartB.FirstTri != ChartB.LastTri);
					}
				}
			}

			NumTris = 0;

			for (size_t i = 0; i < Charts.size(); i++)
			{
				FMeshChart& Chart = Charts[i];
				NumTris += Chart.LastTri - Chart.FirstTri;
			}
			//check(NumTris == SortedTris.size());

			NumTris = 0;
			for (size_t i = 0; i < Charts.size(); i++)
			{
				FMeshChart& ChartA = Charts[i];

				if (ChartA.FirstTri == ChartA.LastTri)
				{
					// Empty chart
					continue;
				}

				uint32 Side = Axis << 1;

				// Find start (left, bottom)
				if (ChartA.Join[Side ^ 0] == -1)
				{
					// Add original tris
					NumTris += ChartA.LastTri - ChartA.FirstTri;

					// Continue joining until no more to the (right, top)
					int32 Next = ChartA.Join[Side ^ 1];
					while (Next != -1)
					{
						FMeshChart& ChartB = Charts[Next];

						NumTris += ChartB.LastTri - ChartB.FirstTri;
						Next = ChartB.Join[Side ^ 1];
					}
				}
			}
			//check(NumTris == SortedTris.Num());

#if 1
			NumTris = 0;
			for (size_t i = 0; i < Charts.size(); i++)
			{
				FMeshChart& ChartA = Charts[i];

				if (ChartA.FirstTri == ChartA.LastTri)
				{
					// Empty chart
					continue;
				}

				// Join[] = { left, right, bottom, top }

				uint32 Side = Axis << 1;

				// Find start (left, bottom)
				if (ChartA.Join[Side ^ 0] == -1)
				{
					uint32 FirstTri = JoinedSortedTris.size();

					// Add original tris
					for (uint32 Tri = ChartA.FirstTri; Tri < ChartA.LastTri; Tri++)
					{
						JoinedSortedTris.push_back(SortedTris[Tri]);
					}
					NumTris += ChartA.LastTri - ChartA.FirstTri;

					// Continue joining until no more to the (right, top)
					while (ChartA.Join[Side ^ 1] != -1)
					{
						FMeshChart& ChartB = Charts[ChartA.Join[Side ^ 1]];

						//check(ChartB.FirstTri != ChartB.LastTri);

						Vector2 Translation = ChartA.MinUV - ChartB.MinUV;
						Translation[Axis] += ChartA.MaxUV[Axis] - ChartA.MinUV[Axis];

						for (uint32 Tri = ChartB.FirstTri; Tri < ChartB.LastTri; Tri++)
						{
							JoinedSortedTris.push_back(SortedTris[Tri]);
							for (int k = 0; k < 3; k++)
							{
								TexCoords[3 * SortedTris[Tri] + k] += Translation;
							}
						}
						NumTris += ChartB.LastTri - ChartB.FirstTri;

						ChartA.Join[Side ^ 1] = ChartB.Join[Side ^ 1];
						ChartA.MaxUV[Axis] += ChartB.MaxUV[Axis] - ChartB.MinUV[Axis];
						ChartA.WorldScale += ChartB.WorldScale;
						ChartA.UVArea += ChartB.UVArea;

						ChartB.FirstTri = 0;
						ChartB.LastTri = 0;
						ChartB.UVArea = 0.0f;

						DisconnectChart(ChartB, Side ^ 2);
						DisconnectChart(ChartB, Side ^ 3);
					}

					ChartA.FirstTri = FirstTri;
					ChartA.LastTri = JoinedSortedTris.size();
				}
				else
				{
					// Make sure a starting chart could connect to this
					FMeshChart& ChartB = Charts[ChartA.Join[Side ^ 0]];
					//check(ChartB.Join[Side ^ 1] == i);
					//check(ChartB.FirstTri != ChartB.LastTri);
				}
			}
			//check(NumTris == SortedTris.Num());

			//check(SortedTris.Num() == JoinedSortedTris.Num());
			Exchange(SortedTris, JoinedSortedTris);
			JoinedSortedTris.clear();
			//JoinedSortedTris.Reset();
#endif
		}

		// Clean out empty charts
		for (size_t i = 0; i < Charts.size(); i++)
		{
			while (i < Charts.size() && Charts[i].FirstTri == Charts[i].LastTri)
			{
				//Charts.RemoveAtSwap(i);
				Charts.erase(Charts.begin() + i);
			}
		}

		for (size_t i = 0; i < Charts.size(); i++)
		{
			FMeshChart& Chart = Charts[i];

			if (LayoutVersion >= MeshDescriptionOperations::ELightmapUVVersion::SmallChartPacking)
			{
				Chart.WorldScale /= Math::Max(Chart.UVArea, 1e-8f);
			}
			else
			{
				if (Chart.UVArea > 1e-4f)
				{
					Chart.WorldScale /= Chart.UVArea;
				}
				else
				{
					Chart.WorldScale = Vector2(0,0);
				}
			}

			TotalUVArea += Chart.UVArea * Chart.WorldScale.X * Chart.WorldScale.Y;
		}
#endif

		//double End = FPlatformTime::Seconds();

		//UE_LOG(LogMeshDescriptionLayoutUV, Display, TEXT("FindCharts: %s"), *FPlatformTime::PrettyTime(End - Begin));
	}

	bool FLayoutUV::FindBestPacking()
	{
		if ((uint32)Charts.size() > TextureResolution * TextureResolution || TotalUVArea == 0.f)
		{
			// More charts than texels
			return false;
		}

		const float LinearSearchStart = 0.5f;
		const float LinearSearchStep = 0.5f;
		const int32 BinarySearchSteps = 6;

		float UVScaleFail = TextureResolution * Math::Sqrt(1.0f / TotalUVArea);
		float UVScalePass = TextureResolution * Math::Sqrt(LinearSearchStart / TotalUVArea);

		// Linear search for first fit
		while (1)
		{
			ScaleCharts(UVScalePass);

			bool bFit = PackCharts();
			if (bFit)
			{
				break;
			}

			UVScaleFail = UVScalePass;
			UVScalePass *= LinearSearchStep;
		}

		// Binary search for best fit
		for (int32 i = 0; i < BinarySearchSteps; i++)
		{
			float UVScale = 0.5f * (UVScaleFail + UVScalePass);
			ScaleCharts(UVScale);

			bool bFit = PackCharts();
			if (bFit)
			{
				UVScalePass = UVScale;
			}
			else
			{
				UVScaleFail = UVScale;
			}
		}

		// TODO store packing scale/bias separate so this isn't necessary
		ScaleCharts(UVScalePass);
		PackCharts();

		return true;
	}

	void FLayoutUV::ScaleCharts(float UVScale)
	{
		for (size_t i = 0; i < Charts.size(); i++)
		{
			FMeshChart& Chart = Charts[i];
			Chart.UVScale = Chart.WorldScale * UVScale;
		}

		// Scale charts such that they all fit and roughly total the same area as before
#if 1
		float UniformScale = 1.0f;
		for (int i = 0; i < 1000; i++)
		{
			uint32 NumMaxedOut = 0;
			float ScaledUVArea = 0.0f;
			for (size_t ChartIndex = 0; ChartIndex < Charts.size(); ChartIndex++)
			{
				FMeshChart& Chart = Charts[ChartIndex];

				Vector2 ChartSize = Chart.MaxUV - Chart.MinUV;
				Vector2 ChartSizeScaled = ChartSize * Chart.UVScale * UniformScale;

				const float MaxChartEdge = TextureResolution - 1.0f;
				const float LongestChartEdge = Math::Max(ChartSizeScaled.X, ChartSizeScaled.Y);

				const float Epsilon = 0.01f;
				if (LongestChartEdge + Epsilon > MaxChartEdge)
				{
					// Rescale oversized charts to fit
					Chart.UVScale.X = MaxChartEdge / Math::Max(ChartSize.X, ChartSize.Y);
					Chart.UVScale.Y = MaxChartEdge / Math::Max(ChartSize.X, ChartSize.Y);
					NumMaxedOut++;
				}
				else
				{
					Chart.UVScale.X *= UniformScale;
					Chart.UVScale.Y *= UniformScale;
				}

				ScaledUVArea += Chart.UVArea * Chart.UVScale.X * Chart.UVScale.Y;
			}

			if (NumMaxedOut == 0)
			{
				// No charts maxed out so no need to rebalance
				break;
			}

			if (NumMaxedOut == Charts.size())
			{
				// All charts are maxed out
				break;
			}

			// Scale up smaller charts to maintain expected total area
			// Want ScaledUVArea == TotalUVArea * UVScale^2
			float RebalanceScale = UVScale * Math::Sqrt(TotalUVArea / ScaledUVArea);
			if (RebalanceScale < 1.01f)
			{
				// Stop if further rebalancing is minor
				break;
			}
			UniformScale = RebalanceScale;
		}
#endif

#if 1
		float NonuniformScale = 1.0f;
		for (int i = 0; i < 1000; i++)
		{
			uint32 NumMaxedOut = 0;
			float ScaledUVArea = 0.0f;
			for (size_t ChartIndex = 0; ChartIndex < Charts.size(); ChartIndex++)
			{
				FMeshChart& Chart = Charts[ChartIndex];

				for (int k = 0; k < 2; k++)
				{
					const float MaximumChartSize = TextureResolution - 1.0f;
					const float ChartSize = Chart.MaxUV[k] - Chart.MinUV[k];
					const float ChartSizeScaled = ChartSize * Chart.UVScale[k] * NonuniformScale;

					const float Epsilon = 0.01f;
					if (ChartSizeScaled + Epsilon > MaximumChartSize)
					{
						// Scale oversized charts to max size
						Chart.UVScale[k] = MaximumChartSize / ChartSize;
						NumMaxedOut++;
					}
					else
					{
						Chart.UVScale[k] *= NonuniformScale;
					}
				}

				ScaledUVArea += Chart.UVArea * Chart.UVScale.X * Chart.UVScale.Y;
			}

			if (NumMaxedOut == 0)
			{
				// No charts maxed out so no need to rebalance
				break;
			}

			if (NumMaxedOut == Charts.size() * 2)
			{
				// All charts are maxed out in both dimensions
				break;
			}

			// Scale up smaller charts to maintain expected total area
			// Want ScaledUVArea == TotalUVArea * UVScale^2
			float RebalanceScale = UVScale * Math::Sqrt(TotalUVArea / ScaledUVArea);
			if (RebalanceScale < 1.01f)
			{
				// Stop if further rebalancing is minor
				break;
			}
			NonuniformScale = RebalanceScale;
		}
#endif

		// Sort charts from largest to smallest
		struct FCompareCharts
		{
			inline bool operator()(const FMeshChart& A, const FMeshChart& B) const
			{
				// Rect area
				Vector2 ChartRectA = (A.MaxUV - A.MinUV) * A.UVScale;
				Vector2 ChartRectB = (B.MaxUV - B.MinUV) * B.UVScale;
				return ChartRectA.X * ChartRectA.Y > ChartRectB.X * ChartRectB.Y;
			}
		};
		std::sort(Charts.begin(), Charts.end(), FCompareCharts());
		//Algo::IntroSort(Charts, FCompareCharts());
	}

	bool FLayoutUV::PackCharts()
	{
		uint32 RasterizeCycles = 0;
		uint32 FindCycles = 0;

		//double BeginPackCharts = FPlatformTime::Seconds();

		LayoutRaster.Clear();

		for (size_t i = 0; i < Charts.size(); i++)
		{
			FMeshChart& Chart = Charts[i];

			// Try different orientations and pick best
			int32				BestOrientation = -1;
			FAllocator2D::FRect	BestRect = { ~0u, ~0u, ~0u, ~0u };

			for (int32 Orientation = 0; Orientation < 8; Orientation++)
			{
				// TODO If any dimension is less than 1 pixel shrink dimension to zero

				OrientChart(Chart, Orientation);

				Vector2 ChartSize = Chart.MaxUV - Chart.MinUV;
				ChartSize = ChartSize.X * Chart.PackingScaleU + ChartSize.Y * Chart.PackingScaleV;

				// Only need half pixel dilate for rects
				FAllocator2D::FRect	Rect;
				Rect.X = 0;
				Rect.Y = 0;
				Rect.W = Math::CeilToInt(Math::Abs(ChartSize.X) + 1.0f);
				Rect.H = Math::CeilToInt(Math::Abs(ChartSize.Y) + 1.0f);

				// Just in case lack of precision pushes it over
				Rect.W = Math::Min(TextureResolution, Rect.W);
				Rect.H = Math::Min(TextureResolution, Rect.H);

				const bool bRectPack = false;

				if (bRectPack)
				{
					if (LayoutRaster.Find(Rect))
					{
						// Is best?
						if (Rect.X + Rect.Y * TextureResolution < BestRect.X + BestRect.Y * TextureResolution)
						{
							BestOrientation = Orientation;
							BestRect = Rect;
						}
					}
					else
					{
						continue;
					}
				}
				else
				{
					if (LayoutVersion >= MeshDescriptionOperations::ELightmapUVVersion::Segments && Orientation % 4 == 1)
					{
						ChartRaster.FlipX(Rect);
					}
					else if (LayoutVersion >= MeshDescriptionOperations::ELightmapUVVersion::Segments && Orientation % 4 == 3)
					{
						ChartRaster.FlipY(Rect);
					}
					else
					{
						//int32 BeginRasterize = FPlatformTime::Cycles();
						RasterizeChart(Chart, Rect.W, Rect.H);
						//RasterizeCycles += FPlatformTime::Cycles() - BeginRasterize;
					}

					bool bFound = false;

					//uint32 BeginFind = FPlatformTime::Cycles();
					if (LayoutVersion == MeshDescriptionOperations::ELightmapUVVersion::BitByBit)
					{
						bFound = LayoutRaster.FindBitByBit(Rect, ChartRaster);
					}
					else if (LayoutVersion >= MeshDescriptionOperations::ELightmapUVVersion::Segments)
					{
						bFound = LayoutRaster.FindWithSegments(Rect, BestRect, ChartRaster);
					}
					//FindCycles += FPlatformTime::Cycles() - BeginFind;

					if (bFound)
					{
						// Is best?
						if (Rect.X + Rect.Y * TextureResolution < BestRect.X + BestRect.Y * TextureResolution)
						{
							BestChartRaster = ChartRaster;

							BestOrientation = Orientation;
							BestRect = Rect;

							if (BestRect.X == 0 && BestRect.Y == 0)
							{
								// BestRect can't be beat, stop here
								break;
							}
						}
					}
					else
					{
						continue;
					}
				}
			}

			if (BestOrientation >= 0)
			{
				// Add chart to layout
				OrientChart(Chart, BestOrientation);

				LayoutRaster.Alloc(BestRect, BestChartRaster);

				Chart.PackingBias.X += BestRect.X;
				Chart.PackingBias.Y += BestRect.Y;
			}
			else
			{
				// Found no orientation that fit
				return false;
			}
		}

		//double EndPackCharts = FPlatformTime::Seconds();

		//UE_LOG(LogMeshDescriptionLayoutUV, Display, TEXT("PackCharts: %s"), *FPlatformTime::PrettyTime(EndPackCharts - BeginPackCharts));
		//UE_LOG(LogMeshDescriptionLayoutUV, Display, TEXT("  Rasterize: %u"), RasterizeCycles);
		//UE_LOG(LogMeshDescriptionLayoutUV, Display, TEXT("  Find: %u"), FindCycles);

		return true;
	}

	void FLayoutUV::OrientChart(FMeshChart& Chart, int32 Orientation)
	{
		switch (Orientation)
		{
		case 0:
			// 0 degrees
			Chart.PackingScaleU = Vector2(Chart.UVScale.X, 0);
			Chart.PackingScaleV = Vector2(0, Chart.UVScale.Y);
			Chart.PackingBias = -Chart.MinUV.X * Chart.PackingScaleU - Chart.MinUV.Y * Chart.PackingScaleV + 0.5f;
			break;
		case 1:
			// 0 degrees, flip x
			Chart.PackingScaleU = Vector2(-Chart.UVScale.X, 0);
			Chart.PackingScaleV = Vector2(0, Chart.UVScale.Y);
			Chart.PackingBias = -Chart.MaxUV.X * Chart.PackingScaleU - Chart.MinUV.Y * Chart.PackingScaleV + 0.5f;
			break;
		case 2:
			// 90 degrees
			Chart.PackingScaleU = Vector2(0, -Chart.UVScale.X);
			Chart.PackingScaleV = Vector2(Chart.UVScale.Y, 0);
			Chart.PackingBias = -Chart.MaxUV.X * Chart.PackingScaleU - Chart.MinUV.Y * Chart.PackingScaleV + 0.5f;
			break;
		case 3:
			// 90 degrees, flip x
			Chart.PackingScaleU = Vector2(0, Chart.UVScale.X);
			Chart.PackingScaleV = Vector2(Chart.UVScale.Y, 0);
			Chart.PackingBias = -Chart.MinUV.X * Chart.PackingScaleU - Chart.MinUV.Y * Chart.PackingScaleV + 0.5f;
			break;
		case 4:
			// 180 degrees
			Chart.PackingScaleU = Vector2(-Chart.UVScale.X, 0);
			Chart.PackingScaleV = Vector2(0, -Chart.UVScale.Y);
			Chart.PackingBias = -Chart.MaxUV.X * Chart.PackingScaleU - Chart.MaxUV.Y * Chart.PackingScaleV + 0.5f;
			break;
		case 5:
			// 180 degrees, flip x
			Chart.PackingScaleU = Vector2(Chart.UVScale.X, 0);
			Chart.PackingScaleV = Vector2(0, -Chart.UVScale.Y);
			Chart.PackingBias = -Chart.MinUV.X * Chart.PackingScaleU - Chart.MaxUV.Y * Chart.PackingScaleV + 0.5f;
			break;
		case 6:
			// 270 degrees
			Chart.PackingScaleU = Vector2(0, Chart.UVScale.X);
			Chart.PackingScaleV = Vector2(-Chart.UVScale.Y, 0);
			Chart.PackingBias = -Chart.MinUV.X * Chart.PackingScaleU - Chart.MaxUV.Y * Chart.PackingScaleV + 0.5f;
			break;
		case 7:
			// 270 degrees, flip x
			Chart.PackingScaleU = Vector2(0, -Chart.UVScale.X);
			Chart.PackingScaleV = Vector2(-Chart.UVScale.Y, 0);
			Chart.PackingBias = -Chart.MaxUV.X * Chart.PackingScaleU - Chart.MaxUV.Y * Chart.PackingScaleV + 0.5f;
			break;
		}
	}

	// Max of 2048x2048 due to precision
	// Dilate in 28.4 fixed point. Half pixel dilation is conservative rasterization.
	// Dilation same as Minkowski sum of triangle and square.
	template< typename TShader, int32 Dilate >
	void RasterizeTriangle(TShader& Shader, const Vector2 Points[3], int32 ScissorWidth, int32 ScissorHeight)
	{
		const Vector2 HalfPixel(0.5f, 0.5f);
		Vector2 p0 = Points[0] - HalfPixel;
		Vector2 p1 = Points[1] - HalfPixel;
		Vector2 p2 = Points[2] - HalfPixel;

		// Correct winding
		float Facing = (p0.X - p1.X) * (p2.Y - p0.Y) - (p0.Y - p1.Y) * (p2.X - p0.X);
		if (Facing < 0.0f)
		{
			Math::Swap(p0, p2);
		}

		// 28.4 fixed point
		const int32 X0 = (int32)(16.0f * p0.X + 0.5f);
		const int32 X1 = (int32)(16.0f * p1.X + 0.5f);
		const int32 X2 = (int32)(16.0f * p2.X + 0.5f);

		const int32 Y0 = (int32)(16.0f * p0.Y + 0.5f);
		const int32 Y1 = (int32)(16.0f * p1.Y + 0.5f);
		const int32 Y2 = (int32)(16.0f * p2.Y + 0.5f);

		// Bounding rect
		int32 MinX = (Math::Min3(X0, X1, X2) - Dilate + 15) / 16;
		int32 MaxX = (Math::Max3(X0, X1, X2) + Dilate + 15) / 16;
		int32 MinY = (Math::Min3(Y0, Y1, Y2) - Dilate + 15) / 16;
		int32 MaxY = (Math::Max3(Y0, Y1, Y2) + Dilate + 15) / 16;

		// Clip to image
		MinX = Math::Clamp(MinX, 0, ScissorWidth);
		MaxX = Math::Clamp(MaxX, 0, ScissorWidth);
		MinY = Math::Clamp(MinY, 0, ScissorHeight);
		MaxY = Math::Clamp(MaxY, 0, ScissorHeight);

		// Deltas
		const int32 DX01 = X0 - X1;
		const int32 DX12 = X1 - X2;
		const int32 DX20 = X2 - X0;

		const int32 DY01 = Y0 - Y1;
		const int32 DY12 = Y1 - Y2;
		const int32 DY20 = Y2 - Y0;

		// Half-edge constants
		int32 C0 = DY01 * X0 - DX01 * Y0;
		int32 C1 = DY12 * X1 - DX12 * Y1;
		int32 C2 = DY20 * X2 - DX20 * Y2;

		// Correct for fill convention
		C0 += (DY01 < 0 || (DY01 == 0 && DX01 > 0)) ? 0 : -1;
		C1 += (DY12 < 0 || (DY12 == 0 && DX12 > 0)) ? 0 : -1;
		C2 += (DY20 < 0 || (DY20 == 0 && DX20 > 0)) ? 0 : -1;

		// Dilate edges
		C0 += (abs(DX01) + abs(DY01)) * Dilate;
		C1 += (abs(DX12) + abs(DY12)) * Dilate;
		C2 += (abs(DX20) + abs(DY20)) * Dilate;

		for (int32 y = MinY; y < MaxY; y++)
		{
			for (int32 x = MinX; x < MaxX; x++)
			{
				// same as Edge1 >= 0 && Edge2 >= 0 && Edge3 >= 0
				int32 IsInside;
				IsInside = C0 + (DX01 * y - DY01 * x) * 16;
				IsInside |= C1 + (DX12 * y - DY12 * x) * 16;
				IsInside |= C2 + (DX20 * y - DY20 * x) * 16;

				if (IsInside >= 0)
				{
					Shader.Process(x, y);
				}
			}
		}
	}

	void FLayoutUV::RasterizeChart(const FMeshChart& Chart, uint32 RectW, uint32 RectH)
	{
		// Bilinear footprint is -1 to 1 pixels. If packed geometrically, only a half pixel dilation
		// would be needed to guarantee all charts were at least 1 pixel away, safe for bilinear filtering.
		// Unfortunately, with pixel packing a full 1 pixel dilation is required unless chart edges exactly
		// align with pixel centers.

		ChartRaster.Clear();

		for (uint32 Tri = Chart.FirstTri; Tri < Chart.LastTri; Tri++)
		{
			Vector2 Points[3];
			for (int k = 0; k < 3; k++)
			{
				const Vector2& UV = TexCoords[3 * SortedTris[Tri] + k];
				Points[k] = UV.X * Chart.PackingScaleU + UV.Y * Chart.PackingScaleV + Chart.PackingBias;
			}

			RasterizeTriangle< FAllocator2DShader, 16 >(ChartShader, Points, RectW, RectH);
		}

		if (LayoutVersion >= MeshDescriptionOperations::ELightmapUVVersion::Segments)
		{
			ChartRaster.CreateUsedSegments();
		}
	}

	void FLayoutUV::CommitPackedUVs()
	{
		// If current DstChannel is out of range of the number of UVs defined by the mesh description, change the index count accordingly
		const uint32 NumUVs = MD.VertexInstanceAttributes().GetAttributeIndexCount<Vector2>(MeshAttribute::VertexInstance::TextureCoordinate);
		if (DstChannel >= NumUVs)
		{
			MD.VertexInstanceAttributes().SetAttributeIndexCount<Vector2>(MeshAttribute::VertexInstance::TextureCoordinate, DstChannel + 1);
			//ensure(false);	// not expecting it to get here
		}

		std::vector<Vector2>& VertexUVs = MD.VertexInstanceAttributes().GetAttributes<Vector2>(MeshAttribute::VertexInstance::TextureCoordinate, DstChannel);

		// Commit chart UVs
		for (size_t i = 0; i < Charts.size(); i++)
		{
			FMeshChart& Chart = Charts[i];

			Chart.PackingScaleU /= TextureResolution;
			Chart.PackingScaleV /= TextureResolution;
			Chart.PackingBias /= TextureResolution;

			for (uint32 Tri = Chart.FirstTri; Tri < Chart.LastTri; Tri++)
			{
				for (int k = 0; k < 3; k++)
				{
					uint32 Index = 3 * SortedTris[Tri] + k;
					const Vector2& UV = TexCoords[Index];
					const int VertexInstanceID(RemapVerts[Index]);
					VertexUVs[VertexInstanceID] = UV.X * Chart.PackingScaleU + UV.Y * Chart.PackingScaleV + Chart.PackingBias;
				}
			}
		}
	}
}
