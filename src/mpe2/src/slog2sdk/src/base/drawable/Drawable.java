/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package base.drawable;

import java.awt.Graphics2D;
import java.awt.Point;
import java.util.Map;

public abstract class Drawable extends InfoBox
{
    // exclusion, nesting_ftr & row_ID are for Nestable Drawable, i.e. state.
    public  static final float    NON_NESTABLE      = 1.0f; 
    public  static final int      INVALID_ROW       = Integer.MIN_VALUE; 

    private              double   exclusion;     // For SLOG-2 Output API

    private              float    nesting_ftr;   // For SLOG-2 Input API
    private              int      row_ID;        // For SLOG-2 Input API

    // non-null parent => this Drawable is part of a Composite Drawable
    private              Drawable parent;        // For SLOG-2 Input API


    public Drawable()
    {
        super();
        exclusion    = 0.0d;
        nesting_ftr  = NON_NESTABLE;
        parent       = null;
    }

    public Drawable( final Category in_type )
    {
        super( in_type );
        exclusion    = 0.0d;
        nesting_ftr  = NON_NESTABLE;
        parent       = null;
    }

    //  This is NOT a copy constructor,
    //  only Category and InfoType[] are copied, not InfoValue[].
    public Drawable( final Drawable dobj )
    {
        super( dobj );  // InfoBox( InfoBox );
        exclusion    = 0.0d;
        nesting_ftr  = NON_NESTABLE;
        // parent       = null; 
        parent       = dobj.parent; 
    }

    public Drawable( Category in_type, final Drawable dobj )
    {
        super( in_type, dobj );
        exclusion    = 0.0d;
        nesting_ftr  = NON_NESTABLE;
        // parent       = null; 
        parent       = dobj.parent;
    }

    //  For support of Trace API's  Primitive/Composite generation
    public Drawable( int in_type_idx, byte[] byte_infovals )
    {
        super( in_type_idx );
        super.setInfoBuffer( byte_infovals );
        exclusion    = 0.0d;
        nesting_ftr  = NON_NESTABLE;
        parent       = null;
    }

    //  For SLOG-2 Output API
    public void initExclusion( Object[] childshades )
    {
        exclusion = super.getDuration();
        for ( int idx = childshades.length-1; idx >= 0; idx-- )
            exclusion -= super.getIntersectionDuration(
                               (TimeBoundingBox) childshades[ idx ] );
    }

    //  For SLOG-2 Output API
    public void decrementExclusion( double decre )
    {
        exclusion -= decre;
    }

    //  For SLOG-2 Output API
    public double getExclusion()
    {
        return exclusion;
    }

    //  For SLOG-2 Input API
    public boolean isNestingFactorUninitialized()
    {
        return nesting_ftr == NON_NESTABLE;
    }

    //  For SLOG-2 Input API
    public void  setNestingFactor( float new_nesting_ftr )
    {
        nesting_ftr = new_nesting_ftr;
    }

    //  For SLOG-2 Input API
    public float getNestingFactor()
    {
        return nesting_ftr;
    }

    //  For SLOG-2 Input API
    public boolean isRowIDUninitialized()
    {
        return row_ID == INVALID_ROW;
    }

    public void setRowID( int new_rowID )
    {
        row_ID = new_rowID;
    }

    public int getRowID()
    {
        return row_ID;
    }

    public void setParent( final Drawable dobj )
    {
        parent = dobj;
    }

    //  getParent() returns null == no parent
    public Drawable getParent()
    {
        return parent;
    }

    /*  getByteSize() cannot be declared ABSTRACT, 
        it would jeopardize the use of super.getByteSize( of InfoBox ) in
        subclass like Primitive/Composite.

    public abstract int       getByteSize();
    */
    // public abstract boolean   isTimeOrdered();

    public abstract int       getNumOfPrimitives();

    public abstract Integer[] getArrayOfLineIDs();

    public abstract Coord     getStartVertex();

    public abstract Coord     getFinalVertex();

    public abstract int       drawState( Graphics2D       g,
                                         CoordPixelXform  coord_xform,
                                         Map              map_line2row,
                                         DrawnBoxSet      drawn_boxes,
                                         ColorAlpha       color );

    public abstract int       drawArrow( Graphics2D       g,
                                         CoordPixelXform  coord_xform,
                                         Map              map_line2row,
                                         DrawnBoxSet      drawn_boxes,
                                         ColorAlpha       color );

    public abstract int       drawEvent( Graphics2D       g,
                                         CoordPixelXform  coord_xform,
                                         Map              map_line2row,
                                         DrawnBoxSet      drawn_boxes,
                                         ColorAlpha       color );

    public abstract boolean   isPixelInState( CoordPixelXform  coord_xform,
                                              Map              map_line2row,
                                              Point            pix_pt );

    public abstract boolean   isPixelOnArrow( CoordPixelXform  coord_xform,
                                              Map              map_line2row,
                                              Point            pix_pt );

    public abstract boolean   isPixelAtEvent( CoordPixelXform  coord_xform,
                                              Map              map_line2row,
                                              Point            pix_pt );

    public abstract boolean   containSearchable();



    /* Caller needs to be sure that the Drawable displayed is a State */
    public void setStateRowAndNesting( CoordPixelXform  coord_xform,
                                       Map              map_line2row,
                                       NestingStacks    nesting_stacks )
    {
        Coord  start_vtx, final_vtx;
        start_vtx = this.getStartVertex();
        // final_vtx = this.getFinalVertex();

        row_ID  = ( (Integer)
                    map_line2row.get( new Integer(start_vtx.lineID) )
                  ).intValue();
        nesting_ftr = nesting_stacks.getNestingFactorFor( this );
    }

    /* return number of primitives drawn */
    public int       drawOnCanvas( Graphics2D      g,
                                   CoordPixelXform coord_xform,
                                   Map             map_line2row,
                                   DrawnBoxSet     drawn_boxes )
    {
        Category type = super.getCategory();
        Topology topo = type.getTopology();
        if ( topo.isEvent() ) {
            return this.drawEvent( g, coord_xform, map_line2row,
                                   drawn_boxes, type.getColor() );
        }
        else if ( topo.isState() ) {
            return this.drawState( g, coord_xform, map_line2row,
                                   drawn_boxes, type.getColor() );
        }
        else if ( topo.isArrow() ) {
            return this.drawArrow( g, coord_xform, map_line2row,
                                   drawn_boxes, type.getColor() );
        }
        else
            System.err.println( "Non-recognized Primitive type! " + this );
        return 0;
    }

    public Drawable getDrawableAt( CoordPixelXform  coord_xform,
                                   Map              map_line2row,
                                   Point            pix_pt )
    {
        Category type = super.getCategory();
        Topology topo = type.getTopology();
        if ( topo.isEvent() ) {
            if ( this.isPixelAtEvent( coord_xform, map_line2row, pix_pt ) )
                return this;
        }
        else if ( topo.isState() ) {
            if ( this.isPixelInState( coord_xform, map_line2row, pix_pt ) )
                return this;
        }
        else if ( topo.isArrow() ) {
            if ( this.isPixelOnArrow( coord_xform, map_line2row, pix_pt ) )
                return this;
        }
        else
            System.err.println( "Non-recognized Primitive type! " + this );
        return null;
    }
}
