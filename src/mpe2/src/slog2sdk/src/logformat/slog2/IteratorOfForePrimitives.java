/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package logformat.slog2;

import java.util.List;
import java.util.TreeSet;
import java.util.Iterator;
import java.util.ListIterator;

import base.drawable.TimeBoundingBox;
import base.drawable.Drawable;
import base.drawable.Composite;
import base.drawable.Primitive;
import base.drawable.DrawOrderComparator;

/*
   Iterator of Primitives in a given List in Increasing StartTime order.
   The primitive returned by next() overlaps with the timeframe specified.
 */
public class IteratorOfForePrimitives implements Iterator
{
    // Drawing Order for all drawables (especially State) is defined to be
    // first Increasing Starttime and then Decreasing EndTime.
    private static final DrawOrderComparator    DRAWING_ORDER
                                                = new DrawOrderComparator();

    private ListIterator     drawables_itr;
    private TimeBoundingBox  timeframe;

    private TreeSet          set_primes;
    private Primitive        next_primitive;

    public IteratorOfForePrimitives(       List             dobjs_list,
                                     final TimeBoundingBox  tframe )
    {
        drawables_itr  = dobjs_list.listIterator( 0 );
        timeframe      = tframe;
        set_primes     = new TreeSet( DRAWING_ORDER );
    }

    public boolean hasNext()
    {
        Drawable   itr_dobj;
        Composite  itr_cmplx;

        next_primitive   = null;
        while ( drawables_itr.hasNext() ) {
            itr_dobj = (Drawable) drawables_itr.next();
            if ( itr_dobj.overlaps( timeframe ) ) {
                if ( itr_dobj instanceof Composite ) {
                    itr_cmplx  = (Composite) itr_dobj;
                    itr_cmplx.addPrimitivesToSet( set_primes, timeframe );
                    next_primitive = (Primitive) set_primes.first();
                    set_primes.remove( next_primitive );
                }
                else { // if ( itr_dobj instanceof Primitive )
                    if ( ! set_primes.isEmpty() ) {
                        set_primes.add( itr_dobj );
                        next_primitive = (Primitive) set_primes.first();
                        set_primes.remove( next_primitive );
                    }
                    else
                        next_primitive = (Primitive) itr_dobj;
                }
                break;
            }
        }

        if ( next_primitive == null && !set_primes.isEmpty() ) {
            next_primitive = (Primitive) set_primes.first();
            set_primes.remove( next_primitive );
        }

        return next_primitive != null;
    }

    public Object next()
    {
        return next_primitive;
    }

    public void remove() {}
}   // private class IteratorOfForePrimitives;
