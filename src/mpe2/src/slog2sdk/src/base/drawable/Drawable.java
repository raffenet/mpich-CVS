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
    /*
       Both nesting_ftr and row_ID are for State Drawable, i.e. nestable.
    */
    public  static final float   NON_NESTABLE      = 1.0f; 
    public  static final int     INVALID_ROW       = Integer.MIN_VALUE; 
    private              float   nesting_ftr;   // For SLOG-2 Input API
    private              int     row_ID;        // For SLOG-2 Input API

    public Drawable()
    {
        super();
        nesting_ftr  = NON_NESTABLE;
    }

    public Drawable( final Category in_type )
    {
        super( in_type );
        nesting_ftr  = NON_NESTABLE;
    }

    //  This is NOT a copy constructor,
    //  only Category and InfoType[] are copied, not InfoValue[].
    public Drawable( final Drawable dobj )
    {
        super( dobj );  // InfoBox( InfoBox );
        nesting_ftr  = NON_NESTABLE;
    }

    public Drawable( Category in_type, final Drawable dobj )
    {
        super( in_type, dobj );
        nesting_ftr  = NON_NESTABLE;
    }

    //  For support of Trace API's  Primitive/Composite generation
    public Drawable( int in_type_idx, byte[] byte_infovals )
    {
        super( in_type_idx );
        super.setInfoBuffer( byte_infovals );
        nesting_ftr  = NON_NESTABLE;
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

    /*  getByteSize() cannot be declared ABSTRACT, 
        it would jeopardize the use of super.getByteSize( of InfoBox ) in
        subclass like Primitive/Composite.

    public abstract int       getByteSize();
    */
    // public abstract boolean   isTimeOrdered();

    public abstract int       getNumOfPrimitives();

    public abstract Integer[] getArrayOfLineIDs();

    /* Caller needs to be sure that the Drawable is a State */
    public abstract void setStateRowAndNesting( CoordPixelXform coord_xform,
                                                Map             map_line2row,
                                                NestingStacks   nest_stacks );

    /* return number of primitives drawn */
    public abstract int       drawOnCanvas( Graphics2D      g,
                                            CoordPixelXform coord_xform,
                                            Map             map_line2row,
                                            DrawnBoxSet     drawn_boxes );

    public abstract Drawable  getDrawableWithPixel( CoordPixelXform coord_xform,
                                                    Map    map_line2row,
                                                    Point  pixel_point );

    public abstract boolean   containSearchable();
}
