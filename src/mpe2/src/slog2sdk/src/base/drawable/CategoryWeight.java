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
    public static final Comparator  WEIGHT_ORDER = new WeightOrder();

    private static final int INVALID_INDEX = Integer.MIN_VALUE;

    private int        type_idx;
    private Category   type;
    private float      weight;

    public CategoryWeight()
    {
        type     = null;
        type_idx = INVALID_INDEX;
        weight   = 0.0f;
    }

    // For SLOG-2 Output
    public CategoryWeight( final Category new_type, float new_weight )
    {
        type     = new_type;
        type_idx = type.getIndex();
        weight   = new_weight;
    }

    // For SLOG-2 Output
    public CategoryWeight( final CategoryWeight type_wgt )
    {
        this.type     = type_wgt.type;
        this.type_idx = type_wgt.type_idx;
        this.weight   = type_wgt.weight;
    }

    public Category getCategory()
    {
        return type;
    }

    public float getWeight()
    {
        return weight;
    }

    public void rescaleWeight( float ftr )
    {
        weight *= ftr;
    }

    public void addWeight( final CategoryWeight a_type_wgt, float ftr )
    {
        this.weight += a_type_wgt.weight * ftr;
    }

    //  For SLOG-2 input API
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
        outs.writeFloat( weight );
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
        weight   = ins.readFloat();
    }

    // For InfoPanelForDrawable
    public String toInfoBoxString()
    {
        if ( type != null )
            return "legend=" + type.getName() + ", fraction=" + weight;
        else
            return "legend=" + type_idx + ":null" + ", fraction=" + weight;
    }

    public String toString()
    {
        if ( type != null )
            return "(type=" + type_idx + ":" + type.getName()
                 + ",wgt=" + weight + ")";
        else
            return "(type=" + type_idx
                 + ",wgt=" + weight + ")";
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

    public static class WeightOrder implements Comparator
    {
        public int compare( Object o1, Object o2 )
        {
            CategoryWeight type_wgt1 = (CategoryWeight) o1;
            CategoryWeight type_wgt2 = (CategoryWeight) o2;
            float diff = type_wgt1.weight - type_wgt2.weight;
            return ( diff < 0.0f ? -1 : ( diff == 0.0f ? 0 : 1 ) );
        }
    }
}
