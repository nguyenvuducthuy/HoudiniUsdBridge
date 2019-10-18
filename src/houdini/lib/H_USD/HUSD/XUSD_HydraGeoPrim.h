/*
 * PROPRIETARY INFORMATION.  This software is proprietary to
 * Side Effects Software Inc., and is not to be reproduced,
 * transmitted, or disclosed in any way without written permission.
 *
 * Produced by:
 *	Side Effects Software Inc
 *	123 Front Street West, Suite 1401
 *	Toronto, Ontario
 *	Canada   M5J 2M2
 *	416-504-9876
 *
 * NAME:	XUSD_HydraGeoPrim.h (HUSD Library, C++)
 *
 * COMMENTS:	Container for a hydra geometry prim (HdRprim)
 */
#ifndef XUSD_HydraGeoPrim_h
#define XUSD_HydraGeoPrim_h

#include <pxr/pxr.h>
#include <pxr/base/gf/matrix4d.h>
#include <pxr/imaging/hd/basisCurves.h>
#include <pxr/imaging/hd/mesh.h>
#include <pxr/imaging/hd/points.h>
#include <pxr/imaging/hd/volume.h>
#include <GT/GT_AttributeList.h>
#include <GT/GT_DataArray.h>
#include <GT/GT_TransformArray.h>
#include <GT/GT_Transform.h>
#include <GT/GT_Types.h>
#include <GEO/GEO_PackedTypes.h>
#include <UT/UT_StringMap.h>
#include <UT/UT_Pair.h>
#include <UT/UT_Tuple.h>
#include <UT/UT_Options.h>
#include <SYS/SYS_Types.h>
#include "HUSD_HydraGeoPrim.h"

class GT_DAIndexedString;

PXR_NAMESPACE_OPEN_SCOPE

class XUSD_HydraGeoBase;

/// Container for a hydra geometry prim (HdRprim)
class XUSD_HydraGeoPrim : public HUSD_HydraGeoPrim
{
public:
	     XUSD_HydraGeoPrim(TfToken const& type_id,
			       SdfPath const& prim_id,
			       SdfPath const& instancer_id,
			       HUSD_Scene &scene);
    virtual ~XUSD_HydraGeoPrim();

    virtual bool       isValid() const { return myHydraPrim != nullptr; }
    virtual void       updateGTSelection();
    virtual void       clearGTSelection();

    HdRprim	      *rprim() const { return myHydraPrim; }

    UT_StringHolder     getTopLevelPath(HdSceneDelegate *delegate,
                                        SdfPath const& prim_id,
                                        SdfPath const& instancer_id);
private:
    HdRprim	       *myHydraPrim;
    XUSD_HydraGeoBase  *myPrimBase;
};


/// Base tie-in class for common data
class XUSD_HydraGeoBase
{
public:
    XUSD_HydraGeoBase(GT_PrimitiveHandle &prim,
		      GT_PrimitiveHandle &instance,
		      int &dirty,
		      XUSD_HydraGeoPrim &hprim);

    void	updateGTSelection();
    void	clearGTSelection();

protected:
    void	resetPrim();
    void	clearDirty(HdDirtyBits *dirty_bits) const;
    bool        isDeferred(HdRenderParam *,
			   HdDirtyBits &bits) const;
    
    GEO_ViewportLOD checkVisibility(HdSceneDelegate *sceneDelegate,
				    const SdfPath   &id,
				    HdDirtyBits     *dirty_bits);
    bool	addBBoxAttrib(HdSceneDelegate* scene_delegate,
			      const SdfPath	     &id,
			      GT_AttributeListHandle &detail,
			      const GT_Primitive     *gt_prim) const;
    
    // buildTransforms() should only be called in Sync() methods
    void	buildTransforms(HdSceneDelegate *scene_delegate,
				const SdfPath  &proto_id,
				const SdfPath  &instr_id,
				HdDirtyBits    *dirty_bits);
    
    bool	updateAttrib(const TfToken	      &usd_attrib,
			     const UT_StringRef       &gt_attrib,
			     HdSceneDelegate	      *scene_delegate,
			     const SdfPath	      &id,
			     HdDirtyBits	      *dirty_bits,
			     GT_Primitive	      *gt_prim,
			     GT_AttributeListHandle   (&attrib_list)[4],
			     int		      *point_freq_size=nullptr,
			     bool		       set_point_freq = false,
			     bool		      *exists = nullptr);
    
    void	createInstance(HdSceneDelegate          *scene_delegate,
			       const SdfPath		&proto_id,
			       const SdfPath		&inst_id,
			       HdDirtyBits		*dirty_bits,
			       GT_Primitive		*geo,
			       GEO_ViewportLOD		 lod,
			       int			 mat_id,
			       bool			 instance_change);
    void        buildShaderInstanceOverrides(
                                HdSceneDelegate         *sd,
                                const SdfPath           &inst_id,
                                const SdfPath           &proto_id,
                                HdDirtyBits             *dirty_bits);
    bool        processInstancerOverrides(
                                HdSceneDelegate         *sd,
                                const SdfPath           &inst_id,
                                const SdfPath           &proto_id,
                                HdDirtyBits             *dirty_bits,
                                int                      inst_level,
                                int                     &num_instances);
    void        processNestedOverrides(int level,
                                       GT_DAIndexedString *overrides,
                                       const UT_Options *input_opt,
                                       int &index) const;
    void        assignOverride(const UT_Options *options,
                               GT_DAIndexedString *overrides,
                               int index) const;

    void	removeFromDisplay();

    XUSD_HydraGeoPrim		&myHydraPrim;
    UT_Matrix4D 		 myPrimTransform;
    GT_TransformHandle           myGTPrimTransform;
    UT_StringMap<UT_Tuple<GT_Owner,int, bool, void *> >  myAttribMap;
    UT_StringMap<UT_StringHolder> myExtraAttribs;
    GT_PrimitiveHandle		&myGTPrim;
    GT_PrimitiveHandle		&myInstance;
    int				&myDirtyMask;
    int64			 myInstanceId;
    GT_TransformArrayHandle	 myInstanceTransforms;
    GT_DataArrayHandle		 mySelection;
    GT_DataArrayHandle		 myMatIDArray;
    GT_DataArrayHandle		 myMaterialsArray;
    int				 myMaterialID;
    GT_DataArrayHandle		 myPickIDArray;
    GT_DataArrayHandle           myInstanceMatID;
    UT_IntArray                  myInstanceLevels;
    UT_StringArray               myLightLink;

    class InstStackEntry
    {
    public:
         InstStackEntry() : nInst(0), options(nullptr) {}
        ~InstStackEntry() { clear(); }
        
        void clear()
            {
                delete options;
                options = nullptr;
                attribs = nullptr;
            }
        int nInst;
        UT_Array<UT_Options> *options;
        GT_AttributeListHandle attribs;
    };
    
    UT_Array<InstStackEntry >    myInstanceAttribStack;
    GT_DataArrayHandle           myInstanceOverridesAttrib;
    GT_AttributeListHandle       myInstanceAttribList;
};
    

/// Container for a hydra mesh primitive
class XUSD_HydraGeoMesh : public HdMesh, public XUSD_HydraGeoBase
{
public:
	     XUSD_HydraGeoMesh(TfToken const& type_id,
			       SdfPath const& prim_id,
			       SdfPath const& instancer_id,
			       GT_PrimitiveHandle &prim,
			       GT_PrimitiveHandle &instance,
			       int &dirty,
			       XUSD_HydraGeoPrim &hprim);
    virtual ~XUSD_HydraGeoMesh();

    virtual void Sync(HdSceneDelegate *delegate,
                      HdRenderParam *rparm,
                      HdDirtyBits *dirty_bits,
                      TfToken const &representation) override;
    
    virtual void Finalize(HdRenderParam *rparm) override;
    virtual HdDirtyBits GetInitialDirtyBitsMask() const override;

protected:
    virtual HdDirtyBits _PropagateDirtyBits(HdDirtyBits bits) const override;
    virtual void	_InitRepr(TfToken const &representation,
				  HdDirtyBits *dirty_bits) override;
   
    GT_DataArrayHandle		 myCounts, myVertex;
    int64			 myTopHash;
    bool			 myIsSubD;
    bool			 myIsLeftHanded;
    int				 myRefineLevel;
};

/// Container for a hydra curves primitive
class XUSD_HydraGeoCurves : public HdBasisCurves, public XUSD_HydraGeoBase
{
public:
	     XUSD_HydraGeoCurves(TfToken const& type_id,
				 SdfPath const& prim_id,
				 SdfPath const& instancer_id,
				 GT_PrimitiveHandle &prim,
				 GT_PrimitiveHandle &instance,
				 int &dirty,
				 XUSD_HydraGeoPrim &hprim);
    virtual ~XUSD_HydraGeoCurves();

    virtual void Sync(HdSceneDelegate *delegate,
                      HdRenderParam *rparm,
                      HdDirtyBits *dirty_bits,
                      TfToken const &representation) override;
    
    virtual void Finalize(HdRenderParam *rparm) override;
    virtual HdDirtyBits GetInitialDirtyBitsMask() const override;

protected:
    virtual HdDirtyBits _PropagateDirtyBits(HdDirtyBits bits) const override;
    virtual void	_InitRepr(TfToken const &representation,
				  HdDirtyBits *dirty_bits) override;

    GT_PrimitiveHandle   myBasisCurve;
    GT_DataArrayHandle   myCounts;
    GT_DataArrayHandle	 myIndices;
    GT_Basis		 myBasis;
    bool		 myWrap;
    
};


/// Container for a hydra volume primitive.
class XUSD_HydraGeoVolume : public HdVolume, public XUSD_HydraGeoBase
{
public:
	     XUSD_HydraGeoVolume(TfToken const& typeId,
			       SdfPath const& primId,
			       SdfPath const& instancerId,
			       GT_PrimitiveHandle &prim,
			       GT_PrimitiveHandle &instance,
			       int &dirty,
			       XUSD_HydraGeoPrim &hprim);
    virtual ~XUSD_HydraGeoVolume();

    virtual void Sync(HdSceneDelegate *delegate,
                      HdRenderParam *rparm,
                      HdDirtyBits *dirty_bits,
                      TfToken const &representation) override;
    
    virtual void Finalize(HdRenderParam *renderParam) override;
    virtual HdDirtyBits GetInitialDirtyBitsMask() const override;

protected:
    virtual HdDirtyBits _PropagateDirtyBits(HdDirtyBits bits) const override;
    virtual void	_InitRepr(TfToken const &representation,
				  HdDirtyBits *dirty_bits) override;
};


/// Container for a hydra curves primitive
class XUSD_HydraGeoPoints : public HdPoints, public XUSD_HydraGeoBase
{
public:
	     XUSD_HydraGeoPoints(TfToken const& type_id,
				 SdfPath const& prim_id,
				 SdfPath const& instancer_id,
				 GT_PrimitiveHandle &prim,
				 GT_PrimitiveHandle &instance,
				 int &dirty,
				 XUSD_HydraGeoPrim &hprim);
    virtual ~XUSD_HydraGeoPoints();

    virtual void Sync(HdSceneDelegate *delegate,
                      HdRenderParam *rparm,
                      HdDirtyBits *dirty_bits,
                      TfToken const &representation) override;
    
    virtual void Finalize(HdRenderParam *rparm) override;
    virtual HdDirtyBits GetInitialDirtyBitsMask() const override;

protected:
    virtual HdDirtyBits _PropagateDirtyBits(HdDirtyBits bits) const override;
    virtual void	_InitRepr(TfToken const &representation,
				  HdDirtyBits *dirty_bits) override;
   
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
