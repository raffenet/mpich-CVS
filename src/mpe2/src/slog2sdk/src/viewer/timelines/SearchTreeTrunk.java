/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package viewer.timelines;

import java.util.Iterator;

import base.drawable.DrawOrderComparator;
import base.drawable.TimeBoundingBox;
import base.drawable.Drawable;
import logformat.slog2.input.TreeTrunk;

public class SearchTreeTrunk
{
    private static final boolean              INCRE_STARTTIME_ORDER = true;
    private static final DrawOrderComparator  DRAWING_ORDER
                                              = new DrawOrderComparator();

    private static       TimeBoundingBox      Infinite_TimeBox;

    private              TreeTrunk            treetrunk;
    private              ModelTime            time_model;
    private              Drawable             last_found_dobj;

    public SearchTreeTrunk( TreeTrunk  treebody,  ModelTime  a_time_model )
    {
        treetrunk        = treebody;
        time_model       = a_time_model;
        last_found_dobj  = null;

        Infinite_TimeBox = new TimeBoundingBox();
        Infinite_TimeBox.setEarliestTime( Double.NEGATIVE_INFINITY );
        Infinite_TimeBox.setLatestTime( Double.POSITIVE_INFINITY );
    }

    public Drawable previousDrawable( boolean isNewSearch )
    {
        Iterator  dobjs;
        Drawable  dobj;
        double    focus_time;

        if ( isNewSearch )
            last_found_dobj = null;
        /*
           Use a infinite TimeBoundingBox so iteratorOfAllDrawables() returns
           all drawables in the memory disregarding the treefloor's timebounds
        */
        dobjs = treetrunk.iteratorOfAllDrawables( Infinite_TimeBox,
                                                  !INCRE_STARTTIME_ORDER,
                                                  true );
        if ( last_found_dobj != null ) {
            while ( dobjs.hasNext() ) {
                dobj = (Drawable) dobjs.next();
                if ( DRAWING_ORDER.compare( dobj, last_found_dobj ) < 0 ) {
                    last_found_dobj = dobj;
                    return dobj;
                }
            }
        }
        else {
            focus_time = time_model.getTimeZoomFocus();
            while ( dobjs.hasNext() ) {
                dobj = (Drawable) dobjs.next();
                if ( dobj.getEarliestTime() <= focus_time ) { 
                    last_found_dobj = dobj;
                    return dobj;
                }
            }
        }
        last_found_dobj = null;
        return null;
    }

    public Drawable nextDrawable( boolean isNewSearch )
    {
        Iterator  dobjs;
        Drawable  dobj;
        double    focus_time;

        if ( isNewSearch )
            last_found_dobj = null;
        /*
           Use a infinite TimeBoundingBox so iteratorOfAllDrawables() returns
           all drawables in the memory disregarding the treefloor's timebounds
        */
        dobjs = treetrunk.iteratorOfAllDrawables( Infinite_TimeBox,
                                                  INCRE_STARTTIME_ORDER,
                                                  true );
        if ( last_found_dobj != null ) {
            while ( dobjs.hasNext() ) {
                dobj = (Drawable) dobjs.next();
                if ( DRAWING_ORDER.compare( dobj, last_found_dobj ) > 0 ) {
                    last_found_dobj = dobj;
                    return dobj;
                }
            }
        }
        else {
            focus_time = time_model.getTimeZoomFocus();
            while ( dobjs.hasNext() ) {
                dobj = (Drawable) dobjs.next();
                if ( dobj.getEarliestTime() >= focus_time ) {
                    last_found_dobj = dobj;
                    return dobj;
                }
            }
        }
        last_found_dobj = null;
        return null;
    }
}
