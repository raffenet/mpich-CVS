/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package logformat.slog2.input;

import java.util.Iterator;
import java.util.TreeMap;
import java.util.NoSuchElementException;

import base.drawable.TimeBoundingBox;
import base.drawable.Drawable;
import logformat.slog2.*;
/*
    The leaf TreeNodes are always at depth = 0, or at TreeFloor[ idx = 0 ].
*/
public class TreeFloorList
{
    private static final short               NULL_DEPTH = -1;

    // FINALTIME_COMPARATOR are for iteratorOfDrawables, i.e. Output API
    private static final CmptrOfFinalTime    FINALTIME_COMPARATOR
                                             = new CmptrOfFinalTime();

    private TreeFloor[]      floors;
    private short            lowest_depth;

    public TreeFloorList()
    {
        floors = null;
        lowest_depth = NULL_DEPTH;
    }

    protected void init( short depth_max )
    {
        floors = new TreeFloor[ depth_max + 1 ];
        for ( int idx = floors.length-1; idx >= 0; idx-- )
            floors[ idx ] = new TreeFloor( (short) idx );
        lowest_depth = depth_max;
    }

    public BufForObjects getRoot()
    {
        return (BufForObjects) floors[ floors.length-1 ].firstKey();
    }

    /*
        add() guarantees that BufForObjects of same depth are added to
    */
    public void put( final BufForObjects nodestub, final BufForObjects node )
    {
        short  node_depth = nodestub.getTreeNodeID().depth;
        floors[ node_depth ].put( nodestub, node );
        if ( node_depth < lowest_depth )
            lowest_depth = node_depth;
    }

    public TreeNode get( final BufForObjects nodestub )
    {
        short  node_depth = nodestub.getTreeNodeID().depth;
        if ( floors[ node_depth ].size() == 0 )
            return null;
        else
            return (TreeNode) floors[ node_depth ].get( nodestub );
    }

    public boolean contains( final BufForObjects nodestub )
    {
        short  node_depth = nodestub.getTreeNodeID().depth;
        if ( floors[ node_depth ].size() == 0 )
            return false;
        else
            return floors[ node_depth ].containsKey( nodestub );
    }

    // Invoking remove() may require calling updateLowestDepth(), recommended.
    public void remove( final BufForObjects nodestub )
    {
        short  node_depth = nodestub.getTreeNodeID().depth;
        if ( floors[ node_depth ].size() > 0 ) {
            floors[ node_depth ].remove( nodestub );
        }
    }

    // updateLowestDepth() is needed after a series of remove()'s
    public void updateLowestDepth()
    {
        for ( short idx = 0; idx < floors.length; idx++ )
            if ( floors[ idx ].size() > 0 ) {
                lowest_depth = idx;
                break;
            }
    }

    /*  For informational service purpose only */
    public short getLowestDepth()
    {
        return lowest_depth;
    }

    /*
       removeAllChildFloorsBelow() guarantees lowest_depth be set correctly,
       no need to call updateLowestDepth().
    */
    public void removeAllChildFloorsBelow( short floor_depth )
    {
        for ( short idepth = 0; idepth < floor_depth; idepth++ ) {
            if ( floors[ idepth ].size() > 0 )
                floors[ idepth ].clear();
        }
        lowest_depth = floor_depth;
    }

    public TreeFloor getLowestFloor()
    {
        return floors[ lowest_depth ];
    }

    /*
       For Zoom-out/Scrolling, i.e. enlargement/moving of the Time View.
       Assumption: | tframe( time_frame ) | >= | floors[ lowest_depth ] |
       Caller is required to check for the assumption before invoking the fn.
       Returned the lowest TreeFloor __totally__ covers tframe
       If not found, return the top TreeFloor
    */
    public TreeFloor getCoveringFloor( final TimeBoundingBox tframe )
    {
        for ( int idepth = lowest_depth; idepth < floors.length; idepth++ ) {
            if ( floors[ idepth ].covers( tframe ) )
                return floors[ idepth ];
        }
        return floors[ floors.length - 1 ];
    }

    /*
       For Zoom-In, i.e. contraction of Time View
       Assumption: | tframe( time_frame ) | <= | floors[ lowest_depth ] |
       Caller is required to check for the assumption before invoking the fn.
       Returned the lowest TreeFloor __barely__ covers tframe
       If not found, return the top TreeFloor
    */
    public TreeFloor pruneToBarelyCoveringFloor( final TimeBoundingBox tframe )
    {
        for ( int idepth = lowest_depth; idepth < floors.length; idepth++ ) {
            if ( floors[ idepth ].coversBarely( tframe ) )
                return floors[ idepth ];
            else
                floors[ idepth ].pruneToBarelyCovering( tframe );
        }
        return floors[ floors.length - 1 ];
    }

    public Iterator iteratorOfDrawables( final TimeBoundingBox  tframe,
                                               boolean          isBackItr,
                                               boolean          isNestable )
    {
        if ( isBackItr )
            return new BackItrOfDrawables( tframe, isNestable );
        else
            return new ForeItrOfDrawables( tframe, isNestable );
    }

    public Iterator iteratorOfLowestFloorShadows( final TimeBoundingBox tframe,
                                                        boolean  isBackItr,
                                                        boolean  isNestable )
    {
        return floors[ lowest_depth ].iteratorOfShadows( tframe,
                                                         isBackItr,
                                                         isNestable );
    }

    public String toStubString()
    {
        StringBuffer rep = new StringBuffer();
        for ( int idx = floors.length-1; idx >= 0; idx-- )
             rep.append( floors[ idx ].toStubString() + "\n" );
        return rep.toString();
    }

    public String toString()
    {
        StringBuffer rep = new StringBuffer();
        for ( int idx = floors.length-1; idx >= 0; idx-- )
             rep.append( floors[ idx ].toString() + "\n" );
        return rep.toString();
    }

    public String toString( final TimeBoundingBox tframe )
    {
        StringBuffer rep = new StringBuffer();
        Iterator dobjs, sobjs;
        int idx;
        dobjs = this.iteratorOfDrawables( tframe, true, true );
        for ( idx = 1; dobjs.hasNext(); idx++ )
            rep.append( idx + ", " + dobjs.next() + "\n" );
        dobjs = this.iteratorOfDrawables( tframe, true, false );
        for ( ; dobjs.hasNext(); idx++ )
            rep.append( idx + ", " + dobjs.next() + "\n" );
        sobjs = this.iteratorOfLowestFloorShadows( tframe, true, true );
        for ( idx = 1; sobjs.hasNext(); idx++ )
            rep.append( idx + ", " + sobjs.next() + "\n" );
        sobjs = this.iteratorOfLowestFloorShadows( tframe, true, false );
        for ( ; sobjs.hasNext(); idx++ )
            rep.append( idx + ", " + sobjs.next() + "\n" );
        return rep.toString();
    }



    /*
        BackItrOfDrawables returns Drawables in Decreasing Endtime Order
     */
    private class BackItrOfDrawables implements Iterator
    {
        /*
           map_obj2itr is a backstore for Iterator of each floor.
           The key of map_obj2itr is the leading drawable out of the Iterator.
         */
        private TreeMap      map_obj2itr;
        private Drawable     this_floor_obj;
        private Iterator     this_floor_itr;
        private Drawable     next_floor_obj;
        private double       next_floor_finaltime;

        public BackItrOfDrawables( final TimeBoundingBox  tframe,
                                         boolean          isNestable )
        {
		    // map_obj2itr has Drawables arranged in Increasing Endtime order
            map_obj2itr = new TreeMap( FINALTIME_COMPARATOR );
            for ( int idx = floors.length-1; idx >= lowest_depth; idx-- ) {
                this_floor_itr = floors[ idx ].iteratorOfDrawables( tframe, 
                                                         true, isNestable );
                if ( this_floor_itr.hasNext() ) {
                    this_floor_obj = (Drawable) this_floor_itr.next();
                    map_obj2itr.put( this_floor_obj, this_floor_itr );
                }
            }

            try {
                this_floor_obj = (Drawable) map_obj2itr.lastKey();
                this_floor_itr = (Iterator)
                                 map_obj2itr.remove( this_floor_obj );
            } catch ( NoSuchElementException err ) {
                // when map_obj2itr is empty, next_floor_obj is null
                this_floor_obj = null;
                this_floor_itr = null;
                next_floor_obj = null;
                next_floor_finaltime = Double.NEGATIVE_INFINITY;
            }

            // Initialize next_floor_finaltime
            try {
                next_floor_obj = (Drawable) map_obj2itr.lastKey();
                next_floor_finaltime = next_floor_obj.getLatestTime();
            } catch ( NoSuchElementException err ) {
                // when map_obj2itr is empty, next_floor_obj is null
                next_floor_obj = null;
                next_floor_finaltime = Double.NEGATIVE_INFINITY;
            }
        }

        public boolean hasNext()
        {
            return this_floor_obj != null;
        }

        public Object next()
        {
            Drawable     next_drawable;
            Iterator     next_floor_itr;

            next_drawable  = this_floor_obj;
            this_floor_obj = null;

            try {
                if ( this_floor_itr.hasNext() ) {
                    this_floor_obj = (Drawable) this_floor_itr.next();
                    if (   this_floor_obj.getLatestTime()
                         < next_floor_finaltime ) {
                       /*
                          pull next_floor_itr out from map_obj2itr
                          deposit this_floor_itr back to map_obj2itr
                          update next_floor_finaltime from map_obj2itr
                        */
                        next_floor_itr = (Iterator)
                                         map_obj2itr.remove( next_floor_obj );
                        map_obj2itr.put( this_floor_obj, this_floor_itr );
                        this_floor_obj = next_floor_obj;
                        this_floor_itr = next_floor_itr;
                        next_floor_obj = (Drawable) map_obj2itr.lastKey();
                        next_floor_finaltime = next_floor_obj.getLatestTime();
                    }
                }
                else {
                    /*
                      update this_floor_obj from next_floor_obj
                      update next_floor_finaltime from map_obj2itr
                     */
                    this_floor_obj = next_floor_obj;
                    if ( this_floor_obj != null ) {
                        this_floor_itr = (Iterator)
                                         map_obj2itr.remove( this_floor_obj );
                        next_floor_obj = (Drawable) map_obj2itr.lastKey();
                        next_floor_finaltime = next_floor_obj.getLatestTime();
                    }
                }
            } catch ( NoSuchElementException err ) {
                // when map_obj2itr is empty, next_floor_obj is null
                next_floor_obj     = null;
                next_floor_finaltime = Double.NEGATIVE_INFINITY;
            }

            return next_drawable;
        }

        public void remove() {}
    }   // private class BackItrOfDrawables


    /*
        ForeItrOfDrawables returns Drawables in Increasing Endtime Order
     */
    private class ForeItrOfDrawables implements Iterator
    {
        /*
           map_obj2itr is a backstore for Iterator of each floor.
           The key of map_obj2itr is the leading drawable out of the Iterator.
         */
        private TreeMap      map_obj2itr;
        private Drawable     this_floor_obj;
        private Iterator     this_floor_itr;
        private Drawable     next_floor_obj;
        private double       next_floor_finaltime;

        public ForeItrOfDrawables( final TimeBoundingBox  tframe,
                                         boolean          isNestable )
        {
            // map_obj2itr has Drawables arranged in Increasing Endtime order
            map_obj2itr = new TreeMap( FINALTIME_COMPARATOR );
            for ( int idx = floors.length-1; idx >= lowest_depth; idx-- ) {
                this_floor_itr = floors[ idx ].iteratorOfDrawables( tframe,
                                                         false, isNestable );
                if ( this_floor_itr.hasNext() ) {
                    this_floor_obj = (Drawable) this_floor_itr.next();
                    map_obj2itr.put( this_floor_obj, this_floor_itr );
                }
            }

            try {
                this_floor_obj = (Drawable) map_obj2itr.firstKey();
                this_floor_itr = (Iterator)
                                 map_obj2itr.remove( this_floor_obj );
            } catch ( NoSuchElementException err ) {
                // when map_obj2itr is empty, next_floor_obj is null
                this_floor_obj = null;
                this_floor_itr = null;
                next_floor_obj = null;
                next_floor_finaltime = Double.POSITIVE_INFINITY;
            }

            // Initialize next_floor_finaltime
            try {
                next_floor_obj = (Drawable) map_obj2itr.firstKey();
                next_floor_finaltime = next_floor_obj.getLatestTime();
            } catch ( NoSuchElementException err ) {
                // when map_obj2itr is empty, next_floor_obj is null
                next_floor_obj = null;
                next_floor_finaltime = Double.POSITIVE_INFINITY;
            }
        }

        public boolean hasNext()
        {
            return this_floor_obj != null;
        }

        public Object next()
        {
            Drawable     next_drawable;
            Iterator     next_floor_itr;

            next_drawable  = this_floor_obj;
            this_floor_obj = null;

            try {
                if ( this_floor_itr.hasNext() ) {
                    this_floor_obj = (Drawable) this_floor_itr.next();
                    if (   this_floor_obj.getLatestTime()
                         > next_floor_finaltime ) {
                       /*
                          pull next_floor_itr out from map_obj2itr
                          deposit this_floor_itr back to map_obj2itr
                          update next_floor_finaltime from map_obj2itr
                        */
                        next_floor_itr = (Iterator)
                                         map_obj2itr.remove( next_floor_obj );
                        map_obj2itr.put( this_floor_obj, this_floor_itr );
                        this_floor_obj = next_floor_obj;
                        this_floor_itr = next_floor_itr;
                        next_floor_obj = (Drawable) map_obj2itr.firstKey();
                        next_floor_finaltime = next_floor_obj.getLatestTime();
                    }
                }
                else {
                    /*
                      update this_floor_obj from next_floor_obj
                      update next_floor_finaltime from map_obj2itr
                     */
                    this_floor_obj = next_floor_obj;
                    if ( this_floor_obj != null ) {
                        this_floor_itr = (Iterator)
                                         map_obj2itr.remove( this_floor_obj );
                        next_floor_obj = (Drawable) map_obj2itr.firstKey();
                        next_floor_finaltime = next_floor_obj.getLatestTime();
                    }
                }
            } catch ( NoSuchElementException err ) {
                // when map_obj2itr is empty, next_floor_obj is null
                next_floor_obj     = null;
                next_floor_finaltime = Double.POSITIVE_INFINITY;
            }

            return next_drawable;
        }

        public void remove() {}
    }   // private class ForeItrOfDrawables
}
