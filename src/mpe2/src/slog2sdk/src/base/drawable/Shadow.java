/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package base.drawable;

import java.awt.Graphics2D;
import java.awt.Stroke;
import java.awt.BasicStroke;
import java.awt.Insets;
import java.awt.Color;
import java.awt.Point;
import java.util.Arrays;
import java.util.Map;
import java.util.HashMap;
import java.util.List;
import java.util.ArrayList;
import java.util.Iterator;

import base.io.MixedDataInput;
import base.io.MixedDataOutput;
import base.topology.Line;
import base.topology.Arrow;
import base.topology.State;
import base.topology.PreviewState;

public class Shadow extends Primitive
{
    private static final int     BYTESIZE = TimeBoundingBox.BYTESIZE /*super*/
                                          + 8  /* num_real_objs */
                                          + 4  /* map_type2twgt's size() */;

    private              long              num_real_objs;

    private              CategoryWeight[]  twgt_ary;         // For Input
    private              Map               map_type2dobjs;   // For Output
    private              Map               map_type2twgt;    // For Output

    private              Category          selected_subtype; // For Jumpshot

    // For SLOG-2 Input
    public Shadow()
    {
        super();
        num_real_objs    = 0;
        twgt_ary         = null;
        map_type2dobjs   = null;
        map_type2twgt    = null;
        selected_subtype = null;
    }

    // For SLOG-2 Output
    public Shadow( final Shadow shade )
    {
        super( shade );
        num_real_objs    = shade.num_real_objs;

        twgt_ary         = null;
        map_type2twgt    = new HashMap();
        map_type2dobjs   = null;

        //  Make a deep copy of shade's map_type2twgt
        CategoryWeight  shade_twgt;
        Iterator        shade_twgts_itr;
        shade_twgts_itr = shade.map_type2twgt.values().iterator();
        while ( shade_twgts_itr.hasNext() ) {
            shade_twgt  = (CategoryWeight) shade_twgts_itr.next();
            map_type2twgt.put( shade_twgt.getCategory(),
                               new CategoryWeight( shade_twgt ) );
        }
        selected_subtype = null;   // meaningless for SLOG-2 Output
    }

    // For SLOG-2 Output
    public Shadow( Category shadow_type, final Primitive prime )
    {
        super( shadow_type, prime );
        num_real_objs   = 1;

        twgt_ary        = null;
        map_type2twgt   = new HashMap();
        map_type2dobjs  = new HashMap();

        List  dobj_list;
        dobj_list       = new ArrayList();
        dobj_list.add( prime );
        map_type2dobjs.put( prime.getCategory(), dobj_list );
        selected_subtype = null;   // meaningless for SLOG-2 Output
    }

    public void mergeWithPrimitive( final Primitive prime )
    {
        Coord[] prime_vtxs = prime.getVertices();
        Coord[] shade_vtxs = super.getVertices();

        if ( prime_vtxs.length != shade_vtxs.length ) {
            System.err.println( "Shadow.mergeWithPrimitive(): ERROR! "
                              + "Incompatible Topology between "
                              + "Shadow and Primitive." );
            System.exit( 1 );
        }

        // do a Time Average over the total number of real drawables
        for ( int idx = 0; idx < shade_vtxs.length; idx++ )
            shade_vtxs[ idx ].time = aveOverAllObjs( shade_vtxs[ idx ].time,
                                                     this.num_real_objs,
                                                     prime_vtxs[ idx ].time,
                                                     1 );
        super.affectTimeBounds( prime );

        // Need to figure out how to do error estimation.
        // Maybe <X^2> is needed to compute the standard dev..
        // time_err = ( super.getLatestTime() - super.getEarliestTime() ) / 2.0;
        num_real_objs++;

        List dobj_list = (List) map_type2dobjs.get( prime.getCategory() );
        if ( dobj_list == null ) {
            dobj_list = new ArrayList();
            dobj_list.add( prime );
            map_type2dobjs.put( prime.getCategory(), dobj_list );
        }
        else
            dobj_list.add( prime );
    }

    public void mergeWithShadow( final Shadow sobj )
    {
        // System.err.println( "Shadow.mergeWithShadow(): START" );
        // System.err.println( "\tThe     Shadow=" + this );
        // System.err.println( "\tAnother Shadow=" + sobj );
        Coord[] sobj_vtxs  = sobj.getVertices();
        Coord[] shade_vtxs = super.getVertices();

        if ( sobj_vtxs.length != shade_vtxs.length ) {
            System.err.println( "Shadow.mergeWithShadow(): ERROR! "
                              + "Incompatible Topology between "
                              + "the 2 Shadows." );
            System.exit( 1 );
        }

        double old_duration, new_duration;
        old_duration = super.getDuration();
        // do a Time Average over the total number of real drawables
        for ( int idx = 0; idx < shade_vtxs.length; idx++ )
            shade_vtxs[ idx ].time = aveOverAllObjs( shade_vtxs[ idx ].time,
                                                     this.num_real_objs,
                                                     sobj_vtxs[ idx ].time,
                                                     sobj.num_real_objs );
        super.affectTimeBounds( sobj );
        new_duration = super.getDuration();

        // Need to figure out how to do error estimation.
        // Maybe <X^2> is needed to compute the standard dev..
        // time_err = ( super.getLatestTime() - super.getEarliestTime() ) / 2.0;
        num_real_objs += sobj.num_real_objs;

        // Since this class's TimeBoundingBox has been affected by sobj,
        // all map_type2twgt must adjust their weight accordingly. 
        CategoryWeight this_twgt, sobj_twgt;
        Iterator       this_twgts_itr, sobj_twgts_itr;
        float          duration_ratio;
        if ( old_duration != new_duration ) {
            duration_ratio = (float) ( old_duration / new_duration );
            this_twgts_itr = this.map_type2twgt.values().iterator();
            while ( this_twgts_itr.hasNext() ) {
                this_twgt = (CategoryWeight) this_twgts_itr.next(); 
                this_twgt.rescaleRatio( duration_ratio ); 
            }
        }

        // Merge with sobj's type_wgt[] with adjustment w.r.t this duration
        Category  sobj_type;
        double sobj_duration  = sobj.getDuration();
        duration_ratio = (float) ( sobj_duration / new_duration );
        sobj_twgts_itr = sobj.map_type2twgt.values().iterator();
        while ( sobj_twgts_itr.hasNext() ) {
            sobj_twgt = (CategoryWeight) sobj_twgts_itr.next();
            sobj_type = sobj_twgt.getCategory();
            this_twgt = (CategoryWeight) this.map_type2twgt.get( sobj_type ); 
            if ( this_twgt == null ) {
                this_twgt = new CategoryWeight( sobj_twgt );// sobj_twgt's clone
                this_twgt.rescaleRatio( duration_ratio ); 
                map_type2twgt.put( sobj_type, this_twgt );
            }
            else
                this_twgt.addRatio( sobj_twgt, duration_ratio );
        }

        // System.err.println( "Shadow.mergeWithShadow(): END" );
    }

    // For SLOG-2 Output API
    public void setMapOfCategoryWeights()
    {
        Iterator   type_dobjs_itr, dobjs_itr;
        Map.Entry  type_dobj;
        List       dobj_list;
        Category   type;
        double     shadow_duration;
        double     ratio;
        float      weight;
        int        size;
        size             = map_type2dobjs.size();
        shadow_duration  = super.getDuration();
        type_dobjs_itr   = map_type2dobjs.entrySet().iterator();
        while ( type_dobjs_itr.hasNext() ) {
            type_dobj  = (Map.Entry) type_dobjs_itr.next();
            type       = (Category) type_dobj.getKey();
            dobj_list  = (List) type_dobj.getValue();

            // Compute the total weight of each Category of dobj in this sobj
            weight     = 0.0f;
            dobjs_itr  = dobj_list.iterator();
            while ( dobjs_itr.hasNext() ) {
                ratio  = ((TimeBoundingBox) dobjs_itr.next()).getDuration()
                       / shadow_duration;
                weight += (float) ratio;
            }
            map_type2twgt.put( type, new CategoryWeight( type, weight ) );
            dobj_list  = null;
        }
        map_type2dobjs.clear();
        map_type2dobjs = null;  // set to null so toString() works
    }

    private static double aveOverAllObjs( double sobj_time, long sobj_Nobjs,
                                          double dobj_time, long dobj_Nobjs )
    {
        return ( ( sobj_time * sobj_Nobjs + dobj_time * dobj_Nobjs )
               / ( sobj_Nobjs + dobj_Nobjs ) );
    }

    public int getByteSize()
    {
        if ( twgt_ary != null )  // For SLOG-2 Input
            return super.getByteSize() + BYTESIZE
                 + CategoryWeight.BYTESIZE * twgt_ary.length;
        else if ( map_type2dobjs != null )  // For SLOG-2 Output
            return super.getByteSize() + BYTESIZE
                 + CategoryWeight.BYTESIZE * map_type2dobjs.size();
        else                                // For SLOG-2 Output
            return super.getByteSize() + BYTESIZE
                 + CategoryWeight.BYTESIZE * map_type2twgt.size();
    }

    // For SLOG-2 Input API, used by BufForShadows.readObject()
    public boolean resolveCategory( Map categorymap )
    {
        boolean  allOK = super.resolveCategory( categorymap );
        if ( twgt_ary != null )
            for ( int idx = twgt_ary.length-1; idx >= 0; idx-- )
                allOK = allOK && twgt_ary[ idx ].resolveCategory( categorymap );
        return allOK;
    }

    // For SLOG-2 Input API i.e. Jumpshot
    public CategoryWeight[] arrayOfCategoryWeights()
    {
        return twgt_ary;
    }

    // For SLOG-2 Input API i.e. Jumpshot
    public Category getSelectedSubCategory()
    {
        return selected_subtype;
    }

    // For SLOG-2 Input API i.e. Jumpshot
    public void clearSelectedSubCategory()
    {
        selected_subtype = null;
    }

    public long getNumOfRealObjects()
    {
        return num_real_objs;
    }

    public void writeObject( MixedDataOutput outs )
    throws java.io.IOException
    {
        super.writeObject( outs );
        TimeBoundingBox.writeObject( this, outs );
        outs.writeLong( num_real_objs );
     // System.err.println( "\touts.size=" + ((DataOutputStream)outs).size() );
        if ( this.map_type2twgt.size() > 0 ) {
            Object[]  twgts;
            twgts = this.map_type2twgt.values().toArray();
            Arrays.sort( twgts, CategoryWeight.RATIO_ORDER );
            int  twgts_length = twgts.length;
            outs.writeInt( twgts_length );
            for ( int idx = 0; idx < twgts_length; idx++ )
                ((CategoryWeight) twgts[ idx ]).writeObject( outs );
        }
        else
            outs.writeInt( 0 );
    }

    public Shadow( MixedDataInput ins )
    throws java.io.IOException
    {
        super();
        this.readObject( ins );
    }

    public void readObject( MixedDataInput ins )
    throws java.io.IOException
    {
        super.readObject( ins );
        TimeBoundingBox.readObject( this, ins );
        num_real_objs = ins.readLong();

        CategoryWeight  twgt;
        int             Nentries, ientry;

        Nentries   = ins.readInt();
        if ( Nentries > 0 ) {
            twgt_ary = new CategoryWeight[ Nentries ];
            for ( ientry = 0; ientry < Nentries; ientry++ )
                twgt_ary[ ientry ] = new CategoryWeight( ins );
        }
        else
            twgt_ary = null;
    }

    public String toString()
    {
        StringBuffer rep = new StringBuffer( super.toString() );
        rep.append( " Nrobjs=" + num_real_objs );

        CategoryWeight[]  twgts = null;
        if ( twgt_ary != null )
            twgts = twgt_ary;
        else
            if ( map_type2twgt != null )
                twgts = (CategoryWeight[]) map_type2twgt.values().toArray();

        if ( twgts != null ) {
            int  twgts_length = twgts.length;
            for ( int idx = 0; idx < twgts_length; idx++ )
                rep.append( "\n" + twgts[ idx ] );
        }
        return rep.toString();
    }



    private static Insets Empty_Border = new Insets( 0, 2, 0, 2 );
    private static Stroke Line_Stroke  = new BasicStroke( 3.0f );

    public static void setStateInsetsDimension( int width, int height )
    {
        Empty_Border = new Insets( height, width, height, width );
    }

    public static void setArrowLineThickness( float thickness )
    {
        Line_Stroke = new BasicStroke( thickness );
    }


    // Implementation of abstract methods.

    /* 
        0.0f < nesting_ftr <= 1.0f
    */
    public  int  drawState( Graphics2D g, CoordPixelXform coord_xform,
                            Map map_line2row, DrawnBoxSet drawn_boxes,
                            ColorAlpha color )
    {
        // Coord  start_vtx, final_vtx;
        // start_vtx = this.getStartVertex();
        // final_vtx = this.getFinalVertex();

        double tStart, tFinal;
        tStart = super.getEarliestTime();    /* different from Primitive */
        tFinal = super.getLatestTime();      /* different from Primitive */

        int    rowID;
        float  nesting_ftr;
        rowID       = super.getRowID();
        nesting_ftr = super.getNestingFactor();

        // System.out.println( "\t" + this + " nestftr=" + nesting_ftr );

        float  rStart, rFinal;
        rStart = (float) rowID - nesting_ftr / 2.0f;
        rFinal = rStart + nesting_ftr;

        return PreviewState.draw( g, color,
                                  twgt_ary, Empty_Border, coord_xform,
                                  drawn_boxes.getLastStatePos( rowID ),
                                  tStart, rStart, tFinal, rFinal );
        // return State.draw( g, color, Empty_Border, coord_xform,
        //                    drawn_boxes.getLastStatePos( rowID ),
        //                    tStart, rStart, tFinal, rFinal );
    }

    public  int  drawArrow( Graphics2D g, CoordPixelXform coord_xform,
                            Map map_line2row, DrawnBoxSet drawn_boxes,
                            ColorAlpha color )
    {
        Coord  start_vtx, final_vtx;
        start_vtx = this.getStartVertex();
        final_vtx = this.getFinalVertex();

        double tStart, tFinal;
        tStart = super.getEarliestTime();    /* different from Primitive */
        tFinal = super.getLatestTime();      /* different from Primitive */

        int    iStart, iFinal;
        iStart = ( (Integer)
                   map_line2row.get( new Integer(start_vtx.lineID) )
                 ).intValue();
        iFinal = ( (Integer)
                   map_line2row.get( new Integer(final_vtx.lineID) )
                 ).intValue();

        return Line.draw( g, color, Line_Stroke, coord_xform,
                          drawn_boxes.getLastArrowPos( iStart, iFinal ),
                          tStart, (float) iStart, tFinal, (float) iFinal );
    }

    /* 
        0.0f < nesting_ftr <= 1.0f
    */
    public  boolean isPixelInState( CoordPixelXform coord_xform,
                                    Map map_line2row, Point pix_pt )
    {
        Coord  start_vtx, final_vtx;
        start_vtx = this.getStartVertex();
        final_vtx = this.getFinalVertex();

        double tStart, tFinal;
        tStart = super.getEarliestTime();    /* different from Primitive */
        tFinal = super.getLatestTime();      /* different from Primitive */

        int    rowID;
        float  nesting_ftr;
        /*
        rowID  = ( (Integer)
                   map_line2row.get( new Integer(start_vtx.lineID) )
                 ).intValue();
        */
        rowID       = super.getRowID();
        /* assume NestingFactor has been calculated */
        nesting_ftr = super.getNestingFactor();

        // System.out.println( "\t" + this + " nestftr=" + nesting_ftr );

        float  rStart, rFinal;
        rStart = (float) rowID - nesting_ftr / 2.0f;
        rFinal = rStart + nesting_ftr;

        selected_subtype = PreviewState.containsPixel( twgt_ary, Empty_Border,
                                                       coord_xform, pix_pt,
                                                       tStart, rStart,
                                                       tFinal, rFinal );
        return selected_subtype != null;
    }

    //  assume this Shadow overlaps with coord_xform.TimeBoundingBox
    public  boolean isPixelOnArrow( CoordPixelXform coord_xform,
                                    Map map_line2row, Point pix_pt )
    {
        Coord  start_vtx, final_vtx;
        start_vtx = this.getStartVertex();
        final_vtx = this.getFinalVertex();

        double tStart, tFinal;
        tStart = super.getEarliestTime();    /* different from Primitive */
        tFinal = super.getLatestTime();      /* different from Primitive */

        float  rStart, rFinal;
        rStart = ( (Integer)
                   map_line2row.get( new Integer(start_vtx.lineID) )
                 ).floatValue();
        rFinal = ( (Integer)
                   map_line2row.get( new Integer(final_vtx.lineID) )
                 ).floatValue();

        return Line.containsPixel( coord_xform, pix_pt,
                                   tStart, rStart, tFinal, rFinal );
    }

    public boolean containSearchable()
    {
        CategoryWeight  twgt;
        int             idx;

        for ( idx = twgt_ary.length-1; idx >= 0; idx-- ) {
             twgt = twgt_ary[ idx ];
             if ( twgt.getCategory().isVisiblySearchable() )
                 return true;
        }
        return false;
    }
}
