/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package base.statistics;

import java.util.Map;
import java.util.HashMap;
import java.util.Arrays;
import java.util.Iterator;

import base.drawable.TimeBoundingBox;
import base.drawable.CategoryWeight;
import base.drawable.Category;
import base.drawable.Drawable;
import base.drawable.Composite;
import base.drawable.Primitive;
import base.drawable.Shadow;

public class TimeAveBox extends TimeBoundingBox
{
    private              Map               map_type2twgt;    // For Input
    private              double            box_duration;
    private              double            num_real_objs;

    public TimeAveBox( final TimeBoundingBox timebox )
    {
        super( timebox );
        map_type2twgt   = new HashMap();
        box_duration    = super.getDuration();
        num_real_objs   = 0.0d;
    }

    public void mergeWithReal( final Drawable dobj )
    {
        Category          type;
        CategoryWeight    twgt;
        double            overlap_duration;
        float             duration_ratio;

        overlap_duration = super.getIntersectionDuration( dobj );
        duration_ratio   = (float) (overlap_duration / box_duration);

        type  = dobj.getCategory();
        twgt  = (CategoryWeight) map_type2twgt.get( type );
        if ( twgt == null ) {
            twgt  = new CategoryWeight( type, duration_ratio );
            map_type2twgt.put( type, twgt );
        }
        else
            twgt.addRatio( duration_ratio );
        // num_real_objs += duration_ratio * dobj.getNumOfPrimitives();
        num_real_objs += overlap_duration / dobj.getDuration()
                       * dobj.getNumOfPrimitives();
    }

    public void mergeWithShadow( final Shadow shade )
    {
        Category          sobj_type;
        CategoryWeight    sobj_twgt, this_twgt;
        CategoryWeight[]  sobj_twgts;
        double            overlap_duration;
        float             duration_ratio;
        int               idx;

        overlap_duration = super.getIntersectionDuration( shade );
        duration_ratio   = (float) (overlap_duration / box_duration);

        sobj_twgts = shade.arrayOfCategoryWeights();
        for ( idx = sobj_twgts.length-1 ; idx >= 0 ; idx-- ) {
            sobj_twgt = sobj_twgts[ idx ];
            sobj_type = sobj_twgt.getCategory();
            this_twgt = (CategoryWeight) map_type2twgt.get( sobj_type );
            if ( this_twgt == null ) {
                this_twgt = new CategoryWeight( sobj_twgt );// sobj_twgt's clone
                this_twgt.rescaleRatio( duration_ratio );
                map_type2twgt.put( sobj_type, this_twgt );
            }
            else
                this_twgt.addRatio( sobj_twgt, duration_ratio );
        }
	// num_real_objs += duration_ratio * shade.getNumOfRealObjects() ;
        num_real_objs += overlap_duration / shade.getDuration()
                       * shade.getNumOfRealObjects() ;
    }

    public String toString()
    {
        StringBuffer rep = new StringBuffer( super.toString() );
        rep.append( " Nrobjs=" + (float) num_real_objs );

        if ( map_type2twgt.size() > 0 ) {
            Object[] twgts;
            twgts = map_type2twgt.values().toArray();
            Arrays.sort( twgts, CategoryWeight.RATIO_ORDER );
            int  twgts_length = twgts.length;
            for ( int idx = 0; idx < twgts_length; idx++ )
                rep.append( "\n" + twgts[ idx ] );
            rep.append( "\n" );
        }
        return rep.toString();
    }
}
