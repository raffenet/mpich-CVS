/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package base.drawable;

import java.util.Comparator;

/*
    This comparator to Collections.sort() will make Drawables be arranged
    in decreasing endtime order.  If endtimes are equals, Drawables will
    then be arranged in inceasing starttime order.
*/
public class DrawOrderComparator implements Comparator
{
    public int compare( Object o1, Object o2 )
    {
        Drawable  dobj1, dobj2;
        dobj1 = (Drawable) o1;
        dobj2 = (Drawable) o2;
        double  dobj1_endtime, dobj2_endtime;
        dobj1_endtime = dobj1.getLatestTime();
        dobj2_endtime = dobj2.getLatestTime();
        if ( dobj1_endtime == dobj2_endtime ) {
            double  dobj1_begtime, dobj2_begtime;
            dobj1_begtime = dobj1.getEarliestTime();
            dobj2_begtime = dobj2.getEarliestTime();
            if ( dobj1_begtime == dobj2_begtime )
                return 0;
            else
                // increasing starttime order
                return ( dobj1_begtime < dobj2_begtime ? -1 : 1 );
        }
        else
            // decreasing endtime order
            return ( dobj1_endtime > dobj2_endtime ? -1 : 1 );
    }
}
