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
    in increasing starttime order.  If starttimes are equals, Drawables will
    then be arranged in decreasing finaltime order.

    Since the comparator is used in the TreeMap of class
    logformat/slog2/input/TreeFloorList.ForeItrOfDrawables
    where the likihood of equal starttime is high.  In order to avoid
    over-written of iterators due to false equivalent drawables,
    i.e. using starttime and endtime may not be enough.  We implement
    a very strict form of comparator for drawable for drawing order.
*/
public class DrawOrderComparator implements Comparator
{
    public int compare( Object o1, Object o2 )
    {
        Drawable  dobj1, dobj2;
        dobj1 = (Drawable) o1;
        dobj2 = (Drawable) o2;
        double  dobj1_starttime, dobj2_starttime;
        dobj1_starttime = dobj1.getEarliestTime();
        dobj2_starttime = dobj2.getEarliestTime();
        if ( dobj1_starttime != dobj2_starttime )
            // increasing starttime order ( 1st order )
            return ( dobj1_starttime < dobj2_starttime ? -1 : 1 );
        else { 
            double  dobj1_finaltime, dobj2_finaltime;
            dobj1_finaltime = dobj1.getLatestTime();
            dobj2_finaltime = dobj2.getLatestTime();
            if ( dobj1_finaltime != dobj2_finaltime )
                // decreasing finaltime order ( 2nd order )
                return ( dobj1_finaltime > dobj2_finaltime ? -1 : 1 );
            else {
                if ( dobj1 == dobj2 )
                    return 0;
                else {
                    int dobj1_type, dobj2_type;
                    dobj1_type = dobj1.getCategoryIndex();
                    dobj2_type = dobj2.getCategoryIndex();
                    if ( dobj1_type != dobj2_type )
                        // arbitary order
                        return dobj1_type - dobj2_type;
                    else {
                        int        dobj1_vtx, dobj2_vtx;
                        Primitive  prime;

                        if ( dobj1 instanceof Composite )
                            prime = ((Composite) dobj1).getStartPrimitive();
                        else
                            prime = (Primitive) dobj1;
                        dobj1_vtx = prime.getStartVertex().lineID;

                        if ( dobj2 instanceof Composite )
                            prime = ((Composite) dobj2).getStartPrimitive();
                        else
                            prime = (Primitive) dobj2;
                        dobj2_vtx = prime.getStartVertex().lineID;

                        if ( dobj1_vtx != dobj2_vtx )
                            // arbitary order
                            return dobj1_vtx - dobj2_vtx;
                        else {
                            if ( dobj1 instanceof Composite )
                                prime = ((Composite) dobj1).getFinalPrimitive();
                            else
                                prime = (Primitive) dobj1;
                            dobj1_vtx = prime.getFinalVertex().lineID;

                            if ( dobj2 instanceof Composite )
                                prime = ((Composite) dobj2).getFinalPrimitive();
                            else
                                prime = (Primitive) dobj2;
                            dobj2_vtx = prime.getFinalVertex().lineID;

                            if ( dobj1_vtx != dobj2_vtx )
                                // arbitary order
                                return dobj1_vtx - dobj2_vtx;
                            else {
                                System.err.println( "DrawOrderComparator: "
                                          + "WARNING! Equal Drawables?\n"
                                          + dobj1 + "\n" + dobj2 );
                                return 0;
                            }   // FinalVertex's lineID
                        }   // StartVertex's lineID
                    }   // CategoryIndex
                }   // direct dobjX comparison
            }   // FinalTime
        }   // StartTime
    }
}
