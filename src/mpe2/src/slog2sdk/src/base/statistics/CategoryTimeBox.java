/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package base.statistics;

import java.awt.Color;
import java.util.Comparator;

import base.drawable.TimeBoundingBox;
import base.drawable.CategoryWeight;

public class CategoryTimeBox extends TimeBoundingBox
{
    public static final Comparator  INDEX_ORDER       = new IndexOrder();
    public static final Comparator  INCL_RATIO_ORDER  = new InclRatioOrder();
    public static final Comparator  EXCL_RATIO_ORDER  = new ExclRatioOrder();

    private CategoryWeight   twgt;
    
    public CategoryTimeBox()
    {
        super();
        twgt  = null;
    }

    public CategoryTimeBox( final CategoryWeight  in_twgt )
    {
        super();
        twgt  = in_twgt;
    }

    public float  getCategoryRatio( boolean isInclusive )
    {
        return twgt.getRatio( isInclusive );
    }

    public Color  getCategoryColor()
    {
        return twgt.getCategory().getColor();
    }

    public boolean  isCategoryVisiblySearchable()
    {
        return twgt.getCategory().isVisiblySearchable();
    }

    public CategoryWeight  getCategoryWeight()
    {
        return twgt;
    }


    private static class IndexOrder implements Comparator
    {
        public int compare( Object o1, Object o2 )
        {
            CategoryTimeBox typebox1 = (CategoryTimeBox) o1;
            CategoryTimeBox typebox2 = (CategoryTimeBox) o2;
            return CategoryWeight.INDEX_ORDER.compare( typebox1.twgt,
                                                       typebox2.twgt );
        }
    }

    private static class InclRatioOrder implements Comparator
    {
        public int compare( Object o1, Object o2 )
        {
            CategoryTimeBox typebox1 = (CategoryTimeBox) o1;
            CategoryTimeBox typebox2 = (CategoryTimeBox) o2;
            return CategoryWeight.INCL_RATIO_ORDER.compare( typebox1.twgt,
                                                            typebox2.twgt );
        }
    }

    private static class ExclRatioOrder implements Comparator
    {
        public int compare( Object o1, Object o2 )
        {
            CategoryTimeBox typebox1 = (CategoryTimeBox) o1;
            CategoryTimeBox typebox2 = (CategoryTimeBox) o2;
            return CategoryWeight.EXCL_RATIO_ORDER.compare( typebox1.twgt,
                                                            typebox2.twgt );
        }
    }
}
