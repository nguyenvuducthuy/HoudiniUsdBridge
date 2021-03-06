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
 *	Side Effects Software Inc.
 *	123 Front Street West, Suite 1401
 *	Toronto, Ontario
 *      Canada   M5J 2M2
 *	416-504-9876
 *
 */

#include "HUSD_Prune.h"
#include "HUSD_FindPrims.h"
#include "HUSD_TimeCode.h"
#include "XUSD_Data.h"
#include "XUSD_PathSet.h"
#include "XUSD_Utils.h"
#include <gusd/UT_Gf.h>
#include <pxr/base/vt/types.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usdGeom/imageable.h>
#include <pxr/usd/usdGeom/pointInstancer.h>
#include <unordered_map>

PXR_NAMESPACE_USING_DIRECTIVE

HUSD_Prune::HUSD_Prune(HUSD_AutoWriteLock &lock)
    : myWriteLock(lock),
      myTimeSampling(HUSD_TimeSampling::NONE)
{
}

HUSD_Prune::~HUSD_Prune()
{
}

bool
HUSD_Prune::prune(const HUSD_FindPrims &findprims,
        const HUSD_FindPrims *excludeprims,
        const HUSD_FindPrims *limitpruneprims,
	const HUSD_TimeCode &timecode,
	HUSD_Prune::PruneMethod prune_method,
        bool prune,
	bool prune_unselected,
        bool prune_ancestors_automatically,
        UT_StringArray *pruned_prims) const
{
    auto	 outdata = myWriteLock.data();

    if (outdata && outdata->isStageValid())
    {
	auto			 stage = outdata->stage();
	XUSD_PathSet	         paths = prune_unselected
				    ? findprims.getExcludedPathSet(true)
				    : findprims.getExpandedPathSet();

        // Exclude the prims from the exclusion rules.
        if (excludeprims)
        {
            const XUSD_PathSet  &excludepaths =
                excludeprims->getExpandedPathSet();
            XUSD_PathSet         combined;

            if (prune_unselected)
            {
                // Pruning unselected. Add the "excludes" to the set of things
                // to prune.
                std::set_union(paths.begin(), paths.end(),
                    excludepaths.begin(), excludepaths.end(),
                    std::inserter(combined, combined.end()));
            }
            else
            {
                // Pruning selected. Remove the "excludes" from the set of
                // things to prune.
                std::set_difference(paths.begin(), paths.end(),
                    excludepaths.begin(), excludepaths.end(),
                    std::inserter(combined, combined.end()));
            }
            paths.swap(combined);
        }

        // After the reversal from inclusion to exclusion, find all paths in
        // the limit set that are contained by any prim in the path set. Only
        // these exact prims should ever be modified.
        if (limitpruneprims)
        {
            const XUSD_PathSet  &limitpaths =
                limitpruneprims->getExpandedPathSet();
            XUSD_PathSet         intersection;

            for (auto it = paths.begin(); it != paths.end(); )
            {
                auto limitit = limitpaths.lower_bound(*it);
                while (limitit != limitpaths.end() && limitit->HasPrefix(*it))
                    intersection.emplace(*limitit++);

                for (auto currit = it++;
                     it != paths.end() && it->HasPrefix(*currit);
                     ++it)
                { /* Advance "it" past descendents of the current path. */ }
            }
            paths.swap(intersection);
        }

        // Promoting the prune operation from children to parents is the very
        // last step, as it should always result in a more efficient way of
        // representing exactly the same set of pruned prims.
        if (prune_ancestors_automatically)
        {
            HUSDgetMinimalPathsForInheritableProperty(
                findprims.getFindPointInstancerIds(),
                stage, paths);
        }

	for (auto &&path : paths)
	{
	    auto	 usdprim = stage->GetPrimAtPath(path);

	    if (!usdprim)
		continue;

	    if (prune_method == HUSD_Prune::MakeInvisible)
	    {
		UsdGeomImageable imageable(usdprim);

		if (!imageable)
		    continue;

                UsdAttribute visattr = imageable.CreateVisibilityAttr();
                HUSDupdateValueTimeSampling(myTimeSampling, visattr);
                UsdTimeCode usdtime(HUSDgetEffectiveUsdTimeCode(
                    timecode, visattr));
                if (prune)
                    visattr.Set(UsdGeomTokens->invisible, usdtime);
                else
                    visattr.Set(UsdGeomTokens->inherited, usdtime);
	    }
	    else
            {
                if (prune)
                    usdprim.SetActive(false);
                else
                    usdprim.SetActive(true);
            }

            if (pruned_prims)
                pruned_prims->append(path.GetText());
	}

        // Prune individual point instancer instances if requested.
        if (findprims.getFindPointInstancerIds())
        {
            UT_StringMap<UT_Int64Array> excluded_ids;
            const UT_StringMap<UT_Int64Array> *instmap;

            if (prune_unselected)
            {
                findprims.getExcludedPointInstancerIds(excluded_ids, timecode);
                instmap = &excluded_ids;
            }
            else
                instmap = &findprims.getPointInstancerIds();

            VtArray<int64> invisible_ids;

            for (auto it = instmap->begin(); it != instmap->end(); ++it)
            {
                SdfPath sdfpath(HUSDgetSdfPath(it->first));
                UsdGeomPointInstancer instancer(stage->GetPrimAtPath(sdfpath));

                if (instancer)
                {
                    UsdAttribute idsattr = instancer.GetInvisibleIdsAttr();
                    HUSDupdateValueTimeSampling(myTimeSampling, idsattr);
                    UsdTimeCode usdtime(HUSDgetEffectiveUsdTimeCode(
                        timecode, idsattr));

                    invisible_ids.clear();
                    if (idsattr.Get(&invisible_ids, usdtime))
                    {
                        UT_Int64Array    combined_ids;

                        combined_ids.setSize(invisible_ids.size());
                        for (int i = 0, n = invisible_ids.size(); i < n; i++)
                            combined_ids(i) = invisible_ids[i];
                        combined_ids.sort();
                        if (prune)
                            combined_ids.sortedUnion(it->second);
                        else
                            combined_ids.sortedSetDifference(it->second);
                        invisible_ids.assign(
                            combined_ids.begin(), combined_ids.end());
                    }
                    else if (prune)
                        invisible_ids.assign(
                            it->second.begin(), it->second.end());
                    idsattr.Set(invisible_ids, usdtime);
                }
            }
        }

        return true;
    }

    return false;
}

bool
HUSD_Prune::getIsTimeVarying() const
{
    return HUSDisTimeVarying(myTimeSampling);
}

