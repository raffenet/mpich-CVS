/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package base.drawable;

import java.util.Map;
import java.util.Comparator;
import java.io.DataInput;
import java.io.DataOutput;

public class CategoryWeight
{
    public  static final int         BYTESIZE          = 12;

    public  static final Comparator  INDEX_ORDER       = new IndexOrder();
    public  static final Comparator  INCL_RATIO_ORDER  = new InclRatioOrder();
    public  static final Comparator  EXCL_RATIO_ORDER  = new ExclRatioOrder();

    public  static final int         PRINT_ALL_RATIOS  = 0;
    public  static final int         PRINT_INCL_RATIO  = 1;
    public  static final int         PRINT_EXCL_RATIO  = 2;

    private static final String      TITLE_ALL_RATIOS
                                     = "*** All Duration Ratios:";
    private static final String      TITLE_INCL_RATIO
                                     = "*** Inclusive Duration Ratio:";
    private static final String      TITLE_EXCL_RATIO
                                     = "*** Exclusive Duration Ratio:";

    private static final int INVALID_INDEX = Integer.MIN_VALUE;

    private int        type_idx;
    private Category   type;
    private float      incl_ratio;
    private float      excl_ratio;

    private int        width;    // pixel width, for SLOG-2 Input & Jumpshot
    private int        height;   // pixel height, for SLOG-2 Input & Jumpshot

    public CategoryWeight()
    {
        type        = null;
        type_idx    = INVALID_INDEX;
        incl_ratio  = 0.0f;
        excl_ratio  = 0.0f;
        width       = 0;
        height      = 0;
    }

    // For SLOG-2 Output
    public CategoryWeight( final Category new_type,
                           float new_incl_r, float new_excl_r )
    {
        type        = new_type;
        type_idx    = type.getIndex();
        incl_ratio  = new_incl_r;
        excl_ratio  = new_excl_r;
    }

    // For SLOG-2 Output
    public CategoryWeight( final CategoryWeight type_wgt )
    {
        this.type        = type_wgt.type;
        this.type_idx    = type_wgt.type_idx;
        this.incl_ratio  = type_wgt.incl_ratio;
        this.excl_ratio  = type_wgt.excl_ratio;
    }

    public void setPixelWidth( int wdh )
    {
        width = wdh;
    }

    public int getPixelWidth()
    {
        return width;
    }

    public void setPixelHeight( int hgt )
    {
        height = hgt;
    }

    public int getPixelHeight()
    {
        return height;
    }

    public Category getCategory()
    {
        return type;
    }

    public float getRatio( boolean isInclusive )
    {
        if ( isInclusive )
            return incl_ratio;
        else
            return excl_ratio;
    }

    public void rescaleAllRatios( float ftr )
    {
        incl_ratio *= ftr;
        excl_ratio *= ftr;
    }

    public void addAllRatios( final CategoryWeight a_type_wgt, float ftr )
    {
        this.incl_ratio += a_type_wgt.incl_ratio * ftr;
        this.excl_ratio += a_type_wgt.excl_ratio * ftr;
    }

    public void addExclusiveRatio( float extra_ratio )
    {
        this.excl_ratio += extra_ratio;
    }

    // For Jumpshot-4
    public void addInclusiveRatio( float extra_ratio )
    {
        this.incl_ratio += extra_ratio;
    }

    //  For SLOG-2 Input API, used by Shadow.resolveCategory() 
    public boolean resolveCategory( final Map categorymap )
    {
        if ( type == null ) {
            if ( type_idx != INVALID_INDEX ) {
                type = (Category) categorymap.get( new Integer( type_idx ) );
                if ( type != null )
                    return true;
            }
        }
        return false;
    }

    public void writeObject( DataOutput outs )
    throws java.io.IOException
    {
        outs.writeInt( type_idx );
        outs.writeFloat( incl_ratio );
        outs.writeFloat( excl_ratio );
    }

    public CategoryWeight( DataInput ins )
    throws java.io.IOException
    {
        super();
        this.readObject( ins );
    }

    public void readObject( DataInput ins )
    throws java.io.IOException
    {
        type_idx   = ins.readInt();
        incl_ratio = ins.readFloat();
        excl_ratio = ins.readFloat();
    }

    // For InfoPanelForDrawable
    public static String getPrintTitle( int print_status )
    {
        if ( print_status == PRINT_INCL_RATIO )
            return TITLE_INCL_RATIO;
        else if ( print_status == PRINT_EXCL_RATIO )
            return TITLE_EXCL_RATIO;
        else // if ( print_status == PRINT_ALL_RATIOS )
            return TITLE_ALL_RATIOS;
    }

    // For InfoPanelForDrawable
    public String toInfoBoxString( int print_status )
    {
        StringBuffer rep = new StringBuffer( "legend=" );
        if ( type != null )
            rep.append( type.getName() );
        else
            rep.append( "null:" + type_idx );
        
        if ( print_status == PRINT_INCL_RATIO )
            rep.append( ", ratio=" + incl_ratio );
        else if ( print_status == PRINT_EXCL_RATIO )
            rep.append( ", ratio=" + excl_ratio ); 
        else // if ( print_status == PRINT_ALL_RATIOS )
            rep.append( ", incl_ratio=" + incl_ratio
                      + ", excl_ratio=" + excl_ratio );
        return rep.toString();
    }

    public String toString()
    {
        if ( type != null )
            return "(type=" + type_idx + ":" + type.getName()
                 + ",wgt=" + incl_ratio + "," + excl_ratio + ")";
        else
            return "(type=" + type_idx
                 + ",wgt=" + incl_ratio + "," + excl_ratio + ")";
    }



    private static class IndexOrder implements Comparator
    {
        public int compare( Object o1, Object o2 )
        {
            CategoryWeight type_wgt1 = (CategoryWeight) o1;
            CategoryWeight type_wgt2 = (CategoryWeight) o2;
            return type_wgt1.type_idx - type_wgt2.type_idx;
        }
    }

    private static class InclRatioOrder implements Comparator
    {
        public int compare( Object o1, Object o2 )
        {
            CategoryWeight type_wgt1 = (CategoryWeight) o1;
            CategoryWeight type_wgt2 = (CategoryWeight) o2;
            float diff = type_wgt1.incl_ratio - type_wgt2.incl_ratio;
            return ( diff < 0.0f ? -1 : ( diff == 0.0f ? 0 : 1 ) );
        }
    }

    private static class ExclRatioOrder implements Comparator
    {
        public int compare( Object o1, Object o2 )
        {
            CategoryWeight type_wgt1 = (CategoryWeight) o1;
            CategoryWeight type_wgt2 = (CategoryWeight) o2;
            float diff = type_wgt1.excl_ratio - type_wgt2.excl_ratio;
            return ( diff < 0.0f ? -1 : ( diff == 0.0f ? 0 : 1 ) );
        }
    }
}
