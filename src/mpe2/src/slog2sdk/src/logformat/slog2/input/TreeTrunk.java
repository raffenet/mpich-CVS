/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package logformat.slog2.input;

import java.util.Iterator;

import base.drawable.TimeBoundingBox;
import base.drawable.Drawable;
import logformat.slog2.*;

/*
    The TreeTrunk consists all of the TreeNodes that are specified by
    a TimeBoundingBox and a lowest_depth_reached( read ).
*/
public class TreeTrunk extends TreeFloorList
{
    private        static boolean          isDebugging = false;
    private        static TimeBoundingBox  timeframe_root;
    private               short            depth_root;  // depth of treeroot
    private               short            depth_init;  // depth initialized
    private               short            iZoom_level;

    private InputLog          slog_ins;

    public TreeTrunk( InputLog  in_slog )
    {
        super();
        slog_ins       = in_slog;
        timeframe_root = null;
    }

    public void initFromTreeTop()
    {
        FileBlockPtr  blockptr;
        TreeNode      treeroot;

        blockptr       = slog_ins.getFileBlockPtrToTreeRoot();
        treeroot       = slog_ins.readTreeNode( blockptr );
        timeframe_root = new TimeBoundingBox( treeroot );
        depth_root     = treeroot.getTreeNodeID().depth;
        depth_init     = depth_root;
        iZoom_level    = 0;
        super.init( depth_root );
        super.put( treeroot, treeroot );
    }

    public TreeNode getTreeRoot()
    {
        return (TreeNode) super.getRoot();
    }

    public void growInTreeWindow( TreeNode treenode, short in_depth_init,
                                  final TimeBoundingBox  time_win )
    {
        depth_init = in_depth_init;
        this.growChildren( treenode, depth_init, time_win );
        super.updateLowestDepth();
    }

    /*
      growChildren() is NOT to be called by anyone
      except growInTreeWindow() or {contract,enlarge,scroll}TimeWindowTo()
      Assumption:  treenode.overlaps( time_win ) == true;
    */
    private void growChildren( final TreeNode treenode, short in_depth,
                               final TimeBoundingBox  time_win )
    {
        BufForObjects[]  childstubs;
        BufForObjects    childstub;
        TreeNode         childnode;
        FileBlockPtr     blockptr;
        int              idx;

        if (    treenode != null
             && treenode.getTreeNodeID().depth > in_depth ) {
            if ( treenode.overlaps( time_win ) ) {
                childstubs = treenode.getChildStubs();
                for ( idx = 0; idx < childstubs.length; idx++ ) {
                    childstub = childstubs[ idx ];
                    childnode = super.get( childstub );
                    if ( childstub.overlaps( time_win ) ) {
                        if ( childnode == null ) {
                            blockptr   = childstub.getFileBlockPtr();
                            childnode  = slog_ins.readTreeNode( blockptr );
                            super.put( childstub, childnode );
                        }
                        // Invoke growChildren() again to make sure all 
                        // childnode's decendants are in memory even if
                        // childnode is already in memory, because childnode's
                        // in-memory descendants may NOT overlap time_win.
                        this.growChildren( childnode, in_depth, time_win );
                    }
                    else { // childstub.disjoints( time_win )
                        if ( childnode != null ) {
                            this.removeChildren( childnode, in_depth );
                            debug_println( "TreeTrunk.growChildren(): "
                              + "remove(" + childstub.getTreeNodeID() + ")" );
                            super.remove( childstub );
                        }
                    }
                }
            }
            else {  // if ( treenode.disjoints( time_win ) )
                System.err.println( "TreeTrunk.growChildren(): ERROR!\n"
                                  + "\t treenode.overlaps( time_win ) "
                                  + "!= true\n"
                                  + "\t " + treenode.getTreeNodeID()
                                  + "does NOT overlap with " + time_win ); 
                System.exit( 1 );
            }
        }
    }

    /*
       removeChildren() does NOT remove input argument, treenode, so
       super.remove( childstub ) is needed after removeChildren( childnode )
    */
    private void removeChildren( final TreeNode treenode, short in_depth )
    {
        BufForObjects[]  childstubs;
        BufForObjects    childstub;
        TreeNode         childnode;
        int              idx;

        if (    treenode != null
             && treenode.getTreeNodeID().depth > in_depth ) {
            childstubs = treenode.getChildStubs();
            for ( idx = 0; idx < childstubs.length; idx++ ) {
                childstub = childstubs[ idx ];
                childnode = super.get( childstub );
                if ( childnode != null ) {
                    this.removeChildren( childnode, in_depth );
                    debug_println( "TreeTrunk.removeChildren(): "
                         + "remove(" + childstub.getTreeNodeID() + ")" );
                    super.remove( childstub );
                }
            }
        }
    }

    // Zoom In : 
    // The argument, time_win, is the new Time Window to be achieved
    public void contractTimeWindowTo( final TimeBoundingBox  time_win )
    {
        debug_println( "contractTimeWindowTo( " + time_win + " )" );
        TreeFloor  coverer, lowester;
        short      coverer_depth, lowester_depth, next_depth;
        TreeNode   treenode;

        iZoom_level    += 1;
        lowester        = super.getLowestFloor();
        // if ( ! lowester.covers( time_win ) ) {
            coverer         = super.pruneToBarelyCoveringFloor( time_win );
            coverer_depth   = coverer.getDepth();
            lowester_depth  = lowester.getDepth();
            next_depth      = (short) ( lowester_depth - 1 );
            if ( next_depth < 0 )
                next_depth = 0;
            debug_println( "coverer_depth = " + coverer_depth );
            debug_println( "lowester_depth = " + lowester_depth );
            debug_println( "iZoom_level = " + iZoom_level );
            debug_println( "next_depth = " + next_depth );
            if ( next_depth < coverer_depth ) {
                Iterator nodes = coverer.values().iterator(); 
                while ( nodes.hasNext() ) {
                    treenode = (TreeNode) nodes.next();
                    if ( treenode.overlaps( time_win ) )
                        this.growChildren( treenode, next_depth, time_win );
                    else {
                        this.removeChildren( treenode, next_depth );
                        debug_println( "TreeTrunk.contractTimeWindowTo(): "
                             + "remove(" + treenode.getTreeNodeID() + ")" );
                        nodes.remove();
                    }
                }
                super.removeAllChildFloorsBelow( next_depth );
            }
        // }
        // return super.getLowestDepth();
    }

    // Zoom Out :
    // The argument, time_win, is the new Time Window to be achieved
    public void enlargeTimeWindowTo( final TimeBoundingBox  time_win )
    {
        debug_println( "enlargeTimeWindowTo( " + time_win + " )" );
        TreeFloor  coverer, lowester;
        short      coverer_depth, lowester_depth, next_depth;
        TreeNode   treenode;

        iZoom_level    -= 1;
        lowester        = super.getLowestFloor();
        // if ( ! lowester.covers( time_win ) ) {
            coverer         = super.getCoveringFloor( time_win );
            coverer_depth   = coverer.getDepth();
            lowester_depth  = lowester.getDepth();
            next_depth      = 0;
            // guarantee enlargeTimeWindowTo() to be the 
            // reverse function of contractTimeWindowTo()
            if ( lowester_depth > 0 )
                next_depth  = (short) ( lowester_depth + 1 );
            else
                next_depth  = (short) ( depth_init - iZoom_level );
            if ( next_depth < 0 )
                next_depth = 0;
            if ( next_depth > depth_root )
                next_depth = depth_root;
            debug_println( "coverer_depth = " + coverer_depth );
            debug_println( "lowester_depth = " + lowester_depth );
            debug_println( "iZoom_level = " + iZoom_level );
            debug_println( "next_depth = " + next_depth );
            if ( next_depth < coverer_depth ) {
                Iterator nodes = coverer.values().iterator();
                while ( nodes.hasNext() ) {
                    treenode = (TreeNode) nodes.next();
                    if ( treenode.overlaps( time_win ) )
                        this.growChildren( treenode, next_depth, time_win );
                    else {
                        this.removeChildren( treenode, next_depth );
                        debug_println( "TreeTrunk.enlargeTimeWindowTo(): "
                             + "remove(" + treenode.getTreeNodeID() + ")" );
                        nodes.remove();
                    }
                }
                super.removeAllChildFloorsBelow( next_depth );
            }
        // }
        // return super.getLowestDepth();
    }

    // Scroll forward and backward
    // The argument, time_win, is the new Time Window to be achieved
    // Returns the lowest-depth of the tree.
    public void scrollTimeWindowTo( final TimeBoundingBox  time_win )
    {
        debug_println( "scrollTimeWindowTo( " + time_win + " )" );
        TreeFloor  coverer, lowester;
        short      coverer_depth, lowester_depth, next_depth;
        TreeNode   treenode;

        lowester        = super.getLowestFloor();
        if ( ! lowester.covers( time_win ) ) {
            coverer         = super.getCoveringFloor( time_win );
            coverer_depth   = coverer.getDepth();
            lowester_depth  = lowester.getDepth();
            next_depth      = lowester_depth;
            debug_println( "coverer_depth = " + coverer_depth );
            debug_println( "lowester_depth = " + lowester_depth );
            debug_println( "iZoom_level = " + iZoom_level );
            debug_println( "next_depth = " + next_depth );
            if ( next_depth < coverer_depth ) {
                Iterator nodes = coverer.values().iterator();
                while ( nodes.hasNext() ) {
                    treenode = (TreeNode) nodes.next();
                    if ( treenode.overlaps( time_win ) )
                        this.growChildren( treenode, next_depth, time_win );
                    else {
                        this.removeChildren( treenode, next_depth );
                        debug_println( "TreeTrunk.scrollTimeWindowTo(): "
                             + "remove(" + treenode.getTreeNodeID() + ")" );
                        nodes.remove();
                    }
                }
                // super.updateLowestDepth();
                super.removeAllChildFloorsBelow( next_depth );
            }
        }
        // return super.getLowestDepth();
    }

    // Float.MIN_VALUE is way to small, set TOLERANCE to 1%
    // private static final double TOLERANCE = 5 * Float.MIN_VALUE;
    private static final double TOLERANCE = 0.01f;

    //  This function returns the lowest-depth of the tree after the update
    //  if there is any error, -1 is returned.
    public boolean updateTimeWindow( final TimeBoundingBox  time_win_old,
                                     final TimeBoundingBox  time_win_new )
    {
        // Error Checking
        /*
        if ( ! super.getLowestFloor().covers( time_win_old ) ) {
            debug_println( "TreeTrunk.updateTimeWindow(): WARNING!\n"
                         + "\t LowestFloorTimeWin = "
                         + super.getLowestFloor().getTimeBounds()
                         + " does NOT cover TimeWin_old.\n"
                         + "\t TimeWin_old = " + time_win_old + ".\n"
                         + "\t TimeWin_new = " + time_win_new + ".\n" );
        }
        */

        double time_ratio;
        if ( timeframe_root.overlaps( time_win_new ) ) { 
            //  Determine if TimeWindow is scrolled, enlarged or contracted.
            if ( ! time_win_old.equals( time_win_new ) ) {
                time_ratio = time_win_new.getLength()
                           / time_win_old.getLength();
                if ( Math.abs( time_ratio - 1.0d ) <= TOLERANCE )
                    scrollTimeWindowTo( time_win_new );
                else
                    if ( time_ratio > 1.0d )
                        enlargeTimeWindowTo( time_win_new );
                    else
                        contractTimeWindowTo( time_win_new );
            }   
            return true;
        }
        else {  // if ( timeframe_root.disjoints( time_win_new ) )
            //  Don't update the TimeWindow, emit a warning message and return
            debug_println( "TreeTrunk.updateTimeWindow(): ERROR!\n"
                         + "\t TimeWindow disjoints from TimeFrame@TreeRoot.\n"
                         + "\t TimeWin@TreeRoot = " + timeframe_root + "\n"
                         + "\t TimeWin_old      = " + time_win_old   + "\n"
                         + "\t TimeWin_new      = " + time_win_new   + "\n" );
            return false;
        }
    }

    public void setDebuggingEnabled( boolean bvalue )
    {
        isDebugging = bvalue;
    }

    public boolean isDebugging()
    {
        return isDebugging;
    }

    private static void debug_println( String str )
    {
        if ( isDebugging )
            System.out.println( str );
    }
}
