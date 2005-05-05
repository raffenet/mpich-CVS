/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package base.statistics;

import java.util.Comparator;
import java.io.DataInput;
import java.io.DataOutput;

import base.drawable.Category;
import base.drawable.CategoryRatios;
import base.drawable.CategoryWeight;

public class CategoryWeightF extends CategoryRatios
{
    public  static final int         BYTESIZE          = CategoryRatios.BYTESIZE
                                                       + 8; // num_real_objs

    public  static final Comparator  COUNT_ORDER       = new CountOrder();

    private double     num_real_objs;

    public CategoryWeightF()
    {
        super();
        num_real_objs  = 0.0;
    }

    // For Jumpshot
    public CategoryWeightF( final Category new_type,
                            float new_incl_r, float new_excl_r,
                            double new_num_real_objs )
    {
        super( new_type, new_incl_r, new_excl_r );
        num_real_objs  = new_num_real_objs;
    }

    // For Jumpshot
    public CategoryWeightF( final CategoryWeightF type_wgf )
    {
        super( type_wgf );
        this.num_real_objs  = type_wgf.num_real_objs;
    }

    // For Jumpshot, copy construct,  CategoryWeight -> CategoryWeightF.
    public CategoryWeightF( final CategoryWeight type_wgt )
    {
        super( type_wgt );
        this.num_real_objs  = (double) type_wgt.getDrawableCount();
    }

    public double getDrawableCount()
    {
        return num_real_objs;
    }

    public void addDrawableCount( double new_num_real_objs )
    {
        this.num_real_objs  += new_num_real_objs;
    }

    // For Jumpshot's TimeAveBox, i.e. statistics window
    public void rescaleDrawableCount( double ftr )
    {
        this.num_real_objs  *= ftr;
    }

    public void writeObject( DataOutput outs )
    throws java.io.IOException
    {
        super.writeObject( outs );
        outs.writeDouble( num_real_objs );
    }

    public CategoryWeightF( DataInput ins )
    throws java.io.IOException
    {
        super();
        this.readObject( ins );
    }

    public void readObject( DataInput ins )
    throws java.io.IOException
    {
        super.readObject( ins );
        num_real_objs  = ins.readDouble();
    }

    // For InfoPanelForDrawable
    public String toInfoBoxString( int print_status )
    {
        return super.toInfoBoxString( print_status )
             + ", count=" + (float) num_real_objs;
    }

    public String toString()
    {
        return "(" + super.toString()
             + ", count=" + (float) num_real_objs + ")";
    }



    private static class CountOrder implements Comparator
    {
        public int compare( Object o1, Object o2 )
        {
            CategoryWeightF type_wgf1 = (CategoryWeightF) o1;
            CategoryWeightF type_wgf2 = (CategoryWeightF) o2;
            double diff = type_wgf1.num_real_objs - type_wgf2.num_real_objs;
            return ( diff < 0.0 ? -1 : ( diff == 0.0 ? 0 : 1 ) );
        }
    }
}
