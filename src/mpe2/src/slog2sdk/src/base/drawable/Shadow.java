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
import java.awt.Color;
import java.awt.Point;
import java.util.Map;

import base.io.MixedDataInput;
import base.io.MixedDataOutput;
import base.topology.Line;
import base.topology.Arrow;
import base.topology.State;

public class Shadow extends Primitive
{
    private static final int     BYTESIZE = TimeBoundingBox.BYTESIZE /*super*/
                                          + 8  /* num_real_objs */ ;

    private static final Stroke  STROKE = new BasicStroke( 3.0f );

    private        long      num_real_objs;

    public Shadow()
    {
        super();
        num_real_objs  = 0;
    }

    public Shadow( final Shadow shade )
    {
        super( shade );
        num_real_objs  = shade.num_real_objs;
    }

    public Shadow( Category shadow_type, final Primitive prime )
    {
        super( shadow_type, prime );
        num_real_objs  = 1;
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

        // do a Time Average over the total number of real drawables
        for ( int idx = 0; idx < shade_vtxs.length; idx++ )
            shade_vtxs[ idx ].time = aveOverAllObjs( shade_vtxs[ idx ].time,
                                                     this.num_real_objs,
                                                     sobj_vtxs[ idx ].time,
                                                     sobj.num_real_objs );

        super.affectTimeBounds( sobj );

        // Need to figure out how to do error estimation.
        // Maybe <X^2> is needed to compute the standard dev..
        // time_err = ( super.getLatestTime() - super.getEarliestTime() ) / 2.0;

        num_real_objs += sobj.num_real_objs;

        // System.err.println( "Shadow.mergeWithShadow(): END" );
    }

    private static double aveOverAllObjs( double sobj_time, long sobj_Nobjs,
                                          double dobj_time, long dobj_Nobjs )
    {
        return ( ( sobj_time * sobj_Nobjs + dobj_time * dobj_Nobjs )
               / ( sobj_Nobjs + dobj_Nobjs ) );
    }

    public int getByteSize()
    {
        return super.getByteSize() + BYTESIZE;
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
    }

    public String toString()
    {
        StringBuffer rep = new StringBuffer( super.toString() );
        rep.append( " Nrobjs=" + num_real_objs );
        return rep.toString();
    }

    public void drawOnCanvas( Graphics2D g, CoordPixelXform coord_xform,
                              Map map_line2row, NestingStacks nesting_stacks )
    {
        Category type = super.getCategory();
        Topology topo = type.getTopology();
        if ( topo.isEvent() )
            System.err.println( "Non-supported yet Event Shadow type." );
        else if ( topo.isState() )
            this.drawState( g, coord_xform, map_line2row, type.getColor() );
        else if ( topo.isArrow() )
            this.drawArrow( g, coord_xform, map_line2row, type.getColor() );
        else
            System.err.println( "Non-recognized Shadow type! " + this );
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
            if ( this.isPixelInState( coord_xform, map_line2row, pix_pt ) )
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

    /* 
        0.0f < nesting_ftr <= 1.0f
    */
    private void drawState( Graphics2D g, CoordPixelXform coord_xform,
                            Map map_line2row, ColorAlpha color )
    {
        Coord  start_vtx, final_vtx;
        start_vtx = this.getStartVertex();
        final_vtx = this.getFinalVertex();

        double tStart, tFinal;
        tStart = super.getEarliestTime();    /* different from Primitive */
        tFinal = super.getLatestTime();      /* different from Primitive */

        int       rowID;
        float     nesting_ftr;
        rowID  = ( (Integer)
                   map_line2row.get( new Integer(start_vtx.lineID) )
                 ).intValue();
        nesting_ftr = NestingStacks.getShadowNestingHeight();

        float  rStart, rFinal;
        rStart   = (float) rowID - nesting_ftr / 2.0f;
        rFinal   = rStart + nesting_ftr;

        State.draw( g, color, null, coord_xform,
                    tStart, rStart, tFinal, rFinal );
    }

    private void drawArrow( Graphics2D g, CoordPixelXform coord_xform,
                            Map map_line2row, ColorAlpha color )
    {
        Coord  start_vtx, final_vtx;
        start_vtx = this.getStartVertex();
        final_vtx = this.getFinalVertex();

        double tStart, tFinal;
        tStart = super.getEarliestTime();    /* different from Primitive */
        tFinal = super.getLatestTime();      /* different from Primitive */

        float  rStart, rFinal;
        rStart   = ( (Integer)
                     map_line2row.get( new Integer(start_vtx.lineID) )
                   ).floatValue();
        rFinal   = ( (Integer)
                     map_line2row.get( new Integer(final_vtx.lineID) )
                   ).floatValue();

        Line.draw( g, color, STROKE, coord_xform,
                   tStart, rStart, tFinal, rFinal );
    }

    /* 
        0.0f < nesting_ftr <= 1.0f
    */
    private boolean isPixelInState( CoordPixelXform coord_xform,
                                    Map map_line2row, Point pix_pt )
    {
        Coord  start_vtx, final_vtx;
        start_vtx = this.getStartVertex();
        final_vtx = this.getFinalVertex();

        double tStart, tFinal;
        tStart = super.getEarliestTime();    /* different from Primitive */
        tFinal = super.getLatestTime();      /* different from Primitive */

        int       rowID;
        float     nesting_ftr;
        rowID  = ( (Integer)
                   map_line2row.get( new Integer(start_vtx.lineID) )
                 ).intValue();
        // nesting_ftr = Primitive.REDUCED_HEIGHT_FACTOR;
        nesting_ftr = 0.9f;

        // System.out.println( "\t" + this + " nestftr=" + nesting_ftr );

        float  rStart, rFinal;
        rStart   = (float) rowID - nesting_ftr / 2.0f;
        rFinal   = rStart + nesting_ftr;

        return State.containsPixel( coord_xform, pix_pt,
                                    tStart, rStart, tFinal, rFinal );
    }

    //  assume this Shadow overlaps with coord_xform.TimeBoundingBox
    private boolean isPixelOnArrow( CoordPixelXform coord_xform,
                                    Map map_line2row, Point pix_pt )
    {
        Coord  start_vtx, final_vtx;
        start_vtx = this.getStartVertex();
        final_vtx = this.getFinalVertex();

        double   tStart, tFinal;
        tStart = super.getEarliestTime();    /* different from Primitive */
        tFinal = super.getLatestTime();      /* different from Primitive */

        float    rStart, rFinal;
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
