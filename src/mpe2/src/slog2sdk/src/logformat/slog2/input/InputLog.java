/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package logformat.slog2.input;

import java.util.Map;
import java.util.List;
import java.util.Iterator;
import java.util.ArrayList;
import java.io.*;

import base.io.MixedRandomAccessFile;
import base.io.MixedDataInputStream;
import base.io.MixedDataIO;
import base.drawable.TimeBoundingBox;
import base.drawable.Drawable;
import logformat.slog2.*;

public class InputLog
{
    public  static final int      ITERATE_ALL       = 0;
    public  static final int      ITERATE_ARROWS    = 1;
    public  static final int      ITERATE_STATES    = 2;

    private MixedRandomAccessFile  rand_file;
    private ByteArrayInputStream   bary_ins;
    private MixedDataInputStream   data_ins;

    private Header                 filehdr;
    private TreeDir                treedir;
    private CategoryMap            objdefs;
    private LineIDMapList          lineIDmaps;

    private byte[]                 buffer;
    private String                 full_pathname;


    public InputLog( String filename )
    {
        full_pathname = filename;
        rand_file     = null;
        try {
            rand_file = new MixedRandomAccessFile( full_pathname, "r" );
        // } catch ( FileNotFoundException ferr ) {
        } catch ( IOException ferr ) {
            System.err.println( "InputLog: Non-recoverable IOException! "
                              + "Exiting ..." );
            ferr.printStackTrace();
            System.exit( 1 );
        }

        buffer    = null;
        bary_ins  = null;
        data_ins  = null;
    }

    // Used by Jumpshot4 to add extra titles to Timeline/Legend windows
    public String getPathnameSuffix()
    {
        String file_sep = System.getProperty( "file.separator" );
        int start_idx = full_pathname.lastIndexOf( file_sep );
        if ( start_idx > 0 )
            return full_pathname.substring( start_idx + 1 );
        else
            return full_pathname;
    }

    public String getLineIDMapName( int view_ID )
    {
        if ( lineIDmaps != null )
            if ( view_ID >= 0 && view_ID < lineIDmaps.size() ) {
                LineIDMap lineIDmap = (LineIDMap) lineIDmaps.get( view_ID );
                return lineIDmap.getTitle();
            }
            else
                return null;
        else
            return null;
    }

    /*
       isSLOG2() has to be called before getCompatibleHeader() or initialize()
    */
    public boolean isSLOG2()
    {
        try {
            rand_file.seek( 0 );
            filehdr   = new Header( rand_file );
        } catch ( IOException ioerr ) {
            System.err.println( "InputLog: Non-recoverable IOException! "
                              + "Exiting ..." );
            ioerr.printStackTrace();
            System.exit( 1 );
        }

        return filehdr != null && filehdr.isSLOG2();
    }

    public String getCompatibleHeader()
    {
        return filehdr.getCompatibleVersionMessage();
    }

    public static void stdoutConfirmation()
    {
        byte[] str_bytes = new byte[ 10 ];
        System.out.print( "Do you still want the program to continue ? "
                        + "y/yes to continue : " );
        try {
            System.in.read( str_bytes );
        } catch ( IOException ioerr ) {
            System.err.println( "InputLog: Non-recoverable IOException! "
                              + "Exiting ..." );
            ioerr.printStackTrace();
            System.exit( 1 );
        }
        String in_str = ( new String( str_bytes ) ).trim();
        if ( in_str.equals( "y" ) || in_str.equals( "yes" ) )
            System.out.println( "Program continues...." );
        else {
            System.out.println( "Program is terminating!..." );
            System.exit( 1 );
        }
    }

    public void initialize()
    {
        buffer    = new byte[ filehdr.getMaxBufferByteSize() ];

        readTreeDir();
        readCategoryMap();
        readLineIDMapList();
    }

    public FileBlockPtr getFileBlockPtrToTreeRoot()
    {
        return filehdr.blockptr2treeroot;
    }

    public short getNumChildrenPerNode()
    {
        return filehdr.getNumChildrenPerNode();
    }

    public short getMaxTreeDepth()
    {
        return filehdr.getMaxTreeDepth();
    }

/*
    private void readHeader()
    {
        try {
            rand_file.seek( 0 );
            filehdr   = new Header( rand_file );
        } catch ( IOException ioerr ) {
            System.err.println( "InputLog: Non-recoverable IOException! "
                              + "Exiting ..." );
            ioerr.printStackTrace();
            System.exit( 1 );
        }

        if ( ! filehdr.isVersionCompatible() ) {
            byte[] str_bytes = new byte[ 10 ];
            System.out.print( "Do you still want the program to continue ? "
                            + "y/yes to continue : " );
            try {
                System.in.read( str_bytes );
            } catch ( IOException ioerr ) {
                System.err.println( "InputLog: Non-recoverable IOException! "
                                  + "Exiting ..." );
                ioerr.printStackTrace();
                System.exit( 1 );
            }
            String in_str = ( new String( str_bytes ) ).trim();
            if ( in_str.equals( "y" ) || in_str.equals( "yes" ) )
                System.out.println( "Program continues...." );
            else {
                System.out.println( "Program is terminating!..." );
                System.exit( 1 );
            }
        }
    }
*/

    /*
       The returned String of readFilePart() is the error message.
       If readFilePart() returns without error, the returned value is null
    */
    private String readFilePart( final FileBlockPtr blockptr,
                                 final String       filepartname,
                                       MixedDataIO  filepart )
    {
        String err_str;
        if ( blockptr.isNULL() ) {
            err_str = "The file block pointer to the " + filepartname + " "
                    + "is NOT initialized!, can't read it.";
            return err_str;
        }
        if ( blockptr.getBlockSize() > filehdr.getMaxBufferByteSize() ) {
            err_str = "Oops! Unexpected Error: "
                    + "The block size of the " + filepartname + " is "
                    + "too big to read into buffer for processing.";
            return err_str;
        }

        long blk_fptr = blockptr.getFilePointer();
        int  blk_size = blockptr.getBlockSize();
        try {
            rand_file.seek( blk_fptr );
            rand_file.readFully( buffer, 0, blk_size );
            bary_ins  = new ByteArrayInputStream( buffer, 0, blk_size );
            data_ins  = new MixedDataInputStream( bary_ins );
            filepart.readObject( data_ins );
            data_ins.close();
        } catch ( IOException ioerr ) {
            System.err.println( "InputLog: Non-recoverable IOException! "
                              + "Exiting ..." );
            ioerr.printStackTrace();
            System.exit( 1 );
        }

        return null;
    }

    private void readLineIDMapList()
    {
        String err_str;
        lineIDmaps = new LineIDMapList();
        err_str    = readFilePart( filehdr.blockptr2lineIDmaps,
                                   "LineIDMapList", lineIDmaps );
        if ( err_str != null ) {
            System.err.println( err_str );
            System.exit( 1 );
        }
    }

    public LineIDMapList getLineIDMapList()
    {
        return lineIDmaps;
    }

    private void readTreeDir()
    {
        String err_str;
        treedir    = new TreeDir();
        err_str    = readFilePart( filehdr.blockptr2treedir,
                                   "Tree Directory", treedir );
        if ( err_str != null ) {
            System.err.println( err_str );
            System.exit( 1 );
        }
    }

    public TreeDir getTreeDir()
    {
        return treedir;
    }

    private void readCategoryMap()
    {
        String err_str;
        objdefs    = new CategoryMap();
        err_str    = readFilePart( filehdr.blockptr2categories,
                                   "CategoryMap", objdefs );
        if ( err_str != null ) {
            System.err.println( err_str );
            System.exit( 1 );
        }
    }

    public CategoryMap getCategoryMap()
    {
        return objdefs;
    }

/*
    public TreeNode readTreeNode( final FileBlockPtr blockptr )
    {
        String    err_str;
        TreeNode  treenode;
        treenode  = new TreeNode();
        err_str   = readFilePart( blockptr, "TreeNode", treenode );
        if ( err_str != null ) {
            System.err.println( err_str );
            System.exit( 1 );
        }
        return treenode;
    }
*/

    public TreeNode readTreeNode( final FileBlockPtr blockptr )
    {
        // Checks for Error!
        if ( blockptr.isNULL() ) {
            System.err.println( "The file block pointer to the TreeNode "
                              + "is NOT initialized!, can't read it." );
            return null;
        }
        if ( blockptr.getBlockSize() > filehdr.getMaxBufferByteSize() ) {
            System.err.println( "Oops! Unexpected Error: "
                              + "The block size of the TreeNode is "
                              + "too big to read into buffer for processing." );            return null;
        }

        TreeNode     treenode;

        long blk_fptr = blockptr.getFilePointer();
        int  blk_size = blockptr.getBlockSize();
        try {
            rand_file.seek( blk_fptr );
            rand_file.readFully( buffer, 0, blk_size );
            bary_ins  = new ByteArrayInputStream( buffer, 0, blk_size );
            data_ins  = new MixedDataInputStream( bary_ins );
            treenode  = new TreeNode( data_ins, objdefs );
            data_ins.close();
        } catch ( IOException ioerr ) {
            System.err.println( "InputLog: Non-recoverable IOException! "
                              + "Program continues ..." );
            ioerr.printStackTrace();
            treenode  = null;
        }

        return treenode;
    }

    public void close()
    {
        try {
            rand_file.close();
        } catch ( IOException ioerr ) {
            System.err.println( "InputLog: Non-recoverable IOException! "
                              + "Exiting ..." );
            ioerr.printStackTrace();
            System.exit( 1 );
        }
    }

    protected void finalize() throws Throwable
    {
        try {
            close();
        } finally {
            super.finalize();
        }
    }

    public String toString()
    {
        StringBuffer rep = new StringBuffer();
        rep.append( filehdr.toString() + "\n" );
        rep.append( objdefs.toString() + "\n" );
        rep.append( treedir.toString() + "\n" );
        rep.append( lineIDmaps.toString() + "\n" );
        return rep.toString();
    }

    // isForeItr=true  : Drawables returned in increasing starttime major order
    // isForeItr=false : Drawables returned in decreasing starttime major order
    public Iterator iteratorOfRealDrawables( TimeBoundingBox timeframe,
                                             boolean         isForeItr,
                                             int             itrTopoLevel )
    {
        if ( isForeItr )
            return new ForeItrOfAllRealDobjs( timeframe, itrTopoLevel );
        else
            return new BackItrOfAllRealDobjs( timeframe, itrTopoLevel );
    }



    private class ForeItrOfAllRealDobjs extends IteratorOfGroupObjects
    {
        private static final boolean  IS_COMPOSITE = true;
        private static final boolean  IS_FORE_ITR  = true;
        private static final short    LOWEST_DEPTH = 0;

        private int              iterateTopoLevel;

        private TimeBoundingBox  current_timebox;
        private TreeTrunk        treetrunk;
        private List             timebox_list;
        private int              timebox_count;
        private int              timebox_idx;

        private Drawable         next_drawable;

        public ForeItrOfAllRealDobjs( final TimeBoundingBox timeframe,
                                            int             itrTopoLevel )
        {
            super( timeframe );
            iterateTopoLevel = itrTopoLevel;

            TreeNode         treeroot;
            TimeBoundingBox  timebox_root;

            treetrunk    = new TreeTrunk( InputLog.this );
            treetrunk.initFromTreeTop();
            treeroot     = treetrunk.getTreeRoot();
            timebox_root = new TimeBoundingBox( treeroot );
            // System.err.println( "Time Window is " + timebox_root );

            Iterator         entries;
            Map.Entry        entry;
            TreeNodeID       ID;
            TreeDirValue     val;
            TimeBoundingBox  timebox;
            boolean          isFirstLeaf;
 
            timebox_list = new ArrayList();
 
            isFirstLeaf  = true;
            entries      = treedir.entrySet().iterator();
            while ( entries.hasNext() ) {
                entry    = (Map.Entry) entries.next();
                ID       = (TreeNodeID) entry.getKey();
                val      = (TreeDirValue) entry.getValue();
                if ( ID.isLeaf() ) {
                    timebox = new TimeBoundingBox( val.getTimeBoundingBox() );
                    // System.out.print( ID + " -> " + timebox );
                    if ( isFirstLeaf ) {
                        isFirstLeaf = false;
                        timebox.setEarliestTime(
                                timebox_root.getEarliestTime() );
                    }
                    if ( ! entries.hasNext() )  // i.e. isLastLeaf
                        timebox.setLatestTime( timebox_root.getLatestTime() );
                    // System.out.println( " -> " + timebox );
                    timebox_list.add( timebox );
                }
            }

            timebox_count  = timebox_list.size();
            timebox_idx    = 0;
            timebox        = (TimeBoundingBox) timebox_list.get( timebox_idx );
            treetrunk.growInTreeWindow( treeroot, LOWEST_DEPTH, timebox );
            super.setObjGrpItr( this.nextObjGrpItr( timeframe ) );

            next_drawable  = this.getNextInQueue();
        }

        protected Iterator nextObjGrpItr( final TimeBoundingBox tframe )
        {
            TimeBoundingBox  timebox;
            Iterator         nestable_itr, nestless_itr;
            Iterator         dobj_itr;

            while ( timebox_idx < timebox_count ) {
                timebox  = (TimeBoundingBox) timebox_list.get( timebox_idx );
                timebox_idx++;
                current_timebox = timebox.getIntersection( tframe );
                if ( current_timebox != null ) {
                    treetrunk.scrollTimeWindowTo( current_timebox );
                    // System.out.println( "\n(" + (timebox_idx-1) + ") : "
                    //                   + current_timebox );
                    // System.out.println( treetrunk.toStubString() );
                    nestable_itr = null;
                    if (    iterateTopoLevel == ITERATE_ALL
                         || iterateTopoLevel == ITERATE_STATES ) {
                        nestable_itr
                        = treetrunk.iteratorOfRealDrawables( current_timebox,
                                                             IS_COMPOSITE,
                                                             IS_FORE_ITR,
                                                             true );
                    }

                    nestless_itr = null;
                    if (    iterateTopoLevel == ITERATE_ALL
                         || iterateTopoLevel == ITERATE_ARROWS ) {
                        nestless_itr
                        = treetrunk.iteratorOfRealDrawables( current_timebox,
                                                             IS_COMPOSITE,
                                                             IS_FORE_ITR,
                                                             false );
                    }

                    dobj_itr = null;
                    if ( nestable_itr != null && nestless_itr != null )
                        dobj_itr
                        = new IteratorOfForeDrawablesOfAll( nestable_itr,
                                                            nestless_itr );
                    else {
                        if ( nestable_itr != null )
                            dobj_itr = nestable_itr;
                        if ( nestless_itr != null )
                            dobj_itr = nestless_itr;
                    }

                    return dobj_itr;
                }
            }
            // return NULL when all timeboxes have been exhausted
            return null;
        }

        private Drawable getNextInQueue()
        {
            Drawable  dobj;
            while ( super.hasNext() ) {
                dobj = (Drawable) super.next();
                if ( current_timebox.containsWithinLeft(
                                     dobj.getEarliestTime() ) )
                    return dobj;
            }
            return null;
        }

        public boolean hasNext()
        {
            return next_drawable != null;
        }

        public Object next()
        {
            Drawable  returning_dobj;

            returning_dobj  = next_drawable;
            next_drawable   = this.getNextInQueue();
            return returning_dobj;
        }
    }   // End of ForeItrOfAllRealDobjs




    private class BackItrOfAllRealDobjs extends IteratorOfGroupObjects
    {
        private static final boolean  IS_COMPOSITE = true;
        private static final boolean  IS_BACK_ITR  = false;
        private static final short    LOWEST_DEPTH = 0;

        private int              iterateTopoLevel;

        private TimeBoundingBox  current_timebox;
        private TreeTrunk        treetrunk;
        private List             timebox_list;
        private int              timebox_count;
        private int              timebox_idx;

        private Drawable         next_drawable;

        public BackItrOfAllRealDobjs( final TimeBoundingBox timeframe,
                                            int             itrTopoLevel )
        {
            super( timeframe );
            iterateTopoLevel = itrTopoLevel;

            TreeNode         treeroot;
            TimeBoundingBox  timebox_root;

            treetrunk    = new TreeTrunk( InputLog.this );
            treetrunk.initFromTreeTop();
            treeroot     = treetrunk.getTreeRoot();
            timebox_root = new TimeBoundingBox( treeroot );
            // System.err.println( "Time Window is " + timebox_root );

            Iterator         entries;
            Map.Entry        entry;
            TreeNodeID       ID;
            TreeDirValue     val;
            TimeBoundingBox  timebox;
            boolean          isFirstLeaf;
    
            timebox_list = new ArrayList();
    
            isFirstLeaf  = true;
            entries      = treedir.entrySet().iterator();
            while ( entries.hasNext() ) {
                entry    = (Map.Entry) entries.next();
                ID       = (TreeNodeID) entry.getKey();
                val      = (TreeDirValue) entry.getValue();
                if ( ID.isLeaf() ) {
                    timebox = new TimeBoundingBox( val.getTimeBoundingBox() );
                    // System.out.print( ID + " -> " + timebox );
                    if ( isFirstLeaf ) {
                        isFirstLeaf = false;
                        timebox.setEarliestTime(
                                timebox_root.getEarliestTime() );
                    }
                    if ( ! entries.hasNext() )  // i.e. isLastLeaf
                        timebox.setLatestTime( timebox_root.getLatestTime() );
                    // System.out.println( " -> " + timebox );
                    timebox_list.add( timebox );
                }
            }

            timebox_count  = timebox_list.size();
            timebox_idx    = timebox_count - 1;
            timebox        = (TimeBoundingBox) timebox_list.get( timebox_idx );
            treetrunk.growInTreeWindow( treeroot, LOWEST_DEPTH, timebox );
            super.setObjGrpItr( this.nextObjGrpItr( timeframe ) );

            next_drawable  = this.getNextInQueue();
        }

        protected Iterator nextObjGrpItr( final TimeBoundingBox tframe )
        {
            TimeBoundingBox  timebox;
            Iterator         nestable_itr, nestless_itr;
            Iterator         dobj_itr;

            while ( timebox_idx >= 0  ) {
                timebox  = (TimeBoundingBox) timebox_list.get( timebox_idx );
                timebox_idx--;
                current_timebox = timebox.getIntersection( tframe );
                if ( current_timebox != null ) {
                    treetrunk.scrollTimeWindowTo( current_timebox );
                    // System.out.println( "\n(" + (timebox_idx+1) + ") : "
                    //                   + current_timebox );
                    // System.out.println( treetrunk.toStubString() );
                    nestable_itr = null;
                    if (    iterateTopoLevel == ITERATE_ALL
                         || iterateTopoLevel == ITERATE_STATES ) {
                        nestable_itr
                        = treetrunk.iteratorOfRealDrawables( current_timebox,
                                                             IS_COMPOSITE,
                                                             IS_BACK_ITR,
                                                             true );
                    }

                    nestless_itr = null;
                    if (    iterateTopoLevel == ITERATE_ALL
                         || iterateTopoLevel == ITERATE_ARROWS ) {
                        nestless_itr
                        = treetrunk.iteratorOfRealDrawables( current_timebox,
                                                             IS_COMPOSITE,
                                                             IS_BACK_ITR,
                                                             false );
                    }

                    dobj_itr = null;
                    if ( nestable_itr != null && nestless_itr != null )
                        dobj_itr
                        = new IteratorOfForeDrawablesOfAll( nestable_itr,
                                                            nestless_itr );
                    else {
                        if ( nestable_itr != null )
                            dobj_itr = nestable_itr;
                        if ( nestless_itr != null )
                            dobj_itr = nestless_itr;
                    }

                    return dobj_itr;
                }
            }
            // return NULL when all timeboxes have been exhausted
            return null;
        }

        private Drawable getNextInQueue()
        {
            Drawable  dobj;
            while ( super.hasNext() ) {
                dobj = (Drawable) super.next();
                if ( current_timebox.containsWithinLeft(
                                     dobj.getEarliestTime() ) )
                // if ( current_timebox.containsWithinRight(
                //                      dobj.getLatestTime() ) )
                    return dobj;
            }
            return null;
        }

        public boolean hasNext()
        {
            return next_drawable != null;
        }

        public Object next()
        {
            Drawable  returning_dobj;

            returning_dobj  = next_drawable;
            next_drawable   = this.getNextInQueue();
            return returning_dobj;
        }
    }   // End of BackItrOfAllRealDobjs
}
