/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package viewer.common;

import java.awt.*;

public class Routines
{
    private static Cursor normal_cursor = new Cursor( Cursor.DEFAULT_CURSOR );
    private static Cursor wait_cursor   = new Cursor( Cursor.WAIT_CURSOR );

    public static void setAllCursors( Component comp, Cursor csr )
    {
        if ( comp == null ) return;
        comp.setCursor( csr );
        if ( comp instanceof Container ) {
            Component [] comps = ( (Container) comp ).getComponents();
            for ( int ii = 0; ii < comps.length; ii++)
                setAllCursors( comps[ ii ], csr );
        }
    }

    public static void setAllCursorsToNormal( Component comp )
    {
        setAllCursors( comp, normal_cursor );
    }

    public static void setAllCursorsToWait( Component comp )
    {
        setAllCursors( comp, wait_cursor );
    }

    public static void setCursorToNormal( Component comp )
    {
        if ( comp == null ) return;
        comp.setCursor( normal_cursor );
    }

    public static void setCursorToWait( Component comp )
    {
        if ( comp == null ) return;
        comp.setCursor( wait_cursor );
    }

    public static Dimension getScreenSize()
    {
        return Toolkit.getDefaultToolkit().getScreenSize();
    }

    public static Dimension correctSize( Dimension size, final Insets insets )
    {
        if ( insets != null ) {
            size.width  += insets.left + insets.right;
            if ( size.width > Short.MAX_VALUE )
                size.width  = Short.MAX_VALUE;
            size.height += insets.top + insets.bottom;
            if ( size.height > Short.MAX_VALUE )
                size.height = Short.MAX_VALUE;
        }
        return size;
    }
}
