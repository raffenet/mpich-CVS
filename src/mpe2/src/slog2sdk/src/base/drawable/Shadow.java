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
import java.util.Collections;
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
                                          + 4  /* type_twgt_map's size() */;

    private              long              num_real_objs;
    private              List              twgt_list;        // For Input
    private              Map               type_dobjs_map;   // For Output
    private              Map               type_twgt_map;    // For Output

    private              Category          selected_subtype; // For Jumpshot

    // For SLOG-2 Input
    public Shadow()
    {
        super();
        num_real_objs    = 0;
        twgt_list        = null;
        type_dobjs_map   = null;
        type_twgt_map    = null;
        selected_subtype = null;
    }

    // For SLOG-2 Output
    public Shadow( final Shadow shade )
    {
        super( shade );
        num_real_objs    = shade.num_real_objs;

        twgt_list        = null;
        type_twgt_map    = new HashMap();
        type_dobjs_map   = null;

        //  Make a depp copy of shade's type_twgt_map
        CategoryWeight  shade_twgt;
        Iterator        shade_twgts_itr;
        shade_twgts_itr = shade.type_twgt_map.values().iterator();
        while ( shade_twgts_itr.hasNext() ) {
            shade_twgt  = (CategoryWeight) shade_twgts_itr.next();
            type_twgt_map.put( shade_twgt.getCategory(),
                               new CategoryWeight( shade_twgt ) );
        }
        selected_subtype = null;
    }

    // For SLOG-2 Output
    public Shadow( Category shadow_type, final Primitive prime )
    {
        super( shadow_type, prime );
        num_real_objs   = 1;

        twgt_list       = null;
        type_twgt_map   = new HashMap();
        type_dobjs_map  = new HashMap();

        List  dobj_list;
        dobj_list       = new ArrayList();
        dobj_list.add( prime );
        type_dobjs_map.put( prime.getCategory(), dobj_list );
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

        List dobj_list = (List) type_dobjs_map.get( prime.getCategory() );
        if ( dobj_list == null ) {
            dobj_list = new ArrayList();
            dobj_list.add( prime );
            type_dobjs_map.put( prime.getCategory(), dobj_list );
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
        // all type_twgt_map must adjust their weight accordingly. 
        CategoryWeight this_twgt, sobj_twgt;
        Iterator       this_twgts_itr, sobj_twgts_itr;
        float          duration_ratio;
        if ( old_duration != new_duration ) {
            duration_ratio = (float) ( old_duration / new_duration );
            this_twgts_itr = this.type_twgt_map.values().iterator();
            while ( this_twgts_itr.hasNext() ) {
                this_twgt = (CategoryWeight) this_twgts_itr.next(); 
                this_twgt.rescaleWeight( duration_ratio ); 
            }
        }

        // Merge with sobj's type_wgt[] with adjustment w.r.t this duration
        Category  sobj_type;
        double sobj_duration  = sobj.getDuration();
        duration_ratio = (float) ( sobj_duration / new_duration );
        sobj_twgts_itr = sobj.type_twgt_map.values().iterator();
        while ( sobj_twgts_itr.hasNext() ) {
            sobj_twgt = (CategoryWeight) sobj_twgts_itr.next();
            sobj_type = sobj_twgt.getCategory();
            this_twgt = (CategoryWeight) this.type_twgt_map.get( sobj_type ); 
            if ( this_twgt == null ) {
                this_twgt = new CategoryWeight( sobj_twgt );// sobj_twgt's clone
                this_twgt.rescaleWeight( duration_ratio ); 
                type_twgt_map.put( sobj_type, this_twgt );
            }
            else
                this_twgt.addWeight( sobj_twgt, duration_ratio );
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
        size             = type_dobjs_map.size();
        shadow_duration  = super.getDuration();
        type_dobjs_itr   = type_dobjs_map.entrySet().iterator();
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
            type_twgt_map.put( type, new CategoryWeight( type, weight ) );
            dobj_list  = null;
        }
        type_dobjs_map.clear();
        type_dobjs_map = null;  // set to null so toString() works
    }

    private static double aveOverAllObjs( double sobj_time, long sobj_Nobjs,
                                          double dobj_time, long dobj_Nobjs )
    {
        return ( ( sobj_time * sobj_Nobjs + dobj_time * dobj_Nobjs )
               / ( sobj_Nobjs + dobj_Nobjs ) );
    }

    public int getByteSize()
    {
        if ( twgt_list != null )  // For SLOG-2 Input
            return super.getByteSize() + BYTESIZE
                 + CategoryWeight.BYTESIZE * twgt_list.size();
        else if ( type_dobjs_map != null )  // For SLOG-2 Output
            return super.getByteSize() + BYTESIZE
                 + CategoryWeight.BYTESIZE * type_dobjs_map.size();
        else                                // For SLOG-2 Output
            return super.getByteSize() + BYTESIZE
                 + CategoryWeight.BYTESIZE * type_twgt_map.size();
    }

    // For SLOG-2 Input API
    public boolean resolveCategory( Map categorymap )
    {
        CategoryWeight  twgt;
        Iterator        twgts_itr;
        boolean         allOK = super.resolveCategory( categorymap );
        twgts_itr = twgt_list.iterator();
        while ( twgts_itr.hasNext() ) {
            twgt = (CategoryWeight) twgts_itr.next();
            allOK = allOK && twgt.resolveCategory( categorymap );
        }
        return allOK;
    }

    // For SLOG-2 Input API i.e. Jumpshot
    public Iterator iteratorOfCategoryWeights()
    {
        return twgt_list.iterator();
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
        List  tmp_twgt_list;
        int   Nentries = type_twgt_map.size();
        outs.writeInt( Nentries );
        if ( Nentries > 0 ) {
            tmp_twgt_list = new ArrayList( this.type_twgt_map.values() );
            Collections.sort( tmp_twgt_list, CategoryWeight.INDEX_ORDER );
            Iterator twgts_itr = tmp_twgt_list.iterator();
            while ( twgts_itr.hasNext() )
                ( (CategoryWeight) twgts_itr.next() ).writeObject( outs );
        }
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
            twgt_list  = new ArrayList( Nentries );
            for ( ientry = 0; ientry < Nentries; ientry++ ) {
                twgt = new CategoryWeight( ins );
                twgt_list.add( twgt );
            }
        }
    }

    public String toString()
    {
        StringBuffer rep = new StringBuffer( super.toString() );
        rep.append( " Nrobjs=" + num_real_objs );

        Iterator        twgts_itr = null;
        if ( twgt_list != null )
            twgts_itr = twgt_list.iterator();
        else
            if ( type_twgt_map != null )
                twgts_itr = type_twgt_map.values().iterator();

        CategoryWeight  twgt;
        if ( twgts_itr != null ) {
            while ( twgts_itr.hasNext() ) {
                twgt = (CategoryWeight) twgts_itr.next();
                rep.append( "\n" + twgt + " " );
            }
        }
        return rep.toString();
    }

    /* Caller needs to be sure that the Drawable is a State */
    public void setStateRowAndNesting( CoordPixelXform  coord_xform,
                                       Map              map_line2row,
                                       NestingStacks    nesting_stacks )
    {
        Coord  start_vtx, final_vtx;
        start_vtx = this.getStartVertex();
        // final_vtx = this.getFinalVertex();

        int    rowID;
        float  nesting_ftr;
        rowID  = ( (Integer)
                   map_line2row.get( new Integer(start_vtx.lineID) )
                 ).intValue();
        super.setRowID( rowID );
        /*
        // if ( nesting_stacks.isReadyToGetNestingFactorFor( this ) ) {
        if ( super.isNestingFactorUninitialized() ) {
            nesting_ftr = nesting_stacks.getNestingFactorFor( this );
            super.setNestingFactor( nesting_ftr );
        }
        */
        nesting_ftr = nesting_stacks.getNestingFactorFor( this );
        super.setNestingFactor( nesting_ftr );
    }

    public int  drawOnCanvas( Graphics2D g, CoordPixelXform coord_xform,
                              Map map_line2row, DrawnBoxSet drawn_boxes )
    {
        Category type = super.getCategory();
        Topology topo = type.getTopology();
        if ( topo.isEvent() )
            System.err.println( "Non-supported yet Event Shadow type." );
        else if ( topo.isState() )
            return this.drawState( g, coord_xform, map_line2row,
                                   drawn_boxes, type.getColor() );
        else if ( topo.isArrow() )
            return this.drawArrow( g, coord_xform, map_line2row,
                                   drawn_boxes, type.getColor() );
        else
            System.err.println( "Non-recognized Shadow type! " + this );
        return 0;
    }

    public Drawable getDrawableWithPixel( CoordPixelXform coord_xform,
                                          Map map_line2row,
                                          Point pix_pt )
    {
        Category type = super.getCategory();
        Topology topo = type.getTopology();
        if ( topo.isEvent() )
            System.err.println( "Non-supported yet Event Shadow type." );
        else if ( topo.isState() ) {
            selected_subtype = this.isPixelInState( coord_xform,
                                                    map_line2row, pix_pt );
            if ( this.selected_subtype != null )
                return this;
        }
        else if ( topo.isArrow() ) {
            if ( this.isPixelOnArrow( coord_xform, map_line2row, pix_pt ) )
                return this;
        }
        else
            System.err.println( "Non-recognized Shadow type! " + this );
        return null;
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

    /* 
        0.0f < nesting_ftr <= 1.0f
    */
    private int  drawState( Graphics2D g, CoordPixelXform coord_xform,
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

        return PreviewState.draw( g, twgt_list, Empty_Border, coord_xform,
                                  drawn_boxes.getLastStatePos( rowID ),
                                  tStart, rStart, tFinal, rFinal );
        // return State.draw( g, color, Empty_Border, coord_xform,
        //                    drawn_boxes.getLastStatePos( rowID ),
        //                    tStart, rStart, tFinal, rFinal );
    }

    private int  drawArrow( Graphics2D g, CoordPixelXform coord_xform,
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
    private Category isPixelInState( CoordPixelXform coord_xform,
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

        return PreviewState.containsPixel( twgt_list, Empty_Border,
                                           coord_xform, pix_pt,
                                           tStart, rStart, tFinal, rFinal );
    }

    //  assume this Shadow overlaps with coord_xform.TimeBoundingBox
    private boolean isPixelOnArrow( CoordPixelXform coord_xform,
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
}
