/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package base.statistics;

import java.awt.Graphics2D;
import java.awt.Point;
import java.awt.Color;
import java.util.Map;
import java.util.HashMap;
import java.util.List;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.Collections;

import base.drawable.TimeBoundingBox;
import base.drawable.Topology;
import base.drawable.Category;
import base.drawable.CategoryWeight;
import base.drawable.Drawable;
import base.drawable.Primitive;
import base.drawable.Shadow;
import base.drawable.CoordPixelXform;
import base.topology.SummaryState;

public class BufForTimeAveBoxes extends TimeBoundingBox
{
    private TimeBoundingBox   timebounds;
    private Map               map_lines2nestable;   /* state and composite */
    private Map               map_lines2nestless;   /* arrow/event */
    private Map               map_rows2nestable;    /* state and composite */
    private Map               map_rows2nestless;    /* arrow/event */
    

    public BufForTimeAveBoxes( final TimeBoundingBox timebox )
    {
        super( timebox );
        timebounds          = timebox;
        map_lines2nestable  = new HashMap();
        map_lines2nestless  = new HashMap();
        map_rows2nestable   = null;
        map_rows2nestless   = null;
    }

    //  Assume dobj is Nestable
    public void mergeWithNestable( final Drawable dobj )
    {
        List        key;
        Topology    topo;
        TimeAveBox  avebox;
        Integer     lineID_start, lineID_final;

        key = new ArrayList();
        topo = dobj.getCategory().getTopology();
        key.add( topo );
        // key.addAll( prime.getListOfVertexLineIDs() );
        lineID_start = new Integer( dobj.getStartVertex().lineID );
        lineID_final = new Integer( dobj.getFinalVertex().lineID );
        key.add( lineID_start );
        key.add( lineID_final );
        avebox = null;
        avebox = (TimeAveBox) map_lines2nestable.get( key );
        if ( avebox == null ) {
            avebox = new TimeAveBox( timebounds, true );
            map_lines2nestable.put( key, avebox );
        }
        if ( dobj instanceof Shadow )
            avebox.mergeWithShadow( (Shadow) dobj );
        else
            avebox.mergeWithReal( dobj );
    }

    //  Assume dobj is Nestless
    public void mergeWithNestless( final Drawable dobj )
    {
        List        key;
        Topology    topo;
        TimeAveBox  avebox;
        Integer     lineID_start, lineID_final;

        key = new ArrayList();
        topo = dobj.getCategory().getTopology();
        key.add( topo );
        // key.addAll( prime.getListOfVertexLineIDs() );
        lineID_start = new Integer( dobj.getStartVertex().lineID );
        lineID_final = new Integer( dobj.getFinalVertex().lineID );
        key.add( lineID_start );
        key.add( lineID_final );
        avebox = null;
        avebox = (TimeAveBox) map_lines2nestless.get( key );
        if ( avebox == null ) {
            avebox = new TimeAveBox( timebounds, false );
            map_lines2nestless.put( key, avebox );
        }
        if ( dobj instanceof Shadow )
            avebox.mergeWithShadow( (Shadow) dobj );
        else
            avebox.mergeWithReal( dobj );
    }

    public void setNestingExclusion()
    {
        Iterator    avebox_itr;
        TimeAveBox  avebox;
        
        avebox_itr = map_lines2nestable.values().iterator();
        while ( avebox_itr.hasNext() ) {
            avebox = (TimeAveBox) avebox_itr.next();
            avebox.setNestingExclusion();
        }
    }

    public String toString()
    {
        StringBuffer rep = new StringBuffer( super.toString() );
        
        if ( map_lines2nestable.size() > 0 ) {
            Map.Entry  entry;
            Object[]   key;
            TimeAveBox avebox;
            Iterator   entries  = map_lines2nestable.entrySet().iterator();
            while ( entries.hasNext() ) {
                entry  = (Map.Entry) entries.next();
                key    = ( (List) entry.getKey() ).toArray();
                avebox = (TimeAveBox) entry.getValue();
                rep.append( "\n" + key[0] + ": " + key[1] + ", " + key[2] );
                rep.append( "\n" + avebox );
            }
            rep.append( "\n" );
        }

        if ( map_lines2nestless.size() > 0 ) {
            Map.Entry  entry;
            Object[]   key;
            TimeAveBox avebox;
            Iterator   entries  = map_lines2nestless.entrySet().iterator();
            while ( entries.hasNext() ) {
                entry  = (Map.Entry) entries.next();
                key    = ( (List) entry.getKey() ).toArray();
                avebox = (TimeAveBox) entry.getValue();
                rep.append( "\n" + key[0] + ": " + key[1] + ", " + key[2] );
                rep.append( "\n" + avebox );
            }
            rep.append( "\n" );
        }

        return rep.toString();
    }



    /*  Drawing related API */

    private static List  getLine2RowMappedKey( final Map   map_line2row,
                                               final List  old_keylist )
    {
        List      new_keylist;
        Object[]  old_keys;

        new_keylist    = new ArrayList(); 
        old_keys       = old_keylist.toArray(); 
        new_keylist.add( old_keys[0] );  // Topology
        new_keylist.add( map_line2row.get( (Integer) old_keys[1] ) );
        new_keylist.add( map_line2row.get( (Integer) old_keys[2] ) );
        return new_keylist;
    }

    public void initializeDrawing( final Map      map_line2row,
                                         boolean  isZeroTimeOrigin )
    {
        Map.Entry         entry;
        Iterator          entry_itr, avebox_itr;
        List              lined_key, rowed_key;
        TimeAveBox        lined_avebox, rowed_avebox;

        // For Nestables, i.e. states
        map_rows2nestable  = new HashMap();
        entry_itr = map_lines2nestable.entrySet().iterator();
        while ( entry_itr.hasNext() ) {
            entry        = (Map.Entry)  entry_itr.next();
            lined_key    = (List)       entry.getKey();
            lined_avebox = (TimeAveBox) entry.getValue();
            
            rowed_key    = getLine2RowMappedKey( map_line2row, lined_key );
            rowed_avebox = (TimeAveBox) map_rows2nestable.get( rowed_key );
            if ( rowed_avebox == null ) {
                rowed_avebox = new TimeAveBox( lined_avebox );
                map_rows2nestable.put( rowed_key, rowed_avebox );
            }
            else
                rowed_avebox.mergeWithTimeAveBox( lined_avebox );
        }

        // Do setNestingExclusion() for map_rows2nestable
        avebox_itr = map_rows2nestable.values().iterator();
        while ( avebox_itr.hasNext() ) {
            rowed_avebox = (TimeAveBox) avebox_itr.next();
            rowed_avebox.setNestingExclusion();
            rowed_avebox.initializeCategoryTimeBoxes( isZeroTimeOrigin );
        }

        // For Nestlesses, i.e. arrows
        map_rows2nestless  = new HashMap();
        entry_itr = map_lines2nestless.entrySet().iterator();
        while ( entry_itr.hasNext() ) {
            entry        = (Map.Entry)  entry_itr.next();
            lined_key    = (List)       entry.getKey();
            lined_avebox = (TimeAveBox) entry.getValue();

            rowed_key    = getLine2RowMappedKey( map_line2row, lined_key );
            rowed_avebox = (TimeAveBox) map_rows2nestless.get( rowed_key );
            if ( rowed_avebox == null ) {
                rowed_avebox = new TimeAveBox( lined_avebox );
                map_rows2nestless.put( rowed_key, rowed_avebox );
            }
            else
                rowed_avebox.mergeWithTimeAveBox( lined_avebox );
        }

        // Initialize map_rows2nestless
        avebox_itr = map_rows2nestless.values().iterator();
        while ( avebox_itr.hasNext() ) {
            rowed_avebox = (TimeAveBox) avebox_itr.next();
            rowed_avebox.initializeCategoryTimeBoxes( isZeroTimeOrigin );
        }
    }

    public int  drawAllStates( Graphics2D       g,
                               CoordPixelXform  coord_xform )
    {
        Map.Entry          entry;
        Object[]           key;
        Iterator           entries;
        Topology           topo;
        TimeAveBox         avebox;
        CategoryTimeBox[]  typeboxes;
        int                rowID;
        float              rStart, rFinal;
        int                count;

        count   = 0;
        entries = map_rows2nestable.entrySet().iterator();
        while ( entries.hasNext() ) {
            entry     = (Map.Entry) entries.next();
            key       = ( (List) entry.getKey() ).toArray();
            avebox    = (TimeAveBox) entry.getValue();

            typeboxes = avebox.arrayOfCategoryTimeBoxes();

            topo      = (Topology) key[0];
            rowID     = ( (Integer) key[1] ).intValue();

            rStart    = rowID - 0.4f;
            rFinal    = rowID + 0.4f;

            count    += SummaryState.draw( g, null, typeboxes, coord_xform,
                                           rStart, rFinal );
        }
        return count;
    }

    public int  drawAllArrows( Graphics2D       g,
                               CoordPixelXform  coord_xform )
    {
        Map.Entry          entry;
        Object[]           key;
        Iterator           entries;
        Topology           topo;
        TimeAveBox         avebox;
        CategoryTimeBox[]  typeboxes;
        int                rowID1, rowID2;
        float              rStart, rFinal;
        int                count;

        count   = 0;
        entries = map_rows2nestless.entrySet().iterator();
        while ( entries.hasNext() ) {
            entry     = (Map.Entry) entries.next();
            key       = ( (List) entry.getKey() ).toArray();
            avebox    = (TimeAveBox) entry.getValue();

            typeboxes = avebox.arrayOfCategoryTimeBoxes();

            topo      = (Topology) key[0];
            rowID1    = ( (Integer) key[1] ).intValue();
            rowID2    = ( (Integer) key[2] ).intValue();

            rStart    = (float) rowID1;
            rFinal    = (float) rowID2;

            // count    += SummaryLine.draw( g, null, typeboxes, coord_xform,
            //                               rStart, rFinal );
        }
        return count;
    }

    public CategoryWeight  getCategoryWeightAt( CoordPixelXform  coord_xform,
                                                Point            pix_pt )
    {
        Map.Entry          entry;
        Object[]           key;
        Iterator           entries;
        Topology           topo;
        TimeAveBox         avebox;
        CategoryTimeBox[]  typeboxes;
        CategoryTimeBox    typebox;
        float              rStart, rFinal;
        int                rowID;

        typebox = null;
        entries = map_rows2nestable.entrySet().iterator();
        while ( entries.hasNext() && typebox == null ) {
            entry     = (Map.Entry) entries.next();
            key       = ( (List) entry.getKey() ).toArray();
            avebox    = (TimeAveBox) entry.getValue();
            typeboxes = avebox.arrayOfCategoryTimeBoxes();

            topo      = (Topology) key[0];
            rowID     = ( (Integer) key[1] ).intValue();

            rStart    = rowID - 0.4f;
            rFinal    = rowID + 0.4f;

            typebox   = SummaryState.containsPixel( typeboxes, null,
                                                  coord_xform, pix_pt,
                                                  rStart, rFinal );
        }

        return ( typebox != null ? typebox.getCategoryWeight() : null );
    }
}
