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
    private              SearchCriteria       criteria;
    private              Drawable             last_found_dobj;

    public SearchTreeTrunk( TreeTrunk  treebody, final YaxisTree y_tree )
    {
        treetrunk        = treebody;
        criteria         = new SearchCriteria( y_tree );
        last_found_dobj  = null;

        Infinite_TimeBox = new TimeBoundingBox();
        Infinite_TimeBox.setEarliestTime( Double.NEGATIVE_INFINITY );
        Infinite_TimeBox.setLatestTime( Double.POSITIVE_INFINITY );
    }

    // This is for a backward NEW SEARCH
    public Drawable previousDrawable( double searching_time )
    {
        Iterator  dobjs;
        Drawable  dobj;
        /*
           Use a infinite TimeBoundingBox so iteratorOfAllDrawables() returns
           all drawables in the memory disregarding the treefloor's timebounds
        */
        dobjs = treetrunk.iteratorOfAllDrawables( Infinite_TimeBox,
                                                  !INCRE_STARTTIME_ORDER,
                                                  true );
        criteria.initMatch();
        while ( dobjs.hasNext() ) {
            dobj = (Drawable) dobjs.next();
            if (    dobj.getCategory().isVisible()
                 && dobj.getEarliestTime() <= searching_time
                 && criteria.isMatched( dobj ) ) { 
                last_found_dobj = dobj;
                return last_found_dobj;
            }
        }
        last_found_dobj = null;
        return null;
    }

    // This is for a backward CONTINUING SEARCH
    public Drawable previousDrawable()
    {
        Iterator  dobjs;
        Drawable  dobj;

        if ( last_found_dobj == null ) {
            System.err.println( "SearchTreeTrunk.previousDrawable(): "
                              + "Unexpected error, last_found_dobj == null" );
            return null;
        }
        /*
           Use a infinite TimeBoundingBox so iteratorOfAllDrawables() returns
           all drawables in the memory disregarding the treefloor's timebounds
        */
        dobjs = treetrunk.iteratorOfAllDrawables( Infinite_TimeBox,
                                                  !INCRE_STARTTIME_ORDER,
                                                  true );
        criteria.initMatch();
        while ( dobjs.hasNext() ) {
            dobj = (Drawable) dobjs.next();
            if (    dobj.getCategory().isVisible()
                 && DRAWING_ORDER.compare( dobj, last_found_dobj ) < 0
                 && criteria.isMatched( dobj ) ) {
                last_found_dobj = dobj;
                return last_found_dobj;
            }
        }
        last_found_dobj = null;
        return null;
    }

    // This is for a forward NEW SEARCH
    public Drawable nextDrawable( double searching_time )
    {
        Iterator  dobjs;
        Drawable  dobj;
        /*
           Use a infinite TimeBoundingBox so iteratorOfAllDrawables() returns
           all drawables in the memory disregarding the treefloor's timebounds
        */
        dobjs = treetrunk.iteratorOfAllDrawables( Infinite_TimeBox,
                                                  INCRE_STARTTIME_ORDER,
                                                  true );
        criteria.initMatch();
        while ( dobjs.hasNext() ) {
            dobj = (Drawable) dobjs.next();
            if (    dobj.getCategory().isVisible()
                 && dobj.getEarliestTime() >= searching_time
                 && criteria.isMatched( dobj ) ) {
                last_found_dobj = dobj;
                return last_found_dobj;
            }
        }
        last_found_dobj = null;
        return null;
    }

    // This is for a forward CONTINUING SEARCH
    public Drawable nextDrawable()
    {
        Iterator  dobjs;
        Drawable  dobj;

        if ( last_found_dobj == null ) {
            System.err.println( "SearchTreeTrunk.nextDrawable(): "
                              + "Unexpected error, last_found_dobj == null" );
            return null;
        }
        /*
           Use a infinite TimeBoundingBox so iteratorOfAllDrawables() returns
           all drawables in the memory disregarding the treefloor's timebounds
        */
        dobjs = treetrunk.iteratorOfAllDrawables( Infinite_TimeBox,
                                                  INCRE_STARTTIME_ORDER,
                                                  true );
        criteria.initMatch();
        while ( dobjs.hasNext() ) {
            dobj = (Drawable) dobjs.next();
            if (    dobj.getCategory().isVisible()
                 && DRAWING_ORDER.compare( dobj, last_found_dobj ) > 0
                 && criteria.isMatched( dobj ) ) {
                last_found_dobj = dobj;
                return last_found_dobj;
            }
        }
        last_found_dobj = null;
        return null;
    }
}
