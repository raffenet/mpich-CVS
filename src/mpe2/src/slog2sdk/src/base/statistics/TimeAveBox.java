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
import java.util.SortedSet;
import java.util.TreeSet;
import java.util.List;
import java.util.ArrayList;
import java.util.Stack;
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
    private static final DrawOrderComparator DRAWING_ORDER
                                             = new DrawOrderComparator();

    private              Map                 map_type2twgt;
    private              List                list_nestables;
    private              SortedSet           set_timeblocks;
    private              CategoryTimeBox[]   typebox_ary;
    private              double              num_real_objs;
    private              double              box_duration;

    private              TimeBoundingBox     curr_timebox;

    public TimeAveBox( final TimeBoundingBox  timebox,
                             boolean          isNestable )
    {
        super( timebox );
        map_type2twgt   = new HashMap();
        box_duration    = super.getDuration();
        num_real_objs   = 0.0d;
        typebox_ary     = null;
        curr_timebox    = null;

        if ( isNestable ) {
            list_nestables = new ArrayList();
            set_timeblocks = new TreeSet( DRAWING_ORDER );
        }
        else {
            list_nestables = null;
            set_timeblocks = null;
        }
    }

    public TimeAveBox( final TimeAveBox  avebox )
    {
        this( avebox, avebox.list_nestables != null );
        this.mergeWithTimeAveBox( avebox );
    }

    public void mergeWithReal( final Drawable  dobj )
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
            twgt  = new CategoryWeight( type, duration_ratio, 0.0f );
            map_type2twgt.put( type, twgt );
        }
        else
            twgt.addInclusiveRatio( duration_ratio );
        // num_real_objs += duration_ratio * dobj.getNumOfPrimitives();
        num_real_objs += overlap_duration / dobj.getDuration()
                       * dobj.getNumOfPrimitives();

        if ( list_nestables != null )
            list_nestables.add( dobj );
    }

    public void mergeWithShadow( final Shadow  shade )
    {
        TimeBoundingBox   timeblock;
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
                this_twgt.rescaleAllRatios( duration_ratio );
                map_type2twgt.put( sobj_type, this_twgt );
            }
            else
                this_twgt.addAllRatios( sobj_twgt, duration_ratio );
        }
	// num_real_objs += duration_ratio * shade.getNumOfRealObjects() ;
        num_real_objs += overlap_duration / shade.getDuration()
                       * shade.getNumOfRealObjects() ;

        if ( list_nestables != null )
            set_timeblocks.add( shade );
    }

    public void mergeWithTimeAveBox( final TimeAveBox  avebox )
    {
        TimeBoundingBox   timeblock;
        Category          abox_type;
        CategoryWeight    abox_twgt, this_twgt;
        Iterator          abox_twgts;
        double            overlap_duration;
        float             duration_ratio;
        int               idx;

        overlap_duration = super.getIntersectionDuration( avebox );
        duration_ratio   = (float) (overlap_duration / box_duration);

        abox_twgts = avebox.map_type2twgt.values().iterator();
        while ( abox_twgts.hasNext() ) {
            abox_twgt = (CategoryWeight) abox_twgts.next();
            abox_type = abox_twgt.getCategory();
            this_twgt = (CategoryWeight) map_type2twgt.get( abox_type );
            if ( this_twgt == null ) {
                this_twgt = new CategoryWeight( abox_twgt );// abox_twgt's clone
                this_twgt.rescaleAllRatios( duration_ratio );
                map_type2twgt.put( abox_type, this_twgt );
            }
            else
                this_twgt.addAllRatios( abox_twgt, duration_ratio );
        }
        num_real_objs += overlap_duration / avebox.getDuration()
                       * avebox.num_real_objs;

        if ( list_nestables != null )
            set_timeblocks.add( avebox );
    }

    private void patchSetOfTimeBlocks()
    {
        TimeBoundingBox  first_timeblock, last_timeblock, new_timeblock;

        new_timeblock   = new TimeBoundingBox( TimeBoundingBox.ALL_TIMES );
        first_timeblock = null;
        if ( ! set_timeblocks.isEmpty() )
            first_timeblock = (TimeBoundingBox) set_timeblocks.first();
        if (    first_timeblock != null
             && first_timeblock.contains( super.getEarliestTime() ) )
            new_timeblock.setLatestTime( first_timeblock.getEarliestTime() );
        else
            new_timeblock.setLatestTime( super.getEarliestTime() );
        set_timeblocks.add( new_timeblock );

        new_timeblock  = new TimeBoundingBox( TimeBoundingBox.ALL_TIMES );
        last_timeblock = null;
        if ( ! set_timeblocks.isEmpty() )
            last_timeblock = (TimeBoundingBox) set_timeblocks.last();
        if (    last_timeblock != null
             && last_timeblock.contains( super.getLatestTime() ) )
            new_timeblock.setEarliestTime( last_timeblock.getLatestTime() );
        else
            new_timeblock.setEarliestTime( super.getLatestTime() );
        set_timeblocks.add( new_timeblock );
    }

    //  same as Shadow.setNestingExclusion()
    private void setRealDrawableExclusion()
    {
        Object[]          timeblocks;
        Stack             nesting_stack;
        Iterator          dobjs_itr;
        Drawable          curr_dobj, stacked_dobj;

        timeblocks     = set_timeblocks.toArray();
        nesting_stack  = new Stack();

        //  Assume dobjs_itr returns in Increasing Starttime order
        dobjs_itr      = list_nestables.iterator();
        while ( dobjs_itr.hasNext() ) {
            curr_dobj  = (Drawable) dobjs_itr.next();
            curr_dobj.initExclusion( timeblocks );
            while ( ! nesting_stack.empty() ) {
                stacked_dobj = (Drawable) nesting_stack.peek();
                if ( stacked_dobj.covers( curr_dobj ) ) {
                    stacked_dobj.decrementExclusion( curr_dobj.getExclusion() );
                    break;
                }
                else
                    nesting_stack.pop();
            }
            nesting_stack.push( curr_dobj );
        }
        nesting_stack.clear();
        nesting_stack  = null;

        timeblocks     = null;
        set_timeblocks.clear();
        set_timeblocks = null;
    }

    private void adjustMapOfCategoryWeights()
    {
        Iterator          dobjs_itr;
        Drawable          curr_dobj;
        Category          dobj_type;
        CategoryWeight    dobj_twgt;
        float             excl_ratio;

        dobjs_itr      = list_nestables.iterator();
        while ( dobjs_itr.hasNext() ) {
            curr_dobj  = (Drawable) dobjs_itr.next();
            excl_ratio = (float) ( curr_dobj.getExclusion() / box_duration );
            dobj_type  = curr_dobj.getCategory();
            // CategoryWeight is guaranteed to be in map_type2twgt
            dobj_twgt  = (CategoryWeight) map_type2twgt.get( dobj_type );
            dobj_twgt.addExclusiveRatio( excl_ratio );
        }
        list_nestables.clear();
        // Don't set list_nestables=null so list_nestables indicates Nestability
        // list_nestables = null;
    }

    public void setNestingExclusion()
    {
        this.patchSetOfTimeBlocks();
        this.setRealDrawableExclusion();
        this.adjustMapOfCategoryWeights();
    }

    public double getAveNumOfRealObjects()
    {
        return num_real_objs;
    }

    public void initializeCategoryTimeBoxes()
    {
        Iterator        twgts_itr;
        CategoryWeight  twgt;
        CategoryTimeBox typebox;
        int             idx;

        if ( typebox_ary == null ) {
            typebox_ary = new CategoryTimeBox[ map_type2twgt.size() ];
            idx         = 0;
            twgts_itr   = map_type2twgt.values().iterator();
            while ( twgts_itr.hasNext() ) {
                twgt    = (CategoryWeight) twgts_itr.next();
                typebox = new CategoryTimeBox( twgt );
                typebox_ary[ idx ] = typebox;
                idx++;
            }
        }
    }

    public CategoryTimeBox[] arrayOfCategoryTimeBoxes()
    { return typebox_ary; }

    public TimeBoundingBox  getCurrentTimeBoundingBox()
    {
        if ( curr_timebox == null )
            curr_timebox = new TimeBoundingBox();
        return curr_timebox;
    }

    public String toString()
    {
        StringBuffer rep = new StringBuffer( super.toString() );
        rep.append( " Nrobjs=" + (float) num_real_objs );

        if ( map_type2twgt.size() > 0 ) {
            Object[] twgts;
            twgts = map_type2twgt.values().toArray( new CategoryWeight[0] );
            Arrays.sort( twgts, CategoryWeight.INCL_RATIO_ORDER );
            int  twgts_length = twgts.length;
            for ( int idx = 0; idx < twgts_length; idx++ )
                rep.append( "\n" + twgts[ idx ] );
            rep.append( "\n" );
        }
        return rep.toString();
    }
}
