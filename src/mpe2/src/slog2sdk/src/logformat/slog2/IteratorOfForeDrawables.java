/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package logformat.slog2;

import java.util.List;
import java.util.Iterator;
import java.util.ListIterator;

import base.drawable.TimeBoundingBox;
import base.drawable.Drawable;

/*
   Iterator of Drawables in a given List in Increasing Endtime order.
   The drawable returned by next() overlaps with the timeframe specified.
 */
public class IteratorOfForeDrawables implements Iterator
{
    private ListIterator     drawables_itr;
    private TimeBoundingBox  timeframe;
    private Drawable         next_drawable;

    public IteratorOfForeDrawables(       List             dobjs_list,
                                    final TimeBoundingBox  tframe )
    {
        drawables_itr  = dobjs_list.listIterator( dobjs_list.size() );
        timeframe      = tframe;
        next_drawable  = null;
    }

    public boolean hasNext()
    {
        while ( drawables_itr.hasPrevious() ) {
            next_drawable = (Drawable) drawables_itr.previous();
            if ( next_drawable.overlaps( timeframe ) )
                return true;
        }
        return false;
    }

    public Object next()
    {
        return next_drawable;
    }

    public void remove() {}
}   // private class IteratorOfForeDrawables
