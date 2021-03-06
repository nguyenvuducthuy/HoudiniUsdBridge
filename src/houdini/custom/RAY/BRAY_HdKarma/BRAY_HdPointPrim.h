/*
 * Copyright 2019 Side Effects Software Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Produced by:
 *      Side Effects Software Inc.
 *      123 Front Street West, Suite 1401
 *      Toronto, Ontario
 *      Canada   M5J 2M2
 *      416-504-9876
 *
 */

#ifndef __BRAY_HDPointPrim__
#define __BRAY_HDPointPrim__

#include <pxr/pxr.h>
#include <pxr/imaging/hd/points.h>
#include <pxr/imaging/hd/enums.h>
#include <pxr/base/gf/matrix4f.h>

#include <BRAY/BRAY_Interface.h>
#include <UT/UT_UniquePtr.h>

class BRAY_Scene;

PXR_NAMESPACE_OPEN_SCOPE

class BRAY_HdParam;

class BRAY_HdPointPrim : public HdPoints
{
public:

    using ObjectPtrList = UT_Array<BRAY::ObjectPtr>;
    using SpaceList = UT_Array<UT_Array<BRAY::SpacePtr>>;

    struct AttribHandleIdx
    {
	GT_AttributeListHandle	myAttrib;
	int			myAttribIndex;
	bool			myConstAttrib;
    };

    BRAY_HdPointPrim(SdfPath const &id,
	    SdfPath const &instancerId = SdfPath());
    virtual ~BRAY_HdPointPrim() = default;

    /// Release any resources this class is holding onto - in this case,
    /// destroy the geometry object in the scene graph.
    virtual void	Finalize(HdRenderParam *renderParam) override final;

    /// Pull invalidated scene data and prepare/update the renderable
    /// representation.
    virtual void	Sync(HdSceneDelegate *sceneDelegate,
				HdRenderParam *renerParam,
				HdDirtyBits *dirtyBits,
				TfToken const &repr) override final;

    /// Inform the scene graph which state needs to be downloaded in the first
    /// Sync() call.  In this case, topology and point data.
    virtual HdDirtyBits	GetInitialDirtyBitsMask() const override final;

protected:
    /// This callback gives the prim an opportunity to set additional dirty
    /// bits based on those already set.
    virtual HdDirtyBits	_PropagateDirtyBits(HdDirtyBits bits)
				const override final;

    /// Initialize the given representation of the prim
    virtual void	_InitRepr(TfToken const &repr,
				HdDirtyBits *dirtyBits) override final;

private:

    void	updatePrims(BRAY_HdParam* rparm, HdSceneDelegate* sd,
			    HdDirtyBits* dirtyBits);

    /// Update method in spirit of the other HdRPrim update* methods
    bool	updateProceduralPrims(const GT_AttributeListHandle& pointAttribs,
				const GT_AttributeListHandle& detailAttribs,
				UT_UniquePtr<BRAY_Procedural> &proc,
				exint offset);

    /// Get the procedural 'type' primvar and create the procedural
    void	getUniqueProcedurals(const GT_AttributeListHandle& pointAttribs,
				const GT_AttributeListHandle& detailAttribs,
				UT_Array<UT_Array<exint>>& indices);

    /// Pass on paramters to the underlying procedural primitive through
    /// the setParameter()* functions
    void	passParameterData(const UT_StringRef& key,
				UT_UniquePtr<BRAY_Procedural> &proc,
				exint offset,
				const GT_DataArrayHandle& handle) const;

    /// compose transforms of point instanced procedurals
    void	computeInstXfms(const GT_AttributeListHandle& pointAttribs,
				const GT_AttributeListHandle& detailAttribs,
				const BRAY::SpacePtr& local,
				const UT_Array<UT_Array<exint>>& idx,
				bool flush, SpaceList& xfms);

    /// Function that uses the following rules to compute the actual transform
    /// based on : http://www.sidefx.com/docs/houdini/copy/instanceattrs.html
    void	composeXfm(UT_Array<AttribHandleIdx>&, exint index, int seg,
			   int xfmTupleSize, UT_Matrix4D& xfm) const;

    ObjectPtrList	    myInstances;
    ObjectPtrList	    myPrims;
    SpaceList		    myOriginalSpace;
    GT_AttributeListHandle  myAlist[2]; // store for procedurals
    bool		    myIsProcedural;
    UT_Array<GfMatrix4d>    myXform;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
