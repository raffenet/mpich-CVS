/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package logformat.trace;

import base.drawable.*;

/*
   This class provides the Java version of TRACE-API.
*/
public class InputLog // implements drawable.InputAPI
{
    private String    filespec;
    private long      filehandle;

    private int       num_topology_returned;

    public InputLog( String spec_str )
    {
        boolean isOK;
        filespec = spec_str;
        isOK = this.open();        // set filehandle
        if ( filehandle == 0 ) {
            if ( isOK ) {
                System.out.println( "trace.InputLog.open() exits normally!" );
                System.exit( 0 );
            }
            else {
                System.err.println( "trace.InputLog.open() fails!\n"
                      + "No slog2 file is generated due to previous errors." );
                System.exit( 1 );
            }
        }

        // Initialize Topology name return counter
        num_topology_returned = 0;
    }

    private native static void initIDs();

    public  native boolean    open();

    public  native boolean    close();

    public  native int        peekNextKindIndex();

    public  native Category   getNextCategory();

    public  native YCoordMap  getNextYCoordMap();

    public  native Primitive  getNextPrimitive();

    public  native Composite  getNextComposite();

    static {
        initIDs();
    }

    public Kind  peekNextKind()
    {
        // Return all the Topology names.
        if ( num_topology_returned < 3 )
            return Kind.TOPOLOGY;

        int next_kind_index  = this.peekNextKindIndex();
        switch ( next_kind_index ) {
            case Kind.TOPOLOGY_ID :
                return Kind.TOPOLOGY;
            case Kind.EOF_ID :
                return Kind.EOF;
            case Kind.PRIMITIVE_ID :
                return Kind.PRIMITIVE;
            case Kind.COMPOSITE_ID :
                return Kind.COMPOSITE;
            case Kind.CATEGORY_ID :
                return Kind.CATEGORY;
            case Kind.YCOORDMAP_ID :
                return Kind.YCOORDMAP;
            default :
                System.err.println( "trace.InputLog.peekNextKind(): "
                                  + "Unknown value, " + next_kind_index );
        }
        return null;
    }

    public Topology getNextTopology()
    {
        switch ( num_topology_returned ) {
            case 0:
                num_topology_returned = 1;
                return new Topology( Topology.EVENT_ID );
            case 1:
                num_topology_returned = 2;
                return new Topology( Topology.STATE_ID );
            case 2:
                num_topology_returned = 3;
                return new Topology( Topology.ARROW_ID );
            default:
                System.err.println( "All Topology Names have been returned" );
        }
        return null;
    }

    public Category  getShadowCategoryForTopology( final Topology aTopo )
    {
        Category    type;
        ColorAlpha  white_color = new ColorAlpha( 255, 255, 255 );
        if ( aTopo.isEvent() ) {
            type = new Category( -1, "Shadow_" + aTopo.toString(),
                                 aTopo, white_color, 5 );
            /*
            type.setInfoKeys( "num_real_objs=%d\ntime_error=%E\n" );
            */
            return type;
        }
        else if ( aTopo.isArrow() ) {
            type = new Category( -2, "Shadow_" + aTopo.toString(),
                                 aTopo, white_color, 5 );
            /*
            type.setInfoKeys( "num_real_objs=%d\ntime_error=%E\nmsg_size=%d\n" );
            */
            return type;
        }
        else if ( aTopo.isState() ) {
            type = new Category( -3, "Shadow_" + aTopo.toString(),
                                 aTopo, white_color, 5 );
            /*
            type.setInfoKeys( "num_real_objs=%d\ntime_error=%E\n" );
            */
            return type;
        }
        return null;
    }

}
