/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package logformat.slog2.input;

import java.io.*;

import base.io.MixedRandomAccessFile;
import base.io.MixedDataInputStream;
import base.io.MixedDataIO;
import logformat.slog2.*;

public class InputLog
{
    private MixedRandomAccessFile  rand_file;
    private ByteArrayInputStream   bary_ins;
    private MixedDataInputStream   data_ins;

    private Header                 filehdr;
    private TreeDir                treedir;
    private CategoryMap            objdefs;
    private LineIDMapList          lineIDmaps;

    private byte[]                 buffer;


    public InputLog( String filename )
    {
        rand_file = null;
        try {
            rand_file = new MixedRandomAccessFile( filename, "r" );
        // } catch ( FileNotFoundException ferr ) {
        } catch ( IOException ferr ) {
            System.err.println( "InputLog: Non-recoverable IOException! "
                              + "Exiting ..." );
            ferr.printStackTrace();
            System.exit( 1 );
        }

        readHeader();

        buffer    = new byte[ filehdr.getMaxBufferByteSize() ];
        bary_ins  = null;
        data_ins  = null;

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
        rep.append( treedir.toString() + "\n" );
        rep.append( objdefs.toString() + "\n" );
        rep.append( lineIDmaps.toString() + "\n" );
        return rep.toString();
    }
}
