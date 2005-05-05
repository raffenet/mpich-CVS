/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package base.drawable;

import java.util.Comparator;
import java.io.DataInput;
import java.io.DataOutput;

public class CategoryWeight extends CategoryRatios
{
    public  static final int         BYTESIZE          = CategoryRatios.BYTESIZE
                                                       + 8; // num_real_objs

    public  static final Comparator  COUNT_ORDER       = new CountOrder();

    private long       num_real_objs;

    public CategoryWeight()
    {
        super();
        num_real_objs  = 0;
    }

    // For SLOG-2 Output
    public CategoryWeight( final Category new_type,
                           float new_incl_r, float new_excl_r,
                           long new_num_real_objs )
    {
        super( new_type, new_incl_r, new_excl_r );
        num_real_objs  = new_num_real_objs;
    }

    // For SLOG-2 Output
    public CategoryWeight( final CategoryWeight type_wgt )
    {
        super( type_wgt );
        this.num_real_objs  = type_wgt.num_real_objs;
    }

    public long getDrawableCount()
    {
        return num_real_objs;
    }

    public void addDrawableCount( long new_num_real_objs )
    {
        this.num_real_objs  += new_num_real_objs;
    }

    public void writeObject( DataOutput outs )
    throws java.io.IOException
    {
        super.writeObject( outs );
        outs.writeLong( num_real_objs );
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
        super.readObject( ins );
        num_real_objs  = ins.readLong();
    }

    // For InfoPanelForDrawable
    public String toInfoBoxString( int print_status )
    {
        return super.toInfoBoxString( print_status )
             + ", count=" + num_real_objs;
    }

    public String toString()
    {
        return "(" + super.toString() + ", count=" + num_real_objs + ")";
    }



    private static class CountOrder implements Comparator
    {
        public int compare( Object o1, Object o2 )
        {
            CategoryWeight type_wgt1 = (CategoryWeight) o1;
            CategoryWeight type_wgt2 = (CategoryWeight) o2;
            long diff = type_wgt1.num_real_objs - type_wgt2.num_real_objs;
            return ( diff < 0 ? -1 : ( diff == 0 ? 0 : 1 ) );
        }
    }
}
