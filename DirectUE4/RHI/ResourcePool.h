#pragma once

#include "UnrealMath.h"

template<typename ResourceType, class ResourcePoolPolicy, class ResourceCreationArguments>
class TResourcePool
{
public:
	TResourcePool()
	{}
	TResourcePool(ResourcePoolPolicy InPolicy)
		: Policy(InPolicy)
	{}
	virtual ~TResourcePool()
	{
		DrainPool(true);
	}
	uint32 PooledSizeForCreationArguments(ResourceCreationArguments Args)
	{
		const uint32 BucketIndex = Policy.GetPoolBucketIndex(Args);
		return Policy.GetPoolBucketSize(BucketIndex);
	}
	ResourceType CreatePooledResource(ResourceCreationArguments Args)
	{
		// Find the appropriate bucket based on size
		const uint32 BucketIndex = Policy.GetPoolBucketIndex(Args);
		std::vector<FPooledResource>& PoolBucket = ResourceBuckets[BucketIndex];
		if (PoolBucket.size() > 0)
		{
			// Reuse the last entry in this size bucket
			FPooledResource Resource = PoolBucket.back();
			PoolBucket.pop_back();
			return Resource.Resource;
		}
		else
		{
			// Nothing usable was found in the free pool, create a new resource
			return Policy.CreateResource(Args);
		}
	}
	void ReleasePooledResource(const ResourceType& Resource)
	{
		extern uint32 GFrameNumberRenderThread;

		FPooledResource NewEntry;
		NewEntry.Resource = Resource;
		NewEntry.FrameFreed = GFrameNumberRenderThread;
		NewEntry.CreationArguments = Policy.GetCreationArguments(Resource);

		// Add to this frame's array of free resources
		const int32 SafeFrameIndex = GFrameNumberRenderThread % ResourcePoolPolicy::NumSafeFrames;
		const uint32 BucketIndex = Policy.GetPoolBucketIndex(NewEntry.CreationArguments);

		SafeResourceBuckets[SafeFrameIndex][BucketIndex].push_back(NewEntry);
	}
	void DrainPool(bool bForceDrainAll)
	{
		extern uint32 GFrameNumberRenderThread;

		uint32 NumToCleanThisFrame = ResourcePoolPolicy::NumToDrainPerFrame;
		uint32 CullAfterFramesNum = ResourcePoolPolicy::CullAfterFramesNum;

		if (!bForceDrainAll)
		{
			// Index of the bucket that is now old enough to be reused
			const int32 SafeFrameIndex = (GFrameNumberRenderThread + 1) % ResourcePoolPolicy::NumSafeFrames;

			// Merge the bucket into the free pool array
			for (int32 BucketIndex = 0; BucketIndex < ResourcePoolPolicy::NumPoolBuckets; BucketIndex++)
			{
				ResourceBuckets[BucketIndex].insert(ResourceBuckets[BucketIndex].begin(), SafeResourceBuckets[SafeFrameIndex][BucketIndex].begin(), SafeResourceBuckets[SafeFrameIndex][BucketIndex].end());
				SafeResourceBuckets[SafeFrameIndex][BucketIndex].clear();
			}
		}
		else
		{
			for (int32 FrameIndex = 0; FrameIndex < ResourcePoolPolicy::NumSafeFrames; FrameIndex++)
			{
				// Merge the bucket into the free pool array
				for (int32 BucketIndex = 0; BucketIndex < ResourcePoolPolicy::NumPoolBuckets; BucketIndex++)
				{
					ResourceBuckets[BucketIndex].insert(ResourceBuckets[BucketIndex].begin(), SafeResourceBuckets[FrameIndex][BucketIndex].begin(), SafeResourceBuckets[FrameIndex][BucketIndex].end());
					SafeResourceBuckets[FrameIndex][BucketIndex].clear();
				}
			}
		}

		// Clean a limited number of old entries to reduce hitching when leaving a large level
		for (int32 BucketIndex = 0; BucketIndex < ResourcePoolPolicy::NumPoolBuckets; BucketIndex++)
		{
			for (uint32 EntryIndex = ResourceBuckets[BucketIndex].size() - 1; EntryIndex >= 0; EntryIndex--)
			{
				FPooledResource& PoolEntry = ResourceBuckets[BucketIndex][EntryIndex];

				// Clean entries that are unlikely to be reused
				if ((GFrameNumberRenderThread - PoolEntry.FrameFreed) > CullAfterFramesNum || bForceDrainAll)
				{
					Policy.FreeResource(ResourceBuckets[BucketIndex][EntryIndex].Resource);

					ResourceBuckets[BucketIndex].erase(ResourceBuckets[BucketIndex].begin() + EntryIndex);

					--NumToCleanThisFrame;
					if (!bForceDrainAll && NumToCleanThisFrame == 0)
					{
						break;
					}
				}
			}

			if (!bForceDrainAll && NumToCleanThisFrame == 0)
			{
				break;
			}
		}
	}
private:
	ResourcePoolPolicy Policy;
	struct FPooledResource
	{
		/** The actual resource */
		ResourceType Resource;
		/** The arguments used to create the resource */
		ResourceCreationArguments CreationArguments;
		/** The frame the resource was freed */
		uint32 FrameFreed;
	};

	// Pool of free Resources, indexed by bucket for constant size search time.
	std::vector<FPooledResource> ResourceBuckets[ResourcePoolPolicy::NumPoolBuckets];

	// Resources that have been freed more recently than NumSafeFrames ago.
	std::vector<FPooledResource> SafeResourceBuckets[ResourcePoolPolicy::NumSafeFrames][ResourcePoolPolicy::NumPoolBuckets];
};

template<typename ResourceType, class ResourcePoolPolicy, class ResourceCreationArguments>
class TRenderResourcePool : public TResourcePool<ResourceType, ResourcePoolPolicy, ResourceCreationArguments>
{
public:
	/** Constructor */
	TRenderResourcePool()
	{
	}

	/** Constructor with policy argument
	* @param InPolicy An initialised policy object
	*/
	TRenderResourcePool(ResourcePoolPolicy InPolicy) :
		TResourcePool<ResourceType, ResourcePoolPolicy, ResourceCreationArguments>(InPolicy)
	{
	}

	/** Destructor */
	virtual ~TRenderResourcePool()
	{
		TResourcePool<ResourceType, ResourcePoolPolicy, ResourceCreationArguments>::DrainPool(true);
	}

	/** Creates a pooled resource.
	* @param Args the argument object for construction.
	* @returns An initialised resource or the policy's NullResource if not initialised.
	*/
	ResourceType CreatePooledResource(ResourceCreationArguments Args)
	{
		return TResourcePool<ResourceType, ResourcePoolPolicy, ResourceCreationArguments>::CreatePooledResource(Args);
	}

	/** Release a resource back into the pool.
	* @param Resource The resource to return to the pool
	*/
	void ReleasePooledResource(ResourceType Resource)
	{

		TResourcePool<ResourceType, ResourcePoolPolicy, ResourceCreationArguments>::ReleasePooledResource(Resource);
	}

public: // From FTickableObjectRenderThread


};