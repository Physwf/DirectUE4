#pragma once

#include <list>

class FSceneViewState;
/**
* Class used to reference an FSceneViewStateInterface that allows destruction and recreation of all FSceneViewStateInterface's when needed.
* This is used to support reloading the renderer module on the fly.
*/
class FSceneViewStateReference
{
public:
	FSceneViewStateReference() :
		Reference(NULL)
	{}

	virtual ~FSceneViewStateReference();

	/** Allocates the Scene view state. */
	void Allocate();

	/** Destorys the Scene view state. */
	void Destroy();

	/** Destroys all view states, but does not remove them from the linked list. */
	static void DestroyAll();

	/** Recreates all view states in the global list. */
	static void AllocateAll();

	FSceneViewState* GetReference()
	{
		return Reference;
	}

private:
	FSceneViewState * Reference;
	std::list<FSceneViewStateReference*>::iterator GlobalListLink;

	static std::list<FSceneViewStateReference*>& GetSceneViewStateList();
};

