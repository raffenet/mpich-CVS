/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package logformat.slog2.input;

import java.util.StringTokenizer;
import java.io.InputStreamReader;
import java.io.BufferedReader;

import base.drawable.*;
import logformat.slog2.*;

public class Navigator
{
    private static boolean            printAll   = false;
    private static boolean            isVerbose  = false;

    private static InputStreamReader  sys_insrdr
                                      = new InputStreamReader( System.in );
    private static BufferedReader     sys_bufrdr
                                      = new BufferedReader( sys_insrdr );

    private static String             in_filename;
    private static short              depth_max, depth;

    public static final void main( String[] args )
    {
        logformat.slog2.input.InputLog slog_ins;
        CategoryMap                    objdefs;
        TreeTrunk                      treetrunk;
        TreeNode                       treeroot;
        TimeBoundingBox                timeframe_root, timeframe_old, timeframe;

        parseCmdLineArgs( args );

        slog_ins   = new logformat.slog2.input.InputLog( in_filename );
        // System.out.println( slog_ins );

        // Initialize the TreeTrunk
        treetrunk       = new TreeTrunk( slog_ins );
        treetrunk.setDebuggingEnabled( isVerbose );
        treetrunk.initFromTreeTop();
        treeroot        = treetrunk.getTreeRoot();
        timeframe_root  = new TimeBoundingBox( treeroot );
        depth_max       = treeroot.getTreeNodeID().depth;
        System.out.println( "TimeWindow = " + timeframe_root
                          + " @ dmax = " + depth_max );
        timeframe_old   = new TimeBoundingBox( timeframe_root );

        // Grow to a fixed size first
        // Init depth before getTimeWindowFromStdin()
        depth           = depth_max; 
        timeframe       = getTimeWindowFromStdin( timeframe_old );
        System.out.println( "TimeWindow = " + timeframe
                          + " @ d = " + depth );
        treetrunk.growInTreeWindow( treeroot, depth, timeframe );
        if ( printAll )
            System.out.println( treetrunk.toString( timeframe ) );
        else
            System.out.println( treetrunk.toStubString() );
        timeframe_old = timeframe;

        // Navigate the slog2 tree
        while (    ( timeframe = getTimeWindowFromStdin( timeframe_old ) )
                != null ) {
            System.out.println( "TimeWindow = " + timeframe
                              + " @ d = " + depth );
            if ( treetrunk.updateTimeWindow( timeframe_old, timeframe ) ) {
                if ( printAll )
                    System.out.println( treetrunk.toString( timeframe ) );
                else
                    System.out.println( treetrunk.toStubString() );
                timeframe_old = timeframe;
            }
        }

        slog_ins.close();
    }

    private static String format_msg = "[d=I]  [[ts=D] [tf=D]]  "
                                     + "[zi[=D]] [zo[=D]] [sf[=F]] [sb[=F]]";

    private static String input_msg = "Interactive Input Options: \n"
                                    + "\t Specification of the Time Frame : \n"
                                    + "\t \t " + format_msg + "\n"
                                    + "\t [d=I], depth=Integer              "
                                    + " Needed only when program starts.\n"
                                    + "\t [ts=D], timeframe_start=Double    "
                                    + " Specify Start of TimeFrame.\n"
                                    + "\t [tf=D], timeframe_final=Double    "
                                    + " Specify Final of TimeFrame.\n"
                                    + "\t [zi[=D]], zoom-in[=Double]        "
                                    + " Specify Center of Zoom-In Frame.\n"
                                    + "\t [zi[=D]], zoom-out[=Double]       "
                                    + " Specify Center of Zoom-Out Frame.\n"
                                    + "\t [sf[=F]], scroll-forward[=Frames] "
                                    + " Specify Frames to Scroll Forward.\n"
                                    + "\t [sb[=F]], scroll-backward[=Frames]"
                                    + " Specify Frames to Scroll Backward.\n";

    private static double zoom_ftr = 2.0d;

    private static TimeBoundingBox getTimeWindow(
                                   final TimeBoundingBox  timeframe_old,
                                   String           argv[] )
    {
        StringBuffer     err_msg   = new StringBuffer();
        TimeBoundingBox  timeframe = new TimeBoundingBox( timeframe_old );
        String        str;
        int           idx = 0;

        try {
            while ( idx < argv.length ) {
                if ( argv[ idx ].indexOf( '=' ) != -1 ) {
                    if ( argv[ idx ].startsWith( "d=" ) ) {
                        str = argv[ idx ].trim().substring( 2 );
                        depth  = Short.parseShort( str );
                        err_msg.append( "\n lowest_depth = " + str );
                        idx++;
                    }
                    else if ( argv[ idx ].startsWith( "ts=" ) ) {
                        str = argv[ idx ].trim().substring( 3 );
                        timeframe.setEarliestTime( Double.parseDouble( str ) );
                        err_msg.append( "\n time_frame_start = " + str );
                        idx++;
                    }
                    else if ( argv[ idx ].startsWith( "tf=" ) ) {
                        str = argv[ idx ].trim().substring( 3 );
                        timeframe.setLatestTime( Double.parseDouble( str ) );
                        err_msg.append( "\n time_frame_final = " + str );
                        idx++;
                    }
                    else if ( argv[ idx ].startsWith( "zi=" ) ) {
                        str = argv[ idx ].trim().substring( 3 );
                        double zoom_ctr = Double.parseDouble( str );
                        double ctr_span = timeframe_old.getDuration() / 2.0d
                                        / zoom_ftr;
                        timeframe.setEarliestTime( zoom_ctr - ctr_span );
                        timeframe.setLatestTime( zoom_ctr + ctr_span );
                        err_msg.append( "\n zoom_in_center = " + str );
                        idx++;
                    }
                    else if ( argv[ idx ].startsWith( "zo=" ) ) {
                        str = argv[ idx ].trim().substring( 3 );
                        double zoom_ctr = Double.parseDouble( str );
                        double ctr_span = timeframe_old.getDuration() / 2.0d
                                        * zoom_ftr;
                        timeframe.setEarliestTime( zoom_ctr - ctr_span );
                        timeframe.setLatestTime( zoom_ctr + ctr_span );
                        err_msg.append( "\n zoom_out_center = " + str );
                        idx++;
                    }
                    else if ( argv[ idx ].startsWith( "sf=" ) ) {
                        str = argv[ idx ].trim().substring( 3 );
                        double Nwins = Double.parseDouble( str );
                        double win_span = timeframe_old.getDuration();
                        timeframe.setEarliestTime( Nwins * win_span
                                 + timeframe_old.getEarliestTime() );
                        timeframe.setLatestTime( win_span
                                 + timeframe.getEarliestTime() );
                        err_msg.append( "\n scroll_forward = " + str
                                      + " frames" );
                        idx++;
                    }
                    else if ( argv[ idx ].startsWith( "sb=" ) ) {
                        str = argv[ idx ].trim().substring( 3 );
                        double Nwins = Double.parseDouble( str );
                        double win_span = timeframe_old.getDuration();
                        timeframe.setEarliestTime( - Nwins * win_span
                                 + timeframe_old.getEarliestTime() );
                        timeframe.setLatestTime( win_span
                                 + timeframe.getEarliestTime() );
                        err_msg.append( "\n scroll_forward = " + str
                                      + " frames" );
                        idx++;
                    }
                    else {
                        System.err.println( "Unrecognized option, "
                                          + argv[ idx ] + ", at "
                                          + indexOrderStr( idx+1 )
                                          + " input argument" );
                        System.err.flush();
                        return null;
                    }
                }   // if ( argv[ idx ].indexOf( '=' ) != -1 )
                else {   // if ( argv[ idx ].indexOf( '=' ) == -1 )
                    if ( argv[ idx ].startsWith( "zi" ) ) {
                        double zoom_ctr = ( timeframe_old.getEarliestTime()
                                          + timeframe_old.getLatestTime() )
                                          / 2.0d;
                        double ctr_span = timeframe_old.getDuration() / 2.0d
                                        / zoom_ftr;
                        timeframe.setEarliestTime( zoom_ctr - ctr_span );
                        timeframe.setLatestTime( zoom_ctr + ctr_span );
                        err_msg.append( "\n zoom_in_center = " + zoom_ctr );
                        idx++;
                    }
                    else if ( argv[ idx ].startsWith( "zo" ) ) {
                        double zoom_ctr = ( timeframe_old.getEarliestTime()
                                          + timeframe_old.getLatestTime() )
                                          / 2.0d;
                        double ctr_span = timeframe_old.getDuration() / 2.0d
                                        * zoom_ftr;
                        timeframe.setEarliestTime( zoom_ctr - ctr_span );
                        timeframe.setLatestTime( zoom_ctr + ctr_span );
                        err_msg.append( "\n zoom_out_center = " + zoom_ctr );
                        idx++;
                    }
                    else if ( argv[ idx ].startsWith( "sf" ) ) {
                        double Nwins = 1.0d;
                        double win_span = timeframe_old.getDuration();
                        timeframe.setEarliestTime( Nwins * win_span
                                 + timeframe_old.getEarliestTime() );
                        timeframe.setLatestTime( win_span
                                 + timeframe.getEarliestTime() );
                        err_msg.append( "\n scroll_forward = " + Nwins
                                      + " frames" );
                        idx++;
                    }
                    else if ( argv[ idx ].startsWith( "sb" ) ) {
                        double Nwins = 1.0d;
                        double win_span = timeframe_old.getDuration();
                        timeframe.setEarliestTime( - Nwins * win_span
                                 + timeframe_old.getEarliestTime() );
                        timeframe.setLatestTime( win_span
                                 + timeframe.getEarliestTime() );
                        err_msg.append( "\n scroll_forward = " + Nwins
                                      + " frames" );
                        idx++;
                    }
                    else {
                        System.err.println( "Unrecognized option, "
                                          + argv[ idx ] + ", at "
                                          + indexOrderStr( idx+1 )
                                          + " input argument" );
                        System.err.flush();
                        return null;
                    }
                }
            }
        } catch ( NumberFormatException numerr ) {
            if ( err_msg.length() > 0 )
                System.err.println( err_msg.toString() );
            String idx_order_str = indexOrderStr( idx );
            System.err.println( "Error occurs after option "
                              + argv[ idx-1 ] + ", " + indexOrderStr( idx )
                              + " input argument.  It needs a number." );
            // System.err.println( help_msg );
            numerr.printStackTrace();
            return null;
        }

        //  Checking if the input value is valid
        if ( depth >= 0 && depth <= depth_max )
            return timeframe;
        else {
            System.err.println( "Invalid TimeWindow!" );
            return null;
        }
    }
    
    private static TimeBoundingBox getTimeWindowFromStdin(
                                   final TimeBoundingBox timeframe_old )
    {
        TimeBoundingBox  timeframe;
        String[]         input_args;
        StringTokenizer  tokens;

        do {
            System.out.print( "Enter TimeWindow: " + format_msg + " ?\n" );
            String  input_str;
            try {
                input_str = sys_bufrdr.readLine();
            } catch ( java.io.IOException ioerr ) {
                ioerr.printStackTrace();
                return null;
            }
            tokens      = new StringTokenizer( input_str );
            input_args  = new String[ tokens.countTokens() ];
            for ( int idx = 0; tokens.hasMoreTokens(); idx++ )
                input_args[ idx ] = tokens.nextToken();    
        } while (    ( timeframe = getTimeWindow( timeframe_old, input_args ) )
                  == null );
        return timeframe;
    }

    private static String stub_msg  = "Format of the TreeNodeStub: \n  " +
"{ TreeNodeID, TreeNodeEndtimes, BlockSize + FilePointer, NumOfDrawables .. }\n"
                                    + "\t TreeNodeID : ID( d, i )           "
                                    + " d is the depth of the TreeNode.\n"
                                    + "\t                                   "
                                    + " d = 0, for LeafNode; \n"
                                    + "\t                                   "
                                    + " d = d_max, for RootNode.\n"
                                    + "\t                                   "
                                    + " i is TreeNode's index(same depth).\n"
                                    + "\t TreeNodeEndtimes                  "
                                    + " TreeNode's Start and End times.\n";

    private static String help_msg  = "Usage: java slog2.input.Navigator "
                                    + "[options] slog2_filename.\n"
                                    + "Options: \n"
                                    + "\t [-h|-help|--help]                 "
                                    + " Display this message.\n"
                                    + "\t [-s|-stub]                        "
                                    + " Print TreeNode's stub (Default).\n"
                                    + "\t [-a|-all]                         "
                                    + " Print TreeNode's drawable content.\n"
                                    + "\t [-v|-verbose]                     "
                                    + " Print detailed diagnostic message.\n"
                                    + "\n" + input_msg
                                    + "\n" + stub_msg;

    private static void parseCmdLineArgs( String argv[] )
    {
        String        arg_str;
        StringBuffer  err_msg = new StringBuffer();
        int           idx = 0;

            while ( idx < argv.length ) {
                if ( argv[ idx ].startsWith( "-" ) ) {
                    if (  argv[ idx ].equals( "-h" )
                       || argv[ idx ].equals( "-help" )
                       || argv[ idx ].equals( "--help" ) ) {
                        System.out.println( help_msg );
                        System.out.flush();
                        System.exit( 0 );
                    }
                    else if (  argv[ idx ].equals( "-s" )
                            || argv[ idx ].equals( "-stub" ) ) {
                         printAll = false;
                         idx++;
                    }
                    else if (  argv[ idx ].equals( "-a" )
                            || argv[ idx ].equals( "-all" ) ) {
                         printAll = true;
                         idx++;
                    }
                    else if (  argv[ idx ].equals( "-v" )
                            || argv[ idx ].equals( "-verbose" ) ) {
                         isVerbose = true;
                         idx++;
                    }
                    else {
                        System.err.println( "Unrecognized option, "
                                          + argv[ idx ] + ", at "
                                          + indexOrderStr( idx+1 )
                                          + " command line argument" );
                        System.out.flush();
                        System.exit( 1 );
                    }
                }
                else {
                    in_filename   = argv[ idx ];
                    idx++;
                }
            }

        if ( in_filename == null ) {
            System.err.println( "The Program needs a SLOG-2 filename as "
                              + "a command line argument." );
            System.err.println( help_msg );
            System.exit( 1 );
        }
    }

    private static String indexOrderStr( int idx )
    {
        switch (idx) {
            case 1  : return Integer.toString( idx ) + "st";
            case 2  : return Integer.toString( idx ) + "nd";
            case 3  : return Integer.toString( idx ) + "rd";
            default : return Integer.toString( idx ) + "th";
        }
    }
}
