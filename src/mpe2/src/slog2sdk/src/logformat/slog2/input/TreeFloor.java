/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package logformat.slog2.input;

import java.util.SortedMap;
import java.util.TreeMap;
import java.util.List;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.ListIterator;

import base.drawable.TimeBoundingBox;
import logformat.slog2.*;

/*
   TreeFloor is a SortedSet( or SortedMap ) of TreeNode
             whose TreeNodeID.depth are the same.
*/
public class TreeFloor extends TreeMap
{
    private short              depth;
    private TimeBoundingBox    timebounds;

    public TreeFloor( short in_depth )
    {
        super();
        depth       = in_depth;
        timebounds  = new TimeBoundingBox();
    }

    public short getDepth()
    {
        return depth;
    }

    public TimeBoundingBox firstTimeBounds()
    {
        return (TimeBoundingBox) super.firstKey();
    }

    public TimeBoundingBox lastTimeBounds()
    {
        return (TimeBoundingBox) super.lastKey();
    }

    public TimeBoundingBox getTimeBounds()
    {
        timebounds.setEarliestTime( firstTimeBounds().getEarliestTime() );
        timebounds.setLatestTime( lastTimeBounds().getLatestTime() );
        if ( ! timebounds.isTimeOrdered() ) {
            System.out.println( "slog2.input.getTimeBounds() returns wrong"
                              + timebounds );
        }
        return timebounds;
    }

    public boolean coversBarely( final TimeBoundingBox  tframe )
    {
        return    firstTimeBounds().contains( tframe.getEarliestTime() )
               && lastTimeBounds().contains( tframe.getLatestTime() );
    }

    public boolean covers( final TimeBoundingBox  tframe )
    {
        this.getTimeBounds();
        return timebounds.covers( tframe );
    }

    public boolean overlaps( final TimeBoundingBox  tframe )
    {
        this.getTimeBounds();
        return timebounds.overlaps( tframe );
    }

    public boolean disjoints( final TimeBoundingBox  tframe )
    {
        this.getTimeBounds();
        return timebounds.disjoints( tframe );
    }

    public void pruneToBarelyCovering( final TimeBoundingBox  tframe )
    {
        if ( this.covers( tframe ) ) {
            double starttime = tframe.getEarliestTime(); 
            double finaltime = tframe.getLatestTime();
            while ( ! this.coversBarely( tframe ) ) {
                if ( ! firstTimeBounds().contains( starttime ) )
                    super.remove( super.firstKey() );
                if ( ! lastTimeBounds().contains( finaltime ) )
                    super.remove( super.lastKey() );
            }
        }
    }

    public Iterator iteratorOfDrawables( final TimeBoundingBox  tframe,
                                         boolean isForeItr, boolean isNestable )
    {
        if ( isForeItr )
            return new ForeItrOfDrawables( tframe, isNestable );
        else
            return new BackItrOfDrawables( tframe, isNestable );
    }

    public Iterator iteratorOfShadows( final TimeBoundingBox  tframe,
                                       boolean isForeItr, boolean isNestable )
    {
        if ( isForeItr )
            return new ForeItrOfShadows( tframe, isNestable );
        else
            return new BackItrOfShadows( tframe, isNestable );
    }

    public String toStubString()
    {
        StringBuffer rep = new StringBuffer();
        Iterator itr = this.keySet().iterator();
        while ( itr.hasNext() ) {
            BufStub nodestub = new BufStub( (BufForObjects) itr.next() );
            rep.append( nodestub.toString() + "\n" );
        }
        return rep.toString();
    }

    public String toString()
    {
        StringBuffer rep = new StringBuffer();
        Iterator itr = this.values().iterator();
        while ( itr.hasNext() )
            rep.append( itr.next().toString() + "\n" );
        return rep.toString();
    }



    private class ForeItrOfDrawables extends IteratorOfGroupObjects
    {
        private static final boolean          INCRE_STARTTIME_ORDER = true;
        private              Iterator         nodes_itr;
        private              boolean          isNestable;

        public ForeItrOfDrawables( final TimeBoundingBox  tframe,
                                         boolean          in_isNestable )
        {
            super( tframe );
            isNestable  = in_isNestable;
            nodes_itr   = values().iterator();
            super.setObjGrpItr( this.nextObjGrpItr( tframe ) );
        }

        protected Iterator nextObjGrpItr( final TimeBoundingBox tframe )
        {
            TreeNode         node;

            // nodes_itr is guaranteed to be NOT null by TreeMap.values()
            // while ( nodes_itr != null )
                while ( nodes_itr.hasNext() ) {
                    node       = (TreeNode) nodes_itr.next();
                    if ( node.overlaps( tframe ) )
                        return node.iteratorOfDrawables( tframe,
                                                         INCRE_STARTTIME_ORDER,
                                                         isNestable );
                }
            // }
            // return NULL when no more node in nodes_itr
            return null;
        }
    }   // private class ForeItrOfDrawables

    private class BackItrOfDrawables extends IteratorOfGroupObjects
    {
        private static final boolean          DECRE_STARTTIME_ORDER = false;
        private              ListIterator     nodes_itr;
        private              boolean          isNestable;

        public BackItrOfDrawables( final TimeBoundingBox  tframe,
                                         boolean          in_isNestable )
        {
            super( tframe );
            isNestable  = in_isNestable;
            List nodes  = new ArrayList( values() );
            nodes_itr   = nodes.listIterator( nodes.size() );
            super.setObjGrpItr( this.nextObjGrpItr( tframe ) );
        }

        protected Iterator nextObjGrpItr( final TimeBoundingBox tframe )
        {
            TreeNode         node;

            // nodes_itr is guaranteed to be NOT null by TreeMap.values()
            // while ( nodes_itr != null )
                while ( nodes_itr.hasPrevious() ) {
                    node       = (TreeNode) nodes_itr.previous();
                    if ( node.overlaps( tframe ) )
                        return node.iteratorOfDrawables( tframe,
                                                         DECRE_STARTTIME_ORDER,
                                                         isNestable );
                }
            // }
            // return NULL when no more node in nodes_itr
            return null;
        }
    }   // private class BackItrOfDrawables


    private class ForeItrOfShadows extends IteratorOfGroupObjects
    {
        private static final boolean          INCRE_STARTTIME_ORDER = true;
        private              Iterator         nodes_itr;
        private              boolean          isNestable;

        public ForeItrOfShadows( final TimeBoundingBox  tframe,
                                       boolean          in_isNestable )
        {
            super( tframe );
            isNestable  = in_isNestable;
            nodes_itr   = values().iterator();
            super.setObjGrpItr( this.nextObjGrpItr( tframe ) );
        }

        protected Iterator nextObjGrpItr( final TimeBoundingBox tframe )
        {
            TreeNode         node;

            // nodes_itr is guaranteed to be NOT null by TreeMap.values()
            // while ( nodes_itr != null )
                while ( nodes_itr.hasNext() ) {
                    node       = (TreeNode) nodes_itr.next();
                    if ( node.overlaps( tframe ) )
                        return node.iteratorOfShadows( tframe,
                                                       INCRE_STARTTIME_ORDER,
                                                       isNestable );
                }
            // }
            // return NULL when no more node in nodes_itr
            return null;
        }
    }   // private class ForeItrOfShadows

    private class BackItrOfShadows extends IteratorOfGroupObjects
    {
        private static final boolean          DECRE_STARTTIME_ORDER = false;
        private              ListIterator     nodes_itr;
        private              boolean          isNestable;

        public BackItrOfShadows( final TimeBoundingBox  tframe,
                                        boolean         in_isNestable )
        {
            super( tframe );
            isNestable  = in_isNestable;
            List nodes  = new ArrayList( values() );
            nodes_itr   = nodes.listIterator( nodes.size() );
            super.setObjGrpItr( this.nextObjGrpItr( tframe ) );
        }

        protected Iterator nextObjGrpItr( final TimeBoundingBox tframe )
        {
            TreeNode         node;

            // nodes_itr is guaranteed to be NOT null by TreeMap.values()
            // while ( nodes_itr != null )
                while ( nodes_itr.hasPrevious() ) {
                    node       = (TreeNode) nodes_itr.previous();
                    if ( node.overlaps( tframe ) )
                        return node.iteratorOfShadows( tframe,
                                                       DECRE_STARTTIME_ORDER,
                                                       isNestable );
                }
            // }
            // return NULL when no more node in nodes_itr
            return null;
        }
    }   // private class BackItrOfShadows

}
