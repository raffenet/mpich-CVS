/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package logformat.slog2;

import java.io.DataOutput;
import java.io.DataInput;

public class TreeNodeID implements Comparable
{
    public static final int BYTESIZE = 2  /* depth */
                                     + 4  /* xpos  */ ;

    public short depth;
    public int   xpos;

    public TreeNodeID( short in_depth, int in_xpos )
    {
        depth  = in_depth;
        xpos   = in_xpos;
    }

    public TreeNodeID( final TreeNodeID ID )
    {
        depth  = ID.depth;
        xpos   = ID.xpos;
    }

    public TreeNodeID getParentNodeID( int num_leafs )
    {
        return new TreeNodeID( (short)( depth+1 ), xpos/num_leafs );
    }


    /*
       Use of toParent(), toNextSibling() or toPreviousSibling() avoids
       creation of new TreeNodeID.  Interfaces like getParent(), 
       getNextSibling() and getPreviousSibling() may involve creation
       of new instance of TreeNodeID.
    */
    public void toParent( int num_leafs )
    {
        depth++;
        xpos /= num_leafs ;
    }

    public void toNextSibling()
    {
        xpos++;
    }

    public void toPreviousSibling()
    {
        xpos--;
    }

    public boolean isPossibleRoot()
    {
        return ( xpos == 0 );
    }

    public boolean isLeaf()
    {
        return ( depth == 0 );
    }

    public boolean equals( final TreeNodeID nodeID )
    {
        return ( this.depth == nodeID.depth && this.xpos == nodeID.xpos );
    }

    /*
       Define the "natural ordering" imposed by Comparable used in SortedMap
       The ordering here needs to be consistent with the primitive time order
       (i.e. increasing or decreasing not the starttime or finaltime part)
       defined in Drawable.DRAWING_ORDER (TimeBoundingBox.INCRE_STARTTIME_ORDER)
       which is first determined by increasing startime then
       decreasing endtime order.
    */
    public int compareTo( final TreeNodeID ID )
    {
        if ( this.depth == ID.depth )
            if ( this.xpos == ID.xpos )
                return 0;
            else
                return ( this.xpos < ID.xpos ? -1 : 1 ); // increasing starttime
             // return ( this.xpos > ID.xpos ? -1 : 1 ); // decreasing endtime
        else
            return ( this.depth > ID.depth ? -1 : 1 );
    }

    // If obj.getClass() != TreeNodeID.class, throws ClassCastException
    public int compareTo( Object obj )
    {
        return this.compareTo( (TreeNodeID) obj );
    }

    public void writeObject( DataOutput outs )
    throws java.io.IOException
    {
        outs.writeShort( depth );
        outs.writeInt( xpos );
    }

    public TreeNodeID( DataInput ins )
    throws java.io.IOException
    {
        this.readObject( ins );
    }

    public void readObject( DataInput ins )
    throws java.io.IOException
    {
        depth  = ins.readShort();
        xpos   = ins.readInt();
    }

    public String toString()
    {
        return( "ID(" + depth + "," + xpos + ")" );
    }

    public final static void main( String[] args )
    {
        final short ll_max = 4;
              short dd;
              short ll;
              int   xp, xp_max;

        java.util.Map map = new java.util.TreeMap();
        for ( ll = ll_max; ll >=0 ; ll-- ) {
            dd = (short) ( ll_max - ll );
            xp_max = (int) Math.pow( (double) 2, (double) ll );
            for ( xp = xp_max-1; xp >= 0; xp-- )
                map.put( new TreeNodeID( dd, xp ),
                         new String( "_" + ll + ", " + xp + "_" ) );
        }

        TreeNodeID ID;
        java.util.Iterator itr = map.entrySet().iterator();
        while ( itr.hasNext() ) {
            ID = (TreeNodeID) ( (java.util.Map.Entry) itr.next() ).getKey();
            if ( ID.isPossibleRoot() ) 
                System.out.println( "\n" + ID );
            else
                System.out.println( ID );
        }
    }
}
