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
    public static final int         BYTESIZE     = 8;

    public static final Comparator  INDEX_ORDER  = new IndexOrder();
    public static final Comparator  RATIO_ORDER  = new RatioOrder();

    private static final int INVALID_INDEX = Integer.MIN_VALUE;

    private int        type_idx;
    private Category   type;
    private float      ratio;
    private float      exclusion;

    private int        width;    // pixel width, for SLOG-2 Input & Jumpshot
    private int        height;   // pixel height, for SLOG-2 Input & Jumpshot

    public CategoryWeight()
    {
        type      = null;
        type_idx  = INVALID_INDEX;
        ratio     = 0.0f;
        exclusion = 0.0f;
        width     = 0;
        height    = 0;
    }

    // For SLOG-2 Output
    public CategoryWeight( final Category new_type, float new_ratio )
    {
        type      = new_type;
        type_idx  = type.getIndex();
        ratio     = new_ratio;
    }

    // For SLOG-2 Output
    public CategoryWeight( final CategoryWeight type_wgt )
    {
        this.type     = type_wgt.type;
        this.type_idx = type_wgt.type_idx;
        this.ratio    = type_wgt.ratio;
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

    public float getRatio()
    {
        return ratio;
    }

    public void rescaleRatio( float ftr )
    {
        ratio *= ftr;
    }

    public void addRatio( final CategoryWeight a_type_wgt, float ftr )
    {
        this.ratio += a_type_wgt.ratio * ftr;
    }

    // For Jumpshot-4
    public void addRatio( float extra_ratio )
    {
        this.ratio += extra_ratio;
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
        outs.writeFloat( ratio );
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
        type_idx = ins.readInt();
        ratio    = ins.readFloat();
    }

    // For InfoPanelForDrawable
    public String toInfoBoxString()
    {
        if ( type != null )
            return "legend=" + type.getName() + ", fraction=" + ratio;
        else
            return "legend=" + type_idx + ":null" + ", fraction=" + ratio;
    }

    public String toString()
    {
        if ( type != null )
            return "(type=" + type_idx + ":" + type.getName()
                 + ",wgt=" + ratio + ")";
        else
            return "(type=" + type_idx
                 + ",wgt=" + ratio + ")";
    }



    public static class IndexOrder implements Comparator
    {
        public int compare( Object o1, Object o2 )
        {
            CategoryWeight type_wgt1 = (CategoryWeight) o1;
            CategoryWeight type_wgt2 = (CategoryWeight) o2;
            return type_wgt1.type_idx - type_wgt2.type_idx;
        }
    }

    public static class RatioOrder implements Comparator
    {
        public int compare( Object o1, Object o2 )
        {
            CategoryWeight type_wgt1 = (CategoryWeight) o1;
            CategoryWeight type_wgt2 = (CategoryWeight) o2;
            float diff = type_wgt1.ratio - type_wgt2.ratio;
            return ( diff < 0.0f ? -1 : ( diff == 0.0f ? 0 : 1 ) );
        }
    }
}
